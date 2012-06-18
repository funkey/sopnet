#ifndef CELLTRACKER_PROBLEM_ASSEMBLER_H__
#define CELLTRACKER_PROBLEM_ASSEMBLER_H__

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include "Segments.h"
#include "SegmentVisitor.h"

class ProblemAssembler : public pipeline::SimpleProcessNode, public SegmentVisitor {

public:

	ProblemAssembler();

	void visit(const EndSegment& end);

	void visit(const ContinuationSegment& continuation);

	void visit(const BranchSegment& branch);

private:

	class SliceIdVisitor : public SegmentVisitor {

	public:

		SliceIdVisitor();

		void visit(const EndSegment& end);

		void visit(const ContinuationSegment& continuation);

		void visit(const BranchSegment& branch);

		void addId(unsigned int id);

		std::map<unsigned int, unsigned int> getSliceIdsMap();
	
	private:

		std::map<unsigned int, unsigned int> _sliceIdsMap;

		unsigned int _numSlices;
	};

	void updateOutputs();

	void collectSegments();

	void collectLinearConstraints();

	void addConsistencyConstraints();

	std::map<unsigned int, unsigned int> getSliceIdsMap();

	// a list of segments for each pair of frames
	pipeline::Inputs<Segments>         _segments;

	// linear constraints on the segments for each pair of frames
	pipeline::Inputs<LinearConstraints> _linearConstraints;

	// all segments in the problem
	pipeline::Output<Segments>         _allSegments;

	// all linear constraints on all segments
	pipeline::Output<LinearConstraints> _allLinearConstraints;

	// mapping of segment ids to a continous range of variable numbers
	pipeline::Output<std::map<unsigned int, unsigned int> > _segmentIdsToVariables;

	// the consistency constraints extracted for the segments
	LinearConstraints _consistencyConstraints;

	// a mapping from slice ids to the number of the consistency constraint it
	// is used in
	std::map<unsigned int, unsigned int> _sliceIdsMap;

	// a counter for the number of segments that came in
	unsigned int _numSegments;
};

#endif // CELLTRACKER_PROBLEM_ASSEMBLER_H__

