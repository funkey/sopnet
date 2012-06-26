#ifndef CELLTRACKER_CELLS_H__
#define CELLTRACKER_CELLS_H__

#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>
#include "Slice.h"

class Slices : public pipeline::Data {

	typedef std::vector<boost::shared_ptr<Slice> > slices_type;

public:

	typedef slices_type::iterator       iterator;

	typedef slices_type::const_iterator const_iterator;

	/**
	 * Remove all slices.
	 */
	void clear();

	/**
	 * Add a single slice to this set of slices.
	 */
	void add(boost::shared_ptr<Slice> slice);

	/**
	 * Add a set of slices to this set of slices.
	 */
	void addAll(boost::shared_ptr<Slices> slices);

	/**
	 * Add information about conflicting slices, e.g., slices that are
	 * overlapping in space.
	 *
	 * @param conflicting A vector of slice ids that are mutually in conflict.
	 */
	template <typename Collection>
	void addConflicts(const Collection& conflicts) {

		foreach (unsigned int id, conflicts) {

			_conflicts[id].reserve(_conflicts[id].size() + conflicts.size() - 1);

			foreach (unsigned int otherId, conflicts)
				if (id != otherId)
					_conflicts[id].push_back(otherId);
		}
	}

	/**
	 * Check, whether to slices (given by their id) are in conflict.
	 */
	bool areConflicting(unsigned int id1, unsigned int id2);

	const const_iterator begin() const { return _slices.begin(); }

	iterator begin() { return _slices.begin(); }

	const const_iterator end() const { return _slices.end(); }

	iterator end() { return _slices.end(); }

	unsigned int size() { return _slices.size(); }

private:

	// the slices
	slices_type _slices;

	// map from ids of slices to all ids of conflicting slices
	std::map<unsigned int, std::vector<unsigned int> > _conflicts;
};

#endif // CELLTRACKER_CELLS_H__

