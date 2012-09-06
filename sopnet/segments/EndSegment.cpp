#include <imageprocessing/ConnectedComponent.h>
#include "EndSegment.h"

EndSegment::EndSegment(
		unsigned int id,
		Direction direction,
		boost::shared_ptr<Slice> slice) :
	Segment(id, direction, slice->getComponent()->getCenter(), slice->getSection() + (direction == Left ? 0 : 1)),
	_slice(slice) {}

boost::shared_ptr<Slice>
EndSegment::getSlice() const {

	return _slice;
}
