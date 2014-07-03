#include "SpheresPainter.h"

void
SpheresPainter::setSpheres(boost::shared_ptr<Spheres> spheres) {

	if (!spheres)
		return;

	_spheres = spheres;
	_size = util::rect<double>(0, 0, 0, 0);

	updateRecording();

	setSize(_size);
}

void
SpheresPainter::updateRecording() {

	// make sure OpenGl operations are save
	gui::OpenGl::Guard guard;

	startRecording();



	stopRecording();
}
