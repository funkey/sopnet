#ifndef CELLTRACKER_TRACKLET_EXTRACTOR_H__
#define CELLTRACKER_TRACKLET_EXTRACTOR_H__

#include <boost/function.hpp>

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include <sopnet/slices/ConflictSets.h>
#include <sopnet/features/Overlap.h>
#include <sopnet/features/Distance.h>
#include <sopnet/slices/Slices.h>
#include <sopnet/segments/Segments.h>

class SegmentExtractor : public pipeline::SimpleProcessNode<> {

public:

	SegmentExtractor();

private:

	void onSlicesModified(const pipeline::Modified& signal);

	void onConflictSetsModified(const pipeline::Modified& signal);

	void updateOutputs();

	void ensureMinContinuationPartners();

	void extractSegments();

	void buildOverlapMap();

	inline bool extractSegment(boost::shared_ptr<Slice> prevSlice, Direction direction);

	inline bool extractSegment(boost::shared_ptr<Slice> prevSlice, boost::shared_ptr<Slice> nextSlice, unsigned int overlap);

	inline void extractSegment(boost::shared_ptr<Slice> prevSlice, boost::shared_ptr<Slice> nextSlice);

	inline bool extractSegment(
			boost::shared_ptr<Slice> source,
			boost::shared_ptr<Slice> target1,
			boost::shared_ptr<Slice> target2,
			Direction direction,
			unsigned int overlap1,
			unsigned int overlap2);

	void assembleLinearConstraints();

	void assembleLinearConstraint(const ConflictSet& conflictSet);

	// the slices
	pipeline::Input<Slices> _prevSlices;
	pipeline::Input<Slices> _nextSlices;

	// the conflict sets on the slices
	pipeline::Input<ConflictSets> _prevConflictSets;
	pipeline::Input<ConflictSets> _nextConflictSets;

	// force exactly one segment per slice conflict set
	pipeline::Input<bool> _forceExplanation;

	// the extracted segments and the linear constraints on them
	pipeline::Output<Segments>          _segments;
	pipeline::Output<LinearConstraints> _linearConstraints;

	// a map from slices to overlapping slices and the overlap value
	typedef boost::shared_ptr<Slice>           slice_ptr;
	typedef std::pair<unsigned int, slice_ptr> overlap_slice_pair;
	std::map<slice_ptr, std::vector<overlap_slice_pair> > _nextOverlaps;
	std::map<slice_ptr, std::vector<overlap_slice_pair> > _prevOverlaps;

	// comparator to sort the vectors of _nextOverlaps and _prevOverlaps by 
	// overlap
	struct OverlapCompare {

		bool operator()(const overlap_slice_pair& a, const overlap_slice_pair& b) {

			// overlaps are the same -- sort by id to ensure same sorted 
			// sequence every time
			if (a.first == b.first)
				return a.second->hashValue() < b.second->hashValue();

			// sort by overlap
			return a.first < b.first;
		}
	};

	// map from slice ids to slice ids if connected by a continuation
	std::map<unsigned int, std::vector<unsigned int> > _continuationPartners;

	// a map from slice ids to segments (ids) they are used in
	std::map<unsigned int, std::vector<unsigned int> > _sliceSegments;

	// functor to compute the number of overlapping pixels between slices
	Overlap _overlap;

	// the minimal overlap between slices of one segment
	double _continuationOverlapThreshold;
	double _branchOverlapThreshold;

	// the minimal number of continuation partners per slice
	unsigned int _minContinuationPartners;

	// the maximal size ratio for targets in a branch
	double _branchSizeRatioThreshold;

	// the maximal slice distance between slices in branches
	double _sliceDistanceThreshold;

	// functor to compute the distance between slices
	Distance _distance;

	bool _slicesChanged;

	bool _conflictSetsChanged;
};

#endif // CELLTRACKER_TRACKLET_EXTRACTOR_H__

