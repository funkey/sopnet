#include "SphereHoughSpace.h"
#include "FindSpheres.h"

FindSpheres::FindSpheres() {

	registerInput(_neuron, "neuron");
	registerOutput(_spheres, "spheres");
	registerOutput(_houghSpace, "hough space");
}

void
FindSpheres::updateOutputs() {

	if (!_spheres)
		_spheres = new Spheres();

	const unsigned int zRes = 10;

	double minX, minY, minZ;
	double maxX, maxY, maxZ;

	getBoundingBox(
			*_neuron,
			minX, maxX,
			minY, maxY,
			minZ, maxZ);

	SphereHoughSpace houghSpace(
			minX,      maxX,      10,
			minY,      maxY,      10,
			minZ*zRes, maxZ*zRes, 1,
			10, 40, 10);

	std::cout
			<< "bounding box is: "
			<< minX << ", " << maxX << "; "
			<< minY << ", " << maxY << "; "
			<< minZ << ", " << maxZ << std::endl;

	foreach (boost::shared_ptr<Segment> segment, _neuron->getSegments()) {
		foreach (boost::shared_ptr<Slice> slice, segment->getSlices()) {

			util::rect<double>                     boundingBox = slice->getComponent()->getBoundingBox();
			const ConnectedComponent::bitmap_type& bitmap      = slice->getComponent()->getBitmap();
			unsigned int                           section     = slice->getSection();

			vigra::Shape2 i;
			for (i[1] = 0; i[1] != bitmap.shape()[1]; i[1]++)
			for (i[0] = 0; i[0] != bitmap.shape()[0]; i[0]++) {

				if (isBoundaryPixel(bitmap, i)) {

					float x = boundingBox.minX + i[0];
					float y = boundingBox.minY + i[1];

					std::cout << "adding point at " << x << ", " << y << ", " << section << std::endl;
					houghSpace.addBoundaryPoint(x, y, section*zRes);
				}
			}
		}
	}

	*_spheres = houghSpace.getSpheres(10);

	_houghSpace = houghSpace.getHoughSpace();
}

void
FindSpheres::getBoundingBox(
		const SegmentTree& neuron,
		double& minX, double& maxX,
		double& minY, double& maxY,
		double& minZ, double& maxZ) {

	minX = std::numeric_limits<double>::max();
	maxX = std::numeric_limits<double>::min();
	minY = std::numeric_limits<double>::max();
	maxY = std::numeric_limits<double>::min();
	minZ = std::numeric_limits<double>::max();
	maxZ = std::numeric_limits<double>::min();

	foreach (boost::shared_ptr<Segment> segment, neuron.getSegments())
		foreach (boost::shared_ptr<Slice> slice, segment->getSlices()) {

			util::rect<double> boundingBox = slice->getComponent()->getBoundingBox();

			minX = std::min(boundingBox.minX, minX);
			maxX = std::max(boundingBox.maxX, maxX);
			minY = std::min(boundingBox.minY, minY);
			maxY = std::max(boundingBox.maxY, maxY);
			minZ = std::min(static_cast<double>(slice->getSection()    ), minZ);
			maxZ = std::max(static_cast<double>(slice->getSection() + 1), maxZ);
		}
}

bool
FindSpheres::isBoundaryPixel(
		const ConnectedComponent::bitmap_type& bitmap,
		vigra::Shape2& location) {

	if (bitmap[location] == 0)
		return false;

	if (location[0] == 0 || location[0] == bitmap.shape()[0] - 1)
		return true;
	if (location[1] == 0 || location[1] == bitmap.shape()[1] - 1)
		return true;

	if (bitmap[location - vigra::Shape2(0, 1)] == 0)
		return true;
	if (bitmap[location + vigra::Shape2(0, 1)] == 0)
		return true;
	if (bitmap[location - vigra::Shape2(1, 0)] == 0)
		return true;
	if (bitmap[location + vigra::Shape2(1, 0)] == 0)
		return true;

	return false;
}
