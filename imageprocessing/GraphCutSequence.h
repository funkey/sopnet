#ifndef IMAGEPROCESSING_GRAPH_CUT_SEQUENCE_H__
#define IMAGEPROCESSING_GRAPH_CUT_SEQUENCE_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include <util/ProgramOptions.h>

class SequenceParameterGenerator : public pipeline::SimpleProcessNode<> {

public:

	SequenceParameterGenerator();

	bool next();

private:

	void updateOutputs();

	pipeline::Output<GraphCutParameters> _parameters;

	float _maxForegroundPrior;
	float _minForegroundPrior;
	float _stepForegroundPrior;
};

class ImageAverager : public pipeline::SimpleProcessNode<> {

public:

	ImageAverager();

	void accumulate();

private:

	void updateOutputs();

	pipeline::Input<Image>  _image;

	pipeline::Output<Image> _average;

	unsigned int _numImages;

	vigra::MultiArray<2, float> _averageData;
};

class GraphCutSequence : public pipeline::SimpleProcessNode<> {

public:

	GraphCutSequence();

	void createSequence();

private:

	void updateOutputs();

	pipeline::Input<ImageStack> _stack;
};

#endif // IMAGEPROCESSING_GRAPH_CUT_SEQUENCE_H__

