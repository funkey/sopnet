#include <boost/range/adaptors.hpp>
#include <vigra/multi_distance.hxx>

#include <inference/LinearConstraints.h>
#include <inference/LinearObjective.h>
#include <inference/LinearSolver.h>
#include <pipeline/Value.h>
#include <util/exceptions.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "TolerantEditDistance.h"

logger::LogChannel tedlog("tedlog", "[TolerantEditDistance] ");

util::ProgramOption optionToleranceDistanceThreshold(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "toleranceDistanceThreshold",
		util::_description_text = "The maximum allowed distance for a boundary shift in nm.",
		util::_default_value    = 100);

TolerantEditDistance::TolerantEditDistance() :
	_maxDistanceThreshold(optionToleranceDistanceThreshold) {

	registerInput(_groundTruth, "ground truth");
	registerInput(_reconstruction, "reconstruction");

	registerOutput(_correctedReconstruction, "corrected reconstruction");
	registerOutput(_errorLocations, "error locations");
	registerOutput(_errors, "errors");
}

void
TolerantEditDistance::updateOutputs() {

	clear();

	extractCells();

	enumerateCellLabels();

	findBestCellLabels();

	findErrors();

	correctReconstruction();
}

void
TolerantEditDistance::extractCells() {

	if (_groundTruth->size() != _reconstruction->size())
		BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("ground truth and reconstruction have different size"));

	if (_groundTruth->height() != _reconstruction->height() || _groundTruth->width() != _reconstruction->width())
		BOOST_THROW_EXCEPTION(SizeMismatchError() << error_message("ground truth and reconstruction have different size"));

	_depth  = _groundTruth->size();
	_width  = _groundTruth->width();
	_height = _groundTruth->height();

	LOG_ALL(tedlog) << "extracting cells in " << _width << "x" << _height << "x" << _depth << " volume" << std::endl;

	for (unsigned int z = 0; z < _groundTruth->size(); z++) {

		boost::shared_ptr<Image> gt  = (*_groundTruth)[z];
		boost::shared_ptr<Image> rec = (*_reconstruction)[z];

		for (unsigned int x = 0; x < gt->width(); x++)
			for (unsigned int y = 0; y < gt->width(); y++) {

				float gtLabel  = (*gt)(x, y);
				float recLabel = (*rec)(x, y);

				// TODO: At the moment, cells are unique wrt (gtLabel,recLabel).
				// Later, we will have multiple cells per labeling. Change this 
				// function accordingly.
				unsigned int cellIndex = getCellIndex(gtLabel, recLabel);

				_cells[cellIndex].add(cell_t::Location(x, y, z));
				_cells[cellIndex].setReconstructionLabel(recLabel);
				_cells[cellIndex].setGroundTruthLabel(gtLabel);

				registerPossibleMatch(gtLabel, recLabel);
			}
	}

	LOG_ALL(tedlog)
			<< "found "
			<< _groundTruthLabels.size()
			<< " ground truth labels and "
			<< _reconstructionLabels.size()
			<< " reconstruction labels"
			<< std::endl;
}

void
TolerantEditDistance::enumerateCellLabels() {

	// get the maximal closest distance of any cell to any reconstruction label

	vigra::Shape3 shape(_width, _height, _depth);
	vigra::MultiArray<3, float> distance(shape);

	// TODO: read from program options
	float pitch[3];
	pitch[0] = 1.0;
	pitch[1] = 1.0;
	pitch[2] = 10.0;

	// for each reconstruction label
	foreach (float recLabel, getReconstructionLabels()) {

		LOG_ALL(tedlog) << "create distance map for reconstruction label " << recLabel << std::endl;

		// create distance map
		distance = 0.0f;
		foreach (std::vector<unsigned int>& cellIndices, _cellsByRecToGtLabel[recLabel] | boost::adaptors::map_values)
			foreach (unsigned int cellIndex, cellIndices)
				foreach (const cell_t::Location& l, _cells[cellIndex])
					distance(l.x, l.y, l.z) = 1.0f;
		vigra::separableMultiDistSquared(
				distance,
				distance,
				true /* background */,
				pitch);

		LOG_ALL(tedlog) << "get all cells within " << _maxDistanceThreshold << "nm..." << std::endl;

		// for each cell that does not have the current reconstruction label
		foreach (cell_t& cell, _cells)
			if (cell.getReconstructionLabel() != recLabel) {

				// get the max closest distance to current reconstruction 
				// label
				float maxDistance = 0;
				foreach (const cell_t::Location& l, cell)
					if (distance(l.x, l.y, l.z) > maxDistance)
						maxDistance = distance(l.x, l.y, l.z);

				// if maximum distance map value < threshold, this cell can 
				// have the current reconstruction label as an alternative
				if (maxDistance < _maxDistanceThreshold) {

					cell.addAlternativeLabel(recLabel);
					registerPossibleMatch(cell.getGroundTruthLabel(), recLabel);
				}
			}
	}
}

