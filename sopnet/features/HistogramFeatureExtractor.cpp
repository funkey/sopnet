#include <boost/lexical_cast.hpp>

#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "HistogramFeatureExtractor.h"

HistogramFeatureExtractor::HistogramFeatureExtractor(unsigned int numBins) :
	_features(boost::make_shared<Features>()),
	_numBins(numBins) {

	registerInput(_segments, "segments");
	registerInput(_sections, "raw sections");
	registerOutput(_features, "features");

	for (int i = 0; i < _numBins; i++)
		_features->addName("histogram " + boost::lexical_cast<std::string>(i));

	for (int i = 0; i < _numBins; i++)
		_features->addName("normalized histogram " + boost::lexical_cast<std::string>(i));
}

void
HistogramFeatureExtractor::updateOutputs() {

	foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds())
		_features->add(segment->getId(), getFeatures(*segment));

	foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations())
		_features->add(segment->getId(), getFeatures(*segment));

	foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches())
		_features->add(segment->getId(), getFeatures(*segment));
}

std::vector<double>
HistogramFeatureExtractor::getFeatures(const EndSegment& end) {

	std::vector<double> features(2*_numBins);

	std::vector<double> histogram = computeHistogram(*end.getSlice().get());

	for (int i = 0; i < _numBins; i++)
		features[i] = histogram[i];

	double sum = 0;
	for (int i = 0; i < _numBins; i++)
		sum += histogram[i];

	for (int i = 0; i < _numBins; i++)
		features[_numBins + i] = histogram[i]/sum;

	return features;
}

std::vector<double>
HistogramFeatureExtractor::getFeatures(const ContinuationSegment& continuation) {

	std::vector<double> features(2*_numBins);

	std::vector<double> sourceHistogram = computeHistogram(*continuation.getSourceSlice().get());
	std::vector<double> targetHistogram = computeHistogram(*continuation.getTargetSlice().get());

	for (int i = 0; i < _numBins; i++)
		features[i] = std::abs(sourceHistogram[i] - targetHistogram[i]);

	double sourceSum = 0;
	double targetSum = 0;
	for (int i = 0; i < _numBins; i++) {

		sourceSum += sourceHistogram[i];
		targetSum += targetHistogram[i];
	}

	for (int i = 0; i < _numBins; i++)
		features[_numBins + i] = std::abs(sourceHistogram[i]/sourceSum - targetHistogram[i]/targetSum);

	return features;
}

std::vector<double>
HistogramFeatureExtractor::getFeatures(const BranchSegment& branch) {

	std::vector<double> features(2*_numBins);

	std::vector<double> sourceHistogram  = computeHistogram(*branch.getSourceSlice().get());
	std::vector<double> targetHistogram1 = computeHistogram(*branch.getTargetSlice1().get());
	std::vector<double> targetHistogram2 = computeHistogram(*branch.getTargetSlice2().get());

	std::vector<double> targetHistogram = targetHistogram1;

	for (int i = 0; i < _numBins; i++)
		targetHistogram[i] += targetHistogram2[i];

	for (int i = 0; i < _numBins; i++)
		features[i] = std::abs(sourceHistogram[i] - targetHistogram[i]);

	double sourceSum = 0;
	double targetSum = 0;
	for (int i = 0; i < _numBins; i++) {

		sourceSum += sourceHistogram[i];
		targetSum += targetHistogram[i];
	}

	for (int i = 0; i < _numBins; i++)
		features[_numBins + i] = std::abs(sourceHistogram[i]/sourceSum - targetHistogram[i]/targetSum);

	return features;
}

std::vector<double>
HistogramFeatureExtractor::computeHistogram(const Slice& slice) {

	std::vector<double> histogram(_numBins, 0);

	unsigned int section = slice.getSection();

	Image& image = *(*_sections)[section];

	foreach (const util::point<unsigned int>& pixel, slice.getComponent()->getPixels()) {

		double value = image(pixel.x, pixel.y);

		unsigned int bin = std::min(_numBins - 1, (unsigned int)(value*_numBins));

		histogram[bin]++;
	}

	return histogram;
}
