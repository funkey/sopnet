#include "SopnetDialog.h"

logger::LogChannel sopnetdialoglog("sopnetdialoglog", "[SopnetDialog] ");

SopnetDialog::SopnetDialog(bool forceExplanation) :
	_forceExplanationSwitch(boost::make_shared<gui::Switch>("force slice explanation", forceExplanation)),
	_segmentationCostWeightSlider(
			boost::make_shared<gui::Slider<double> >(
					"segmentation cost weight",
					0.0,
					100.0,
					SegmentationCostFunctionParameters::optionSegmentationCostFunctionWeight)),
	_segmentationCostPottsWeightSlider(
			boost::make_shared<gui::Slider<double> >(
					"segmentation cost potts weight",
					-100,
					100,
					SegmentationCostFunctionParameters::optionSegmentationCostPottsWeight)),
	_segmentationCostPriorForegroundSlider(
			boost::make_shared<gui::Slider<double> >(
					"segmentation cost prior foreground",
					0,
					1,
					SegmentationCostFunctionParameters::optionSegmentationCostPriorForeground)),
	_priorEndSlider(
			boost::make_shared<gui::Slider<double> >(
					"prior end",
					-10000000,
					10000000.0,
					PriorCostFunctionParameters::optionPriorEnds.as<double>())),
	_priorContinuationSlider(
			boost::make_shared<gui::Slider<double> >(
					"prior continuation",
					-10000000.0,
					10000000.0,
					PriorCostFunctionParameters::optionPriorContinuations.as<double>())),
	_priorBranchSlider(
			boost::make_shared<gui::Slider<double> >(
					"prior branch",
					-10000000.0,
					10000000.0,
					PriorCostFunctionParameters::optionPriorBranches.as<double>())),
	_containerView(boost::make_shared<gui::ContainerView<gui::VerticalPlacing> >("sopnet dialog")),
	_parameterAssembler(boost::make_shared<ParameterAssembler>()) {

	registerOutput(_forceExplanationSwitch->getOutput("value"), "force explanation");
	registerOutput(_parameterAssembler->getOutput("segmentation cost parameters"), "segmentation cost parameters");
	registerOutput(_parameterAssembler->getOutput("prior cost parameters"), "prior cost parameters");
	registerOutput(_containerView->getOutput(), "painter");

	_parameterAssembler->setInput("segmentation cost weight", _segmentationCostWeightSlider->getOutput("value"));
	_parameterAssembler->setInput("segmentation cost potts weight", _segmentationCostPottsWeightSlider->getOutput("value"));
	_parameterAssembler->setInput("segmentation cost prior foreground", _segmentationCostPriorForegroundSlider->getOutput("value"));
	_parameterAssembler->setInput("prior end", _priorEndSlider->getOutput("value"));
	_parameterAssembler->setInput("prior continuation", _priorContinuationSlider->getOutput("value"));
	_parameterAssembler->setInput("prior branch", _priorBranchSlider->getOutput("value"));

	_containerView->setAlign(gui::VerticalPlacing::Left);
	_containerView->addInput(_forceExplanationSwitch->getOutput("painter"));
	_containerView->addInput(_segmentationCostWeightSlider->getOutput("painter"));
	_containerView->addInput(_segmentationCostPottsWeightSlider->getOutput("painter"));
	_containerView->addInput(_segmentationCostPriorForegroundSlider->getOutput("painter"));
	_containerView->addInput(_priorEndSlider->getOutput("painter"));
	_containerView->addInput(_priorContinuationSlider->getOutput("painter"));
	_containerView->addInput(_priorBranchSlider->getOutput("painter"));
}

SopnetDialog::ParameterAssembler::ParameterAssembler() :
	_segmentationCostFunctionParameters(new SegmentationCostFunctionParameters()),
	_priorCostFunctionParameters(new PriorCostFunctionParameters()) {

	registerInput(_segmentationCostWeight, "segmentation cost weight");
	registerInput(_segmentationCostPottsWeight, "segmentation cost potts weight");
	registerInput(_segmentationCostPriorForeground, "segmentation cost prior foreground");
	registerInput(_priorEnd, "prior end");
	registerInput(_priorContinuation, "prior continuation");
	registerInput(_priorBranch, "prior branch");

	registerOutput(_segmentationCostFunctionParameters, "segmentation cost parameters");
	registerOutput(_priorCostFunctionParameters, "prior cost parameters");
}

void
SopnetDialog::ParameterAssembler::updateOutputs() {

	LOG_ALL(sopnetdialoglog) << "updating parameters" << std::endl;

	_segmentationCostFunctionParameters->weight          = *_segmentationCostWeight;
	_segmentationCostFunctionParameters->weightPotts     = *_segmentationCostPottsWeight;
	_segmentationCostFunctionParameters->priorForeground = *_segmentationCostPriorForeground;

	_priorCostFunctionParameters->priorEnd          = *_priorEnd;
	_priorCostFunctionParameters->priorContinuation = *_priorContinuation;
	_priorCostFunctionParameters->priorBranch       = *_priorBranch;
}
