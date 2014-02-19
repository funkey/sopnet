#include <gui/OpenGl.h>
#include "SplitMergePainter.h"

logger::LogChannel splitmergepainterlog("splitmergepainterlog", "[SplitMergePainter] ");

SplitMergePainter::SplitMergePainter() :
		_section(0) {

	updateSize();
}

bool
SplitMergePainter::draw(
		const util::rect<double>&  /*roi*/,
		const util::point<double>& /*resolution*/) {

	LOG_ALL(splitmergepainterlog) << "drawing selection for section " << _section << std::endl;

	foreach (boost::shared_ptr<Slice> slice, *_selection)
		if (slice->getSection() == (unsigned int)_section)
			drawSlice(*slice);

	return false;
}

void
SplitMergePainter::updateSize() {

	util::rect<double> bb(0, 0, 0, 0);

	if (_selection) {

		foreach (boost::shared_ptr<Slice> slice, *_selection) {

			if (bb.isZero())
				bb = slice->getComponent()->getBoundingBox();
			else
				bb.fit(slice->getComponent()->getBoundingBox());
		}
	}

	setSize(bb);
}

void
SplitMergePainter::drawSlice(const Slice& slice) {

	util::point<double> center = slice.getComponent()->getCenter();

	glEnable(GL_BLEND);
	glColor4f(0.7, 03, 0.4, 0.5);
	glBegin(GL_QUADS);
	glVertex2d(center.x - 10, center.y - 10);
	glVertex2d(center.x - 10, center.y + 10);
	glVertex2d(center.x + 10, center.y + 10);
	glVertex2d(center.x + 10, center.y - 10);
	glEnd();
}

