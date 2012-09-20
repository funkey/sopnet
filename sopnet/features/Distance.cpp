#include <imageprocessing/ConnectedComponent.h>
#include <util/rect.hpp>
#include <sopnet/slices/Slice.h>
#include "Distance.h"

void
Distance::operator()(
		const Slice& slice1,
		const Slice& slice2,
		bool symmetric,
		bool align,
		double& avgSliceDistance,
		double& maxSliceDistance) {

	// values to add to slice2's pixel positions
	util::point<unsigned int> offset2(0, 0);

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
	util::point<unsigned int> offset2(0, 0);

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
		const util::point<unsigned int>& offset2,
		double& avgSliceDistance,
		double& maxSliceDistance) {

	const ConnectedComponent& c1 = *s1.getComponent();
	const ConnectedComponent& c2 = *s2.getComponent();

	const util::rect<unsigned int> s2dmbb = s2.getDistanceMapBoundingBox();

	double totalDistance = 0.0;

	maxSliceDistance = 0.0;

	foreach (util::point<unsigned int> p1, c1.getPixels()) {

		// correct for offset2
		p1 += offset2;

		// is it within s2's distance map bounding box?
		if (!s2dmbb.contains(p1)) {

			totalDistance += _maxDistance;
			maxSliceDistance = std::max(maxSliceDistance, _maxDistance);
			continue;
		}

		// get p1's position in s2's distance map
		p1 -= util::point<unsigned int>(s2dmbb.minX, s2dmbb.minY);

		// add up the value
		double dist = s2.getDistanceMap()(p1.x, p1.y);
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
		const util::point<unsigned int>& offset2,
		double& avgSliceDistance,
		double& maxSliceDistance) {

	const ConnectedComponent& c1 = *s1.getComponent();
	const ConnectedComponent& c2a = *s2a.getComponent();
	const ConnectedComponent& c2b = *s2b.getComponent();

	const util::rect<unsigned int> s2dmbba = s2a.getDistanceMapBoundingBox();
	const util::rect<unsigned int> s2dmbbb = s2b.getDistanceMapBoundingBox();

	double totalDistance = 0.0;

	maxSliceDistance = 0.0;

	foreach (util::point<unsigned int> p1, c1.getPixels()) {

		// correct for offset2
		p1 += offset2;

		double distancea;

		{
			util::point<unsigned int> p1a = p1;

			// is it within s2a's distance map bounding box?
			if (!s2dmbba.contains(p1a)) {

				distancea = _maxDistance;

			} else {

				// get p1a's position in s2a's distance map
				p1a -= util::point<unsigned int>(s2dmbba.minX, s2dmbba.minY);

				// add up the value
				distancea = s2a.getDistanceMap()(p1a.x, p1a.y);
			}
		}

		double distanceb;

		{
			util::point<unsigned int> p1b = p1;

			// is it within s2b's distance map bounding box?
			if (!s2dmbbb.contains(p1b)) {

				distanceb = _maxDistance;

			} else {

				// get p1b's position in s2b's distance map
				p1b -= util::point<unsigned int>(s2dmbbb.minX, s2dmbbb.minY);

				// add up the value
				distanceb = s2b.getDistanceMap()(p1b.x, p1b.y);
			}
		}

		// take the minimum of both distances
		double dist = std::min(distancea, distanceb);
		totalDistance += dist;
		maxSliceDistance = std::max(maxSliceDistance, dist);
	}

	avgSliceDistance = totalDistance/s1.getComponent()->getSize();
}
