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
#include <sopnet/training/io/GoldStandardFileReader.h>
#include <sopnet/training/SegmentRandomForestTrainer.h>
#include <sopnet/training/io/StructuredProblemWriter.h>
#include <sopnet/training/io/MinimalImpactTEDWriter.h>
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

util::ProgramOption optionPriorCostFunction(
		util::_module           = "sopnet.inference",
		util::_long_name        = "priorCostFunction",
		util::_description_text = "Use the prior cost function for segments (only really makes sense for gridSearch, since linearCostFunction provides the same).",
		util::_default_value    = false);

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

util::ProgramOption optionReadGoldStandardFromFile(
		util::_module           = "sopnet.training",
		util::_long_name        = "readGoldStandardFromFile",
		util::_description_text = "Instead of looking at the ground truth, read the gold standard from a file with segment hashes, where each line is of the form: [0 or 1 (part of GS or not)] # [hash].");

Sopnet::Sopnet(
		const std::string& projectDirectory,
		boost::shared_ptr<ProcessNode> problemWriter) :
	_problemAssembler(boost::make_shared<ProblemAssembler>()),
	_segmentFeaturesExtractor(boost::make_shared<SegmentFeaturesExtractor>()),
	_randomForestReader(boost::make_shared<RandomForestHdf5Reader>(optionRandomForestFile.as<std::string>())),
	_objectiveGenerator(boost::make_shared<ObjectiveGenerator>()),
	_linearSolver(boost::make_shared<LinearSolver>()),
	_reconstructor(boost::make_shared<Reconstructor>()),
	_groundTruthExtractor(boost::make_shared<GroundTruthExtractor>()),
	_segmentRfTrainer(boost::make_shared<SegmentRandomForestTrainer>()),
	_spWriter(boost::make_shared<StructuredProblemWriter>()),
	_mitWriter(boost::make_shared<MinimalImpactTEDWriter>()),
	_projectDirectory(projectDirectory),
	_problemWriter(problemWriter),
	_pipelineCreated(false) {

	// tell the outside world what we need
	registerInput(_rawSections, "raw sections");
	registerInput(_membranes, "membranes");
	registerInput(_neuronSlices, "neuron slices", pipeline::Optional);
	registerInput(_neuronSliceStackDirectories, "neuron slice stack directories", pipeline::Optional);
	registerInput(_mitochondriaSlices, "mitochondria slices", pipeline::Optional);
	registerInput(_mitochondriaSliceStackDirectories, "mitochondria slice stack directories", pipeline::Optional);
	registerInput(_synapseSlices, "synapse slices", pipeline::Optional);
	registerInput(_synapseSliceStackDirectories, "synapse slice stack directories", pipeline::Optional);
	registerInput(_groundTruth, "ground truth", pipeline::Optional);
	registerInput(_segmentationCostFunctionParameters, "segmentation cost parameters");
	registerInput(_priorCostFunctionParameters, "prior cost parameters");
	registerInput(_forceExplanation, "force explanation");

	if (optionReadGoldStandardFromFile) {

		LOG_USER(sopnetlog) << "reading gold standard from file " << optionReadGoldStandardFromFile.as<std::string>() << std::endl;

		_goldStandardProvider = boost::make_shared<GoldStandardFileReader>(optionReadGoldStandardFromFile.as<std::string>());

	} else {

		LOG_USER(sopnetlog) << "extract gold standard from ground truth" << std::endl;

		_goldStandardProvider = boost::make_shared<GoldStandardExtractor>();
	}

	// tell the outside world what we've got
	registerOutput(_reconstructor->getOutput(), "solution");
	registerOutput(_problemAssembler->getOutput("segments"), "segments");
	registerOutput(_problemAssembler->getOutput("problem configuration"), "problem configuration");
	registerOutput(_objectiveGenerator->getOutput("objective"), "objective");
	registerOutput(_groundTruthExtractor->getOutput("ground truth segments"), "ground truth segments");
	registerOutput(_goldStandardProvider->getOutput("gold standard"), "gold standard");
	registerOutput(_goldStandardProvider->getOutput("negative samples"), "negative samples");
	registerOutput(_segmentRfTrainer->getOutput("random forest"), "random forest");
	registerOutput(_segmentFeaturesExtractor->getOutput("all features"), "all features");

	// set input-output dependencies
	setDependency(_rawSections, _reconstructor->getOutput());
	setDependency(_rawSections, _objectiveGenerator->getOutput("objective"));
	setDependency(_rawSections, _segmentRfTrainer->getOutput("random forest"));
	setDependency(_rawSections, _segmentFeaturesExtractor->getOutput("all features"));

	setDependency(_membranes, _reconstructor->getOutput());
	setDependency(_membranes, _objectiveGenerator->getOutput("objective"));
	setDependency(_membranes, _segmentRfTrainer->getOutput("random forest"));
	setDependency(_membranes, _segmentFeaturesExtractor->getOutput("all features"));

	setDependency(_neuronSlices, _reconstructor->getOutput());
	setDependency(_neuronSlices, _problemAssembler->getOutput("segments"));
	setDependency(_neuronSlices, _problemAssembler->getOutput("problem configuration"));
	setDependency(_neuronSlices, _objectiveGenerator->getOutput("objective"));
	setDependency(_neuronSlices, _goldStandardProvider->getOutput("gold standard"));
	setDependency(_neuronSlices, _goldStandardProvider->getOutput("negative samples"));
	setDependency(_neuronSlices, _segmentRfTrainer->getOutput("random forest"));
	setDependency(_neuronSlices, _segmentFeaturesExtractor->getOutput("all features"));
	setDependency(_neuronSliceStackDirectories, _reconstructor->getOutput());
	setDependency(_neuronSliceStackDirectories, _problemAssembler->getOutput("segments"));
	setDependency(_neuronSliceStackDirectories, _problemAssembler->getOutput("problem configuration"));
	setDependency(_neuronSliceStackDirectories, _objectiveGenerator->getOutput("objective"));
	setDependency(_neuronSliceStackDirectories, _goldStandardProvider->getOutput("gold standard"));
	setDependency(_neuronSliceStackDirectories, _goldStandardProvider->getOutput("negative samples"));
	setDependency(_neuronSliceStackDirectories, _segmentRfTrainer->getOutput("random forest"));
	setDependency(_neuronSliceStackDirectories, _segmentFeaturesExtractor->getOutput("all features"));
	setDependency(_mitochondriaSlices, _reconstructor->getOutput());
	setDependency(_mitochondriaSlices, _problemAssembler->getOutput("segments"));
	setDependency(_mitochondriaSlices, _problemAssembler->getOutput("problem configuration"));
	setDependency(_mitochondriaSlices, _objectiveGenerator->getOutput("objective"));
	setDependency(_mitochondriaSlices, _goldStandardProvider->getOutput("gold standard"));
	setDependency(_mitochondriaSlices, _goldStandardProvider->getOutput("negative samples"));
	setDependency(_mitochondriaSlices, _segmentRfTrainer->getOutput("random forest"));
	setDependency(_mitochondriaSlices, _segmentFeaturesExtractor->getOutput("all features"));
	setDependency(_mitochondriaSliceStackDirectories, _reconstructor->getOutput());
	setDependency(_mitochondriaSliceStackDirectories, _problemAssembler->getOutput("segments"));
	setDependency(_mitochondriaSliceStackDirectories, _problemAssembler->getOutput("problem configuration"));
	setDependency(_mitochondriaSliceStackDirectories, _objectiveGenerator->getOutput("objective"));
	setDependency(_mitochondriaSliceStackDirectories, _goldStandardProvider->getOutput("gold standard"));
	setDependency(_mitochondriaSliceStackDirectories, _goldStandardProvider->getOutput("negative samples"));
	setDependency(_mitochondriaSliceStackDirectories, _segmentRfTrainer->getOutput("random forest"));
	setDependency(_mitochondriaSliceStackDirectories, _segmentFeaturesExtractor->getOutput("all features"));
	setDependency(_synapseSlices, _reconstructor->getOutput());
	setDependency(_synapseSlices, _problemAssembler->getOutput("segments"));
	setDependency(_synapseSlices, _problemAssembler->getOutput("problem configuration"));
	setDependency(_synapseSlices, _objectiveGenerator->getOutput("objective"));
	setDependency(_synapseSlices, _goldStandardProvider->getOutput("gold standard"));
	setDependency(_synapseSlices, _goldStandardProvider->getOutput("negative samples"));
	setDependency(_synapseSlices, _segmentRfTrainer->getOutput("random forest"));
	setDependency(_synapseSlices, _segmentFeaturesExtractor->getOutput("all features"));
	setDependency(_synapseSliceStackDirectories, _reconstructor->getOutput());
	setDependency(_synapseSliceStackDirectories, _problemAssembler->getOutput("segments"));
	setDependency(_synapseSliceStackDirectories, _problemAssembler->getOutput("problem configuration"));
	setDependency(_synapseSliceStackDirectories, _objectiveGenerator->getOutput("objective"));
	setDependency(_synapseSliceStackDirectories, _goldStandardProvider->getOutput("gold standard"));
	setDependency(_synapseSliceStackDirectories, _goldStandardProvider->getOutput("negative samples"));
	setDependency(_synapseSliceStackDirectories, _segmentRfTrainer->getOutput("random forest"));
	setDependency(_synapseSliceStackDirectories, _segmentFeaturesExtractor->getOutput("all features"));

	setDependency(_groundTruth, _groundTruthExtractor->getOutput("ground truth segments"));
	setDependency(_groundTruth, _goldStandardProvider->getOutput("gold standard"));
	setDependency(_groundTruth, _goldStandardProvider->getOutput("negative samples"));
	setDependency(_groundTruth, _segmentRfTrainer->getOutput("random forest"));

	setDependency(_segmentationCostFunctionParameters, _reconstructor->getOutput());
	setDependency(_segmentationCostFunctionParameters, _objectiveGenerator->getOutput("objective"));

	setDependency(_priorCostFunctionParameters, _reconstructor->getOutput());
	setDependency(_priorCostFunctionParameters, _objectiveGenerator->getOutput("objective"));

	setDependency(_forceExplanation, _reconstructor->getOutput());
	setDependency(_forceExplanation, _goldStandardProvider->getOutput("gold standard"));
	setDependency(_forceExplanation, _goldStandardProvider->getOutput("negative samples"));
	setDependency(_forceExplanation, _segmentRfTrainer->getOutput("random forest"));
}

