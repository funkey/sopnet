#ifndef SOPNET_INFERENCE_SEGMENTATION_COST_FUNCTION_PARAMETERS_H__
#define SOPNET_INFERENCE_SEGMENTATION_COST_FUNCTION_PARAMETERS_H__

#include <pipeline/all.h>
#include <util/ProgramOptions.h>

struct SegmentationCostFunctionParameters : public pipeline::Data {

	SegmentationCostFunctionParameters() :
		weightPotts(1.0) {}

	static util::ProgramOption optionSegmentationCostFunctionWeight;
	static util::ProgramOption optionSegmentationCostPottsWeight;
	static util::ProgramOption optionSegmentationCostPriorForeground;

	// the weight of the whole term
	double weight;

	// the weight of the potts-term
	double weightPotts;

	// the prior on the foreground pixels
	double priorForeground;
};

#endif // SOPNET_INFERENCE_SEGMENTATION_COST_FUNCTION_PARAMETERS_H__

