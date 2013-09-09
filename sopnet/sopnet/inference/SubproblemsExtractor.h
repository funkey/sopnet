#ifndef SOPNET_INFERENCE_SUBPROBLEMS_EXTRACTOR_H__
#define SOPNET_INFERENCE_SUBPROBLEMS_EXTRACTOR_H__

#include <pipeline/SimpleProcessNode.h>
#include <inference/LinearObjective.h>
#include <inference/LinearConstraints.h>
#include "ProblemConfiguration.h"
#include "Subproblems.h"

class SubproblemsExtractor : public pipeline::SimpleProcessNode<> {

public:

	SubproblemsExtractor();

private:

	void updateOutputs();

	pipeline::Input<LinearObjective>      _objective;
	pipeline::Input<LinearConstraints>    _constraints;
	pipeline::Input<ProblemConfiguration> _configuration;

	pipeline::Output<Subproblems> _subproblems;
};

#endif // SOPNET_INFERENCE_SUBPROBLEMS_EXTRACTOR_H__

