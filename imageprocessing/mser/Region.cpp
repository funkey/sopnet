#include "Region.h"

namespace mser {

Region::Region() :
	_head(PixelList::None),
	_tail(PixelList::None),
	_pixelList(0),
	_history(0),
	_value(0),
	_size(0),
	_center(0, 0),
	_meanValue(0.0),
	_variation(0.0),
	_varChanged(false),
	_topLevel(true),
	_source(),
	_parameters(0) {}

Region::Region(
		int                      value,
		PixelList*               pixelList,
		boost::shared_ptr<Image> source,
		MserParameters*          parameters,
		int                      head,
		int                      tail) :
	_head(head),
	_tail(tail),
	_pixelList(pixelList),
	_history(0),
	_value(value),
	_size(0),
	_center(0, 0),
	_meanValue(0.0),
	_variation(0.0),
	_varChanged(false),
	_topLevel(true),
	_source(source),
	_parameters(parameters) {}

void
Region::setValue(int value) {

	_value = value;
}

int
Region::getValue() {

	return _value;
}

int
Region::getHeadIndex() {

	return _head;
}

int
Region::getTailIndex() {

	return _tail;
}

void
Region::addHistory(GrowHistory* newHistory) {

	newHistory->child = newHistory;

	if (_history == 0) {

		newHistory->shortcut   = newHistory;
		newHistory->stableSize = 0;

	} else {

		_history->child        = newHistory;
		newHistory->shortcut   = _history->shortcut;
		newHistory->stableSize = _history->stableSize;
	}

	newHistory->value = _value;
	newHistory->size  = _size;
	_history          = newHistory;
}

void
Region::merge(Region* other, GrowHistory* newHistory) {

	newHistory->child = newHistory;

	Region* bigger;
	Region* smaller;

	// find the bigger region
	if (_size >= other->_size) {

		bigger  = this;
		smaller = other;

	} else {

		bigger  = other;
		smaller = this;
	}

	if (bigger->_history == 0) {

		newHistory->shortcut   = newHistory;
		newHistory->stableSize = 0;

	} else {

		bigger->_history->child = newHistory;
		newHistory->shortcut   = bigger->_history->shortcut;
		newHistory->stableSize = bigger->_history->stableSize;
	}

	if (smaller->_history != 0 && smaller->_history->stableSize > newHistory->stableSize)
		newHistory->stableSize = smaller->_history->stableSize;

	newHistory->value = bigger->_value;
	newHistory->size  = bigger->_size;

	_variation  = bigger->_variation;
	_varChanged = bigger->_varChanged;

	// merge pixel sets
	if (bigger->_size > 0 && smaller->_size > 0) {

		_pixelList->next[bigger->_tail]  = smaller->_head;
		_pixelList->prev[smaller->_head] = bigger->_tail;
	}

	int newHead = (bigger->_size  > 0) ? bigger->_head  : smaller->_head;
	int newTail = (smaller->_size > 0) ? smaller->_tail : bigger->_tail;

	// update center position and mean gray value
	double thisWeight = (double)(_size)/(_size + other->_size);

	_center    =    _center*thisWeight +    other->_center*(1.0 - thisWeight);
	_meanValue = _meanValue*thisWeight + other->_meanValue*(1.0 - thisWeight);

	_head = newHead;
	_tail = newTail;
	_history = newHistory;
	_size = _size + other->_size;

	// update children
	addChildRegions(other->getChildRegions());
}

double
Region::calculateVariation() {

	if (_history != 0) {

		GrowHistory* shortcut = _history->shortcut;

		while (shortcut != shortcut->shortcut && shortcut->value + _parameters->delta > _value)
			shortcut = shortcut->shortcut;

		GrowHistory* child = shortcut->child;

		while (child != child->child && child->value + _parameters->delta <= _value) {

			shortcut = child;
			child = child->child;
		}

		// store the shortcut for later calls
		_history->shortcut = shortcut;

		// NOTE: this is not how it is proposed in the paper! this is
		// the opencv version:
		//
		// OpenCV: |R_{i}       - R_{i-delta}|/|R_{i-delta}|
		// Paper : |R_{i+delta} - R_{i-delta}|/|R_{i}|
		return (double)(_size - shortcut->size)/(double)shortcut->size;
	}

	return 1.0;
}

bool
Region::isStable() {

	if (_size <= _parameters->minArea || _size >= _parameters->maxArea) {

		LOG_ALL(mserregionlog) << "[Region] I am unstable (my size of " << _size
							   << " is not within [" << _parameters->minArea << ", " << _parameters->maxArea << ")"
							   << std::endl;

		return false;
	}

	if (_history == 0) {

		LOG_ALL(mserregionlog) << "[Region] I am unstable (I have no history)" << std::endl;

		return true;
	}

	double div = (double)(_history->size - _history->stableSize)/(double)_history->size;
	double var = calculateVariation();

	// change in variation?
	bool dvar     = (_variation < var || _history->value + 1 < _value);
	bool isStable = (dvar && !_varChanged && _variation < _parameters->maxVariation && div > _parameters->minDiversity);

	if (isStable) {

		LOG_ALL(mserregionlog) << "[Region] I am stable!" << std::endl;

		_history->stableSize = _history->size;

	} else {

		LOG_ALL(mserregionlog) << "[Region] I am unstable (variation: " << _variation
							   << ", diversity: " << div << ", var. changed: " << _varChanged << ")"
							   << std::endl;
	}

	_variation  = var;
	_varChanged = dvar;

	return isStable;
}

void
Region::addPosition(int index, const util::point<unsigned int>& position, int value) {

	if (_size > 0) {

		_pixelList->prev[index] = _tail;
		_pixelList->next[_tail] = index;
		_pixelList->next[index] = PixelList::None;

	} else {

		_pixelList->prev[index] = PixelList::None;
		_pixelList->next[index] = PixelList::None;
		_head = index;
	}

	_tail = index;
	_size++;

	// update center and mean gray value
	double weight = 1.0/(double)(_size);
	_center    =    _center*(1.0 - weight) + (util::point<double>)position*weight;
	_meanValue = _meanValue*(1.0 - weight) +                         value*weight;
}

void
Region::setTopLevel(bool topLevel) {

	_topLevel = topLevel;
}

bool
Region::isTopLevel() {

	return _topLevel;
}

/**
 * @param child The index of a stable region (vector _mser in class Mser).
 */
void
Region::setChildRegion(int child) {

	_childRegions.clear();

	addChildRegion(child);
}

/**
 * @param child The index of a stable region (vector _mser in class Mser).
 */
void
Region::addChildRegion(int child) {

	_childRegions.push_back(child);
}

/**
 * @param children The indices of stable regions (vector _mser in class Mser).
 */
void
Region::addChildRegions(const std::vector<int>& children) {

	_childRegions.insert(_childRegions.end(), children.begin(), children.end());
}

const std::vector<int>&
Region::getChildRegions() const {

	return _childRegions;
}

} // namespace mser
