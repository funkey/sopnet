#include "SegmentVisitor.h"
#include "BranchSegment.h"

BranchSegment::BranchSegment(
		unsigned int id,
		Direction direction,
		boost::shared_ptr<Slice> sourceSlice,
		boost::shared_ptr<Slice> targetSlice1,
		boost::shared_ptr<Slice> targetSlice2) :
	Segment(id, direction),
	_sourceSlice(sourceSlice),
	_targetSlice1(targetSlice1),
	_targetSlice2(targetSlice2) {}

void
BranchSegment::accept(SegmentVisitor& visitor) {

	visitor.visit(*this);
}

void
BranchSegment::accept(SegmentVisitor& visitor) const {

	visitor.visit(*this);
}

boost::shared_ptr<Slice>
BranchSegment::getSourceSlice() const {

	return _sourceSlice;
}

boost::shared_ptr<Slice>
BranchSegment::getTargetSlice1() const {

	return _targetSlice1;
}

boost::shared_ptr<Slice>
BranchSegment::getTargetSlice2() const {

	return _targetSlice2;
}
