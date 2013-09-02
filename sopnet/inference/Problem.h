#ifndef SOPNET_INFERENCE_PROBLEM_H__
#define SOPNET_INFERENCE_PROBLEM_H__

#include <pipeline/all.h>
#include <inference/LinearObjective.h>
#include <inference/LinearConstraints.h>
#include "ProblemConfiguration.h"

class Problem : public pipeline::Data {

public:

	Problem(unsigned int numVariables) :
		_objective(boost::make_shared<LinearObjective>(numVariables)),
		_linearConstraints(boost::make_shared<LinearConstraints>()),
		_configuration(boost::make_shared<ProblemConfiguration>()) {}

	void setObjective(boost::shared_ptr<LinearObjective> objective) {

		_objective = objective;
	}

	void setLinearConstraints(boost::shared_ptr<LinearConstraints> constraints) {

		_linearConstraints = constraints;
	}

	void setConfiguration(boost::shared_ptr<ProblemConfiguration> configuration) {

		_configuration = configuration;
	}

	boost::shared_ptr<LinearObjective> getObjective() {

		return _objective;
	}

	boost::shared_ptr<LinearConstraints> getLinearConstraints() {

		return _linearConstraints;
	}

	boost::shared_ptr<ProblemConfiguration> getConfiguration() {

		return _configuration;
	}

private:

	boost::shared_ptr<LinearObjective>      _objective;
	boost::shared_ptr<LinearConstraints>    _linearConstraints;
	boost::shared_ptr<ProblemConfiguration> _configuration;
};

#endif // SOPNET_INFERENCE_PROBLEM_H__

