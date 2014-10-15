#include <boost/timer/timer.hpp>
#include <boost/range/adaptors.hpp>
#include <util/exceptions.h>
#include <util/foreach.h>
#include <util/Logger.h>
#include "Errors.h"

logger::LogChannel errorslog("errorslog", "[Errors] ");

Errors::Errors() :
	_haveBackgroundLabel(false),
	_dirty(true) {

	clear();

	LOG_DEBUG(errorslog) << "created errors data structure without background label" << std::endl;
}

Errors::Errors(float gtBackgroundLabel, float recBackgroundLabel) :
	_haveBackgroundLabel(true),
	_gtBackgroundLabel(gtBackgroundLabel),
	_recBackgroundLabel(recBackgroundLabel),
	_dirty(true) {

	clear();

	LOG_DEBUG(errorslog) << "created errors data structure with background label" << std::endl;
}

void
Errors::setCells(cells_t cells) {

	_cells = cells;
	clear();
}

void
Errors::clear() {

	_cellsByGtToRecLabel.clear();
	_cellsByRecToGtLabel.clear();

	_dirty = true;
}

void
Errors::addMapping(unsigned int cellIndex, float recLabel) {

	if (!_cells)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("cells need to be set before using addMapping()") << STACK_TRACE);

	float gtLabel = (*_cells)[cellIndex].getGroundTruthLabel();

	addEntry(_cellsByRecToGtLabel, recLabel, gtLabel, cellIndex);
	addEntry(_cellsByGtToRecLabel, gtLabel, recLabel, cellIndex);

	_dirty = true;
}

std::vector<float>
Errors::getReconstructionLabels(float gtLabel) {

	std::vector<float> recLabels;

	foreach(float recLabel, _cellsByGtToRecLabel[gtLabel] | boost::adaptors::map_keys)
		recLabels.push_back(recLabel);

	return recLabels;
}

std::vector<float>
Errors::getGroundTruthLabels(float recLabel) {

	std::vector<float> gtLabels;

	foreach(float gtLabel, _cellsByRecToGtLabel[recLabel] | boost::adaptors::map_keys)
		gtLabels.push_back(gtLabel);

	return gtLabels;
}

unsigned int
Errors::getOverlap(float gtLabel, float recLabel) {

	if (!_cells)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("cells need to be set before using getOverlap()") << STACK_TRACE);

	if (_cellsByGtToRecLabel.count(gtLabel) == 0 || _cellsByGtToRecLabel[gtLabel].count(recLabel) == 0)
		return 0;

	unsigned int overlap = 0;
	foreach (unsigned int cellIndex, _cellsByGtToRecLabel[gtLabel][recLabel])
		overlap += (*_cells)[cellIndex].size();

	return overlap;
}

unsigned int
Errors::getNumSplits() {

	updateErrorCounts();
	return _numSplits;
}

unsigned int
Errors::getNumMerges() {

	updateErrorCounts();
	return _numMerges;
}

unsigned int
Errors::getNumFalsePositives() {

	updateErrorCounts();
	return _numFalsePositives;
}

unsigned int
Errors::getNumFalseNegatives() {

	updateErrorCounts();
	return _numFalseNegatives;
}

std::set<float>
Errors::getMergeLabels() {

	updateErrorCounts();

	std::set<float> mergeLabels;
	foreach (float k, _merges | boost::adaptors::map_keys)
		if (!_haveBackgroundLabel || k != _recBackgroundLabel)
			mergeLabels.insert(k);
	return mergeLabels;
}

std::set<float>
Errors::getSplitLabels() {

	updateErrorCounts();

	std::set<float> splitLabels;
	foreach (float k, _splits | boost::adaptors::map_keys)
		if (!_haveBackgroundLabel || k != _gtBackgroundLabel)
			splitLabels.insert(k);
	return splitLabels;
}

const Errors::cell_map_t::mapped_type&
Errors::getSplits(float gtLabel) {

	updateErrorCounts();
	return _splits[gtLabel];
}

const Errors::cell_map_t::mapped_type&
Errors::getMerges(float recLabel) {

	updateErrorCounts();
	return _merges[recLabel];
}

const Errors::cell_map_t::mapped_type&
Errors::getFalsePositives() {

	if (!_haveBackgroundLabel)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("we don't hav a background label -- cannot give false positives"));

	updateErrorCounts();
	return _splits[_gtBackgroundLabel];
}

const Errors::cell_map_t::mapped_type&
Errors::getFalseNegatives() {

	if (!_haveBackgroundLabel)
		BOOST_THROW_EXCEPTION(UsageError() << error_message("we don't hav a background label -- cannot give false negatives"));

	updateErrorCounts();
	return _merges[_recBackgroundLabel];
}

void
Errors::updateErrorCounts() {

	if (!_dirty)
		return;

	boost::timer::auto_cpu_timer timer("\tErrors::updateErrorCounts():\t\t%w\n");

	_dirty = false;

	_numSplits = 0;
	_numMerges = 0;
	_numFalsePositives = 0;
	_numFalseNegatives = 0;
	_splits.clear();
	_merges.clear();

	findSplits(_cellsByGtToRecLabel, _splits, _numSplits, _numFalsePositives, _gtBackgroundLabel);
	findSplits(_cellsByRecToGtLabel, _merges, _numMerges, _numFalseNegatives, _recBackgroundLabel);
}

void
Errors::findSplits(
		const cell_map_t& cellMap,
		cell_map_t&       splits,
		unsigned int&     numSplits,
		unsigned int&     numFalsePositives,
		float             backgroundLabel) {

	typedef cell_map_t::value_type mapping_t;

	foreach (const mapping_t& i, cellMap) {

		unsigned int partners = i.second.size();

		// one-to-one mapping is okay
		if (partners == 1)
			continue;

		// remeber the split
		splits[i.first] = i.second;

		// increase count
		if (_haveBackgroundLabel && i.first == backgroundLabel)
			numFalsePositives += partners - 1;
		else
			numSplits += partners - 1;
	}
}

void
Errors::addEntry(cell_map_t& map, float a, float b, unsigned int cellIndex) {

	map[a][b].insert(cellIndex);
}
