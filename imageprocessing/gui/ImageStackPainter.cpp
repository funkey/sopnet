#include "ImageStackPainter.h"

static logger::LogChannel imagestackpainterlog("imagestackpainterlog", "[ImageStackPainter] ");

ImageStackPainter::ImageStackPainter(unsigned int numImages) :
	_numImages(numImages),
	_section(0) {

	for (int i = 0; i < _numImages; i++)
		_imagePainters.push_back(boost::make_shared<gui::ImagePainter<Image> >());
}

void
ImageStackPainter::setImageStack(boost::shared_ptr<ImageStack> stack) {

	LOG_DEBUG(imagestackpainterlog) << "got a new stack" << std::endl;

	_stack = stack;

	setCurrentSection(_section);
}

void
ImageStackPainter::setCurrentSection(unsigned int section) {

	if (!_stack || _stack->size() == 0 || _imagePainters.size() == 0)
		return;

	_section = std::min(section, _stack->size() - 1);

	for (int i = 0; i < _numImages; i++) {

		int imageIndex = std::max(std::min(static_cast<int>(_section) + static_cast<int>(i - _numImages/2), static_cast<int>(_stack->size()) - 1), 0);

		LOG_ALL(imagestackpainterlog) << "index for image " << i << " is " << imageIndex << std::endl;

		_imagePainters[i]->setImage((*_stack)[imageIndex]);
		_imagePainters[i]->update();
	}

	util::rect<double> size = _imagePainters[0]->getSize();

	_imageHeight = size.height();

	size.minY -= _numImages/2*_imageHeight;
	size.maxY += (_numImages/2 - (_numImages + 1)%2)*_imageHeight;

	setSize(size);

	LOG_DEBUG(imagestackpainterlog) << "current section set to " << _section << std::endl;
}

void
ImageStackPainter::draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution) {

	LOG_ALL(imagestackpainterlog) << "redrawing section " << _section << std::endl;

	for (int i = 0; i < _numImages; i++) {

		int offset = i - _numImages/2;

		glTranslated(0, -offset*_imageHeight, 0);

		_imagePainters[i]->draw(roi - util::point<double>(static_cast<double>(0), -offset*_imageHeight), resolution);

		glTranslated(0,  offset*_imageHeight, 0);
	}
}
