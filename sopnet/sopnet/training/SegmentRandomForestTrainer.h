#ifndef SOPNET_SEGMENT_RANDOM_FOREST_TRAINER_H__
#define SOPNET_SEGMENT_RANDOM_FOREST_TRAINER_H__

#include <pipeline/all.h>
#include <inference/RandomForest.h>
#include <sopnet/features/Features.h>
#include <sopnet/segments/Segments.h>

/**
 * Trains a Random Forest on positive and negative samples of segments.
 */
class SegmentRandomForestTrainer : public pipeline::SimpleProcessNode<> {

public:

	SegmentRandomForestTrainer();

private:

	void updateOutputs();

	// the positive segments
	pipeline::Input<Segments> _positiveSamples;

	// the negative segments
	pipeline::Input<Segments> _negativeSamples;

	// the features of the segments
	pipeline::Input<Features> _features;

	// the learnt random forest classifier
	pipeline::Output<RandomForest> _randomForest;
};

#endif // SOPNET_SEGMENT_RANDOM_FOREST_TRAINER_H__

