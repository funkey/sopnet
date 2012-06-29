#ifndef SOPNET_INFERENCE_SEGMENTATION_COST_FUNCTION_PARAMETERS_H__
#define SOPNET_INFERENCE_SEGMENTATION_COST_FUNCTION_PARAMETERS_H__

#include <pipeline/all.h>

struct SegmentationCostFunctionParameters : public pipeline::Data {

	SegmentationCostFunctionParameters() :
		weightPotts(1.0) {}

	// the weight of the whole term
	double weight;

	// the weight of the potts-term
	double weightPotts;
};

#endif // SOPNET_INFERENCE_SEGMENTATION_COST_FUNCTION_PARAMETERS_H__

