#include <util/rect.hpp>
#include <imageprocessing/ConnectedComponent.h>
#include <GoldStandardExtractor.h>
#include "EndSegment.h"
#include "ContinuationSegment.h"
#include "BranchSegment.h"

logger::LogChannel goldstandardextractorlog("goldstandardextractorlog", "[GoldStandardExtractor] ");

GoldStandardExtractor::GoldStandardExtractor() :
	_goldStandard(boost::make_shared<Segments>()),
	_negativeSamples(boost::make_shared<Segments>()) {

	registerInput(_groundTruth, "ground truth");
	registerInput(_allSegments, "all segments");

	registerOutput(_goldStandard, "gold standard");
	registerOutput(_negativeSamples, "negative samples");
}

void
GoldStandardExtractor::updateOutputs() {

	LOG_DEBUG(goldstandardextractorlog) << "searching for best-fitting segments to ground truth" << std::endl;

	_goldStandard->clear();
	_allSegmentIds.clear();
	_segmentConstraints.clear();

	// create a lookup table for all found segment ids to the segments
	foreach (boost::shared_ptr<Segment> segment, *_allSegments)
		_allSegmentIds[segment->getId()] = segment;

	// seperate ground truth segments by their inter-section interval and type
	SeparateVisitor separateGtVisitor;
	foreach (boost::shared_ptr<Segment> segment, *_groundTruth)
		segment->accept(separateGtVisitor);

	// seperate all found segments by their inter-section interval and type
	SeparateVisitor separateAllVisitor;
	foreach (boost::shared_ptr<Segment> segment, *_allSegments)
		segment->accept(separateAllVisitor);

	LOG_DEBUG(goldstandardextractorlog)
			<< "ground truth contains segments in "
			<< separateGtVisitor.getNumInterSectionIntervals()
			<< " inter-section intervalls" << std::endl;

	for (unsigned int interval = 0; interval < separateGtVisitor.getNumInterSectionIntervals(); interval++) {

		LOG_DEBUG(goldstandardextractorlog) << "finding correspondences in inter-section interval " << interval << std::endl;

		std::vector<const Segment*> segments;
		std::vector<const Segment*> negativeSamples;
		
		boost::tie(segments, negativeSamples) = findGoldStandard(
				separateGtVisitor.getEndSegments(interval),
				separateGtVisitor.getContinuationSegments(interval),
				separateGtVisitor.getBranchSegments(interval),
				separateAllVisitor.getEndSegments(interval),
				separateAllVisitor.getContinuationSegments(interval),
				separateAllVisitor.getBranchSegments(interval));

		foreach (const Segment* segment, segments)
			if (_allSegmentIds.count(segment->getId()))
				_goldStandard->add(_allSegmentIds[segment->getId()]);
			else
				LOG_ERROR(goldstandardextractorlog) << "I found a segment that I didn't know before!" << std::endl;

		foreach (const Segment* segment, negativeSamples)
			if (_allSegmentIds.count(segment->getId()))
				_negativeSamples->add(_allSegmentIds[segment->getId()]);
			else
				LOG_ERROR(goldstandardextractorlog) << "I found a segment that I didn't know before!" << std::endl;
	}
}

