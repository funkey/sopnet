#include "StructuredProblemWriter.h"
#include <iostream>
#include <fstream>
#include <algorithm>

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
	write_labels(filename_labels);
	write_features(filename_features);
	write_constraints(filename_constraints);

}

void
StructuredProblemWriter::write_labels(std::string filename_labels) {
	
	// write only the labels.
	// How many variables are there?
	unsigned int maxVariable = 0;
        foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
        {
                maxVariable = std::max(maxVariable,_problemConfiguration->getVariable(segment->getId()));
        }

	// Get a vector with all ground truth segments.
	//const std::vector<boost::shared_ptr<Segment> > groundTruthSegments = _groundTruthSegments->getSegments();

	// Get a vector with all gold standard segments.
	const std::vector<boost::shared_ptr<Segment> > goldStandard = _goldStandard->getSegments();

	// Output stream
	std::ofstream labelsOutput;
        labelsOutput.open(filename_labels.c_str());

	// For every variable...
	for (unsigned int i = 0; i <= maxVariable; i++)
	{
		// ...check if the segment that corresponds to that variable is contained in the ground truth.
		unsigned int segmentId = _problemConfiguration->getSegmentId(i);
				
		bool isContained = false;
		foreach (boost::shared_ptr<Segment> s, goldStandard)
		{
                        if (s->getId() == segmentId)
			{
                                isContained = true;
			}
		}
		
		if (isContained == true)
		{
			labelsOutput << 1 << std::endl;
		}
		else
		{
			labelsOutput << 0 << std::endl;
		}
	}

	labelsOutput.close();
}

void
StructuredProblemWriter::write_features(std::string filename_features) {

	// write only the features.

	// Figure out how many variables there are.
	// After that iterate through all the segments and get the variable number for the segmend id of that segment from
	// the problem assembler. 
	// Maybe there is a better way to do this?
	unsigned int maxVariable = 0;
	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments()) 
	{
		maxVariable = std::max(maxVariable,_problemConfiguration->getVariable(segment->getId()));
	}

	std::ofstream featuresOutput;
	featuresOutput.open(filename_features.c_str());
	for (unsigned int i = 0; i <= maxVariable; i++)
	{
		const std::vector<double>& features = _features->get(_problemConfiguration->getSegmentId(i));
		for (unsigned int j = 0; j < features.size(); j++)
		{
			featuresOutput << features[j] << " ";
		}	
		featuresOutput << std::endl;
	}
	featuresOutput.close();

}

void
StructuredProblemWriter::write_constraints(std::string filenames_constraints) {

	// write only the constraints.

        std::ofstream constraintOutput;
        constraintOutput.open(filenames_constraints.c_str());
	foreach (const LinearConstraint& constraint, *_linearConstraints) {
        	constraintOutput << constraint << std::endl;
	}
        constraintOutput.close();

}
