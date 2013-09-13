#include <gui/OpenGl.h>
#include "SplitMergePainter.h"

logger::LogChannel splitmergepainterlog("splitmergepainterlog", "[SplitMergePainter] ");

SplitMergePainter::SplitMergePainter() :
		_section(0) {}

bool
SplitMergePainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	LOG_ALL(splitmergepainterlog) << "drawing selection for section " << _section << std::endl;

	foreach (boost::shared_ptr<EndSegment> end, _selection->getEnds(_section)) {

		LOG_ALL(splitmergepainterlog) << "drawing end segment" << std::endl;

		util::rect<int> bb = end->getSlice()->getComponent()->getBoundingBox();

		glEnable(GL_BLEND);
		glColor4f(0.7, 03, 0.4, 0.5);
		glBegin(GL_QUADS);
		glVertex2d(bb.minX, bb.minY);
		glVertex2d(bb.minX, bb.maxY);
		glVertex2d(bb.maxX, bb.maxY);
		glVertex2d(bb.maxX, bb.minY);
		glEnd();
	}

	foreach (boost::shared_ptr<ContinuationSegment> continuation, _selection->getContinuations(_section)) {

		LOG_ALL(splitmergepainterlog) << "drawing continuation segment" << std::endl;

		boost::shared_ptr<Slice> slice = (continuation->getDirection() == Left ? continuation->getSourceSlice() : continuation->getTargetSlice());
		util::rect<int> bb = slice->getComponent()->getBoundingBox();

		glEnable(GL_BLEND);
		glColor4f(0.7, 03, 0.4, 0.5);
		glBegin(GL_QUADS);
		glVertex2d(bb.minX, bb.minY);
		glVertex2d(bb.minX, bb.maxY);
		glVertex2d(bb.maxX, bb.maxY);
		glVertex2d(bb.maxX, bb.minY);
		glEnd();
	}

	return false;
}
