#include <util/foreach.h>
#include <util/ProgramOptions.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "ProblemAssembler.h"

util::ProgramOption optionMaxMitochondriaNeuronDistance(
		util::_module           = "sopnet.segments",
		util::_long_name        = "maxMitochondriaNeuronDistance",
		util::_description_text = "The maximal center distance between a mitochondria slice and an enclosing neuron slice.",
		util::_default_value    = 1000);

util::ProgramOption optionMitochondriaEnclosingThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "mitochondriaEnclosingThreshold",
		util::_description_text = "The minimal ratio (<=1) of overlap with neuron to size of mitochondria to consider the mitochondria enclosed.",
		util::_default_value    = 0.9);

util::ProgramOption optionMaxSynapseNeuronDistance(
		util::_module           = "sopnet.segments",
		util::_long_name        = "maxSynapseNeuronDistance",
		util::_description_text = "The maximal center distance between a synapse slice and an enclosing neuron slice.",
		util::_default_value    = 1000);

util::ProgramOption optionSynapseEnclosingThreshold(
		util::_module           = "sopnet.segments",
		util::_long_name        = "synapseEnclosingThreshold",
		util::_description_text = "The minimal ratio (<=1) of overlap with neuron to size of synapse to consider the synapse enclosed.",
		util::_default_value    = 0.9);

static logger::LogChannel problemassemblerlog("problemassemblerlog", "[ProblemAssembler] ");

ProblemAssembler::ProblemAssembler() :
	_allSegments(new Segments()),
	_allNeuronSegments(new Segments()),
	_allMitochondriaSegments(new Segments()),
	_allSynapseSegments(new Segments()),
	_allLinearConstraints(new LinearConstraints()),
	_problemConfiguration(new ProblemConfiguration()),
	_overlap(false, false) {

	registerInputs(_neuronSegments, "neuron segments");
	registerInputs(_neuronLinearConstraints, "neuron linear constraints");
	registerInputs(_mitochondriaSegments, "mitochondria segments");
	registerInputs(_mitochondriaLinearConstraints, "mitochondria linear constraints");
	registerInputs(_synapseSegments, "synapse segments");
	registerInputs(_synapseLinearConstraints, "synapse linear constraints");

	registerOutput(_allSegments, "segments");
	registerOutput(_allNeuronSegments, "neuron segments");
	registerOutput(_allMitochondriaSegments, "mitochondria segments");
	registerOutput(_allSynapseSegments, "synapse segments");
	registerOutput(_allLinearConstraints, "linear constraints");
	registerOutput(_problemConfiguration, "problem configuration");
}

void
ProblemAssembler::updateOutputs() {

	collectSegments();

	// make sure slices are used from both sides
	addExplanationConstraints();

	// make sure segments don't overlap
	addConsistencyConstraints();

	// make sure mitochondria are enclosed by a single neuron
	addMitochondriaConstraints();

	// make sure synapses are enclosed by a single neuron
	addSynapseConstraints();
}

void
ProblemAssembler::collectSegments() {

	LOG_DEBUG(problemassemblerlog) << "collecting segments..." << std::endl;

	_allSegments->clear(); 
	_allNeuronSegments->clear();
	_allMitochondriaSegments->clear();
	_numMitochondriaSegments = 0;
	_allSynapseSegments->clear();
	_numSynapseSegments = 0;

	if (_neuronSegments.size() > 0) {

		_allSegments->setResolution(
				_neuronSegments[0]->getResolutionX(),
				_neuronSegments[0]->getResolutionY(),
				_neuronSegments[0]->getResolutionZ());
		_allNeuronSegments->setResolution(
				_neuronSegments[0]->getResolutionX(),
				_neuronSegments[0]->getResolutionY(),
				_neuronSegments[0]->getResolutionZ());
		_allMitochondriaSegments->setResolution(
				_neuronSegments[0]->getResolutionX(),
				_neuronSegments[0]->getResolutionY(),
				_neuronSegments[0]->getResolutionZ());
		_allSynapseSegments->setResolution(
				_neuronSegments[0]->getResolutionX(),
				_neuronSegments[0]->getResolutionY(),
				_neuronSegments[0]->getResolutionZ());
	}

	foreach (boost::shared_ptr<Segments> segments, _neuronSegments) {

		_allSegments->addAll(segments);
		_allNeuronSegments->addAll(segments);
	}

	foreach (boost::shared_ptr<Segments> segments, _mitochondriaSegments) {

		_allSegments->addAll(segments);
		_allMitochondriaSegments->addAll(segments);
		_numMitochondriaSegments += segments->size();
	}

	foreach (boost::shared_ptr<Segments> segments, _synapseSegments) {

		_allSegments->addAll(segments);
		_allSynapseSegments->addAll(segments);
		_numSynapseSegments += segments->size();
	}

	LOG_DEBUG(problemassemblerlog) << "collected " << _allSegments->size() << " segments" << std::endl;
}

