#include "SphereHoughSpace.h"
#include "FindSpheres.h"
#include <vigra/multi_convolution.hxx>
#include <vigra/multi_distance.hxx>
#include <vigra/multi_pointoperators.hxx>
#include <vigra/multi_localminmax.hxx>
#include <vigra/functorexpression.hxx>
#include <util/Logger.h>

logger::LogChannel findsphereslog("findsphereslog", "[FindSpheres] ");

FindSpheres::FindSpheres() {

	registerInput(_neuron, "neuron");
	registerInput(_smooth, "smooth");
	registerOutput(_spheres, "spheres");
	registerOutput(_maxDistances, "distances");
}

void
FindSpheres::updateOutputs() {

	if (!_spheres)
		_spheres = new Spheres();
	else
		_spheres->clear();

	if (!_maxDistances)
		_maxDistances = new ImageStack();
	else
		_maxDistances->clear();

	const unsigned int resZ = 10;

	double minX, minY, minZ;
	double maxX, maxY, maxZ;

	getBoundingBox(
			*_neuron,
			minX, maxX,
			minY, maxY,
			minZ, maxZ);

	vigra::Shape3 size(maxX - minX, maxY - minY, maxZ - minZ);

	vigra::MultiArray<3, float> distance(size);
	vigra::MultiArray<3, float> maxima(size);
	distance = 0;
	maxima = 0;

	// prepare foreground-background picture
	foreach (boost::shared_ptr<Segment> segment, _neuron->getSegments()) {
		foreach (boost::shared_ptr<Slice> slice, segment->getSlices()) {

			util::rect<double>                     boundingBox = slice->getComponent()->getBoundingBox();
			const ConnectedComponent::bitmap_type& bitmap      = slice->getComponent()->getBitmap();
			unsigned int                           section     = slice->getSection();

			vigra::copyMultiArray(
					bitmap,
					distance.bind<2>(section - minZ).subarray(
							vigra::Shape2(boundingBox.minX - minX, boundingBox.minY - minY),
							vigra::Shape2(boundingBox.maxX - minX, boundingBox.maxY - minY)));

		}
	}

	// perform distance transform
	float pitch[3];
	pitch[0] = 1; pitch[1] = 1; pitch[2] = resZ;
	vigra::separableMultiDistSquared(
			distance,
			distance,
			false, // distance to background
			pitch);

	// smooth a little
	float scale = *_smooth;
	if (scale >= 1e-10)
		vigra::gaussianSmoothMultiArray(
				distance,
				maxima,
				scale,
				vigra::ConvolutionOptions<3>()
						.stepSize(1, 1, resZ));

	// find maxima
	vigra::localMaxima(
			maxima,
			maxima,
			vigra::LocalMinmaxOptions().allowAtBorder().markWith(-1));

	// extract spheres
	typedef vigra::CoupledIteratorType<3, float, float>::type CoupledIterator;

	CoupledIterator i = vigra::createCoupledIterator(maxima, distance);

	for (; i != i.getEndIterator(); i++) {

		if (i.get<1>() == -1) {

			vigra::Shape3 location  = i.get<0>();
			float         distance  = sqrt(i.get<2>());

			_spheres->add(
					Sphere(
							 location[0] + minX,
							 location[1] + minY,
							(location[2] + minZ)*resZ,
							 distance));
		}
	}

	LOG_DEBUG(findsphereslog) << "found " << _spheres->size() << " spheres" << std::endl;

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
	_maxDistances->setOffset(minX, minY, minZ);
	_maxDistances->setResolution(1, 1, resZ);
	for (unsigned int z = 0; z < size[2]; z++) {

		boost::shared_ptr<Image> image = boost::make_shared<Image>(size[0], size[1]);
		vigra::copyMultiArray(
				distance.bind<2>(z),
				*image);
		_maxDistances->add(image);
	}
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

