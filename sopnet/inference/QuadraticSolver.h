#ifndef INFERENCE_QUADRATIC_SOLVER_H__
#define INFERENCE_QUADRATIC_SOLVER_H__

#include <string>

#include <boost/shared_ptr.hpp>

#include "DefaultFactory.h"
#include "LinearConstraints.h"
#include "QuadraticObjective.h"
#include "QuadraticSolverBackend.h"
#include "QuadraticSolverBackendFactory.h"
#include "QuadraticSolverParameters.h"
#include "Solution.h"

/**
 * Abstract class for quadratic program solvers. Implementations are supposed to
 * solve the following (integer) quadratic program:
 *
 * min  <a,x> + xQx
 * s.t. Ax  == b
 *      Cx  <= d
 *      optionally: x_i \in {0,1} for all i
 *
 * Where (A,b) describes all quadratic equality constraints, (C,d) all quadratic
 * inequality constraints and x is the solution vector. a is a real-valued
 * vector denoting the coefficients of the objective and Q the quadratic
 * coefficients.
 */
class QuadraticSolver : public pipeline::SimpleProcessNode<> {

public:

	QuadraticSolver(const QuadraticSolverBackendFactory& backendFactory = DefaultFactory());

	~QuadraticSolver();

private:

	////////////////////////
	// pipeline interface //
	////////////////////////

	pipeline::Input<QuadraticObjective>        _objective;
	pipeline::Input<LinearConstraints>         _linearConstraints;
	pipeline::Input<QuadraticSolverParameters> _parameters;

	pipeline::Output<Solution> _solution;

	void updateOutputs();

	////////////////////
	// solver backend //
	////////////////////

	void updateQuadraticProgram();

	void solve();

	unsigned int getNumVariables();

	QuadraticSolverBackend* _solver;
};

#endif // QUADRATIC_SOLVER_H__
