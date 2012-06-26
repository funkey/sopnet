#ifndef CELLTRACKER_GUI_TRACKLETS_PAINTER_H__
#define CELLTRACKER_GUI_TRACKLETS_PAINTER_H__

#include <gui/RecordablePainter.h>
#include <gui/Texture.h>
#include <sopnet/Segments.h>

class SegmentsPainter : public gui::RecordablePainter {

public:

	SegmentsPainter();

	~SegmentsPainter();

	void setSegments(boost::shared_ptr<Segments> segments);

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

	void deleteTextures();

	boost::shared_ptr<Segments> _segments;

	util::rect<double> _size;

	// the distance between two sections
	double _zScale;

	// slice textures from slice ids
	std::map<unsigned int, gui::Texture*> _textures;

	// which side of the faces to draw
	bool _leftSide;
	bool _rightSide;
};

#endif // CELLTRACKER_GUI_TRACKLETS_PAINTER_H__

