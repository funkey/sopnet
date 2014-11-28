#ifndef SOPNET_GUI_SLICE_ERRORS_VIEW_H__
#define SOPNET_GUI_SLICE_ERRORS_VIEW_H__

#include <sstream>

#include <pipeline/all.h>
#include <gui/TextPainter.h>

class ErrorsView : public pipeline::SimpleProcessNode<> {

public:

	ErrorsView() :
		_painter(new gui::TextPainter()) {

		registerInput(_errorReport, "error report");
		registerOutput(_painter, "painter");

		_painter.registerSlot(_sizeChanged);
	}

private:

	void updateOutputs() {

		_painter->setText(*_errorReport);
		_sizeChanged(_painter->getSize());
	}

	pipeline::Input<std::string>       _errorReport;
	pipeline::Output<gui::TextPainter> _painter;

	signals::Slot<const gui::SizeChanged> _sizeChanged;
};

#endif // SOPNET_GUI_SLICE_ERRORS_VIEW_H__

