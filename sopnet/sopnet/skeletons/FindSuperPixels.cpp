#include "FindSuperPixels.h"
#include <vigra/multi_pointoperators.hxx>
#include <vigra/multi_resize.hxx>
#include <vigra/multi_watersheds.hxx>
#include <vigra/seededregiongrowing3d.hxx>
#include <util/ProgramOptions.h>

util::ProgramOption optionPerformWatershed(
		util::_long_name        = "performWatershed",
		util::_module           = "sopnet.skeletons",
		util::_description_text = "Perform a watershed to obtain superpixels from seeds and a boundary map. If not set, seeded region growing is used instead.",
		util::_default_value    = true);

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

	float minX = _boundaryMap->getBoundingBox().getMinX();
	float minY = _boundaryMap->getBoundingBox().getMinY();
	float minZ = _boundaryMap->getBoundingBox().getMinZ();

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

	if (optionPerformWatershed) {

		// perform watershed
		vigra::watershedsMultiArray(
				boundaries,
				seeds,
				vigra::DirectNeighborhood,
				vigra::WatershedOptions().stopAtThreshold(0.99));

	} else {

		vigra::ArrayOfRegionStatistics<vigra::SeedRgDirectValueFunctor<float> >  stats(_seeds->size());

		vigra::transformMultiArray(
				boundaries,
				boundaries,
				vigra::functor::Arg1() >= vigra::functor::Param(0.99));

		vigra::seededRegionGrowing3D(
				boundaries,
				seeds,
				seeds,
				stats,
				vigra::StopAtThreshold,
				vigra::NeighborCode3DSix(),
				0.99);
	}

	// provide labels as output
	_labels->setBoundingBox(
			BoundingBox(
					minX, minY, minZ,
					minX + size[0]*resX, minY + size[1]*resY, minZ + size[2]*resZ));
	_labels->setResolution(resX, resY, resZ);
	for (unsigned int z = 0; z < size[2]; z++) {

		boost::shared_ptr<Image> image = boost::make_shared<Image>(size[0], size[1]);
		vigra::copyMultiArray(
				seeds.bind<2>(z),
				*image);
		_labels->add(image);
	}
}
