#ifndef CELLTRACKER_TRACKLET_EVALUATOR_H__
#define CELLTRACKER_TRACKLET_EVALUATOR_H__

#include <pipeline/all.h>
#include "Segment.h"
#include "SegmentCostFunctionParameters.h"

// forward declarations
class EndSegment;
class ContinuationSegment;
class BranchSegment;

class SegmentEvaluator : public pipeline::SimpleProcessNode {

public:

	SegmentEvaluator();

private:


	void updateOutputs();

	double getCosts(const EndSegment& end);

	double getCosts(const ContinuationSegment& continuation);

	double getCosts(const BranchSegment& branch);

	double costs(const Segment& segment, boost::shared_ptr<SegmentCostFunctionParameters> parameters);

	pipeline::Input<SegmentCostFunctionParameters> _parameters;

	pipeline::Output<boost::function<double(const Segment&)> > _costFunction;
};

#endif // CELLTRACKER_TRACKLET_EVALUATOR_H__