void
ProblemAssembler::addExplanationConstraints() {

	LOG_DEBUG(problemassemblerlog) << "adding explanation constraints..." << std::endl;

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

	LOG_DEBUG(problemassemblerlog) << "created " << _consistencyConstraints.size() << " linear constraints" << std::endl;

	foreach (const LinearConstraint& constraint, _consistencyConstraints)
		LOG_ALL(problemassemblerlog) << constraint << std::endl;

	_allLinearConstraints->addAll(_consistencyConstraints);
}

void
ProblemAssembler::addConsistencyConstraints() {

	LOG_DEBUG(problemassemblerlog) << "adding consistency constraints..." << std::endl;

	foreach (boost::shared_ptr<LinearConstraints> linearConstraints, _neuronLinearConstraints)
		mapConstraints(linearConstraints);
	foreach (boost::shared_ptr<LinearConstraints> linearConstraints, _mitochondriaLinearConstraints)
		mapConstraints(linearConstraints);

	LOG_DEBUG(problemassemblerlog) << "collected " << _allLinearConstraints->size() << " linear constraints" << std::endl;
}

void
ProblemAssembler::addMitochondriaConstraints() {

	LOG_DEBUG(problemassemblerlog) << "adding mitochondria constraints..." << std::endl;

	LOG_ALL(problemassemblerlog) << "got " << _numMitochondriaSegments << " mitochondria segments" << std::endl;

	// build a map of mitochondria segments to enclosing neuron segments
	extractMitochondriaEnclosingNeuronSegments();

	/* Make sure that for each picked mitochondria segment, one of the enclosing
	 * neuron segments gets chosen as well.
	 *
	 * [mitochondria segment] - [sum of enclosing neuron segments] <= 0
	 */

	// allocate a set of linear constraints
	_mitochondriaConstraints = LinearConstraints(_numMitochondriaSegments);

	// set the relation and value
	foreach (LinearConstraint& constraint, _mitochondriaConstraints) {

		constraint.setValue(0);
		constraint.setRelation(LessEqual);
	}

	LinearConstraints::iterator constraint = _mitochondriaConstraints.begin();
	foreach (boost::shared_ptr<Segment> mitochondriaSegment, _allMitochondriaSegments->getSegments()) {

		unsigned int mitochondriaSegmentId = mitochondriaSegment->getId();

		constraint->setCoefficient(_problemConfiguration->getVariable(mitochondriaSegmentId), 1);

		foreach (unsigned int neuronSegmentId, getMitochondriaEnclosingNeuronSegments(mitochondriaSegmentId))
			constraint->setCoefficient(_problemConfiguration->getVariable(neuronSegmentId), -1);

		constraint++;
	}

	LOG_DEBUG(problemassemblerlog) << "created " << _mitochondriaConstraints.size() << " linear constraints" << std::endl;

	_allLinearConstraints->addAll(_mitochondriaConstraints);
}

