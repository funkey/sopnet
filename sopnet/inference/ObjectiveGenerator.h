#ifndef CELLTRACKER_OBJECTIVE_GENERATOR_H__
#define CELLTRACKER_OBJECTIVE_GENERATOR_H__

#include <pipeline/all.h>
#include <inference/LinearObjective.h>
#include <sopnet/segments/Segments.h>

class ObjectiveGenerator : public pipeline::SimpleProcessNode {

public:

	ObjectiveGenerator();

private:

	void updateOutputs();

	void updateObjective();

	template <typename SegmentType>
	void setCosts(const SegmentType& segment);

	pipeline::Input<Segments> _segments;

	pipeline::Input<boost::function<double(const Segment&)> > _costFunction;

	pipeline::Input<std::map<unsigned int, unsigned int> > _segmentIdsToVariables;

	pipeline::Output<LinearObjective> _objective;
};

#endif // CELLTRACKER_OBJECTIVE_GENERATOR_H__

