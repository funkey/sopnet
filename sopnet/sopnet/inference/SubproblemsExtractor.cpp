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

	unsigned int subproblemsSize    = optionSubproblemsSize;
	unsigned int subproblemsOverlap = optionSubproblemsOverlap;
	unsigned int minInterSectionInterval = _configuration->getMinInterSectionInterval();
	unsigned int maxInterSectionInterval = _configuration->getMaxInterSectionInterval();

	for (unsigned int startSubproblem = minInterSectionInterval; startSubproblem < maxInterSectionInterval; startSubproblem += subproblemsSize - subproblemsOverlap) {

		unsigned int endSubproblem = startSubproblem += subproblemsSize;

		// get all global variable ids for this subproblem (subproblem variable 
		// id is index in this array)
		std::vector<unsigned int> globalVarIds = _configuration->getVariables(startSubproblem, endSubproblem);

		// get all segment ids for these variables
		std::vector<unsigned int> segmentIds; // 1:1 to globalVarIds
		foreach (unsigned int globalVarId, globalVarIds)
			segmentIds.push_back(_configuration->getSegmentId(globalVarId));

		// create a problem
		boost::shared_ptr<Problem> problem = boost::make_shared<Problem>(globalVarIds.size());

		// map new problem variables to segment ids
		for (unsigned int i = 0; i < segmentIds.size(); i++)
			problem->getConfiguration()->setVariable(segmentIds[i], i);

		// set coefficients in new objective
		for (unsigned int i = 0; i < globalVarIds.size(); i++) {

			unsigned int globalVarId = globalVarIds[i];
			double       coefficient = _objective->getCoefficients()[globalVarId];

			problem->getObjective()->setCoefficient(i, coefficient);
		}

		// create map from global variable ids to subproblem variable ids
		std::map<unsigned int, unsigned int> globalToSubproblemVariableIds;
		for (unsigned int i = 0; i < globalVarIds.size(); i++)
			globalToSubproblemVariableIds[globalVarIds[i]] = i;

		// find all constraints that involve the global variable ids
		boost::shared_ptr<LinearConstraints> constraints = boost::make_shared<LinearConstraints>(_constraints->getConstraints(globalVarIds));

		// map constraints from global variable ids to subproblem variable ids
		foreach (LinearConstraint& constraint, *constraints) {

			LinearConstraint mappedConstraint;

			unsigned int globalVarId;
			double       coef;
			foreach (boost::tie(globalVarId, coef), constraint.getCoefficients()) {

				unsigned int mappedVarId;

				// variable is mapped already
				if (globalToSubproblemVariableIds.count(globalVarId)) {

					mappedVarId = globalToSubproblemVariableIds[globalVarId];

				// unknown variable
				} else {

					mappedVarId = globalVarIds.size();
					unsigned int segmentId = _configuration->getSegmentId(globalVarId);

					// resize problem
					problem->resize(mappedVarId);

					// update mappings
					globalVarIds.push_back(globalVarId);
					segmentIds.push_back(segmentId);
					globalToSubproblemVariableIds[globalVarId] = mappedVarId;

					// set objective value in subproblem
					problem->getObjective()->setCoefficient(mappedVarId, _objective->getCoefficients()[globalVarId]);

					// set segment id for this variable
					problem->getConfiguration()->setVariable(segmentId, mappedVarId);
				}

				mappedConstraint.setCoefficient(mappedVarId, coef);
			}

			mappedConstraint.setRelation(constraint.getRelation());
			mappedConstraint.setValue(constraint.getValue());

			problem->getLinearConstraints()->add(mappedConstraint);
		}

		_subproblems->addProblem(problem);
	}
}
