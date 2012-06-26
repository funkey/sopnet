#include <imageprocessing/ConnectedComponent.h>
#include "BranchSegment.h"
#include "ContinuationSegment.h"
#include "EndSegment.h"
#include "GroundTruthSegmentExtractor.h"
#include "SetDifference.h"

logger::LogChannel groundtruthsegmentextractorlog("groundtruthsegmentextractorlog", "[GroundTruthSegmentExtractor] ");

GroundTruthSegmentExtractor::GroundTruthSegmentExtractor() {

	registerInput(_prevSlices, "previous slices");
	registerInput(_nextSlices, "next slices");
	registerOutput(_segments, "segments");
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

	SetDifference setDifference;

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
								setDifference(*prevSlice, *nextSlice, true),
								boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, prevSlice, nextSlice)));

		// add all possible branches to the next slice
		foreach (boost::shared_ptr<Slice> prevSlice, prevSlices)
			foreach (boost::shared_ptr<Slice> nextSlice1, nextSlices)
				foreach (boost::shared_ptr<Slice> nextSlice2, nextSlices)
					if (nextSlice1->getId() < nextSlice2->getId())
						branchSegments.push_back(
								std::make_pair(
										setDifference(*prevSlice, *nextSlice1, *nextSlice2, true),
										boost::make_shared<BranchSegment>(Segment::getNextSegmentId(), Right, prevSlice, nextSlice1, nextSlice2)));

		// add all possible branches to the previous slice
		foreach (boost::shared_ptr<Slice> nextSlice, nextSlices)
			foreach (boost::shared_ptr<Slice> prevSlice1, prevSlices)
				foreach (boost::shared_ptr<Slice> prevSlice2, prevSlices)
					if (prevSlice1->getId() < prevSlice2->getId())
						branchSegments.push_back(
								std::make_pair(
										setDifference(*nextSlice, *prevSlice1, *prevSlice2, true),
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

		unsigned int nextContinuation = 0;
		unsigned int nextBranch = 0;

		while (_remainingNextSlices.size() > 0 && _remainingPrevSlices.size() > 0) {

			if (nextContinuation == continuationSegments.size()) {

				LOG_ALL(groundtruthsegmentextractorlog)
						<< "there are no more continuation segments -- processing remaining branches"
						<< std::endl;

				for (int i = nextBranch; i < branchSegments.size(); i++)
					probeBranch(branchSegments[i].second);
				break;
			}

			if (nextBranch == branchSegments.size()) {

				LOG_ALL(groundtruthsegmentextractorlog)
						<< "there are no more branch segments -- processing remaining continuations"
						<< std::endl;

				for (int i = nextContinuation; i < continuationSegments.size(); i++)
					probeContinuation(continuationSegments[i].second);
				break;
			}

			// find the next best segment

			double continuationDifference = continuationSegments[nextContinuation].first;
			boost::shared_ptr<ContinuationSegment> continuation = continuationSegments[nextContinuation].second;

			double branchDifference = branchSegments[nextBranch].first;
			boost::shared_ptr<BranchSegment> branch = branchSegments[nextBranch].second;

			double difference = std::min(continuationDifference, branchDifference);

			LOG_ALL(groundtruthsegmentextractorlog) << "current segment set difference: " << difference << std::endl;

			// if the difference is too high (almost no overlap), stop looking
			// for segments
			if (difference >= 0.9) {

				LOG_ALL(groundtruthsegmentextractorlog) << "value too high -- stopping search" << std::endl;
				break;
			}

			if (continuationDifference < branchDifference) {

				probeContinuation(continuation);
				nextContinuation++;

			} else {

				probeBranch(branch);
				nextBranch++;
			}
		}

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

