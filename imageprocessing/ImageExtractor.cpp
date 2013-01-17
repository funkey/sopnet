#include <boost/lexical_cast.hpp>

#include <imageprocessing/ImageStack.h>
#include <imageprocessing/Image.h>
#include "ImageExtractor.h"

static logger::LogChannel imageextractorlog("imageextractorlog", "[ImageExtractor] ");

ImageExtractor::ImageExtractor() {

	registerInput(_stack, "stack");

	_stack.registerBackwardCallback(&ImageExtractor::onInputSet, this);
}

void
ImageExtractor::onInputSet(const pipeline::InputSet<ImageStack>&) {

	LOG_ALL(imageextractorlog) << "input image stack set" << std::endl;

	updateInputs();

	_images.resize(_stack->size());

	// for each image in the stack
	for (unsigned int i = 0; i < _stack->size(); i++) {

		LOG_ALL(imageextractorlog) << "(re)setting output " << i << std::endl;

		if (_images[i]) {

			LOG_ALL(imageextractorlog) << "image was " << _images[i]->width() << "x" << _images[i]->height() << std::endl;

		} else {

			LOG_ALL(imageextractorlog) << "image was unset" << std::endl;
		}

		_images[i] = (*_stack)[i];

		LOG_ALL(imageextractorlog) << "image is " << _images[i]->width() << "x" << _images[i]->height() << std::endl;

		LOG_ALL(imageextractorlog) << "registering output 'image " << i << "'" << std::endl;

		registerOutput(_images[i], "image " + boost::lexical_cast<std::string>(i));
	}
}

void
ImageExtractor::updateOutputs() {

	// there is nothing to do here
}

unsigned int
ImageExtractor::getNumImages() {

	return _images.size();
}
