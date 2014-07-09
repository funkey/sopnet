#include "FindSuperPixels.h"
#include <vigra/multi_pointoperators.hxx>
#include <vigra/multi_resize.hxx>
#include <vigra/multi_watersheds.hxx>

FindSuperPixels::FindSuperPixels() {

	registerInput(_boundaryMap, "boundary map");
	registerInput(_seeds, "seeds");
	registerOutput(_labels, "labels");
}

void
FindSuperPixels::updateOutputs() {

	if (!_labels)
		_labels = new ImageStack();
	else
		_labels->clear();

	float minX = _boundaryMap->getOffsetX();
	float minY = _boundaryMap->getOffsetY();
	float minZ = _boundaryMap->getOffsetZ();

	float resX = _boundaryMap->getResolutionX();
	float resY = _boundaryMap->getResolutionY();
	float resZ = _boundaryMap->getResolutionZ();

	if (resX != resY)
		UTIL_THROW_EXCEPTION(
				NotYetImplemented,
				"anisotropic x and y is not yet implemented");

	vigra::Shape3 size(
			_boundaryMap->width(),
			_boundaryMap->height(),
			_boundaryMap->size());

	// create boundary map
	vigra::MultiArray<3, float> anisotropic(size);
	for (int z = 0; z < size[2]; z++)
		vigra::copyMultiArray(
				*(*_boundaryMap)[z],
				anisotropic.bind<2>(z));

	// correct for anisotropy
	float correctZ = resZ/resX;
	resZ /= correctZ;
	size[2] *= correctZ;

	vigra::MultiArray<3, float> boundaries(size);
	vigra::resizeMultiArraySplineInterpolation(
			anisotropic,
			boundaries);

	// create seed point image
	vigra::MultiArray<3, float> seeds(size);
	unsigned int label = 1;
	foreach (Sphere& sphere, *_seeds) {

		unsigned int x = (sphere.getX() - minX)/resX;
		unsigned int y = (sphere.getY() - minY)/resY;
		unsigned int z = (sphere.getZ() - minZ)/resZ;

		seeds(x, y, z) = label++;
	}

	// perform watershed
	vigra::watershedsMultiArray(
			boundaries,
			seeds,
			vigra::DirectNeighborhood,
			vigra::WatershedOptions().stopAtThreshold(0.99));

	// provide labels as output
	_labels->setOffset(minX, minY, minZ);
	_labels->setResolution(resX, resY, resZ);
	for (unsigned int z = 0; z < size[2]; z++) {

		boost::shared_ptr<Image> image = boost::make_shared<Image>(size[0], size[1]);
		vigra::copyMultiArray(
				seeds.bind<2>(z),
				*image);
		_labels->add(image);
	}
}
