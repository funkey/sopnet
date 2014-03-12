#include <limits>

#include <util/ProgramOptions.h>
#include <util/rect.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "GoldStandardExtractor.h"

logger::LogChannel goldstandardextractorlog("goldstandardextractorlog", "[GoldStandardExtractor] ");

util::ProgramOption optionMaxGoldStandardDistance(
		util::_module           = "sopnet",
		util::_long_name        = "maxGoldStandardDistance",
		util::_description_text = "The distance within correspondences between the ground-truth and the gold-standard are sought.",
		util::_default_value    = 100);

GoldStandardExtractor::GoldStandardExtractor() :
	_goldStandard(new Segments()),
	_negativeSamples(new Segments()),
	_groundTruthScore(new std::map<unsigned int, double>()),
	_overlap(false, false) {

	_maxEndDistance          = optionMaxGoldStandardDistance;
	_maxContinuationDistance = optionMaxGoldStandardDistance;
	_maxBranchDistance       = optionMaxGoldStandardDistance;

	registerInput(_groundTruth, "ground truth");
	registerInput(_allSegments, "all segments");

	registerOutput(_goldStandard, "gold standard");
	registerOutput(_negativeSamples, "negative samples");
	registerOutput(_groundTruthScore, "ground truth score");
}

void
GoldStandardExtractor::updateOutputs() {

	LOG_DEBUG(goldstandardextractorlog) << "searching for best-fitting segments to ground truth" << std::endl;

	_goldStandard->clear();
	_negativeSamples->clear();

	LOG_DEBUG(goldstandardextractorlog)
			<< "ground truth contains segments in "
			<< _groundTruth->getNumInterSectionIntervals()
			<< " inter-section intervalls" << std::endl;

	for (unsigned int interval = 0; interval < _groundTruth->getNumInterSectionIntervals(); interval++) {

		LOG_DEBUG(goldstandardextractorlog) << "finding correspondences in inter-section interval " << interval << std::endl;

		findGoldStandard(interval);
	}
}

