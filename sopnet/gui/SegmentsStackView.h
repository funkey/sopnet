#include <pipeline/all.h>
#include <sopnet/segments/Segments.h>
#include <gui/Keys.h>
#include <gui/Buttons.h>
#include <gui/Signals.h>
#include "SegmentsStackPainter.h"

class SegmentsStackView : public pipeline::SimpleProcessNode<> {

public:

	/**
	 * @param onlyOneSegment Start with only-one-segment mode enabled.
	 */
	SegmentsStackView(bool onlyOneSegment = false);

private:

	void onSegmentsModified(const pipeline::Modified& signal);

	void updateOutputs();

	void onKeyDown(gui::KeyDown& signal);

	void onMouseDown(gui::MouseDown& signal);

	// the segments to show
	pipeline::Input<Segments> _segments;

	// the painter for the currently selected section
	pipeline::Output<SegmentsStackPainter> _painter;

	// signal to report size changes
	signals::Slot<gui::SizeChanged> _sizeChanged;

	// signal to report content changes
	signals::Slot<gui::ContentChanged> _contentChanged;

	// the section to show
	int _section;

	// indicates that the segments modified
	bool _segmentsModified;
};
