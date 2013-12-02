#ifndef SOPNET_GUI_SPLIT_MERGE_PAINTER_H__
#define SOPNET_GUI_SPLIT_MERGE_PAINTER_H__

#include <boost/shared_ptr.hpp>

#include <gui/Painter.h>

#include <sopnet/segments/Segments.h>

class SplitMergePainter : public gui::Painter {

public:

	SplitMergePainter();

	void setSelection(boost::shared_ptr<std::set<boost::shared_ptr<Slice> > > selection) { _selection = selection; }

	void setSection(int section) { _section = section; }

	void updateSize();

	bool draw(
			const util::rect<double>& roi,
			const util::point<double>& resolution);

private:

	void drawSlice(const Slice& slice);

	boost::shared_ptr<std::set<boost::shared_ptr<Slice> > > _selection;

	int _section;
};

#endif // SOPNET_GUI_SPLIT_MERGE_PAINTER_H__