void
TolerantEditDistance::findBestCellLabels() {

	pipeline::Value<LinearConstraints>      constraints;
	pipeline::Value<LinearSolverParameters> parameters;

	// the default are binary variables
	parameters->setVariableType(Binary);

	// introduce indicators for each cell and each possible label of that cell
	unsigned int var = 0;
	foreach (float recLabel, getReconstructionLabels())
		foreach (std::vector<unsigned int>& cellIndices, _cellsByRecToGtLabel[recLabel] | boost::adaptors::map_values)
			foreach (unsigned int cellIndex, cellIndices) {

				cell_t& cell = _cells[cellIndex];

				// first indicator variable for this cell
				unsigned int begin = var;

				// one variable for the default label
				LOG_ALL(tedlog) << "add indicator for default label of current cell: " << std::endl;
				assignIndicatorVariable(var++, cellIndex, cell.getGroundTruthLabel(), cell.getReconstructionLabel());

				LOG_ALL(tedlog) << "add indicators for alternative labels of current cell: " << std::endl;
				foreach (float l, cell.getAlternativeLabels())
					assignIndicatorVariable(var++, cellIndex, cell.getGroundTruthLabel(), l);

				// last +1 indicator variable for this cell
				unsigned int end = var;

				// every cell needs to have a label
				LinearConstraint constraint;
				for (unsigned int i = begin; i < end; i++)
					constraint.setCoefficient(i, 1.0);
				constraint.setRelation(Equal);
				constraint.setValue(1);
				constraints->add(constraint);
			}
	_numIndicatorVars = var;

	// labels can not disappear
	foreach (float recLabel, getReconstructionLabels()) {

		LinearConstraint constraint;
		foreach (unsigned int v, getIndicatorsByRec(recLabel))
			constraint.setCoefficient(v, 1.0);
		constraint.setRelation(GreaterEqual);
		constraint.setValue(1);
		constraints->add(constraint);
	}

	// introduce indicators for each match of ground truth label to 
	// reconstruction label
	foreach (float gtLabel, getGroundTruthLabels())
		foreach (float recLabel, getPossibleMatchesByGt(gtLabel))
			assignMatchVariable(var++, gtLabel, recLabel);

	// cell label selection activates match
	foreach (float gtLabel, getGroundTruthLabels()) {
		foreach (float recLabel, getPossibleMatchesByGt(gtLabel)) {

			unsigned int matchVar = getMatchVariable(gtLabel, recLabel);

			// no assignment of gtLabel to recLabel -> match is zero
			LinearConstraint noMatchConstraint;

			foreach (unsigned int v, getIndicatorsGtToRec(gtLabel, recLabel)) {

				noMatchConstraint.setCoefficient(v, 1);

				// at least one assignment of gtLabel to recLabel -> match is 
				// one
				LinearConstraint matchConstraint;
				matchConstraint.setCoefficient(matchVar, 1);
				matchConstraint.setCoefficient(v, -1);
				matchConstraint.setRelation(GreaterEqual);
				matchConstraint.setValue(0);
				constraints->add(matchConstraint);
			}

			noMatchConstraint.setCoefficient(matchVar, -1);
			noMatchConstraint.setRelation(GreaterEqual);
			noMatchConstraint.setValue(0);
			constraints->add(noMatchConstraint);
		}
	}

	// introduce split number for each ground truth label

	unsigned int splitBegin = var;

	foreach (float gtLabel, getGroundTruthLabels()) {

		unsigned int splitVar = var++;

		LOG_ALL(tedlog) << "variable " << splitVar << " counts the number of splits for ground truth label " << gtLabel << std::endl;

		parameters->setVariableType(splitVar, Integer);

		LinearConstraint positive;
		positive.setCoefficient(splitVar, 1);
		positive.setRelation(GreaterEqual);
		positive.setValue(0);
		constraints->add(positive);

		LinearConstraint numSplits;
		numSplits.setCoefficient(splitVar, 1);
		foreach (float recLabel, getPossibleMatchesByGt(gtLabel))
			numSplits.setCoefficient(getMatchVariable(gtLabel, recLabel), -1);
		numSplits.setRelation(Equal);
		numSplits.setValue(-1);
		constraints->add(numSplits);
	}

	unsigned int splitEnd = var;

	// introduce total split number

	_splits = var++;
	parameters->setVariableType(_splits, Integer);

	LinearConstraint sumOfSplits;
	sumOfSplits.setCoefficient(_splits, 1);
	for (unsigned int i = splitBegin; i < splitEnd; i++)
		sumOfSplits.setCoefficient(i, -1);
	sumOfSplits.setRelation(Equal);
	sumOfSplits.setValue(0);
	constraints->add(sumOfSplits);

	// introduce merge number for each reconstruction label

	unsigned int mergeBegin = var;

	foreach (float recLabel, getReconstructionLabels()) {

		unsigned int mergeVar = var++;

		LOG_ALL(tedlog) << "variable " << mergeVar << " counts the number of merges for reconstruction label " << recLabel << std::endl;

		parameters->setVariableType(mergeVar, Integer);

		LinearConstraint positive;
		positive.setCoefficient(mergeVar, 1);
		positive.setRelation(GreaterEqual);
		positive.setValue(0);
		constraints->add(positive);

		LinearConstraint numMerges;
		numMerges.setCoefficient(mergeVar, 1);
		foreach (float gtLabel, getPossibleMathesByRec(recLabel))
			numMerges.setCoefficient(getMatchVariable(gtLabel, recLabel), -1);
		numMerges.setRelation(Equal);
		numMerges.setValue(-1);
		constraints->add(numMerges);
	}

	unsigned int mergeEnd = var;

	// introduce total merge number

	_merges = var++;
	parameters->setVariableType(_merges, Integer);

	LinearConstraint sumOfMerges;
	sumOfMerges.setCoefficient(_merges, 1);
	for (unsigned int i = mergeBegin; i < mergeEnd; i++)
		sumOfMerges.setCoefficient(i, -1);
	sumOfMerges.setRelation(Equal);
	sumOfMerges.setValue(0);
	constraints->add(sumOfMerges);

	LOG_ALL(tedlog) << "final constraints are: " << std::endl;
	foreach (LinearConstraint& c, *constraints)
		LOG_ALL(tedlog) << "\t" << c << std::endl;

	// create objective

	pipeline::Value<LinearObjective> objective(var);

	objective->setCoefficient(_splits, 1);
	objective->setCoefficient(_merges, 1);
	objective->setSense(Minimize);

	// solve

	pipeline::Process<LinearSolver> solver;

	solver->setInput("objective", objective);
	solver->setInput("linear constraints", constraints);
	solver->setInput("parameters", parameters);

	_solution = solver->getOutput("solution");
}

