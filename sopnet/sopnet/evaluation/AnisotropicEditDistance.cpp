#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/helpers.hpp>
#include "AnisotropicEditDistance.h"

logger::LogChannel resultevaluatorlog("resultevaluatorlog", "[AnisotropicEditDistance] ");

util::ProgramOption optionEvaluatinMinOverlap(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "minOverlap",
		util::_description_text = "The minimal normalized overlap between a result and ground-truth slice to consider them as a match.");

AnisotropicEditDistance::AnisotropicEditDistance(double minOverlap) :
	_errors(new AnisotropicEditDistanceErrors()),
	_overlap(true /* normalize */, false /* don't align */),
	_minOverlap(optionEvaluatinMinOverlap ? optionEvaluatinMinOverlap : minOverlap) {

	registerInput(_result, "result");
	registerInput(_groundTruth, "ground truth");
	registerOutput(_errors, "errors");
}

void
AnisotropicEditDistance::updateOutputs() {

	LOG_DEBUG(resultevaluatorlog) << "updating outputs..." << std::endl;

	_numSections = std::max((int)_result->getNumInterSectionIntervals(), (int)_groundTruth->getNumInterSectionIntervals());

	LOG_DEBUG(resultevaluatorlog) << "compute slice errors for " << _numSections << " sections" << std::endl;

	if (_numSections == 0)
		return;

	// First, find all slices in the result and ground-truth.
	findAllSlicesAndLinks();

	// all mappings of all sections
	std::vector<Mappings> allMappings;

	// pointers from any mapping of any section to the mapping in the previous 
	// section with minimal accumulated slice error
	std::vector<std::map<int, int> > bestPreviousMapping;

	// all minimal sliceErrors until a section and a mapping of this section
	std::vector<std::vector<AnisotropicEditDistanceErrors> > accumulatedSliceErrors;

	// For each section...
	for (unsigned int section = 0; section < _numSections; section++) {

		LOG_DEBUG(resultevaluatorlog) << "processing section " << section << std::endl;

		// Get all possible mappings of result to ground truth.
		allMappings.push_back(getAllMappings(section));

		Mappings& currentMappings = allMappings[section];

		// new map of mappings to mappings for current section
		bestPreviousMapping.push_back(std::map<int, int>());

		if (section == 0) {

			LOG_ALL(resultevaluatorlog) << "calculating slice errors of first section" << std::endl;

			// The slice errors of the first section consist only of the 
			// intra-section slice errors.
			std::vector<AnisotropicEditDistanceErrors> firstSliceErrors;

			foreach (Mapping& mapping, currentMappings)
				firstSliceErrors.push_back(getIntraSliceErrors(mapping, 0));

			accumulatedSliceErrors.push_back(firstSliceErrors);

			LOG_ALL(resultevaluatorlog) << "section 0: " << firstSliceErrors << std::endl;

			continue;
		}

		// new vector of slice errors for current section
		accumulatedSliceErrors.push_back(std::vector<AnisotropicEditDistanceErrors>(currentMappings.size(), AnisotropicEditDistanceErrors()));

		Mappings& previousMappings = allMappings[section - 1];

		// Compute slice errors of all mappings of current section when combined 
		// with any mapping of previous section.

		std::map<int, std::map<int, AnisotropicEditDistanceErrors> > sliceErrors;

		LOG_DEBUG(resultevaluatorlog)
				<< "computing slice errors for all " << currentMappings.size() << "x"
				<< previousMappings.size() << " combinations" << std::endl;

		for (unsigned int i = 0; i < currentMappings.size(); i++)
			for (unsigned int j = 0; j < previousMappings.size(); j++)
				sliceErrors[i][j] = getSliceErrors(currentMappings[i], previousMappings[j], section);

		// For each mapping of the current section, get the minimal number of 
		// slice errors up to the current section, remember which mapping in the 
		// previous section was involved.

		LOG_ALL(resultevaluatorlog) << "searching for minimal accumulated slice errors" << std::endl;

		for (unsigned int i = 0; i < currentMappings.size(); i++) {

			int numSliceErrors = -1;

			for (unsigned int j = 0; j < previousMappings.size(); j++) {

				// the sliceError until j in the previous section
				const AnisotropicEditDistanceErrors& previousSliceErrors = accumulatedSliceErrors[section-1][j];

				// the sliceError until i when going over j
				AnisotropicEditDistanceErrors currentSliceErrors = sliceErrors[i][j] + previousSliceErrors;

				// remember the best j
				if (currentSliceErrors.total() < numSliceErrors || numSliceErrors == -1) {

					numSliceErrors = currentSliceErrors.total();
					bestPreviousMapping[section][i] = j;
					accumulatedSliceErrors[section][i] = currentSliceErrors;
				}
			}
		}

		LOG_ALL(resultevaluatorlog) << "section " << section << ": " << accumulatedSliceErrors[section] << std::endl;

		LOG_ALL(resultevaluatorlog) << "done with section " << section << std::endl;
	}

	// Choose the mapping with the lowest acummulated slice error.

	AnisotropicEditDistanceErrors minSliceErrors;
	int bestMapping = -1;

	LOG_DEBUG(resultevaluatorlog) << "walking backwards to find sequence of optimal mappings" << std::endl;

	for (unsigned int i = 0; i < allMappings[_numSections-1].size(); i++) {

		if (accumulatedSliceErrors[_numSections-1][i].total() < minSliceErrors.total() || bestMapping == -1) {

			minSliceErrors   = accumulatedSliceErrors[_numSections-1][i];
			bestMapping = i;
		}
	}

	// Follow shortest path backwards to get all other mappings.

	Mappings bestMappings(_numSections);

	for (int section = _numSections - 1; section >= 0; section--) {

		bestMappings[section] = allMappings[section][bestMapping];

		if (section > 0)
			bestMapping = bestPreviousMapping[section][bestMapping];
	}

	// Set outputs.

	*_errors = minSliceErrors;

	LOG_DEBUG(resultevaluatorlog) << "done" << std::endl;

	LOG_ALL(resultevaluatorlog) << "false merges:" << std::endl;
	int a, b;
	foreach (boost::tie(a, b), minSliceErrors.falseMerges())
		LOG_ALL(resultevaluatorlog) << "[" << a << ", " << b << "]" << std::endl;
	LOG_ALL(resultevaluatorlog) << "false splits:" << std::endl;
	foreach (boost::tie(a, b), minSliceErrors.falseSplits())
		LOG_ALL(resultevaluatorlog) << "[" << a << ", " << b << "]" << std::endl;

	// dump to output (useful for redirection into file)
	LOG_USER(resultevaluatorlog) << "# FP FN FS FM" << std::endl;
	LOG_USER(resultevaluatorlog)
			<< minSliceErrors.numFalsePositives() << " "
			<< minSliceErrors.numFalseNegatives() << " "
			<< minSliceErrors.numFalseSplits() << " "
			<< minSliceErrors.numFalseMerges() << std::endl;
}

