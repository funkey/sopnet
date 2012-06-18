#include <util/foreach.h>
#include <util/Logger.h>
#include "ComponentTreeDownSampler.h"

static logger::LogChannel componenttreedownsamplerlog("componenttreedownsamplerlog", "[ComponentTreeDownSampler] ");

ComponentTreeDownSampler::ComponentTreeDownSampler() {

	registerInput(_componentTree, "component tree");
	registerOutput(_downsampled,  "component tree");
}

void
ComponentTreeDownSampler::updateOutputs() {

	downsample();
}

void
ComponentTreeDownSampler::downsample() {

	// copy and downsample on-the-fly, starting with the root node
	_downsampled->setRoot(downsample(_componentTree->getRoot()));
}

boost::shared_ptr<ComponentTree::Node>
ComponentTreeDownSampler::downsample(boost::shared_ptr<ComponentTree::Node> node) {

	// create a clone of the node
	boost::shared_ptr<ComponentTree::Node> nodeClone = boost::make_shared<ComponentTree::Node>(node->getComponent());

	// skip over all single children
	while (node->getChildren().size() == 1)
		node = node->getChildren().front();

	// downsample the trees under every child and add them to the clone
	foreach (boost::shared_ptr<ComponentTree::Node> child, node->getChildren()) {

		boost::shared_ptr<ComponentTree::Node> childClone = downsample(child);

		nodeClone->addChild(childClone);
	}

	// return the clone
	return nodeClone;
}
