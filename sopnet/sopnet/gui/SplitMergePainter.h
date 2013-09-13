#ifndef SOPNET_GUI_SPLIT_MERGE_PAINTER_H__
#define SOPNET_GUI_SPLIT_MERGE_PAINTER_H__

#include <boost/shared_ptr.hpp>

#include <gui/Painter.h>

#include <sopnet/segments/Segments.h>

class SplitMergePainter : public gui::Painter {

public:

	SplitMergePainter();

	void setSelection(boost::shared_ptr<Segments> selection) { _selection = selection; }

	void setSection(int section) { _section = section; }

	bool draw(
			const util::rect<double>& roi,
			const util::point<double>& resolution);

private:

	boost::shared_ptr<Segments> _selection;

	int _section;
};

#endif // SOPNET_GUI_SPLIT_MERGE_PAINTER_H__

