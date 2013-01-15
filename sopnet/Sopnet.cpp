#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include <imageprocessing/ImageStack.h>
#include <imageprocessing/ImageExtractor.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <inference/io/RandomForestHdf5Reader.h>
#include <inference/LinearSolver.h>
#include <util/foreach.h>
#include <util/ProgramOptions.h>
#include <sopnet/evaluation/GroundTruthExtractor.h>
#include <sopnet/features/SegmentFeaturesExtractor.h>
#include <sopnet/inference/ObjectiveGenerator.h>
#include <sopnet/inference/ProblemAssembler.h>
#include <sopnet/inference/RandomForestCostFunction.h>
#include <sopnet/inference/SegmentationCostFunction.h>
#include <sopnet/inference/PriorCostFunction.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>
#include <sopnet/inference/Reconstructor.h>
#include <sopnet/segments/SegmentExtractor.h>
#include <sopnet/slices/SliceExtractor.h>
#include <sopnet/slices/StackSliceExtractor.h>
#include <sopnet/training/GoldStandardExtractor.h>
#include <sopnet/training/RandomForestTrainer.h>
#include "Sopnet.h"

static logger::LogChannel sopnetlog("sopnetlog", "[Sopnet] ");

util::ProgramOption optionRandomForestFile(
		util::_module           = "sopnet",
		util::_long_name        = "segmentRandomForest",
		util::_description_text = "Path to an HDF5 file containing the segment random forest.",
		util::_default_value    = "segment_rf.hdf");

util::ProgramOption optionDisableSegmentationCosts(
		util::_module           = "sopnet.inference",
		util::_long_name        = "disableSegmentationCosts",
		util::_description_text = "Disable the segmentation costs in the objective.");

Sopnet::Sopnet(const std::string& projectDirectory) :
	_sliceImageExtractor(boost::make_shared<ImageExtractor>()),
	_problemAssembler(boost::make_shared<ProblemAssembler>()),
	_segmentFeaturesExtractor(boost::make_shared<SegmentFeaturesExtractor>()),
	_randomForestReader(boost::make_shared<RandomForestHdf5Reader>(optionRandomForestFile.as<std::string>())),
	_randomForestCostFunction(boost::make_shared<RandomForestCostFunction>()),
	_segmentationCostFunction(boost::make_shared<SegmentationCostFunction>()),
	_priorCostFunction(boost::make_shared<PriorCostFunction>()),
	_objectiveGenerator(boost::make_shared<ObjectiveGenerator>()),
	_linearSolver(boost::make_shared<LinearSolver>()),
	_reconstructor(boost::make_shared<Reconstructor>()),
	_groundTruthExtractor(boost::make_shared<GroundTruthExtractor>()),
	_segmentRfTrainer(boost::make_shared<RandomForestTrainer>()),
	_projectDirectory(projectDirectory) {

	// tell the outside world what we need
	registerInput(_rawSections, "raw sections");
	registerInput(_membranes, "membranes");
	registerInput(_slices, "slices");
	registerInput(_sliceStackDirectories, "slice stack directories");
	registerInput(_groundTruth, "ground truth");
	registerInput(_segmentationCostFunctionParameters, "segmentation cost parameters");
	registerInput(_priorCostFunctionParameters, "prior cost parameters");
	registerInput(_forceExplanation, "force explanation");

	_membranes.registerBackwardCallback(&Sopnet::onMembranesSet, this);
	_slices.registerBackwardCallback(&Sopnet::onSlicesSet, this);
	_slices.registerBackwardSlot(_update);
	_sliceStackDirectories.registerBackwardCallback(&Sopnet::onSlicesSet, this);
	_sliceStackDirectories.registerBackwardSlot(_update);
	_rawSections.registerBackwardCallback(&Sopnet::onRawSectionsSet, this);
	_groundTruth.registerBackwardCallback(&Sopnet::onGroundTruthSet, this);
	_segmentationCostFunctionParameters.registerBackwardCallback(&Sopnet::onParametersSet, this);
	_priorCostFunctionParameters.registerBackwardCallback(&Sopnet::onParametersSet, this);
	_forceExplanation.registerBackwardCallback(&Sopnet::onParametersSet, this);

	// tell the outside world what we've got
	registerOutput(_reconstructor->getOutput(), "solution");
	registerOutput(_problemAssembler->getOutput("segments"), "segments");
	registerOutput(_problemAssembler->getOutput("problem configuration"), "problem configuration");
	registerOutput(_objectiveGenerator->getOutput("objective"), "objective");
	registerOutput(_groundTruthExtractor->getOutput("ground truth segments"), "ground truth segments");
	registerOutput(_segmentRfTrainer->getOutput("gold standard"), "gold standard");
	registerOutput(_segmentRfTrainer->getOutput("negative samples"), "negative samples");
	registerOutput(_segmentRfTrainer->getOutput("random forest"), "random forest");
	registerOutput(_segmentFeaturesExtractor->getOutput("all features"), "all features");
}

void
Sopnet::onMembranesSet(const pipeline::InputSet<ImageStack>&) {

	LOG_DEBUG(sopnetlog) << "membranes set" << std::endl;

	createPipeline();
}

void
Sopnet::onSlicesSet(const pipeline::InputSet<ImageStack>&) {

	LOG_DEBUG(sopnetlog) << "slices set" << std::endl;

	createPipeline();
}

void
Sopnet::onRawSectionsSet(const pipeline::InputSet<ImageStack>&) {

	LOG_DEBUG(sopnetlog) << "raw sections set" << std::endl;

	createPipeline();
}

