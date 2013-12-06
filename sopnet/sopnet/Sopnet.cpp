#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include <imageprocessing/ImageStack.h>
#include <inference/io/RandomForestHdf5Reader.h>
#include <inference/LinearSolver.h>
#include <pipeline/Process.h>
#include <util/foreach.h>
#include <util/ProgramOptions.h>
#include <sopnet/evaluation/GroundTruthExtractor.h>
#include <sopnet/features/SegmentFeaturesExtractor.h>
#include <sopnet/inference/ProblemGraphWriter.h>
#include <sopnet/inference/ObjectiveGenerator.h>
#include <sopnet/inference/ProblemAssembler.h>
#include <sopnet/inference/SubproblemsExtractor.h>
#include <sopnet/inference/SubproblemsSolver.h>
#include <sopnet/inference/LinearCostFunction.h>
#include <sopnet/inference/io/LinearCostFunctionParametersReader.h>
#include <sopnet/inference/RandomForestCostFunction.h>
#include <sopnet/inference/SegmentationCostFunction.h>
#include <sopnet/inference/PriorCostFunction.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>
#include <sopnet/inference/Reconstructor.h>
#include <sopnet/io/FileContentProvider.h>
#include <sopnet/training/GoldStandardExtractor.h>
#include <sopnet/training/RandomForestTrainer.h>
#include "Sopnet.h"

static logger::LogChannel sopnetlog("sopnetlog", "[Sopnet] ");

util::ProgramOption optionLinearCostFunction(
		util::_module           = "sopnet.inference",
		util::_long_name        = "linearCostFunction",
		util::_description_text = "Use the linear cost function for segments.",
		util::_default_value    = true);

util::ProgramOption optionRandomForestCostFunction(
		util::_module           = "sopnet.inference",
		util::_long_name        = "rfCostFunction",
		util::_description_text = "Use the random forest cost function for segments.",
		util::_default_value    = true);

util::ProgramOption optionSegmentationCostFunction(
		util::_module           = "sopnet.inference",
		util::_long_name        = "segmentationCostFunction",
		util::_description_text = "Use the segmentation cost function (based on membrane probabilites) for segments.");

util::ProgramOption optionRandomForestFile(
		util::_module           = "sopnet.inference",
		util::_long_name        = "segmentRandomForest",
		util::_description_text = "Path to an HDF5 file containing the segment random forest.",
		util::_default_value    = "segment_rf.hdf");

util::ProgramOption optionLinearCostFunctionParametersFile(
		util::_module           = "sopnet.inference",
		util::_long_name        = "linearCostFunctionParameters",
		util::_description_text = "Path to a file containing the weights for the linear cost function.",
		util::_default_value    = "./feature_weights.dat");

util::ProgramOption optionDecomposeProblem(
		util::_module           = "sopnet.inference",
		util::_long_name        = "decomposeProblem",
		util::_description_text = "Decompose the problem into overlapping subproblems and solve them using SCALAR.",
		util::_default_value    = false);

