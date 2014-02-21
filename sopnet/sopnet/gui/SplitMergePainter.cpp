#include <gui/OpenGl.h>
#include "SplitMergePainter.h"

logger::LogChannel splitmergepainterlog("splitmergepainterlog", "[SplitMergePainter] ");

SplitMergePainter::SplitMergePainter() :
		_section(0) {

	updateSize();
}

void
SplitMergePainter::setSliceImage(boost::shared_ptr<Image> sliceImage, const util::point<int>& offset) {

	_sliceImagePainter = boost::make_shared<gui::ImagePainter<Image> >();
	_sliceImagePainter->setImage(sliceImage);
	_sliceImageOffset = offset;
}

void
SplitMergePainter::reloadSliceImage() {

	_sliceImagePainter->reload();
}

void
SplitMergePainter::unsetSliceImage() {

	_sliceImagePainter.reset();
}

bool
SplitMergePainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	if (_sliceImagePainter) {

		LOG_ALL(splitmergepainterlog) << "drawing slice image for section " << _section << std::endl;

		glPushMatrix();
		glTranslatef(_sliceImageOffset.x, _sliceImageOffset.y, 0);
		_sliceImagePainter->draw(roi - _sliceImageOffset, resolution);
		glPopMatrix();

		return false;
	}

	LOG_ALL(splitmergepainterlog) << "drawing selection for section " << _section << std::endl;

	foreach (boost::shared_ptr<Slice> slice, *_selection)
		if (slice->getSection() == (unsigned int)_section)
			drawSlice(*slice);

	return false;
}

void
SplitMergePainter::updateSize() {

	util::rect<double> bb(0, 0, 0, 0);

	if (_sliceImagePainter) {

		bb = _sliceImagePainter->getSize() + _sliceImageOffset;
	}

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

