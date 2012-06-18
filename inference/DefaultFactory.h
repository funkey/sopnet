#ifndef INFERENCE_DEFAULT_FACTORY_H__
#define INFERENCE_DEFAULT_FACTORY_H__

#include <exceptions.h>
#include "LinearSolverBackendFactory.h"
#include "QuadraticSolverBackendFactory.h"

struct NoSolverException : virtual Exception {};

class DefaultFactory :
		public LinearSolverBackendFactory,
		public QuadraticSolverBackendFactory {

public:

	LinearSolverBackend* createLinearSolverBackend() const;

	QuadraticSolverBackend* createQuadraticSolverBackend() const;
};

#endif // INFERENCE_DEFAULT_FACTORY_H__

