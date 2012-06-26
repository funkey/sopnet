#include "Slices.h"

void
Slices::clear() {

	_slices.clear();
	_conflicts.clear();
}

void
Slices::add(boost::shared_ptr<Slice> slice) {

	_slices.push_back(slice);
}

void
Slices::addAll(boost::shared_ptr<Slices> slices) {

	_slices.insert(_slices.end(), slices->begin(), slices->end());
}

bool
Slices::areConflicting(unsigned int id1, unsigned int id2) {

	// If we don't have any information about slice id1,
	// we assume that there is no conflict.
	if (!_conflicts.count(id1))
		return false;

	foreach (unsigned int conflictId, _conflicts[id1])
		if (conflictId == id2)
			return true;

	return false;
}
