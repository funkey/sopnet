#ifndef SOPNET_GUI_FEATURES_VIEW_H__
#define SOPNET_GUI_FEATURES_VIEW_H__

#include <pipeline/all.h>
#include <gui/TextPainter.h>
#include <sopnet/features/Features.h>
#include <sopnet/segments/Segments.h>

class FeaturesView : public pipeline::SimpleProcessNode<> {

public:

	FeaturesView();

private:

	void updateOutputs();

	void appendFeatures(std::stringstream& stream, unsigned int segmentId);

	pipeline::Input<Segments> _segments;
	pipeline::Input<Features> _features;

	pipeline::Output<gui::TextPainter> _painter;
};

#endif // SOPNET_GUI_FEATURES_VIEW_H__

