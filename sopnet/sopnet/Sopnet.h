#ifndef CELLTRACKER_CELLTRACKER_H__
#define CELLTRACKER_CELLTRACKER_H__

#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>
#include <sopnet/inference/PriorCostFunctionParameters.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>
#include <sopnet/segments/SegmentExtractionPipeline.h>

// forward declarations
class GoldStandardExtractor;
class GroundTruthExtractor;
class ImageExtractor;
class ImageStack;
class LinearSolver;
class ObjectiveGenerator;
class PriorCostFunction;
class ProblemAssembler;
class RandomForestCostFunction;
class RandomForestHdf5Reader;
class Reconstructor;
class SectionSelector;
class SegmentEvaluator;
class SegmentExtractor;
class SegmentFeaturesExtractor;
class SegmentRandomForestTrainer;
class SegmentationCostFunction;
class StructuredProblemWriter;
class MinimalImpactTEDWriter;
template <typename Precision> class SliceExtractor;

class Sopnet : public pipeline::SimpleProcessNode<> {

public:

	/**
	 * Create a new Sopnet process node. All the data will be read and written
	 * from and to HDF5 files in the given project directory.
	 *
	 * @param projectDirectory The directory to read and write the data from and to.
	 */
	Sopnet(const std::string& projectDirectory, boost::shared_ptr<ProcessNode> problemWriter = boost::shared_ptr<ProcessNode>());

	void writeStructuredProblem(std::string filename_labels, std::string filename_features, std::string filename_constraints);

	void writeMinimalImpactRANDCoefficients(std::string filename);
	void writeMinimalImpactTEDCoefficients(std::string filename);

private:

	void updateOutputs();

	void createPipeline();

	void createBasicPipeline();

	void createInferencePipeline();

	void createTrainingPipeline();

	void createStructuredProblemPipeline();

	void createMinimalImpactTEDPipeline();

	/**********
	 * INPUTS *
	 **********/

	// the raw images of the slices
	pipeline::Input<ImageStack> _rawSections;

	// the membrane classification output for the slices
	pipeline::Input<ImageStack> _membranes;

	// the segmentation hypotheses for the neuron slices
	pipeline::Input<ImageStack> _neuronSlices;

	// the names of the neuron slice stacks directories
	pipeline::Input<std::vector<std::string> > _neuronSliceStackDirectories;

	// the segmentation hypotheses for the mitochondria slices
	pipeline::Input<ImageStack> _mitochondriaSlices;

	// the names of the mitochondria slice stacks directories
	pipeline::Input<std::vector<std::string> > _mitochondriaSliceStackDirectories;

	// the segmentation hypotheses for the synapse slices
	pipeline::Input<ImageStack> _synapseSlices;

	// the names of the synapse slice stacks directories
	pipeline::Input<std::vector<std::string> > _synapseSliceStackDirectories;

	// the ground truth images
	pipeline::Input<ImageStack> _groundTruth;

	// parameters of the segmentation cost function
	pipeline::Input<SegmentationCostFunctionParameters> _segmentationCostFunctionParameters;

	// parameters of the prior cost function
	pipeline::Input<PriorCostFunctionParameters> _priorCostFunctionParameters;

	// force the explanation of every component tree
	pipeline::Input<bool> _forceExplanation;

	/*********************
	 * INTERNAL PIPELINE *
	 *********************/

	/*
	 * basic part
	 */

	boost::shared_ptr<SegmentExtractionPipeline>      	_neuronSegmentExtractorPipeline;

	boost::shared_ptr<SegmentExtractionPipeline>      	_mitochondriaSegmentExtractorPipeline;

	boost::shared_ptr<SegmentExtractionPipeline>      	_synapseSegmentExtractorPipeline;

	// the problem assembler that collects all segments and linear constraints
	boost::shared_ptr<ProblemAssembler>               	_problemAssembler;

	/*
	 * inference part
	 */

	// a feature extractor computing features for each segment
	boost::shared_ptr<SegmentFeaturesExtractor>       	_segmentFeaturesExtractor;

	// a random forest file reader
	boost::shared_ptr<RandomForestHdf5Reader>         	_randomForestReader;

	// the objective generator that computes the costs for each segment
	boost::shared_ptr<ObjectiveGenerator>             	_objectiveGenerator;

	// the linear solver
	boost::shared_ptr<LinearSolver>                   	_linearSolver;

	// the last proess node in the internal pipeline, providing the final
	// solution
	boost::shared_ptr<Reconstructor>                  	_reconstructor;

	/*
	 * training part
	 */

	// the ground truth extractor, gives segments from ground truth images
	boost::shared_ptr<GroundTruthExtractor>           	_groundTruthExtractor;

	// the gold standard extractor, gives the closest candidate solution to the 
	// groundtruth
	boost::shared_ptr<pipeline::ProcessNode>          _goldStandardProvider;

	// the training node, trains a random forest classifier on the gold standard
	boost::shared_ptr<SegmentRandomForestTrainer>     _segmentRfTrainer;

	// a writer to produce output for structured learning
	boost::shared_ptr<StructuredProblemWriter>		_spWriter;

	// a writer to write the coefficients for minimal imapact TED for structured learning
	boost::shared_ptr<MinimalImpactTEDWriter>		_mitWriter;

	/**************************
	 * PROJECT INFRASTRUCTURE *
	 **************************/

	// the project directory
	std::string _projectDirectory;

	// a writer to dump a description of the problem
	boost::shared_ptr<ProcessNode> _problemWriter;

	bool _pipelineCreated;
};

#endif // CELLTRACKER_CELLTRACKER_H__

