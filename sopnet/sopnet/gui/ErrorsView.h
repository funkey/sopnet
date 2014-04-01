#ifndef SOPNET_GUI_SLICE_ERRORS_VIEW_H__
#define SOPNET_GUI_SLICE_ERRORS_VIEW_H__

#include <sstream>

#include <pipeline/all.h>
#include <gui/TextPainter.h>
#include <sopnet/evaluation/SliceErrors.h>

class ErrorsView : public pipeline::SimpleProcessNode<> {

public:

	ErrorsView() :
		_painter(new gui::TextPainter()) {

		registerInput(_sliceErrors, "slice errors");
		registerInput(_variationOfInformation, "variation of information", pipeline::Optional);
		registerOutput(_painter, "painter");

		_painter.registerSlot(_sizeChanged);
	}

private:

	void updateOutputs() {

		std::stringstream ss;

		ss
				<< "false positives: " << _sliceErrors->numFalsePositives() << ", "
				<< "false negatives: " << _sliceErrors->numFalseNegatives() << ", "
				<< "false splits: " << _sliceErrors->numFalseSplits() << ", "
				<< "false merges: " << _sliceErrors->numFalseMerges() << "; "
				<< "total: " <<
					(_sliceErrors->numFalsePositives() +
					 _sliceErrors->numFalseNegatives() +
					 _sliceErrors->numFalseSplits() +
					 _sliceErrors->numFalseNegatives());

		if (_variationOfInformation.isSet())
			ss
				<< " -- "
				<< "variation of information: " << *_variationOfInformation << std::endl;

		_painter->setText(ss.str());

		_sizeChanged(_painter->getSize());
	}

	pipeline::Input<SliceErrors>       _sliceErrors;
	pipeline::Input<double>            _variationOfInformation;
	pipeline::Output<gui::TextPainter> _painter;

	signals::Slot<const gui::SizeChanged> _sizeChanged;
};

#endif // SOPNET_GUI_SLICE_ERRORS_VIEW_H__

