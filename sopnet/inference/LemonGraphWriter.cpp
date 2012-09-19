#include <lemon/lgf_writer.h>

#include <external/embryonic/hypotheses.h>
#include "LemonGraphWriter.h"

static logger::LogChannel lemongraphwriterlog("lemongraphwriterlog", "[LemonGraphWriter] ");

LemonGraphWriter::LemonGraphWriter() :
	_segmentsDirty(true),
	_linearConstraintsDirty(true),
	_segmentIdsToVariablesDirty(true) {

	registerInput(_segments, "segments");
	registerInput(_linearConstraints, "linear constraints");
	registerInput(_segmentIdsToVariables, "segment ids map");

	_segments.registerBackwardCallback(&LemonGraphWriter::onSegmentsModified, this);
	_linearConstraints.registerBackwardCallback(&LemonGraphWriter::onConstraintsModified, this);
	_segmentIdsToVariables.registerBackwardCallback(&LemonGraphWriter::onIdMapModified, this);

	_segments.registerBackwardCallback(&LemonGraphWriter::onSegmentsUpdated, this);
	_linearConstraints.registerBackwardCallback(&LemonGraphWriter::onConstraintsUpdated, this);
	_segmentIdsToVariables.registerBackwardCallback(&LemonGraphWriter::onIdMapUpdated, this);
}

void
LemonGraphWriter::update() {

	if (_segmentsDirty || _linearConstraintsDirty || _segmentIdsToVariablesDirty) {

		LOG_DEBUG(lemongraphwriterlog) << "one of the inputs is still dirty -- skip dumping" << std::endl;
		return;
	}

	LOG_DEBUG(lemongraphwriterlog) << "dumping lemon graph..." << std::endl;

	Tracking::HypothesesGraph graph;

	Tracking::HypothesesGraph::Node node1 = graph.add_node(0);
	Tracking::HypothesesGraph::Node node2 = graph.add_node(0);

	graph.addArc(node1, node2);

	lemon::DigraphWriter<Tracking::HypothesesGraph>(graph, lemongraphwriterlog(logger::Debug)).
			nodeMap("node_timestep", graph.get(Tracking::node_timestep())).
			run();

	LOG_DEBUG(lemongraphwriterlog) << "done" << std::endl;
}

void
LemonGraphWriter::onSegmentsModified(const pipeline::Modified& signal) {

	_segmentsDirty = true;
}

void
LemonGraphWriter::onConstraintsModified(const pipeline::Modified& signal) {

	_linearConstraintsDirty = true;
}

void
LemonGraphWriter::onIdMapModified(const pipeline::Modified& signal) {

	_segmentIdsToVariablesDirty = true;
}

void
LemonGraphWriter::onSegmentsUpdated(const pipeline::Updated& signal) {

	LOG_DEBUG(lemongraphwriterlog) << "segments updated" << std::endl;
	_segmentsDirty = false;

	update();
}

void
LemonGraphWriter::onConstraintsUpdated(const pipeline::Updated& signal) {

	LOG_DEBUG(lemongraphwriterlog) << "constraints updated" << std::endl;
	_linearConstraintsDirty = false;

	update();
}

void
LemonGraphWriter::onIdMapUpdated(const pipeline::Updated& signal) {

	LOG_DEBUG(lemongraphwriterlog) << "id map updated" << std::endl;
	_segmentIdsToVariablesDirty = false;

	update();
}
