#include <util/ProgramOptions.h>
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/BranchSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/EndSegment.h>
#include "GroundTruthSegmentExtractor.h"

logger::LogChannel groundtruthsegmentextractorlog("groundtruthsegmentextractorlog", "[GroundTruthSegmentExtractor] ");

util::ProgramOption optionMaxSegmentDistance(
		util::_module           = "sopnet",
		util::_long_name        = "groundTruthMaxSegmentDistance",
		util::_description_text = "The maximal distance between slices in a ground-truth segment.",
		util::_default_value    = 100);

GroundTruthSegmentExtractor::GroundTruthSegmentExtractor() {

	registerInput(_prevSlices, "previous slices");
	registerInput(_nextSlices, "next slices");
	registerOutput(_segments, "segments");

	_maxSegmentDistance = optionMaxSegmentDistance;
}

void
GroundTruthSegmentExtractor::updateOutputs() {

	_segments->clear();

	_prevSliceValues.clear();
	_nextSliceValues.clear();

	LOG_ALL(groundtruthsegmentextractorlog)
			<< "extracting segments between " << _prevSlices->size()
			<< " slices in the previous section and " << _nextSlices->size()
			<< " slices in the next section" << std::endl;

	// collect all slices of same intensity in previous slice
	foreach (boost::shared_ptr<Slice> slice, *_prevSlices) {

		_prevSliceValues[slice->getComponent()->getValue()].push_back(slice);
		_values.insert(slice->getComponent()->getValue());
	}

	LOG_ALL(groundtruthsegmentextractorlog) << "found " << _prevSliceValues.size() << " different values in previous section" << std::endl;

	// collect all slices of same intensity in next slice
	foreach (boost::shared_ptr<Slice> slice, *_nextSlices) {

		_nextSliceValues[slice->getComponent()->getValue()].push_back(slice);
		_values.insert(slice->getComponent()->getValue());
	}

	LOG_ALL(groundtruthsegmentextractorlog) << "found " << _nextSliceValues.size() << " different values in next section" << std::endl;

	// for each intensity, extract all segments
	foreach (float value, _values) {

		LOG_ALL(groundtruthsegmentextractorlog) << "processing value " << value << std::endl;

		const std::vector<boost::shared_ptr<Slice> >& prevSlices = _prevSliceValues[value];
		const std::vector<boost::shared_ptr<Slice> >& nextSlices = _nextSliceValues[value];

		LOG_ALL(groundtruthsegmentextractorlog)
				<< "have to connect " << prevSlices.size() << " slices in previous section to "
				<< nextSlices.size() << " slices in next section" << std::endl;

		// a list of all possible non-end segments between the ground truth slices
		std::vector<std::pair<double, boost::shared_ptr<ContinuationSegment> > > continuationSegments;
		std::vector<std::pair<double, boost::shared_ptr<BranchSegment> > >       branchSegments;

		// add all possible continuations
		foreach (boost::shared_ptr<Slice> prevSlice, prevSlices)
			foreach (boost::shared_ptr<Slice> nextSlice, nextSlices)
				continuationSegments.push_back(
						std::make_pair(
								distance(*prevSlice, *nextSlice),
								boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, prevSlice, nextSlice)));

		// add all possible branches to the next slice
		foreach (boost::shared_ptr<Slice> prevSlice, prevSlices)
			foreach (boost::shared_ptr<Slice> nextSlice1, nextSlices)
				foreach (boost::shared_ptr<Slice> nextSlice2, nextSlices)
					if (nextSlice1->getId() < nextSlice2->getId())
						branchSegments.push_back(
								std::make_pair(
										distance(*prevSlice, *nextSlice1) + distance(*prevSlice, *nextSlice2),
										boost::make_shared<BranchSegment>(Segment::getNextSegmentId(), Right, prevSlice, nextSlice1, nextSlice2)));

		// add all possible branches to the previous slice
		foreach (boost::shared_ptr<Slice> nextSlice, nextSlices)
			foreach (boost::shared_ptr<Slice> prevSlice1, prevSlices)
				foreach (boost::shared_ptr<Slice> prevSlice2, prevSlices)
					if (prevSlice1->getId() < prevSlice2->getId())
						branchSegments.push_back(
								std::make_pair(
										distance(*nextSlice, *prevSlice1) + distance(*nextSlice, *prevSlice2),
										boost::make_shared<BranchSegment>(Segment::getNextSegmentId(), Left, nextSlice, prevSlice1, prevSlice2)));

		LOG_ALL(groundtruthsegmentextractorlog) << "considering " << continuationSegments.size() << " possible continuation segments" << std::endl;
		LOG_ALL(groundtruthsegmentextractorlog) << "considering " << branchSegments.size() << " possible branch segments" << std::endl;

		// sort segments based on their set difference ratio
		std::sort(continuationSegments.begin(), continuationSegments.end());
		std::sort(branchSegments.begin(), branchSegments.end());

		// create sets of remaining slices
		_remainingPrevSlices.clear();
		_remainingNextSlices.clear();
		std::copy(prevSlices.begin(), prevSlices.end(), std::inserter(_remainingPrevSlices, _remainingPrevSlices.begin()));
		std::copy(nextSlices.begin(), nextSlices.end(), std::inserter(_remainingNextSlices, _remainingNextSlices.begin()));

		// accept all branches that have spatial overlap
		double distance;
		boost::shared_ptr<BranchSegment> branch;
		foreach (boost::tie(distance, branch), branchSegments)
			probeBranch(branch);

		// accept all remaining possible continuations
		boost::shared_ptr<ContinuationSegment> continuation;
		foreach (boost::tie(distance, continuation), continuationSegments)
			probeContinuation(continuation);

		LOG_ALL(groundtruthsegmentextractorlog)
				<< "have " << _remainingPrevSlices.size()
				<< " slices in previous section left over" << std::endl;

		LOG_ALL(groundtruthsegmentextractorlog)
				<< "have " << _remainingNextSlices.size()
				<< " slices in next section left over" << std::endl;

		// all remaining slices must have ended
		foreach (boost::shared_ptr<Slice> prevSlice, _remainingPrevSlices)
			_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Right, prevSlice));

		foreach (boost::shared_ptr<Slice> nextSlice, _remainingNextSlices)
			_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Left, nextSlice));
	}
}

