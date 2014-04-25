#ifndef SOPNET_MINIMAL_IMPACT_TED_WRITER_H__
#define SOPNET_MINIMAL_IMPACT_TED_WRITER_H__

#include <pipeline/all.h>
#include <sopnet/inference/LinearConstraints.h>
#include <sopnet/inference/ProblemConfiguration.h>
#include <sopnet/segments/Segments.h>

class MinimalImpactTEDWriter : public pipeline::SimpleProcessNode<> {

public:

	MinimalImpactTEDWriter();

	void write(std::string filename);

private:

	void updateOutputs() {}
 
	pipeline::Input<Segments> _goldStandard;
};

#endif // SOPNET_MINIMAL_IMPACT_TED_WRITER_H___

