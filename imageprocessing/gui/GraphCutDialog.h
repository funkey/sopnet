#ifndef GUI_GRAPH_CUT_DIALOG_H__
#define GUI_GRAPH_CUT_DIALOG_H__

#include <boost/shared_ptr.hpp>

#include <gui/ContainerView.h>
#include <gui/Slider.h>
#include <gui/Switch.h>
#include <gui/VerticalPlacing.h>
#include <imageprocessing/GraphCutParameters.h>
#include <pipeline/all.h>

class GraphCutDialog : public pipeline::ProcessNode {

public:

	GraphCutDialog();

	~GraphCutDialog();

private:

	/**
	 * A collector that bundles the outputs of the gui elements into a single
	 * GraphCutParameters object.
	 */
	class ParametersCollector : public pipeline::SimpleProcessNode<> {

		public:

			ParametersCollector();

			~ParametersCollector();

		private:

			void updateOutputs();

			// the foreground prior
			pipeline::Input<double> _foregroundPrior;

			// the potts-term weight
			pipeline::Input<double> _pottsWeight;

			// the contrast-term weight
			pipeline::Input<double> _contrastWeight;

			// the sigma for the gray-scale distribution
			pipeline::Input<double> _contrastSigma;

			// whether to use a four- or eight-neighborhood
			pipeline::Input<bool>   _eightNeighborhood;

			// the graph cut parameters in a single object
			pipeline::Output<GraphCutParameters> _parameters;
	};

	// a slider controlling the foreground prior
	boost::shared_ptr<gui::Slider<double> > _foregroundPriorSlider;

	// a slider controlling the potts term weight
	boost::shared_ptr<gui::Slider<double> > _pottsWeightSlider;

	// a slider controlling the contrast term weight
	boost::shared_ptr<gui::Slider<double> > _contrastWeightSlider;

	// a slider controlling the contrast term sigma
	boost::shared_ptr<gui::Slider<double> > _contrastSigmaSlider;

	// a switch to selecet betwee 4 and 8 neighborhood
	boost::shared_ptr<gui::Switch> _eightNeighborhoodSwitch;

	// a container of gui elements to control the fields of the parameters
	// object
	boost::shared_ptr<gui::ContainerView<gui::VerticalPlacing> > _gui;

	// a collector that creates the parameters object from the output of the gui
	// elements
	boost::shared_ptr<ParametersCollector> _parametersCollector;
};

#endif // GUI_GRAPH_CUT_DIALOG_H__