void
GroundTruthSegmentExtractor::probeContinuation(boost::shared_ptr<ContinuationSegment> continuation) {

	if (distance(*continuation->getSourceSlice(), *continuation->getTargetSlice()) > _maxSegmentDistance)
		return;

	if (continuation->getDirection() == Left) {

		if (_remainingPrevSlices.count(continuation->getTargetSlice()) == 0)
			return;

		if (_remainingNextSlices.count(continuation->getSourceSlice()) == 0)
			return;

		_segments->add(continuation);

		_remainingPrevSlices.erase(continuation->getTargetSlice());
		_remainingNextSlices.erase(continuation->getSourceSlice());

	} else {

		if (_remainingNextSlices.count(continuation->getTargetSlice()) == 0)
			return;

		if (_remainingPrevSlices.count(continuation->getSourceSlice()) == 0)
			return;

		_segments->add(continuation);

		_remainingPrevSlices.erase(continuation->getSourceSlice());
		_remainingNextSlices.erase(continuation->getTargetSlice());
	}
}

void
GroundTruthSegmentExtractor::probeBranch(boost::shared_ptr<BranchSegment> branch) {

	// be picky about branches:

	LOG_ALL(groundtruthsegmentextractorlog)
			<< "value is " << branch->getSourceSlice()->getComponent()->getValue() << ", "
			<< "evaluating branch from " << std::endl
			<< branch->getSourceSlice()->getComponent()->getCenter() << " to "
			<< branch->getTargetSlice1()->getComponent()->getCenter() << " and "
			<< branch->getTargetSlice2()->getComponent()->getCenter() << std::endl;

	LOG_ALL(groundtruthsegmentextractorlog)
		<< _setDifference(*branch->getSourceSlice(), *branch->getTargetSlice1(), true, false) << " "
		<< _setDifference(*branch->getSourceSlice(), *branch->getTargetSlice2(), true, false) << std::endl;

	// there has to be overlap between source and both targets
	if (_setDifference(*branch->getSourceSlice(), *branch->getTargetSlice1(), true, false) > 0.9)
		return;
	if (_setDifference(*branch->getSourceSlice(), *branch->getTargetSlice2(), true, false) > 0.9)
		return;

	// targets shouldn't move too much
	if (distance(*branch->getSourceSlice(), *branch->getTargetSlice1()) > _maxSegmentDistance)
		return;
	if (distance(*branch->getSourceSlice(), *branch->getTargetSlice2()) > _maxSegmentDistance)
		return;

	if (branch->getDirection() == Left) {

		if (_remainingPrevSlices.count(branch->getTargetSlice1()) == 0)
			return;

		if (_remainingPrevSlices.count(branch->getTargetSlice2()) == 0)
			return;

		if (_remainingNextSlices.count(branch->getSourceSlice()) == 0)
			return;

		_segments->add(branch);

		_remainingPrevSlices.erase(branch->getTargetSlice1());
		_remainingPrevSlices.erase(branch->getTargetSlice2());
		_remainingNextSlices.erase(branch->getSourceSlice());

	} else {

		if (_remainingNextSlices.count(branch->getTargetSlice1()) == 0)
			return;

		if (_remainingNextSlices.count(branch->getTargetSlice2()) == 0)
			return;

		if (_remainingPrevSlices.count(branch->getSourceSlice()) == 0)
			return;

		_segments->add(branch);

		_remainingNextSlices.erase(branch->getTargetSlice1());
		_remainingNextSlices.erase(branch->getTargetSlice2());
		_remainingPrevSlices.erase(branch->getSourceSlice());
	}
}

double
GroundTruthSegmentExtractor::distance(const Slice& slice1, const Slice& slice2) {

	const util::point<double>& center1 = slice1.getComponent()->getCenter();
	const util::point<double>& center2 = slice2.getComponent()->getCenter();

	util::point<double> diff = center1 - center2;

	return sqrt(diff.x*diff.x + diff.y*diff.y);
}

