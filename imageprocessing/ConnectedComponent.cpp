#include <boost/make_shared.hpp>

#include <imageprocessing/exceptions.h>
#include "ConnectedComponent.h"

ConnectedComponent::ConnectedComponent() :
	_boundingBox(0, 0, 0, 0),
	_center(0, 0) {}

ConnectedComponent::ConnectedComponent(boost::shared_ptr<Image> source, double value, boost::shared_ptr<pixel_list_type> pixelList, unsigned int begin, unsigned int end) :
	_value(value),
	_boundingBox(0, 0, 0, 0),
	_center(0, 0),
	_source(source),
	_pixels(pixelList),
	_begin(_pixels->begin() + begin),
	_end(_pixels->begin() + end) {

	if (getSize() > 0) {

		_boundingBox.minX = (*_pixels)[0].x;
		_boundingBox.maxX = (*_pixels)[0].x + 1;
		_boundingBox.minY = (*_pixels)[0].y;
		_boundingBox.maxY = (*_pixels)[0].y + 1;
	}

	foreach (const util::point<unsigned int>& pixel, getPixels()) {

		_boundingBox.minX = std::min(_boundingBox.minX, (double)pixel.x);
		_boundingBox.maxX = std::max(_boundingBox.maxX, (double)pixel.x + 1);
		_boundingBox.minY = std::min(_boundingBox.minY, (double)pixel.y);
		_boundingBox.maxY = std::max(_boundingBox.maxY, (double)pixel.y + 1);

		_center += pixel;
	}

	_center /= getSize();
}

double
ConnectedComponent::getValue() const {

	return _value;
}

void
ConnectedComponent::addPixel(const util::point<unsigned int>& pixel) {

	if (!_pixels)
		_pixels = boost::make_shared<pixel_list_type>();

	if (_end != _pixels->end())
		BOOST_THROW_EXCEPTION(
				InvalidOperation()
						<< error_message("Cannot add pixels to connected components with shared pixel lists")
						<< STACK_TRACE);

	// update the size of the bounding box
	if (getSize() == 0) {

		_boundingBox.minX = pixel.x;
		_boundingBox.maxX = pixel.x + 1;
		_boundingBox.minY = pixel.y;
		_boundingBox.maxY = pixel.y + 1;

	} else {

		_boundingBox.minX = std::min(_boundingBox.minX, (double)pixel.x);
		_boundingBox.maxX = std::max(_boundingBox.maxX, (double)pixel.x + 1);
		_boundingBox.minY = std::min(_boundingBox.minY, (double)pixel.y);
		_boundingBox.maxY = std::max(_boundingBox.maxY, (double)pixel.y + 1);
	}

	// update the center
	_center = (_center*getSize() + pixel)/(getSize() + 1);

	// add the pixel
	_pixels->push_back(pixel);

	// remember the new end
	_end++;
}

const util::point<double>&
ConnectedComponent::getCenter() const {

	return _center;
}

void
ConnectedComponent::addPixels(const std::vector<util::point<unsigned int> >& pixels) {

	for (int i = 0; i < pixels.size(); i++)
		addPixel(pixels[i]);
}

const std::pair<ConnectedComponent::const_iterator, ConnectedComponent::const_iterator>
ConnectedComponent::getPixels() const {

	return std::make_pair(_begin, _end);
}

const boost::shared_ptr<ConnectedComponent::pixel_list_type>
ConnectedComponent::getPixelList() const {

	return _pixels;
}

const unsigned int
ConnectedComponent::getSize() const {

	return _end - _begin;
}

const util::rect<double>&
ConnectedComponent::getBoundingBox() const {

	return _boundingBox;
}

bool
ConnectedComponent::operator<(const ConnectedComponent& other) const {

	return getSize() < other.getSize();
}