AnisotropicEditDistance::Mappings
AnisotropicEditDistance::getAllMappings(unsigned int section) {

	LOG_ALL(resultevaluatorlog) << "computing all mappings for section " << section << std::endl;

	// Get all result slices in 'section'.
	std::vector<boost::shared_ptr<Slice> > resultSlices = getResultSlices(section);

	// Get all ground-truth slices in 'section'.
	std::vector<boost::shared_ptr<Slice> > groundTruthSlices = getGroundTruthSlices(section);

	// Get all possible partners of one result slice to ground-truth slices.

	std::map<int, std::vector<int> > resultPartners;

	LOG_ALL(resultevaluatorlog) << "finding partners for each result slice" << std::endl;

	foreach (boost::shared_ptr<Slice> resultSlice, resultSlices) {
		foreach (boost::shared_ptr<Slice> groundTruthSlice, groundTruthSlices) {

			if (_overlap.exceeds(*resultSlice, *groundTruthSlice, _minOverlap)) {

				resultPartners[resultSlice->getId()].push_back(groundTruthSlice->getId());
			}
		}
	}

	// Recursively get all possible mappings.
	Mappings mappings;
	Mapping  currentMapping;

	createMappings(mappings, currentMapping, resultPartners, resultSlices, (unsigned int)0);

	LOG_ALL(resultevaluatorlog) << mappings.size() << " mappings found" << std::endl;

	return mappings;
}

