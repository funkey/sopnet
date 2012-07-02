#ifndef INFERENCE_LINEAR_SOLVER_H__
#define INFERENCE_LINEAR_SOLVER_H__

#include <string>

#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>
#include "DefaultFactory.h"
#include "LinearConstraints.h"
#include "LinearObjective.h"
#include "LinearSolverBackend.h"
#include "LinearSolverBackendFactory.h"
#include "LinearSolverParameters.h"
#include "Solution.h"

/**
 * Abstract class for linear program solvers. Implementations are supposed to
 * solve the following (integer) linear program:
 *
 * min  <a,x>
 * s.t. Ax  == b
 *      Cx  <= d
 *      optionally: x_i \in {0,1} for all i
 *
 * Where (A,b) describes all linear equality constraints, (C,d) all linear
 * inequality constraints and x is the solution vector. a is a real-valued
 * vector denoting the coefficients of the objective.
 *
 * The implementation is supposed to accept the inputs
 *
 *   objective   : LinearObjective
 *   constraints : LinearConstraints
 *   parameters  : SolverParameters
 *
 * and provide the output
 *
 *   solution    : Solution.
 */
class LinearSolver : public pipeline::SimpleProcessNode {

public:

	LinearSolver(const LinearSolverBackendFactory& backendFactory = DefaultFactory());

	~LinearSolver();

private:

	void onObjectiveModified(const pipeline::Modified& signal);

	void onLinearConstraintsModified(const pipeline::Modified& signal);

	void onParametersModified(const pipeline::Modified& signal);

	////////////////////////
	// pipeline interface //
	////////////////////////

	pipeline::Input<LinearObjective>        _objective;
	pipeline::Input<LinearConstraints>      _linearConstraints;
	pipeline::Input<LinearSolverParameters> _parameters;

	pipeline::Output<Solution> _solution;

	void updateOutputs();

	////////////////////
	// solver backend //
	////////////////////

	void updateLinearProgram();

	void solve();

	unsigned int getNumVariables();

	LinearSolverBackend* _solver;

	bool _objectiveDirty;

	bool _linearConstraintsDirty;

	bool _parametersDirty;
};

#endif // INFERENCE_LINEAR_SOLVER_H__

