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

std::vector<unsigned int>
LinearConstraints::getConstraints(const std::vector<unsigned int>& variableIds) {

	std::vector<unsigned int> indices;

	for (unsigned int i = 0; i < size(); i++) {

		LinearConstraint& constraint = _linearConstraints[i];

		foreach (unsigned int v, variableIds) {

			if (constraint.getCoefficients().count(v) != 0) {

				indices.push_back(i);
				break;
			}
		}
	}

	return indices;
}