void
Sopnet::onGroundTruthSet(const pipeline::InputSet<ImageStack>&) {

	LOG_DEBUG(sopnetlog) << "ground-truth set" << std::endl;

	createPipeline();
}

void
Sopnet::onParametersSet(const pipeline::InputSetBase&) {

	LOG_DEBUG(sopnetlog) << "parameters set" << std::endl;

	createPipeline();
}

void
Sopnet::createPipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating pipeline" << std::endl;

	if (!_membranes || !(_slices || _sliceStackDirectories) || !_rawSections || !_groundTruth || !_segmentationCostFunctionParameters || !_priorCostFunctionParameters || !_forceExplanation) {

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

	unsigned int numSections = 0;

	std::vector<boost::shared_ptr<ImageStackDirectoryReader> > stackSliceReaders;

	// make sure relevant input information is available
	_update();

	if (_sliceStackDirectories) {

		// for every stack directory
		foreach (std::string directory, *_sliceStackDirectories) {

			if (boost::filesystem::is_directory(directory)) {

				numSections++;

				LOG_DEBUG(sopnetlog) << "creating stack reader for " << directory << std::endl;

				// create a new image stack reader
				boost::shared_ptr<ImageStackDirectoryReader> reader = boost::make_shared<ImageStackDirectoryReader>(directory);

				stackSliceReaders.push_back(reader);
			}
		}

	} else {

		numSections = _slices->size();

		// let the internal image extractor know where to look for the image stack
		_sliceImageExtractor->setInput(_slices.getAssignedOutput());
	}

	LOG_DEBUG(sopnetlog) << "creating pipeline for " << numSections << " sections" << std::endl;

	// for every section
	for (unsigned int section = 0; section < numSections; section++) {

		boost::shared_ptr<ProcessNode> sliceExtractor;

		LOG_DEBUG(sopnetlog) << "creating pipeline for section " << section << std::endl;

		if (_sliceStackDirectories) {

			// create image stack slice extractor
			sliceExtractor = boost::make_shared<StackSliceExtractor>(section);

			// set its input
			sliceExtractor->setInput("slices", stackSliceReaders[section]->getOutput());

		} else {

			// create a single image slice extractor
			sliceExtractor = boost::make_shared<SliceExtractor>(section);

			// set its input
			sliceExtractor->setInput("membrane", _sliceImageExtractor->getOutput(section));
		}

		sliceExtractor->setInput("force explanation", _forceExplanation);

		// store it in the list of all slice extractors
		_sliceExtractors.push_back(sliceExtractor);

		if (_sliceExtractors.size() <= 1)
			continue;

		// get the previous slice file reader
		boost::shared_ptr<ProcessNode> prevSliceExtractor = _sliceExtractors[_sliceExtractors.size() - 2];

		// create a segment extractor
		boost::shared_ptr<SegmentExtractor> segmentExtractor = boost::make_shared<SegmentExtractor>();

		// connect current and previous slices to that
		segmentExtractor->setInput("previous slices", prevSliceExtractor->getOutput("slices"));
		segmentExtractor->setInput("next slices", sliceExtractor->getOutput("slices"));
		segmentExtractor->setInput("previous linear constraints", prevSliceExtractor->getOutput("linear constraints"));
		if (section == numSections - 1) // only for the last pair of slices
			segmentExtractor->setInput("next linear constraints", sliceExtractor->getOutput("linear constraints"));

		_segmentExtractors.push_back(segmentExtractor);

		// add segments and linear constraints to problem assembler
		_problemAssembler->addInput("segments", segmentExtractor->getOutput("segments"));
		_problemAssembler->addInput("linear constraints", segmentExtractor->getOutput("linear constraints"));
	}
}

void
Sopnet::createInferencePipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating inference part..." << std::endl;

	// setup the segment feature extractor
	_segmentFeaturesExtractor->setInput("segments", _problemAssembler->getOutput("segments"));
	_segmentFeaturesExtractor->setInput("raw sections", _rawSections.getAssignedOutput());

	// setup the segment evaluation functions
	_randomForestCostFunction->setInput("features", _segmentFeaturesExtractor->getOutput("all features"));
	_randomForestCostFunction->setInput("random forest", _randomForestReader->getOutput("random forest"));
	_segmentationCostFunction->setInput("membranes", _membranes);
	_segmentationCostFunction->setInput("parameters", _segmentationCostFunctionParameters);
	_priorCostFunction->setInput("parameters", _priorCostFunctionParameters);

	// feed all segments to objective generator
	_objectiveGenerator->setInput("segments", _problemAssembler->getOutput("segments"));
	_objectiveGenerator->addInput("cost functions", _randomForestCostFunction->getOutput("cost function"));
	_objectiveGenerator->addInput("cost functions", _priorCostFunction->getOutput("cost function"));

	if (!optionDisableSegmentationCosts)
		_objectiveGenerator->addInput("cost functions", _segmentationCostFunction->getOutput("cost function"));

	// feed objective and linear constraints to ilp creator
	_linearSolver->setInput("objective", _objectiveGenerator->getOutput());
	_linearSolver->setInput("linear constraints", _problemAssembler->getOutput("linear constraints"));
	_linearSolver->setInput("parameters", boost::make_shared<LinearSolverParameters>(Binary));

	// feed solution and segments to reconstructor
	_reconstructor->setInput("solution", _linearSolver->getOutput("solution"));
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
