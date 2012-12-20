#ifndef SOPNET_INFERENCE_PROBLEM_GRAPH_WRITER_H__
#define SOPNET_INFERENCE_PROBLEM_GRAPH_WRITER_H__

#include <pipeline/all.h>

#include <inference/LinearConstraints.h>
#include <inference/LinearObjective.h>
#include <sopnet/segments/Segments.h>

/**
 * A sink process node that dumps a problem (i.e., sets of Segments and
 * LinearConstraints) as a lemon graph structure.
 */
class ProblemGraphWriter : public pipeline::SimpleProcessNode<> {

public:

	ProblemGraphWriter();

	void write(
			const std::string& slicesFile,
			const std::string& segmentsFile,
			const std::string& constraintsFile,
			const std::string& sliceImageDirectory);

private:

	// we produce nothing
	void updateOutputs() {};

	void writeSlices(
			const std::string& slicesFile,
			const std::string& sliceImageDirectory);

	void writeSegments(const std::string& segmentsFile);

	void writeConstraints();

	void writeSlice(const Slice& slice, std::ofstream& out);

	void writeSliceImage(const Slice& slice, const std::string& sliceImageDirectory);

	void writeSegment(const Segment& segment, std::ofstream& out);

	// all extracted segments
	pipeline::Input<Segments> _segments;

	// map from segment ids to variable numbers in the linear constraints
	pipeline::Input<std::map<unsigned int, unsigned int> > _segmentIdsToVariables;

	// the segment costs
	pipeline::Input<LinearObjective> _objective;

	// all linear constraints on the slices (one set per section)
	pipeline::Inputs<LinearConstraints> _linearConstraints;
};

#endif // SOPNET_INFERENCE_PROBLEM_GRAPH_WRITER_H__

