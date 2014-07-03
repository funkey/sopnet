#include "FindSpheres.h"

FindSpheres::FindSpheres() {

	registerInput(_neuron, "neuron");
	registerOutput(_spheres, "spheres");
}

void
FindSpheres::updateOutputs() {

	if (!_spheres)
		_spheres = new Spheres();

	_spheres->add(Sphere(0, 0, 0, 100));
}
