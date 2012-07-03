#include <sstream>

#include <imageprocessing/GraphCut.h>
#include <imageprocessing/io/ImageWriter.h>
#include "GraphCutSequence.h"

logger::LogChannel graphcutsequencelog("graphcutsequencelog", "[GraphCutSequence] ");

util::ProgramOption optionMinForegroundPrior(
		util::_module           = "graphcut",
		util::_long_name        = "minForegroundPrior",
		util::_description_text = "The minimal value of the foreground prior (used for a sequence of graph-cuts)",
		util::_default_value    = 0.0);

util::ProgramOption optionMaxForegroundPrior(
		util::_module           = "graphcut",
		util::_long_name        = "maxForegroundPrior",
		util::_description_text = "The maximal value of the foreground prior (used for a sequence of graph-cuts)",
		util::_default_value    = 1.0);

util::ProgramOption optionStepForegroundPrior(
		util::_module           = "graphcut",
		util::_long_name        = "stepForegroundPrior",
		util::_description_text = "The step-size to change the value of the foreground prior (used for a sequence of graph-cuts)",
		util::_default_value    = 0.01);

util::ProgramOption optionPottsWeight(
		util::_module           = "graphcut",
		util::_long_name        = "pottsWeight",
		util::_description_text = "The potts weight to be used for a sequence of graph-cuts",
		util::_default_value    = 0.5);

util::ProgramOption optionContrastWeight(
		util::_module           = "graphcut",
		util::_long_name        = "contrastWeight",
		util::_description_text = "The contrast weight to be used for a sequence of graph-cuts",
		util::_default_value    = 0.5);

util::ProgramOption optionContrastSigma(
		util::_module           = "graphcut",
		util::_long_name        = "contrastSigma",
		util::_description_text = "The contrast sigma to be used for a sequence of graph-cuts",
		util::_default_value    = 0.5);

util::ProgramOption optionEightNeighborhood(
		util::_module           = "graphcut",
		util::_long_name        = "eightNeighborhood",
		util::_description_text = "Enable an eight-neighborhood for the graph-cut.");

SequenceParameterGenerator::SequenceParameterGenerator() :
	_parameters(boost::make_shared<GraphCutParameters>()),
	_maxForegroundPrior(optionMaxForegroundPrior),
	_minForegroundPrior(optionMinForegroundPrior),
	_stepForegroundPrior(optionStepForegroundPrior) {

	registerOutput(_parameters, "parameters");

	_parameters->foregroundPrior = _minForegroundPrior;
	_parameters->pottsWeight     = optionPottsWeight;
	_parameters->contrastWeight  = optionContrastWeight;
	_parameters->contrastSigma   = optionContrastSigma;
	_parameters->eightNeighborhood = optionEightNeighborhood;
}

bool
SequenceParameterGenerator::next() {

	if (_parameters->foregroundPrior < _maxForegroundPrior) {

		setDirty(_parameters);

		return true;
	}

	return false;
}

void
SequenceParameterGenerator::updateOutputs() {

	_parameters->foregroundPrior += _stepForegroundPrior;

	LOG_DEBUG(graphcutsequencelog)
			<< "[SequenceParameterGenerator] set foreground prior to "
			<< _parameters->foregroundPrior << std::endl;
}

ImageAverager::ImageAverager() :
	_average(boost::make_shared<Image>()),
	_numImages(0) {

	registerInput(_image, "image");
	registerOutput(_average, "image");
}

void
ImageAverager::accumulate() {

	updateInputs();

	if (_averageData.shape()  != _image->shape()) {

		_averageData.reshape(_image->shape());
		_averageData.init(0.0);

		*_average = _averageData;
	}

	*_average += *_image;

	_numImages++;
}

void
ImageAverager::updateOutputs() {

	if (_numImages > 0)
		*_average /= _numImages;
}

GraphCutSequence::GraphCutSequence() {

	registerInput(_stack, "image stack");
}

void
GraphCutSequence::createSequence() {

	updateInputs();

	unsigned int i = 0;

	foreach (boost::shared_ptr<Image> image, *_stack) {

		LOG_DEBUG(graphcutsequencelog) << "setting up processing pipeline of image " << i << std::endl;

		// create a leading-zero string of the image number
		std::stringstream imageSs;
		imageSs << std::setw(5) << std::setfill('0') << i;
		std::string imageNumber = imageSs.str();

		boost::shared_ptr<SequenceParameterGenerator> parameterGenerator = boost::make_shared<SequenceParameterGenerator>();
		boost::shared_ptr<GraphCut>                   graphCut           = boost::make_shared<GraphCut>();
		boost::shared_ptr<ImageWriter>                imageWriter        = boost::make_shared<ImageWriter>("");
		boost::shared_ptr<ImageAverager>              imageAverager      = boost::make_shared<ImageAverager>();
		boost::shared_ptr<ImageWriter>                averageImageWriter = boost::make_shared<ImageWriter>(std::string("./slices/slices_") + imageNumber + ".png");

		graphCut->setInput("parameters", parameterGenerator->getOutput());
		graphCut->setInput("image", image);
		graphCut->setInput("potts image", image);

		imageAverager->setInput(graphCut->getOutput("segmentation"));
		imageWriter->setInput(graphCut->getOutput("segmentation"));
		averageImageWriter->setInput(imageAverager->getOutput());

		unsigned int j = 0;
		while (parameterGenerator->next()) {

			LOG_DEBUG(graphcutsequencelog) << "processing image " << i << ", parameters " << j << std::endl;

			std::stringstream parameterSs;
			parameterSs << std::setw(5) << std::setfill('0') << j;
			std::string parameterNumber = parameterSs.str();

			imageWriter->write(
					std::string("./sequence/slices_") +
					imageNumber + "_" +
					parameterNumber + ".png");

			imageAverager->accumulate();

			j++;
		}

		// save the average image, i.e., the slices image
		averageImageWriter->write();

		i++;
	}
}

void
GraphCutSequence::updateOutputs() {

	// nothing to do here
}
