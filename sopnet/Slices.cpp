#include "Slices.h"

void
Slices::clear() {

	_slices.clear();
}

void
Slices::add(boost::shared_ptr<Slice> slice) {

	_slices.push_back(slice);
}

void
Slices::addAll(boost::shared_ptr<Slices> slices) {

	_slices.insert(_slices.end(), slices->begin(), slices->end());
}
