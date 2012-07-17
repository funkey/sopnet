#ifndef SOPNET_INFERENCE_PRIOR_COST_FUNCTION_PARAMETERS_H__
#define SOPNET_INFERENCE_PRIOR_COST_FUNCTION_PARAMETERS_H__

#include <pipeline/all.h>

struct PriorCostFunctionParameters : public pipeline::Data {

	PriorCostFunctionParameters() :
		priorEnd(1.0),
		priorContinuation(1.0),
		priorBranch(1.0) {}

	// the prior for end segments
	double priorEnd;

	// the prior for continuation segments
	double priorContinuation;

	// the prior for branch segments
	double priorBranch;
};

#endif // SOPNET_INFERENCE_PRIOR_COST_FUNCTION_PARAMETERS_H__

