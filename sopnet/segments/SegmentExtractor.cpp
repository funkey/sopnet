#include <boost/function.hpp>

#include <imageprocessing/ConnectedComponent.h>
#include <util/foreach.h>
#include <util/ProgramOptions.h>
#include "EndSegment.h"
#include "ContinuationSegment.h"
#include "BranchSegment.h"
#include "BranchSegment.h"
#include "SegmentExtractor.h"

static logger::LogChannel segmentextractorlog("segmentextractorlog", "[SegmentExtractor] ");

util::ProgramOption optionDistanceThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "distanceThreshold",
		util::_description_text = "The maximal center distance between slices to consider them for segment hypotheses.",
		util::_default_value    = 50);

util::ProgramOption optionOverlapThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "overlapThreshold",
		util::_description_text = "The minimal normalized overlap between slices to consider them for segment hypotheses.",
		util::_default_value    = 0.5);

SegmentExtractor::SegmentExtractor() :
	_slicesChanged(true),
	_linearCosntraintsChanged(true) {

	registerInput(_prevSlices, "previous slices");
	registerInput(_nextSlices, "next slices");
	registerInput(_prevLinearConstraints, "previous linear constraints");
	registerInput(_nextLinearConstraints, "next linear constraints", pipeline::Optional);
	registerInput(_distanceThreshold, "distance threshold", pipeline::Optional);

	registerOutput(_segments, "segments");
	registerOutput(_linearConstraints, "linear constraints");

	_prevSlices.registerBackwardCallback(&SegmentExtractor::onSlicesModified, this);
	_nextSlices.registerBackwardCallback(&SegmentExtractor::onSlicesModified, this);
	_prevLinearConstraints.registerBackwardCallback(&SegmentExtractor::onLinearConstraintsModified, this);
	_nextLinearConstraints.registerBackwardCallback(&SegmentExtractor::onLinearConstraintsModified, this);
}

void
SegmentExtractor::onSlicesModified(const pipeline::Modified& signal) {

	_slicesChanged = true;
}

void
SegmentExtractor::onLinearConstraintsModified(const pipeline::Modified& signal) {

	_linearCosntraintsChanged = true;
}

void
SegmentExtractor::updateOutputs() {

	if (_slicesChanged) {

		extractSegments();
		_slicesChanged = false;
	}

	if (_linearCosntraintsChanged) {

		assembleLinearConstraints();
		_linearCosntraintsChanged = false;
	}
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

	LOG_DEBUG(segmentextractorlog) << "extracting continuations and bisections to next section..." << std::endl;

	double distanceThreshold;

	// prefer the distance threshold that was set via the input, if available
	if (_distanceThreshold)
		distanceThreshold = *_distanceThreshold;
	else
		distanceThreshold = optionDistanceThreshold;

	// for all slices in previous section...
	foreach (boost::shared_ptr<Slice> prevSlice, *_prevSlices) {

		std::vector<boost::shared_ptr<Slice> > closeNextSlices = _nextSlices->find(prevSlice->getComponent()->getCenter(), distanceThreshold);

		LOG_ALL(segmentextractorlog) << "found " << closeNextSlices.size() << " partners" << std::endl;

		// ...and all next slices within a threshold distance
		foreach (boost::shared_ptr<Slice> nextSlice, closeNextSlices)
			extractSegment(prevSlice, nextSlice);

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

		std::vector<boost::shared_ptr<Slice> > closePrevSlices = _prevSlices->find(nextSlice->getComponent()->getCenter(), distanceThreshold);

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

	LOG_ALL(segmentextractorlog)
			<< "overlap between slice " << prevSlice->getId()
			<< " and " << nextSlice->getId() << " is "
			<< _overlap(*prevSlice, *nextSlice, false, false)
			<< ", normalized by the slice sizes (" << prevSlice->getComponent()->getSize()
			<< " and " << nextSlice->getComponent()->getSize() << ") this is "
			<< _overlap(*prevSlice, *nextSlice, true, false) << std::endl;

	if (_overlap(*prevSlice, *nextSlice, true, false) < optionOverlapThreshold.as<double>()) {

		LOG_ALL(segmentextractorlog) << "discarding this segment hypothesis" << std::endl;
		return;
	}

	LOG_ALL(segmentextractorlog) << "accepting this segment hypothesis" << std::endl;

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

	LOG_ALL(segmentextractorlog)
			<< "overlap between slice " << source->getId()
			<< " and both " << target1->getId() << " and " << target2->getId() << " is "
			<< _overlap(*target1, *target2, *source, false, false)
			<< "(" << _overlap(*target1, *source, false, false) << " + "
			<< _overlap(*target2, *source, false, false) << ")"
			<< ", normalized by the slice sizes (" << source->getComponent()->getSize()
			<< " and " << target1->getComponent()->getSize() << " and " << target2->getComponent()->getSize()
			<< ") this is " << _overlap(*target1, *target2, *source, true, false) << std::endl;

	if (_overlap(*target1, *target2, *source, true, false) < optionOverlapThreshold.as<double>()) {

		LOG_ALL(segmentextractorlog) << "discarding this segment hypothesis" << std::endl;
		return;
	}

	LOG_ALL(segmentextractorlog) << "accepting this segment hypothesis" << std::endl;

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
