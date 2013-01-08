#include <util/Logger.h>
#include "Mser.h"

logger::LogChannel mserlog("mserlog", "[Mser] ");

Mser::Mser() {

	registerInput(_image, "image");
	registerInput(_parameters, "parameters");

	registerOutput(_componentTree, "component tree");

	_neighborOffsets.push_back(util::point<int>(-1,  0)); // left
	_neighborOffsets.push_back(util::point<int>( 0, -1)); // up
	_neighborOffsets.push_back(util::point<int>( 1,  0)); // right
	_neighborOffsets.push_back(util::point<int>( 0,  1)); // down
}

void
Mser::updateOutputs() {

	process();
}

void
Mser::process() {

	LOG_DEBUG(mserlog) << "starting MSER extraction..." << std::endl;

	allocate();

	copyImage();

	if (_parameters->darkToBright) {

		LOG_DEBUG(mserlog) << "...from dark to bright..." << std::endl;
		process(true);
	}

	if (_parameters->brightToDark) {

		LOG_DEBUG(mserlog) << "...from bright to dark..." << std::endl;
		process(false);
	}

	createComponentTree();

	deallocate();

	LOG_DEBUG(mserlog) << "done" << std::endl;
}

void
Mser::allocate() {

	_size = _image->width()*_image->height();

	LOG_ALL(mserlog) << "allocating memory for " << _size << " pixels" << std::endl;

	_values.resize(_size);
	_visited.resize(_size);
	_pixelList.resize(_size);
	_nextNeighbors.resize(_size);
	_stacks.resize(256);
	_histories.resize(_size);
	_regions.resize(257);
}

void
Mser::deallocate() {

	LOG_ALL(mserlog) << "deallocating memory" << std::endl;

	std::vector<unsigned char>().swap(_values);
	std::vector<bool>().swap(_visited);
	std::vector<unsigned char>().swap(_nextNeighbors);
	std::vector<std::stack<unsigned int> >().swap(_stacks);
	std::vector<GrowHistory>().swap(_histories);
	std::vector<mser::Region>().swap(_regions);

	_pixelList.clear();
}

void
Mser::copyImage() {

	if (!_parameters->sameIntensityComponents) {

		unsigned int i = 0;
		for (Image::iterator p = _image->begin(); p != _image->end(); i++, p++)
			_values[i] = (unsigned char)((*p)*255.0);

	} else {

		for (unsigned int x = 0; x < _image->width(); x++)
			for (unsigned int y = 0; y < _image->height(); y++) {

				unsigned char value = (unsigned char)((*_image)(x, y)*255.0);

				if (x > 0) {

					unsigned char leftValue = (unsigned char)((*_image)(x - 1, y)*255.0);

					if (leftValue != value)
						value = 0;
				}

				if (y > 0) {

					unsigned char topValue = (unsigned char)((*_image)(x, y - 1)*255.0);

					if (topValue != value)
						value = 0;
				}

				_values[positionToIndex(util::point<unsigned int>(x, y))] = value;
			}
	}
}

void
Mser::reset() {

	// reset the contents of the per-pixel data strucures
	for (int i = 0; i < _size; i++) {

		_visited[i]       = false;
		_nextNeighbors[i] = 0;
		_histories[i]     = GrowHistory();
	}

	// reset the contents of the per-value data structures
	for (unsigned int i = 0; i < _stacks.size(); i++)
		_stacks[i] = std::stack<unsigned int>();

	for (unsigned int i = 0; i < _regions.size(); i++)
		_regions[i] = mser::Region();

	// reset counters
	_currentRegion  = 0;
	_currentHistory = 0;

	// clear the found msers
	_msers.clear();
}

