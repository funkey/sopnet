#include <algorithm>

#include <boost/range/adaptors.hpp>
#include <boost/tuple/tuple.hpp>
#include <vigra/multi_distance.hxx>
#include <vigra/multi_labeling.hxx>
#include <vigra/impex.hxx>

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

util::ProgramOption optionHaveBackgroundLabel(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "haveBackgroundLabel",
		util::_description_text = "Indicates that there is a background label with a default value of 0.",
		util::_default_value    = false);

util::ProgramOption optionGroundTruthBackgroundLabel(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "groundTruthBackgroundLabel",
		util::_description_text = "The value of the ground-truth background label.",
		util::_default_value    = 0.0);

util::ProgramOption optionReconstructionBackgroundLabel(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "reconstructionBackgroundLabel",
		util::_description_text = "The value of the reconstruction background label.",
		util::_default_value    = 0.0);

TolerantEditDistance::TolerantEditDistance() :
	_haveBackgroundLabel(optionHaveBackgroundLabel),
	_gtBackgroundLabel(optionGroundTruthBackgroundLabel),
	_recBackgroundLabel(optionReconstructionBackgroundLabel),
	_errors(_haveBackgroundLabel ? boost::make_shared<Errors>(_gtBackgroundLabel, _recBackgroundLabel) : boost::make_shared<Errors>()),
	_cells(boost::make_shared<std::vector<cell_t> >()),
	_maxDistanceThreshold(optionToleranceDistanceThreshold) {

	if (optionHaveBackgroundLabel) {
		LOG_ALL(tedlog) << "started TolerantEditDistance with background label" << std::endl;
	} else {
		LOG_ALL(tedlog) << "started TolerantEditDistance without background label" << std::endl;
	}

	registerInput(_groundTruth, "ground truth");
	registerInput(_reconstruction, "reconstruction");

	registerOutput(_correctedReconstruction, "corrected reconstruction");
	registerOutput(_splitLocations, "splits");
	registerOutput(_mergeLocations, "merges");
	registerOutput(_fpLocations, "false positives");
	registerOutput(_fnLocations, "false negatives");
	registerOutput(_errors, "errors");
}

