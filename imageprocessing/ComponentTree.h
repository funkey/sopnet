#ifndef IMAGEPROCESSING_COMPONENT_TREE_H__
#define IMAGEPROCESSING_COMPONENT_TREE_H__

#include <imageprocessing/ConnectedComponent.h>
#include <util/foreach.h>
#include <util/Logger.h>
#include <pipeline/all.h>

static logger::LogChannel componenttreelog("componenttreelog", "[ComponentTree] ");

class ComponentTree : public pipeline::Data {

public:

	/**
	 * A node in the component tree. Stores the connected component and pointers
	 * to the children and parent node(s). Pointers to the connected component
	 * and children are shared pointers, pointers to the parent weak pointers.
	 * It is thereby enough to keep the root of the tree alive to ensure that
	 * every connected component in the tree is still available.
	 */
	class Node {

	public:

		/**
		 * Create a new node from a connected component.
		 *
		 * @param component The connected component that is to be represented by
		 *                  this node.
		 */
		Node(boost::shared_ptr<ConnectedComponent> component);

		/**
		 * Set the parent of this node.
		 *
		 * @param A shared pointer to the new parent.
		 */
		void setParent(boost::shared_ptr<Node> parent);

		/**
		 * Get the parent of this node.
		 *
		 * @return The parent of this node.
		 */
		boost::shared_ptr<Node> getParent();

		/**
		 * Add a child to this node.
		 *
		 * @param A shared pointer to the new child.
		 */
		void addChild(boost::shared_ptr<Node> componentNode);

		/**
		 * Get all children of this node.
		 *
		 * @return A vector of shared pointers to the children of this node.
		 */
		std::vector<boost::shared_ptr<Node> > getChildren();

		/**
		 * Get the connected component represented by this node.
		 *
		 * @return The connected component help by this node.
		 */
		boost::shared_ptr<ConnectedComponent> getComponent();

	private:

		boost::shared_ptr<ConnectedComponent> _component;

		boost::weak_ptr<Node> _parent;

		std::vector<boost::shared_ptr<Node> > _children;
	};

	/**
	 * Base class for visitors. This class is just provided for convenience. If
	 * you derive from this class, you don't have to implement callbacks, that
	 * you are not interested in.
	 */
	class Visitor {

	public:

		void visitNode(boost::shared_ptr<Node>) {};
		void visitEdge(boost::shared_ptr<Node>, boost::shared_ptr<Node>) {};
		void leaveNode(boost::shared_ptr<Node>) {};
		void leaveEdge(boost::shared_ptr<Node>, boost::shared_ptr<Node>) {};
	};

	/**
	 * Default constructor.
	 */
	ComponentTree();

	/**
	 * Remove all nodes from the tree.
	 */
	void clear();

	/**
	 * Replace or set the root node of this tree.
	 *
	 * @param root The new root node.
	 */
	void setRoot(boost::shared_ptr<Node> root);

	/**
	 * Get the root node of the tree.
	 *
	 * @return The root node.
	 */
	boost::shared_ptr<Node> getRoot();

	/**
	 * Get the number of components in this tree.
	 *
	 * @return the number of components.
	 */
	unsigned int size() const;

	/**
	 * Visit each node and edge in the tree. This method performs a
	 * depth-first-search on the tree. The argument type has to model Visitor
	 * (see the default implementation above).
	 *
	 * @param visitor A callback class for visiting nodes and edges.
	 */
	template <class Visitor>
	void visit(Visitor& visitor) {

		visit(_root, visitor);
	}

	/**
	 * Visit each node below (and including) the given node. This method
	 * performs a depth-first-search on the tree. The argument type has to
	 * model Visitor (see the default implementation above).
	 *
	 * @param node The starting node.
	 * @param visitor A callback class for visiting nodes and edges.
	 */
	template <class Visitor>
	void visit(boost::shared_ptr<Node> node, Visitor& visitor) {

		visitor.visitNode(node);

		foreach (boost::shared_ptr<Node> child, node->getChildren()) {

			visitor.visitEdge(node, child);

			visit(child, visitor);

			visitor.leaveEdge(node, child);
		}

		visitor.leaveNode(node);
	}

	/**
	 * Get the bounding box of all components in the component tree.
	 *
	 * @return A rectangle encapsulating all components in the tree.
	 */
	const util::rect<double>& getBoundingBox() const;

	/**
	 * Creates a copy of the component tree, but not a copy of the involved
	 * connected components.
	 *
	 * @return A copy of the component tree.
	 */
	ComponentTree clone();

private:

	unsigned int count(boost::shared_ptr<ComponentTree::Node> node) const;

	boost::shared_ptr<Node> clone(boost::shared_ptr<Node> node);

	void updateBoundingBox();

	util::rect<double> updateBoundingBox(boost::shared_ptr<Node> node);

	boost::shared_ptr<Node> _root;

	util::rect<double> _boundingBox;
};

#endif // IMAGEPROCESSING_COMPONENT_TREE_H__

