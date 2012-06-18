#ifndef SOPNET_SEGMENT_RANDOM_FOREST_TRAINER_H__
#define SOPNET_SEGMENT_RANDOM_FOREST_TRAINER_H__

#include <pipeline.h>
#include <imageprocessing/ImageStack.h>
#include <inference/LinearConstraints.h>
#include <inference/RandomForest.h>
#include "RandomForestTrainer.h"
#include "Segments.h"

// forward declarations
class GoldStandardExtractor;
class SegmentFeaturesExtractor;

class SegmentRandomForestTrainer : public pipeline::ProcessNode {

public:

	SegmentRandomForestTrainer();

private:

	void onSegmentsSet(const pipeline::InputSet<Segments>& signal);

	pipeline::Input<Segments> _groundTruth;

	pipeline::Input<Segments> _allSegments;

	pipeline::Input<LinearConstraints> _linearConstraints;

	pipeline::Input<ImageStack> _rawSections;

	pipeline::Output<RandomForest> _randomForest;

	// finds the closest segements to the given ground truth
	boost::shared_ptr<GoldStandardExtractor> _goldStandardExtractor;

	// extracts the features for all given segments
	boost::shared_ptr<SegmentFeaturesExtractor> _segmentFeaturesExtractor;

	// trains a random forest give positve and negative samples
	boost::shared_ptr<RandomForestTrainer> _rfTrainer;
};

#endif // SOPNET_SEGMENT_RANDOM_FOREST_TRAINER_H__

