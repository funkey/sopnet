#include <pipeline/Process.h>
#include <pipeline/Value.h>

#include <sopnet/io/SolutionReader.h>
#include <sopnet/io/SubproblemsWriter.h>
#include "SubproblemsSolver.h"

SubproblemsSolver::SubproblemsSolver() {

	registerInput(_subproblems, "subproblems");
	registerOutput(_solution, "solution");
}

void
SubproblemsSolver::updateOutputs() {

	pipeline::Process<SubproblemsWriter> writer("subproblems.dat");

	writer->setInput(_subproblems);
	writer->write();

	// TODO: call scalar

	pipeline::Process<SolutionReader> reader("solution.dat");
	pipeline::Value<Solution> solution = reader->getOutput();

	*_solution = *solution;
}
