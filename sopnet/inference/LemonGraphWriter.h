#ifndef SOPNET_INFERENCE_LEMON_GRAPH_WRITER_H__
#define SOPNET_INFERENCE_LEMON_GRAPH_WRITER_H__

#include <pipeline/all.h>

#include <external/embryonic/hypotheses.h>

#include <inference/LinearConstraints.h>
#include <sopnet/segments/Segments.h>

/**
 * A sink process node that dumps a problem (i.e., sets of Segments and
 * LinearConstraints) as a lemon graph structure.
 */
class LemonGraphWriter : public pipeline::SimpleProcessNode {

	typedef boost::function<
			void
			(const std::vector<boost::shared_ptr<EndSegment> >&          ends,
			 const std::vector<boost::shared_ptr<ContinuationSegment> >& continuations,
			 const std::vector<boost::shared_ptr<BranchSegment> >&       branches,
			 std::vector<double>& costs)>
			costs_function_type;

	typedef boost::function<double (const Slice&)> slice_cost_function_type;

public:

	LemonGraphWriter();

private:

	void update();

	// check whether two nodes are connected in the graph
	bool connected(
		const Tracking::HypothesesGraph& graph,
		const Tracking::HypothesesGraph::Node& node1,
		const Tracking::HypothesesGraph::Node& node2);

	int nodeId(const Tracking::HypothesesGraph& graph, const Slice& slice);

	void onSegmentsModified(const pipeline::Modified& signal);

	void onConstraintsModified(const pipeline::Modified& signal);

	void onIdMapModified(const pipeline::Modified& signal);

	void onSegmentsUpdated(const pipeline::Updated& signal);

	void onConstraintsUpdated(const pipeline::Updated& signal);

	void onIdMapUpdated(const pipeline::Updated& signal);

	// we produce nothing
	void updateOutputs() {};

	// all extracted segments
	pipeline::Input<Segments> _segments;

	// map from segment ids to variable numbers in the linear constraints
	pipeline::Input<std::map<unsigned int, unsigned int> > _segmentIdsToVariables;

	// a cost function for slices
	pipeline::Input<slice_cost_function_type> _sliceCostFunction;

	// all linear constraints on the slices (one set per section)
	pipeline::Inputs<LinearConstraints> _linearConstraints;

	// cost functions for segments
	pipeline::Inputs<costs_function_type> _segmentCostFunctions;

	bool _updatingInputs;

	bool _segmentsDirty;
	bool _linearConstraintsDirty;
	bool _segmentIdsToVariablesDirty;

	std::map<unsigned int, Tracking::HypothesesGraph::Node> _slicesToNodes;
};

#endif // SOPNET_INFERENCE_LEMON_GRAPH_WRITER_H__

