#ifndef CELLTRACKER_CELLTRACKER_H__
#define CELLTRACKER_CELLTRACKER_H__

#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>

// forward declarations
class GroundTruthExtractor;
class ImageExtractor;
class ImageStack;
class LinearSolver;
class ObjectiveGenerator;
class ProblemAssembler;
class RandomForestHdf5Reader;
class Reconstructor;
class SectionSelector;
class SegmentationCostFunction;
class SegmentEvaluator;
class SegmentExtractor;
class SegmentFeaturesExtractor;
class RandomForestCostFunction;
class RandomForestTrainer;
class SliceExtractor;

class Sopnet : public pipeline::ProcessNode {

public:

	/**
	 * Create a new Sopnet process node. All the data will be read and written
	 * from and to HDF5 files in the given project directory.
	 *
	 * @param projectDirectory The directory to read and write the data from and to.
	 */
	Sopnet(const std::string& projectDirectory);

private:

	void onMembranesSet(const pipeline::InputSet<ImageStack>& signal);

	void onRawSectionsSet(const pipeline::InputSet<ImageStack>& signal);

	void onGroundTruthSet(const pipeline::InputSet<ImageStack>& signal);

	void createPipeline();

	void createBasicPipeline();

	void createInferencePipeline();

	void createTrainingPipeline();

	/**********
	 * INPUTS *
	 **********/

	// the raw images of the slices
	pipeline::Input<ImageStack> _rawSections;

	// the membrane classification output for the slices
	pipeline::Input<ImageStack> _membranes;

	// the ground truth images
	pipeline::Input<ImageStack> _groundTruth;

	/***********
	 * SIGNALS *
	 ***********/

	signals::Slot<pipeline::Update> _update;

	/*********************
	 * INTERNAL PIPELINE *
	 *********************/

	/*
	 * basic part
	 */

	// an image stack to image converter for the membranes
	boost::shared_ptr<ImageExtractor>                 _membraneExtractor;

	// a slice extractor for each section
	std::vector<boost::shared_ptr<SliceExtractor> >   _sliceExtractors;

	// a segment extractor for each pair of timesteps
	std::vector<boost::shared_ptr<SegmentExtractor> > _segmentExtractors;

	// the problem assembler that collects all segments and linear constraints
	boost::shared_ptr<ProblemAssembler>               _problemAssembler;

	/*
	 * inference part
	 */

	// a feature extractor computing features for each segment
	boost::shared_ptr<SegmentFeaturesExtractor>       _segmentFeaturesExtractor;

	// a random forest file reader
	boost::shared_ptr<RandomForestHdf5Reader>         _randomForestReader;

	// a segment evaluator that provides a cost function for segments
	boost::shared_ptr<RandomForestCostFunction>       _randomForestCostFunction;

	// a segment evaluator that provides a cost function for slices
	boost::shared_ptr<SegmentationCostFunction>       _segmentationCostFunction;

	// the objective generator that computes the costs for each segment
	boost::shared_ptr<ObjectiveGenerator>             _objectiveGenerator;

	// the linear solver
	boost::shared_ptr<LinearSolver>                   _linearSolver;

	// the last proess node in the internal pipeline, providing the final
	// solution
	boost::shared_ptr<Reconstructor>                  _reconstructor;

	/*
	 * training part
	 */

	// the ground truth extractor, gives segments from ground truth images
	boost::shared_ptr<GroundTruthExtractor>           _groundTruthExtractor;

	// the training node, trains a random forest classifier on the ground truth
	boost::shared_ptr<RandomForestTrainer>            _segmentRfTrainer;

	/**************************
	 * PROJECT INFRASTRUCTURE *
	 **************************/

	// the project directory
	std::string _projectDirectory;
};

#endif // CELLTRACKER_CELLTRACKER_H__

