#include "Segments.h"

Segments::~Segments() {

	clear();
}

void
Segments::clear() {

	// delete all trees

	foreach (EndSegmentKdTree* endTree, _endTrees)
		if (endTree)
			delete endTree;
	_endTrees.clear();

	foreach (ContinuationSegmentKdTree* continuationTree, _continuationTrees)
		if (continuationTree)
			delete continuationTree;
	_continuationTrees.clear();

	foreach (BranchSegmentKdTree* branchTree, _branchTrees)
		if (branchTree)
			delete branchTree;
	_branchTrees.clear();

	// delete all adaptors

	foreach (EndSegmentVectorAdaptor* endVectorAdaptor, _endAdaptors)
		if (endVectorAdaptor)
			delete endVectorAdaptor;
	_endAdaptors.clear();

	foreach (ContinuationSegmentVectorAdaptor* continuationVectorAdaptor, _continuationAdaptors)
		if (continuationVectorAdaptor)
			delete continuationVectorAdaptor;
	_continuationAdaptors.clear();

	foreach (BranchSegmentVectorAdaptor* branchVectorAdaptor, _branchAdaptors)
		if (branchVectorAdaptor)
			delete branchVectorAdaptor;
	_branchAdaptors.clear();

	// clear all segments

	_ends.clear();
	_continuations.clear();
	_branches.clear();
}

void
Segments::resize(int numInterSectionInterval) {

		_ends.resize(numInterSectionInterval);
		_endAdaptors.resize(numInterSectionInterval, 0);
		_endTrees.resize(numInterSectionInterval, 0);
		_endTreeDirty.resize(numInterSectionInterval, true);

		_continuations.resize(numInterSectionInterval);
		_continuationAdaptors.resize(numInterSectionInterval, 0);
		_continuationTrees.resize(numInterSectionInterval, 0);
		_continuationTreeDirty.resize(numInterSectionInterval, true);

		_branches.resize(numInterSectionInterval);
		_branchAdaptors.resize(numInterSectionInterval, 0);
		_branchTrees.resize(numInterSectionInterval, 0);
		_branchTreeDirty.resize(numInterSectionInterval, true);
}

void
Segments::add(boost::shared_ptr<EndSegment> end) {

	unsigned int interSectionInterval = end->getInterSectionInterval();

	// resize all end inter-section interval vectors
	if (_ends.size() < interSectionInterval + 1) {

		resize(interSectionInterval + 1);
	}

	_endTreeDirty[interSectionInterval] = true;

	_ends[interSectionInterval].push_back(end);
}

void
Segments::add(boost::shared_ptr<ContinuationSegment> continuation) {

	unsigned int interSectionInterval = continuation->getInterSectionInterval();

	// resize the vector of trees to hold as many trees as we have inter-section
	// intervals
	if (_continuationTrees.size() < interSectionInterval + 1) {

		resize(interSectionInterval + 1);
	}

	_continuationTreeDirty[interSectionInterval] = true;

	_continuations[interSectionInterval].push_back(continuation);
}

void
Segments::add(boost::shared_ptr<BranchSegment> branch) {

	unsigned int interSectionInterval = branch->getInterSectionInterval();

	// resize the vector of trees to hold as many trees as we have inter-section
	// intervals
	if (_branchTrees.size() < interSectionInterval + 1) {

		resize(interSectionInterval + 1);
	}

	_branchTreeDirty[interSectionInterval] = true;

	_branches[interSectionInterval].push_back(branch);
}

void
Segments::addAll(boost::shared_ptr<Segments> segments) {

	addAll(segments->getEnds());
	addAll(segments->getContinuations());
	addAll(segments->getBranches());
}

const std::vector<boost::shared_ptr<EndSegment> >&
Segments::getEnds(int interval) {

	return _ends[interval];
}

const std::vector<boost::shared_ptr<ContinuationSegment> >&
Segments::getContinuations(int interval) {

	return _continuations[interval];
}

const std::vector<boost::shared_ptr<BranchSegment> >&
Segments::getBranches(int interval) {

	return _branches[interval];
}

std::vector<boost::shared_ptr<EndSegment> >
Segments::getEnds() {

	return get(_ends);
}

std::vector<boost::shared_ptr<ContinuationSegment> >
Segments::getContinuations() {

	return get(_continuations);
}

std::vector<boost::shared_ptr<BranchSegment> >
Segments::getBranches() {

	return get(_branches);
}

std::vector<boost::shared_ptr<EndSegment> >
Segments::findEnds(
		boost::shared_ptr<EndSegment> reference,
		double distance) {

	return find(reference->getCenter(), reference->getInterSectionInterval(), distance, _ends, _endAdaptors, _endTrees, _endTreeDirty);
}

std::vector<boost::shared_ptr<ContinuationSegment> >
Segments::findContinuations(
		boost::shared_ptr<ContinuationSegment> reference,
		double distance) {

	return find(reference->getCenter(), reference->getInterSectionInterval(), distance, _continuations, _continuationAdaptors, _continuationTrees, _continuationTreeDirty);
}

std::vector<boost::shared_ptr<BranchSegment> >
Segments::findBranches(
		boost::shared_ptr<BranchSegment> reference,
		double distance) {

	return find(reference->getCenter(), reference->getInterSectionInterval(), distance, _branches, _branchAdaptors, _branchTrees, _branchTreeDirty);
}

std::vector<boost::shared_ptr<EndSegment> >
Segments::findEnds(
		const util::point<double>& center,
		unsigned int interSectionInterval,
		double distance) {

	return find(center, interSectionInterval, distance, _ends, _endAdaptors, _endTrees, _endTreeDirty);
}

std::vector<boost::shared_ptr<ContinuationSegment> >
Segments::findContinuations(
		const util::point<double>& center,
		unsigned int interSectionInterval,
		double distance) {

	return find(center, interSectionInterval, distance, _continuations, _continuationAdaptors, _continuationTrees, _continuationTreeDirty);
}

std::vector<boost::shared_ptr<BranchSegment> >
Segments::findBranches(
		const util::point<double>& center,
		unsigned int interSectionInterval,
		double distance) {

	return find(center, interSectionInterval, distance, _branches, _branchAdaptors, _branchTrees, _branchTreeDirty);
}

unsigned int
Segments::getNumInterSectionIntervals() {

	return std::max(_ends.size(), std::max(_continuations.size(), _branches.size()));
}

unsigned int
Segments::size() {

	unsigned int size = 0;

	foreach (std::vector<boost::shared_ptr<EndSegment> > ends, _ends)
		size += ends.size();
	foreach (std::vector<boost::shared_ptr<ContinuationSegment> > continuations, _continuations)
		size += continuations.size();
	foreach (std::vector<boost::shared_ptr<BranchSegment> > branches, _branches)
		size += branches.size();

	return size;
}

