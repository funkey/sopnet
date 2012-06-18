#include <boost/make_shared.hpp>

#include <imageprocessing/ImageStack.h>
#include <imageprocessing/ImageExtractor.h>
#include <inference/LinearSolver.h>
#include <util/foreach.h>
#include "GroundTruthExtractor.h"
#include "GoldStandardExtractor.h"
#include "ObjectiveGenerator.h"
#include "ProblemAssembler.h"
#include "Reconstructor.h"
#include "SegmentEvaluator.h"
#include "SegmentExtractor.h"
#include "SegmentRandomForestTrainer.h"
#include "SliceExtractor.h"
#include "Sopnet.h"

static logger::LogChannel sopnetlog("sopnetlog", "[Sopnet] ");

Sopnet::Sopnet(const std::string& projectDirectory) :
	_projectDirectory(projectDirectory),
	_membraneExtractor(boost::make_shared<ImageExtractor>()),
	_segmentEvaluator(boost::make_shared<SegmentEvaluator>()),
	_problemAssembler(boost::make_shared<ProblemAssembler>()),
	_objectiveGenerator(boost::make_shared<ObjectiveGenerator>()),
	_linearSolver(boost::make_shared<LinearSolver>()),
	_reconstructor(boost::make_shared<Reconstructor>()),
	_groundTruthExtractor(boost::make_shared<GroundTruthExtractor>()),
	_segmentRfTrainer(boost::make_shared<SegmentRandomForestTrainer>()) {

	// tell the outside world what we need
	registerInput(_rawSections, "raw sections");
	registerInput(_membranes, "membranes");
	registerInput(_groundTruth, "ground truth");
	registerInput(_segmentExtractionThreshold, "segment extraction threshold");

	_membranes.registerBackwardCallback(&Sopnet::onMembranesSet, this);
	_rawSections.registerBackwardCallback(&Sopnet::onRawSectionsSet, this);
	_groundTruth.registerBackwardCallback(&Sopnet::onGroundTruthSet, this);

	// tell the outside world what we've got
	registerOutput(_reconstructor->getOutput(), "solution");
	registerOutput(_problemAssembler->getOutput("segments"), "segments");
	registerOutput(_groundTruthExtractor->getOutput("ground truth segments"), "ground truth segments");
	registerOutput(_segmentRfTrainer->getOutput("gold standard"), "gold standard");
	registerOutput(_segmentRfTrainer->getOutput("negative samples"), "negative samples");
	registerOutput(_segmentRfTrainer->getOutput("random forest"), "random forest");
}

void
Sopnet::onMembranesSet(const pipeline::InputSet<ImageStack>& signal) {

	LOG_DEBUG(sopnetlog) << "membranes set" << std::endl;

	createPipeline();
}

void
Sopnet::onRawSectionsSet(const pipeline::InputSet<ImageStack>& signal) {

	LOG_DEBUG(sopnetlog) << "raw sections set" << std::endl;

	createPipeline();
}

void
Sopnet::onGroundTruthSet(const pipeline::InputSet<ImageStack>& signal) {

	LOG_DEBUG(sopnetlog) << "ground-truth set" << std::endl;

	createPipeline();
}

void
Sopnet::createPipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating pipeline" << std::endl;

	if (!_membranes || !_rawSections || !_groundTruth || !_segmentExtractionThreshold) {

		LOG_DEBUG(sopnetlog) << "not all inputs present -- skip pipeline creation" << std::endl;
		return;
	}

	createBasicPipeline();

	createInferencePipeline();

	createTrainingPipeline();
}

void
Sopnet::createBasicPipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating basic part..." << std::endl;

	// clear previous pipeline
	_sliceExtractors.clear();
	_segmentExtractors.clear();
	_problemAssembler->clearInputs("segments");
	_problemAssembler->clearInputs("linear constraints");

	// let the internal image extractor know where to look for the image stack
	_membraneExtractor->setInput(_membranes.getAssignedOutput());

	// set the parameters for the segment evaluation function
	// TODO: expose relevant parameters
	_segmentEvaluator->setInput(boost::make_shared<SegmentCostFunctionParameters>());

	LOG_DEBUG(sopnetlog) << "creating pipeline for " << _membranes->size() << " sections" << std::endl;

	// for every section
	for (int section = 0; section < _membranes->size(); section++) {

		LOG_DEBUG(sopnetlog) << "creating pipeline for section " << section << std::endl;

		// create a slice extractor
		boost::shared_ptr<SliceExtractor> sliceExtractor = boost::make_shared<SliceExtractor>(section);

		// set its input
		sliceExtractor->setInput(_membraneExtractor->getOutput(section));

		// store it in the list of all slice extractors
		_sliceExtractors.push_back(sliceExtractor);

		if (_sliceExtractors.size() <= 1)
			continue;

		// get the previous slice file reader
		boost::shared_ptr<SliceExtractor> prevSliceExtractor = _sliceExtractors[_sliceExtractors.size() - 2];

		// create a segment extractor
		boost::shared_ptr<SegmentExtractor> segmentExtractor = boost::make_shared<SegmentExtractor>();

		// connect current and previous slices to that
		segmentExtractor->setInput("previous slices", prevSliceExtractor->getOutput("slices"));
		segmentExtractor->setInput("next slices", sliceExtractor->getOutput("slices"));
		segmentExtractor->setInput("previous linear constraints", prevSliceExtractor->getOutput("linear constraints"));
		if (section == _membranes->size() - 1) // only for the last pair of slices
			segmentExtractor->setInput("next linear constraints", sliceExtractor->getOutput("linear constraints"));
		segmentExtractor->setInput("cost function", _segmentEvaluator->getOutput("cost function"));
		segmentExtractor->setInput("cost threshold", _segmentExtractionThreshold.getAssignedOutput());

		_segmentExtractors.push_back(segmentExtractor);

		// add segments and linear constraints to problem assembler
		_problemAssembler->addInput("segments", segmentExtractor->getOutput("segments"));
		_problemAssembler->addInput("linear constraints", segmentExtractor->getOutput("linear constraints"));
	}
}

void
Sopnet::createInferencePipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating inference part..." << std::endl;

	// feed all segments to objective generator
	_objectiveGenerator->setInput("segments", _problemAssembler->getOutput("segments"));
	_objectiveGenerator->setInput("cost function", _segmentEvaluator->getOutput("cost function"));
	_objectiveGenerator->setInput("segment ids map", _problemAssembler->getOutput("segment ids map"));

	// feed objective and linear constraints to ilp creator
	_linearSolver->setInput("objective", _objectiveGenerator->getOutput());
	_linearSolver->setInput("linear constraints", _problemAssembler->getOutput("linear constraints"));
	_linearSolver->setInput("parameters", boost::make_shared<LinearSolverParameters>(Binary));

	// feed solution and segments to reconstructor
	_reconstructor->setInput("solution", _linearSolver->getOutput("solution"));
	_reconstructor->setInput("segment ids map", _problemAssembler->getOutput("segment ids map"));
	_reconstructor->setInput("segments", _problemAssembler->getOutput("segments"));
}

void
Sopnet::createTrainingPipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating training part..." << std::endl;

	_groundTruthExtractor->setInput(_groundTruth.getAssignedOutput());

	_segmentRfTrainer->setInput("ground truth", _groundTruthExtractor->getOutput());
	_segmentRfTrainer->setInput("all segments", _problemAssembler->getOutput("segments"));
	_segmentRfTrainer->setInput("raw sections", _rawSections.getAssignedOutput());
}
