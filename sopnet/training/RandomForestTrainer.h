#ifndef SOPNET_RANDOM_FOREST_TRAINER_H__
#define SOPNET_RANDOM_FOREST_TRAINER_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include <inference/LinearConstraints.h>
#include <inference/RandomForest.h>
#include <sopnet/segments/Segments.h>
#include "SegmentRandomForestTrainer.h"

// forward declarations
class GoldStandardExtractor;
class SegmentFeaturesExtractor;

/**
 * Sets up a pipeline to train a random forest classifier given some
 * ground-truth segments.
 *
 * Positive samples can be the ground-truth samples itself, or the so-called
 * "gold-standard": extracted segment hypotheses that are the closest to the
 * ground-truth. The type of positive samples to use can be selected by the
 * program option useGroundTruth.
 *
 * Negative samples are found by picking extracted segmentation hypotheses that
 * are in conflict with the gold-standard.
 */
class RandomForestTrainer : public pipeline::ProcessNode {

public:

	RandomForestTrainer();

private:

	void onGroundTruthSet(const pipeline::InputSet<Segments>& signal);

	void onSegmentsSet(const pipeline::InputSet<Segments>& signal);

	void onRawSectionsSet(const pipeline::InputSet<ImageStack>& signal);

	pipeline::Input<Segments> _groundTruth;

	pipeline::Input<Segments> _allSegments;

	pipeline::Input<ImageStack> _rawSections;

	pipeline::Output<RandomForest> _randomForest;

	// finds the closest segements to the given ground truth
	boost::shared_ptr<GoldStandardExtractor> _goldStandardExtractor;

	// extracts the features for all extracted segments
	boost::shared_ptr<SegmentFeaturesExtractor> _segmentFeaturesExtractor;

	// extracts the features for the ground-truth
	boost::shared_ptr<SegmentFeaturesExtractor> _gtFeaturesExtractor;

	// trains a random forest give positve and negative samples
	boost::shared_ptr<SegmentRandomForestTrainer> _rfTrainer;
};

#endif // SOPNET_RANDOM_FOREST_TRAINER_H__

