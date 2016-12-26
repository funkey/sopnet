#ifndef GUROBI_SOLVER_H__
#define GUROBI_SOLVER_H__

#ifdef HAVE_GUROBI

#include <string>

extern "C" {
#include <gurobi_c.h>
}

#include "LinearConstraints.h"
#include "QuadraticObjective.h"
#include "QuadraticSolverBackend.h"
#include "Sense.h"
#include "Solution.h"
#include <util/exceptions.h>

class GurobiException : public Exception {};

/**
 * Gurobi interface to solve the following (integer) quadratic program:
 *
 * min  <a,x> + xQx
 * s.t. Ax  == b
 *      Cx  <= d
 *      optionally: x_i \in {0,1} for all i
 *
 * Where (A,b) describes all linear equality constraints, (C,d) all linear
 * inequality constraints and x is the solution vector. a is a real-valued
 * vector denoting the coefficients of the objective and Q a PSD matrix giving
 * the quadratic coefficients of the objective.
 */
class GurobiBackend : public QuadraticSolverBackend {

public:

	GurobiBackend();

	virtual ~GurobiBackend();

	///////////////////////////////////
	// solver backend implementation //
	///////////////////////////////////

	void initialize(
			unsigned int numVariables,
			VariableType variableType);

	void initialize(
			unsigned int                                numVariables,
			VariableType                                defaultVariableType,
			const std::map<unsigned int, VariableType>& specialVariableTypes);

	void setObjective(const LinearObjective& objective);

	void setObjective(const QuadraticObjective& objective);

	void setConstraints(const LinearConstraints& constraints);

	void addConstraint(const LinearConstraint& constraint);

	/**
	 * Force the value of a variable to be a given value, i.e., pin the variable 
	 * to a fixed value.
	 *
	 * @param varNum
	 *              The number of the variable to pin.
	 *
	 * @param value
	 *              The value the variable has to assume.
	 */
	void pinVariable(unsigned int varNum, double value);

	/**
	 * Remove a previous pin from a variable.
	 *
	 * @param varNum
	 *              The number of the variable to unpin.
	 *
	 * @return True, if the variable was pinned before.
	 */
	bool unpinVariable(unsigned int varNum);

	bool solve(Solution& solution, double& value, std::string& message);

private:

	//////////////
	// internal //
	//////////////

	// dump the current problem to a file
	void dumpProblem(std::string filename);

	// set the optimality gap
	void setMIPGap(double gap);

	// set the mpi focus
	void setMIPFocus(unsigned int focus);

	// set a timeout
	void setTimeout(double timeout);

	// set the number of threads to use
	void setNumThreads(unsigned int numThreads);

	// enable solver output
	void setVerbose(bool verbose);

	// check error status and throw exception, used by our macro GRB_CHECK
	void grbCheck(const char* call, const char* file, int line, int error);

	// size of a and x
	unsigned int _numVariables;

	// number of rows in A and C
	unsigned int _numConstraints;

	// the GRB environment
	GRBenv* _env;

	// the GRB model containing the objective and constraints
	GRBmodel* _model;
};

#endif // HAVE_GUROBI

#endif // GUROBI_SOLVER_H__


