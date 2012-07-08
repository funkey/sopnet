#include "SopnetDialog.h"

logger::LogChannel sopnetdialoglog("sopnetdialoglog", "[SopnetDialog] ");

SopnetDialog::SopnetDialog() :
	_forceExplanationSwitch(boost::make_shared<gui::Switch>("force slice explanation")),
	_segmentationCostWeightSlider(boost::make_shared<gui::Slider>("segmentation cost weight", 0.0, 100.0, 1.0)),
	_segmentationCostPottsWeightSlider(boost::make_shared<gui::Slider>("segmentation cost potts weight", -100, 100, 1.0)),
	_segmentationCostPriorForegroundSlider(boost::make_shared<gui::Slider>("segmentation cost prior foreground", 0, 1, 0.5)),
	_containerView(boost::make_shared<gui::ContainerView<gui::VerticalPlacing> >()),
	_parameterAssembler(boost::make_shared<ParameterAssembler>()) {

	registerOutput(_containerView->getOutput(), "painter");
	registerOutput(_parameterAssembler->getOutput("segmentation cost parameters"), "segmentation cost parameters");
	registerOutput(_forceExplanationSwitch->getOutput("value"), "force explanation");

	_parameterAssembler->setInput("segmentation cost weight", _segmentationCostWeightSlider->getOutput("value"));
	_parameterAssembler->setInput("segmentation cost potts weight", _segmentationCostPottsWeightSlider->getOutput("value"));
	_parameterAssembler->setInput("segmentation cost prior foreground", _segmentationCostPriorForegroundSlider->getOutput("value"));

	_containerView->addInput(_forceExplanationSwitch->getOutput("painter"));
	_containerView->addInput(_segmentationCostWeightSlider->getOutput("painter"));
	_containerView->addInput(_segmentationCostPottsWeightSlider->getOutput("painter"));
	_containerView->addInput(_segmentationCostPriorForegroundSlider->getOutput("painter"));
}

SopnetDialog::ParameterAssembler::ParameterAssembler() {

	registerInput(_segmentationCostWeight, "segmentation cost weight");
	registerInput(_segmentationCostPottsWeight, "segmentation cost potts weight");
	registerInput(_segmentationCostPriorForeground, "segmentation cost prior foreground");

	registerOutput(_segmentationCostFunctionParameters, "segmentation cost parameters");
}

void
SopnetDialog::ParameterAssembler::updateOutputs() {

	LOG_ALL(sopnetdialoglog) << "updating parameters" << std::endl;

	_segmentationCostFunctionParameters->weight          = *_segmentationCostWeight;
	_segmentationCostFunctionParameters->weightPotts     = *_segmentationCostPottsWeight;
	_segmentationCostFunctionParameters->priorForeground = *_segmentationCostPriorForeground;
}