Sopnet::Sopnet(
		const std::string& projectDirectory,
		boost::shared_ptr<ProcessNode> problemWriter) :
	_problemAssembler(boost::make_shared<ProblemAssembler>()),
	_segmentFeaturesExtractor(boost::make_shared<SegmentFeaturesExtractor>()),
	_randomForestReader(boost::make_shared<RandomForestHdf5Reader>(optionRandomForestFile.as<std::string>())),
	_priorCostFunction(boost::make_shared<PriorCostFunction>()),
	_objectiveGenerator(boost::make_shared<ObjectiveGenerator>()),
	_linearSolver(boost::make_shared<LinearSolver>()),
	_reconstructor(boost::make_shared<Reconstructor>()),
	_groundTruthExtractor(boost::make_shared<GroundTruthExtractor>()),
	_segmentRfTrainer(boost::make_shared<RandomForestTrainer>()),
	_projectDirectory(projectDirectory),
	_problemWriter(problemWriter) {

	// tell the outside world what we need
	registerInput(_rawSections, "raw sections");
	registerInput(_membranes, "membranes");
	registerInput(_neuronSlices, "neuron slices");
	registerInput(_neuronSliceStackDirectories, "neuron slice stack directories");
	registerInput(_mitochondriaSlices, "mitochondria slices");
	registerInput(_mitochondriaSliceStackDirectories, "mitochondria slice stack directories");
	registerInput(_synapseSlices, "synapse slices");
	registerInput(_synapseSliceStackDirectories, "synapse slice stack directories");
	registerInput(_groundTruth, "ground truth");
	registerInput(_segmentationCostFunctionParameters, "segmentation cost parameters");
	registerInput(_priorCostFunctionParameters, "prior cost parameters");
	registerInput(_forceExplanation, "force explanation");

	_membranes.registerBackwardCallback(&Sopnet::onMembranesSet, this);
	_neuronSlices.registerBackwardCallback(&Sopnet::onSlicesSet, this);
	_neuronSlices.registerBackwardSlot(_update);
	_neuronSliceStackDirectories.registerBackwardCallback(&Sopnet::onSlicesSet, this);
	_neuronSliceStackDirectories.registerBackwardSlot(_update);
	_mitochondriaSlices.registerBackwardCallback(&Sopnet::onSlicesSet, this);
	_mitochondriaSlices.registerBackwardSlot(_update);
	_mitochondriaSliceStackDirectories.registerBackwardCallback(&Sopnet::onSlicesSet, this);
	_mitochondriaSliceStackDirectories.registerBackwardSlot(_update);
	_synapseSlices.registerBackwardCallback(&Sopnet::onSlicesSet, this);
	_synapseSlices.registerBackwardSlot(_update);
	_synapseSliceStackDirectories.registerBackwardCallback(&Sopnet::onSlicesSet, this);
	_synapseSliceStackDirectories.registerBackwardSlot(_update);
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
	registerOutput(_segmentRfTrainer->getOutput("ground truth score"), "ground truth score");
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

	if (
			!_membranes ||
			!(_neuronSlices || _neuronSliceStackDirectories) ||
			!_rawSections ||
			!_segmentationCostFunctionParameters ||
			!_priorCostFunctionParameters ||
			!_forceExplanation) {

		LOG_DEBUG(sopnetlog) << "not all inputs present -- skip pipeline creation" << std::endl;
		return;
	}

	createBasicPipeline();

	createInferencePipeline();
	if (_groundTruth)
		createTrainingPipeline();
}

void
Sopnet::createBasicPipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating basic part..." << std::endl;

	// clear previous pipeline
	_problemAssembler->clearInputs("neuron segments");
	_problemAssembler->clearInputs("neuron linear constraints");
	_problemAssembler->clearInputs("mitochondria segments");
	_problemAssembler->clearInputs("mitochondria linear constraints");
	_problemAssembler->clearInputs("synapse segments");
	_problemAssembler->clearInputs("synapse linear constraints");

	// make sure relevant input information is available
	_update();

	if (_neuronSliceStackDirectories)
		_neuronSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_neuronSliceStackDirectories, !_problemWriter);
	else
		_neuronSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_neuronSlices, _forceExplanation, !_problemWriter);

	_mitochondriaSegmentExtractorPipeline.reset();
	if (_mitochondriaSliceStackDirectories)
		_mitochondriaSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_mitochondriaSliceStackDirectories, !_problemWriter);
	else if (_mitochondriaSlices)
		_mitochondriaSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_mitochondriaSlices, false, !_problemWriter);

	_synapseSegmentExtractorPipeline.reset();
	if (_synapseSliceStackDirectories)
		_synapseSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_synapseSliceStackDirectories, !_problemWriter);
	else if (_synapseSlices)
		_synapseSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_synapseSlices, false, !_problemWriter);

	for (unsigned int i = 0; i < _neuronSegmentExtractorPipeline->numIntervals(); i++) {

		// add segments and linear constraints to problem assembler
		_problemAssembler->addInput("neuron segments", _neuronSegmentExtractorPipeline->getSegments(i));
		_problemAssembler->addInput("neuron linear constraints", _neuronSegmentExtractorPipeline->getConstraints(i));

		if (_mitochondriaSegmentExtractorPipeline) {

			_problemAssembler->addInput("mitochondria segments", _mitochondriaSegmentExtractorPipeline->getSegments(i));
			_problemAssembler->addInput("mitochondria linear constraints", _mitochondriaSegmentExtractorPipeline->getConstraints(i));
		}

		if (_synapseSegmentExtractorPipeline) {

			_problemAssembler->addInput("synapse segments", _synapseSegmentExtractorPipeline->getSegments(i));
			_problemAssembler->addInput("synapse linear constraints", _synapseSegmentExtractorPipeline->getConstraints(i));
		}
	}
}

