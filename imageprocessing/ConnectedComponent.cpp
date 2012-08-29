#include <boost/make_shared.hpp>

#include <imageprocessing/exceptions.h>
#include "ConnectedComponent.h"

ConnectedComponent::ConnectedComponent() :
	_boundingBox(0, 0, 0, 0),
	_center(0, 0) {}

ConnectedComponent::ConnectedComponent(
		boost::shared_ptr<Image> source,
		double value,
		boost::shared_ptr<pixel_list_type> pixelList,
		unsigned int begin,
		unsigned int end) :

	_value(value),
	_boundingBox(0, 0, 0, 0),
	_center(0, 0),
	_source(source),
	_pixels(pixelList),
	_begin(_pixels->begin() + begin),
	_end(_pixels->begin() + end) {

	// if there is at least one pixel
	if (begin != end) {

		_boundingBox.minX = _begin->x;
		_boundingBox.maxX = _begin->x + 1;
		_boundingBox.minY = _begin->y;
		_boundingBox.maxY = _begin->y + 1;
	}

	foreach (const util::point<unsigned int>& pixel, getPixels()) {

		_boundingBox.minX = std::min(_boundingBox.minX, (double)pixel.x);
		_boundingBox.maxX = std::max(_boundingBox.maxX, (double)pixel.x + 1);
		_boundingBox.minY = std::min(_boundingBox.minY, (double)pixel.y);
		_boundingBox.maxY = std::max(_boundingBox.maxY, (double)pixel.y + 1);

		_center += pixel;
	}

	_center /= getSize();

	_bitmap.reshape(bitmap_type::size_type(_boundingBox.width(), _boundingBox.height()), false);

	foreach (const util::point<unsigned int>& pixel, getPixels())
		_bitmap(pixel.x - _boundingBox.minX, pixel.y - _boundingBox.minY) = true;
}

double
ConnectedComponent::getValue() const {

	return _value;
}

const util::point<double>&
ConnectedComponent::getCenter() const {

	return _center;
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

const ConnectedComponent::bitmap_type&
ConnectedComponent::getBitmap() const {

	return _bitmap;
}

bool
ConnectedComponent::operator<(const ConnectedComponent& other) const {

	return getSize() < other.getSize();
}

