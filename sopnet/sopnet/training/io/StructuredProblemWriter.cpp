#include <iostream>
#include <fstream>
#include <algorithm>
#include <util/Logger.h>
#include "StructuredProblemWriter.h"

logger::LogChannel structuredproblemwriterlog("structuredproblemwriterlog", "[StructuredProblemWriter] ");

StructuredProblemWriter::StructuredProblemWriter()
{

	registerInput(_linearConstraints, "linear constraints");
	registerInput(_problemConfiguration, "problem configuration");
	registerInput(_features, "features");
	registerInput(_segments, "segments");
	registerInput(_goldStandard, "gold standard");

	/* The objective that was used to compute the gold standard. If set, it will 
	 * be written to an additional file, with one coeffient per variable.  
	 * Additionally, the objective value of the gold standard y' will be stored 
	 * in that file as a constant term. The coefficients and constant term can 
	 * then be used for structured learning as the application specific cost 
	 * function Δ(y',y), with Δ(y',y') = 0 and Δ(y', y) ≥ 0.
	 */
	registerInput(_goldStandardObjective, "gold standard objective", pipeline::Optional);
}

void
StructuredProblemWriter::write(std::string filename_labels,
		   	       std::string filename_features,
		   	       std::string filename_constraints,
				   std::string filename_objective) {

	updateInputs();

	// create maps from segment ids to hashes
	foreach (boost::shared_ptr<Segment> segment, _goldStandard->getSegments())
		_gsHashes[segment->getId()] = segment->hashValue();
	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
		_allHashes[segment->getId()] = segment->hashValue();

	// call write functions for the different files to write.
	writeLabels(filename_labels, filename_objective);
	writeFeatures(filename_features);
	writeConstraints(filename_constraints);

}

void
StructuredProblemWriter::writeLabels(std::string filename_labels, std::string filename_objective) {

	bool writeObjective = false;
	if (_goldStandardObjective.isSet())
		writeObjective = true;
	
	// write the labels.
	// How many variables are there?
	unsigned int maxVariable = 0;
	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
		maxVariable = std::max(maxVariable,_problemConfiguration->getVariable(segment->getId()));

	// Get a vector with all gold standard segments.
	const std::vector<boost::shared_ptr<Segment> > goldStandard = _goldStandard->getSegments();

	// Output streams
	std::ofstream labelsOutput;
	std::ofstream objectiveOutput;

	labelsOutput.open(filename_labels.c_str());
	if (writeObjective)
		objectiveOutput.open(filename_objective.c_str());

	double goldStandardObjectiveValue = 0;

	std::set<SegmentHash> segmentHashes;

	// For every variable...
	for (unsigned int i = 0; i <= maxVariable; i++) {

		// ...check if the segment that corresponds to that variable is contained in the gold standard.
		unsigned int segmentId = _problemConfiguration->getSegmentId(i);

		bool isGoldStandard;
		SegmentHash segmentHash = findSegmentHash(segmentId, isGoldStandard);

		if (isGoldStandard) {
			labelsOutput << 1 << " # " << segmentHash << std::endl;
		} 
		else {
			labelsOutput << 0 << " # " << segmentHash << std::endl;
		}

		if (writeObjective) {

			double coefficient = _goldStandardObjective->getCoefficients()[i];

			objectiveOutput << "c" << i << " " << coefficient << std::endl;

			if (isGoldStandard)
				goldStandardObjectiveValue += coefficient;
		}

		if (!segmentHashes.insert(segmentHash).second)
			LOG_ERROR(structuredproblemwriterlog)
					<< "hash collision detected: hash " << segmentHash
					<< " appears at least twice!" << std::endl;
	}

	labelsOutput.close();

	if (writeObjective) {

		objectiveOutput << "constant " << -goldStandardObjectiveValue << std::endl;
		objectiveOutput.close();
	}
}

void
StructuredProblemWriter::writeFeatures(std::string filename_features) {

	// write only the features.

	// Figure out how many variables there are.
	// After that iterate through all the segments and get the variable number for the segmend id of that segment from
	// the problem assembler. 
	// Maybe there is a better way to do this?
	unsigned int maxVariable = 0;
	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments()) {
		maxVariable = std::max(maxVariable,_problemConfiguration->getVariable(segment->getId()));
	}

	std::ofstream featuresOutput;
	featuresOutput.open(filename_features.c_str());
	for (unsigned int i = 0; i <= maxVariable; i++) {

		const std::vector<double>& features = _features->get(_problemConfiguration->getSegmentId(i));
		for (unsigned int j = 0; j < features.size(); j++) {
			featuresOutput << features[j] << " ";
		}	
		featuresOutput << std::endl;
	}
	featuresOutput.close();

}

void
StructuredProblemWriter::writeConstraints(std::string filenames_constraints) {

	// write only the constraints.

        std::ofstream constraintOutput;
        constraintOutput.open(filenames_constraints.c_str());
	foreach (const LinearConstraint& constraint, *_linearConstraints) {
        	constraintOutput << constraint << std::endl;
	}
        constraintOutput.close();

}

SegmentHash
StructuredProblemWriter::findSegmentHash(unsigned int segmentId, bool& isGoldStandard) {

	// try to find it in the gold-standard

	if (_gsHashes.count(segmentId)) {

		isGoldStandard = true;
		return _gsHashes[segmentId];
	}

	// try to find it in all segments
	if (_allHashes.count(segmentId)) {

		isGoldStandard = false;
		return _allHashes[segmentId];
	}

	UTIL_THROW_EXCEPTION(
			UsageError,
			"segment with id " << segmentId << " is not contained in given segment set");
}