void
ProblemAssembler::addSynapseConstraints() {

	LOG_DEBUG(problemassemblerlog) << "adding synapse constraints..." << std::endl;

	LOG_ALL(problemassemblerlog) << "got " << _numSynapseSegments << " synapse segments" << std::endl;

	// build a map of synapse segments to enclosing neuron segments
	extractSynapseEnclosingNeuronSegments();

	/* Make sure that for each picked synapse segment, none of the n enclosing
	 * neuron segments gets chosen as well.
	 *
	 * [synapse segment]*n + [sum of enclosing neuron segments] <= n
	 */

	// allocate a set of linear constraints
	_synapseConstraints = LinearConstraints(_numSynapseSegments);

	LinearConstraints::iterator constraint = _synapseConstraints.begin();
	foreach (boost::shared_ptr<Segment> synapseSegment, _allSynapseSegments->getSegments()) {

		LOG_ALL(problemassemblerlog) << "processing synapse segment " << synapseSegment->getId() << std::endl;

		unsigned int synapseSegmentId = synapseSegment->getId();
		unsigned int n = getSynapseEnclosingNeuronSegments(synapseSegmentId).size();

		LOG_ALL(problemassemblerlog) << "synapse segment has " << n << " enclosing neuron segments" << std::endl;

		if (n == 0)
			continue;

		constraint->setValue(n);
		constraint->setRelation(LessEqual);

		LOG_ALL(problemassemblerlog) << "corresponding variable id is " << _problemConfiguration->getVariable(synapseSegmentId) << std::endl;

		constraint->setCoefficient(_problemConfiguration->getVariable(synapseSegmentId), n);

		LOG_ALL(problemassemblerlog) << "setting enclosing segments coefficients" << std::endl;

		foreach (unsigned int neuronSegmentId, getSynapseEnclosingNeuronSegments(synapseSegmentId)) {

			LOG_ALL(problemassemblerlog) << "setting coefficient for segment " << neuronSegmentId << std::endl;
			constraint->setCoefficient(_problemConfiguration->getVariable(neuronSegmentId), 1);
		}

		LOG_ALL(problemassemblerlog) << "finished constraint for synapse segment " << synapseSegment->getId() << std::endl;

		constraint++;
	}

	LOG_DEBUG(problemassemblerlog) << "created " << _synapseConstraints.size() << " linear constraints" << std::endl;

	_allLinearConstraints->addAll(_synapseConstraints);
}

