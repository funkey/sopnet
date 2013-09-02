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

	/**
	 * Set a new set of segments.
	 */
	void setSegments(boost::shared_ptr<Segments> segments);

	/**
	 * Change the currently visible section.
	 */
	void setCurrentSection(unsigned int section);

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
	 * Show the ids of the slices and their connection partners.
	 */
	void showSliceIds(bool show);

	/**
	 * Set the focus.
	 */
	void setFocus(const util::point<double>& position);

	/**
	 * Show the next segment.
	 */
	void nextSegment();

	/**
	 * Show the previous segment.
	 */
	void prevSegment();

	/**
	 * Overwritten from painter.
	 */
	virtual bool draw(
		const util::rect<double>&  roi,
		const util::point<double>& resolution);

	/**
	 * Fill segments with the currently visible segments.
	 */
	void getVisibleSegments(Segments& segments);

private:

	// add the given slice's bounding box to the current size
	util::rect<double> sizeAddSlice(const util::rect<double>& currentSize, const Slice& slice);

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

	// all currently visible segments to the previous section
	boost::shared_ptr<Segments> _prevSegments;

	// all currently visible segments to the next section
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

	// show segments in certain directions
	bool _showPrev;
	bool _showNext;

	// show certain types of segments
	bool _showEnds;
	bool _showContinuations;
	bool _showBranches;

	// show slice ids
	bool _showSliceIds;

	// the point around which to show segments
	util::point<double> _focus;

	// the distance between sections
	double _zScale;

	// the height of the section
	double _sectionHeight;
};

#endif // SOPNET_GUI_SEGMENTS_STACK_PAINTER_H__

