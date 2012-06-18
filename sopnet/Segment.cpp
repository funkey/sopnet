#include "Segment.h"

Segment::Segment(unsigned int id, Direction direction) :
	_id(id),
	_direction(direction) {}

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

unsigned int Segment::NextSegmentId = 0;
boost::mutex Segment::SegmentIdMutex;