void
Mser::process(bool darkToBright) {

	LOG_DEBUG(mserlog) << "Processing from " << (darkToBright ? "dark to bright" : "bright to dark") << std::endl;

	reset();

	// add dummy region
	_regions[_currentRegion] = mser::Region(256, &_pixelList, _image, &(*_parameters));

	// setup indices
	_curIndex = 0;
	_curPosition = util::point<int>(0, 0);
	util::point<int> neighborPosition(0, 0);

	// get value of first pixel to process
	_curValue = (darkToBright ? _values[_curIndex] : 255 - _values[_curIndex]);

	LOG_ALL(mserlog) << "initial value at " << _curPosition << " is " << (int)_curValue << std::endl;

	// start a first region
	_currentRegion++;
	_regions[_currentRegion] = mser::Region(_curValue, &_pixelList, _image, &(*_parameters));

	// remember that we have been here
	_visited[_curIndex] = true;

	// select the stack of pixel indices that have the current value
	int curStack = _curValue;

	// initialise a variable to measure the progress
	int progress = 0;

	while (true) {

		// TODO:
		//setProgress(progress);

		// as long as we didn't see every neighbor
		while (_nextNeighbors[_curIndex] < 4) {

			// set neighborPosition to next neighbor of current position
			neighborPosition = _curPosition + _neighborOffsets[_nextNeighbors[_curIndex]];

			// check if we are still inside the image
			if (neighborPosition.x < 0 || neighborPosition.x >= _image->width() ||
				neighborPosition.y < 0 || neighborPosition.y >= _image->height()) {

				// process next neighbor
				_nextNeighbors[_curIndex]++;
				continue;
			}

			// get the index of the neighbor pixel
			int neighborIndex = positionToIndex(neighborPosition);

			// if not visited already
			if (!_visited[neighborIndex]) {

				// remember that we looked at this pixel already
				_visited[neighborIndex] = true;

				progress++;

				// get the value of the neighbor
				unsigned char neighborValue = (darkToBright ? _values[neighborIndex] : 255 - _values[neighborIndex]);

				// neighbor value smaller than current value?
				if (neighborValue < _curValue) {

					// add _current_ pixel to bundary heap and continue
					// processing the _neighbor_ pixel instead
					_stacks[curStack].push(_curIndex);

					// done with this neighbor
					_nextNeighbors[_curIndex]++;

					// continue with neighbor pixel
					curStack    = neighborValue;
					_curIndex    = neighborIndex;
					_curPosition = neighborPosition;
					_curValue    = neighborValue;

					// create a new region for it
					_currentRegion++;
					_regions[_currentRegion] = mser::Region(_curValue, &_pixelList, _image, &(*_parameters));

					// look at the neighbors of the new current pixel
					continue;

				// neighbor value equal or bigger than current value
				} else {

					// add the neighbor pixel to the boundary heap
					_stacks[neighborValue].push(neighborIndex);
				}

			}

			// continue with the next neighbor
			_nextNeighbors[_curIndex]++;
		}

		// add current pixel to current region
		_regions[_currentRegion].addPosition(_curIndex, indexToPosition(_curIndex), _values[_curIndex]);

		// try to get the next pixel of equal value from the boundary heap

		// if there are some...
		if (!_stacks[curStack].empty()) {

			// ...make the first one the current pixel
			_curIndex = _stacks[curStack].top();
			_stacks[curStack].pop();

			_curValue = (darkToBright ? _values[_curIndex] : 255 - _values[_curIndex]);

			// get the position from the index
			_curPosition = indexToPosition(_curIndex);

		// if there are none...
		} else {

			// start searching in the next stack...
			curStack++;

			// ...until we find a non-empty one
			int nextValue = 0;
			for (int i = curStack; i < 256; i++) {

				if (!_stacks[curStack].empty()) {

					nextValue = i;
					break;
				}
				curStack++;
			}

			// there was a next non-empty heap
			if (nextValue != 0) {

				// set current pixel to next one in the heap
				_curIndex = _stacks[curStack].top();
				_stacks[curStack].pop();

				_curValue = (darkToBright ? _values[_curIndex] : 255 - _values[_curIndex]);
				_curPosition = indexToPosition(_curIndex);

				processStack(nextValue);

			} else { // no next non-empty heap

				break;
			}

		} // if current heap empty

	} // while (true)

	// TODO:
	//setProgress(progress);
}

void
Mser::processStack(int nextValue) {

	while (true) {

		processCurrentRegion();

		if (nextValue < _regions[_currentRegion - 1].getValue()) {

			setCurrentRegionValue(nextValue);

			return;

		} else {

			_regions[_currentRegion - 1].merge(&_regions[_currentRegion], &_histories[_currentHistory]);

			_currentRegion--;
			_currentHistory++;

			if (nextValue <= _regions[_currentRegion].getValue())
				return;
		}
	}
}

void
Mser::processCurrentRegion() {

	// check for stability of the current region
	if (_regions[_currentRegion].isStable()) {

		/*
		 *  (stable) A          A  (can grow further)
		 *          / \    ⇒    |
		 *         /   \        A' (stable copy)
		 *        /     \      / \
		 *       B       C    B   C
		 */

		// create a copy of the current region
		// (We need to do this, because the original region will be further
		// worked with. In particular, it will grow. Therefore, we just add an
		// artificial region in between that will not be changed)
		_msers.push_back(_regions[_currentRegion]);

		int copyId = _msers.size() - 1;

		// all children of the current region are not top-level msers anymore
		foreach (int child, _regions[_currentRegion].getChildRegions())
			_msers[child].setTopLevel(false);

		// make the original region point to this copy as a child
		_regions[_currentRegion].setChildRegion(copyId);
	}
}

void
Mser::setCurrentRegionValue(int value) {

	_regions[_currentRegion].addHistory(&_histories[_currentHistory]);
	_regions[_currentRegion].setValue(value);

	_currentHistory++;
}

