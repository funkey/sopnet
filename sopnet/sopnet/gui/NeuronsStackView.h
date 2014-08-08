#include <pipeline/all.h>
#include <sopnet/segments/SegmentTrees.h>
#include <gui/Keys.h>
#include <gui/Buttons.h>
#include <gui/GuiSignals.h>
#include <gui/KeySignals.h>
#include <gui/MouseSignals.h>
#include "NeuronsStackPainter.h"

class NeuronsStackView : public pipeline::SimpleProcessNode<> {

public:

	/**
	 * @param onlyOneSegment Start with only-one-segment mode enabled.
	 */
	NeuronsStackView();

private:

	void onNeuronsModified(const pipeline::Modified& signal);

	void onCurrentNeuronModified(const pipeline::Modified& signal);

	void updateOutputs();

	void onKeyDown(gui::KeyDown& signal);

	void onMouseDown(gui::MouseDown& signal);

	// the neurons to show
	pipeline::Input<SegmentTrees> _neurons;

	// the number of the neuron to show currently
	pipeline::Input<unsigned int> _currentNeuron;

	// the painter for the currently selected section
	pipeline::Output<NeuronsStackPainter> _painter;

	// the currently user-selected set of neurons
	pipeline::Output<SegmentTrees> _selection;

	// signal to report size changes
	signals::Slot<gui::SizeChanged> _sizeChanged;

	// signal to report content changes
	signals::Slot<gui::ContentChanged> _contentChanged;

	// the section to show
	int _section;

	// indicates that the neurons modified
	bool _neuronsModified;

	// indicates that the current neuron modified
	bool _currentNeuronModified;

	// the alpha value for the painter
	double _alpha;

	// internal set of selected neurons
	boost::shared_ptr<SegmentTrees> _selectedNeurons;
};

