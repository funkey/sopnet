#ifndef SOPNET_GUI_SPLIT_MERGE_PAINTER_H__
#define SOPNET_GUI_SPLIT_MERGE_PAINTER_H__

#include <boost/shared_ptr.hpp>

#include <gui/Painter.h>
#include <imageprocessing/gui/ImagePainter.h>

#include <sopnet/segments/Segments.h>

class SplitMergePainter : public gui::Painter {

public:

	SplitMergePainter();

	void setSelection(boost::shared_ptr<std::set<boost::shared_ptr<Slice> > > selection) { _selection = selection; }

	void setSection(int section) { _section = section; }

	void setSliceImage(boost::shared_ptr<Image> sliceImage, const util::point<int>& offset = util::point<int>(0, 0));

	/**
	 * Indicate that the slice image has changed and needs to be reloaded.
	 */
	void reloadSliceImage();

	void unsetSliceImage();

	void updateSize();

	bool draw(
			const util::rect<double>& roi,
			const util::point<double>& resolution);

private:

	void drawSlice(const Slice& slice);

	boost::shared_ptr<std::set<boost::shared_ptr<Slice> > > _selection;

	int _section;

	boost::shared_ptr<gui::ImagePainter<Image> > _sliceImagePainter;
	util::point<int>                             _sliceImageOffset;
};

#endif // SOPNET_GUI_SPLIT_MERGE_PAINTER_H__

