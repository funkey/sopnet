#ifndef SOPNET_INFERENCE_SUBPROBLEMS_SOLVER_H__
#define SOPNET_INFERENCE_SUBPROBLEMS_SOLVER_H__

#include <pipeline/SimpleProcessNode.h>
#include <inference/Solution.h>
#include "Subproblems.h"

/**
 * Given a list of subproblems, invoces SCALAR to find a consistent solution.
 */
class SubproblemsSolver : public pipeline::SimpleProcessNode<> {

public:

	SubproblemsSolver();

private:

	void updateOutputs();

	pipeline::Input<Subproblems> _subproblems;
	pipeline::Output<Solution>   _solution;
};

#endif // SOPNET_INFERENCE_SUBPROBLEMS_SOLVER_H__

