#include "PriorCostFunctionParameters.h"

util::ProgramOption PriorCostFunctionParameters::optionPriorEnds(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorEnds",
		util::_description_text = "A prior cost value added to each end segment.",
		util::_default_value    = 0.0);

util::ProgramOption PriorCostFunctionParameters::optionPriorContinuations(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorContinuations",
		util::_description_text = "A prior cost value added to each continuation segment.",
		util::_default_value    = 0.0);

util::ProgramOption PriorCostFunctionParameters::optionPriorBranches(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorBranches",
		util::_description_text = "A prior cost value added to each branch segment.",
		util::_default_value    = 0.0);
