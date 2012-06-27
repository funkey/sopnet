#include "ContinuationSegment.h"

ContinuationSegment::ContinuationSegment(
		unsigned int id,
		Direction direction,
		boost::shared_ptr<Slice> sourceSlice,
		boost::shared_ptr<Slice> targetSlice) :
	Segment(id, direction, sourceSlice->getSection() + (direction == Left ? 0 : 1)),
	_sourceSlice(sourceSlice),
	_targetSlice(targetSlice) {}

boost::shared_ptr<Slice>
ContinuationSegment::getSourceSlice() const {

	return _sourceSlice;
}

boost::shared_ptr<Slice>
ContinuationSegment::getTargetSlice() const {

	return _targetSlice;
}