std::pair<std::vector<const Segment*>, std::vector<const Segment*> >
GoldStandardExtractor::findGoldStandard(
		const std::vector<const EndSegment*>&          groundTruthEndSegments,
		const std::vector<const ContinuationSegment*>& groundTruthContinuationSegments,
		const std::vector<const BranchSegment*>&       groundTruthBranchSegments,
		const std::vector<const EndSegment*>&          allEndSegments,
		const std::vector<const ContinuationSegment*>& allContinuationSegments,
		const std::vector<const BranchSegment*>&       allBranchSegments) {

	// set of pairs of segments with a similarity value
	typedef std::set<std::pair<float, std::pair<const Segment*, const Segment*> > > pairs_type;
	pairs_type pairs;

	// all remaining slices that are not used by a segment
	std::set<const Slice*> remainingSlices =
			collectSlices(
					allEndSegments,
					allContinuationSegments,
					allBranchSegments);

	// find all pairs and evaluate their similarity
	foreach (const EndSegment* segment1, groundTruthEndSegments) {

		SimilarityVisitor similarityVisitor(segment1);

		foreach (const Segment* segment2, allEndSegments) {

			if (segment1->getDirection() != segment2->getDirection())
				continue;

			segment2->accept(similarityVisitor);

			float similarity = similarityVisitor.getSimilarity();

			pairs.insert(std::make_pair(similarity, std::make_pair(segment1, segment2)));
		}
	}
	foreach (const ContinuationSegment* segment1, groundTruthContinuationSegments) {

		SimilarityVisitor similarityVisitor(segment1);

		foreach (const Segment* segment2, allContinuationSegments) {

			if (segment1->getDirection() != segment2->getDirection())
				continue;

			segment2->accept(similarityVisitor);

			float similarity = similarityVisitor.getSimilarity();

			pairs.insert(std::make_pair(similarity, std::make_pair(segment1, segment2)));
		}
	}
	foreach (const BranchSegment* segment1, groundTruthBranchSegments) {

		SimilarityVisitor similarityVisitor(segment1);

		foreach (const Segment* segment2, allBranchSegments) {

			if (segment1->getDirection() != segment2->getDirection())
				continue;

			segment2->accept(similarityVisitor);

			float similarity = similarityVisitor.getSimilarity();

			pairs.insert(std::make_pair(similarity, std::make_pair(segment1, segment2)));
		}
	}

	LOG_DEBUG(goldstandardextractorlog) << "greedily accepting best matches..." << std::endl;

	// all assigned ground truth segments
	std::set<const Segment*> assignedGtSegments;

	// the corresponding found segments
	std::vector<const Segment*> goldStandard;
	goldStandard.reserve(
			groundTruthEndSegments.size() +
			groundTruthContinuationSegments.size() +
			groundTruthBranchSegments.size());

	// all segments that conflict with the gold standard
	std::vector<const Segment*> negativeSamples;
	negativeSamples.reserve(
			allEndSegments.size() +
			allContinuationSegments.size() +
			allBranchSegments.size());

	// create a mapping from slices to segments they are involved in to find the
	// negative samples
	std::map<const Slice*, std::vector<const Segment*> > slicesToSegments =
			createSlicesToSegmentsMap(
					allEndSegments,
					allContinuationSegments,
					allBranchSegments);

	foreach (pairs_type::value_type pair, pairs) {

		// the next best pair of segments
		float similarity = pair.first;
		const Segment* gtSegment = pair.second.first;
		const Segment* gsSegment = pair.second.second;

		// get the slices that are involved in the gsSegment
		SliceCollectorVisitor gsSliceCollector;
		gsSegment->accept(gsSliceCollector);

		// check if the ground truth segment has been assigned already
		if (assignedGtSegments.count(gtSegment))
			continue;

		// check if a segment for any of these slices has been found already
		bool notValid = false;

		foreach (const Slice* slice, gsSliceCollector.getSlices())
			if (remainingSlices.count(slice) == 0) {

				notValid = true;
				break;
			}

		// if so, continue with the next best pair of segments
		if (notValid)
			continue;

		// otherwise, we can accept the gsSegment to be part of the gold
		// standard
		goldStandard.push_back(gsSegment);

		// remember that we assigned this gtSegment
		assignedGtSegments.insert(gtSegment);

		// remove all the slices that are involved from the remainig slices
		foreach (const Slice* slice, gsSliceCollector.getSlices())
			remainingSlices.erase(slice);

		// find all conflicting segments to the found one and add them as
		// negative samples
		foreach (const Slice* slice, gsSliceCollector.getSlices())
			foreach (const Segment* negative, slicesToSegments[slice])
				if (negative != gsSegment)
					negativeSamples.push_back(negative);

		// if found segments for all slices, we are done
		if (remainingSlices.size() == 0)
			break;
	}

	LOG_DEBUG(goldstandardextractorlog) << "remaining slices             : " << remainingSlices.size() << std::endl;
	LOG_DEBUG(goldstandardextractorlog) << "found gold standard segments : " << goldStandard.size() << std::endl;

	LOG_DEBUG(goldstandardextractorlog) << "searching for negative samples..." << std::endl;


	return std::make_pair(goldStandard, negativeSamples);
}