std::vector<boost::shared_ptr<Slice> >
AnisotropicEditDistance::getGroundTruthSlices(unsigned int section) {

	return _groundTruthSlices[section];
}

std::vector<boost::shared_ptr<Slice> >
AnisotropicEditDistance::getResultSlices(unsigned int section) {

	return _resultSlices[section];
}

void
AnisotropicEditDistance::findAllSlicesAndLinks() {

	_resultSlices      = std::vector<std::vector<boost::shared_ptr<Slice> > >();
	_groundTruthSlices = std::vector<std::vector<boost::shared_ptr<Slice> > >();
	_resultLinks       = std::vector<std::set<std::pair<int, int> > >();
	_groundTruthLinks  = std::vector<std::set<std::pair<int, int> > >();

	LOG_DEBUG(resultevaluatorlog) << "collecting all ground-truth slices" << std::endl;
	_groundTruthSlices = findSlices(*_groundTruth);

	LOG_DEBUG(resultevaluatorlog) << "collecting all result slices" << std::endl;
	_resultSlices      = findSlices(*_result);

	LOG_DEBUG(resultevaluatorlog) << "collecting all ground-truth links" << std::endl;
	_groundTruthLinks = findLinks(*_groundTruth);

	LOG_DEBUG(resultevaluatorlog) << "collecting all result links" << std::endl;
	_resultLinks      = findLinks(*_result);
}

std::vector<std::vector<boost::shared_ptr<Slice> > >
AnisotropicEditDistance::findSlices(Segments& segments) {

	// Fill a set of slices for each section.
	std::vector<std::set<boost::shared_ptr<Slice> > > sliceSets;

	sliceSets.resize(_numSections);

	foreach (boost::shared_ptr<EndSegment> end, segments.getEnds())
		insertSlice(sliceSets, end->getSlice());

	foreach (boost::shared_ptr<ContinuationSegment> continuation, segments.getContinuations()) {

		insertSlice(sliceSets, continuation->getSourceSlice());
		insertSlice(sliceSets, continuation->getTargetSlice());
	}

	foreach (boost::shared_ptr<BranchSegment> branch, segments.getBranches()) {

		insertSlice(sliceSets, branch->getSourceSlice());
		insertSlice(sliceSets, branch->getTargetSlice1());
		insertSlice(sliceSets, branch->getTargetSlice2());
	}

	// Transform the sets into vectors.

	std::vector<std::vector<boost::shared_ptr<Slice> > > result(sliceSets.size());

	for (unsigned int i = 0; i < sliceSets.size(); i++) {

		result[i].reserve(sliceSets[i].size()); 
		std::copy(sliceSets[i].begin(), sliceSets[i].end(), std::back_inserter(result[i]));
	}

	return result;
}

std::vector<std::set<std::pair<int, int> > >
AnisotropicEditDistance::findLinks(Segments& segments) {

	std::vector<std::set<std::pair<int, int> > > links;

	links.resize(_numSections);

	foreach (boost::shared_ptr<ContinuationSegment> continuation, segments.getContinuations()) {

		unsigned int section = continuation->getInterSectionInterval();

		if (continuation->getDirection() == Right)

			links[section].insert(
					std::make_pair(
							continuation->getSourceSlice()->getId(),
							continuation->getTargetSlice()->getId()));

		else

			links[section].insert(
					std::make_pair(
							continuation->getTargetSlice()->getId(),
							continuation->getSourceSlice()->getId()));
	}

	foreach (boost::shared_ptr<BranchSegment> branch, segments.getBranches()) {

		unsigned int section = branch->getInterSectionInterval();

		if (branch->getDirection() == Right) {

			links[section].insert(
					std::make_pair(
							branch->getSourceSlice()->getId(),
							branch->getTargetSlice1()->getId()));

			links[section].insert(
					std::make_pair(
							branch->getSourceSlice()->getId(),
							branch->getTargetSlice2()->getId()));
		} else {

			links[section].insert(
					std::make_pair(
							branch->getTargetSlice1()->getId(),
							branch->getSourceSlice()->getId()));

			links[section].insert(
					std::make_pair(
							branch->getTargetSlice2()->getId(),
							branch->getSourceSlice()->getId()));
		}
	}

	return links;
}

