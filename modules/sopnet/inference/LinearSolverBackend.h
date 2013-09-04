#ifndef INFERENCE_LINEAR_SOLVER_BACKEND_H__
#define INFERENCE_LINEAR_SOLVER_BACKEND_H__

#include "LinearObjective.h"
#include "LinearConstraints.h"
#include "Solution.h"
#include "VariableType.h"

class LinearSolverBackend {

public:

	/**
	 * Initialise the linear solver for the given type of variables.
	 *
	 * @param numVariables The number of variables in the problem.
	 * @param variableType The type of the variables (Continuous, Integer,
	 *                     Binary).
	 */
	virtual void initialize(unsigned int numVariables, VariableType variableType) = 0;

	/**
	 * Set the objective.
	 *
	 * @param objective A linear objective.
	 */
	virtual void setObjective(const LinearObjective& objective) = 0;

	/**
	 * Set the linear (in)equality constraints.
	 *
	 * @param constraints A set of linear constraints.
	 */
	virtual void setConstraints(const LinearConstraints& constraints) = 0;

	/**
	 * Solve the problem.
	 *
	 * @param solution A solution object to write the solution to.
	 * @param value The optimal value of the objective.
	 * @param message A status message from the solver.
	 * @return true, if the optimal value was found.
	 */
	virtual bool solve(Solution& solution, double& value, std::string& message) = 0;
};

#endif // INFERENCE_LINEAR_SOLVER_BACKEND_H__