void
Mser::createComponentTree() {

	LOG_DEBUG(mserlog) << "creating component tree for " << _msers.size() << " regions" << std::endl;

	_componentTree->clear();

	/* Here, we "straighten out" the pixel list that was build by the mser
	 * algorithm. This means, that
	 *
	 *           b         e
	 *   value 5 3 6 9 2 8 1 0 4 7
	 *         -------------------
	 *   index 0 1 2 3 4 5 6 7 8 9
	 *
	 * becomes
	 *
	 *         b                 e
	 *   value 1 3 9 7 0 5 8 4 2 6
	 *         -------------------
	 *   index 0 1 2 3 4 5 6 7 8 9
	 * .
	 *
	 * The conversion is done by traversing through all found regions in the
	 * component tree and converting the old pixel indices on-the-fly into 2D
	 * pixel positions.
	 *
	 * For that, the global begin and end of the old pixel list has to be found
	 * first.
	 */

	// find the first and last pixel index
	int begin = PixelList::None;
	int end   = PixelList::None;

	for (unsigned int i = 0; i < _pixelList.size() && (begin == PixelList::None || end == PixelList::None); i++) {

		if (_pixelList.prev[i] == PixelList::None)
			begin = i;

		if (_pixelList.next[i] == PixelList::None)
			end= i;
	}

	// create an artifical root region that contains all pixels
	mser::Region root(255, &_pixelList, _image, &(*_parameters), begin, end);

	// add all top-level msers as children to root
	for (unsigned int i = 0; i < _msers.size(); i++)
		if (_msers[i].isTopLevel())
			root.addChildRegion(i);

	// add it to list of msers
	_msers.push_back(root);
	int rootId = _msers.size() - 1;

	// create a shared new pixel list
	boost::shared_ptr<std::vector<util::point<unsigned int> > > sharedPixelList = boost::make_shared<std::vector<util::point<unsigned int> > >(_pixelList.size());

	// the current pixel in the new pixel list
	unsigned int currentPixel = 0;

	// create the tree and straighten out the pixel list on-the-fly
	boost::shared_ptr<ComponentTree::Node> rootNode = createSubComponentTree(sharedPixelList, currentPixel, rootId);

	// set the root of the component tree
	_componentTree->setRoot(rootNode);

	LOG_DEBUG(mserlog) << "created component tree" << std::endl;
}

boost::shared_ptr<ComponentTree::Node>
Mser::createSubComponentTree(boost::shared_ptr<std::vector<util::point<unsigned int> > > sharedPixelList, unsigned int& currentPixel, int mserId) {

	LOG_ALL(mserlog) << "exploring region " << mserId << std::endl;

	// get the old pixel list indices
	int head = _msers[mserId].getHeadIndex();
	int tail = _msers[mserId].getTailIndex();

	// the begin of this component in the shared pixel list
	int begin = currentPixel;

	// get the start indices of all children
	std::map<unsigned int, int> indexToChildId;
	foreach (int child, _msers[mserId].getChildRegions())
		indexToChildId[_msers[child].getHeadIndex()] = child;

	// create a list of children
	std::vector<boost::shared_ptr<ComponentTree::Node> > children;

	/* Go over all old pixel indices of this region. Whenever we detect the
	 * beginning of a child region, we let the child fill the pixel list.
	 */
	for (int i = head;; i = _pixelList.next[i]) {

		// we are at the beginning of a child region
		if (indexToChildId.count(i)) {

			int childId = indexToChildId[i];

			// let the child fill the pixel list
			children.push_back(createSubComponentTree(sharedPixelList, currentPixel, childId));

			// continue behind the child pixels
			i = _msers[childId].getTailIndex();

			continue;
		}

		// we found a pixel that exclusively belongs to us
		(*sharedPixelList)[currentPixel] = util::point<unsigned int>(i%_image->width(), i/_image->width());

		LOG_ALL(mserlog) << "added pixel" << (*sharedPixelList)[currentPixel] << std::endl;

		currentPixel++;

		if (i == tail)
			break;
	}

	// the end (exclusive) of this component in the shared pixel list
	int end = currentPixel;

	// create a new connected component
	boost::shared_ptr<Image> source = _image;
	boost::shared_ptr<ConnectedComponent> component = boost::make_shared<ConnectedComponent>(source, (double)_msers[mserId].getValue()/255.0, sharedPixelList, begin, end);

	// create a component tree node for this connected component
	boost::shared_ptr<ComponentTree::Node> componentNode = boost::make_shared<ComponentTree::Node>(component);

	// add the children
	foreach (boost::shared_ptr<ComponentTree::Node> childNode, children)
		componentNode->addChild(childNode);

	LOG_ALL(mserlog) << "finished region " << mserId << " with " << (end - begin) << " pixels" << std::endl;

	return componentNode;
}

util::point<unsigned int>
Mser::indexToPosition(unsigned int index) {

	util::point<int> position;

	position.x = index%_image->width();
	position.y = index/_image->width();

	return position;
}

unsigned int
Mser::positionToIndex(const util::point<int>& position) {

	return position.y*_image->width() + position.x;
}
