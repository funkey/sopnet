#ifndef CELLTRACKER_TRACKLET_EVALUATOR_H__
#define CELLTRACKER_TRACKLET_EVALUATOR_H__

#include <pipeline/all.h>
#include <sopnet/segments/Segment.h>
#include "LinearCostFunctionParameters.h"

// forward declarations
class EndSegment;
class ContinuationSegment;
class BranchSegment;

class LinearCostFunction : public pipeline::SimpleProcessNode<> {

public:

	LinearCostFunction();

private:


	void updateOutputs();

	double getCosts(const EndSegment& end);

	double getCosts(const ContinuationSegment& continuation);

	double getCosts(const BranchSegment& branch);

	double costs(const Segment& segment, boost::shared_ptr<LinearCostFunctionParameters> parameters);

	pipeline::Input<LinearCostFunctionParameters> _parameters;

	pipeline::Output<boost::function<double(const Segment&)> > _costFunction;
};

#endif // CELLTRACKER_TRACKLET_EVALUATOR_H__

