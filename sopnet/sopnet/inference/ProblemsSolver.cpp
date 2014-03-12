#include <pipeline/Value.h>
#include <inference/LinearSolver.h>
#include "ProblemsSolver.h"

ProblemsSolver::ProblemsSolver() :
	_solutions(new Solutions()) {

	registerInput(_problems, "problems");
	registerOutput(_solutions, "solutions");
}

void
ProblemsSolver::updateOutputs() {

	// create internal pipeline

	_solutionAssembler->clearInputs("solutions");

	foreach (boost::shared_ptr<Problem> problem, *_problems) {

		pipeline::Process<LinearSolver> solver;

		solver->setInput("objective", problem->getObjective());
		solver->setInput("linear constraints", problem->getLinearConstraints());
		solver->setInput("parameters", boost::make_shared<LinearSolverParameters>(Binary));

		_solutionAssembler->addInput("solutions", solver->getOutput("solution"));
	}

	pipeline::Value<Solutions> solutions = _solutionAssembler->getOutput("solutions");

	// copy internal pipeline output to output structure (this is not too bad, 
	// since Solutions is a lightweight data structure)
	*_solutions = *solutions;
}

ProblemsSolver::SolutionsAssembler::SolutionsAssembler() {

	registerInputs(_singleSolutions, "solutions");
	registerOutput(_solutions, "solutions");
}

void
ProblemsSolver::SolutionsAssembler::updateOutputs() {

	_solutions->clear();

	foreach (boost::shared_ptr<Solution> solution, _singleSolutions) {

		_solutions->addSolution(solution);
	}
}
