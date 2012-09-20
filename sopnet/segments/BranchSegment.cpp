#include <imageprocessing/ConnectedComponent.h>
#include "BranchSegment.h"

BranchSegment::BranchSegment(
		unsigned int id,
		Direction direction,
		boost::shared_ptr<Slice> sourceSlice,
		boost::shared_ptr<Slice> targetSlice1,
		boost::shared_ptr<Slice> targetSlice2) :
	Segment(
			id,
			direction,
			(sourceSlice->getComponent()->getCenter()*sourceSlice->getComponent()->getSize()   +
			 targetSlice1->getComponent()->getCenter()*targetSlice1->getComponent()->getSize() +
			 targetSlice2->getComponent()->getCenter()*targetSlice2->getComponent()->getSize())/
			 (sourceSlice->getComponent()->getSize() + targetSlice1->getComponent()->getSize() + targetSlice2->getComponent()->getSize()),
			sourceSlice->getSection() + (direction == Left ? 0 : 1)),
	_sourceSlice(sourceSlice),
	_targetSlice1(targetSlice1),
	_targetSlice2(targetSlice2) {}

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