void
TolerantEditDistance::updateOutputs() {

	clear();

	extractCells();

	enumerateCellLabels();

	findBestCellLabels();

	correctReconstruction();

	findErrors();
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

	vigra::MultiArray<3, std::pair<float, float> > gtAndRec(vigra::Shape3(_width, _height, _depth));
	vigra::MultiArray<3, unsigned int>             cellIds(vigra::Shape3(_width, _height, _depth));

	// prepare gt and rec image

	for (unsigned int z = 0; z < _depth; z++) {

		boost::shared_ptr<Image> gt  = (*_groundTruth)[z];
		boost::shared_ptr<Image> rec = (*_reconstruction)[z];

		for (unsigned int x = 0; x < _width; x++)
			for (unsigned int y = 0; y < _height; y++) {

				float gtLabel  = (*gt)(x, y);
				float recLabel = (*rec)(x, y);

				gtAndRec(x, y, z) = std::make_pair(gtLabel, recLabel);
			}
	}

	// find connected components in gt and rec image

	cellIds = 0;
	unsigned int numCells = vigra::labelMultiArray(gtAndRec, cellIds);

	LOG_DEBUG(tedlog) << "found " << numCells << " cells" << std::endl;

	// create a cell for each found connected component

	_cells->resize(numCells);
	std::set<unsigned int> foundCells;
	for (unsigned int z = 0; z < _depth; z++) {

		boost::shared_ptr<Image> gt  = (*_groundTruth)[z];
		boost::shared_ptr<Image> rec = (*_reconstruction)[z];

		for (unsigned int x = 0; x < _width; x++)
			for (unsigned int y = 0; y < _height; y++) {

				float gtLabel  = (*gt)(x, y);
				float recLabel = (*rec)(x, y);

				// argh, vigra starts counting at 1!
				unsigned int cellIndex = cellIds(x, y, z) - 1;

				(*_cells)[cellIndex].add(cell_t::Location(x, y, z));
				(*_cells)[cellIndex].setReconstructionLabel(recLabel);
				(*_cells)[cellIndex].setGroundTruthLabel(gtLabel);

				if (foundCells.count(cellIndex) == 0) {

					registerCell(gtLabel, recLabel, cellIndex);
					registerPossibleMatch(gtLabel, recLabel);
					foundCells.insert(cellIndex);
				}
			}
	}

	foreach (unsigned int cellIndex, foundCells)
		LOG_ALL(tedlog)
				<< "cell " << cellIndex
				<< ": size = " << (*_cells)[cellIndex].size()
				<< ", gt_label = " << (*_cells)[cellIndex].getGroundTruthLabel()
				<< ", rec_label = " << (*_cells)[cellIndex].getReconstructionLabel()
				<< std::endl;

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

	// TODO: read from program options
	_resolutionX = 4.0;
	_resolutionY = 4.0;
	_resolutionZ = 40.0;

	_maxDistanceThresholdX = std::min((float)_width,  _maxDistanceThreshold/_resolutionX);
	_maxDistanceThresholdY = std::min((float)_height, _maxDistanceThreshold/_resolutionY);
	_maxDistanceThresholdZ = std::min((float)_depth,  _maxDistanceThreshold/_resolutionZ);

	LOG_DEBUG(tedlog) << "getting relabel candidates" << std::endl;

	std::vector<unsigned int> relabelCandidates = getRelabelCandidates();

	LOG_DEBUG(tedlog) << "there are " << relabelCandidates.size() << " cells that can be relabeled" << std::endl;

	if (relabelCandidates.size() == 0)
		return;

	LOG_DEBUG(tedlog) << "creating distance threshold neighborhood" << std::endl;

	// list of all location offsets within threshold distance
	std::vector<cell_t::Location> neighborhood = createNeighborhood();

	LOG_DEBUG(tedlog) << "there are " << neighborhood.size() << " pixels in the neighborhood for a threshold of " << _maxDistanceThreshold << std::endl;

	// for each cell
	foreach (unsigned int index, relabelCandidates) {

		cell_t& cell = (*_cells)[index];

		LOG_ALL(tedlog) << "processing cell " << index << std::endl;

		std::set<float> alternativeLabels = getAlternativeLabels(cell, neighborhood);

		// for each alternative label
		foreach (float recLabel, alternativeLabels) {

			// register possible match
			cell.addAlternativeLabel(recLabel);
			registerPossibleMatch(cell.getGroundTruthLabel(), recLabel);
		}

		// DEBUG
		index++;
	}
}

std::vector<unsigned int>
TolerantEditDistance::getRelabelCandidates() {

	std::vector<unsigned int> candidates;

	// get the closest distance of any pixel to any label boundary

	vigra::Shape3 shape(_width, _height, _depth);
	vigra::MultiArray<3, float> boundaryDistance2(shape);

	float pitch[3];
	pitch[0] = _resolutionX;
	pitch[1] = _resolutionY;
	pitch[2] = _resolutionZ;

	// create boundary map
	LOG_DEBUG(tedlog) << "creating boundary map of size " << shape << std::endl;
	boundaryDistance2 = 0.0f;
	for (unsigned int x = 0; x < _width; x++)
		for (unsigned int y = 0; y < _height; y++)
			for (unsigned int z = 0; z < _depth; z++)
				if (isBoundaryVoxel(x, y, z, *_reconstruction))
					boundaryDistance2(x, y, z) = 1.0f;

	vigra::MultiArray<2, float> exportImage(vigra::Shape2(_width, _height));
	for (int x = 0; x < _width; x++)
		for (int y = 0; y < _height; y++)
			exportImage(x, y) = boundaryDistance2(x, y, 0);
	vigra::exportImage(exportImage, vigra::ImageExportInfo("boundaries.png"));

	// compute l2 distance for each pixel to boundary
	LOG_DEBUG(tedlog) << "computing boundary distances" << std::endl;
	vigra::separableMultiDistSquared(
			boundaryDistance2,
			boundaryDistance2,
			true /* background */,
			pitch);

	for (int x = 0; x < _width; x++)
		for (int y = 0; y < _height; y++)
			exportImage(x, y) = boundaryDistance2(x, y, 0);
	vigra::exportImage(exportImage, vigra::ImageExportInfo("distances.png"));

	float maxDistanceThreshold2 = _maxDistanceThreshold*_maxDistanceThreshold;

	LOG_DEBUG(tedlog) << "checking for relabel candidates" << std::endl;
	for (unsigned int cellIndex = 0; cellIndex < _cells->size(); cellIndex++) {

		LOG_ALL(tedlog) << "checking cell " << cellIndex << std::endl;

		bool isCandidate = true;

		foreach (const cell_t::Location& l, (*_cells)[cellIndex]) {

			if (boundaryDistance2(l.x, l.y, l.z) >= maxDistanceThreshold2) {

				LOG_ALL(tedlog)
						<< "\tthis cell is not a candidate, location ("
						<< l.x << ", " << l.y << ", " << l.z
						<< " has distance " << sqrt(boundaryDistance2(l.x, l.y, l.z))
						<< " to boundary" << std::endl;
				isCandidate = false;
				break;
			}
		}

		if (isCandidate) {

			LOG_ALL(tedlog) << "\tthis cell is a candidate" << std::endl;
			candidates.push_back(cellIndex);
		}
	}

	return candidates;
}

