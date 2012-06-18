#ifndef IMAGEPROCESSING_MSER_PARAMETERS_H__
#define IMAGEPROCESSING_MSER_PARAMETERS_H__


struct MserParameters : public pipeline::Data {

	MserParameters() :
		delta(1),
		minArea(0),
		maxArea(100000),
		maxVariation(100),
		minDiversity(0.0),
		darkToBright(true),
		brightToDark(false) {}

	// increase of the intensity level to check for stability
	int delta;

	// minimum and maximum size of connected components
	int minArea;
	int maxArea;

	// maximum variation for a stable region
	double maxVariation;

	// minumum diversity for a stable region
	double minDiversity;

	// perform processing from dark to bright pixels
	bool darkToBright;

	// perform processing from bright to dark pixels
	bool brightToDark;
};

#endif // IMAGEPROCESSING_MSER_PARAMETERS_H__

