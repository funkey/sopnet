#include "SegmentVisitor.h"
#include "ContinuationSegment.h"

ContinuationSegment::ContinuationSegment(
		unsigned int id,
		Direction direction,
		boost::shared_ptr<Slice> sourceSlice,
		boost::shared_ptr<Slice> targetSlice) :
	Segment(id, direction),
	_sourceSlice(sourceSlice),
	_targetSlice(targetSlice) {}

void
ContinuationSegment::accept(SegmentVisitor& visitor) {

	visitor.visit(*this);
}

void
ContinuationSegment::accept(SegmentVisitor& visitor) const {

	visitor.visit(*this);
}

boost::shared_ptr<Slice>
ContinuationSegment::getSourceSlice() const {

	return _sourceSlice;
}

boost::shared_ptr<Slice>
ContinuationSegment::getTargetSlice() const {

	return _targetSlice;
}
