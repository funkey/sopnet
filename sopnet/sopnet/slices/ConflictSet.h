#ifndef SOPNET_SLICES_CONFLICT_SET_H__
#define SOPNET_SLICES_CONFLICT_SET_H__

#include <set>

/**
 * Collection of slice ids that are in conflict, i.e., only one of them can be 
 * chosen at the same time.
 */
class ConflictSet {

public:

	void addSlice(unsigned int sliceId) {

		_sliceIds.insert(sliceId);
	}

	void removeSlice(unsigned int sliceId) {

		std::set<unsigned int>::iterator i = _sliceIds.find(sliceId);

		if (i != _sliceIds.end())
			_sliceIds.erase(i);
	}

	void clear() {

		_sliceIds.clear();
	}

	const std::set<unsigned int>& getSlices() const {

		return _sliceIds;
	}

private:

	std::set<unsigned int> _sliceIds;
};

#endif // SOPNET_SLICES_CONFLICT_SET_H__

