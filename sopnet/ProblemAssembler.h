#ifndef CELLTRACKER_PROBLEM_ASSEMBLER_H__
#define CELLTRACKER_PROBLEM_ASSEMBLER_H__

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include "Segments.h"

class ProblemAssembler : public pipeline::SimpleProcessNode {

public:

	ProblemAssembler();

private:

	void updateOutputs();

	void collectSegments();

	void collectLinearConstraints();

	void addConsistencyConstraints();

	void setCoefficient(const EndSegment& end);

	void setCoefficient(const ContinuationSegment& continuation);

	void setCoefficient(const BranchSegment& branch);

	void extractSliceIdsMap();

	void addSlices(const EndSegment& end);

	void addSlices(const ContinuationSegment& continuation);

	void addSlices(const BranchSegment& branch);

	void addId(unsigned int id);

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

	// the total number of slices
	unsigned int _numSlices;
};

#endif // CELLTRACKER_PROBLEM_ASSEMBLER_H__

