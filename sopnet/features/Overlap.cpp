#include <imageprocessing/ConnectedComponent.h>
#include <util/rect.hpp>
#include <sopnet/slices/Slice.h>
#include "Overlap.h"

double
Overlap::operator()(const Slice& slice1, const Slice& slice2, bool normalized, bool align) {

	// values to add to slice2's pixel positions
	util::point<int> offset2(0, 0);

	// ...only non-zero if we want to align both slices
	if (align)
		offset2 = slice1.getComponent()->getCenter() - slice2.getComponent()->getCenter();

	unsigned int numOverlap = overlap(
			*slice1.getComponent(),
			*slice2.getComponent(),
			offset2);

	if (normalized) {

		unsigned int totalSize = slice1.getComponent()->getSize() + slice2.getComponent()->getSize() - numOverlap;

		return static_cast<double>(numOverlap)/totalSize;

	} else {

		return numOverlap;
	}
}

double
Overlap::operator()(const Slice& slice1a, const Slice& slice1b, const Slice& slice2, bool normalized, bool align) {

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

	unsigned int numOverlapa = overlap(
			*slice1a.getComponent(),
			*slice2.getComponent(),
			offset2);
	unsigned int numOverlapb = overlap(
			*slice1b.getComponent(),
			*slice2.getComponent(),
			offset2);

	unsigned int numOverlap = numOverlapa + numOverlapb;

	if (normalized) {

		unsigned int totalSize = slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize() + slice2.getComponent()->getSize() - numOverlap;

		return static_cast<double>(numOverlap)/totalSize;

	} else {

		return numOverlap;
	}
}

unsigned int
Overlap::overlap(
		const ConnectedComponent& c1,
		const ConnectedComponent& c2,
		const util::point<int>& offset2) {

	if (!c1.getBoundingBox().intersects(c2.getBoundingBox() + offset2))
		return 0;

	unsigned int numOverlap = 0;

	const ConnectedComponent& smaller = (c1 < c2 ? c1 : c2);
	const ConnectedComponent& bigger  = (c1 < c2 ? c2 : c1);

	const ConnectedComponent::bitmap_type& biggerBitmap = (c1 < c2 ? c2.getBitmap() : c1.getBitmap());

	// the offset from the smaller component to the bigger component
	util::point<int> smallerToBigger = (c1 < c2 ? -offset2 : offset2);

	// the same, but to the pixel positions in the bigger component's bitmap
	util::point<int> toBitmap = smallerToBigger - util::point<int>(bigger.getBoundingBox().minX, bigger.getBoundingBox().minY);

	// width and height of the bigger bounding box
	int width  = bigger.getBoundingBox().width();
	int height = bigger.getBoundingBox().height();

	// iterate over all pixels in the smaller component
	foreach (const util::point<unsigned int>& pixel, smaller.getPixels()) {

		// add offset from smaller to bigger pixel positions in the bitmap
		util::point<int> inBitmap = util::point<int>(pixel) + toBitmap;

		if (inBitmap.x >= 0 && inBitmap.y >= 0 && inBitmap.x < width && inBitmap.y < height)
			if (biggerBitmap(inBitmap.x, inBitmap.y))
				numOverlap++;
	}

	return numOverlap;
}

