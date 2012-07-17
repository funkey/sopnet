#ifndef SOPNET_INFERENCE_PRIOR_COST_FUNCTION_H__
#define SOPNET_INFERENCE_PRIOR_COST_FUNCTION_H__

#include "PriorCostFunctionParameters.h"

// forward declarations
class EndSegment;
class ContinuationSegment;
class BranchSegment;

class PriorCostFunction : public pipeline::SimpleProcessNode {

	typedef boost::function<
			void
			(const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			 const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			 const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			 std::vector<double>& costs)>
			costs_function_type;

public:

	PriorCostFunction();

private:

	void updateOutputs();

	void costs(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			std::vector<double>& costs);

	pipeline::Input<PriorCostFunctionParameters> _parameters;

	pipeline::Output<costs_function_type> _costFunction;
};

#endif // SOPNET_INFERENCE_PRIOR_COST_FUNCTION_H__