void
GoldStandardExtractor::findGoldStandard(unsigned int interval) {

	// set of pairs of segments with a similarity value
	std::vector<Pair<EndSegment> >          endPairs;
	std::vector<Pair<ContinuationSegment> > continuationPairs;
	std::vector<Pair<BranchSegment> >       branchPairs;

	LOG_ALL(goldstandardextractorlog)
			<< "collecting slices of all extracted segments..."
			<< std::endl;

	// all remaining ground-truth slices that are not used by a segment
	_remainingSlices =
			collectSlices(
					_groundTruth->getEnds(interval),
					_groundTruth->getContinuations(interval),
					_groundTruth->getBranches(interval));

	LOG_ALL(goldstandardextractorlog)
			<< "finding pairs of ground-truth/extracted end segments..."
			<< std::endl;

	// find all end pairs and evaluate their similarity
	foreach (boost::shared_ptr<EndSegment> end1, _groundTruth->getEnds(interval))
		foreach (boost::shared_ptr<EndSegment> end2, _allSegments->findEnds(end1, _maxEndDistance)) {

			if (end1->getDirection() != end2->getDirection())
				continue;

			// normalized, non-aligned set difference
			float similarity = normalizedSetDifference(*end1->getSlice(), *end2->getSlice());

			endPairs.push_back(Pair<EndSegment>(similarity, end1, end2));
			(*_groundTruthScore)[end2->getId()] = similarity;
		}

	LOG_ALL(goldstandardextractorlog)
			<< "finding pairs of ground-truth/extracted continuation segments..."
			<< std::endl;

	// find all continuation pairs and evaluate their similarity
	foreach (boost::shared_ptr<ContinuationSegment> continuation1, _groundTruth->getContinuations(interval))
		foreach (boost::shared_ptr<ContinuationSegment> continuation2, _allSegments->findContinuations(continuation1, _maxContinuationDistance)) {

			// mean of the two normalized set differences
			float similarity =
					(normalizedSetDifference(*continuation1->getSourceSlice(), *continuation2->getSourceSlice()) +
					 normalizedSetDifference(*continuation1->getTargetSlice(), *continuation2->getTargetSlice()))/2.0;

			continuationPairs.push_back(Pair<ContinuationSegment>(similarity, continuation1, continuation2));
			(*_groundTruthScore)[continuation2->getId()] = similarity;
		}

	LOG_ALL(goldstandardextractorlog)
			<< "finding pairs of ground-truth/extracted branch segments..."
			<< std::endl;

	// find all branch pairs and evaluate their similarity
	foreach (boost::shared_ptr<BranchSegment> branch1, _groundTruth->getBranches(interval))
		foreach (boost::shared_ptr<BranchSegment> branch2, _allSegments->findBranches(branch1, _maxBranchDistance)) {

			// mean of the three normalized set differences
			float similarity =
					(normalizedSetDifference(*branch1->getSourceSlice(),  *branch2->getSourceSlice()) +
					 normalizedSetDifference(*branch1->getTargetSlice1(), *branch2->getTargetSlice1()) +
					 normalizedSetDifference(*branch1->getTargetSlice2(), *branch2->getTargetSlice2()))/3.0;

			branchPairs.push_back(Pair<BranchSegment>(similarity, branch1, branch2));
			(*_groundTruthScore)[branch2->getId()] = similarity;
		}

	LOG_ALL(goldstandardextractorlog) << "sorting found paris..." << std::endl;

	std::sort(endPairs.begin(), endPairs.end());
	std::sort(continuationPairs.begin(), continuationPairs.end());
	std::sort(branchPairs.begin(), branchPairs.end());

	LOG_DEBUG(goldstandardextractorlog) << "greedily accepting best matches..." << std::endl;

	// all assigned ground truth segments
	std::set<const Segment*> assignedGtSegments;

	LOG_ALL(goldstandardextractorlog) << "  creating a lookup-tables from slices to segments..." << std::endl;

	// create a mapping from slices to segments they are involved in to find the
	// negative samples
	_endSegments          = slicesToSegments(_allSegments->getEnds(interval));
	_continuationSegments = slicesToSegments(_allSegments->getContinuations(interval));
	_branchSegments       = slicesToSegments(_allSegments->getBranches(interval));

	unsigned int nextEnd = 0;
	unsigned int nextContinuation = 0;
	unsigned int nextBranch = 0;

	while (
			nextEnd          < endPairs.size()          ||
			nextContinuation < continuationPairs.size() ||
			nextBranch       < branchPairs.size()) {

		LOG_ALL(goldstandardextractorlog) << "  searching for next best segment pair..." << std::endl;

		// find the next best pair
		float endSimilarity          = (nextEnd          < endPairs.size()          ? endPairs[nextEnd].similarity                   : std::numeric_limits<float>::infinity());
		float continuationSimilarity = (nextContinuation < continuationPairs.size() ? continuationPairs[nextContinuation].similarity : std::numeric_limits<float>::infinity());
		float branchSimilarity       = (nextBranch       < branchPairs.size()       ? branchPairs[nextBranch].similarity             : std::numeric_limits<float>::infinity());

		// end is the winner
		if (endSimilarity <= std::min(continuationSimilarity, branchSimilarity)) {

			LOG_ALL(goldstandardextractorlog) << "  it's an end pair..." << std::endl;

			if (endSimilarity >= 0.9) {

				LOG_ALL(goldstandardextractorlog) << "similarity value got too worse -- stop searching for further pairs" << std::endl;
				break;
			}

			boost::shared_ptr<EndSegment> gtEnd = endPairs[nextEnd].segment1;
			boost::shared_ptr<EndSegment> gsEnd = endPairs[nextEnd].segment2;

			probe(gtEnd, gsEnd);

			nextEnd++;
		}

		// continuation is the winner
		if (continuationSimilarity <= std::min(endSimilarity, branchSimilarity)) {

			LOG_ALL(goldstandardextractorlog) << "  it's a continuation pair..." << std::endl;

			if (continuationSimilarity >= 0.9) {

				LOG_ALL(goldstandardextractorlog) << "similarity value got too worse -- stop searching for further pairs" << std::endl;
				break;
			}

			boost::shared_ptr<ContinuationSegment> gtContinuation = continuationPairs[nextContinuation].segment1;
			boost::shared_ptr<ContinuationSegment> gsContinuation = continuationPairs[nextContinuation].segment2;

			probe(gtContinuation, gsContinuation);

			nextContinuation++;
		}

		// branch is the winner
		if (branchSimilarity <= std::min(endSimilarity, continuationSimilarity)) {

			LOG_ALL(goldstandardextractorlog) << "  it's a branch pair..." << std::endl;

			if (branchSimilarity >= 0.9) {

				LOG_ALL(goldstandardextractorlog) << "similarity value got too worse -- stop searching for further pairs" << std::endl;
				break;
			}

			boost::shared_ptr<BranchSegment> gtBranch = branchPairs[nextBranch].segment1;
			boost::shared_ptr<BranchSegment> gsBranch = branchPairs[nextBranch].segment2;

			probe(gtBranch, gsBranch);

			nextBranch++;
		}

		LOG_ALL(goldstandardextractorlog) << "  there are still " << _remainingSlices.size() << " ground-truth slices unexplained" << std::endl;

		// if found segments for all slices, we are done
		if (_remainingSlices.size() == 0)
			break;
	}

	LOG_DEBUG(goldstandardextractorlog) << "remaining grount-truth slices : " << _remainingSlices.size() << std::endl;
}

