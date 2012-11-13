#ifndef CELLTRACKER_GUI_TRACKLETS_PAINTER_H__
#define CELLTRACKER_GUI_TRACKLETS_PAINTER_H__

#include <gui/RecordablePainter.h>
#include <gui/Texture.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/segments/Segments.h>
#include "SliceTextures.h"

class SegmentsPainter : public gui::RecordablePainter {

public:

	SegmentsPainter(std::string name = "");

	/**
	 * Set the image stack that should be used to texture the segments.
	 */
	void setImageStack(boost::shared_ptr<ImageStack> imageStack);

	/**
	 * Set the segments to draw.
	 */
	void setSegments(boost::shared_ptr<Segments> segments);

	void showEnds(bool show);

	void showContinuations(bool show);

	void showBranches(bool show);

private:

	void loadTextures();

	void loadTextures(const EndSegment& end);

	void loadTextures(const ContinuationSegment& continuation);

	void loadTextures(const BranchSegment& branch);

	void loadTexture(const Slice& slice);

	void updateRecording();

	void draw(const EndSegment& end);

	void draw(const ContinuationSegment& continuation);

	void draw(const BranchSegment& branch);

	void drawSlice(boost::shared_ptr<Slice> slice, double red, double green, double blue);

	boost::shared_ptr<ImageStack> _imageStack;

	boost::shared_ptr<Segments> _segments;

	util::rect<double> _size;

	// the distance between two sections
	double _zScale;

	// slice textures from slice ids
	SliceTextures _textures;

	// which side of the faces to draw
	bool _leftSide;
	bool _rightSide;

	bool _showEnds;
	bool _showContinuations;
	bool _showBranches;
};

#endif // CELLTRACKER_GUI_TRACKLETS_PAINTER_H__

