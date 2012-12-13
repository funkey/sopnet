#include <boost/make_shared.hpp>

#include <util/Logger.h>
#include "MserDialog.h"

logger::LogChannel mserdialoglog("mserdialoglog", "[MserDialog] ");

MserDialog::MserDialog() :
	_deltaSlider(boost::make_shared<gui::Slider<double> >("delta", 0, 256, 1)),
	_minAreaSlider(boost::make_shared<gui::Slider<double> >("min area", 0, 100000)),
	_maxAreaSlider(boost::make_shared<gui::Slider<double> >("max area", 0, 100000, 10000)),
	_maxVariationSlider(boost::make_shared<gui::Slider<double> >("max variation", 0.0, 10.0, 10.0)),
	_minDiversitySlider(boost::make_shared<gui::Slider<double> >("min diversity", 0.0, 10.0, 0.2)),
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