void
Sopnet::updateOutputs() {

	LOG_DEBUG(sopnetlog) << "update requested" << std::endl;

	createPipeline();
}

void
Sopnet::createPipeline() {

	if (_pipelineCreated) {

		LOG_DEBUG(sopnetlog) << "pipeline has been created already, skipping" << std::endl;
		return;
	}

	LOG_DEBUG(sopnetlog) << "creating pipeline" << std::endl;

	if (!_neuronSlices.isSet() && !_neuronSliceStackDirectories.isSet())
		UTIL_THROW_EXCEPTION(
				UsageError,
				"either an image stack of slices or a list of directory names has to be set as 'neuron slices' or 'neuron slice stack directories'");

	createBasicPipeline();
	createInferencePipeline();
	if (_groundTruth.isSet()) {

		createTrainingPipeline();
		createStructuredProblemPipeline();
		createMinimalImpactTEDPipeline();
	}

	_pipelineCreated = true;
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

	bool finishLastSection = !_problemWriter;

	LOG_DEBUG(sopnetlog) << "creating neuron segment part..." << std::endl;

	if (_neuronSliceStackDirectories.isSet())
		_neuronSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_neuronSliceStackDirectories.getSharedPointer(), finishLastSection);
	else
		_neuronSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_neuronSlices.getSharedPointer(), _forceExplanation, finishLastSection);

	LOG_DEBUG(sopnetlog) << "creating mitochondria segment part..." << std::endl;

	_mitochondriaSegmentExtractorPipeline.reset();
	if (_mitochondriaSliceStackDirectories.isSet())
		_mitochondriaSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_mitochondriaSliceStackDirectories.getSharedPointer(), finishLastSection);
	else if (_mitochondriaSlices.isSet())
		_mitochondriaSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_mitochondriaSlices.getSharedPointer(), false, finishLastSection);

	LOG_DEBUG(sopnetlog) << "creating synapse segment part..." << std::endl;

	_synapseSegmentExtractorPipeline.reset();
	if (_synapseSliceStackDirectories.isSet())
		_synapseSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_synapseSliceStackDirectories.getSharedPointer(), finishLastSection);
	else if (_synapseSlices.isSet())
		_synapseSegmentExtractorPipeline = boost::make_shared<SegmentExtractionPipeline>(_synapseSlices.getSharedPointer(), false, finishLastSection);

	LOG_DEBUG(sopnetlog) << "feeding output into problem assembler..." << std::endl;

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

	if (_groundTruth.isSet())
		_groundTruthExtractor->setInput(_groundTruth);
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
	boost::shared_ptr<PriorCostFunction>        priorCostFunction;

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

	if (optionPriorCostFunction) {

		priorCostFunction = boost::make_shared<PriorCostFunction>();
		priorCostFunction->setInput("parameters", _priorCostFunctionParameters);
	}

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
		if (priorCostFunction)
			_objectiveGenerator->addInput("cost functions", priorCostFunction->getOutput("cost function"));

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

	if (optionReadGoldStandardFromFile) {

		_goldStandardProvider->setInput("all segments", _problemAssembler->getOutput("segments"));

	} else {

		_goldStandardProvider->setInput("ground truth", _groundTruthExtractor->getOutput());
		_goldStandardProvider->setInput("all segments", _problemAssembler->getOutput("segments"));
		_goldStandardProvider->setInput("all linear constraints", _problemAssembler->getOutput("linear constraints"));
	}

	_segmentRfTrainer->setInput("positive samples", _goldStandardProvider->getOutput("gold standard"));
	_segmentRfTrainer->setInput("negative samples", _goldStandardProvider->getOutput("negative samples"));
	_segmentRfTrainer->setInput("features", _segmentFeaturesExtractor->getOutput("all features"));
}

