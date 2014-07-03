#include "SphereHoughSpace.h"
#include <vigra/multi_convolution.hxx>
#include <vigra/multi_localminmax.hxx>
#include <vigra/multi_pointoperators.hxx>
#include <vigra/transformimage.hxx>

SphereHoughSpace::SphereHoughSpace(
		unsigned int xMin, unsigned int xMax, unsigned int xStep,
		unsigned int yMin, unsigned int yMax, unsigned int yStep,
		unsigned int zMin, unsigned int zMax, unsigned int zStep,
		unsigned int radiusMin,
		unsigned int radiusMax,
		unsigned int radiusStep) :
	_xMin(xMin), _xMax(xMax), _xStep(std::max(xStep, static_cast<unsigned int>(1))),
	_yMin(yMin), _yMax(yMax), _yStep(std::max(yStep, static_cast<unsigned int>(1))),
	_zMin(zMin), _zMax(zMax), _zStep(std::max(zStep, static_cast<unsigned int>(1))),
	_radiusMin(radiusMin),
	_radiusMax(radiusMax),
	_radiusStep(radiusStep),
	_needProcessing(true) {

	_width  = (_xMax - _xMin)/_xStep;
	_height = (_yMax - _yMin)/_yStep;
	_depth  = (_zMax - _zMin)/_zStep;
	_radii  = (_radiusMax - _radiusMin)/_radiusStep;

	_houghSpace.reshape(hough_space_type::difference_type(_width, _height, _depth, _radii));
}

void
SphereHoughSpace::addBoundaryPoint(
		float x,
		float y,
		float z) {

	unsigned int discreteX = (static_cast<unsigned int>(x/_xStep) - _xMin)/_xStep;
	unsigned int discreteY = (static_cast<unsigned int>(y/_yStep) - _yMin)/_yStep;
	unsigned int discreteZ = (static_cast<unsigned int>(z/_zStep) - _zMin)/_zStep;

	for (unsigned int radius = 0; radius < _radii; radius++)
		_houghSpace(discreteX, discreteY, discreteZ, radius) += 1.0;
}

Spheres
SphereHoughSpace::getSpheres(unsigned int maxSpheres, float minVotes) {

	if (_needProcessing)
		processHoughSpace();

	typedef std::pair<float, Sphere> voted_sphere_type;
	std::vector<voted_sphere_type> sortedSpheres;

	hough_space_type::difference_type i;

	for (i[3] = 0; i[3] < _radii;  i[3]++)
	for (i[2] = 0; i[2] < _depth;  i[2]++)
	for (i[1] = 0; i[1] < _height; i[1]++)
	for (i[0] = 0; i[0] < _width;  i[0]++) {

		if (_houghSpace[i] >= minVotes) {

			double x = _xMin + i[0]*_xStep;
			double y = _yMin + i[1]*_yStep;
			double z = _zMin + i[2]*_zStep;
			double radius = _radiusMin + i[3]*_radiusStep;

			sortedSpheres.push_back(std::make_pair(_houghSpace[i], Sphere(x, y, z, radius)));
		}
	}

	std::sort(sortedSpheres.rbegin(), sortedSpheres.rend());

	Spheres spheres;
	unsigned int numSpheres = 0;
	foreach (const voted_sphere_type& votedSphere, sortedSpheres) {

		std::cout << "found sphere " << votedSphere.second << " with vote " << votedSphere.first << std::endl;

		spheres.add(votedSphere.second);
		numSpheres++;
		if (numSpheres >= maxSpheres)
			break;
	}

	return spheres;
}

boost::shared_ptr<ImageStack>
SphereHoughSpace::getHoughSpace() {

	if (_needProcessing)
		processHoughSpace();

	boost::shared_ptr<ImageStack> stack = boost::make_shared<ImageStack>();

	for (unsigned int radius = 0; radius < _radii; radius++)
	for (unsigned int z = 0; z < _depth; z++) {

		boost::shared_ptr<Image> image = boost::make_shared<Image>(_width, _height);

		vigra::copyMultiArray(
				_houghSpace.bind<3>(radius).bind<2>(z),
				*image);

		stack->add(image);
	}

	return stack;
}

void
SphereHoughSpace::processHoughSpace() {

	vigra::MultiArray<3, float> smallBlur(vigra::Shape3(_width, _height, _depth));
	vigra::Kernel1D<float> smallKernel, largeKernel;

	for (unsigned int radius = 0; radius < _radii; radius++) {

		createKernels(radius, smallKernel, largeKernel);

		vigra::MultiArrayView<3, float> houghSlice = _houghSpace.bind<3>(radius);

		// convolve each radius Hough space dimension with the small kernel
		vigra::separableConvolveMultiArray(
				houghSlice,
				smallBlur,
				smallKernel);

		// convolve each radius Hough space dimension with the large kernel
		vigra::separableConvolveMultiArray(
				houghSlice,
				houghSlice,
				largeKernel);

		// subtract small blur version from large blur version
		houghSlice -= smallBlur;

		// find maxima
		smallBlur = 0;
		vigra::localMaxima(
				houghSlice,
				smallBlur, // reuse small blur image
				vigra::LocalMinmaxOptions().allowAtBorder());

		// remove non-maxima
		vigra::combineTwoMultiArrays(
				houghSlice,
				smallBlur,
				houghSlice,
				vigra::functor::Arg1()*vigra::functor::Arg2());
	}

	_needProcessing = false;
}

void
SphereHoughSpace::createKernels(
		unsigned int radius,
		vigra::Kernel1D<float>& smallKernel,
		vigra::Kernel1D<float>& largeKernel) {

	float targetRadius = _radiusMin + radius*_radiusStep;

	// empirically estimated conversion from requested radius to std
	float smallStd = 1.6*targetRadius/5.0;
	float largeStd = 2*smallStd;

	std::cout << "target radius for index " << radius << " is " << targetRadius << std::endl;

	smallKernel.initGaussian(smallStd);
	largeKernel.initGaussian(largeStd);

	// find the max of applying large - small kernel in R^3
	double maxDiff = 0;
	for (int i = largeKernel.left(); i <= largeKernel.right(); i++)
	for (int j = i; j <= largeKernel.right(); j++)
	for (int k = j; k <= largeKernel.right(); k++) {

		double large_i = largeKernel[i];
		double small_i = (i >= smallKernel.left() && i <= smallKernel.right() ? smallKernel[i] : 0);
		double large_j = largeKernel[j];
		double small_j = (j >= smallKernel.left() && j <= smallKernel.right() ? smallKernel[j] : 0);
		double large_k = largeKernel[k];
		double small_k = (k >= smallKernel.left() && k <= smallKernel.right() ? smallKernel[k] : 0);

		double diff = large_i*large_j*large_k - small_i*small_j*small_k;

		maxDiff = std::max(maxDiff, diff);
	}

	// the 3rd root of 1.0/maxDiff
	double normalize = pow(1.0/maxDiff, 1.0/3);

	// normalize the kernels, such that the maximum of their difference is 1
	for (int i = smallKernel.left(); i <= smallKernel.right(); i++)
		smallKernel[i] *= normalize;
	for (int i = largeKernel.left(); i <= largeKernel.right(); i++)
		largeKernel[i] *= normalize;

	smallKernel.setBorderTreatment(vigra::BORDER_TREATMENT_ZEROPAD);
	largeKernel.setBorderTreatment(vigra::BORDER_TREATMENT_ZEROPAD);
}

