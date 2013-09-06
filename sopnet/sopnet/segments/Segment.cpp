#include "Segment.h"

Segment::Segment(
		unsigned int id,
		Direction direction,
		const util::point<double>& center,
		unsigned int interSectionInterval) :
	_id(id),
	_direction(direction),
	_center(center),
	_interSectionInterval(interSectionInterval) {}

unsigned int
Segment::getNextSegmentId() {

	unsigned int id;

	{
		boost::mutex::scoped_lock lock(SegmentIdMutex);

		id = NextSegmentId;

		NextSegmentId++;
	}

	return id;
}

unsigned int
Segment::getId() const {

	return _id;
}

Direction
Segment::getDirection() const {

	return _direction;
}

unsigned int
Segment::getInterSectionInterval() const {

	return _interSectionInterval;
}

unsigned int Segment::NextSegmentId = 0;
boost::mutex Segment::SegmentIdMutex;
