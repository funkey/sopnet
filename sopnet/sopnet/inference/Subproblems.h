#ifndef SOPNET_INFERENCE_SUBPROBLEMS_H__
#define SOPNET_INFERENCE_SUBPROBLEMS_H__

#include <pipeline/Data.h>
#include "Problem.h"

/**
 * Describes a set of subproblem as part of a larger working problem.
 */
class Subproblems : public pipeline::Data {

public:

	/**
	 * Set the working problem that is decomposed by this set of subproblems.
	 */
	void setProblem(boost::shared_ptr<Problem> problem) {

		_problem = problem;
	}

	/**
	 * Get the problem that is decomposed by this set of subproblems.
	 */
	boost::shared_ptr<Problem> getProblem() { return _problem; }

	/**
	 * Assign a working problem variable to a subproblem. Since subproblems are 
	 * defined as sets of factors, this corresponds rather to assigning the 
	 * unary factor of this variable to a subproblem.
	 */
	void assignVariable(unsigned int variable, unsigned int subproblem) {

		_variablesToSubproblems[variable].insert(subproblem);
	}

	/**
	 * Assign a working problem constraint to a subproblem.
	 */
	void assignConstraint(unsigned int constraint, unsigned int subproblem) {

		_constraintsToSubproblems[constraint].insert(subproblem);
	}

	/**
	 * Get all subproblems that a variable is assigned to.
	 */
	std::set<unsigned int>& getVariableSubproblems(unsigned int variable) { return _variablesToSubproblems[variable]; }

	/**
	 * Get all subproblems that a constraint is assigned to.
	 */
	std::set<unsigned int>& getConstraintSubproblems(unsigned int constraint) { return _constraintsToSubproblems[constraint]; }

	/**
	 * Reset the decomposition.
	 */
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

