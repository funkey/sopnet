#ifndef SOPNET_GUI_SLICE_ERRORS_VIEW_H__
#define SOPNET_GUI_SLICE_ERRORS_VIEW_H__

#include <sstream>

#include <pipeline/all.h>
#include <gui/TextPainter.h>
#include <sopnet/evaluation/SliceErrors.h>
#include <sopnet/evaluation/Errors.h>

class ErrorsView : public pipeline::SimpleProcessNode<> {

public:

	ErrorsView() :
		_painter(new gui::TextPainter()) {

		registerInput(_sliceErrors, "slice errors");
		registerInput(_variationOfInformation, "variation of information", pipeline::Optional);
		registerInput(_tedErrors, "tolerant edit distance errors", pipeline::Optional);
		registerOutput(_painter, "painter");

		_painter.registerSlot(_sizeChanged);
	}

private:

	void updateOutputs() {

		std::stringstream ss;

		ss
				<< "false positives: " << _sliceErrors->numFalsePositives() << ", "
				<< "false negatives: " << _sliceErrors->numFalseNegatives() << ", "
				<< "splits: " << _sliceErrors->numFalseSplits() << ", "
				<< "merges: " << _sliceErrors->numFalseMerges() << "; "
				<< "total: " <<
					(_sliceErrors->numFalsePositives() +
					 _sliceErrors->numFalseNegatives() +
					 _sliceErrors->numFalseSplits() +
					 _sliceErrors->numFalseMerges());

		if (_variationOfInformation.isSet())
			ss
				<< " -- "
				<< "variation of information: " << *_variationOfInformation;

		if (_tedErrors.isSet())
			ss
				<< " -- TED "
				<< "false positives " << _tedErrors->getNumFalsePositives() << ", "
				<< "false negatives " << _tedErrors->getNumFalseNegatives() << ", "
				<< "splits: " << _tedErrors->getNumSplits() << ", "
				<< "merges: " << _tedErrors->getNumMerges() << std::endl;

		_painter->setText(ss.str());

		_sizeChanged(_painter->getSize());
	}

	pipeline::Input<SliceErrors>       _sliceErrors;
	pipeline::Input<double>            _variationOfInformation;
	pipeline::Input<Errors>		   _tedErrors;
	pipeline::Output<gui::TextPainter> _painter;

	signals::Slot<const gui::SizeChanged> _sizeChanged;
};

#endif // SOPNET_GUI_SLICE_ERRORS_VIEW_H__

