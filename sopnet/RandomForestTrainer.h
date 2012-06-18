#ifndef SOPNET_RANDOM_FOREST_TRAINER_H__
#define SOPNET_RANDOM_FOREST_TRAINER_H__

#include <pipeline/all.h>
#include <inference/RandomForest.h>
#include "Features.h"
#include "Segments.h"

class RandomForestTrainer : public pipeline::SimpleProcessNode {

public:

	RandomForestTrainer();

private:

	void updateOutputs();

	pipeline::Input<Segments> _positiveSamples;

	pipeline::Input<Segments> _negativeSamples;

	pipeline::Input<Features> _features;

	pipeline::Output<RandomForest> _randomForest;
};

#endif // SOPNET_RANDOM_FOREST_TRAINER_H__