bool
TolerantEditDistance::isBoundaryVoxel(int x, int y, int z, ImageStack& stack) {

	float center = (*stack[z])(x, y);

	for (int dz = -1; dz <= 1; dz++)
		for (int dy = -1; dy <= 1; dy++)
			for (int dx = -1; dx <= 1; dx++)
				if (
						!(dx == 0 && dy == 0 && dz == 0) &&
						(x + dx) >= 0 && (x + dx) < _width  &&
						(y + dy) >= 0 && (y + dy) < _height &&
						(z + dz) >= 0 && (z + dz) < _depth)

					if ((*stack[z + dz])(x + dx, y + dy) != center)
						return true;

	return false;
}

std::vector<TolerantEditDistance::cell_t::Location>
TolerantEditDistance::createNeighborhood() {

	std::vector<cell_t::Location> thresholdOffsets;

	for (int x = -_maxDistanceThresholdX; x <= _maxDistanceThresholdX; x++)
		for (int y = -_maxDistanceThresholdY; y <= _maxDistanceThresholdY; y++)
			for (int z = -_maxDistanceThresholdZ; z <= _maxDistanceThresholdZ; z++)
				if (
						x*_resolutionX*x*_resolutionX +
						y*_resolutionY*y*_resolutionY +
						z*_resolutionZ*z*_resolutionZ <= _maxDistanceThreshold*_maxDistanceThreshold)

					thresholdOffsets.push_back(cell_t::Location(x, y, z));

	return thresholdOffsets;
}

