#ifndef SOPNET_INFERENCE_SUBPROBLEMS_H__
#define SOPNET_INFERENCE_SUBPROBLEMS_H__

#include <pipeline/Data.h>
#include "Problem.h"

/**
 * Describes a set of subproblem as part of a larger working problem.
 */
class Subproblems : public pipeline::Data {

public:

	void setProblem(boost::shared_ptr<Problem> problem) {

		_problem = problem;
	}

	void assignVariable(unsigned int variable, unsigned int subproblem) {

		_variablesToSubproblems[variable].insert(subproblem);
	}

	void assignConstraint(unsigned int constraint, unsigned int subproblem) {

		_constraintsToSubproblems[constraint].insert(subproblem);
	}

	boost::shared_ptr<Problem> getProblem() { return _problem; }

	std::set<unsigned int>& getVariableSubproblems(unsigned int variable) { return _variablesToSubproblems[variable]; }

	std::set<unsigned int>& getConstraintSubproblems(unsigned int constraint) { return _constraintsToSubproblems[constraint]; }

	void clear() {

		_problem.reset();
		_variablesToSubproblems.clear();
		_constraintsToSubproblems.clear();
	}

private:

	std::map<unsigned int, std::set<unsigned int> > _variablesToSubproblems;

	std::map<unsigned int, std::set<unsigned int> > _constraintsToSubproblems;

	// the problem that is decomposed into these subproblems
	boost::shared_ptr<Problem> _problem;
};

#endif // SOPNET_INFERENCE_SUBPROBLEMS_H__

