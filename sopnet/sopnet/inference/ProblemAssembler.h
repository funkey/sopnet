#ifndef CELLTRACKER_PROBLEM_ASSEMBLER_H__
#define CELLTRACKER_PROBLEM_ASSEMBLER_H__

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include <sopnet/features/Overlap.h>
#include <sopnet/segments/Segments.h>
#include "ProblemConfiguration.h"

class ProblemAssembler : public pipeline::SimpleProcessNode<> {

public:

	ProblemAssembler();

private:

	void updateOutputs();

	void collectSegments();

	void addExplanationConstraints();

	void addConsistencyConstraints();

	void addMitochondriaConstraints();

	void addSynapseConstraints();

	void mapConstraints(boost::shared_ptr<LinearConstraints> linearConstraints);

	void setCoefficient(const EndSegment& end);

	void setCoefficient(const ContinuationSegment& continuation);

	void setCoefficient(const BranchSegment& branch);

	void extractSliceIdsMap();

	void addSlices(const EndSegment& end);

	void addSlices(const ContinuationSegment& continuation);

	void addSlices(const BranchSegment& branch);

	void addId(unsigned int id);

	void extractMitochondriaEnclosingNeuronSegments();

	void extractSynapseEnclosingNeuronSegments();

	bool encloses(
			boost::shared_ptr<Segment> neuronSegment,
			boost::shared_ptr<Segment> otherSegment,
			double enclosingThreshold);

	unsigned int getOverlap(
			const std::vector<boost::shared_ptr<Slice> >& slices1,
			const std::vector<boost::shared_ptr<Slice> >& slices2);

	std::vector<unsigned int>& getMitochondriaEnclosingNeuronSegments(unsigned int otherSegmentId);

	std::vector<unsigned int>& getSynapseEnclosingNeuronSegments(unsigned int otherSegmentId);

	unsigned int getSliceNum(unsigned int sliceId);

	// a list of neuron segments for each pair of frames
	pipeline::Inputs<Segments>         _neuronSegments;

	// neuron linear constraints on the segments for each pair of frames
	pipeline::Inputs<LinearConstraints> _neuronLinearConstraints;

	// a list of mitochondria segments for each pair of frames
	pipeline::Inputs<Segments>         _mitochondriaSegments;

	// mitochondria linear constraints on the segments for each pair of frames
	pipeline::Inputs<LinearConstraints> _mitochondriaLinearConstraints;

	// a list of synapse segments for each pair of frames
	pipeline::Inputs<Segments>         _synapseSegments;

	// synapse linear constraints on the segments for each pair of frames
	pipeline::Inputs<LinearConstraints> _synapseLinearConstraints;

	// all segments in the problem
	pipeline::Output<Segments>          _allSegments;

	// all neuron segments in the problem
	pipeline::Output<Segments>          _allNeuronSegments;

	// all mitochondria segments in the problem
	pipeline::Output<Segments>          _allMitochondriaSegments;

	// all synapse segments in the problem
	pipeline::Output<Segments>          _allSynapseSegments;

	// all linear constraints on all segments
	pipeline::Output<LinearConstraints> _allLinearConstraints;

	// mapping of segment ids to a continous range of variable numbers
	pipeline::Output<ProblemConfiguration> _problemConfiguration;

	// the consistency constraints extracted for the segments
	LinearConstraints _consistencyConstraints;

	// the mitochondria constraints
	LinearConstraints _mitochondriaConstraints;

	// the synapse constraints
	LinearConstraints _synapseConstraints;

	// a mapping from slice ids to the number of the consistency constraint it
	// is used in
	std::map<unsigned int, unsigned int> _sliceIdsMap;

	// map from mitochondria semgment ids to enclosing neuron segment ids
	std::map<unsigned int, std::vector<unsigned int> > _mitochondriaEnclosingNeuronSegments;

	// map from synapse semgment ids to enclosing neuron segment ids
	std::map<unsigned int, std::vector<unsigned int> > _synapseEnclosingNeuronSegments;

	// a counter for the number of segments that came in
	unsigned int _numSegments;

	// number of mitochondria segments
	unsigned int _numMitochondriaSegments;

	// number of synapse segments
	unsigned int _numSynapseSegments;

	// the total number of slices
	unsigned int _numSlices;

	// functor to compute the overlap between slices
	Overlap _overlap;

	// min ratio of overlap to size to consider a mitochondria or synapse 
	// segment to be enclosed by a neuron segment
	double _enclosingMitochondriaThreshold;

	// min ratio of overlap to size to consider a mitochondria or synapse 
	// segment to be enclosed by a neuron segment
	double _enclosingSynapseThreshold;
};

#endif // CELLTRACKER_PROBLEM_ASSEMBLER_H__

