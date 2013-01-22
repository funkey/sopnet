#include <imageprocessing/ConnectedComponent.h>
#include "ContinuationSegment.h"

ContinuationSegment::ContinuationSegment(
		unsigned int id,
		Direction direction,
		boost::shared_ptr<Slice> sourceSlice,
		boost::shared_ptr<Slice> targetSlice) :
	Segment(
			id,
			direction,
			(sourceSlice->getComponent()->getCenter()*sourceSlice->getComponent()->getSize() +
			 targetSlice->getComponent()->getCenter()*targetSlice->getComponent()->getSize())/
			 (sourceSlice->getComponent()->getSize() + targetSlice->getComponent()->getSize()),
			sourceSlice->getSection() + (direction == Left ? 0 : 1)),
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

std::vector<boost::shared_ptr<Slice> >
ContinuationSegment::getSlices() const {

	std::vector<boost::shared_ptr<Slice> > slices;

	slices.push_back(getSourceSlice());
	slices.push_back(getTargetSlice());

	return slices;
}
