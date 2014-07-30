#include <util/foreach.h>
#include <gui/OpenGl.h>
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

	GLUquadric* quadric = gluNewQuadric();

	if (quadric) {

		foreach (const Sphere& sphere, *_spheres) {

			glPushMatrix();
			glTranslated(sphere.getX(), sphere.getY(), sphere.getZ());
			gluSphere(quadric, sphere.getRadius(), 10, 10);
			glPopMatrix();

			util::rect<double> boundingBox(
					sphere.getX() - sphere.getRadius(),
					sphere.getY() - sphere.getRadius(),
					sphere.getX() + sphere.getRadius(),
					sphere.getY() + sphere.getRadius());

			if (_size.area() == 0)
				_size = boundingBox;
			else
				_size.fit(boundingBox);
		}

		gluDeleteQuadric(quadric);
	}

	stopRecording();
}
