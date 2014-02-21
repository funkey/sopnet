#ifndef SOPNET_SLICES_SLICE_EDITS_H__
#define SOPNET_SLICES_SLICE_EDITS_H__

#include <vector>
#include <boost/shared_ptr.hpp>
#include <sopnet/slices/Slice.h>

class SliceReplacements {

public:

	SliceReplacements(
			const std::vector<boost::shared_ptr<Slice> >& oldSlices,
			const std::vector<boost::shared_ptr<Slice> >& newSlices) :
		_oldSlices(oldSlices),
		_newSlices(newSlices) {}

	const std::vector<boost::shared_ptr<Slice> >& getOldSlices() {

		return _oldSlices;
	}

	const std::vector<boost::shared_ptr<Slice> >& getNewSlices() {

		return _newSlices;
	}

private:

	std::vector<boost::shared_ptr<Slice> > _oldSlices;
	std::vector<boost::shared_ptr<Slice> > _newSlices;
};

class SliceEdits {

public:

	void addReplacement(
			const std::vector<boost::shared_ptr<Slice> >& oldSlices,
			const std::vector<boost::shared_ptr<Slice> >& newSlices) {

		_replacements.push_back(SliceReplacements(oldSlices, newSlices));
	}

	const std::vector<SliceReplacements>& getReplacements() {

		return _replacements;
	}

private:

	std::vector<SliceReplacements> _replacements;
};

#endif // SOPNET_SLICES_SLICE_EDITS_H__

