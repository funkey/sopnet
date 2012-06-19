#include <boost/function.hpp>

#include <external/kdtree++/kdtree.hpp>

#include <imageprocessing/ConnectedComponent.h>
#include <util/foreach.h>
#include "EndSegment.h"
#include "ContinuationSegment.h"
#include "BranchSegment.h"
#include "SegmentExtractor.h"

static logger::LogChannel segmentextractorlog("segmentextractorlog", "[SegmentExtractor] ");

SegmentExtractor::SegmentExtractor() {

	registerInput(_prevSlices, "previous slices");
	registerInput(_nextSlices, "next slices");
	registerInput(_prevLinearConstraints, "previous linear constraints");
	registerInput(_nextLinearConstraints, "next linear constraints", pipeline::Optional);
	registerInput(_distanceThreshold, "distance threshold");

	registerOutput(_segments, "segments");
	registerOutput(_linearConstraints, "linear constraints");
}

void
SegmentExtractor::updateOutputs() {

	extractSegments();

	assembleLinearConstraints();
}

struct CoordAccessor {

	double operator()(boost::shared_ptr<Slice> slice, size_t k) {

		if (k == 0)
			return slice->getComponent()->getCenter().x;
		else
			return slice->getComponent()->getCenter().y;
	}
};

void
SegmentExtractor::extractSegments() {

	LOG_DEBUG(segmentextractorlog) << "extracting segments..." << std::endl;

	LOG_ALL(segmentextractorlog) << "extracting ends to previous section..." << std::endl;

	// end segments for every previous slice
	foreach (boost::shared_ptr<Slice> prevSlice, *_prevSlices) {

		extractSegment(prevSlice, Left);
		extractSegment(prevSlice, Right);
	}

	LOG_ALL(segmentextractorlog) << "extracting ends to next section..." << std::endl;

	// end segments for every next slice, if we are the last segment extractor
	if (_nextLinearConstraints)
		foreach (boost::shared_ptr<Slice> nextSlice, *_nextSlices) {

			extractSegment(nextSlice, Left);
			extractSegment(nextSlice, Right);
		}

	LOG_ALL(segmentextractorlog) << "creating kd-tree for next slices..." << std::endl;

	// put all next slices in a kd-tree
	typedef KDTree::KDTree<2, boost::shared_ptr<Slice>, boost::function<double(boost::shared_ptr<Slice>,size_t)> > tree_type;
	CoordAccessor coordAccessor;
	tree_type kdtree(coordAccessor);

	foreach (boost::shared_ptr<Slice> nextSlice, *_nextSlices)
		kdtree.insert(nextSlice);
	kdtree.optimise();

	LOG_ALL(segmentextractorlog) << "extracting continuations to next slice..." << std::endl;

	// for all slices in previous section...
	foreach (boost::shared_ptr<Slice> prevSlice, *_prevSlices) {

		LOG_ALL(segmentextractorlog) << "searching partners of " << prevSlice->getComponent()->getCenter() << " within " << *_distanceThreshold << std::endl;

		std::vector<boost::shared_ptr<Slice> > closeNextSlices;
		kdtree.find_within_range(prevSlice, *_distanceThreshold, std::back_inserter(closeNextSlices));

		LOG_ALL(segmentextractorlog) << "found " << closeNextSlices.size() << " partners" << std::endl;

		// ...and all next slices within a threshold distance
		foreach (boost::shared_ptr<Slice> nextSlice, closeNextSlices)
			extractSegment(prevSlice, nextSlice);
	}

	LOG_DEBUG(segmentextractorlog) << "extracted " << _segments->size() << " segments" << std::endl;
}

void
SegmentExtractor::extractSegment(boost::shared_ptr<Slice> slice, Direction direction) {

	boost::shared_ptr<Segment> segment = boost::make_shared<EndSegment>(Segment::getNextSegmentId(), direction, slice);

	_segments->add(segment);

	// only for ends that have the slice on the ride side
	if (direction == Right)
		_sliceSegments[slice->getId()].push_back(segment->getId());
}

void
SegmentExtractor::extractSegment(boost::shared_ptr<Slice> prevSlice, boost::shared_ptr<Slice> nextSlice) {

	boost::shared_ptr<Segment> segment = boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, prevSlice, nextSlice);

	_segments->add(segment);

	// only for the left slice
	_sliceSegments[prevSlice->getId()].push_back(segment->getId());
}

void
SegmentExtractor::assembleLinearConstraints() {

	LOG_DEBUG(segmentextractorlog) << "assembling linear constraints..." << std::endl;

	_linearConstraints->clear();

	/* For each linear constraint on the slices, create a corresponding linear
	 * constraint on the segments by replacing every sliceId by all segmentIds
	 * that are using this sliceId on the left side.
	 */
	foreach (const LinearConstraint& sliceConstraint, *_prevLinearConstraints)
		assembleLinearConstraint(sliceConstraint);

	/* If linear constraints were also given for the next slice, consider them
	 * as well.
	 */
	if (_nextLinearConstraints) {

		LOG_DEBUG(segmentextractorlog) << "using linear constraints of next slice" << std::endl;

		foreach (const LinearConstraint& sliceConstraint, *_nextLinearConstraints)
			assembleLinearConstraint(sliceConstraint);
	}

	LOG_DEBUG(segmentextractorlog) << "assembled " << _linearConstraints->size() << " linear constraints" << std::endl;
}

void
SegmentExtractor::assembleLinearConstraint(const LinearConstraint& sliceConstraint) {

	LinearConstraint constraint;

	// for each slice in the constraint
	typedef std::map<unsigned int, double>::value_type pair_t;
	foreach (const pair_t& pair, sliceConstraint.getCoefficients()) {

		unsigned int sliceId = pair.first;

		// for all the segments that involve this slice
		const std::vector<unsigned int> segmentIds = _sliceSegments[sliceId];

		foreach (unsigned int segmentId, segmentIds)
			constraint.setCoefficient(segmentId, 1.0);
	}

	constraint.setRelation(sliceConstraint.getRelation());

	constraint.setValue(1);

	LOG_ALL(segmentextractorlog) << "created constraint " << constraint << std::endl;
	LOG_ALL(segmentextractorlog) << "from               " << sliceConstraint << std::endl;

	_linearConstraints->add(constraint);
}
