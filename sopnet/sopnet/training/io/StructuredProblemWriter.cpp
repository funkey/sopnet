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
	registerInput(_groundTruthSegments, "ground truth segments");
	registerInput(_goldStandard, "gold standard");

}

void
StructuredProblemWriter::write(std::string filename_labels,
		   	       std::string filename_features,
		   	       std::string filename_constraints ) {

	updateInputs();

	// call write functions for the different files to write.
	writeLabels(filename_labels);
	writeFeatures(filename_features);
	writeConstraints(filename_constraints);

}

void
StructuredProblemWriter::writeLabels(std::string filename_labels) {
	
	// write only the labels.
	// How many variables are there?
	unsigned int maxVariable = 0;
        foreach (boost::shared_ptr<Segment> segment, _segments->getSegments()) {
                maxVariable = std::max(maxVariable,_problemConfiguration->getVariable(segment->getId()));
        }

	// Get a vector with all ground truth segments.
	//const std::vector<boost::shared_ptr<Segment> > groundTruthSegments = _groundTruthSegments->getSegments();

	// Get a vector with all gold standard segments.
	const std::vector<boost::shared_ptr<Segment> > goldStandard = _goldStandard->getSegments();

	// Output stream
	std::ofstream labelsOutput;
        labelsOutput.open(filename_labels.c_str());

	std::set<SegmentHash> segmentHashes;

	// For every variable...
	for (unsigned int i = 0; i <= maxVariable; i++) {
		
		// ...check if the segment that corresponds to that variable is contained in the gold standard.
		unsigned int segmentId = _problemConfiguration->getSegmentId(i);

		bool isGoldStandard;
		boost::shared_ptr<Segment> segment = findSegment(segmentId, isGoldStandard);

		SegmentHash segmentHash = segment->hashValue();

		if (isGoldStandard) {
			labelsOutput << 1 << " # " << segmentHash << std::endl;
		} 
		else {
			labelsOutput << 0 << " # " << segmentHash << std::endl;
		}

		if (!segmentHashes.insert(segmentHash).second)
			LOG_ERROR(structuredproblemwriterlog)
					<< "hash collision detected: hash " << segmentHash
					<< " appears at least twice!" << std::endl;
	}

	labelsOutput.close();
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

boost::shared_ptr<Segment>
StructuredProblemWriter::findSegment(unsigned int segmentId, bool& isGoldStandard) {

	// try to find it in the gold-standard
	foreach (boost::shared_ptr<Segment> segment, _goldStandard->getSegments())
		if (segment->getId() == segmentId) {

			isGoldStandard = true;
			return segment;
		}

	// try to find it in all segments
	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
		if (segment->getId() == segmentId) {

			isGoldStandard = false;
			return segment;
		}

	UTIL_THROW_EXCEPTION(
			UsageError,
			"segment with id " << segmentId << " is not contained in given segment set");
}
