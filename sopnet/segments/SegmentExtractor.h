#ifndef CELLTRACKER_TRACKLET_EXTRACTOR_H__
#define CELLTRACKER_TRACKLET_EXTRACTOR_H__

#include <boost/function.hpp>

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include <sopnet/features/Overlap.h>
#include <sopnet/features/Distance.h>
#include <sopnet/slices/Slices.h>
#include <sopnet/segments/Segments.h>

class SegmentExtractor : public pipeline::SimpleProcessNode<> {

public:

	SegmentExtractor();

private:

	void onSlicesModified(const pipeline::Modified& signal);

	void onLinearConstraintsModified(const pipeline::Modified& signal);

	void updateOutputs();

	void extractSegments();

	void buildOverlapMap();

	inline bool extractSegment(boost::shared_ptr<Slice> prevSlice, Direction direction);

	inline bool extractSegment(boost::shared_ptr<Slice> prevSlice, boost::shared_ptr<Slice> nextSlice, unsigned int overlap);

	inline bool extractSegment(
			boost::shared_ptr<Slice> source,
			boost::shared_ptr<Slice> target1,
			boost::shared_ptr<Slice> target2,
			Direction direction,
			unsigned int overlap1,
			unsigned int overlap2);

	void assembleLinearConstraints();

	void assembleLinearConstraint(const LinearConstraint& sliceConstraint);

	// the slices
	pipeline::Input<Slices> _prevSlices;
	pipeline::Input<Slices> _nextSlices;

	// the linear constraints on the slices
	pipeline::Input<LinearConstraints> _prevLinearConstraints;
	pipeline::Input<LinearConstraints> _nextLinearConstraints;

	// the extracted segments and the linear constraints on them
	pipeline::Output<Segments>          _segments;
	pipeline::Output<LinearConstraints> _linearConstraints;

	// a map from slices to overlapping slices and the overlap value
	std::map<unsigned int, std::vector<std::pair<unsigned int, unsigned int> > > _nextOverlaps;
	std::map<unsigned int, std::vector<std::pair<unsigned int, unsigned int> > > _prevOverlaps;

	// a map from slice ids to segments (ids) they are used in
	std::map<unsigned int, std::vector<unsigned int> > _sliceSegments;

	// functor to compute the number of overlapping pixels between slices
	Overlap _overlap;

	// the minimal overlap between slices of one segment
	double _continuationOverlapThreshold;
	double _branchOverlapThreshold;

	// the maximal size ratio for targets in a branch
	double _branchSizeRatioThreshold;

	// the maximal slice distance between slices in branches
	double _sliceDistanceThreshold;

	// functor to compute the distance between slices
	Distance _distance;

	bool _slicesChanged;

	bool _linearCosntraintsChanged;
};

#endif // CELLTRACKER_TRACKLET_EXTRACTOR_H__

