#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <imageprocessing/ImageExtractor.h>
#include <sopnet/slices/SliceExtractor.h>
#include "GroundTruthExtractor.h"

logger::LogChannel groundtruthextractorlog("groundtruthextractorlog", "[GroundTruthExtractor] ");

GroundTruthExtractor::GroundTruthExtractor(int firstSection, int lastSection, bool addIntensityBoundaries) :
	_firstSection(firstSection),
	_lastSection(lastSection),
	_addIntensityBoundaries(addIntensityBoundaries) {

	registerInput(_groundTruthSections, "ground truth sections");
	registerOutput(_groundTruthSegments, "ground truth segments");
}

void
GroundTruthExtractor::updateOutputs() {

	unsigned int firstSection = (_firstSection >= 0 ? _firstSection : 0);
	unsigned int lastSection  = (_lastSection  >= 0 ? _lastSection  : _groundTruthSections->size() - 1);

	LOG_ALL(groundtruthextractorlog)
			<< "extacting groundtruth from sections "
			<< firstSection << " - " << lastSection
			<< std::endl;

	std::vector<Slices> slices = extractSlices(firstSection, lastSection);

	*_groundTruthSegments = findMinimalTrees(slices);
}

std::vector<Slices>
GroundTruthExtractor::extractSlices(int firstSection, int lastSection) {

	// create mser parameters suitable to extract ground-truth connected
	// components
	pipeline::Value<MserParameters> mserParameters;
	mserParameters->delta        = 1;
	mserParameters->minArea      = 50; // this is to avoid this tiny annotation that mess up the result
	mserParameters->maxArea      = 10000000;
	mserParameters->maxVariation = 100;
	mserParameters->minDiversity = 0;
	mserParameters->darkToBright = false;
	mserParameters->brightToDark = true;
	mserParameters->sameIntensityComponents = _addIntensityBoundaries; // only extract connected components of same intensity

	// create a section extractor to access the sections in the stack
	pipeline::Process<ImageExtractor> sectionExtractor;
	sectionExtractor->setInput(_groundTruthSections);

	// list of all slices for each section
	std::vector<Slices> slices;

	for (int section = firstSection; section <= lastSection; section++) {

		LOG_DEBUG(groundtruthextractorlog) << "extracting slices in section " << section << std::endl;

		// create a SliceExtractor
		pipeline::Process<SliceExtractor<unsigned short> > sliceExtractor(section);

		// give it the section it has to process and our mser parameters
		sliceExtractor->setInput("membrane", sectionExtractor->getOutput(section));
		sliceExtractor->setInput("mser parameters", mserParameters);

		// get the slices in the current section
		pipeline::Value<Slices> sectionSlices = sliceExtractor->getOutput("slices");
		slices.push_back(*sectionSlices);

		LOG_ALL(groundtruthextractorlog) << "found " << sectionSlices->size() << " slices" << std::endl;
	}

	return slices;
}

