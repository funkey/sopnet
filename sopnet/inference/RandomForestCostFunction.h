#ifndef SOPNET_SEGMENT_RANDOM_FOREST_EVALUATOR_H__
#define SOPNET_SEGMENT_RANDOM_FOREST_EVALUATOR_H__

#include <pipeline/all.h>
#include <inference/RandomForest.h>
#include <sopnet/features/Features.h>
#include <sopnet/segments/Segment.h>

class RandomForestCostFunction : public pipeline::SimpleProcessNode<> {

	typedef boost::function<
			void
			(const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			 const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			 const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			 std::vector<double>& costs)>
			costs_function_type;

public:

	RandomForestCostFunction();

private:

	void updateOutputs();

	void costs(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			std::vector<double>& costs);

	double costs(const Segment& segment);

	pipeline::Input<Features> _features;

	pipeline::Input<RandomForest> _randomForest;

	pipeline::Output<costs_function_type> _costFunction;

	std::vector<double> _cache;
};

#endif // SOPNET_SEGMENT_RANDOM_FOREST_EVALUATOR_H__

