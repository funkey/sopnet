#ifndef SOPNET_GUI_SOPNET_DIALOG_H__
#define SOPNET_GUI_SOPNET_DIALOG_H__

#include <pipeline/all.h>
#include <gui/ContainerView.h>
#include <gui/VerticalPlacing.h>
#include <gui/Slider.h>
#include <gui/Switch.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>
#include <sopnet/inference/PriorCostFunctionParameters.h>

class SopnetDialog : public pipeline::ProcessNode {

public:

	SopnetDialog();

private:

	class ParameterAssembler : public pipeline::SimpleProcessNode<> {

	public:

		ParameterAssembler();

	private:

		void updateOutputs();

		pipeline::Input<double> _segmentationCostWeight;

		pipeline::Input<double> _segmentationCostPottsWeight;

		pipeline::Input<double> _segmentationCostPriorForeground;

		pipeline::Output<SegmentationCostFunctionParameters> _segmentationCostFunctionParameters;

		pipeline::Input<double> _priorEnd;

		pipeline::Input<double> _priorContinuation;

		pipeline::Input<double> _priorBranch;

		pipeline::Output<PriorCostFunctionParameters> _priorCostFunctionParameters;
	};

	// general

	boost::shared_ptr<gui::Switch> _forceExplanationSwitch;

	// segmentation cost parameters

	boost::shared_ptr<gui::Slider> _segmentationCostWeightSlider;

	boost::shared_ptr<gui::Slider> _segmentationCostPottsWeightSlider;

	boost::shared_ptr<gui::Slider> _segmentationCostPriorForegroundSlider;

	// prior cost parameters

	boost::shared_ptr<gui::Slider> _priorEndSlider;

	boost::shared_ptr<gui::Slider> _priorContinuationSlider;

	boost::shared_ptr<gui::Slider> _priorBranchSlider;

	// collection and gui

	boost::shared_ptr<gui::ContainerView<gui::VerticalPlacing> > _containerView;

	boost::shared_ptr<ParameterAssembler> _parameterAssembler;
};

#endif // SOPNET_GUI_SOPNET_DIALOG_H__

