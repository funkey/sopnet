#include "Segments.h"

Segments::~Segments() {

	clear();
}

void
Segments::clear() {

	foreach (end_tree_type* endTree, _endTrees)
		if (endTree)
			delete endTree;
	_endTrees.clear();

	foreach (continuation_tree_type* continuationTree, _continuationTrees)
		if (continuationTree)
			delete continuationTree;
	_continuationTrees.clear();

	foreach (branch_tree_type* branchTree, _branchTrees)
		if (branchTree)
			delete branchTree;
	_branchTrees.clear();

	_ends.clear();
	_continuations.clear();
	_branches.clear();
}

void
Segments::add(boost::shared_ptr<EndSegment> end) {

	unsigned int interSectionInterval = end->getInterSectionInterval();

	// resize the vector of trees to hold as many trees as we have inter-section
	// intervals
	if (_endTrees.size() < interSectionInterval + 1)
		_endTrees.resize(interSectionInterval + 1, 0);

	// create a new tree, if none exists for this inter-section interval
	if (_endTrees[interSectionInterval] == 0)
		_endTrees[interSectionInterval] = new end_tree_type(_endCoordinates);

	// insert the end segment into the tree
	_endTrees[interSectionInterval]->insert(end);

	_ends.push_back(end);
}

void
Segments::add(boost::shared_ptr<ContinuationSegment> continuation) {

	unsigned int interSectionInterval = continuation->getInterSectionInterval();

	// resize the vector of trees to hold as many trees as we have inter-section
	// intervals
	if (_continuationTrees.size() < interSectionInterval + 1)
		_continuationTrees.resize(interSectionInterval + 1, 0);

	// create a new tree, if none exists for this inter-section interval
	if (_continuationTrees[interSectionInterval] == 0)
		_continuationTrees[interSectionInterval] = new continuation_tree_type(_continuationCoordinates);

	// insert the continuation segment into the tree
	_continuationTrees[interSectionInterval]->insert(continuation);

	_continuations.push_back(continuation);
}

void
Segments::add(boost::shared_ptr<BranchSegment> branch) {

	unsigned int interSectionInterval = branch->getInterSectionInterval();

	// resize the vector of trees to hold as many trees as we have inter-section
	// intervals
	if (_branchTrees.size() < interSectionInterval + 1)
		_branchTrees.resize(interSectionInterval + 1, 0);

	// create a new tree, if none exists for this inter-section interval
	if (_branchTrees[interSectionInterval] == 0)
		_branchTrees[interSectionInterval] = new branch_tree_type(_branchCoordinates);

	// insert the branch segment into the tree
	_branchTrees[interSectionInterval]->insert(branch);

	_branches.push_back(branch);
}

void
Segments::addAll(boost::shared_ptr<Segments> segments) {

	addAll(segments->getEnds());
	addAll(segments->getContinuations());
	addAll(segments->getBranches());
}

template <typename SegmentType>
void
Segments::addAll(const std::vector<boost::shared_ptr<SegmentType> >& segments) {

	foreach (boost::shared_ptr<SegmentType> segment, segments)
		add(segment);
}

std::vector<boost::shared_ptr<EndSegment> >
Segments::getEnds(int interval) {

	// all ends for interval == -1
	if (interval < 0)
		return _ends;

	std::vector<boost::shared_ptr<EndSegment> > ends;

	// nothing
	if (interval >= _endTrees.size() || !_endTrees[interval])
		return ends;

	std::copy(_endTrees[interval]->begin(), _endTrees[interval]->end(), std::back_inserter(ends));

	return ends;
}

std::vector<boost::shared_ptr<ContinuationSegment> >
Segments::getContinuations(int interval) {

	// all continuations for interval == -1
	if (interval < 0)
		return _continuations;

	std::vector<boost::shared_ptr<ContinuationSegment> > continuations;

	// nothing
	if (interval >= _continuationTrees.size() || !_continuationTrees[interval])
		return continuations;

	std::copy(_continuationTrees[interval]->begin(), _continuationTrees[interval]->end(), std::back_inserter(continuations));

	return continuations;
}

std::vector<boost::shared_ptr<BranchSegment> >
Segments::getBranches(int interval) {

	// all branches for interval == -1
	if (interval < 0)
		return _branches;

	std::vector<boost::shared_ptr<BranchSegment> > branches;

	// nothing
	if (interval >= _branchTrees.size() || !_branchTrees[interval])
		return branches;

	std::copy(_branchTrees[interval]->begin(), _branchTrees[interval]->end(), std::back_inserter(branches));

	return branches;
}

std::vector<boost::shared_ptr<EndSegment> >
Segments::findEnds(
		boost::shared_ptr<EndSegment> reference,
		double distance) {

	unsigned int interSectionInterval = reference->getInterSectionInterval();

	std::vector<boost::shared_ptr<EndSegment> > ends;

	if (interSectionInterval >= _endTrees.size() || !_endTrees[interSectionInterval])
		return ends;

	_endTrees[interSectionInterval]->optimise();

	size_t numEnds = _endTrees[interSectionInterval]->count_within_range(reference, distance);

	ends.reserve(numEnds);

	_endTrees[interSectionInterval]->find_within_range(reference, distance, std::back_inserter(ends));

	return ends;
}

std::vector<boost::shared_ptr<ContinuationSegment> >
Segments::findContinuations(
		boost::shared_ptr<ContinuationSegment> reference,
		double distance) {

	unsigned int interSectionInterval = reference->getInterSectionInterval();

	std::vector<boost::shared_ptr<ContinuationSegment> > continuations;

	if (interSectionInterval >= _continuationTrees.size() || !_continuationTrees[interSectionInterval])
		return continuations;

	_continuationTrees[interSectionInterval]->optimise();

	size_t numContinuations = _continuationTrees[interSectionInterval]->count_within_range(reference, distance);

	continuations.reserve(numContinuations);

	_continuationTrees[interSectionInterval]->find_within_range(reference, distance, std::back_inserter(continuations));

	return continuations;
}

std::vector<boost::shared_ptr<BranchSegment> >
Segments::findBranches(
		boost::shared_ptr<BranchSegment> reference,
		double distance) {

	unsigned int interSectionInterval = reference->getInterSectionInterval();

	std::vector<boost::shared_ptr<BranchSegment> > branches;

	if (interSectionInterval >= _branchTrees.size() || !_branchTrees[interSectionInterval])
		return branches;

	_branchTrees[interSectionInterval]->optimise();

	size_t numBranchs = _branchTrees[interSectionInterval]->count_within_range(reference, distance);

	branches.reserve(numBranchs);

	_branchTrees[interSectionInterval]->find_within_range(reference, distance, std::back_inserter(branches));

	return branches;
}

unsigned int
Segments::getNumInterSectionIntervals() {

	return std::max(_endTrees.size(), std::max(_continuationTrees.size(), _branchTrees.size()));
}

unsigned int
Segments::size() {

	return _ends.size() + _continuations.size() + _branches.size();
}

