#ifndef GUI_MSER_DIALOG_H__
#define GUI_MSER_DIALOG_H__

#include <boost/shared_ptr.hpp>

#include <gui/ContainerView.h>
#include <gui/Slider.h>
#include <gui/VerticalPlacing.h>
#include <imageprocessing/MserParameters.h>
#include <imageprocessing/MserParametersCollector.h>
#include <pipeline/all.h>

class MserDialog : public pipeline::ProcessNode {

public:

	MserDialog();

	~MserDialog();

private:

	// a slider controlling the potts weight
	boost::shared_ptr<gui::Slider<double> > _deltaSlider;
	boost::shared_ptr<gui::Slider<double> > _minAreaSlider;
	boost::shared_ptr<gui::Slider<double> > _maxAreaSlider;
	boost::shared_ptr<gui::Slider<double> > _maxVariationSlider;
	boost::shared_ptr<gui::Slider<double> > _minDiversitySlider;

	// a container of gui elements to control the fields of the parameters
	// object
	boost::shared_ptr<gui::ContainerView<gui::VerticalPlacing> > _gui;

	// a collector that creates the parameters object from the output of the gui
	// elements
	boost::shared_ptr<ParametersCollector> _parametersCollector;

	signals::Slot<pipeline::Modified> _modified;
};

#endif // GUI_MSER_DIALOG_H__

