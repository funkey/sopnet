#ifndef CELLTRACKER_TRACKLET_EVALUATOR_H__
#define CELLTRACKER_TRACKLET_EVALUATOR_H__

#include <pipeline/all.h>
#include "Segment.h"
#include "SegmentVisitor.h"
#include "SegmentCostFunctionParameters.h"

class SegmentEvaluator : public pipeline::SimpleProcessNode {

public:

	SegmentEvaluator();

private:

	class CostVisitor : public SegmentVisitor {

	public:

		void visit(const EndSegment& end);

		void visit(const ContinuationSegment& continuation);

		void visit(const BranchSegment& branch);

		double getCosts();

	private:

		double _costs;
	};

	void updateOutputs();

	double costs(const Segment& segment, boost::shared_ptr<SegmentCostFunctionParameters> parameters);

	pipeline::Input<SegmentCostFunctionParameters> _parameters;

	pipeline::Output<boost::function<double(const Segment&)> > _costFunction;
};

#endif // CELLTRACKER_TRACKLET_EVALUATOR_H__

