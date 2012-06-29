#ifndef SOPNET_INFERENCE_SEGMENTATION_COST_FUNCTION_H__
#define SOPNET_INFERENCE_SEGMENTATION_COST_FUNCTION_H__

#include <imageprocessing/ImageStack.h>
#include "SegmentationCostFunctionParameters.h"

// forward declarations
class EndSegment;
class ContinuationSegment;
class BranchSegment;
class Slice;

class SegmentationCostFunction : public pipeline::SimpleProcessNode {

	typedef boost::function<
			void
			(const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			 const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			 const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			 std::vector<double>& costs)>
			costs_function_type;

public:

	SegmentationCostFunction();

private:

	void updateOutputs();

	void costs(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			std::vector<double>& costs);

	double costs(const EndSegment& end);

	double costs(const ContinuationSegment& continuation);

	double costs(const BranchSegment& branch);

	double costs(const Slice& slice);

	unsigned int getBoundaryLength(const Slice& slice);

	pipeline::Input<ImageStack> _membranes;

	pipeline::Input<SegmentationCostFunctionParameters> _parameters;

	pipeline::Output<costs_function_type> _costFunction;
};

#endif // SOPNET_INFERENCE_SEGMENTATION_COST_FUNCTION_H__

