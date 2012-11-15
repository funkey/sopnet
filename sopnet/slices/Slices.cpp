#include "Slices.h"

Slices::Slices() :
	_kdTree(0),
	_adaptor(0),
	_kdTreeDirty(true) {}

Slices::Slices(const Slices& other) :
	_slices(other._slices),
	_conflicts(other._conflicts),
	_kdTree(0),
	_adaptor(0),
	_kdTreeDirty(true) {}

Slices&
Slices::operator=(const Slices& other) {

	if (_kdTree)
		delete _kdTree;

	if (_adaptor)
		delete _adaptor;

	_kdTree = 0;
	_adaptor = 0;
	_kdTreeDirty = true;

	_slices = other._slices;
	_conflicts = other._conflicts;

	return *this;
}

Slices::~Slices() {

	if (_kdTree)
		delete _kdTree;

	if (_adaptor)
		delete _adaptor;
}

void
Slices::clear() {

	_slices.clear();
	_conflicts.clear();
}

void
Slices::add(boost::shared_ptr<Slice> slice) {

	_slices.push_back(slice);

	_kdTreeDirty = true;
}

void
Slices::addAll(const Slices& slices) {

	_slices.insert(_slices.end(), slices.begin(), slices.end());

	_kdTreeDirty = true;
}

void
Slices::remove(boost::shared_ptr<Slice> slice) {

	for (int i = 0; i < _slices.size(); i++)
		if (_slices[i] == slice) {

			_slices.erase(_slices.begin() + i);
			return;
		}

	return;
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

std::vector<boost::shared_ptr<Slice> >
Slices::find(const util::point<double>& center, double distance) {

	if (_kdTreeDirty) {

		delete _adaptor;
		delete _kdTree;

		_adaptor = 0;
		_kdTree = 0;
	}

	// create kd-tree, if it does not exist
	if (!_kdTree) {

		// create slice vector adaptor
		_adaptor = new SliceVectorAdaptor(_slices);

		// create the tree
		_kdTree = new SliceKdTree(2, *_adaptor, nanoflann::KDTreeSingleIndexAdaptorParams(10));

		// create index
		_kdTree->buildIndex();
	}

	// find close indices
	std::vector<std::pair<size_t, double> > results;

	double query[2];
	query[0] = center.x;
	query[1] = center.y;

	nanoflann::SearchParams params;

	_kdTree->radiusSearch(&query[0], distance, results, params);

	// fill result vector
	size_t index;
	double dist;

	std::vector<boost::shared_ptr<Slice> > found;

	foreach (boost::tie(index, dist), results)
		found.push_back(_slices[index]);

	return found;
}
