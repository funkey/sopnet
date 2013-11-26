#ifndef SOPNET_GOLD_STANDARD_EXTRACTOR_H__
#define SOPNET_GOLD_STANDARD_EXTRACTOR_H__

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include <sopnet/features/SetDifference.h>
#include <sopnet/segments/Segments.h>
#include <util/point.hpp>

class GoldStandardExtractor : public pipeline::SimpleProcessNode<> {

public:

	GoldStandardExtractor();

private:

	template <class SegmentType>
	struct Pair {

		Pair(
				float similarity_,
				boost::shared_ptr<SegmentType> segment1_,
				boost::shared_ptr<SegmentType> segment2_) :

			similarity(similarity_),
			segment1(segment1_),
			segment2(segment2_) {}

		float similarity;

		boost::shared_ptr<SegmentType> segment1;

		boost::shared_ptr<SegmentType> segment2;

		bool operator<(const Pair<SegmentType>& other) const {

			return similarity < other.similarity;
		}
	};

	void updateOutputs();

	void findGoldStandard(unsigned int interval);

	std::vector<const Slice*>
	getSlices(boost::shared_ptr<EndSegment> end);

	std::vector<const Slice*>
	getSlices(boost::shared_ptr<ContinuationSegment> continuation);

	std::vector<const Slice*>
	getSlices(boost::shared_ptr<BranchSegment> branch);

	std::set<const Slice*> collectSlices(
			const std::vector<boost::shared_ptr<EndSegment> >&          endSegments,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuationSegments,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branchSegments);

	template <typename SegmentType>
	void probe(
			boost::shared_ptr<SegmentType> gtSegment,
			boost::shared_ptr<SegmentType> gsSegment);

	template <typename SegmentType>
	std::map<const Slice*, std::vector<boost::shared_ptr<SegmentType> > >
	slicesToSegments(const std::vector<boost::shared_ptr<SegmentType> >& segments);

	unsigned int setDifference(const Slice& slice1, const Slice& slice2);

	pipeline::Input<Segments> _groundTruth;

	pipeline::Input<Segments> _allSegments;

	pipeline::Output<Segments> _goldStandard;

	pipeline::Output<Segments> _negativeSamples;

	pipeline::Output<std::map<unsigned int, double> > _groundTruthScore;

	// functor to compute the set difference between slices
	SetDifference _setDifference;

	std::map<unsigned int, boost::shared_ptr<Segment> > _allSegmentIds;

	std::map<const Slice*, std::vector<boost::shared_ptr<EndSegment> > > _endSegments;

	std::map<const Slice*, std::vector<boost::shared_ptr<ContinuationSegment> > > _continuationSegments;

	std::map<const Slice*, std::vector<boost::shared_ptr<BranchSegment> > > _branchSegments;

	std::set<const Slice*> _remainingSlices;

	double _maxEndDistance;

	double _maxContinuationDistance;

	double _maxBranchDistance;
};

#endif // SOPNET_GOLD_STANDARD_EXTRACTOR_H__

