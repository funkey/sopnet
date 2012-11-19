#include <imageprocessing/ConnectedComponent.h>
#include <util/rect.hpp>
#include <sopnet/slices/Slice.h>
#include "SetDifference.h"

double
SetDifference::operator()(const Slice& slice1, const Slice& slice2, bool normalized, bool align) {

	const util::rect<double>& bb = slice1.getComponent()->getBoundingBox();

	util::point<int> offset(static_cast<int>(bb.minX), static_cast<int>(bb.minY));
	util::point<int> size(static_cast<int>(bb.width() + 2), static_cast<int>(bb.height() + 2));

	std::vector<bool> pixels(size.x*size.y, false);

	foreach (const util::point<unsigned int>& pixel, slice1.getComponent()->getPixels()) {

		int x = (int)pixel.x - offset.x;
		int y = (int)pixel.y - offset.y;

		pixels[x + y*size.x] = true;
	}

	util::point<double> centerOffset(0, 0);
	if (align)
		centerOffset = slice1.getComponent()->getCenter() - slice2.getComponent()->getCenter();

	unsigned int different = numDifferent(
			slice1.getComponent()->getSize(),
			pixels,
			size,
			centerOffset,
			offset,
			slice2);

	if (normalized)
		return static_cast<double>(different)/(slice1.getComponent()->getSize() + slice2.getComponent()->getSize());
	else
		return different;
}

double
SetDifference::operator()(const Slice& slice1a, const Slice& slice1b, const Slice& slice2, bool normalized, bool align) {

	const util::rect<double>& bba = slice1a.getComponent()->getBoundingBox();
	const util::rect<double>& bbb = slice1b.getComponent()->getBoundingBox();

	util::rect<double> bb(std::min(bba.minX, bbb.minX), std::min(bba.minY, bbb.minY), std::max(bba.maxX, bbb.maxX), std::max(bba.maxY, bbb.maxY));

	util::point<int> offset(static_cast<int>(bb.minX), static_cast<int>(bb.minY));
	util::point<int> size(static_cast<int>(bb.width() + 2), static_cast<int>(bb.height() + 2));

	std::vector<bool> pixels(size.x*size.y, false);

	foreach (const util::point<unsigned int>& pixel, slice1a.getComponent()->getPixels()) {

		int x = (int)pixel.x - offset.x;
		int y = (int)pixel.y - offset.y;

		pixels[x + y*size.x] = true;
	}
	foreach (const util::point<unsigned int>& pixel, slice1b.getComponent()->getPixels()) {

		int x = (int)pixel.x - offset.x;
		int y = (int)pixel.y - offset.y;

		pixels[x + y*size.x] = true;
	}

	util::point<double> centerOffset(0, 0);
	if (align) 
		centerOffset =
				(slice1a.getComponent()->getCenter()*slice1a.getComponent()->getSize()
				 +
				 slice1b.getComponent()->getCenter()*slice1b.getComponent()->getSize())
				/
				(double)(slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize())
				-
				slice2.getComponent()->getCenter();

	unsigned int different = numDifferent(
			slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize(),
			pixels,
			size,
			centerOffset,
			offset,
			slice2);

	if (normalized)
		return static_cast<double>(different)/(slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize() + slice2.getComponent()->getSize());
	else
		return different;
}

unsigned int
SetDifference::numDifferent(
		unsigned int size1,
		const std::vector<bool>& pixels,
		const util::point<int>& size,
		const util::point<double>& centerOffset,
		const util::point<double>& offset,
		const Slice& slice2) {

	// number of pixels in 2 but not 1
	unsigned int in2not1 = 0;

	// number of pixels that are both in 1 and 2
	unsigned int shared  = 0;

	foreach (const util::point<unsigned int>& pixel, slice2.getComponent()->getPixels()) {

		int x = (int)pixel.x - centerOffset.x - offset.x;
		int y = (int)pixel.y - centerOffset.x - offset.y;

		// not even close
		if (x < 0 || x >= size.x || y < 0 || y >= size.y) {

			in2not1++;
			continue;
		}
		
		// within bounding box
		if (pixels[x + y*size.x] == false)
			// not in 1, but in 2
			in2not1++;
		else
			// both in 1 and 2
			shared++;
	}

	unsigned int in1not2 = size1 - shared;

	assert(in2not1 + shared == slice2.getComponent()->getSize());

	return in2not1 + in1not2;
}
