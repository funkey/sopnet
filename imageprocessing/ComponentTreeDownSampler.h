#ifndef IMAGEPROCESSING_COMPONENT_TREE_DOWN_SAMPLER_H__
#define IMAGEPROCESSING_COMPONENT_TREE_DOWN_SAMPLER_H__

#include <pipeline/all.h>
#include <imageprocessing/ComponentTree.h>

class ComponentTreeDownSampler : public pipeline::SimpleProcessNode<> {

public:

	ComponentTreeDownSampler();

private:

	void updateOutputs();

	void downsample();

	boost::shared_ptr<ComponentTree::Node> downsample(boost::shared_ptr<ComponentTree::Node> node);

	pipeline::Input<ComponentTree>  _componentTree;
	pipeline::Output<ComponentTree> _downsampled;
};

#endif // IMAGEPROCESSING_COMPONENT_TREE_DOWN_SAMPLER_H__