void
Sopnet::createStructuredProblemPipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating structured problem part..." << std::endl;

	_spWriter->setInput("linear constraints", _problemAssembler->getOutput("linear constraints"));
	_spWriter->setInput("problem configuration", _problemAssembler->getOutput("problem configuration"));
	_spWriter->setInput("features", _segmentFeaturesExtractor->getOutput("all features"));
	_spWriter->setInput("segments", _problemAssembler->getOutput("segments"));
	_spWriter->setInput("gold standard", _goldStandardExtractor->getOutput("gold standard"));
	_spWriter->setInput("gold standard objective", _goldStandardExtractor->getOutput("gold standard objective"));
}

void
Sopnet::writeStructuredProblem(std::string filename_labels, std::string filename_features, std::string filename_constraints) {

	LOG_DEBUG(sopnetlog) << "requested to write structured problem, updating inputs" << std::endl;

	updateInputs();

	LOG_DEBUG(sopnetlog) << "creating internal pipeline, if not created yet" << std::endl;

	createPipeline();

	LOG_DEBUG(sopnetlog) << "writing structured learning files" << std::endl;

	_spWriter->write(filename_labels, filename_features, filename_constraints);
}

void
Sopnet::createMinimalImpactTEDPipeline() {

	LOG_DEBUG(sopnetlog) << "re-creating minimal impact TED part..." << std::endl;

	// Set inputs to MinimalImpactTEDWriter
	_mitWriter->setInput("gold standard", _goldStandardProvider->getOutput("gold standard"));
	_mitWriter->setInput("ground truth", _groundTruth);
	_mitWriter->setInput("segments", _problemAssembler->getOutput("segments"));
	_mitWriter->setInput("linear constraints", _problemAssembler->getOutput("linear constraints"));
	_mitWriter->setInput("reference", _rawSections);
	_mitWriter->setInput("problem configuration", _problemAssembler->getOutput("problem configuration"));

}

void
Sopnet::writeMinimalImpactTEDCoefficients(std::string filename) {

	LOG_DEBUG(sopnetlog) << "requested to write minimal impact TED coefficients, updating inputs" << std::endl;

	updateInputs();

	LOG_DEBUG(sopnetlog) << "creating internal pipeline, if not created yet" << std::endl;

	createPipeline();

	LOG_DEBUG(sopnetlog) << "writing minimal impact TED coefficient files..." << std::endl;

	_mitWriter->write(filename);
}

