#include "Segments.h"

void
Segments::clear() {

	_segments.clear();
}

void
Segments::add(boost::shared_ptr<Segment> segment) {

	_segments.push_back(segment);
}

void
Segments::addAll(boost::shared_ptr<Segments> segments) {

	_segments.insert(_segments.end(), segments->begin(), segments->end());
}
