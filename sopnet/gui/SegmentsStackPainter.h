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

	/**
	 * @param onlyOneSegment Start with the painter in only-one-segment mode,
	 *                       i.e., show only one segment at a time.
	 */
	SegmentsStackPainter(bool onlyOneSegment = false);

	/**
	 * Set a new set of segments.
	 */
	void setSegments(boost::shared_ptr<Segments> segments);

	/**
	 * Change the currently visible section.
	 */
	void setCurrentSection(unsigned int section);

	/**
	 * Toggle between showing only one segment and all segments.
	 *
	 * @return The new state.
	 */
	bool toggleShowOnlyOneSegment();

	/**
	 * Show segments to the previous section.
	 */
	void showPrev(bool show);

	/**
	 * Show segments to the next section.
	 */
	void showNext(bool show);

	/**
	 * Show end segments.
	 */
	void showEnds(bool show);

	/**
	 * Show continuation segments.
	 */
	void showContinuations(bool show);

	/**
	 * Show branch segments.
	 */
	void showBranches(bool show);

	/**
	 * Set the focus in the only-one-segment mode.
	 */
	void setFocus(const util::point<double>& position);

	/**
	 * Show the next segment in the only-one-segment mode.
	 */
	void nextSegment();

	/**
	 * Show the previous segment in the only-one-segment mode.
	 */
	void prevSegment();

	/**
	 * Overwritten from painter.
	 */
	virtual void draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution);

	/**
	 * Returns true if the painter is in the only-one-segment mode.
	 */
	bool onlyOneSegment();

private:

	// add the given slice's bounding box to the current size
	util::rect<double> sizeAddSlice(const util::rect<double>& currentSize, const Slice& slice);

	// find a random color for all slices of one neuron
	void assignColors();

	// add a slice to the same-neuron lookup table
	void addSlice(unsigned int slice);

	// merge the slices of the neurons belonging to slice1 and slice2
	void mergeSlices(unsigned int slice1, unsigned int slice2);

	// fill the sets of prev/next segments
	void updateVisibleSegments();

	void drawSlice(
		const Slice& slice,
		double z,
		double red, double green, double blue,
		double alpha,
		const util::rect<double>&  roi,
		const util::point<double>& resolution);

	// the segments
	boost::shared_ptr<Segments> _segments;

	// the previous segments of the currently selected section
	boost::shared_ptr<Segments> _prevSegments;

	// the next segments of the currently selected section
	boost::shared_ptr<Segments> _nextSegments;

	// the closest previous segments to the current focus
	std::vector<boost::shared_ptr<EndSegment> >          _closestPrevEndSegments;
	std::vector<boost::shared_ptr<ContinuationSegment> > _closestPrevContinuationSegments;
	std::vector<boost::shared_ptr<BranchSegment> >       _closestPrevBranchSegments;

	// the closest next segments to the current focus
	std::vector<boost::shared_ptr<EndSegment> >          _closestNextEndSegments;
	std::vector<boost::shared_ptr<ContinuationSegment> > _closestNextContinuationSegments;
	std::vector<boost::shared_ptr<BranchSegment> >       _closestNextBranchSegments;

	// the closest previous segment to show currently
	unsigned int _closestPrevSegment;

	// the closest next segment to show currently
	unsigned int _closestNextSegment;

	// the textures of the slices to draw
	SliceTextures _textures;

	// the section to draw
	unsigned int _section;

	// show only one segment at a time
	bool _onlyOneSegment;

	// show segments in certain directions
	bool _showPrev;
	bool _showNext;

	// show certain types of segments
	bool _showEnds;
	bool _showContinuations;
	bool _showBranches;

	// the focus for the only-one-segment mode
	util::point<double> _focus;

	// the distance between sections
	double _zScale;

	// the height of the section
	double _sectionHeight;

	// a lookup table for slice colors
	std::map<unsigned int, boost::array<double, 3> > _colors;

	// a lookup table for slices of the same neuron
	std::map<unsigned int, std::set<unsigned int> > _slices;
};

#endif // SOPNET_GUI_SEGMENTS_STACK_PAINTER_H__

