#ifndef CELLTRACKER_CELLS_H__
#define CELLTRACKER_CELLS_H__

#include <boost/shared_ptr.hpp>

#include <pipeline.h>
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

	const const_iterator begin() const { return _slices.begin(); }

	iterator begin() { return _slices.begin(); }

	const const_iterator end() const { return _slices.end(); }

	iterator end() { return _slices.end(); }

	unsigned int size() { return _slices.size(); }


private:

	slices_type _slices;
};

#endif // CELLTRACKER_CELLS_H__

