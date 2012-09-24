#ifndef GUI_COMPONENT_TREE_VIEW_H__
#define GUI_COMPONENT_TREE_VIEW_H__

#include <gui/Signals.h>
#include <imageprocessing/ComponentTree.h>
#include <pipeline/all.h>
#include "ComponentTreePainter.h"

class ComponentTreeView : public pipeline::SimpleProcessNode<> {

public:

	ComponentTreeView();

private:

	void updateOutputs();

	pipeline::Input<ComponentTree>         _componentTree;
	pipeline::Output<ComponentTreePainter> _painter;

	signals::Slot<const gui::ContentChanged> _contentChanged;
	signals::Slot<const gui::SizeChanged>    _sizeChanged;
};

#endif // GUI_COMPONENT_TREE_VIEW_H__

