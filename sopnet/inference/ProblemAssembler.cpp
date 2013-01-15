#include <util/foreach.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "ProblemAssembler.h"

static logger::LogChannel problemassemblerlog("problemassemblerlog", "[ProblemAssembler] ");

ProblemAssembler::ProblemAssembler() {

	registerInputs(_segments, "segments");
	registerInputs(_linearConstraints, "linear constraints");

	registerOutput(_allSegments, "segments");
	registerOutput(_allLinearConstraints, "linear constraints");
	registerOutput(_problemConfiguration, "problem configuration");
}

void
ProblemAssembler::updateOutputs() {

	collectSegments();

	if (_allLinearConstraints) {

		addConsistencyConstraints();

		collectLinearConstraints();
	}
}

void
ProblemAssembler::collectSegments() {

	LOG_DEBUG(problemassemblerlog) << "collecting segments..." << std::endl;

	_allSegments->clear(); 
	foreach (boost::shared_ptr<Segments> segments, _segments)
		_allSegments->addAll(segments);

	LOG_DEBUG(problemassemblerlog) << "collected " << _allSegments->size() << " segments" << std::endl;
}

void
ProblemAssembler::addConsistencyConstraints() {

	LOG_DEBUG(problemassemblerlog) << "adding consistency constraints..." << std::endl;

	_allLinearConstraints->clear();

	/* Get a map from slice ids to slice numbers in [0, numSlices-1]. We need this
	 * map to find the correct linear constraint for each slice.
	 */
	extractSliceIdsMap();

	_numSegments = 0;

	/* Make sure that the number of accepted segments having a certain slice at
	 * the right side is equal to the number of accepted segments having this
	 * slice on the left side.
	 *
	 * For this, we add the following linear constraint for every slice:
	 *
	 * [sum of segments with slice right] - [sum of segments with slice left] = 0
	 */

	// allocate a set of linear constraints
	_consistencyConstraints = LinearConstraints(_numSlices);

	// set the relation and value
	foreach (LinearConstraint& constraint, _consistencyConstraints) {

		constraint.setValue(0);
		constraint.setRelation(Equal);
	}

	// set the coefficients
	foreach (boost::shared_ptr<EndSegment> segment, _allSegments->getEnds())
		setCoefficient(*segment);

	foreach (boost::shared_ptr<ContinuationSegment> segment, _allSegments->getContinuations())
		setCoefficient(*segment);

	foreach (boost::shared_ptr<BranchSegment> segment, _allSegments->getBranches())
		setCoefficient(*segment);

	LOG_DEBUG(problemassemblerlog) << "created " << _consistencyConstraints.size() << " consistency constraints" << std::endl;

	foreach (const LinearConstraint& constraint, _consistencyConstraints)
		LOG_ALL(problemassemblerlog) << constraint << std::endl;

	_allLinearConstraints->addAll(_consistencyConstraints);
}

void
ProblemAssembler::collectLinearConstraints() {

	LOG_DEBUG(problemassemblerlog) << "collecting linear constraints..." << std::endl;

	foreach (boost::shared_ptr<LinearConstraints> linearConstraints, _linearConstraints) {

		foreach (const LinearConstraint& linearConstraint, *linearConstraints) {

			LinearConstraint mappedConstraint;

			unsigned int id;
			double value;

			foreach(boost::tie(id, value), linearConstraint.getCoefficients())
				mappedConstraint.setCoefficient(_problemConfiguration->getVariable(id), value);

			mappedConstraint.setRelation(linearConstraint.getRelation());

			mappedConstraint.setValue(linearConstraint.getValue());

			_allLinearConstraints->add(mappedConstraint);
		}
	}

	LOG_DEBUG(problemassemblerlog) << "collected " << _allLinearConstraints->size() << " linear constraints" << std::endl;
}

void
ProblemAssembler::setCoefficient(const EndSegment& end) {

	unsigned int sliceId = end.getSlice()->getId();

	/* Pick the correct linear constraint by translating the slice id to
	 * the number of the slice in the problem.
	 */
	if (end.getDirection() == Left) // slice is on the right
		_consistencyConstraints[getSliceNum(sliceId)].setCoefficient(_numSegments,  1.0);
	else                            // slice is on the left
		_consistencyConstraints[getSliceNum(sliceId)].setCoefficient(_numSegments, -1.0);

	/* Sneakily we assigned a variable number (_numSegments) to every
	 * segment we found. Remember this mapping -- we will need it to
	 * transform the linear constraints on the segments and to
	 * reconstruct the result.
	 */
	_problemConfiguration->setVariable(end.getId(), _numSegments);

	_numSegments++;
}

