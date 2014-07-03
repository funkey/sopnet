#ifndef SOPNET_SKELETONS_SPHERE_HOUGH_SPACE_H__
#define SOPNET_SKELETONS_SPHERE_HOUGH_SPACE_H__

#include <vigra/separableconvolution.hxx>
#include <imageprocessing/ImageStack.h>
#include "Spheres.h"

class SphereHoughSpace {

public:

	/**
	 * Create a new sphere Hough space for the given intervals and 
	 * discretization. The max values are exclusive.
	 */
	SphereHoughSpace(
			unsigned int xMin, unsigned int xMax, unsigned int xStep,
			unsigned int yMin, unsigned int yMax, unsigned int yStep,
			unsigned int zMin, unsigned int zMax, unsigned int zStep,
			unsigned int radiusMin,
			unsigned int radiusMax,
			unsigned int radiusStep);

	/**
	 * Add a boundary point to vote for a sphere.
	 */
	void addBoundaryPoint(
			float x,
			float y,
			float z);

	/**
	 * Get maxSpheres most voted spheres that have at least the given number of 
	 * votes.
	 */
	Spheres getSpheres(unsigned int maxSpheres, float minVotes = 0);

	/**
	 * Get the whole Hough space as an image stack.
	 */
	boost::shared_ptr<ImageStack> getHoughSpace();

private:

	// take the boundary points and create the Hough space representation
	void processHoughSpace();

	// create kernels that produce the Hough transform for a point with the 
	// given radius
	void createKernels(
			unsigned int radius,
			vigra::Kernel1D<float>& smallKernel,
			vigra::Kernel1D<float>& largeKernel);

	typedef vigra::MultiArray<4, float> hough_space_type;

	hough_space_type  _houghSpace;

	unsigned int _xMin, _xMax, _xStep;
	unsigned int _yMin, _yMax, _yStep;
	unsigned int _zMin, _zMax, _zStep;
	unsigned int _radiusMin;
	unsigned int _radiusMax;
	unsigned int _radiusStep;

	unsigned int _width;
	unsigned int _depth;
	unsigned int _height;
	unsigned int _radii;

	bool _needProcessing;
};

#endif // SOPNET_SKELETONS_SPHERE_HOUGH_SPACE_H__

