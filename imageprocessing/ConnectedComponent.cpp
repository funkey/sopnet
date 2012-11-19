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

	_pixels(pixelList),
	_value(value),
	_boundingBox(0, 0, 0, 0),
	_center(0, 0),
	_source(source),
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

		_boundingBox.minX = std::min(_boundingBox.minX, (int)pixel.x);
		_boundingBox.maxX = std::max(_boundingBox.maxX, (int)pixel.x + 1);
		_boundingBox.minY = std::min(_boundingBox.minY, (int)pixel.y);
		_boundingBox.maxY = std::max(_boundingBox.maxY, (int)pixel.y + 1);

		_center += pixel;
	}

	_center /= getSize();

	_bitmap.reshape(bitmap_type::size_type(_boundingBox.width(), _boundingBox.height()), false);

	foreach (const util::point<int>& pixel, getPixels())
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

unsigned int
ConnectedComponent::getSize() const {

	return _end - _begin;
}

const util::rect<int>&
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

ConnectedComponent
ConnectedComponent::intersect(const ConnectedComponent& other) {

	boost::shared_ptr<pixel_list_type> intersection = boost::make_shared<pixel_list_type>();

	bitmap_type::size_type size = _bitmap.shape();

	foreach (const util::point<unsigned int>& pixel, other.getPixels())
		if (_boundingBox.contains(pixel)) {

			unsigned int x = pixel.x - _boundingBox.minX;
			unsigned int y = pixel.y - _boundingBox.minY;

			if (x >= size[0] || y >= size[1])
				continue;

			if (_bitmap(x, y))
				intersection->push_back(pixel);
		}

	return ConnectedComponent(_source, _value, intersection, 0, intersection->size());
}
