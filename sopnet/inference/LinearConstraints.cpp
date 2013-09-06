#include "LinearConstraints.h"

LinearConstraints::LinearConstraints(size_t size) {

	_linearConstraints.resize(size);
}

void
LinearConstraints::add(const LinearConstraint& linearConstraint) {

	_linearConstraints.push_back(linearConstraint);
}

void
LinearConstraints::addAll(const LinearConstraints& linearConstraints) {

	_linearConstraints.insert(_linearConstraints.end(), linearConstraints.begin(), linearConstraints.end());
}
