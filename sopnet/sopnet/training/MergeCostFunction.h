#ifndef SOPNET_TRAINING_MERGE_COST_FUNCTION_H__
#define SOPNET_TRAINING_MERGE_COST_FUNCTION_H__

#include <pipeline/SimpleProcessNode.h>
#include <pipeline/Value.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/segments/Segments.h>

// forward declarations
class EndSegment;
class ContinuationSegment;
class BranchSegment;

class MergeCostFunction : public pipeline::SimpleProcessNode<> {

	typedef boost::function<
			void
			(const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			 const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			 const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			 std::vector<double>& costs)>
			costs_function_type;

public:

	MergeCostFunction();

private:


	void updateOutputs();

	void costs(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			std::vector<double>& costs);

	double costs(const Segment& segment);

	void getGtLabels(const Segment& segment);

	int segmentSize(const Segment& segment);

	pipeline::Input<ImageStack> _groundTruth;

	pipeline::Output<costs_function_type> _costFunction;

	std::map<float, unsigned int> _gtLabels;

	// per pixel reward (i.e., this number is supposed to be negative) for each 
	// correctly merged pixel
	double _correctlyMergedPairReward;

	// number of incorrectly merged pixels after which a segment is considered a 
	// false merge
	unsigned int _incorrectlyMergedThreshold;

	// per segment costs for false merges
	double _falseMergeCosts;
};

#endif // SOPNET_TRAINING_MERGE_COST_FUNCTION_H__

