#ifndef IMAGEPROCESSING_MSER_H__
#define IMAGEPROCESSING_MSER_H__

#include <vector>
#include <stack>

#include <pipeline/all.h>
#include <imageprocessing/ComponentTree.h>
#include "MserParameters.h"
#include "mser/Region.h"
#include "mser/GrowHistory.h"

class Mser : public pipeline::SimpleProcessNode<> {

public:

	Mser();

private:

	void updateOutputs();

	void process();

	/**
	 * Allocate the memory needed for the current image.
	 */
	void allocate();

	/**
	 * Make a copy of the image an convert it to unsigned char on-the-fly.
	 */
	void copyImage();

	/**
	 * Resets the contents of the local data structures.
	 */
	void reset();

	/**
	 * Find all MSERs in the current image and append them to the current
	 * connected components.
	 *
	 * @param darkToBright If true, searches from dark to bright, otherwise from
	 *                     bright to dark.
	 */
	void process(bool darkToBright);

	void processStack(int nextValue);

	void processCurrentRegion();

	void setCurrentRegionValue(int value);

	void createComponentTree();

	/**
	 * Free all the memory that was needed to generate the component tree.
	 */
	void deallocate();

	// creates a (partial) tree from an mser, gives back the top component tree
	// node
	boost::shared_ptr<ComponentTree::Node> createSubComponentTree(
			boost::shared_ptr<std::vector<util::point<unsigned int> > > sharedPixelList,
			unsigned int& currentPixel,
			int mserId);

	/**
	 * Convert a pixel index into a 2D position.
	 */
	util::point<unsigned int> indexToPosition(unsigned int index);

	/**
	 * Convert a 2D position into a pixel index.
	 */
	unsigned int positionToIndex(const util::point<int>& position);

	// inputs
	pipeline::Input<Image>          _image;
	pipeline::Input<MserParameters> _parameters;

	// output
	pipeline::Output<ComponentTree> _componentTree;

	// the number of pixels to process
	int _size;

	// integer values in the range [0,255] that represent the image
	std::vector<unsigned char> _values;

	// the four offsets to reach a neighbor
	std::vector<util::point<int> > _neighborOffsets;

	// true for each visited pixel
	std::vector<bool>  _visited;

	// the next nextNeighbors to explore for each pixel
	std::vector<unsigned char> _nextNeighbors;

	// indices of the pixels in a linked list
	PixelList _pixelList;

	// all found regions
	std::vector<mser::Region> _regions;

	// number of found regions
	unsigned int _currentRegion;

	// a history for each pixel
	std::vector<GrowHistory> _histories;

	// the index of the current pixel
	int _curIndex;

	// the position of the current pixel
	util::point<int> _curPosition;

	// the value of the current pixel
	unsigned char _curValue;

	// the current history
	unsigned int _currentHistory;

	// boundary pixel indices stacks (for each possible value)
	std::vector<std::stack<unsigned int> > _stacks;

	// the found stable regions
	std::vector<mser::Region> _msers;
};

#endif // IMAGEPROCESSING_MSER_H__

