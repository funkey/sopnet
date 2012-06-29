#ifndef SOPNET_GUI_SOPNET_DIALOG_H__
#define SOPNET_GUI_SOPNET_DIALOG_H__

#include <pipeline/all.h>
#include <gui/ContainerView.h>
#include <gui/VerticalPlacing.h>
#include <gui/Slider.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>

class SopnetDialog : public pipeline::ProcessNode {

public:

	SopnetDialog();

private:

	class ParameterAssembler : public pipeline::SimpleProcessNode {

	public:

		ParameterAssembler();

	private:

		void updateOutputs();

		pipeline::Input<double> _segmentationCostWeight;

		pipeline::Input<double> _segmentationCostPottsWeight;

		pipeline::Output<SegmentationCostFunctionParameters> _segmentationCostFunctionParameters;
	};

	boost::shared_ptr<gui::Slider> _segmentationCostWeightSlider;

	boost::shared_ptr<gui::Slider> _segmentationCostPottsWeightSlider;

	boost::shared_ptr<gui::ContainerView<gui::VerticalPlacing> > _containerView;

	boost::shared_ptr<ParameterAssembler> _parameterAssembler;
};

#endif // SOPNET_GUI_SOPNET_DIALOG_H__

