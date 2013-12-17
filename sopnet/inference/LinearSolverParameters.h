#ifndef INFERENCE_LINEAR_SOLVER_PARAMETERS_H__
#define INFERENCE_LINEAR_SOLVER_PARAMETERS_H__

#include <pipeline/all.h>

#include "VariableType.h"

struct LinearSolverParameters : pipeline::Data {

public:

	LinearSolverParameters() :
		_variableType(Continuous) {};

	LinearSolverParameters(const VariableType& variableType) :
		_variableType(variableType) {}

	/**
	 * Set the default variable type for all variables.
	 */
	void setVariableType(const VariableType& variableType) {

		_variableType = variableType;
	}

	void setVariableType(unsigned int var, const VariableType& variableType) {

		_variableTypes[var] = variableType;
	}

	VariableType getVariableType(unsigned int var) {

		if (_variableTypes.count(var) == 0)
			return _variableType;
		else
			return _variableTypes[var];
	}

	VariableType getDefaultVariableType() const {

		return _variableType;
	}

	const std::map<unsigned int, VariableType>& getSpecialVariableTypes() const {

		return _variableTypes;
	}

private:

	// the default variable type
	VariableType _variableType;

	// individual variable types
	std::map<unsigned int, VariableType> _variableTypes;
};

#endif // INFERENCE_LINEAR_SOLVER_PARAMETERS_H__

