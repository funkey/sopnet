#include <boost/make_shared.hpp>

#include <vigra/functorexpression.hxx>
#include <vigra/distancetransform.hxx>
#include <vigra/transformimage.hxx>

#include <imageprocessing/ConnectedComponent.h>
#include "Slice.h"

util::ProgramOption
Slice::optionMaxDistanceMapValue(
		util::_module           = "sopnet.slices",
		util::_long_name        = "maxDistanceMapValue",
		util::_description_text = "The maximal Euclidean distance value to consider for point-to-slice comparisons. Points further away than this value will have this value.",
		util::_default_value    = 50);

Slice::Slice(
		unsigned int id,
		unsigned int section,
		boost::shared_ptr<ConnectedComponent> component) :
	_id(id),
	_section(section),
	_component(component) {

	computeDistanceMap();
}

unsigned int
Slice::getId() const {

	return _id;
}

unsigned int
Slice::getSection() const {

	return _section;
}

boost::shared_ptr<ConnectedComponent>
Slice::getComponent() const {

	return _component;
}

void
Slice::intersect(const Slice& other) {

	_component = boost::make_shared<ConnectedComponent>(getComponent()->intersect(*other.getComponent()));

	computeDistanceMap();
}

void
Slice::computeDistanceMap() {

	unsigned int maxDistance = optionMaxDistanceMapValue.as<unsigned int>();

	const util::rect<double>& boundingBox = _component->getBoundingBox();

	// comput size and offset of distance map
	_distanceMapSize.minX = boundingBox.minX - maxDistance;
	_distanceMapSize.minY = boundingBox.minY - maxDistance;
	_distanceMapSize.maxX = boundingBox.maxX + maxDistance;
	_distanceMapSize.maxY = boundingBox.maxY + maxDistance;

	distance_map_type::size_type shape(_distanceMapSize.width(), _distanceMapSize.height());

	// create object image
	distance_map_type objectImage(shape, 0.0);

	// copy slice pixels into object image
	foreach (const util::point<unsigned int>& pixel, _component->getPixels()) {

		unsigned int x = pixel.x - boundingBox.minX + maxDistance;
		unsigned int y = pixel.y - boundingBox.minY + maxDistance;

		if (x < 0 || x >= _distanceMapSize.width() || y < 0 || y >= _distanceMapSize.height()) {

			LOG_ERROR(logger::out) << "[Slice] invalid pixel position: " << x << ", " << y << std::endl;

		} else {

			objectImage(x, y) = 1.0;
		}
	}

	// reshape distance map
	_distanceMap.reshape(shape);

	// perform distance transform with Euclidean norm
	vigra::distanceTransform(srcImageRange(objectImage), destImage(_distanceMap), 0.0, 2);

	using namespace vigra::functor;

	// cut values to maxDistance
	vigra::transformImage(srcImageRange(_distanceMap), destImage(_distanceMap), min(Arg1(), Param(maxDistance)));
}
