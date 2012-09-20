#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include <lemon/lgf_writer.h>

#include "LemonGraphWriter.h"

static logger::LogChannel lemongraphwriterlog("lemongraphwriterlog", "[LemonGraphWriter] ");

util::ProgramOption optionLemonGraphFile(
		util::_module           = "sopnet",
		util::_long_name        = "lemonGraphFile",
		util::_description_text = "Path to the lemon graph file to produce.",
		util::_default_value    = "hypotheses_graph.lem");

util::ProgramOption optionCostsFile(
		util::_module           = "sopnet",
		util::_long_name        = "costsFile",
		util::_description_text = "Path to the costs file to produce.",
		util::_default_value    = "costs.ar");

LemonGraphWriter::LemonGraphWriter() :
	_segmentsDirty(true),
	_linearConstraintsDirty(true),
	_segmentIdsToVariablesDirty(true),
	_updatingInputs(false) {

	registerInput(_segments, "segments");
	registerInput(_linearConstraints, "linear constraints");
	registerInput(_segmentIdsToVariables, "segment ids map");
	registerInput(_sliceCostFunction, "slice cost function");
	registerInputs(_segmentCostFunctions, "segment cost functions");

	_segments.registerBackwardCallback(&LemonGraphWriter::onSegmentsModified, this);
	_linearConstraints.registerBackwardCallback(&LemonGraphWriter::onConstraintsModified, this);
	_segmentIdsToVariables.registerBackwardCallback(&LemonGraphWriter::onIdMapModified, this);

	_segments.registerBackwardCallback(&LemonGraphWriter::onSegmentsUpdated, this);
	_linearConstraints.registerBackwardCallback(&LemonGraphWriter::onConstraintsUpdated, this);
	_segmentIdsToVariables.registerBackwardCallback(&LemonGraphWriter::onIdMapUpdated, this);
}

