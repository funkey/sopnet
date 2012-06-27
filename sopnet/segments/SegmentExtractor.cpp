#include <boost/function.hpp>

#include <external/kdtree++/kdtree.hpp>

#include <imageprocessing/ConnectedComponent.h>
#include <util/foreach.h>
#include "EndSegment.h"
#include "ContinuationSegment.h"
#include "BranchSegment.h"
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

	unsigned int oldSize = 0;

	LOG_DEBUG(segmentextractorlog) << "extracting segments..." << std::endl;

	LOG_DEBUG(segmentextractorlog)
			<< "previous sections contains " << _prevSlices->size() << " slices,"
			<< "next sections contains "     << _nextSlices->size() << " slices" << std::endl;

	LOG_DEBUG(segmentextractorlog) << "extracting ends to and from previous section..." << std::endl;

	// end segments for every previous slice
	foreach (boost::shared_ptr<Slice> prevSlice, *_prevSlices) {

		extractSegment(prevSlice, Left);
		extractSegment(prevSlice, Right);
	}

	LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	oldSize = _segments->size();

	// end segments for every next slice, if we are the last segment extractor
	if (_nextLinearConstraints) {

		LOG_DEBUG(segmentextractorlog) << "extracting ends to and from next section..." << std::endl;

		foreach (boost::shared_ptr<Slice> nextSlice, *_nextSlices) {

			extractSegment(nextSlice, Left);
			extractSegment(nextSlice, Right);
		}
	}

	LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	oldSize = _segments->size();

	LOG_DEBUG(segmentextractorlog) << "creating kd-tree for next slices..." << std::endl;

	// put all next slices in a kd-tree
	typedef KDTree::KDTree<2, boost::shared_ptr<Slice>, boost::function<double(boost::shared_ptr<Slice>,size_t)> > tree_type;
	CoordAccessor coordAccessor;
	tree_type nextKDTree(coordAccessor);

	foreach (boost::shared_ptr<Slice> nextSlice, *_nextSlices)
		nextKDTree.insert(nextSlice);
	nextKDTree.optimise();

	LOG_DEBUG(segmentextractorlog) << "creating kd-tree for prev slices..." << std::endl;

	// put all prev slices in a kd-tree
	typedef KDTree::KDTree<2, boost::shared_ptr<Slice>, boost::function<double(boost::shared_ptr<Slice>,size_t)> > tree_type;
	tree_type prevKDTree(coordAccessor);

	foreach (boost::shared_ptr<Slice> prevSlice, *_prevSlices)
		prevKDTree.insert(prevSlice);
	prevKDTree.optimise();

	LOG_DEBUG(segmentextractorlog) << "extracting continuations to next section..." << std::endl;

	// for all slices in previous section...
	foreach (boost::shared_ptr<Slice> prevSlice, *_prevSlices) {

		std::vector<boost::shared_ptr<Slice> > closeNextSlices;
		nextKDTree.find_within_range(prevSlice, *_distanceThreshold, std::back_inserter(closeNextSlices));

		LOG_ALL(segmentextractorlog) << "found " << closeNextSlices.size() << " partners" << std::endl;

		// ...and all next slices within a threshold distance
		foreach (boost::shared_ptr<Slice> nextSlice, closeNextSlices)
			extractSegment(prevSlice, nextSlice);
	}

	LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	oldSize = _segments->size();

	LOG_DEBUG(segmentextractorlog) << "extracting bisections from previous to next section..." << std::endl;

	// for all slices in previous section...
	foreach (boost::shared_ptr<Slice> prevSlice, *_prevSlices) {

		std::vector<boost::shared_ptr<Slice> > closeNextSlices;
		nextKDTree.find_within_range(prevSlice, *_distanceThreshold, std::back_inserter(closeNextSlices));

		LOG_ALL(segmentextractorlog) << "found " << closeNextSlices.size() << " partners" << std::endl;

		// ...and all pairs of next slices within a threshold distance
		foreach (boost::shared_ptr<Slice> nextSlice1, closeNextSlices)
			foreach (boost::shared_ptr<Slice> nextSlice2, closeNextSlices)
				if (nextSlice1->getId() < nextSlice2->getId())
					if (!_nextSlices->areConflicting(nextSlice1->getId(), nextSlice2->getId()))
						extractSegment(prevSlice, nextSlice1, nextSlice2, Right);
	}

	LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	oldSize = _segments->size();

	LOG_DEBUG(segmentextractorlog) << "extracting bisections from next to previous section..." << std::endl;

	// for all slices in next section...
	foreach (boost::shared_ptr<Slice> nextSlice, *_nextSlices) {

		std::vector<boost::shared_ptr<Slice> > closePrevSlices;
		prevKDTree.find_within_range(nextSlice, *_distanceThreshold, std::back_inserter(closePrevSlices));

		LOG_ALL(segmentextractorlog) << "found " << closePrevSlices.size() << " partners" << std::endl;

		// ...and all pairs of prev slices within a threshold distance
		foreach (boost::shared_ptr<Slice> prevSlice1, closePrevSlices)
			foreach (boost::shared_ptr<Slice> prevSlice2, closePrevSlices)
				if (prevSlice1->getId() < prevSlice2->getId())
					if (!_prevSlices->areConflicting(prevSlice1->getId(), prevSlice2->getId()))
						extractSegment(nextSlice, prevSlice1, prevSlice2, Left);
	}

	LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	oldSize = _segments->size();

	LOG_DEBUG(segmentextractorlog) << "extracted " << _segments->size() << " segments in total" << std::endl;
}

void
SegmentExtractor::extractSegment(boost::shared_ptr<Slice> slice, Direction direction) {

	boost::shared_ptr<EndSegment> segment = boost::make_shared<EndSegment>(Segment::getNextSegmentId(), direction, slice);

	_segments->add(segment);

	// only for ends that have the slice on the left side
	if (direction == Right)
		_sliceSegments[slice->getId()].push_back(segment->getId());
}

void
SegmentExtractor::extractSegment(boost::shared_ptr<Slice> prevSlice, boost::shared_ptr<Slice> nextSlice) {

	boost::shared_ptr<ContinuationSegment> segment = boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, prevSlice, nextSlice);

	_segments->add(segment);

	// only for the left slice
	_sliceSegments[prevSlice->getId()].push_back(segment->getId());
}

void
SegmentExtractor::extractSegment(
		boost::shared_ptr<Slice> source,
		boost::shared_ptr<Slice> target1,
		boost::shared_ptr<Slice> target2,
		Direction direction) {

	boost::shared_ptr<BranchSegment> segment = boost::make_shared<BranchSegment>(Segment::getNextSegmentId(), direction, source, target1, target2);

	_segments->add(segment);

	// only for the left slice(s)

	if (direction == Left) {

		_sliceSegments[target1->getId()].push_back(segment->getId());
		_sliceSegments[target2->getId()].push_back(segment->getId());

	} else {

		_sliceSegments[source->getId()].push_back(segment->getId());
	}
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