Segments
GroundTruthExtractor::findMinimalTrees(const std::vector<Slices>& slices) {

	// tree segments of all found neurons
	Segments segments;

	// all possible continuation segments by neuron label
	std::map<float, std::vector<ContinuationSegment> > links;

	// number of links to left and right for each slice
	std::map<unsigned int, unsigned int> linksLeft;
	std::map<unsigned int, unsigned int> linksRight;

	// fill links
	links = extractContinuations(slices);

	// for each neuron label
	typedef std::map<float, std::vector<ContinuationSegment> >::value_type pair_t;
	foreach (pair_t& pair, links) {

		float label = pair.first;
		std::vector<ContinuationSegment>& continuations = pair.second;

		LOG_ALL(groundtruthextractorlog) << "processing neuron label " << label << std::endl;

		// all currently connected slices
		std::set<unsigned int> connectedSlices;

		// sort continuations by overlap
		std::sort(continuations.begin(), continuations.end(), ContinuationComparator());

		// pick initial slice, put into connected slices
		unsigned int initialSlice = continuations.begin()->getSourceSlice()->getId();
		connectedSlices.insert(initialSlice);

		LOG_ALL(groundtruthextractorlog) << "initial slice is " << initialSlice << std::endl;

		// iterate
		bool allConnected = false;
		while (!allConnected) {

			bool foundOpenEdge = false;
			bool foundDisconnectedSlices = false;

			// find cheapest continuation for any connected slice to any not 
			// connected slice
			for (std::vector<ContinuationSegment>::const_iterator i = continuations.begin(); i != continuations.end(); i++) {

				unsigned int source = i->getSourceSlice()->getId();
				unsigned int target = i->getTargetSlice()->getId();

				int sourceTargetCount = connectedSlices.count(source) + connectedSlices.count(target);

				if (sourceTargetCount != 1) {

					if (sourceTargetCount == 0)
						foundDisconnectedSlices = true;

					continue;
				}

				LOG_ALL(groundtruthextractorlog)
						<< "next best continuation from connected to not-connected slice is "
						<< source << " -> "
						<< target << std::endl;

				foundOpenEdge = true;

				// put continuation in segments
				segments.add(boost::make_shared<ContinuationSegment>(*i));

				// count number of usages of involved slices
				if (i->getDirection() == Right) {

					linksLeft[target]++;
					linksRight[source]++;

				} else {

					linksRight[target]++;
					linksLeft[source]++;
				}

				// put new slice into connected slices
				connectedSlices.insert(target);
				connectedSlices.insert(source);
			}

			if (!foundOpenEdge && foundDisconnectedSlices) {

				LOG_USER(groundtruthextractorlog) << "Warning: ground-truth contains disconnected neuron with same label: " << label << std::endl;

				// add a new disconnected slice to the set of connected slices 
				// to continue growing the tree
				for (std::vector<ContinuationSegment>::const_iterator i = continuations.begin(); i != continuations.end(); i++) {

					if (connectedSlices.count(i->getSourceSlice()->getId()) == 0) {

						connectedSlices.insert(i->getSourceSlice()->getId());
						break;
					}

					if (connectedSlices.count(i->getTargetSlice()->getId()) == 0) {

						connectedSlices.insert(i->getTargetSlice()->getId());
						break;
					}
				}

				continue;
			}

			if (!foundOpenEdge)
				allConnected = true;
		}
	}

	// postprocessing, for each slice
	for (unsigned int i = 0; i < slices.size(); i++)
		foreach (boost::shared_ptr<Slice> slice, slices[i]) {

			// if slice is used from one side only, add an end segment
			if (linksLeft[slice->getId()] == 0)
				segments.add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Left, slice));
			if (linksRight[slice->getId()] == 0)
				segments.add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Right, slice));
		}

	return segments;
}

std::map<float, std::vector<ContinuationSegment> >
GroundTruthExtractor::extractContinuations(const std::vector<Slices>& slices) {

	std::map<float, std::vector<ContinuationSegment> > continuations;

	for (unsigned int i = 0; i < slices.size() - 1; i++) {

		LOG_ALL(groundtruthextractorlog) << "extracting potential continuations between " << i << " and " << (i+1) << std::endl;

		foreach (boost::shared_ptr<Slice> leftSlice, slices[i]) {

			float leftValue = leftSlice->getComponent()->getValue();

			foreach (boost::shared_ptr<Slice> rightSlice, slices[i+1]) {

				float rightValue = rightSlice->getComponent()->getValue();

				if (leftValue == rightValue) {

					LOG_ALL(groundtruthextractorlog) << "found a potential continuation" << std::endl;

					continuations[leftValue].push_back(
							ContinuationSegment(
									Segment::getNextSegmentId(),
									Right,
									leftSlice,
									rightSlice));
				}
			}
		}
	}

	return continuations;
}