void
Sopnet::createInferencePipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating inference part..." << std::endl;

	// setup the segment feature extractor
	_segmentFeaturesExtractor->setInput("segments", _problemAssembler->getOutput("segments"));
	_segmentFeaturesExtractor->setInput("raw sections", _rawSections.getAssignedOutput());

	boost::shared_ptr<LinearCostFunction>       linearCostFunction;
	boost::shared_ptr<RandomForestCostFunction> rfCostFunction;
	boost::shared_ptr<SegmentationCostFunction> segmentationCostFunction;

	// setup the segment evaluation functions
	if (optionLinearCostFunction) {

		LOG_DEBUG(sopnetlog) << "creating linear segment cost function" << std::endl;

		linearCostFunction = boost::make_shared<LinearCostFunction>();
		boost::shared_ptr<LinearCostFunctionParametersReader> reader
				= boost::make_shared<LinearCostFunctionParametersReader>();
		boost::shared_ptr<FileContentProvider> contentProvider
				= boost::make_shared<FileContentProvider>(optionLinearCostFunctionParametersFile.as<std::string>());
		reader->setInput(contentProvider->getOutput());
		linearCostFunction->setInput("features", _segmentFeaturesExtractor->getOutput("all features"));
		linearCostFunction->setInput("parameters", reader->getOutput());

	}

	if (optionRandomForestCostFunction) {

		LOG_DEBUG(sopnetlog) << "creating random forest segment cost function" << std::endl;

		rfCostFunction = boost::make_shared<RandomForestCostFunction>();
		rfCostFunction->setInput("features", _segmentFeaturesExtractor->getOutput("all features"));
		rfCostFunction->setInput("random forest", _randomForestReader->getOutput("random forest"));
	}

	if (optionSegmentationCostFunction) {

		segmentationCostFunction = boost::make_shared<SegmentationCostFunction>();
		segmentationCostFunction->setInput("membranes", _membranes);
		segmentationCostFunction->setInput("parameters", _segmentationCostFunctionParameters);
	}

	_priorCostFunction->setInput("parameters", _priorCostFunctionParameters);

	if (_problemWriter) {

		_problemWriter->setInput("segments", _problemAssembler->getOutput("segments"));
		_problemWriter->setInput("problem configuration", _problemAssembler->getOutput("problem configuration"));
		_problemWriter->setInput("features", _segmentFeaturesExtractor->getOutput("all features"));
		
		// assuming one of the two was selected
		if (rfCostFunction)
			_problemWriter->setInput("segment cost function", rfCostFunction->getOutput("cost function"));
		else
			_problemWriter->setInput("segment cost function", linearCostFunction->getOutput("cost function"));

		_problemWriter->setInput("segmentation cost function", segmentationCostFunction->getOutput("cost function"));
		_problemWriter->addInput("linear constraints", _problemAssembler->getOutput("linear constraints"));

	} else {

		// feed all segments to objective generator
		_objectiveGenerator->setInput("segments", _problemAssembler->getOutput("segments"));
		if (rfCostFunction)
			_objectiveGenerator->addInput("cost functions", rfCostFunction->getOutput("cost function"));
		if (linearCostFunction)
			_objectiveGenerator->addInput("cost functions", linearCostFunction->getOutput("cost function"));
		if (segmentationCostFunction)
			_objectiveGenerator->addInput("cost functions", segmentationCostFunction->getOutput("cost function"));

		if (optionDecomposeProblem) {

			pipeline::Process<SubproblemsExtractor> subproblemsExtractor;
			pipeline::Process<SubproblemsSolver>    subproblemsSolver;

			subproblemsExtractor->setInput("objective", _objectiveGenerator->getOutput());
			subproblemsExtractor->setInput("linear constraints", _problemAssembler->getOutput("linear constraints"));
			subproblemsExtractor->setInput("problem configuration", _problemAssembler->getOutput("problem configuration"));

			subproblemsSolver->setInput(subproblemsExtractor->getOutput());

			// feed solution and segments to reconstructor
			_reconstructor->setInput("solution", subproblemsSolver->getOutput("solution"));
			_reconstructor->setInput("segments", _problemAssembler->getOutput("segments"));

		} else {

			// feed objective and linear constraints to ilp creator
			_linearSolver->setInput("objective", _objectiveGenerator->getOutput());
			_linearSolver->setInput("linear constraints", _problemAssembler->getOutput("linear constraints"));
			_linearSolver->setInput("parameters", boost::make_shared<LinearSolverParameters>(Binary));

			// feed solution and segments to reconstructor
			_reconstructor->setInput("solution", _linearSolver->getOutput("solution"));
			_reconstructor->setInput("segments", _problemAssembler->getOutput("segments"));
		}
	}

}

void
Sopnet::createTrainingPipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating training part..." << std::endl;

	_groundTruthExtractor->setInput(_groundTruth.getAssignedOutput());

	_segmentRfTrainer->setInput("ground truth", _groundTruthExtractor->getOutput());
	_segmentRfTrainer->setInput("all segments", _problemAssembler->getOutput("segments"));
	_segmentRfTrainer->setInput("raw sections", _rawSections.getAssignedOutput());
}
