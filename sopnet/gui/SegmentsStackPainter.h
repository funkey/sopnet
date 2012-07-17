#ifndef SOPNET_GUI_SEGMENTS_STACK_PAINTER_H__
#define SOPNET_GUI_SEGMENTS_STACK_PAINTER_H__

#include <set>

#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>

#include <gui/Painter.h>
#include <sopnet/gui/SegmentsPainter.h>
#include <sopnet/segments/Segments.h>
#include "SliceTextures.h"

class SegmentsStackPainter : public gui::Painter {

public:

	SegmentsStackPainter();

	void setSegments(boost::shared_ptr<Segments> segments);

	void setCurrentSection(unsigned int section);

	/**
	 * Overwritten from painter.
	 */
	virtual void draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution);

private:

	// add the given slice's bounding box to the current size
	util::rect<double> sizeAddSlice(const util::rect<double>& currentSize, const Slice& slice);

	// find a random color for all slices of one neuron
	void assignColors();

	// add a slice to the same-neuron lookup table
	void addSlice(unsigned int slice);

	// merge the slices of the neurons belonging to slice1 and slice2
	void mergeSlices(unsigned int slice1, unsigned int slice2);

	void drawSlice(
		const Slice& slice,
		double z,
		double red, double green, double blue,
		double alpha);

	// the segments
	boost::shared_ptr<Segments> _segments;

	// the previous segments of the currently selected section
	boost::shared_ptr<Segments> _prevSegments;

	// the next segments of the currently selected section
	boost::shared_ptr<Segments> _nextSegments;

	// the textures of the slices to draw
	SliceTextures _textures;

	// the section to draw
	unsigned int _section;

	// the distance between sections
	double _zScale;

	// a lookup table for slice colors
	std::map<unsigned int, boost::array<double, 3> > _colors;

	// a lookup table for slices of the same neuron
	std::map<unsigned int, std::set<unsigned int> > _slices;
};

#endif // SOPNET_GUI_SEGMENTS_STACK_PAINTER_H__

