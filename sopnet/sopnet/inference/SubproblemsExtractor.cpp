#include <util/ProgramOptions.h>
#include "SubproblemsExtractor.h"

util::ProgramOption optionSubproblemsSize(
		util::_long_name        = "subproblemsSize",
		util::_description_text = "The size of the subproblems in sections.");

util::ProgramOption optionSubproblemsOverlap(
		util::_long_name        = "subproblemsOverlap",
		util::_description_text = "The overlap between neighboring subproblems in sections.");

SubproblemsExtractor::SubproblemsExtractor() {

	registerInput(_objective, "objective");
	registerInput(_constraints, "constraints");
	registerInput(_configuration, "configuration");

	registerOutput(_subproblems, "subproblems");
}

void
SubproblemsExtractor::updateOutputs() {

	// compute the sizes of the subproblems

	// get all the variables of the respective subproblems

	// add all constraints and their variables that involve variables of the 
	// subproblems
}
