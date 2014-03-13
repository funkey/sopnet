#include <limits>

#include <util/ProgramOptions.h>
#include <sopnet/inference/ProblemAssembler.h>
#include <sopnet/inference/ObjectiveGenerator.h>
#include <inference/LinearSolver.h>
#include "GoldStandardExtractor.h"
#include "GoldStandardCostFunction.h"

logger::LogChannel goldstandardextractorlog("goldstandardextractorlog", "[GoldStandardExtractor] ");

GoldStandardExtractor::GoldStandardExtractor() {

	registerInput(_groundTruth, "ground truth");
	registerInput(_allSegments, "all segments");
	registerInput(_allLinearConstraints, "all linear constraints");

	registerOutput(_reconstructor->getOutput("reconstruction"), "gold standard");
	registerOutput(_reconstructor->getOutput("discarded segments"), "negative samples");
}

void
GoldStandardExtractor::updateOutputs() {

	LOG_DEBUG(goldstandardextractorlog) << "searching for best-fitting segments to ground truth" << std::endl;

	pipeline::Process<ProblemAssembler>         problemAssembler;
	pipeline::Process<GoldStandardCostFunction> goldStandardCostFunction;
	pipeline::Process<ObjectiveGenerator>       objectiveGenerator;
	pipeline::Process<LinearSolver>             linearSolver;

	problemAssembler->addInput("neuron segments",           _allSegments);
	problemAssembler->addInput("neuron linear constraints", _allLinearConstraints);

	goldStandardCostFunction->setInput("ground truth", _groundTruth);

	objectiveGenerator->setInput("segments", _allSegments);
	objectiveGenerator->addInput("cost functions", goldStandardCostFunction->getOutput());

	linearSolver->setInput("objective", objectiveGenerator->getOutput());
	linearSolver->setInput("linear constraints", problemAssembler->getOutput("linear constraints"));
	linearSolver->setInput("parameters", boost::make_shared<LinearSolverParameters>(Binary));

	_reconstructor->setInput("solution", linearSolver->getOutput("solution"));
	_reconstructor->setInput("segments", problemAssembler->getOutput("segments"));
}

