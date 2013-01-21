#ifndef GUROBI_SOLVER_H__
#define GUROBI_SOLVER_H__

#ifdef HAVE_GUROBI

#include <string>

#include <gurobi_c++.h>

#include "LinearConstraints.h"
#include "LinearSolverParameters.h"
#include "QuadraticObjective.h"
#include "QuadraticSolverBackend.h"
#include "Sense.h"
#include "Solution.h"

using namespace std;

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

	void initialize(unsigned int numVariables, VariableType varibleType);

	void setObjective(const LinearObjective& objective);

	void setObjective(const QuadraticObjective& objective);

	void setConstraints(const LinearConstraints& constraints);

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

	/**
	 * Enable solver output.
	 */
	void setVerbose(bool verbose);

	// size of a and x
	unsigned int _numVariables;

	// rows in A
	unsigned int _numEqConstraints;

	// rows in C
	unsigned int _numIneqConstraints;

	// the GRB environment
	GRBEnv _env;

	// the (binary) variables x
	GRBVar* _variables;

	// the objective
	GRBQuadExpr _objective;

	std::vector<GRBConstr> _constraints;

	// the GRB model containing the objective and constraints
	GRBModel _model;

	// the verbosity of the output
	int _verbosity;

	// a value by which to scale the objective
	double _scale;
};

#endif // HAVE_GUROBI

#endif // GUROBI_SOLVER_H__


