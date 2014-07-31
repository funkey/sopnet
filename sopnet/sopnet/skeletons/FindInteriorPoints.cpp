#include "SphereHoughSpace.h"
#include "FindInteriorPoints.h"
#include <vigra/multi_convolution.hxx>
#include <vigra/multi_distance.hxx>
#include <vigra/multi_pointoperators.hxx>
#include <vigra/multi_localminmax.hxx>
#include <vigra/functorexpression.hxx>

FindInteriorPoints::FindInteriorPoints() {

	registerInput(_membranes, "membranes");
	registerInput(_smooth, "smooth");
	registerOutput(_spheres, "spheres");
	registerOutput(_boundaries, "boundaries");
}

void
FindInteriorPoints::updateOutputs() {

	if (!_spheres)
		_spheres = new Spheres();
	else
		_spheres->clear();

	if (!_boundaries)
		_boundaries = new ImageStack();
	else
		_boundaries->clear();

	const unsigned int minX = _membranes->getOffsetX();
	const unsigned int minY = _membranes->getOffsetY();
	const unsigned int minZ = _membranes->getOffsetZ();
	const unsigned int resX = _membranes->getResolutionX();
	const unsigned int resY = _membranes->getResolutionY();
	const unsigned int resZ = _membranes->getResolutionZ();

	unsigned int width  = _membranes->width();
	unsigned int height = _membranes->height();
	unsigned int depth  = _membranes->size();

	vigra::Shape3 size(width, height, depth);

	vigra::MultiArray<3, float> distance(size);
	vigra::MultiArray<3, float> maxima(size);
	distance = 0;
	maxima = 0;

	// prepare foreground-background picture
	for (unsigned int section = 0; section < depth; section++) {

			vigra::copyMultiArray(
					*(*_membranes)[section],
					distance.bind<2>(section));
	}

	// perform distance transform
	float pitch[3];
	pitch[0] = resX; pitch[1] = resY; pitch[2] = resZ;
	vigra::separableMultiDistSquared(
			distance,
			distance,
			true, // distance to foreground
			pitch);

	// smooth a little
	float scale = *_smooth;
	if (scale >= 1e-10)
		vigra::gaussianSmoothMultiArray(
				distance,
				distance,
				scale,
				vigra::ConvolutionOptions<3>()
						.stepSize(resX, resY, resZ));

	// find maxima
	vigra::localMaxima(
			distance,
			maxima,
			vigra::LocalMinmaxOptions().allowAtBorder());

	// extract spheres
	typedef vigra::CoupledIteratorType<3, float, float>::type CoupledIterator;

	CoupledIterator i = vigra::createCoupledIterator(maxima, distance);

	for (; i != i.getEndIterator(); i++) {

		if (i.get<1>() > 0) {

			vigra::Shape3 location  = i.get<0>();
			float         distance  = sqrt(i.get<2>());

			_spheres->add(
					Sphere(
							(location[0] + minX)*resX,
							(location[1] + minY)*resY,
							(location[2] + minZ)*resZ,
							 distance));
		}
	}

	// normalize distance transform image
	vigra::FindMinMax<float> findMinMax;
	vigra::inspectMultiArray(
			distance,
			findMinMax);
	vigra::transformMultiArray(
			distance,
			distance,
			vigra::functor::Param(1.0) - vigra::functor::Arg1()/vigra::functor::Param(findMinMax.max));

	// provide distances as output
	_boundaries->setOffset(minX, minY, minZ);
	_boundaries->setResolution(resX, resY, resZ);
	for (unsigned int z = 0; z < size[2]; z++) {

		boost::shared_ptr<Image> image = boost::make_shared<Image>(size[0], size[1]);
		vigra::copyMultiArray(
				distance.bind<2>(z),
				*image);
		_boundaries->add(image);
	}
}

