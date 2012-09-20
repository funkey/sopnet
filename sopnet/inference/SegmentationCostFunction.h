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

	typedef boost::function<double (const Slice&)> slice_cost_function_type;

public:

	SegmentationCostFunction();

private:

	void updateOutputs();

	// the cost for a single slice
	double sliceCost(const Slice& slice);

	// the costs for all given segments
	void costs(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			std::vector<double>& costs);

	void computeSegmentationCosts(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches);

	void computeBoundaryLengths(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches);

	void computeSegmentationCost(const EndSegment& end);

	void computeSegmentationCost(const ContinuationSegment& continuation);

	void computeSegmentationCost(const BranchSegment& branch);

	void computeBoundaryLength(const EndSegment& end);

	void computeBoundaryLength(const ContinuationSegment& continuation);

	void computeBoundaryLength(const BranchSegment& branch);

	double computeSegmentationCost(const Slice& slice);

	unsigned int computeBoundaryLength(const Slice& slice);

	pipeline::Input<ImageStack> _membranes;

	pipeline::Input<SegmentationCostFunctionParameters> _parameters;

	pipeline::Output<costs_function_type> _costFunction;

	pipeline::Output<slice_cost_function_type> _sliceCostFunction;

	std::vector<double> _segmentationCosts;

	std::vector<unsigned int> _boundaryLengths;

	SegmentationCostFunctionParameters _prevParameters;
};

#endif // SOPNET_INFERENCE_SEGMENTATION_COST_FUNCTION_H__

