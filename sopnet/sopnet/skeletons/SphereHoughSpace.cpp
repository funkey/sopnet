#include "SphereHoughSpace.h"
#include <vigra/functorexpression.hxx>
#include <vigra/multi_convolution.hxx>
#include <vigra/multi_fft.hxx>
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

	// enlarge the spatial dimensions, such that there are no points withing the 
	// maximal radius if the borders are reflected (which is the case for the 
	// convolveFFT call)
	_width  = (_xMax - _xMin + 2*_radiusMax)/_xStep;
	_height = (_yMax - _yMin + 2*_radiusMax)/_yStep;
	_depth  = (_zMax - _zMin + 2*_radiusMax)/_zStep;
	_radii  = (_radiusMax - _radiusMin)/_radiusStep;

	_houghSpace.reshape(hough_space_type::difference_type(_width, _height, _depth, _radii));
}

void
SphereHoughSpace::addBoundaryPoint(
		float x,
		float y,
		float z) {

	// account for the radius padding
	x += _radiusMax;
	y += _radiusMax;
	z += _radiusMax;

	std::cout << "corrected by padding " << x << ", " << y << ", " << z << std::endl;

	unsigned int discreteX = (x - _xMin)/_xStep;
	unsigned int discreteY = (y - _yMin)/_yStep;
	unsigned int discreteZ = (z - _zMin)/_zStep;

	std::cout << "in Hough: " << discreteX << ", " << discreteY << ", " << discreteZ << std::endl;

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

			// account for the padding
			double x = _xMin + i[0]*_xStep - _radiusMax;
			double y = _yMin + i[1]*_yStep - _radiusMax;
			double z = _zMin + i[2]*_zStep - _radiusMax;
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

	vigra::MultiArray<3, float> houghKernel;

	for (unsigned int radius = 0; radius < _radii; radius++) {

		createKernel(radius, houghKernel);

		vigra::MultiArrayView<3, float> houghSlice = _houghSpace.bind<3>(radius);

		// convolve each radius Hough space dimension with the Hough kernel
		vigra::convolveFFT(
				houghSlice,
				houghKernel,
				houghSlice);

		vigra::MultiArray<3, float> maxima(houghSlice.shape());

		// find maxima
		maxima = 0;
		vigra::localMaxima(
				houghSlice,
				maxima,
				vigra::LocalMinmaxOptions().allowAtBorder().allowPlateaus());

		// remove non-maxima
		vigra::combineTwoMultiArrays(
				houghSlice,
				maxima,
				houghSlice,
				vigra::functor::Arg1()*vigra::functor::Arg2());
	}

	_needProcessing = false;
}

void
SphereHoughSpace::createKernel(
		unsigned int radiusIndex,
		vigra::MultiArray<3, float>& houghKernel) {

	// the requested radius
	unsigned int radius = _radiusMin + radiusIndex*_radiusStep;

	std::cout << "create kernel for radius " << radius << std::endl;

	// pad the sphere by at least one pixel to the inside and outside
	unsigned int padding = std::max(1.0, 0.1*radius);
	unsigned int outerDistance2 = (radius + padding)*(radius + padding);
	unsigned int innerDistance2 = (radius - padding)*(radius - padding);

	std::cout << "padding is " << padding << std::endl;
	std::cout << "outer distance^2 is " << outerDistance2 << std::endl;
	std::cout << "inner distance^2 is " << innerDistance2 << std::endl;

	// make the kernel slightly larger (3*padding instead of 1*padding), because 
	// we smooth it later
	unsigned int outerX = (radius + 3*padding)/_xStep + 1;
	unsigned int outerY = (radius + 3*padding)/_yStep + 1;
	unsigned int outerZ = (radius + 3*padding)/_zStep + 1;

	// create an odd sized kernel
	vigra::Shape3 size(
			2*outerX + 1,
			2*outerY + 1,
			2*outerZ + 1);
	vigra::Shape3 center(
			size[0]/2,
			size[1]/2,
			size[2]/2
	);

	houghKernel.reshape(size);

	// draw a ring
	vigra::Shape3 i;
	for (i[2] = 0; i[2] != size[2]; i[2]++)
	for (i[1] = 0; i[1] != size[1]; i[1]++)
	for (i[0] = 0; i[0] != size[0]; i[0]++) {

		unsigned int dx = (i[0] - center[0])*_xStep;
		unsigned int dy = (i[1] - center[1])*_yStep;
		unsigned int dz = (i[2] - center[2])*_zStep;

		unsigned int distance2 = dx*dx + dy*dy + dz*dz;

		if (distance2 <= outerDistance2 && distance2 >= innerDistance2)
			houghKernel[i] = 1.0;
		else
			houghKernel[i] = 0.0;
	}

	float scale = std::max(0.01f, static_cast<float>(padding));
	vigra::gaussianSmoothMultiArray(
			houghKernel,
			houghKernel,
			scale,
			vigra::ConvolutionOptions<3>()
					.stepSize(_xStep, _yStep, _zStep));
}

