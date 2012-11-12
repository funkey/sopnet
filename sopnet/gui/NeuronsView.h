#ifndef SOPNET_GUI_NEURONS_VIEW_H__
#define SOPNET_GUI_NEURONS_VIEW_H__

#include <pipeline/all.h>
#include <gui/ContainerView.h>
#include <gui/HorizontalPlacing.h>
#include <sopnet/neurons/Neurons.h>

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

	pipeline::Input<Neurons> _neurons;
	pipeline::Output<gui::ContainerPainter> _painter;
};

#endif // SOPNET_GUI_NEURONS_VIEW_H__

