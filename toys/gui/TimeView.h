#ifndef GUI_TIME_VIEW_H__
#define GUI_TIME_VIEW_H__

#include <toys/Time.h>
#include <pipeline/all.h>
#include <gui/TextPainter.h>
#include <gui/Signals.h>

class TimeView : public pipeline::ProcessNode {

public:

	TimeView();

	~TimeView();

private:

	void onModified(const pipeline::Modified& signal);

	void onUpdate(const pipeline::Update& signal);

	pipeline::Input<TimeStamp>         _time;
	pipeline::Output<gui::TextPainter> _textPainter;

	signals::Slot<const pipeline::Modified>  _modified;
	signals::Slot<const gui::SizeChanged>    _sizeChanged;

	bool _dirty;
};

#endif // GUI_TIME_VIEW_H__

