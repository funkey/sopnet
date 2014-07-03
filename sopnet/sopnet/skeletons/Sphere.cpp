#include "Sphere.h"

std::ostream&
operator<<(std::ostream& os, const Sphere& sphere) {

	os
			<< "("  << sphere.getX()
			<< ", " << sphere.getY()
			<< ", " << sphere.getZ()
			<< ": " << sphere.getRadius() << ")";

	return os;
}
