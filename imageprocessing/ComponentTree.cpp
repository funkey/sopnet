#include "ComponentTree.h"

ComponentTree::Node::Node(boost::shared_ptr<ConnectedComponent> component) :
	_component(component) {}

void
ComponentTree::Node::setParent(boost::shared_ptr<ComponentTree::Node> parent) {

	_parent = parent;
}

boost::shared_ptr<ComponentTree::Node>
ComponentTree::Node::getParent() {

	boost::shared_ptr<ComponentTree::Node> parent = _parent.lock();

	return parent;
}

void
ComponentTree::Node::addChild(boost::shared_ptr<ComponentTree::Node> componentNode) {

	_children.push_back(componentNode);
}

std::vector<boost::shared_ptr<ComponentTree::Node> >
ComponentTree::Node::getChildren() {

	return _children;
}

boost::shared_ptr<ConnectedComponent>
ComponentTree::Node::getComponent() {

	return _component;
}

ComponentTree::ComponentTree() :
	_boundingBox(0, 0, 0, 0) {}

void
ComponentTree::clear() {

	_root.reset();
	_boundingBox = util::rect<double>(0, 0, 0, 0);
}

void
ComponentTree::setRoot(boost::shared_ptr<ComponentTree::Node> root) {

	_root = root;
	updateBoundingBox();
}

boost::shared_ptr<ComponentTree::Node>
ComponentTree::getRoot() {

	return _root;
}

unsigned int
ComponentTree::size() const {

	return count(_root);
}

unsigned int
ComponentTree::count(boost::shared_ptr<ComponentTree::Node> node) const {

	int numNodes = 0;

	foreach (boost::shared_ptr<ComponentTree::Node> child, node->getChildren())
		numNodes += count(child);

	numNodes++;

	return numNodes;
}

const util::rect<double>&
ComponentTree::getBoundingBox() const {

	return _boundingBox;
}

/**
 * Creates a copy of the component tree, but not a copy of the involved
 * connected components.
 *
 * @return A copy of the component tree.
 */
ComponentTree
ComponentTree::clone() {

	boost::shared_ptr<ComponentTree::Node> root = clone(_root);

	ComponentTree tree;

	tree.setRoot(_root);

	return tree;
}

boost::shared_ptr<ComponentTree::Node>
ComponentTree::clone(boost::shared_ptr<ComponentTree::Node> node) {

	boost::shared_ptr<ComponentTree::Node> nodeClone = boost::make_shared<ComponentTree::Node>(node->getComponent());

	foreach (boost::shared_ptr<ComponentTree::Node> child, node->getChildren()) {

		boost::shared_ptr<ComponentTree::Node> childClone = clone(child);

		nodeClone->addChild(childClone);
	}

	return nodeClone;
}

void
ComponentTree::updateBoundingBox() {

	_boundingBox = updateBoundingBox(_root);
}

util::rect<double>
ComponentTree::updateBoundingBox(boost::shared_ptr<ComponentTree::Node> node) {

	util::rect<double> boundingBox = node->getComponent()->getBoundingBox();

	foreach (boost::shared_ptr<ComponentTree::Node> child, node->getChildren()) {

		util::rect<double> childBoundingBox = updateBoundingBox(child);

		boundingBox.minX = std::min(boundingBox.minX, childBoundingBox.minX);
		boundingBox.maxX = std::max(boundingBox.maxX, childBoundingBox.maxX);
		boundingBox.minY = std::min(boundingBox.minY, childBoundingBox.minY);
		boundingBox.maxY = std::max(boundingBox.maxY, childBoundingBox.maxY);
	}

	return boundingBox;
}