void
AnisotropicEditDistance::insertSlice(
		std::vector<std::set<boost::shared_ptr<Slice> > >& sliceSets,
		boost::shared_ptr<Slice>                           slice) {

	LOG_ALL(resultevaluatorlog) << "inserting slice " << slice->getId() << " to set of section " << slice->getSection() << std::endl;

	// Add the slice to its section set.
	sliceSets[slice->getSection()].insert(slice);
}

void
AnisotropicEditDistance::createMappings(
		Mappings&                               mappings,
		Mapping&                                currentMapping,
		std::map<int, std::vector<int> >&       resultPartners,
		std::vector<boost::shared_ptr<Slice> >& resultSlices,
		unsigned int                            numSlice) {

	// If there are no more slices to assign, add the current mapping to the set 
	// of all mappings.

	if (numSlice == resultSlices.size()) {

		LOG_ALL(resultevaluatorlog) << "all slices mapped -- this is a new mapping" << std::endl;

		mappings.push_back(currentMapping);
		return;
	}

	LOG_ALL(resultevaluatorlog) << "mapping slice #" << numSlice << std::endl;

	unsigned int currentSlice = resultSlices[numSlice]->getId();

	LOG_ALL(resultevaluatorlog) << "slice id is " << currentSlice << std::endl;

	std::vector<int> partners = resultPartners[currentSlice];

	LOG_ALL(resultevaluatorlog) << "this slice has " << partners.size() << " partners" << std::endl;

	// If there are no partners, there's nothing to do here
	if (partners.size() == 0) {

		// Call create mappings for the next slice.
		createMappings(mappings, currentMapping, resultPartners, resultSlices, numSlice + 1);

	} else {

		// For each combination of partners...
		for (unsigned int combination = 1; combination < pow(2, partners.size()); combination++) {

			unsigned int numPartners = 0;

			// ...map to them.
			for (unsigned int i = 0; i < partners.size(); i++) {

				if (combination & (1 << i)) {

					currentMapping.push_back(std::make_pair(currentSlice, partners[i]));
					numPartners++;
				}
			}

			LOG_ALL(resultevaluatorlog)
					<< "selecting partner combination " << combination
					<< ", which maps to " << numPartners << " partners" << std::endl;

			// Call create mappings for the next slice.
			createMappings(mappings, currentMapping, resultPartners, resultSlices, numSlice + 1);

			LOG_ALL(resultevaluatorlog) << "removing " << numPartners << " mappings from current mapping" << std::endl;

			// Remove this mapping and continue with the next combination.
			currentMapping.erase(currentMapping.end() - numPartners, currentMapping.end());
		}
	}

	LOG_ALL(resultevaluatorlog) << "done with slice #" << numSlice << std::endl;
}

AnisotropicEditDistanceErrors
AnisotropicEditDistance::getSliceErrors(
		const Mapping& mapping,
		const Mapping& previousMapping,
		unsigned int section) {

	// Compute the slice errors for the given mappings of 'section' and the 
	// previous section.

	// Get intra-section slice errors.

	AnisotropicEditDistanceErrors intraSliceErrors = getIntraSliceErrors(mapping, section);

	// Get inter-section slice errors.

	AnisotropicEditDistanceErrors interSliceErrors = getInterSliceErrors(mapping, previousMapping, section);

	return intraSliceErrors + interSliceErrors;
}

