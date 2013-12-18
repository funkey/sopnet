#include <boost/range/adaptors.hpp>
#include <util/foreach.h>
#include "Errors.h"

Errors::Errors() :
	_haveBackgroundLabel(false),
	_dirty(true) {

	clear();
}

Errors::Errors(float gtBackgroundLabel, float recBackgroundLabel) :
	_haveBackgroundLabel(true),
	_gtBackgroundLabel(gtBackgroundLabel),
	_recBackgroundLabel(recBackgroundLabel),
	_dirty(true) {

	clear();
}

void
Errors::clear() {

	_recLabelsByGt.clear();
	_gtLabelsByRec.clear();

	_dirty = true;
}

void
Errors::addMapping(float gtLabel, float recLabel, unsigned int size) {

	addEntry(_gtLabelsByRec, recLabel, gtLabel, size);
	addEntry(_recLabelsByGt, gtLabel, recLabel, size);

	_dirty = true;
}

std::vector<float>
Errors::getReconstructionLabels(float gtLabel) {

	std::vector<float> recLabels;

	foreach(float recLabel, _recLabelsByGt[gtLabel] | boost::adaptors::map_keys)
		recLabels.push_back(recLabel);

	return recLabels;
}

std::vector<float>
Errors::getGroundTruthLabels(float recLabel) {

	std::vector<float> gtLabels;

	foreach(float gtLabel, _gtLabelsByRec[recLabel] | boost::adaptors::map_keys)
		gtLabels.push_back(gtLabel);

	return gtLabels;
}

unsigned int
Errors::getOverlap(float gtLabel, float recLabel) {

	if (_recLabelsByGt.count(gtLabel) == 0 || _recLabelsByGt[gtLabel].count(recLabel) == 0)
		return 0;

	return _recLabelsByGt[gtLabel][recLabel];
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

void
Errors::updateErrorCounts() {

	if (!_dirty)
		return;

	_dirty = false;

	_numSplits = 0;
	_numMerges = 0;
	_numFalsePositives = 0;
	_numFalseNegatives = 0;

	typedef std::map<float, std::map<float, unsigned int> >::value_type mapping_t;

	// ground truth to reconstruction labels
	foreach (mapping_t& i, _recLabelsByGt)
		if (_haveBackgroundLabel && i.first == _gtBackgroundLabel)
			_numFalsePositives += i.second.size() - 1;
		else
			_numSplits += i.second.size() - 1;

	// reconstruction to ground truth labels
	foreach (mapping_t& i, _gtLabelsByRec)
		if (_haveBackgroundLabel && i.first == _recBackgroundLabel)
			_numFalseNegatives += i.second.size() - 1;
		else
			_numMerges += i.second.size() - 1;
}

void
Errors::addEntry(std::map<float, std::map<float, unsigned int> >& map, float a, float b, unsigned int value) {

	if (map.count(a) == 0 || map[a].count(b) == 0)
		map[a][b] = 0;

	map[a][b] += value;
}
