#ifndef IMAGEPROCESSING_MSER_GROW_HISTORY_H__
#define IMAGEPROCESSING_MSER_GROW_HISTORY_H__

struct GrowHistory {

	GrowHistory* shortcut;
	GrowHistory* child;

	// the size of the component when it was stable, zero if the component
	// was never stable
	int stableSize;

	// the current gray value of the component
	int value;

	// the current size of the component
	int size;
};

#endif // IMAGEPROCESSING_MSER_GROW_HISTORY_H__

