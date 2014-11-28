#include <limits>

#include <util/ProgramOptions.h>
#include <sopnet/inference/ProblemAssembler.h>
#include <sopnet/inference/ObjectiveGenerator.h>
#include <inference/LinearSolver.h>
#include "GoldStandardExtractor.h"
#include "OverlapCostFunction.h"
#include "MergeCostFunction.h"

util::ProgramOption optionMergeCostFunction(
		util::_module           = "sopnet.training.gold_standard",
		util::_long_name        = "mergeCostFunction",
		util::_description_text = "Use the merge cost function instead of the default overlap cost function for finding the gold standard.");

logger::LogChannel goldstandardextractorlog("goldstandardextractorlog", "[GoldStandardExtractor] ");

GoldStandardExtractor::GoldStandardExtractor() {

	registerInput(_groundTruth, "ground truth");
	registerInput(_allSegments, "all segments");
	registerInput(_allLinearConstraints, "all linear constraints");

	registerOutput(_reconstructor->getOutput("reconstruction"), "gold standard");
	registerOutput(_reconstructor->getOutput("discarded segments"), "negative samples");
	registerOutput(_objectiveGenerator->getOutput(), "gold standard objective");
}

void
GoldStandardExtractor::updateOutputs() {

	LOG_DEBUG(goldstandardextractorlog) << "searching for best-fitting segments to ground truth" << std::endl;

	boost::shared_ptr<pipeline::ProcessNode> goldStandardCostFunction;
	if (optionMergeCostFunction)
		goldStandardCostFunction = boost::make_shared<MergeCostFunction>();
	else
		goldStandardCostFunction = boost::make_shared<OverlapCostFunction>();

	pipeline::Process<LinearSolver> linearSolver;

	goldStandardCostFunction->setInput("ground truth", _groundTruth);

	_objectiveGenerator->setInput("segments", _allSegments);
	_objectiveGenerator->addInput("cost functions", goldStandardCostFunction->getOutput());

	linearSolver->setInput("objective", _objectiveGenerator->getOutput());
	linearSolver->setInput("linear constraints", _allLinearConstraints);
	linearSolver->setInput("parameters", boost::make_shared<LinearSolverParameters>(Binary));

	_reconstructor->setInput("solution", linearSolver->getOutput("solution"));
	_reconstructor->setInput("segments", _allSegments);
}

