#include <gui/OpenGl.h>
#include <util/point.hpp>
#include <util/foreach.h>
#include <util/Logger.h>
#include "ComponentTreePainter.h"

static logger::LogChannel componenttreepainterlog("componenttreepainterlog", "[ComponentTreePainter] ");

void
ComponentTreePainter::setComponentTree(boost::shared_ptr<ComponentTree> componentTree) {

	_componentTree = componentTree;

	updateRecording();

	LOG_DEBUG(componenttreepainterlog) << "update size to " << _componentTree->getBoundingBox() << std::endl;

	setSize(_componentTree->getBoundingBox());
}

void
ComponentTreePainter::updateRecording() {

	if (!_componentTree)
		return;

	// make sure OpenGl operations are save
	gui::OpenGl::Guard guard;

	startRecording();

	// enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// always draw the components
	glDisable(GL_CULL_FACE);

	// draw each connected component except for the fake root node
	ComponentPaintVisitor painter;

	foreach (boost::shared_ptr<ComponentTree::Node> node, _componentTree->getRoot()->getChildren())
		_componentTree->visit(node, painter);

	glDisable(GL_BLEND);

	stopRecording();
}

ComponentTreePainter::ComponentPaintVisitor::ComponentPaintVisitor() :
	_zScale(-50.0) {}

void
ComponentTreePainter::ComponentPaintVisitor::visitNode(boost::shared_ptr<ComponentTree::Node> node) const {

	boost::shared_ptr<ConnectedComponent> component = node->getComponent();

	double value = component->getValue();

	glColor4f(value, value, value, 0.5);

	glEnable(GL_DEPTH_TEST);

	LOG_ALL(componenttreepainterlog) << "drawing component with " << (component->getPixels().second - component->getPixels().first) << " pixels" << std::endl;

	foreach (util::point<unsigned int> p, component->getPixels()) {

		glBegin(GL_QUADS);
		glVertex3d(p.x,     p.y,     value*_zScale);
		glVertex3d(p.x + 1, p.y,     value*_zScale);
		glVertex3d(p.x + 1, p.y + 1, value*_zScale);
		glVertex3d(p.x,     p.y + 1, value*_zScale);
		glEnd();
	}
}

void
ComponentTreePainter::ComponentPaintVisitor::visitEdge(
		boost::shared_ptr<ComponentTree::Node> parent,
		boost::shared_ptr<ComponentTree::Node> child) const {

	double parentValue = parent->getComponent()->getValue();
	double childValue  = child->getComponent()->getValue();

	const util::point<double> parentCenter = parent->getComponent()->getCenter();
	const util::point<double> childCenter  = child->getComponent()->getCenter();

	LOG_ALL(componenttreepainterlog) << "drawing edge from " << parentCenter << " to " << childCenter << std::endl;

	glColor4f(0.0, 0.0, 0.0, 1.0);

	glBegin(GL_LINES);
	glVertex3d(parentCenter.x, parentCenter.y, parentValue*_zScale);
	glVertex3d(childCenter.x,  childCenter.y,   childValue*_zScale);
	glEnd();
}
