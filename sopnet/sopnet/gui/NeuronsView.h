#ifndef SOPNET_GUI_NEURONS_VIEW_H__
#define SOPNET_GUI_NEURONS_VIEW_H__

#include <pipeline/all.h>
#include <pipeline/Process.h>
#include <gui/ContainerView.h>
#include <gui/HorizontalPlacing.h>
#include <gui/GuiSignals.h>
#include <sopnet/evaluation/SliceErrors.h>
#include <sopnet/segments/SegmentTrees.h>

class NeuronsView : public pipeline::SimpleProcessNode<> {

public:

	NeuronsView();

private:

	class PainterAssigner : public pipeline::SimpleProcessNode<> {

	public:

		PainterAssigner() {

			registerInput(_painter, "painter");
		}

		void assignTo(gui::ContainerPainter& painter) {

			updateInputs();

			painter = *_painter;
		}

	private:

		void updateOutputs() {}

		pipeline::Input<gui::ContainerPainter> _painter;
	};

	void updateOutputs();

	void onNeuronsModified(const pipeline::Modified& signal);

	void onMouseDownOnNeuron(const gui::MouseDown& signal, unsigned int neuron);

	pipeline::Input<SegmentTrees>  _neurons;
	pipeline::Input<SliceErrors>   _sliceErrors;
	pipeline::Output<unsigned int> _currentNeuron;

	pipeline::Process<gui::ContainerView<gui::HorizontalPlacing> > _container;

	bool _neuronsChanged;
};

#endif // SOPNET_GUI_NEURONS_VIEW_H__

