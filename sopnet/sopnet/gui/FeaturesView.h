#ifndef SOPNET_GUI_FEATURES_VIEW_H__
#define SOPNET_GUI_FEATURES_VIEW_H__

#include <pipeline/all.h>
#include <inference/LinearObjective.h>
#include <sopnet/gui/FeaturesPainter.h>
#include <sopnet/features/Features.h>
#include <sopnet/inference/ProblemConfiguration.h>
#include <sopnet/segments/Segments.h>

class FeaturesView : public pipeline::SimpleProcessNode<> {

public:

	FeaturesView();

private:

	void updateOutputs();

	pipeline::Input<Segments>             _segments;
	pipeline::Input<Features>             _features;
	pipeline::Input<ProblemConfiguration> _problemConfiguration;
	pipeline::Input<LinearObjective>      _objective;

	pipeline::Output<FeaturesPainter> _painter;
};

#endif // SOPNET_GUI_FEATURES_VIEW_H__

