#ifndef SOPNET_GOLD_STANDARD_EXTRACTOR_H__
#define SOPNET_GOLD_STANDARD_EXTRACTOR_H__

#include <pipeline.h>
#include <inference/LinearConstraints.h>
#include <util/point.hpp>
#include "Segments.h"
#include "SegmentVisitor.h"

class GoldStandardExtractor : public pipeline::SimpleProcessNode {

public:

	GoldStandardExtractor();

private:

	void updateOutputs();

	std::pair<std::vector<const Segment*>, std::vector<const Segment*> > findGoldStandard(
			const std::vector<const EndSegment*>&          groundTruthEndSegments,
			const std::vector<const ContinuationSegment*>& groundTruthContinuationSegments,
			const std::vector<const BranchSegment*>&       groundTruthBranchSegments,
			const std::vector<const EndSegment*>&          allEndSegments,
			const std::vector<const ContinuationSegment*>& allContinuationSegments,
			const std::vector<const BranchSegment*>&       allBranchSegments);

	std::set<const Slice*> collectSlices(
			const std::vector<const EndSegment*>&          endSegments,
			const std::vector<const ContinuationSegment*>& continuationSegments,
			const std::vector<const BranchSegment*>&       branchSegments);

	std::map<const Slice*, std::vector<const Segment*> > createSlicesToSegmentsMap(
			const std::vector<const EndSegment*>&          endSegments,
			const std::vector<const ContinuationSegment*>& continuationSegments,
			const std::vector<const BranchSegment*>&       branchSegments);

	void parseConstraint(const LinearConstraint& constraint);

	class SimilarityVisitor : public SegmentVisitor {

	public:

		SimilarityVisitor(const Segment* compare);

		void visit(const EndSegment& end);

		void visit(const ContinuationSegment& continuation);

		void visit(const BranchSegment& branch);

		float getSimilarity();

	private:

		float getSimilarity(const EndSegment& end1, const EndSegment& end2);

		float getSimilarity(const ContinuationSegment& continuation1, const ContinuationSegment& continuation2);

		float getSimilarity(const BranchSegment& branch1, const BranchSegment& branch2);

		unsigned int setDifference(const Slice& slice1, const Slice& slice2);

		const Segment* _compare;

		float _similarity;

		std::vector<bool> _pixels1;

		util::point<unsigned int> _size1;
	};

	class SliceCollectorVisitor : public SegmentVisitor {

	public:

		SliceCollectorVisitor();

		void visit(const EndSegment& end);

		void visit(const ContinuationSegment& continuation);

		void visit(const BranchSegment& branch);

		const std::vector<const Slice*>& getSlices();

	private:

		std::vector<const Slice*> _slices;

	};

	class SeparateVisitor : public SegmentVisitor {

	public:

		SeparateVisitor();

		void visit(const EndSegment& end);

		void visit(const ContinuationSegment& continuation);

		void visit(const BranchSegment& branch);

		unsigned int getNumInterSectionIntervals();

		const std::vector<const EndSegment*>& getEndSegments(unsigned int interval);

		const std::vector<const ContinuationSegment*>& getContinuationSegments(unsigned int interval);

		const std::vector<const BranchSegment*>& getBranchSegments(unsigned int interval);

	private:

		unsigned int _numIntervals;

		std::map<unsigned int, std::vector<const EndSegment*> > _endSegments;

		std::map<unsigned int, std::vector<const ContinuationSegment*> > _continuationSegments;

		std::map<unsigned int, std::vector<const BranchSegment*> > _branchSegments;
	};

	pipeline::Input<Segments> _groundTruth;

	pipeline::Input<Segments> _allSegments;

	pipeline::Output<Segments> _goldStandard;

	pipeline::Output<Segments> _negativeSamples;

	std::map<unsigned int, boost::shared_ptr<Segment> > _allSegmentIds;

	std::map<unsigned int, std::vector<const LinearConstraint*> > _segmentConstraints;
};

#endif // SOPNET_GOLD_STANDARD_EXTRACTOR_H__

