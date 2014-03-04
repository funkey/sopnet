#ifndef SOPNET_STRUCTURED_PROBLEM_WRITER_H__
#define SOPNET_STRUCTURED_PROBLEM_WRITER_H__

#include <pipeline/all.h>
#include <sopnet/inference/LinearConstraints.h>
#include <sopnet/inference/ProblemConfiguration.h>
#include <sopnet/features/Features.h>
#include <sopnet/segments/Segments.h>

class StructuredProblemWriter : public pipeline::SimpleProcessNode<> {

public:

	StructuredProblemWriter();

	void write(std::string filename_labels,
		   std::string filename_features,
		   std::string filename_constraints);

private:

	void updateOutputs() {}

	void write_labels(std::string filename_labels);
	void write_features(std::string filename_features);
	void write_constraints(std::string filename_constraints);
 
	pipeline::Input<LinearConstraints> _linearConstraints;
	pipeline::Input<ProblemConfiguration> _problemConfiguration;
	pipeline::Input<Features> _features;
	pipeline::Input<Segments> _segments;
	pipeline::Input<Segments> _groundTruthSegments;
	pipeline::Input<Segments> _goldStandard;
};

#endif // SOPNET_STRUCTURED_PROBLEM_WRITER_H__

