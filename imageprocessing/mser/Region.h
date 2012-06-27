#ifndef IMAGEPROCESSING_MSER_REGION_H__
#define IMAGEPROCESSING_MSER_REGION_H__

#include <boost/shared_ptr.hpp>

#include <imageprocessing/Image.h>
#include <imageprocessing/MserParameters.h>
#include <util/point.hpp>
#include <util/Logger.h>
#include "GrowHistory.h"
#include "PixelList.h"

namespace mser {

static logger::LogChannel mserregionlog("mserregionlog", "[Region] ");

class Region {

public:

	Region();

	Region(
			int                      value,
			PixelList*               pixelList,
			boost::shared_ptr<Image> source,
			MserParameters*          parameters,
			int                      head = PixelList::None,
			int                      tail = PixelList::None);

	void setValue(int value);

	int getValue();

	int getHeadIndex();

	int getTailIndex();

	void addHistory(GrowHistory* newHistory);

	void merge(Region* other, GrowHistory* newHistory);

	double calculateVariation();

	bool isStable();

	void addPosition(int index, const util::point<unsigned int>& position, int value);

	void setTopLevel(bool topLevel);

	bool isTopLevel();

	/**
	 * @param child The index of a stable region (vector _mser in class Mser).
	 */
	void setChildRegion(int child);

	/**
	 * @param child The index of a stable region (vector _mser in class Mser).
	 */
	void addChildRegion(int child);

	/**
	 * @param children The indices of stable regions (vector _mser in class Mser).
	 */
	void addChildRegions(const std::vector<int>& children);

	const std::vector<int>& getChildRegions() const;

private:

	// the start and end index of the pixels belonging to this component
	int _head;
	int _tail;

	// the image as a linked list of pixel indices
	PixelList* _pixelList;

	// the grow history of this connected component
	GrowHistory* _history;

	// the maximal gray-value of this component
	int _value;

	// the number of pixels in this component
	int _size;

	// the center of mass of this component
	util::point<double> _center;

	// the mean gray-value of this component
	double   _meanValue;

	// the variation of this component
	double   _variation;

	// indicates a change in the variation
	bool     _varChanged;

	// indicates that this region is a top-level region
	bool     _topLevel;

	// all child regions of this region
	std::vector<int> _childRegions;

	// the image this region was created from
	boost::shared_ptr<Image> _source;

	// the parameters for stability
	MserParameters* _parameters;
};

} // namespace mser

#endif // IMAGEPROCESSING_MSER_REGION_H__

