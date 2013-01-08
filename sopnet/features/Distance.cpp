#include <vigra/functorexpression.hxx>
#include <vigra/distancetransform.hxx>
#include <vigra/transformimage.hxx>

#include <imageprocessing/ConnectedComponent.h>
#include <util/rect.hpp>
#include <sopnet/slices/Slice.h>
#include "Distance.h"

util::ProgramOption optionMaxDistanceMapValue(
		util::_module           = "sopnet.features",
		util::_long_name        = "maxDistanceMapValue",
		util::_description_text = "The maximal Euclidean distance value to consider for point-to-slice comparisons. Points further away than this value will have this value.",
		util::_default_value    = 50);

Distance::Distance(double maxDistance) :
	_maxDistance(maxDistance) {

	if (_maxDistance < 0)
		_maxDistance = optionMaxDistanceMapValue;
}

void
Distance::operator()(
		const Slice& slice1,
		const Slice& slice2,
		bool symmetric,
		bool align,
		double& avgSliceDistance,
		double& maxSliceDistance) {

	// values to add to slice2's pixel positions
	util::point<int> offset2(0, 0);

	// ...only non-zero if we want to align both slices
	if (align)
		offset2 = slice1.getComponent()->getCenter() - slice2.getComponent()->getCenter();

	distance(slice1, slice2, offset2, avgSliceDistance, maxSliceDistance);

	if (symmetric) {

		double avgSliceDistanceS;
		double maxSliceDistanceS;

		distance(slice2, slice1, -offset2, avgSliceDistanceS, maxSliceDistanceS);

		avgSliceDistance += avgSliceDistanceS;
		avgSliceDistance /= 2;

		maxSliceDistance = std::max(maxSliceDistance, maxSliceDistanceS);
	}
}

void
Distance::operator()(
		const Slice& slice1a,
		const Slice& slice1b,
		const Slice& slice2,
		bool symmetric,
		bool align,
		double& avgSliceDistance,
		double& maxSliceDistance) {

	// values to add to slice2's pixel positions
	util::point<int> offset2(0, 0);

	// ...only non-zero if we want to align slice2 to both slice1s
	if (align) {

		// the mean pixel location of slice1a and slice1b
		util::point<double> center1 = 
				(slice1a.getComponent()->getCenter()*slice1a.getComponent()->getSize()
				 +
				 slice1b.getComponent()->getCenter()*slice1b.getComponent()->getSize())
				/
				(double)(slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize());

		offset2 = center1 - slice2.getComponent()->getCenter();
	}

	double avgSliceDistancea, avgSliceDistanceb;
	double maxSliceDistancea, maxSliceDistanceb;

	distance(slice1a, slice2, offset2, avgSliceDistancea, maxSliceDistancea);
	distance(slice1b, slice2, offset2, avgSliceDistanceb, maxSliceDistanceb);

	avgSliceDistance =
			(avgSliceDistancea*slice1a.getComponent()->getSize() +
			 avgSliceDistanceb*slice1b.getComponent()->getSize())/
			(slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize());

	maxSliceDistance = std::max(maxSliceDistancea, maxSliceDistanceb);

	if (symmetric) {

		double avgSliceDistanceS;
		double maxSliceDistanceS;

		distance(slice2, slice1a, slice1b, -offset2, avgSliceDistanceS, maxSliceDistanceS);

		avgSliceDistance += avgSliceDistanceS;
		avgSliceDistance /= 2;

		maxSliceDistance = std::max(maxSliceDistance, maxSliceDistanceS);
	}
}

void
Distance::distance(
		const Slice& s1,
		const Slice& s2,
		const util::point<int>& offset2,
		double& avgSliceDistance,
		double& maxSliceDistance) {

	const ConnectedComponent& c1 = *s1.getComponent();

	const util::rect<int> s2dmbb = getDistanceMapBoundingBox(s2);

	double totalDistance = 0.0;

	maxSliceDistance = 0.0;

	foreach (util::point<int> p1, c1.getPixels()) {

		// correct for offset2
		p1 += offset2;

		// is it within s2's distance map bounding box?
		if (!s2dmbb.contains(p1)) {

			totalDistance += _maxDistance;
			maxSliceDistance = std::max(maxSliceDistance, _maxDistance);
			continue;
		}

		// get p1's position in s2's distance map
		p1 -= util::point<int>(s2dmbb.minX, s2dmbb.minY);

		// add up the value
		double dist = getDistanceMap(s2)(p1.x, p1.y);
		totalDistance += dist;
		maxSliceDistance = std::max(maxSliceDistance, dist);
	}

	avgSliceDistance = totalDistance/s1.getComponent()->getSize();
}

