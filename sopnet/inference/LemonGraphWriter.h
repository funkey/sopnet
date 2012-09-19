#ifndef SOPNET_INFERENCE_LEMON_GRAPH_WRITER_H__
#define SOPNET_INFERENCE_LEMON_GRAPH_WRITER_H__

#include <pipeline/all.h>

#include <inference/LinearConstraints.h>
#include <sopnet/segments/Segments.h>

/**
 * A sink process node that dumps a problem (i.e., sets of Segments and
 * LinearConstraints) as a lemon graph structure.
 */
class LemonGraphWriter : public pipeline::SimpleProcessNode {

public:

	LemonGraphWriter();

private:

	void update();

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

	// all linear constraints on the segments
	pipeline::Input<LinearConstraints> _linearConstraints;

	// map from segment ids to variable numbers in the linear constraints
	pipeline::Input<std::map<unsigned int, unsigned int> > _segmentIdsToVariables;

	bool _segmentsDirty;
	bool _linearConstraintsDirty;
	bool _segmentIdsToVariablesDirty;
};

#endif // SOPNET_INFERENCE_LEMON_GRAPH_WRITER_H__

