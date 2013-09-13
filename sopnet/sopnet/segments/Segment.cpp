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

std::vector<boost::shared_ptr<Slice> >
Segment::getSourceSlices() const {

	std::vector<boost::shared_ptr<Slice> > slices = getSlices();

	unsigned int interSectionInterval = getInterSectionInterval();
	unsigned int sourceSection = (getDirection() == Left ? interSectionInterval : interSectionInterval - 1);

	std::vector<boost::shared_ptr<Slice> > sourceSlices;

	foreach (boost::shared_ptr<Slice> slice, slices)
		if (slice->getSection() == sourceSection)
			sourceSlices.push_back(slice);

	return sourceSlices;
}

std::vector<boost::shared_ptr<Slice> >
Segment::getTargetSlices() const {

	std::vector<boost::shared_ptr<Slice> > slices = getSlices();

	unsigned int interSectionInterval = getInterSectionInterval();
	unsigned int targetSection = (getDirection() == Left ? interSectionInterval - 1 : interSectionInterval);

	std::vector<boost::shared_ptr<Slice> > targetSlices;

	foreach (boost::shared_ptr<Slice> slice, slices)
		if (slice->getSection() == targetSection)
			targetSlices.push_back(slice);

	return targetSlices;
}


unsigned int Segment::NextSegmentId = 0;
boost::mutex Segment::SegmentIdMutex;
