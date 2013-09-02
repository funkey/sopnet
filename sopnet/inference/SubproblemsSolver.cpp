#include <pipeline/Value.h>
#include <inference/LinearSolver.h>
#include "SubproblemsSolver.h"

SubproblemsSolver::SubproblemsSolver() {

	registerInput(_subproblems, "subproblems");
	registerOutput(_subsolutions, "subsolutions");
}

void
SubproblemsSolver::updateOutputs() {

	// create internal pipeline

	_subsolutionAssembler->clearInputs("subsolutions");

	foreach (boost::shared_ptr<Problem> problem, *_subproblems) {

		pipeline::Process<LinearSolver> solver;

		solver->setInput("objective", problem->getObjective());
		solver->setInput("linear constraints", problem->getLinearConstraints());
		solver->setInput("parameters", boost::make_shared<LinearSolverParameters>(Binary));

		_subsolutionAssembler->addInput("subsolutions", solver->getOutput("solution"));
	}

	pipeline::Value<Subsolutions> subsolutions = _subsolutionAssembler->getOutput("subsolutions");

	// copy internal pipeline output to output structure (this is not too bad, 
	// since Subsolutions is a lightweight data structure)
	*_subsolutions = *subsolutions;
}

SubproblemsSolver::SubsolutionsAssembler::SubsolutionsAssembler() {

	registerInputs(_singleSubsolutions, "subsolutions");
	registerOutput(_subsolutions, "subsolutions");
}

void
SubproblemsSolver::SubsolutionsAssembler::updateOutputs() {

	_subsolutions->clear();

	foreach (boost::shared_ptr<Solution> solution, _singleSubsolutions) {

		_subsolutions->addSolution(solution);
	}
}
