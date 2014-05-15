#include <sopnet/exceptions.h>
#include "Features.h"

double Features::NoFeatureValue = 0;

Features::Features() :
	_nextSegmentIndex(0) {}

void
Features::addName(const std::string& name) {

	_featureNames.push_back(name);
}

const std::vector<std::string>&
Features::getNames() const {

	return _featureNames;
}

void
Features::clear(){

	_features.clear();
	_featureNames.clear();
	_segmentIdsMap.clear();

	_nextSegmentIndex = 0;
}

void
Features::resize(unsigned int numVectors, unsigned int numFeatures) {

	std::vector<double> templ(numFeatures, 0.0);

	_features.resize(numVectors, templ);
}

unsigned int
Features::numFeatures() {

	return _featureNames.size();
}

std::vector<double>&
Features::get(unsigned int segmentId) {

	if (!_segmentIdsMap.count(segmentId)) {

		_segmentIdsMap[segmentId] = _nextSegmentIndex;
		_nextSegmentIndex++;
	}

	return _features[_segmentIdsMap[segmentId]];
}

unsigned int
Features::size(){

	return _features.size();
}

Features::iterator
Features::begin(){

	return _features.begin();
}

Features::iterator
Features::end(){

	return _features.end();
}

Features::const_iterator
Features::begin() const{

	return _features.begin();
}

Features::const_iterator
Features::end() const{

	return _features.end();
}

std::vector<double>&
Features::operator[](unsigned int i) {

	return _features[i];
}

const std::vector<double>&
Features::operator[](unsigned int i) const {

	return _features[i];
}

void
Features::setSegmentIdsMap(const segment_ids_map& map) {

	_segmentIdsMap = map;
}

const Features::segment_ids_map&
Features::getSegmentsIdsMap() const {

	return _segmentIdsMap;
}

std::ostream&
operator<<(std::ostream& out, const Features& features) {

	foreach (const std::string& name, features.getNames())
		out << "\t" << name << std::endl;;
	out << std::endl;

	return out;
}
