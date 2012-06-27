#ifndef SOPNET_GROUND_TRUTH_SEGMENT_EXTRACTOR_H__
#define SOPNET_GROUND_TRUTH_SEGMENT_EXTRACTOR_H__

#include <pipeline/all.h>
#include <sopnet/features/SetDifference.h>
#include <sopnet/slices/Slices.h>
#include <sopnet/segments/Segments.h>

class GroundTruthSegmentExtractor : public pipeline::SimpleProcessNode {

public:

	GroundTruthSegmentExtractor();

private:

	void updateOutputs();

	/**
	 * Checks, whether the given segment is consistent with the currently found
	 * ones and adds it to the output, if this is the case.
	 */
	void probeContinuation(boost::shared_ptr<ContinuationSegment> continuation);

	/**
	 * Checks, whether the given segment is consistent with the currently found
	 * ones and adds it to the output, if this is the case.
	 */
	void probeBranch(boost::shared_ptr<BranchSegment> branch);

	/**
	 * Get the distance between two slices.
	 */
	double distance(const Slice& slice1, const Slice& slice2);

	// slices of the previous section
	pipeline::Input<Slices> _prevSlices;

	// slices of the next section
	pipeline::Input<Slices> _nextSlices;

	// extracted segments
	pipeline::Output<Segments> _segments;

	// mapping from intensity values to the slices having it
	std::map<float, std::vector<boost::shared_ptr<Slice> > > _prevSliceValues;

	// mapping from intensity values to the slices having it
	std::map<float, std::vector<boost::shared_ptr<Slice> > > _nextSliceValues;

	// all values in the ground truth images
	std::set<float> _values;

	// set of slices that have not been explained so far
	std::set<boost::shared_ptr<Slice> > _remainingPrevSlices;

	// set of slices that have not been explained so far
	std::set<boost::shared_ptr<Slice> > _remainingNextSlices;

	// the maximally allowed distance between slices in a segment
	double _maxSegmentDistance;

	// functor to compute the set difference between slices
	SetDifference _setDifference;
};

#endif // SOPNET_GROUND_TRUTH_SEGMENT_EXTRACTOR_H__

