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

LinearConstraints
LinearConstraints::getConstraints(const std::vector<unsigned int>& variableIds) {

	LinearConstraints constraints;

	foreach (LinearConstraint& constraint, _linearConstraints) {

		foreach (unsigned int v, variableIds) {

			if (constraint.getCoefficients().count(v) != 0) {

				constraints.add(constraint);
				break;
			}
		}
	}

	return constraints;
}
