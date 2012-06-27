#ifndef SOPNET_SEGMENT_RANDOM_FOREST_EVALUATOR_H__
#define SOPNET_SEGMENT_RANDOM_FOREST_EVALUATOR_H__

#include <pipeline/all.h>
#include <inference/RandomForest.h>
#include <sopnet/features/Features.h>
#include <sopnet/segments/Segment.h>

class RandomForestCostFunction : public pipeline::SimpleProcessNode {

public:

	RandomForestCostFunction();

private:

	void updateOutputs();

	double costs(const Segment& segment);

	pipeline::Input<Features> _features;

	pipeline::Input<RandomForest> _randomForest;

	pipeline::Output<boost::function<double(const Segment&)> > _costFunction;
};

#endif // SOPNET_SEGMENT_RANDOM_FOREST_EVALUATOR_H__