void
Distance::distance(
		const Slice& s1,
		const Slice& s2a,
		const Slice& s2b,
		const util::point<int>& offset2,
		double& avgSliceDistance,
		double& maxSliceDistance) {

	const ConnectedComponent& c1 = *s1.getComponent();

	const util::rect<int> s2dmbba = getDistanceMapBoundingBox(s2a);
	const util::rect<int> s2dmbbb = getDistanceMapBoundingBox(s2b);

	double totalDistance = 0.0;

	maxSliceDistance = 0.0;

	foreach (util::point<int> p1, c1.getPixels()) {

		// correct for offset2
		p1 += offset2;

		double distancea;

		{
			util::point<int> p1a = p1;

			// is it within s2a's distance map bounding box?
			if (!s2dmbba.contains(p1a)) {

				distancea = _maxDistance;

			} else {

				// get p1a's position in s2a's distance map
				p1a -= util::point<int>(s2dmbba.minX, s2dmbba.minY);

				// add up the value
				distancea = getDistanceMap(s2a)(p1a.x, p1a.y);
			}
		}

		double distanceb;

		{
			util::point<int> p1b = p1;

			// is it within s2b's distance map bounding box?
			if (!s2dmbbb.contains(p1b)) {

				distanceb = _maxDistance;

			} else {

				// get p1b's position in s2b's distance map
				p1b -= util::point<int>(s2dmbbb.minX, s2dmbbb.minY);

				// add up the value
				distanceb = getDistanceMap(s2b)(p1b.x, p1b.y);
			}
		}

		// take the minimum of both distances
		double dist = std::min(distancea, distanceb);
		totalDistance += dist;
		maxSliceDistance = std::max(maxSliceDistance, dist);
	}

	avgSliceDistance = totalDistance/s1.getComponent()->getSize();
}

util::rect<int>
Distance::getDistanceMapBoundingBox(const Slice& slice) {


	const util::rect<int>& boundingBox = slice.getComponent()->getBoundingBox();

	// comput size and offset of distance map
	util::rect<int> distanceMapBoundingBox;
	distanceMapBoundingBox.minX = boundingBox.minX - _maxDistance;
	distanceMapBoundingBox.minY = boundingBox.minY - _maxDistance;
	distanceMapBoundingBox.maxX = boundingBox.maxX + _maxDistance;
	distanceMapBoundingBox.maxY = boundingBox.maxY + _maxDistance;

	return distanceMapBoundingBox;
}

const Distance::distance_map_type&
Distance::getDistanceMap(const Slice& slice) {

	if (!_distanceMaps.count(slice.getId()))
		_distanceMaps[slice.getId()] = computeDistanceMap(slice);

	return _distanceMaps[slice.getId()];
}

Distance::distance_map_type
Distance::computeDistanceMap(const Slice& slice) {

	const util::rect<int>& boundingBox = slice.getComponent()->getBoundingBox();

	// comput size and offset of distance map
	util::rect<int> distanceMapBoundingBox = getDistanceMapBoundingBox(slice);

	distance_map_type::size_type shape(distanceMapBoundingBox.width(), distanceMapBoundingBox.height());

	// create object image
	distance_map_type objectImage(shape, 0.0);

	// copy slice pixels into object image
	foreach (const util::point<unsigned int>& pixel, slice.getComponent()->getPixels()) {

		int x = pixel.x - boundingBox.minX + _maxDistance;
		int y = pixel.y - boundingBox.minY + _maxDistance;

		if (x < 0 || x >= (int)distanceMapBoundingBox.width() || y < 0 || y >= (int)distanceMapBoundingBox.height()) {

			LOG_ERROR(logger::out) << "[Distance] invalid pixel position: " << x << ", " << y << std::endl;

		} else {

			objectImage(x, y) = 1.0;
		}
	}

	// reshape distance map
	distance_map_type distanceMap;
	distanceMap.reshape(shape);

	// perform distance transform with Euclidean norm
	vigra::distanceTransform(srcImageRange(objectImage), destImage(distanceMap), 0.0, 2);

	using namespace vigra::functor;

	// cut values to maxDistance
	vigra::transformImage(srcImageRange(distanceMap), destImage(distanceMap), min(Arg1(), Param(_maxDistance)));

	return distanceMap;
}
