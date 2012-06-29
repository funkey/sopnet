#include "SopnetDialog.h"

logger::LogChannel sopnetdialoglog("sopnetdialoglog", "[SopnetDialog] ");

SopnetDialog::SopnetDialog() :
	_segmentationCostWeightSlider(boost::make_shared<gui::Slider>("segmentation cost weight", 0.0, 100.0)),
	_segmentationCostPottsWeightSlider(boost::make_shared<gui::Slider>("segmentation cost potts weight", -100, 100)),
	_containerView(boost::make_shared<gui::ContainerView<gui::VerticalPlacing> >()),
	_parameterAssembler(boost::make_shared<ParameterAssembler>()) {

	registerOutput(_containerView->getOutput(), "painter");
	registerOutput(_parameterAssembler->getOutput("segmentation cost parameters"), "segmentation cost parameters");

	_parameterAssembler->setInput("segmentation cost weight", _segmentationCostWeightSlider->getOutput("value"));
	_parameterAssembler->setInput("segmentation cost potts weight", _segmentationCostPottsWeightSlider->getOutput("value"));

	_containerView->addInput(_segmentationCostWeightSlider->getOutput("painter"));
	_containerView->addInput(_segmentationCostPottsWeightSlider->getOutput("painter"));
}

SopnetDialog::ParameterAssembler::ParameterAssembler() {

	registerInput(_segmentationCostWeight, "segmentation cost weight");
	registerInput(_segmentationCostPottsWeight, "segmentation cost potts weight");

	registerOutput(_segmentationCostFunctionParameters, "segmentation cost parameters");
}

void
SopnetDialog::ParameterAssembler::updateOutputs() {

	LOG_ALL(sopnetdialoglog) << "updating parameters" << std::endl;

	_segmentationCostFunctionParameters->weight      = *_segmentationCostWeight;
	_segmentationCostFunctionParameters->weightPotts = *_segmentationCostPottsWeight;
}