std::set<const Slice*>
GoldStandardExtractor::collectSlices(
		const std::vector<const EndSegment*>&          endSegments,
		const std::vector<const ContinuationSegment*>& continuationSegments,
		const std::vector<const BranchSegment*>&       branchSegments) {

	std::set<const Slice*> slices;

	// collect all slices that are used in the allEndSegments
	foreach (const EndSegment* segment, endSegments)
		slices.insert(segment->getSlice().get());

	// collect all slices that are used in the allContinuationSegments
	foreach (const ContinuationSegment* segment, continuationSegments) {

		slices.insert(segment->getSourceSlice().get());
		slices.insert(segment->getTargetSlice().get());
	}

	// collect all slices that are used in the allSegments
	foreach (const BranchSegment* segment, branchSegments) {

		slices.insert(segment->getSourceSlice().get());
		slices.insert(segment->getTargetSlice1().get());
		slices.insert(segment->getTargetSlice2().get());
	}

	return slices;
}

std::map<const Slice*, std::vector<const Segment*> >
GoldStandardExtractor::createSlicesToSegmentsMap(
		const std::vector<const EndSegment*>&          endSegments,
		const std::vector<const ContinuationSegment*>& continuationSegments,
		const std::vector<const BranchSegment*>&       branchSegments) {

	std::map<const Slice*, std::vector<const Segment*> > slices;

	// collect all slices that are used in the allEndSegments
	foreach (const EndSegment* segment, endSegments)
		slices[segment->getSlice().get()].push_back(segment);

	// collect all slices that are used in the allContinuationSegments
	foreach (const ContinuationSegment* segment, continuationSegments) {

		slices[segment->getSourceSlice().get()].push_back(segment);
		slices[segment->getTargetSlice().get()].push_back(segment);
	}

	// collect all slices that are used in the allSegments
	foreach (const BranchSegment* segment, branchSegments) {

		slices[segment->getSourceSlice().get()].push_back(segment);
		slices[segment->getTargetSlice1().get()].push_back(segment);
		slices[segment->getTargetSlice2().get()].push_back(segment);
	}

	return slices;
}

void
GoldStandardExtractor::parseConstraint(const LinearConstraint& constraint) {

	if (constraint.getRelation() != Equal && constraint.getRelation() != LessEqual)
		return;

	if (constraint.getValue() != 1)
		return;

	typedef std::map<unsigned int, double>::value_type pair_type;
	foreach (pair_type pair, constraint.getCoefficients())
		_segmentConstraints[pair.first].push_back(&constraint);
}

///////////////////////
// SimilarityVisitor //
///////////////////////

GoldStandardExtractor::SimilarityVisitor::SimilarityVisitor(const Segment* compare) :
	_compare(compare) {}

float
GoldStandardExtractor::SimilarityVisitor::getSimilarity() {

	return _similarity;
}

void
GoldStandardExtractor::SimilarityVisitor::visit(const EndSegment& end) {

	_similarity = getSimilarity((const EndSegment&)(*_compare), end);
}

void
GoldStandardExtractor::SimilarityVisitor::visit(const ContinuationSegment& continuation) {

	_similarity = getSimilarity((const ContinuationSegment&)(*_compare), continuation);
}

void
GoldStandardExtractor::SimilarityVisitor::visit(const BranchSegment& branch) {

	_similarity = getSimilarity((const BranchSegment&)(*_compare), branch);
}

float
GoldStandardExtractor::SimilarityVisitor::getSimilarity(const EndSegment& end1, const EndSegment& end2) {

	return setDifference(*end1.getSlice(), *end2.getSlice());
}

float
GoldStandardExtractor::SimilarityVisitor::getSimilarity(const ContinuationSegment& continuation1, const ContinuationSegment& continuation2) {

	return
			setDifference(*continuation1.getSourceSlice(), *continuation2.getSourceSlice()) +
			setDifference(*continuation1.getTargetSlice(), *continuation2.getTargetSlice());
}

float
GoldStandardExtractor::SimilarityVisitor::getSimilarity(const BranchSegment& branch1, const BranchSegment& branch2) {

	return
			setDifference(*branch1.getSourceSlice(),  *branch2.getSourceSlice())  +
			setDifference(*branch1.getTargetSlice1(), *branch2.getTargetSlice1()) +
			setDifference(*branch1.getTargetSlice2(), *branch2.getTargetSlice2());
}