AnisotropicEditDistanceErrors
AnisotropicEditDistance::getIntraSliceErrors(const Mapping& mapping, unsigned int section) {

	// Get all result slice ids of the current section.

	std::set<int> resultIds;

	foreach (boost::shared_ptr<Slice> slice, getResultSlices(section))
		resultIds.insert(slice->getId());

	LOG_ALL(resultevaluatorlog) << "collected " << resultIds.size() << " result ids" << std::endl;

	// Get all ground-truth slice ids of the current section.

	std::set<int> groundTruthIds;

	foreach (boost::shared_ptr<Slice> slice, getGroundTruthSlices(section))
		groundTruthIds.insert(slice->getId());

	LOG_ALL(resultevaluatorlog) << "collected " << groundTruthIds.size() << " ground-truth ids" << std::endl;

	// Remove all mapped slice ids from result slice ids and all mapped slice 
	// ids from ground-truth slice ids.

	LOG_ALL(resultevaluatorlog) << "erasing all mapped ids" << std::endl;

	int resultId;
	int groundTruthId;
	foreach (boost::tie(resultId, groundTruthId), mapping) {

		resultIds.erase(resultId);
		groundTruthIds.erase(groundTruthId);
	}

	AnisotropicEditDistanceErrors sliceErrors;

	// Remaining result slice ids are false positives.

	sliceErrors.falsePositives() = resultIds;

	// Remaining ground-truth slice ids are false negatives.

	sliceErrors.falseNegatives() = groundTruthIds;

	LOG_ALL(resultevaluatorlog)
			<< sliceErrors.numFalsePositives() << " false positives, "
			<< sliceErrors.numFalseNegatives() << " false negatives" << std::endl;

	return sliceErrors;
}

AnisotropicEditDistanceErrors
AnisotropicEditDistance::getInterSliceErrors(
		const Mapping& mapping,
		const Mapping& previousMapping,
		unsigned int section) {

	AnisotropicEditDistanceErrors interSliceErrors;

	// Create a look-up table for result ids to ground-truth ids under the 
	// current mapping.
	std::map<int, std::vector<int> > partnersOf;
	std::map<int, std::vector<int> > previousPartnersOf;

	int r, g;
	foreach (boost::tie(r, g), mapping)
		partnersOf[r].push_back(g);
	foreach (boost::tie(r, g), previousMapping)
		previousPartnersOf[r].push_back(g);

	// Get all links in result.
	std::set<std::pair<int, int> > resultLinks = _resultLinks[section];
	std::set<std::pair<int, int> > trueResultLinks;

	// Get all links in ground-truth.
	std::set<std::pair<int, int> > groundTruthLinks = _groundTruthLinks[section];
	std::set<std::pair<int, int> > foundGroundTruthLinks;

	int a, b;

	LOG_ALL(resultevaluatorlog) << "result links: ";
	foreach (boost::tie(a, b), resultLinks)
		LOG_ALL(resultevaluatorlog) << "    (" << a << ", " << b << ")" << std::endl;

	LOG_ALL(resultevaluatorlog) << "ground truth links: ";
	foreach (boost::tie(a, b), groundTruthLinks)
		LOG_ALL(resultevaluatorlog) << "    (" << a << ", " << b << ")" << std::endl;

	// For each link in result...
	foreach (boost::tie(a, b), resultLinks) {

		// ...find all corresponding links in ground-truth.

		// For each partner pa of a
		foreach (int pa, previousPartnersOf[a]) {

			// For each partner pb of b
			foreach (int pb, partnersOf[b]) {

				// If (pa, pb) is in groundTruthLinks
				if (groundTruthLinks.count(std::make_pair(pa, pb))) {

					// ...remember result and ground-truth link as true 
					// positives
					trueResultLinks.insert(std::make_pair(a, b));
					foundGroundTruthLinks.insert(std::make_pair(pa, pb));

					LOG_ALL(resultevaluatorlog) << "found a mathing link: (" << a << ", " << b << ") -> (" << pa << ", " << pb << ")" << std::endl;
				}
			}
		}
	}

	LOG_ALL(resultevaluatorlog) << "true result links: ";
	foreach (boost::tie(a, b), trueResultLinks)
		LOG_ALL(resultevaluatorlog) << "    (" << a << ", " << b << ")" << std::endl;

	LOG_ALL(resultevaluatorlog) << "found ground truth links: ";
	foreach (boost::tie(a, b), foundGroundTruthLinks)
		LOG_ALL(resultevaluatorlog) << "    (" << a << ", " << b << ")" << std::endl;

	// Remaining result links are false merges.

	foreach (boost::tie(a, b), trueResultLinks)
		resultLinks.erase(std::make_pair(a, b));
	interSliceErrors.falseMerges() = resultLinks;

	// Remaining ground-truth links are false splits.

	foreach (boost::tie(a, b), foundGroundTruthLinks)
		groundTruthLinks.erase(std::make_pair(a, b));
	interSliceErrors.falseSplits() = groundTruthLinks;

	return interSliceErrors;
}

