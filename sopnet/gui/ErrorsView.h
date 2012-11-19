#ifndef SOPNET_GUI_ERRORS_VIEW_H__
#define SOPNET_GUI_ERRORS_VIEW_H__

#include <sstream>

#include <pipeline/all.h>
#include <gui/TextPainter.h>
#include <sopnet/evaluation/Errors.h>

class ErrorsView : public pipeline::SimpleProcessNode<> {

public:

	ErrorsView() {

		registerInput(_errors, "errors");
		registerOutput(_painter, "painter");

		_painter.registerForwardSlot(_sizeChanged);
	}

private:

	void updateOutputs() {

		std::stringstream ss;

		ss
				<< "false positives: " << _errors->numFalsePositives() << ", "
				<< "false negatives: " << _errors->numFalseNegatives() << ", "
				<< "false splits: " << _errors->numFalseSplits() << ", "
				<< "false merges: " << _errors->numFalseMerges();

		_painter->setText(ss.str());

		_sizeChanged(_painter->getSize());
	}

	pipeline::Input<Errors>            _errors;
	pipeline::Output<gui::TextPainter> _painter;

	signals::Slot<const gui::SizeChanged> _sizeChanged;
};

#endif // SOPNET_GUI_ERRORS_VIEW_H__

