#ifndef SOPNET_SLICES_CONFLICT_SETS_H__
#define SOPNET_SLICES_CONFLICT_SETS_H__

#include <vector>
#include <pipeline/Data.h>

#include "ConflictSet.h"

/**
 * Collection of slice conflict sets.
 */
class ConflictSets : public pipeline::Data {

public:

	typedef std::vector<ConflictSet>::iterator       iterator;
	typedef std::vector<ConflictSet>::const_iterator const_iterator;

	void add(const ConflictSet& conflictSet) {

		_conflictSets.push_back(conflictSet);
	}

	std::vector<ConflictSet>::iterator begin() {

		return _conflictSets.begin();
	}

	std::vector<ConflictSet>::const_iterator begin() const {

		return _conflictSets.begin();
	}

	std::vector<ConflictSet>::iterator end() {

		return _conflictSets.end();
	}

	std::vector<ConflictSet>::const_iterator end() const {

		return _conflictSets.end();
	}

	void clear() {

		_conflictSets.clear();
	}

private:

	std::vector<ConflictSet> _conflictSets;
};

#endif // SOPNET_SLICES_CONFLICT_SETS_H__

