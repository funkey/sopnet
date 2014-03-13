#ifndef SOPNET_TRAINING_GOLD_STANDARD_COST_FUNCTION_H__
#define SOPNET_TRAINING_GOLD_STANDARD_COST_FUNCTION_H__

#include <pipeline/SimpleProcessNode.h>
#include <sopnet/segments/Segments.h>

// forward declarations
class EndSegment;
class ContinuationSegment;
class BranchSegment;

class GoldStandardCostFunction : public pipeline::SimpleProcessNode<> {

	typedef boost::function<
			void
			(const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			 const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			 const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			 std::vector<double>& costs)>
			costs_function_type;

public:

	GoldStandardCostFunction();

private:


	void updateOutputs();

	void costs(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			std::vector<double>& costs);

	pipeline::Input<Segments> _groundTruth;

	pipeline::Output<costs_function_type> _costFunction;
};

#endif // SOPNET_TRAINING_GOLD_STANDARD_COST_FUNCTION_H__

