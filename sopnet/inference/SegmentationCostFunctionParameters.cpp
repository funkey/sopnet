#include "SegmentationCostFunctionParameters.h"

util::ProgramOption SegmentationCostFunctionParameters::optionSegmentationCostFunctionWeight(
		util::_module           = "sopnet.inference",
		util::_long_name        = "segmentationCostFunctionWeight",
		util::_description_text = "The influence of the segmentation cost function on the objective.",
		util::_default_value    = 1.0);

util::ProgramOption SegmentationCostFunctionParameters::optionSegmentationCostPottsWeight(
		util::_module           = "sopnet.inference",
		util::_long_name        = "segmentationCostPottsWeight",
		util::_description_text = "The influence of slice boundary length on the segmentation cost function.",
		util::_default_value    = 1.0);

util::ProgramOption SegmentationCostFunctionParameters::optionSegmentationCostPriorForeground(
		util::_module           = "sopnet.inference",
		util::_long_name        = "segmentationCostPriorForeground",
		util::_description_text = "The prior probability for each pixel to belong to a neuron.",
		util::_default_value    = 0.5);