unsigned int
GoldStandardExtractor::SimilarityVisitor::setDifference(const Slice& slice1, const Slice& slice2) {

	const util::rect<double>& bb1 = slice1.getComponent()->getBoundingBox();
	util::point<unsigned int> offset1(static_cast<unsigned int>(bb1.minX), static_cast<unsigned int>(bb1.minY));
	util::point<unsigned int> size1(static_cast<unsigned int>(bb1.width() + 2), static_cast<unsigned int>(bb1.height() + 2));

	std::vector<bool> pixels1(size1.x*size1.y, false);

	foreach (const util::point<unsigned int>& pixel, slice1.getComponent()->getPixels()) {

		unsigned int x = pixel.x - offset1.x;
		unsigned int y = pixel.y - offset1.y;

		pixels1[x + y*size1.x] = true;
	}

	unsigned int different = 0;

	foreach (const util::point<unsigned int>& pixel, slice2.getComponent()->getPixels()) {

		unsigned int x = pixel.x - offset1.x;
		unsigned int y = pixel.y - offset1.y;

		if (x < 0 || x >= size1.x || y < 0 || y >= size1.y || pixels1[x + y*size1.x] != true)
			different++;
	}

	return different;
}

///////////////////////////
// SliceCollectorVisitor //
///////////////////////////

GoldStandardExtractor::SliceCollectorVisitor::SliceCollectorVisitor() {

	_slices.reserve(3);
}

void
GoldStandardExtractor::SliceCollectorVisitor::visit(const EndSegment& end) {

	_slices.push_back(end.getSlice().get());
}

void
GoldStandardExtractor::SliceCollectorVisitor::visit(const ContinuationSegment& continuation) {

	_slices.push_back(continuation.getSourceSlice().get());
	_slices.push_back(continuation.getTargetSlice().get());
}

void
GoldStandardExtractor::SliceCollectorVisitor::visit(const BranchSegment& branch) {

	_slices.push_back(branch.getSourceSlice().get());
	_slices.push_back(branch.getTargetSlice2().get());
	_slices.push_back(branch.getTargetSlice1().get());
}

const std::vector<const Slice*>&
GoldStandardExtractor::SliceCollectorVisitor::getSlices() {

	return _slices;
}

/////////////////////
// SeperateVisitor //
/////////////////////

GoldStandardExtractor::SeparateVisitor::SeparateVisitor() :
	_numIntervals(0) {}

void
GoldStandardExtractor::SeparateVisitor::visit(const EndSegment& end) {

	unsigned int interval = end.getSlice()->getSection() + (end.getDirection() == Right ? 1 : 0);

	_numIntervals = std::max(_numIntervals, interval + 1);

	_endSegments[interval].push_back(&end);
}

void
GoldStandardExtractor::SeparateVisitor::visit(const ContinuationSegment& continuation) {

	unsigned int interval = continuation.getSourceSlice()->getSection() + (continuation.getDirection() == Right ? 1 : 0);

	_numIntervals = std::max(_numIntervals, interval + 1);

	_continuationSegments[interval].push_back(&continuation);
}

void
GoldStandardExtractor::SeparateVisitor::visit(const BranchSegment& branch) {

	unsigned int interval = branch.getSourceSlice()->getSection() + (branch.getDirection() == Right ? 1 : 0);

	_numIntervals = std::max(_numIntervals, interval + 1);

	_branchSegments[interval].push_back(&branch);
}

unsigned int
GoldStandardExtractor::SeparateVisitor::getNumInterSectionIntervals() {

	return _numIntervals;
}

const std::vector<const EndSegment*>&
GoldStandardExtractor::SeparateVisitor::getEndSegments(unsigned int interval) {

	return _endSegments[interval];
}

const std::vector<const ContinuationSegment*>&
GoldStandardExtractor::SeparateVisitor::getContinuationSegments(unsigned int interval) {

	return _continuationSegments[interval];
}

const std::vector<const BranchSegment*>&
GoldStandardExtractor::SeparateVisitor::getBranchSegments(unsigned int interval) {

	return _branchSegments[interval];
}
