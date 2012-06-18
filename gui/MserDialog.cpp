#include <boost/make_shared.hpp>

#include <util/Logger.h>
#include "MserDialog.h"

logger::LogChannel mserdialoglog("mserdialoglog", "[MserDialog] ");

MserDialog::ParametersCollector::ParametersCollector() {

	LOG_ALL(mserdialoglog) << "[ParametersCollector] create new parameters collector" << std::endl;

	registerInput(_delta, "delta");
	registerInput(_minArea, "min area");
	registerInput(_maxArea, "max area");
	registerInput(_maxVariation, "max variation");
	registerInput(_minDiversity, "min diversity");
	registerInput(_darkToBright, "dark to bright");
	registerInput(_brightToDark, "bright to dark");
	registerOutput(_parameters, "mser parameters");
}

MserDialog::ParametersCollector::~ParametersCollector() {

	LOG_ALL(mserdialoglog) << "[ParametersCollector] destructed" << std::endl;
}

void
MserDialog::ParametersCollector::updateOutputs() {

	LOG_ALL(mserdialoglog) << "[ParametersCollector] updating outputs" << std::endl;

	_parameters->delta        = (int)*_delta;
	_parameters->minArea      = (int)*_minArea;
	_parameters->maxArea      = (int)*_maxArea;
	_parameters->maxVariation = *_maxVariation;
	_parameters->minDiversity = *_minDiversity;
	_parameters->darkToBright = true;//*_darkToBright;
	_parameters->brightToDark = false;//*_brightToDark;
}

MserDialog::MserDialog() :
	_deltaSlider(boost::make_shared<gui::Slider>("delta", 0, 256, 1)),
	_minAreaSlider(boost::make_shared<gui::Slider>("min area", 0, 100000)),
	_maxAreaSlider(boost::make_shared<gui::Slider>("max area", 0, 100000, 10000)),
	_maxVariationSlider(boost::make_shared<gui::Slider>("max variation", 0.0, 10.0, 10.0)),
	_minDiversitySlider(boost::make_shared<gui::Slider>("min diversity", 0.0, 10.0, 0.2)),
	_gui(boost::make_shared<gui::ContainerView<gui::VerticalPlacing> >()),
	_parametersCollector(boost::make_shared<ParametersCollector>()) {

	LOG_ALL(mserdialoglog) << "create new mser dialog" << std::endl;

	// establish internal pipeline connections
	_parametersCollector->setInput("delta",         _deltaSlider->getOutput("value"));
	_parametersCollector->setInput("min area",      _minAreaSlider->getOutput("value"));
	_parametersCollector->setInput("max area",      _maxAreaSlider->getOutput("value"));
	_parametersCollector->setInput("max variation", _maxVariationSlider->getOutput("value"));
	_parametersCollector->setInput("min diversity", _minDiversitySlider->getOutput("value"));

	_gui->setAlign(gui::VerticalPlacing::Left);
	_gui->addInput(_deltaSlider->getOutput("painter"));
	_gui->addInput(_minAreaSlider->getOutput("painter"));
	_gui->addInput(_maxAreaSlider->getOutput("painter"));
	_gui->addInput(_maxVariationSlider->getOutput("painter"));
	_gui->addInput(_minDiversitySlider->getOutput("painter"));

	registerOutput(_parametersCollector->getOutput(), "mser parameters");
	registerOutput(_gui->getOutput(), "painter");
}

MserDialog::~MserDialog() {

	LOG_ALL(mserdialoglog) << "destructed" << std::endl;
}
