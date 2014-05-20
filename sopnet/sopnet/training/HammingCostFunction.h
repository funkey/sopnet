#ifndef SOPNET_TRAINING_HAMMING_COST_FUNCTION_H__
#define SOPNET_TRAINING_HAMMING_COST_FUNCTION_H__

#include <pipeline/SimpleProcessNode.h>
#include <sopnet/segments/Segments.h>

// forward declarations
class EndSegment;
class ContinuationSegment;
class BranchSegment;

class HammingCostFunction : public pipeline::SimpleProcessNode<> {

	typedef boost::function<
			void
			(const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			 const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			 const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			 std::vector<double>& costs)>
			costs_function_type;

public:

	HammingCostFunction();

private:

	void updateOutputs();

	void costs(
			const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			std::vector<double>& costs);

	//double costs(boost::shared_ptr<Segment> segment);
	double costs(boost::shared_ptr<EndSegment> end);
	double costs(boost::shared_ptr<ContinuationSegment> continuation);
	double costs(boost::shared_ptr<BranchSegment> branch);
	
	pipeline::Input<Segments> _goldStandard;
	
	pipeline::Output<costs_function_type> _costFunction;

};

#endif // SOPNET_TRAINING_HAMMING_COST_FUNCTION_H__