std::set<float>
TolerantEditDistance::getAlternativeLabels(const cell_t& cell, const std::vector<cell_t::Location>& neighborhood) {

	std::set<float> alternativeLabels;

	float cellLabel = cell.getReconstructionLabel();

	// for each location i in that cell
	foreach (const cell_t::Location& i, cell) {

		// all the labels in the neighborhood of i
		std::set<float> neighborhoodLabels;

		// for all locations within the neighborhood, get alternative labels
		foreach (const cell_t::Location& n, neighborhood) {

			cell_t::Location j(i.x + n.x, i.y + n.y, i.z + n.z);

			// are we leaving the image?
			if (j.x < 0 || j.x >= (int)_width || j.y < 0 || j.y >= (int)_height || j.z < 0 || j.z >= (int)_depth)
				continue;

			float label = (*(*_reconstruction)[j.z])(j.x, j.y);

			// collect all alternative labels
			if (label != cellLabel)
				neighborhoodLabels.insert(label);
		}

		if (alternativeLabels.size() != 0) {

			// intersect new alternative labels with current alternative 
			// labels
			std::set<float> intersection;
			std::insert_iterator<std::set<float> > inserter(intersection, intersection.begin());
			std::set_intersection(alternativeLabels.begin(), alternativeLabels.end(), neighborhoodLabels.begin(), neighborhoodLabels.end(), inserter);
			std::swap(intersection, alternativeLabels);

			// if empty, break
			if (alternativeLabels.size() == 0)
				break;

		} else {

			// this must be the first location we test, simply accept new 
			// alternative labels
			alternativeLabels = neighborhoodLabels;
		}
	}

	return alternativeLabels;
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

				cell_t& cell = (*_cells)[cellIndex];

				// first indicator variable for this cell
				unsigned int begin = var;

				// one variable for the default label
				assignIndicatorVariable(var++, cellIndex, cell.getGroundTruthLabel(), cell.getReconstructionLabel());

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

	// prepare error location image stack

	for (unsigned int i = 0; i < _depth; i++) {

		boost::shared_ptr<std::vector<float> > data = boost::make_shared<std::vector<float> >(_width*_height, 0.5);
		_splitLocations->add(boost::make_shared<Image>(_width, _height, data));
		data = boost::make_shared<std::vector<float> >(_width*_height, 0.5);
		_mergeLocations->add(boost::make_shared<Image>(_width, _height, data));
		data = boost::make_shared<std::vector<float> >(_width*_height, 0.5);
		_fpLocations->add(boost::make_shared<Image>(_width, _height, data));
		data = boost::make_shared<std::vector<float> >(_width*_height, 0.5);
		_fnLocations->add(boost::make_shared<Image>(_width, _height, data));
	}

	// prepare error data structure

	_errors->setCells(_cells);

	// fill error data structure

	for (unsigned int i = 0; i < _numIndicatorVars; i++) {

		if ((*_solution)[i]) {

			unsigned int cellIndex = _labelingByVar[i].first;
			float        recLabel  = _labelingByVar[i].second;

			_errors->addMapping(cellIndex, recLabel);
		}
	}

	// fill error location image stack

	// all cells that changed label within tolerance

	// all cells that split the ground truth
	float gtLabel;
	typedef Errors::cell_map_t::mapped_type::value_type mapping_t;
	foreach (gtLabel, _errors->getSplitLabels())
		foreach (const mapping_t& cells, _errors->getSplits(gtLabel))
			foreach (unsigned int cellIndex, cells.second)
				foreach (const cell_t::Location& l, (*_cells)[cellIndex])
					(*(*_splitLocations)[l.z])(l.x, l.y) = cells.first;

	// all cells that split the reconstruction
	float recLabel;
	foreach (recLabel, _errors->getMergeLabels())
		foreach (const mapping_t& cells, _errors->getMerges(recLabel))
			foreach (unsigned int cellIndex, cells.second)
				foreach (const cell_t::Location& l, (*_cells)[cellIndex])
					(*(*_mergeLocations)[l.z])(l.x, l.y) = cells.first;

	if (_haveBackgroundLabel) {

		// all cells that are false positives
		foreach (const mapping_t& cells, _errors->getFalsePositives())
			if (cells.first != _recBackgroundLabel) {
				foreach (unsigned int cellIndex, cells.second)
					foreach (const cell_t::Location& l, (*_cells)[cellIndex])
						(*(*_fpLocations)[l.z])(l.x, l.y) = cells.first;
			}

		// all cells that are false negatives
		foreach (const mapping_t& cells, _errors->getFalseNegatives())
			if (cells.first != _gtBackgroundLabel) {
				foreach (unsigned int cellIndex, cells.second)
					foreach (const cell_t::Location& l, (*_cells)[cellIndex])
						(*(*_fnLocations)[l.z])(l.x, l.y) = cells.first;
			}
	}

	LOG_DEBUG(tedlog) << "error counts from Errors data structure:" << std::endl;
	LOG_DEBUG(tedlog) << "num splits: " << _errors->getNumSplits() << std::endl;
	LOG_DEBUG(tedlog) << "num merges: " << _errors->getNumMerges() << std::endl;
	LOG_DEBUG(tedlog) << "num false positives: " << _errors->getNumFalsePositives() << std::endl;
	LOG_DEBUG(tedlog) << "num false negatives: " << _errors->getNumFalseNegatives() << std::endl;
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
			cell_t&      cell      = (*_cells)[cellIndex];

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

	_cells->clear();
	_cellsByRecToGtLabel.clear();
	_indicatorVarsByRecLabel.clear();
	_indicatorVarsByGtToRecLabel.clear();
	_labelingByVar.clear();
	_possibleGroundTruthMatches.clear();
	_groundTruthLabels.clear();
	_reconstructionLabels.clear();
	_errors->clear();
	_correctedReconstruction->clear();
	_splitLocations->clear();
	_mergeLocations->clear();
}

void
TolerantEditDistance::registerCell(float gtLabel, float recLabel, unsigned int cellIndex) {

	_cellsByRecToGtLabel[recLabel][gtLabel].push_back(cellIndex);
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

	_matchVars[gtLabel][recLabel] = var;
}

unsigned int
TolerantEditDistance::getMatchVariable(float gtLabel, float recLabel) {

	return _matchVars[gtLabel][recLabel];
}
