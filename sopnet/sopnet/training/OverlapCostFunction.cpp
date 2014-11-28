#include <pipeline/Process.h>
#include <util/Logger.h>
#include <sopnet/exceptions.h>
#include <sopnet/evaluation/GroundTruthExtractor.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "OverlapCostFunction.h"

static logger::LogChannel linearcostfunctionlog("linearcostfunctionlog", "[OverlapCostFunction] ");

OverlapCostFunction::OverlapCostFunction() :
	_costFunction(new costs_function_type(boost::bind(&OverlapCostFunction::costs, this, _1, _2, _3, _4))),
	_overlap(false, false),
	_gtFromSkeletons(optionGroundTruthFromSkeletons) {

	registerInput(_groundTruth, "ground truth");
	registerOutput(_costFunction, "cost function");
}

void
OverlapCostFunction::updateOutputs() {}

void
OverlapCostFunction::costs(
		const std::vector<boost::shared_ptr<EndSegment> >&          ends,
		const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
		const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
		std::vector<double>& segmentCosts) {

	unsigned int i = 0;

	foreach (boost::shared_ptr<EndSegment> end, ends) {

		double c = costs(*end);

		segmentCosts[i] += c;
		i++;
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations) {

		double c = costs(*continuation);

		segmentCosts[i] += c;
		i++;
	}

	foreach (boost::shared_ptr<BranchSegment> branch, branches) {

		double c = costs(*branch);

		segmentCosts[i] += c;
		i++;
	}
}

double
OverlapCostFunction::costs(const Segment& segment) {

	pipeline::Process<NeuronExtractor> connectedSegmentsExtractor;

	pipeline::Value<Segments> overlappingGtSegments = getOverlappingGroundTruthSegments(segment);
	connectedSegmentsExtractor->setInput(overlappingGtSegments);
	pipeline::Value<SegmentTrees> gtSegmentTrees = connectedSegmentsExtractor->getOutput();

	double minCosts = getDefaultCosts(segment);

	foreach (boost::shared_ptr<SegmentTree> gtSegmentTree, *gtSegmentTrees) {

		double costs = getMatchingCosts(segment, *gtSegmentTree);
		if (costs < minCosts)
			minCosts = costs;
	}

	return minCosts;
}


pipeline::Value<Segments>
OverlapCostFunction::getOverlappingGroundTruthSegments(const Segment& segment) {

	unsigned int interSectionInterval = segment.getInterSectionInterval();

	pipeline::Value<Segments> overlappingGtSegments;

	foreach (boost::shared_ptr<EndSegment> gtSegment, _groundTruth->getEnds(interSectionInterval))
		if (overlaps(segment, *gtSegment))
			overlappingGtSegments->add(gtSegment);
	foreach (boost::shared_ptr<ContinuationSegment> gtSegment, _groundTruth->getContinuations(interSectionInterval))
		if (overlaps(segment, *gtSegment))
			overlappingGtSegments->add(gtSegment);
	foreach (boost::shared_ptr<BranchSegment> gtSegment, _groundTruth->getBranches(interSectionInterval))
		if (overlaps(segment, *gtSegment))
			overlappingGtSegments->add(gtSegment);

	return overlappingGtSegments;
}

double
OverlapCostFunction::getDefaultCosts(const Segment& segment) {

	if (_gtFromSkeletons)
		return 0;

	return sumSizes(segment.getSlices());
}

double
OverlapCostFunction::getMatchingCosts(const Segment& segment, const Segments& segments) {

	std::vector<boost::shared_ptr<Slice> > aLeftSlices;
	std::vector<boost::shared_ptr<Slice> > aRightSlices;

	addLeftRightSlices(segment, aLeftSlices, aRightSlices);

	std::vector<boost::shared_ptr<Slice> > bLeftSlices;
	std::vector<boost::shared_ptr<Slice> > bRightSlices;

	foreach (boost::shared_ptr<Segment> gtSegment, segments.getSegments())
		addLeftRightSlices(*gtSegment, bLeftSlices, bRightSlices);

	int leftOverlap  = overlap(aLeftSlices,  bLeftSlices);
	int rightOverlap = overlap(aRightSlices, bRightSlices);

	// if ground truth is skeleton, number of different pixels don't mean 
	// anything -- in this case, return the number of connected skeleton nodes
	if (_gtFromSkeletons)
		return -(std::min(leftOverlap, rightOverlap));

	int leftSum  = sumSizes(aLeftSlices)  + sumSizes(bLeftSlices);
	int rightSum = sumSizes(aRightSlices) + sumSizes(bRightSlices);

	// number of different pixels - overlap
	// = sum - 3*overlap
	return (leftSum + rightSum) - 3*(leftOverlap + rightOverlap);
}

bool
OverlapCostFunction::overlaps(const Segment& a, const Segment& b) {

	std::vector<boost::shared_ptr<Slice> > aLeftSlices;
	std::vector<boost::shared_ptr<Slice> > aRightSlices;

	addLeftRightSlices(a, aLeftSlices, aRightSlices);

	std::vector<boost::shared_ptr<Slice> > bLeftSlices;
	std::vector<boost::shared_ptr<Slice> > bRightSlices;

	addLeftRightSlices(b, bLeftSlices, bRightSlices);

	foreach (boost::shared_ptr<Slice> aSlice, aLeftSlices)
		foreach (boost::shared_ptr<Slice> bSlice, bLeftSlices)
			if (_overlap.exceeds(*aSlice, *bSlice, 0))
				return true;

	foreach (boost::shared_ptr<Slice> aSlice, aRightSlices)
		foreach (boost::shared_ptr<Slice> bSlice, bRightSlices)
			if (_overlap.exceeds(*aSlice, *bSlice, 0))
				return true;

	return false;
}

void
OverlapCostFunction::addLeftRightSlices(
		const Segment& segment,
		std::vector<boost::shared_ptr<Slice> >& leftSlices,
		std::vector<boost::shared_ptr<Slice> >& rightSlices) {

	if (segment.getDirection() == Right) {

		foreach (boost::shared_ptr<Slice> slice, segment.getSourceSlices())
			leftSlices.push_back(slice);
		foreach (boost::shared_ptr<Slice> slice, segment.getTargetSlices())
			rightSlices.push_back(slice);

	} else {

		foreach (boost::shared_ptr<Slice> slice, segment.getSourceSlices())
			rightSlices.push_back(slice);
		foreach (boost::shared_ptr<Slice> slice, segment.getTargetSlices())
			leftSlices.push_back(slice);
	}
}

int
OverlapCostFunction::sumSizes(const std::vector<boost::shared_ptr<Slice> >& slices) {

	unsigned int sum = 0;

	foreach (boost::shared_ptr<Slice> slice, slices)
		sum += slice->getComponent()->getSize();

	return sum;
}

int
OverlapCostFunction::overlap(
		const std::vector<boost::shared_ptr<Slice> >& aSlices,
		const std::vector<boost::shared_ptr<Slice> >& bSlices) {

	unsigned int overlap = 0;

	foreach (boost::shared_ptr<Slice> aSlice, aSlices)
		foreach (boost::shared_ptr<Slice> bSlice, bSlices)
			overlap += _overlap(*aSlice, *bSlice);

	return overlap;
}

