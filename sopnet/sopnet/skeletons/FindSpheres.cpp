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

	SphereHoughSpace houghSpace(
			0, 100, 1,
			0, 100, 1,
			0, 100, 1,
			10, 40, 10);

	houghSpace.addBoundaryPoint(50, 50, 0);
	houghSpace.addBoundaryPoint(25, 25, 50);

	*_spheres = houghSpace.getSpheres(2);
	_houghSpace = houghSpace.getHoughSpace();
}
