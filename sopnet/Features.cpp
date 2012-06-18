#include "Features.h"

double Features::None = -100;

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
	_segmentIdsMap.clear();
}

void
Features::add(unsigned int segmentId, const std::vector<double>& features){

	_segmentIdsMap[segmentId] = _features.size();

	_features.push_back(features);
}

const std::vector<double>&
Features::get(unsigned int segmentId) {

	if (_segmentIdsMap.count(segmentId))

		return _features[_segmentIdsMap[segmentId]];

	else

		BOOST_THROW_EXCEPTION(Exception() << error_message("invalid segment id given") << STACK_TRACE);
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
		out << name << " ";
	out << std::endl;

	foreach (const std::vector<double>& feature, features) {
		foreach (double f, feature)
			out << f << " ";
		out << std::endl;
	}

	return out;
}