void
TolerantEditDistance::findErrors() {

	for (unsigned int i = 0; i < _numIndicatorVars; i++) {

		if ((*_solution)[i]) {

			unsigned int cellIndex = _labelingByVar[i].first;
			float        recLabel  = _labelingByVar[i].second;
			cell_t&      cell      = _cells[cellIndex];

			_errors->addMapping(cell.getGroundTruthLabel(), recLabel, cell.size());
		}
	}

	LOG_DEBUG(tedlog) << "num splits: " << (*_solution)[_splits] << std::endl;
	LOG_DEBUG(tedlog) << "num merges: " << (*_solution)[_merges] << std::endl;

	LOG_ALL(tedlog) << "error counts from Errors data structure:" << std::endl;
	LOG_ALL(tedlog) << "num splits: " << _errors->getNumSplits() << std::endl;
	LOG_ALL(tedlog) << "num merges: " << _errors->getNumMerges() << std::endl;
}

void
TolerantEditDistance::correctReconstruction() {

	// prepare output image

	for (unsigned int i = 0; i < _depth; i++) {

		boost::shared_ptr<std::vector<float> > data = boost::make_shared<std::vector<float> >(_width*_height, 0.0);
		_correctedReconstruction->add(boost::make_shared<Image>(_width, _height, data));
	}

	// read solution

	for (unsigned int i = 0; i < _numIndicatorVars; i++) {

		if ((*_solution)[i]) {

			unsigned int cellIndex = _labelingByVar[i].first;
			float        recLabel  = _labelingByVar[i].second;
			cell_t&      cell      = _cells[cellIndex];

			foreach (const cell_t::Location& l, cell)
				(*(*_correctedReconstruction)[l.z])(l.x, l.y) = recLabel;
		}
	}
}

