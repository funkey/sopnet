#ifndef NUMBER_VIEW_H__
#define NUMBER_VIEW_H__

#include <pipeline/all.h>
#include <gui/TextPainter.h>
#include <gui/Signals.h>

class NumberView : public pipeline::ProcessNode {

public:

	NumberView();

private:

	void onModified(const pipeline::Modified& signal);
	void onUpdate(const pipeline::Update& signal);

	pipeline::Input<double>            _value;
	pipeline::Output<gui::TextPainter> _painter;

	signals::Slot<const pipeline::Modified> _modified;
	signals::Slot<const gui::SizeChanged>   _sizeChanged;

	bool _dirty;
};

#endif // NUMBER_VIEW_H__

