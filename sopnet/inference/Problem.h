#ifndef SOPNET_INFERENCE_PROBLEM_H__
#define SOPNET_INFERENCE_PROBLEM_H__

#include <pipeline/all.h>
#include <inference/LinearObjective.h>
#include <inference/LinearConstraints.h>
#include "ProblemConfiguration.h"

class Problem : public pipeline::Data {

public:

	Problem(unsigned int numVariables) :
		_objective(numVariables) {}

	void setObjective(const LinearObjective& objective) {

		_objective = objective;
	}

	void setLinearConstraints(const LinearConstraints& constraints) {

		_linearConstraints = constraints;
	}

	void setConfiguration(const ProblemConfiguration& configuration) {

		_configuration = configuration;
	}

	LinearObjective& getObjective() {

		return _objective;
	}

	LinearConstraints& getLinearConstraints() {

		return _linearConstraints;
	}

	ProblemConfiguration& getConfiguration() {

		return _configuration;
	}

private:

	LinearObjective      _objective;
	LinearConstraints    _linearConstraints;
	ProblemConfiguration _configuration;
};

#endif // SOPNET_INFERENCE_PROBLEM_H__

