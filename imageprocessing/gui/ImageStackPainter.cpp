#include "ImageStackPainter.h"

static logger::LogChannel imagestackpainterlog("imagestackpainterlog", "[ImageStackPainter] ");

ImageStackPainter::ImageStackPainter() :
	_section(0) {}

void
ImageStackPainter::setImageStack(boost::shared_ptr<ImageStack> stack) {

	LOG_DEBUG(imagestackpainterlog) << "got a new stack" << std::endl;

	_stack = stack;

	setCurrentSection(_section);
}

void
ImageStackPainter::setCurrentSection(unsigned int section) {

	_section = std::min(section, _stack->size() - 1);

	_imagePainter.setImage((*_stack)[_section]);
	_imagePainter.update();

	setSize(_imagePainter.getSize());

	LOG_DEBUG(imagestackpainterlog) << "current section set to " << _section << std::endl;
}

void
ImageStackPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	LOG_ALL(imagestackpainterlog) << "redrawing section " << _section << std::endl;

	_imagePainter.draw(roi, resolution);
}
