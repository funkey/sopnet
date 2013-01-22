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

util::ProgramOption optionContinuationOverlapThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "continuationOverlapThreshold",
		util::_description_text = "The minimal normalized overlap between slices to consider them for continuation segment hypotheses.",
		util::_default_value    = 0.5);

util::ProgramOption optionMinContinuationPartners(
		util::_module           = "sopnet.segments",
		util::_long_name        = "minContinuationPartners",
		util::_description_text = "The minimal number of continuation partners for each slice, even if they do not meet the overlap threshold.",
		util::_default_value    = 0);

util::ProgramOption optionBranchOverlapThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "branchOverlapThreshold",
		util::_description_text = "The minimal normalized overlap between slices to consider them for branch segment hypotheses.",
		util::_default_value    = 0.5);

util::ProgramOption optionBranchSizeRatioThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "branchSizeRatioThreshold",
		util::_description_text = "The maximal size ratio (between 0 and 1) of the two target slices of a branch.",
		util::_default_value    = 0.5);

util::ProgramOption optionSliceDistanceThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "sliceDistanceThreshold",
		util::_description_text = "The maximal slice distance between slices to consider them for segment hypotheses."
		                          "The slice distance is the average minimal distance of a pixel from one slice to any "
		                          "pixel of another slice.",
		util::_default_value    = 10);

util::ProgramOption optionDisableBranches(
		util::_module           = "sopnet.segments",
		util::_long_name        = "disableBranches",
		util::_description_text = "Disable the extraction of branch segments.");


SegmentExtractor::SegmentExtractor() :
	_overlap(false /* don't normalize */, false /* don't align */),
	_continuationOverlapThreshold(optionContinuationOverlapThreshold.as<double>()),
	_branchOverlapThreshold(optionBranchOverlapThreshold.as<double>()),
	_minContinuationPartners(optionMinContinuationPartners.as<unsigned int>()),
	_branchSizeRatioThreshold(optionBranchSizeRatioThreshold.as<double>()),
	_sliceDistanceThreshold(optionSliceDistanceThreshold.as<double>()),
	_slicesChanged(true),
	_linearCosntraintsChanged(true) {

	registerInput(_prevSlices, "previous slices");
	registerInput(_nextSlices, "next slices");
	registerInput(_prevLinearConstraints, "previous linear constraints");
	registerInput(_nextLinearConstraints, "next linear constraints", pipeline::Optional);

	registerOutput(_segments, "segments");
	registerOutput(_linearConstraints, "linear constraints");

	_prevSlices.registerBackwardCallback(&SegmentExtractor::onSlicesModified, this);
	_nextSlices.registerBackwardCallback(&SegmentExtractor::onSlicesModified, this);
	_prevLinearConstraints.registerBackwardCallback(&SegmentExtractor::onLinearConstraintsModified, this);
	_nextLinearConstraints.registerBackwardCallback(&SegmentExtractor::onLinearConstraintsModified, this);
}

void
SegmentExtractor::onSlicesModified(const pipeline::Modified&) {

	_slicesChanged = true;
}

void
SegmentExtractor::onLinearConstraintsModified(const pipeline::Modified&) {

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

	_distance.clearCache();
}

