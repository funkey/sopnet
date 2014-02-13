#ifndef INFERENCE_QUADRATIC_SOLVER_BACKEND_H__
#define INFERENCE_QUADRATIC_SOLVER_BACKEND_H__

#include "QuadraticObjective.h"
#include "LinearSolverBackend.h"

class QuadraticSolverBackend : public LinearSolverBackend {

public:

	virtual ~QuadraticSolverBackend() {}

	/**
	 * Set the objective.
	 *
	 * @param objective A quadratic objective.
	 */
	virtual void setObjective(const QuadraticObjective& objective) = 0;
};

#endif // INFERENCE_QUADRATIC_SOLVER_BACKEND_H__