void
ProblemAssembler::setCoefficient(const ContinuationSegment& continuation) {

	unsigned int sourceSliceId = continuation.getSourceSlice()->getId();
	unsigned int targetSliceId = continuation.getTargetSlice()->getId();

	/* Pick the correct linear constraint by translating the slice id to
	 * the number of the slice in the problem.
	 */
	if (continuation.getDirection() == Left) { // target is left

		_consistencyConstraints[getSliceNum(targetSliceId)].setCoefficient(_numSegments, -1.0);
		_consistencyConstraints[getSliceNum(sourceSliceId)].setCoefficient(_numSegments,  1.0);

	} else  {                                  // target is right

		_consistencyConstraints[getSliceNum(targetSliceId)].setCoefficient(_numSegments,  1.0);
		_consistencyConstraints[getSliceNum(sourceSliceId)].setCoefficient(_numSegments, -1.0);
	}

	/* Sneakily we assigned a variable number (_numSegments) to every
	 * segment we found. Remember this mapping -- we will need it to
	 * transform the linear constraints on the segments and to
	 * reconstruct the result.
	 */
	_problemConfiguration->setVariable(continuation.getId(), _numSegments);

	_numSegments++;
}

void
ProblemAssembler::setCoefficient(const BranchSegment& branch) {

	unsigned int sourceSliceId  = branch.getSourceSlice()->getId();
	unsigned int targetSlice1Id = branch.getTargetSlice1()->getId();
	unsigned int targetSlice2Id = branch.getTargetSlice2()->getId();

	/* Pick the correct linear constraint by translating the slice id to
	 * the number of the slice in the problem.
	 */
	if (branch.getDirection() == Left) { // targets are left

		_consistencyConstraints[getSliceNum(targetSlice1Id)].setCoefficient(_numSegments, -1.0);
		_consistencyConstraints[getSliceNum(targetSlice2Id)].setCoefficient(_numSegments, -1.0);
		_consistencyConstraints[getSliceNum(sourceSliceId)].setCoefficient(_numSegments,   1.0);

	} else  {                                  // target is right

		_consistencyConstraints[getSliceNum(targetSlice1Id)].setCoefficient(_numSegments,  1.0);
		_consistencyConstraints[getSliceNum(targetSlice2Id)].setCoefficient(_numSegments,  1.0);
		_consistencyConstraints[getSliceNum(sourceSliceId)].setCoefficient(_numSegments,  -1.0);
	}

	/* Sneakily we assigned a variable number (_numSegments) to every
	 * segment we found. Remember this mapping -- we will need it to
	 * transform the linear constraints on the segments and to
	 * reconstruct the result.
	 */
	_problemConfiguration->setVariable(branch.getId(), _numSegments);

	_numSegments++;
}

void
ProblemAssembler::extractSliceIdsMap() {

	_numSlices = 0;
	_sliceIdsMap.clear();

	/* Collect all slice ids and assign them uniquely to a number between 0 and
	 * the number of slices in the problem.
	 */
	foreach (boost::shared_ptr<EndSegment> segment, _allSegments->getEnds())
		addSlices(*segment);

	foreach (boost::shared_ptr<ContinuationSegment> segment, _allSegments->getContinuations())
		addSlices(*segment);

	foreach (boost::shared_ptr<BranchSegment> segment, _allSegments->getBranches())
		addSlices(*segment);
}

void
ProblemAssembler::addSlices(const EndSegment& end) {

	addId(end.getSlice()->getId());
}

void
ProblemAssembler::addSlices(const ContinuationSegment& continuation) {

	addId(continuation.getSourceSlice()->getId());
	addId(continuation.getTargetSlice()->getId());
}

void
ProblemAssembler::addSlices(const BranchSegment& branch) {

	addId(branch.getSourceSlice()->getId());
	addId(branch.getTargetSlice1()->getId());
	addId(branch.getTargetSlice2()->getId());
}

void
ProblemAssembler::addId(unsigned int id) {

	if (_sliceIdsMap.find(id) == _sliceIdsMap.end()) {

		// map does not contain prevSliceId yet
		_sliceIdsMap[id] = _numSlices;
		_numSlices++;
	}
}

unsigned int
ProblemAssembler::getSliceNum(unsigned int sliceId) {

	if (_sliceIdsMap.count(sliceId) == 0)
		LOG_ERROR(problemassemblerlog) << "unknown slice id!" << std::endl;

	return _sliceIdsMap[sliceId];
}

