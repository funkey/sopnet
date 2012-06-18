#ifndef INFERENCE_LINEAR_SOLVER_PARAMETERS_H__
#define INFERENCE_LINEAR_SOLVER_PARAMETERS_H__

#include <pipeline.h>

#include "VariableType.h"

struct LinearSolverParameters : pipeline::Data {

public:

	LinearSolverParameters() :
		_variableType(Continuous) {};

	LinearSolverParameters(const VariableType& variableType) :
		_variableType(variableType) {}

	void setVariableType(const VariableType& variableType) {

		_variableType = variableType;
	}

	VariableType getVariableType() { return _variableType; }

private:

	VariableType _variableType;
};

#endif // INFERENCE_LINEAR_SOLVER_PARAMETERS_H__