void
LemonGraphWriter::update() {

	if (_updatingInputs) {

		LOG_DEBUG(lemongraphwriterlog) << "update is already called" << std::endl;
		return;
	}

	if (_segmentsDirty || _linearConstraintsDirty || _segmentIdsToVariablesDirty) {

		LOG_DEBUG(lemongraphwriterlog) << "one of the inputs is still dirty -- skip dumping" << std::endl;
		return;
	}

	_updatingInputs = true;

	updateInputs();

	_updatingInputs = false;

	LOG_DEBUG(lemongraphwriterlog) << "dumping lemon graph..." << std::endl;

	Tracking::HypothesesGraph graph;

	_slicesToNodes.clear();

	unsigned int numNodes = 0;

	// add all slices to the graph
	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds())
		if (end->getDirection() == Left) {

			Tracking::HypothesesGraph::Node node = graph.add_node(end->getInterSectionInterval());

			_slicesToNodes[end->getSlice()->getId()] = node;

			numNodes++;
		}

	// establish all links
	foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations()) {

		assert(_slicesToNodes.count(continuation->getSourceSlice()->getId()));
		assert(_slicesToNodes.count(continuation->getTargetSlice()->getId()));

		Tracking::HypothesesGraph::Node node1 = _slicesToNodes[continuation->getSourceSlice()->getId()];
		Tracking::HypothesesGraph::Node node2 = _slicesToNodes[continuation->getTargetSlice()->getId()];

		if (continuation->getDirection() == Left)
			std::swap(node1, node2);

		graph.addArc(node1, node2);
	}
	foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches()) {

		assert(_slicesToNodes.count(branch->getSourceSlice()->getId()));
		assert(_slicesToNodes.count(branch->getTargetSlice1()->getId()));
		assert(_slicesToNodes.count(branch->getTargetSlice2()->getId()));

		Tracking::HypothesesGraph::Node node1  = _slicesToNodes[branch->getSourceSlice()->getId()];
		Tracking::HypothesesGraph::Node node2a = _slicesToNodes[branch->getTargetSlice1()->getId()];
		Tracking::HypothesesGraph::Node node2b = _slicesToNodes[branch->getTargetSlice2()->getId()];

		if (branch->getDirection() == Left) {

			if (!connected(graph, node2a, node1))
				graph.addArc(node2a, node1);
			if (!connected(graph, node2b, node1))
				graph.addArc(node2b, node1);

		} else {

			if (!connected(graph, node1, node2a))
				graph.addArc(node1, node2a);
			if (!connected(graph, node1, node2b))
				graph.addArc(node1, node2b);
		}
	}

	std::ofstream out(optionLemonGraphFile.as<std::string>().c_str());

	lemon::DigraphWriter<Tracking::HypothesesGraph>(graph, out).
			nodeMap("node_timestep", graph.get(Tracking::node_timestep())).
			run();

	LOG_DEBUG(lemongraphwriterlog) << "done" << std::endl;

	LOG_DEBUG(lemongraphwriterlog) << "dumping slice and segment costs" << std::endl;

	// create slice cost vector

	std::map<int, double> sliceCosts;

	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds())
		if (end->getDirection() == Left) {

			double sliceCost = (*_sliceCostFunction)(*end->getSlice());

			sliceCosts[nodeId(graph, *end->getSlice())] = sliceCost;
		}

	// create a vector of all segment costs

	std::vector<double> allCosts(_segments->size(), 0);

	// accumulate costs
	for (int i = 0; i < _segmentCostFunctions.size(); i++) {

		costs_function_type& costFunction = *_segmentCostFunctions[i];
		costFunction(_segments->getEnds(), _segments->getContinuations(), _segments->getBranches(), allCosts);
	}

	// create end cost vectors

	std::map<int, double> endCostsLeft;
	std::map<int, double> endCostsRight;

	unsigned int i = 0;
	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds()) {

		if (end->getDirection() == Left)
			endCostsLeft[nodeId(graph, *end->getSlice())] = allCosts[i];
		else
			endCostsRight[nodeId(graph, *end->getSlice())] = allCosts[i];

		i++;
	}

	// create continuation cost vector

	std::map<int, std::map<int, double> > continuationCosts;

	foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations()) {

		if (continuation->getDirection() == Left)
			continuationCosts[nodeId(graph, *continuation->getTargetSlice())][nodeId(graph, *continuation->getSourceSlice())] = allCosts[i];
		else
			continuationCosts[nodeId(graph, *continuation->getSourceSlice())][nodeId(graph, *continuation->getTargetSlice())] = allCosts[i];

		i++;
	}

	// create branch cost vectors

	std::map<int, std::map<std::pair<int, int>, double> > branchCostsLeft;
	std::map<int, std::map<std::pair<int, int>, double> > branchCostsRight;

	foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches()) {

		if (branch->getDirection() == Left)
			branchCostsLeft[nodeId(graph, *branch->getSourceSlice())][std::make_pair(nodeId(graph, *branch->getTargetSlice1()), nodeId(graph, *branch->getTargetSlice2()))] = allCosts[i];
		else
			branchCostsRight[nodeId(graph, *branch->getSourceSlice())][std::make_pair(nodeId(graph, *branch->getTargetSlice1()), nodeId(graph, *branch->getTargetSlice2()))] = allCosts[i];

		i++;
	}

	// create archive

	std::ofstream costsOut(optionCostsFile.as<std::string>().c_str());

	boost::archive::text_oarchive archive(costsOut);

	// write to archive

	archive << sliceCosts;
	archive << endCostsLeft;
	archive << endCostsRight;
	archive << continuationCosts;
	archive << branchCostsLeft;
	archive << branchCostsRight;

	LOG_DEBUG(lemongraphwriterlog) << "done" << std::endl;
}

bool
LemonGraphWriter::connected(
		const Tracking::HypothesesGraph& graph,
		const Tracking::HypothesesGraph::Node& node1,
		const Tracking::HypothesesGraph::Node& node2) {

	for (Tracking::HypothesesGraph::OutArcIt i(graph, node1); i != lemon::INVALID; ++i)
		if (graph.runningNode(i) == node2)
			return true;

	return false;
}

int
LemonGraphWriter::nodeId(const Tracking::HypothesesGraph& graph, const Slice& slice) {

	return graph.id(_slicesToNodes[slice.getId()]);
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
