#ifndef SOPNET_INFERENCE_SUBPROBLEMS_SOLVER_H__
#define SOPNET_INFERENCE_SUBPROBLEMS_SOLVER_H__

#include <pipeline/all.h>
#include <pipeline/Process.h>
#include <inference/Solution.h>
#include "Subproblems.h"
#include "Subsolutions.h"

class SubproblemsSolver : public pipeline::SimpleProcessNode<> {

public:

	SubproblemsSolver();

private:

	class SubsolutionsAssembler : public pipeline::SimpleProcessNode<> {

	public:

		SubsolutionsAssembler();

	private:

		void updateOutputs();

		pipeline::Inputs<Solution>     _singleSubsolutions;
		pipeline::Output<Subsolutions> _subsolutions;
	};

	void updateOutputs();

	pipeline::Input<Subproblems>   _subproblems;
	pipeline::Output<Subsolutions> _subsolutions;

	pipeline::Process<SubsolutionsAssembler> _subsolutionAssembler;
};

#endif // SOPNET_INFERENCE_SUBPROBLEMS_SOLVER_H__

