#ifndef GUI_COMPONENT_TREE_PAINTER_H__
#define GUI_COMPONENT_TREE_PAINTER_H__

#include <boost/shared_ptr.hpp>

#include <gui/RecordablePainter.h>
#include <imageprocessing/ComponentTree.h>

class ComponentTreePainter : public gui::RecordablePainter {

public:

	void setComponentTree(boost::shared_ptr<ComponentTree> componentTree);

private:

	class ComponentPaintVisitor : public ComponentTree::Visitor {

	public:

		ComponentPaintVisitor();

		void visitNode(boost::shared_ptr<ComponentTree::Node> node) const;

		void visitEdge(boost::shared_ptr<ComponentTree::Node> parent, boost::shared_ptr<ComponentTree::Node> child) const;

	private:

		// the z-scale
		double _zScale;
	};

	void updateRecording();

	boost::shared_ptr<ComponentTree> _componentTree;

	util::rect<double>               _size;
};

#endif // GUI_COMPONENT_TREE_PAINTER_H__