void
ProblemAssembler::mapConstraints(boost::shared_ptr<LinearConstraints> linearConstraints) {

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
	_problemConfiguration->setVariable(end, _numSegments);

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
	_problemConfiguration->setVariable(continuation, _numSegments);

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
	_problemConfiguration->setVariable(branch, _numSegments);

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

void
ProblemAssembler::extractMitochondriaEnclosingNeuronSegments() {

	unsigned int maxMitochondriaNeuronDistance = optionMaxMitochondriaNeuronDistance;
	_enclosingMitochondriaThreshold = optionMitochondriaEnclosingThreshold;

	foreach (boost::shared_ptr<Segment> mitochondriaSegment, _allMitochondriaSegments->getSegments()) {

		unsigned int mitochondriaSegmentId = mitochondriaSegment->getId();

		boost::shared_ptr<EndSegment>          end;
		boost::shared_ptr<ContinuationSegment> continuation;
		boost::shared_ptr<BranchSegment>       branch;
		double distance;

		foreach (boost::tie(end, distance), _allNeuronSegments->findEnds(
				mitochondriaSegment->getCenter(),
				mitochondriaSegment->getInterSectionInterval(),
				maxMitochondriaNeuronDistance))
			if (encloses(end, mitochondriaSegment, _enclosingMitochondriaThreshold))
				_mitochondriaEnclosingNeuronSegments[mitochondriaSegmentId].push_back(end->getId());

		foreach (boost::tie(continuation, distance), _allNeuronSegments->findContinuations(
				mitochondriaSegment->getCenter(),
				mitochondriaSegment->getInterSectionInterval(),
				maxMitochondriaNeuronDistance))
			if (encloses(continuation, mitochondriaSegment, _enclosingMitochondriaThreshold))
				_mitochondriaEnclosingNeuronSegments[mitochondriaSegmentId].push_back(continuation->getId());

		foreach (boost::tie(branch, distance), _allNeuronSegments->findBranches(
				mitochondriaSegment->getCenter(),
				mitochondriaSegment->getInterSectionInterval(),
				maxMitochondriaNeuronDistance))
			if (encloses(branch, mitochondriaSegment, _enclosingMitochondriaThreshold))
				_mitochondriaEnclosingNeuronSegments[mitochondriaSegmentId].push_back(branch->getId());
	}
}

void
ProblemAssembler::extractSynapseEnclosingNeuronSegments() {

	unsigned int maxSynapseNeuronDistance = optionMaxSynapseNeuronDistance;
	_enclosingSynapseThreshold = optionSynapseEnclosingThreshold;

	foreach (boost::shared_ptr<Segment> synapseSegment, _allSynapseSegments->getSegments()) {

		unsigned int synapseSegmentId = synapseSegment->getId();

		boost::shared_ptr<EndSegment> end;
		boost::shared_ptr<ContinuationSegment> continuation;
		boost::shared_ptr<BranchSegment> branch;
		double distance;

		foreach (boost::tie(end, distance), _allNeuronSegments->findEnds(
				synapseSegment->getCenter(),
				synapseSegment->getInterSectionInterval(),
				maxSynapseNeuronDistance))
			if (encloses(end, synapseSegment, _enclosingSynapseThreshold))
				_synapseEnclosingNeuronSegments[synapseSegmentId].push_back(end->getId());

		foreach (boost::tie(continuation, distance), _allNeuronSegments->findContinuations(
				synapseSegment->getCenter(),
				synapseSegment->getInterSectionInterval(),
				maxSynapseNeuronDistance))
			if (encloses(continuation, synapseSegment, _enclosingSynapseThreshold))
				_synapseEnclosingNeuronSegments[synapseSegmentId].push_back(continuation->getId());

		foreach (boost::tie(branch, distance), _allNeuronSegments->findBranches(
				synapseSegment->getCenter(),
				synapseSegment->getInterSectionInterval(),
				maxSynapseNeuronDistance))
			if (encloses(branch, synapseSegment, _enclosingSynapseThreshold))
				_synapseEnclosingNeuronSegments[synapseSegmentId].push_back(branch->getId());
	}
}

bool
ProblemAssembler::encloses(boost::shared_ptr<Segment> neuronSegment, boost::shared_ptr<Segment> otherSegment, double enclosingThreshold) {

	/* We say that a neuron segment encloses a other segment, if the 
	 * slices' overlap is more than (threshold % of) the sum of sizes of the 
	 * other slices.
	 */

	// get the sum of sizes of the other slices
	unsigned int mitoSize = 0;
	foreach (boost::shared_ptr<Slice> slice, otherSegment->getSourceSlices())
		mitoSize += slice->getComponent()->getSize();
	foreach (boost::shared_ptr<Slice> slice, otherSegment->getTargetSlices())
		mitoSize += slice->getComponent()->getSize();

	// get the neuron source and target slices
	std::vector<boost::shared_ptr<Slice> > neuronSourceSlices = neuronSegment->getSourceSlices();
	std::vector<boost::shared_ptr<Slice> > neuronTargetSlices = neuronSegment->getTargetSlices();

	if (neuronSegment->getDirection() != otherSegment->getDirection())
		std::swap(neuronSourceSlices, neuronTargetSlices);

	// get the overlap
	unsigned int sourceOverlap = getOverlap(neuronSourceSlices, otherSegment->getSourceSlices());
	unsigned int targetOverlap = getOverlap(neuronTargetSlices, otherSegment->getTargetSlices());

	return (double)(sourceOverlap + targetOverlap)/mitoSize >= enclosingThreshold;
}

unsigned int
ProblemAssembler::getOverlap(
		const std::vector<boost::shared_ptr<Slice> >& slices1,
		const std::vector<boost::shared_ptr<Slice> >& slices2) {

	if (slices1.size() == 0 || slices2.size() == 0)
		return 0;

	if (slices1.size() == 1 && slices2.size() == 1)
		return _overlap(*slices1[0], *slices2[0]);

	if (slices1.size() == 2 && slices2.size() == 1)
		return _overlap(*slices1[0], *slices1[1], *slices2[0]);

	if (slices2.size() == 2 && slices1.size() == 1)
		return _overlap(*slices2[0], *slices2[1], *slices1[0]);

	// both have two slices
	return std::max(
			_overlap(*slices1[0], *slices2[0]) + _overlap(*slices1[1], *slices2[1]),
			_overlap(*slices1[0], *slices2[1]) + _overlap(*slices1[1], *slices2[0]));
}

std::vector<unsigned int>&
ProblemAssembler::getMitochondriaEnclosingNeuronSegments(unsigned int mitochondriaSegmentId) {

	return _mitochondriaEnclosingNeuronSegments[mitochondriaSegmentId];
}

std::vector<unsigned int>&
ProblemAssembler::getSynapseEnclosingNeuronSegments(unsigned int synapseSegmentId) {

	return _synapseEnclosingNeuronSegments[synapseSegmentId];
}
