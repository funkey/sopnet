#include <imageprocessing/ConnectedComponent.h>
#include <util/rect.hpp>
#include <sopnet/slices/Slice.h>
#include "SetDifference.h"

double
SetDifference::operator()(const Slice& slice1, const Slice& slice2, bool normalized) {

	const util::rect<double>& bb = slice1.getComponent()->getBoundingBox();

	util::point<unsigned int> offset(static_cast<unsigned int>(bb.minX), static_cast<unsigned int>(bb.minY));
	util::point<unsigned int> size(static_cast<unsigned int>(bb.width() + 2), static_cast<unsigned int>(bb.height() + 2));

	std::vector<bool> pixels(size.x*size.y, false);

	foreach (const util::point<unsigned int>& pixel, slice1.getComponent()->getPixels()) {

		unsigned int x = pixel.x - offset.x;
		unsigned int y = pixel.y - offset.y;

		pixels[x + y*size.x] = true;
	}

	util::point<double> centerOffset = slice1.getComponent()->getCenter() - slice2.getComponent()->getCenter();

	unsigned int different = 0;

	foreach (const util::point<unsigned int>& pixel, slice2.getComponent()->getPixels()) {

		unsigned int x = pixel.x - centerOffset.x - offset.x;
		unsigned int y = pixel.y - centerOffset.x - offset.y;

		if (x < 0 || x >= size.x || y < 0 || y >= size.y || pixels[x + y*size.x] != true)
			different++;
	}

	if (normalized)
		return static_cast<double>(different)/(slice1.getComponent()->getSize() + slice2.getComponent()->getSize());
	else
		return different;
}

double
SetDifference::operator()(const Slice& slice1a, const Slice& slice1b, const Slice& slice2, bool normalized) {

	const util::rect<double>& bba = slice1a.getComponent()->getBoundingBox();
	const util::rect<double>& bbb = slice1b.getComponent()->getBoundingBox();

	util::rect<double> bb(std::min(bba.minX, bbb.minX), std::min(bba.minY, bbb.minY), std::max(bba.maxX, bbb.maxX), std::max(bba.maxY, bbb.maxY));

	util::point<unsigned int> offset(static_cast<unsigned int>(bb.minX), static_cast<unsigned int>(bb.minY));
	util::point<unsigned int> size(static_cast<unsigned int>(bb.width() + 2), static_cast<unsigned int>(bb.height() + 2));

	std::vector<bool> pixels(size.x*size.y, false);

	foreach (const util::point<unsigned int>& pixel, slice1a.getComponent()->getPixels()) {

		unsigned int x = pixel.x - offset.x;
		unsigned int y = pixel.y - offset.y;

		pixels[x + y*size.x] = true;
	}
	foreach (const util::point<unsigned int>& pixel, slice1b.getComponent()->getPixels()) {

		unsigned int x = pixel.x - offset.x;
		unsigned int y = pixel.y - offset.y;

		pixels[x + y*size.x] = true;
	}

	util::point<double> centerOffset =
			(slice1a.getComponent()->getCenter()*slice1a.getComponent()->getSize()
			 +
			 slice1b.getComponent()->getCenter()*slice1b.getComponent()->getSize())
			/
			(double)(slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize())
			-
			slice2.getComponent()->getCenter();

	unsigned int different = 0;

	foreach (const util::point<unsigned int>& pixel, slice2.getComponent()->getPixels()) {

		unsigned int x = pixel.x - centerOffset.x - offset.x;
		unsigned int y = pixel.y - centerOffset.x - offset.y;

		if (x < 0 || x >= size.x || y < 0 || y >= size.y || pixels[x + y*size.x] != true)
			different++;
	}

	if (normalized)
		return static_cast<double>(different)/(slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize() + slice2.getComponent()->getSize());
	else
		return different;
}
