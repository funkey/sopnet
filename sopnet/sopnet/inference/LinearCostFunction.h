#ifndef CELLTRACKER_TRACKLET_EVALUATOR_H__
#define CELLTRACKER_TRACKLET_EVALUATOR_H__

#include <pipeline/all.h>
#include <sopnet/features/Features.h>
#include <sopnet/segments/Segment.h>
#include "LinearCostFunctionParameters.h"

// forward declarations
class EndSegment;
class ContinuationSegment;
class BranchSegment;

class LinearCostFunction : public pipeline::SimpleProcessNode<> {

	typedef boost::function<
			void
			(const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			 const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			 const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			 std::vector<double>& costs)>
			costs_function_type;

public:

	LinearCostFunction();

private:


	void updateOutputs();

	void costs(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			std::vector<double>& costs);

	double costs(const Segment& segment, const std::vector<double>& weights);

	pipeline::Input<Features> _features;

	pipeline::Input<LinearCostFunctionParameters> _parameters;

	pipeline::Output<costs_function_type> _costFunction;

	std::vector<double> _cache;
};

#endif // CELLTRACKER_TRACKLET_EVALUATOR_H__

