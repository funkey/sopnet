#include <imageprocessing/ConnectedComponent.h>
#include "BranchSegment.h"
#include "ContinuationSegment.h"
#include "EndSegment.h"
#include "GroundTruthSegmentExtractor.h"

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

	LOG_ALL(groundtruthsegmentextractorlog) << "found " << _values.size() << " neurons in previous section" << std::endl;

	// collect all slices of same intensity in next slice
	foreach (boost::shared_ptr<Slice> slice, *_nextSlices) {

		_nextSliceValues[slice->getComponent()->getValue()].push_back(slice);
		_values.insert(slice->getComponent()->getValue());
	}

	LOG_ALL(groundtruthsegmentextractorlog) << "found " << _values.size() << " in previous and next section" << std::endl;

	// for each intensity, create a segment
	foreach (float value, _values) {

		const std::vector<boost::shared_ptr<Slice> >& prevSlices = _prevSliceValues[value];
		const std::vector<boost::shared_ptr<Slice> >& nextSlices = _nextSliceValues[value];

		if (prevSlices.size() == 0 && nextSlices.size() == 1) {

			createEnd(Left, nextSlices[0]);

		} else if (prevSlices.size() == 1 && nextSlices.size() == 0) {

			createEnd(Right, prevSlices[0]);

		} else if (prevSlices.size() == 1 && nextSlices.size() == 1) {

			createContinuation(prevSlices[0], nextSlices[0]);

		} else if (prevSlices.size() == 1 && nextSlices.size() == 2) {

			createBranch(Right, prevSlices[0], nextSlices[0], nextSlices[1]);

		} else if (prevSlices.size() == 2 && nextSlices.size() == 1) {

			createBranch(Left, nextSlices[0], prevSlices[0], prevSlices[1]);

		} else {

			LOG_ERROR(groundtruthsegmentextractorlog)
					<< "there is a connection from " << prevSlices.size() << " slice to "
					<< nextSlices.size() << " slice and I don't know how to handle that!"
					<< std::endl;
		}
	}
}

void
GroundTruthSegmentExtractor::createEnd(Direction direction, boost::shared_ptr<Slice> slice) {

	LOG_ALL(groundtruthsegmentextractorlog) << "found an end, from slice " << slice->getSection() << ", " << direction << std::endl;

	_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), direction, slice));
}

void
GroundTruthSegmentExtractor::createContinuation(boost::shared_ptr<Slice> prev, boost::shared_ptr<Slice> next) {

	LOG_ALL(groundtruthsegmentextractorlog) << "found a continuation, from slice " << prev->getSection() << " to " << next->getSection() << std::endl;

	_segments->add(boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, prev, next));
}

void
GroundTruthSegmentExtractor::createBranch(Direction direction, boost::shared_ptr<Slice> source, boost::shared_ptr<Slice> target1, boost::shared_ptr<Slice> target2) {

	LOG_ALL(groundtruthsegmentextractorlog) << "found a branch , from slice " << source->getSection() << " to " << target1->getSection() << ", " << direction << std::endl;

	_segments->add(boost::make_shared<BranchSegment>(Segment::getNextSegmentId(), direction, source, target1, target2));
}
