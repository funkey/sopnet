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

	std::string filename("subproblems.dat");

	pipeline::Process<SubproblemsWriter> writer(filename);

	writer->setInput(_subproblems);
	writer->write();

	// TODO: call scalar

	pipeline::Process<SolutionReader> reader(filename);
	pipeline::Value<Solution> solution = reader->getOutput();

	_solution = solution;
}
