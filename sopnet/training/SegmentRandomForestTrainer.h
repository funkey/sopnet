#ifndef SOPNET_SEGMENT_RANDOM_FOREST_TRAINER_H__
#define SOPNET_SEGMENT_RANDOM_FOREST_TRAINER_H__

#include <pipeline/all.h>
#include <inference/RandomForest.h>
#include <sopnet/features/Features.h>
#include <sopnet/segments/Segments.h>

class SegmentRandomForestTrainer : public pipeline::SimpleProcessNode {

public:

	SegmentRandomForestTrainer();

private:

	void updateOutputs();

	pipeline::Input<Segments> _positiveSamples;

	pipeline::Input<Segments> _negativeSamples;

	pipeline::Input<Features> _features;

	pipeline::Output<RandomForest> _randomForest;
};

#endif // SOPNET_SEGMENT_RANDOM_FOREST_TRAINER_H__

