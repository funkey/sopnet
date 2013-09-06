#ifndef SOPNET_INFERENCE_PRIOR_COST_FUNCTION_PARAMETERS_H__
#define SOPNET_INFERENCE_PRIOR_COST_FUNCTION_PARAMETERS_H__

#include <pipeline/all.h>
#include <util/ProgramOptions.h>

struct PriorCostFunctionParameters : public pipeline::Data {

	PriorCostFunctionParameters() :
		priorEnd(0.0),
		priorContinuation(0.0),
		priorBranch(0.0) {}

	// program options to set default values
	static util::ProgramOption optionPriorEnds;
	static util::ProgramOption optionPriorContinuations;
	static util::ProgramOption optionPriorBranches;

	// the prior for end segments
	double priorEnd;

	// the prior for continuation segments
	double priorContinuation;

	// the prior for branch segments
	double priorBranch;
};

#endif // SOPNET_INFERENCE_PRIOR_COST_FUNCTION_PARAMETERS_H__

