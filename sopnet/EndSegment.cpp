#include "SegmentVisitor.h"
#include "EndSegment.h"

EndSegment::EndSegment(
		unsigned int id,
		Direction direction,
		boost::shared_ptr<Slice> slice) :
	Segment(id, direction),
	_slice(slice) {}

void
EndSegment::accept(SegmentVisitor& visitor) {

	visitor.visit(*this);
}

void
EndSegment::accept(SegmentVisitor& visitor) const {

	visitor.visit(*this);
}

boost::shared_ptr<Slice>
EndSegment::getSlice() const {

	return _slice;
}