void
SegmentExtractor::extractSegments() {

	LOG_DEBUG(segmentextractorlog)
			<< "previous sections contains " << _prevSlices->size() << " slices,"
			<< "next sections contains "     << _nextSlices->size() << " slices" << std::endl;

	buildOverlapMap();

	unsigned int oldSize = 0;

	LOG_DEBUG(segmentextractorlog) << "extracting segments..." << std::endl;

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

		LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
		oldSize = _segments->size();
	}

	LOG_DEBUG(segmentextractorlog) << "extracting continuations to next section..." << std::endl;

	// for all slices in previous section...
	for (unsigned int i = 0; i < _prevSlices->size(); i++) {

		LOG_ALL(segmentextractorlog) << "found " << _nextOverlaps[i].size() << " partners" << std::endl;

		// ...and all overlapping slices in the next section...
		unsigned int j, overlap;
		foreach (boost::tie(overlap, j), _nextOverlaps[i]) {

			// ...try to extract the segment
			extractSegment((*_prevSlices)[i], (*_nextSlices)[j], overlap);
		}
	}

	LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	oldSize = _segments->size();

	LOG_DEBUG(segmentextractorlog) << "ensuring at least " << _minContinuationPartners << " continuation partners for each slice..." << std::endl;

	ensureMinContinuationPartners();

	LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	oldSize = _segments->size();

	if (!optionDisableBranches) {

		LOG_DEBUG(segmentextractorlog) << "extracting bisections from previous to next section..." << std::endl;

		for (unsigned int i = 0; i < _prevSlices->size(); i++) {

			for (unsigned int j1_index = 0; j1_index < _nextOverlaps[i].size(); j1_index++)
				for (unsigned int j2_index = j1_index + 1; j2_index < _nextOverlaps[i].size(); j2_index++) {

					const unsigned int j1 = _nextOverlaps[i][j1_index].second;
					const unsigned int j2 = _nextOverlaps[i][j2_index].second;

					const unsigned int overlap1 = _nextOverlaps[i][j1_index].first;
					const unsigned int overlap2 = _nextOverlaps[i][j2_index].first;

					if (!_nextSlices->areConflicting((*_nextSlices)[j1]->getId(), (*_nextSlices)[j2]->getId())) {

						extractSegment((*_prevSlices)[i], (*_nextSlices)[j1], (*_nextSlices)[j2], Right, overlap1, overlap2);
					}
				}
		}

		LOG_DEBUG(segmentextractorlog) << "extracting bisections from next to previous section..." << std::endl;

		for (unsigned int i = 0; i < _nextSlices->size(); i++) {

			for (unsigned int j1_index = 0; j1_index < _prevOverlaps[i].size(); j1_index++)
				for (unsigned int j2_index = j1_index + 1; j2_index < _prevOverlaps[i].size(); j2_index++) {

					const unsigned int j1 = _prevOverlaps[i][j1_index].second;
					const unsigned int j2 = _prevOverlaps[i][j2_index].second;

					const unsigned int overlap1 = _prevOverlaps[i][j1_index].first;
					const unsigned int overlap2 = _prevOverlaps[i][j2_index].first;

					if (!_prevSlices->areConflicting((*_prevSlices)[j1]->getId(), (*_prevSlices)[j2]->getId())) {

						extractSegment((*_nextSlices)[i], (*_prevSlices)[j1], (*_prevSlices)[j2], Left, overlap1, overlap2);
					}
				}
		}

		LOG_DEBUG(segmentextractorlog) << _segments->size() << " segments extraced so far (+" << (_segments->size() - oldSize) << ")" << std::endl;
	}

	LOG_DEBUG(segmentextractorlog) << "extracted " << _segments->size() << " segments in total" << std::endl;
}

void
SegmentExtractor::ensureMinContinuationPartners() {

	// for all slices with fewer than the required number of partners
	for (unsigned int i = 0; i < _prevSlices->size(); i++) {

		unsigned int prevId = (*_prevSlices)[i]->getId();

		unsigned int numPartners = _continuationPartners[prevId].size();

		if (numPartners < _minContinuationPartners) {

			// ...and all overlapping slices in the next section...
			unsigned int j, overlap;
			foreach (boost::tie(overlap, j), _nextOverlaps[i]) {

				unsigned int nextId = (*_nextSlices)[j]->getId();

				// ...if not already a partner...
				if (std::count(_continuationPartners[prevId].begin(), _continuationPartners[prevId].end(), nextId))
					continue;

				// ...extract the segment
				extractSegment((*_prevSlices)[i], (*_nextSlices)[j]);

				numPartners++;

				if (numPartners == _minContinuationPartners)
					break;
			}
		}
	}

	// for all slices with fewer than the required number of partners
	for (unsigned int i = 0; i < _nextSlices->size(); i++) {

		unsigned int nextId = (*_nextSlices)[i]->getId();

		unsigned int numPartners = _continuationPartners[nextId].size();

		if (numPartners < _minContinuationPartners) {

			// ...and all overlapping slices in the prev section...
			unsigned int j, overlap;
			foreach (boost::tie(overlap, j), _prevOverlaps[i]) {

				unsigned int prevId = (*_prevSlices)[j]->getId();

				// ...if not already a partner...
				if (std::count(_continuationPartners[nextId].begin(), _continuationPartners[nextId].end(), prevId))
					continue;

				// ...extract the segment
				extractSegment((*_prevSlices)[j], (*_nextSlices)[i]);

				numPartners++;

				if (numPartners == _minContinuationPartners)
					break;
			}
		}
	}
}

