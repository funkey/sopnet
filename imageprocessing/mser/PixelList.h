#ifndef IMAGEPROCESSING_MSER_PIXEL_LIST_H__
#define IMAGEPROCESSING_MSER_PIXEL_LIST_H__

struct PixelList {

	static const int None = -1;

	PixelList() {};

	PixelList(int size) :
		prev(size, None),
		next(size, None) {}

	void resize(size_t size) {

		prev.resize(size);
		next.resize(size);
	}

	unsigned int size() const {

		return prev.size();
	}

	void clear() {

		std::vector<int>().swap(next);
		std::vector<int>().swap(prev);
	}

	std::vector<int> prev;
	std::vector<int> next;
};

#endif // IMAGEPROCESSING_MSER_PIXEL_LIST_H__

