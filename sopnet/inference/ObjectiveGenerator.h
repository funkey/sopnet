#ifndef CELLTRACKER_OBJECTIVE_GENERATOR_H__
#define CELLTRACKER_OBJECTIVE_GENERATOR_H__

#include <pipeline/all.h>
#include <inference/LinearObjective.h>
#include <sopnet/segments/Segments.h>

class ObjectiveGenerator : public pipeline::SimpleProcessNode<> {

	typedef boost::function<
			void
			(const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			 const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			 const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			 std::vector<double>& costs)>
			costs_function_type;

public:

	ObjectiveGenerator();

private:

	void updateOutputs();

	void updateObjective();

	template <typename SegmentType>
	void setCosts(const SegmentType& segment);

	pipeline::Input<Segments> _segments;

	pipeline::Inputs<costs_function_type> _costFunctions;

	pipeline::Output<LinearObjective> _objective;
};

#endif // CELLTRACKER_OBJECTIVE_GENERATOR_H__