std::set<float>&
TolerantEditDistance::getReconstructionLabels() {

	return _reconstructionLabels;
}

std::set<float>&
TolerantEditDistance::getGroundTruthLabels() {

	return _groundTruthLabels;
}

void
TolerantEditDistance::clear() {

	_cells.clear();
	_cellsByRecToGtLabel.clear();
	_indicatorVarsByRecLabel.clear();
	_indicatorVarsByGtToRecLabel.clear();
	_labelingByVar.clear();
	_possibleGroundTruthMatches.clear();
	_groundTruthLabels.clear();
	_reconstructionLabels.clear();
	_errors->clear();

	_correctedReconstruction->clear();
}

unsigned int
TolerantEditDistance::getCellIndex(float gtLabel, float recLabel) {

	if (
			_cellsByRecToGtLabel.count(recLabel) == 0 ||
			_cellsByRecToGtLabel[recLabel].count(gtLabel) == 0) {

		// create a new cell
		_cells.push_back(cell_t());
		_cellsByRecToGtLabel[recLabel][gtLabel].push_back(static_cast<unsigned int>(_cells.size() - 1));
	}

	// NOTE: For now we know there is only one.
	return _cellsByRecToGtLabel[recLabel][gtLabel][0];
}

void
TolerantEditDistance::registerPossibleMatch(float gtLabel, float recLabel) {

	_possibleGroundTruthMatches[gtLabel].insert(recLabel);
	_possibleReconstructionMatches[recLabel].insert(gtLabel);
	_groundTruthLabels.insert(gtLabel);
	_reconstructionLabels.insert(recLabel);
}

std::set<float>&
TolerantEditDistance::getPossibleMatchesByGt(float gtLabel) {

	return _possibleGroundTruthMatches[gtLabel];
}

std::set<float>&
TolerantEditDistance::getPossibleMathesByRec(float recLabel) {

	return _possibleReconstructionMatches[recLabel];
}

void
TolerantEditDistance::assignIndicatorVariable(unsigned int var, unsigned int cellIndex, float gtLabel, float recLabel) {

	LOG_ALL(tedlog) << "variable " << var << " indicates a single mapping from " << gtLabel << " to " << recLabel << std::endl;

	_indicatorVarsByRecLabel[recLabel].push_back(var);
	_indicatorVarsByGtToRecLabel[gtLabel][recLabel].push_back(var);

	_labelingByVar[var] = std::make_pair(cellIndex, recLabel);
}

std::vector<unsigned int>&
TolerantEditDistance::getIndicatorsByRec(float recLabel) {

	return _indicatorVarsByRecLabel[recLabel];
}

std::vector<unsigned int>&
TolerantEditDistance::getIndicatorsGtToRec(float gtLabel, float recLabel) {

	return _indicatorVarsByGtToRecLabel[gtLabel][recLabel];
}

void
TolerantEditDistance::assignMatchVariable(unsigned int var, float gtLabel, float recLabel) {

	LOG_ALL(tedlog) << "variable " << var << " indicates a match of " << gtLabel << " to " << recLabel << std::endl;

	_matchVars[gtLabel][recLabel] = var;
}

unsigned int
TolerantEditDistance::getMatchVariable(float gtLabel, float recLabel) {

	return _matchVars[gtLabel][recLabel];
}