template <typename SegmentType>
void
GoldStandardExtractor::probe(
		boost::shared_ptr<SegmentType> gtSegment,
		boost::shared_ptr<SegmentType> gsSegment) {

		std::vector<const Slice*> gtSlices = getSlices(gtSegment);
		std::vector<const Slice*> gsSlices = getSlices(gsSegment);

		// check if a segment for any of these slices has been found already
		foreach (const Slice* slice, gtSlices)
			if (_remainingSlices.count(slice) == 0) {

				// this is a negative sample
				(*_groundTruthScore)[gsSegment->getId()] = -1;
				return;
			}

		// otherwise, we can accept the gsSegment to be part of the gold
		// standard
		_goldStandard->add(gsSegment);

		// remove all the slices that are involved from the remainig slices
		foreach (const Slice* slice, gtSlices)
			_remainingSlices.erase(slice);

		// find all conflicting segments to the found one and add them as
		// negative samples
		foreach (const Slice* slice, gsSlices) {

			foreach (boost::shared_ptr<EndSegment> negative, _endSegments[slice])
				if (negative != boost::static_pointer_cast<Segment>(gsSegment)) {

					(*_groundTruthScore)[negative->getId()] = -1;
					_negativeSamples->add(negative);
				}

			foreach (boost::shared_ptr<ContinuationSegment> negative, _continuationSegments[slice])
				if (negative != boost::static_pointer_cast<Segment>(gsSegment)) {

					(*_groundTruthScore)[negative->getId()] = -1;
					_negativeSamples->add(negative);
				}

			foreach (boost::shared_ptr<BranchSegment> negative, _branchSegments[slice])
				if (negative != boost::static_pointer_cast<Segment>(gsSegment)) {

					(*_groundTruthScore)[negative->getId()] = -1;
					_negativeSamples->add(negative);
				}
		}
}

std::vector<const Slice*>
GoldStandardExtractor::getSlices(boost::shared_ptr<EndSegment> end) {

	std::vector<const Slice*> slices;

	slices.push_back(end->getSlice().get());

	return slices;
}

std::vector<const Slice*>
GoldStandardExtractor::getSlices(boost::shared_ptr<ContinuationSegment> continuation) {

	std::vector<const Slice*> slices;

	slices.push_back(continuation->getSourceSlice().get());
	slices.push_back(continuation->getTargetSlice().get());

	return slices;
}

std::vector<const Slice*>
GoldStandardExtractor::getSlices(boost::shared_ptr<BranchSegment> branch) {

	std::vector<const Slice*> slices;

	slices.push_back(branch->getSourceSlice().get());
	slices.push_back(branch->getTargetSlice1().get());
	slices.push_back(branch->getTargetSlice2().get());

	return slices;
}

std::set<const Slice*>
GoldStandardExtractor::collectSlices(
		const std::vector<boost::shared_ptr<EndSegment> >&          endSegments,
		const std::vector<boost::shared_ptr<ContinuationSegment> >& continuationSegments,
		const std::vector<boost::shared_ptr<BranchSegment> >&       branchSegments) {

	std::set<const Slice*> slices;

	// collect all slices that are used in the allEndSegments
	foreach (boost::shared_ptr<EndSegment> segment, endSegments)
		slices.insert(segment->getSlice().get());

	// collect all slices that are used in the allContinuationSegments
	foreach (boost::shared_ptr<ContinuationSegment> segment, continuationSegments) {

		slices.insert(segment->getSourceSlice().get());
		slices.insert(segment->getTargetSlice().get());
	}

	// collect all slices that are used in the allSegments
	foreach (boost::shared_ptr<BranchSegment> segment, branchSegments) {

		slices.insert(segment->getSourceSlice().get());
		slices.insert(segment->getTargetSlice1().get());
		slices.insert(segment->getTargetSlice2().get());
	}

	return slices;
}

template <typename SegmentType>
std::map<
		const Slice*,
		std::vector<boost::shared_ptr<SegmentType> >
>
GoldStandardExtractor::slicesToSegments(const std::vector<boost::shared_ptr<SegmentType> >& segments) {

	std::map<const Slice*, std::vector<boost::shared_ptr<SegmentType> > > slices;

	// collect all slices that are used in the segments
	foreach (boost::shared_ptr<SegmentType> segment, segments)
		foreach (const Slice* slice, getSlices(segment))
			slices[slice].push_back(segment);

	return slices;
}

double
GoldStandardExtractor::normalizedSetDifference(const Slice& slice1, const Slice& slice2) {

	unsigned int overlap = _overlap(slice1, slice2);
	unsigned int size1   = slice1.getComponent()->getSize();
	unsigned int size2   = slice2.getComponent()->getSize();

	return ((size1 - overlap) + (size2 - overlap))/(size1 + size2);
}