void
SegmentExtractor::buildOverlapMap() {

	LOG_DEBUG(segmentextractorlog) << "building overlap maps..." << std::endl;

	_prevOverlaps.clear();
	_nextOverlaps.clear();

	for (unsigned int i = 0; i < _prevSlices->size(); i++) {

		for (unsigned int j = 0; j < _nextSlices->size(); j++) {

			const Slice& prev = *(*_prevSlices)[i];
			const Slice& next = *(*_nextSlices)[j];

			double value;

			if (_overlap.exceeds(prev, next, 0, value)) {

				_nextOverlaps[i].push_back(std::make_pair(static_cast<unsigned int>(value), j));
				_prevOverlaps[j].push_back(std::make_pair(static_cast<unsigned int>(value), i));
			}
		}

		if (i % (std::max(static_cast<unsigned int>(1), _prevSlices->size()/10)) == 0) {

			LOG_DEBUG(segmentextractorlog) << round(static_cast<double>(i)*100/std::max(static_cast<unsigned int>(1), _prevSlices->size())) << "%" << std::endl;
		}
	}

	LOG_DEBUG(segmentextractorlog) << "done." << std::endl;
}

bool
SegmentExtractor::extractSegment(boost::shared_ptr<Slice> slice, Direction direction) {

	boost::shared_ptr<EndSegment> segment = boost::make_shared<EndSegment>(Segment::getNextSegmentId(), direction, slice);

	_segments->add(segment);

	// only for ends that have the slice on the left side
	if (direction == Right)
		_sliceSegments[slice->getId()].push_back(segment->getId());

	return true;
}

bool
SegmentExtractor::extractSegment(boost::shared_ptr<Slice> prevSlice, boost::shared_ptr<Slice> nextSlice, unsigned int overlap) {

	double normalizedOverlap = Overlap::normalize(*prevSlice, *nextSlice, overlap);

	if (normalizedOverlap < _continuationOverlapThreshold) {

		LOG_ALL(segmentextractorlog) << "discarding this segment hypothesis" << std::endl;
		return false;
	}

	LOG_ALL(segmentextractorlog) << "accepting this segment hypothesis" << std::endl;

	extractSegment(prevSlice, nextSlice);

	return true;
}


void
SegmentExtractor::extractSegment(boost::shared_ptr<Slice> prevSlice, boost::shared_ptr<Slice> nextSlice) {

	boost::shared_ptr<ContinuationSegment> segment = boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, prevSlice, nextSlice);

	_segments->add(segment);

	// only for the left slice
	_sliceSegments[prevSlice->getId()].push_back(segment->getId());

	_continuationPartners[prevSlice->getId()].push_back(nextSlice->getId());
	_continuationPartners[nextSlice->getId()].push_back(prevSlice->getId());
}

bool
SegmentExtractor::extractSegment(
		boost::shared_ptr<Slice> source,
		boost::shared_ptr<Slice> target1,
		boost::shared_ptr<Slice> target2,
		Direction direction,
		unsigned int overlap1,
		unsigned int overlap2) {

	double normalizedOverlap = Overlap::normalize(*target1, *target2, *source, overlap1 + overlap2);

	if (normalizedOverlap > 1) {

		LOG_DEBUG(segmentextractorlog) << normalizedOverlap << std::endl;
		LOG_DEBUG(segmentextractorlog) << overlap1 << std::endl;
		LOG_DEBUG(segmentextractorlog) << overlap2 << std::endl;
		LOG_DEBUG(segmentextractorlog) << target1->getComponent()->getSize() << std::endl;
		LOG_DEBUG(segmentextractorlog) << target2->getComponent()->getSize() << std::endl;
		LOG_DEBUG(segmentextractorlog) << source->getComponent()->getSize() << std::endl;
		LOG_DEBUG(segmentextractorlog) << std::endl;
	}

	if (normalizedOverlap < _branchOverlapThreshold)
		return false;

	unsigned int size1 = target1->getComponent()->getSize();
	unsigned int size2 = target2->getComponent()->getSize();

	double sizeRatio = static_cast<double>(std::min(size1, size2))/std::max(size1, size2);

	if (sizeRatio < _branchSizeRatioThreshold)
		return false;

	//double avgSliceDistance, maxSliceDistance;

	//_distance(*target1, *target2, *source, true, false, avgSliceDistance, maxSliceDistance);

	//if (maxSliceDistance >= _sliceDistanceThreshold)
		//return;

	boost::shared_ptr<BranchSegment> segment = boost::make_shared<BranchSegment>(Segment::getNextSegmentId(), direction, source, target1, target2);

	_segments->add(segment);

	// only for the left slice(s)

	if (direction == Left) {

		_sliceSegments[target1->getId()].push_back(segment->getId());
		_sliceSegments[target2->getId()].push_back(segment->getId());

	} else {

		_sliceSegments[source->getId()].push_back(segment->getId());
	}

	return true;
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
