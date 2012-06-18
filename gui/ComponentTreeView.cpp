#include <util/Logger.h>
#include "ComponentTreeView.h"

static logger::LogChannel componenttreeviewlog("componenttreeviewlog", "[ComponentTreeView] ");

ComponentTreeView::ComponentTreeView() {

	registerInput(_componentTree, "component tree");
	registerOutput(_painter, "painter");

	_painter.registerForwardSlot(_contentChanged);
	_painter.registerForwardSlot(_sizeChanged);
}

void
ComponentTreeView::updateOutputs() {

	util::rect<double> oldSize = _painter->getSize();

	_painter->setComponentTree(_componentTree);

	if (_painter->getSize() == oldSize)
		_contentChanged();
	else
		_sizeChanged(_painter->getSize());
}
