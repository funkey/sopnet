#ifndef SOPNET_INFERENCE_PROBLEMS_SOLVER_H__
#define SOPNET_INFERENCE_PROBLEMS_SOLVER_H__

#include <pipeline/all.h>
#include <pipeline/Process.h>
#include <inference/Solution.h>
#include "Problems.h"
#include "Solutions.h"

class ProblemsSolver : public pipeline::SimpleProcessNode<> {

public:

	ProblemsSolver();

private:

	class SolutionsAssembler : public pipeline::SimpleProcessNode<> {

	public:

		SolutionsAssembler();

	private:

		void updateOutputs();

		pipeline::Inputs<Solution>  _singleSolutions;
		pipeline::Output<Solutions> _solutions;
	};

	void updateOutputs();

	pipeline::Input<Problems>   _problems;
	pipeline::Output<Solutions> _solutions;

	pipeline::Process<SolutionsAssembler> _solutionAssembler;
};

#endif // SOPNET_INFERENCE_PROBLEMS_SOLVER_H__

