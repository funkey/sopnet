#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/helpers.hpp>
#include "ResultEvaluator.h"

logger::LogChannel resultevaluatorlog("resultevaluatorlog", "[ResultEvaluator] ");

util::ProgramOption optionEvaluatinMinOverlap(
		util::_module           = "sopnet.evaluation",
		util::_long_name        = "minOverlap",
		util::_description_text = "The minimal normalized overlap between a result and ground-truth slice to consider them as a match.");

ResultEvaluator::ResultEvaluator(double minOverlap) :
	_overlap(true /* normalize */, false /* don't align */),
	_minOverlap(optionEvaluatinMinOverlap ? optionEvaluatinMinOverlap : minOverlap) {

	registerInput(_result, "result");
	registerInput(_groundTruth, "ground truth");
	registerOutput(_errors, "errors");
}

void
ResultEvaluator::updateOutputs() {

	LOG_DEBUG(resultevaluatorlog) << "updating outputs..." << std::endl;

	_numSections = std::max((int)_result->getNumInterSectionIntervals(), (int)_groundTruth->getNumInterSectionIntervals());

	LOG_DEBUG(resultevaluatorlog) << "compute errors for " << _numSections << " sections" << std::endl;

	// First, find all slices in the result and ground-truth.
	findAllSlicesAndLinks();

	// all mappings of all sections
	std::vector<Mappings> allMappings;

	// pointers from any mapping of any section to the mapping in the previous 
	// section with minimal accumulated error
	std::vector<std::map<int, int> > bestPreviousMapping;

	// all minimal errors until a section and a mapping of this section
	std::vector<std::vector<Errors> > accumulatedErrors;

	// For each section...
	for (unsigned int section = 0; section < _numSections; section++) {

		LOG_DEBUG(resultevaluatorlog) << "processing section " << section << std::endl;

		// Get all possible mappings of result to ground truth.
		allMappings.push_back(getAllMappings(section));

		Mappings& currentMappings = allMappings[section];

		// new map of mappings to mappings for current section
		bestPreviousMapping.push_back(std::map<int, int>());

		if (section == 0) {

			LOG_ALL(resultevaluatorlog) << "calculating errors of first section" << std::endl;

			// The errors of the first section consist only of the intra-section 
			// errors.
			std::vector<Errors> firstErrors;

			foreach (Mapping& mapping, currentMappings)
				firstErrors.push_back(getIntraErrors(mapping, 0));

			accumulatedErrors.push_back(firstErrors);

			LOG_ALL(resultevaluatorlog) << "section 0: " << firstErrors << std::endl;

			continue;
		}

		// new vector of errors for current section
		accumulatedErrors.push_back(std::vector<Errors>(currentMappings.size(), Errors()));

		Mappings& previousMappings = allMappings[section - 1];

		// Compute errors of all mappings of current section when combined with 
		// any mapping of previous section.

		std::map<int, std::map<int, Errors> > errors;

		LOG_DEBUG(resultevaluatorlog)
				<< "computing errors for all " << currentMappings.size() << "x"
				<< previousMappings.size() << " combinations" << std::endl;

		for (unsigned int i = 0; i < currentMappings.size(); i++)
			for (unsigned int j = 0; j < previousMappings.size(); j++)
				errors[i][j] = getErrors(currentMappings[i], previousMappings[j], section);

		// For each mapping of the current section, get the minimal number of 
		// errors up to the current section, remember which mapping in the 
		// previous section was involved.

		LOG_ALL(resultevaluatorlog) << "searching for minimal accumulated errors" << std::endl;

		for (unsigned int i = 0; i < currentMappings.size(); i++) {

			int numErrors = -1;

			for (unsigned int j = 0; j < previousMappings.size(); j++) {

				// the error until j in the previous section
				const Errors& previousErrors = accumulatedErrors[section-1][j];

				// the error until i when going over j
				Errors currentErrors = errors[i][j] + previousErrors;

				// remember the best j
				if (currentErrors.total() < numErrors || numErrors == -1) {

					numErrors = currentErrors.total();
					bestPreviousMapping[section][i] = j;
					accumulatedErrors[section][i] = currentErrors;
				}
			}
		}

		LOG_ALL(resultevaluatorlog) << "section " << section << ": " << accumulatedErrors[section] << std::endl;

		LOG_ALL(resultevaluatorlog) << "done with section " << section << std::endl;
	}

	// Choose the mapping with the lowest acummulated error.

	Errors minErrors;
	int bestMapping = -1;

	LOG_DEBUG(resultevaluatorlog) << "walking backwards to find sequence of optimal mappings" << std::endl;

	for (unsigned int i = 0; i < allMappings[_numSections-1].size(); i++) {

		if (accumulatedErrors[_numSections-1][i].total() < minErrors.total() || bestMapping == -1) {

			minErrors   = accumulatedErrors[_numSections-1][i];
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

	*_errors = minErrors;

	LOG_DEBUG(resultevaluatorlog) << "done" << std::endl;

	LOG_ALL(resultevaluatorlog) << "false merges:" << std::endl;
	int a, b;
	foreach (boost::tie(a, b), minErrors.falseMerges())
		LOG_ALL(resultevaluatorlog) << "[" << a << ", " << b << "]" << std::endl;
	LOG_ALL(resultevaluatorlog) << "false splits:" << std::endl;
	foreach (boost::tie(a, b), minErrors.falseSplits())
		LOG_ALL(resultevaluatorlog) << "[" << a << ", " << b << "]" << std::endl;

	// dump to output (useful for redirection into file)
	LOG_USER(resultevaluatorlog) << "# FP FN FS FM" << std::endl;
	LOG_USER(resultevaluatorlog)
			<< minErrors.numFalsePositives() << " "
			<< minErrors.numFalseNegatives() << " "
			<< minErrors.numFalseSplits() << " "
			<< minErrors.numFalseMerges() << std::endl;
}

ResultEvaluator::Mappings
ResultEvaluator::getAllMappings(unsigned int section) {

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
ResultEvaluator::getGroundTruthSlices(unsigned int section) {

	return _groundTruthSlices[section];
}

std::vector<boost::shared_ptr<Slice> >
ResultEvaluator::getResultSlices(unsigned int section) {

	return _resultSlices[section];
}

void
ResultEvaluator::findAllSlicesAndLinks() {

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
ResultEvaluator::findSlices(Segments& segments) {

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
ResultEvaluator::findLinks(Segments& segments) {

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
ResultEvaluator::insertSlice(
		std::vector<std::set<boost::shared_ptr<Slice> > >& sliceSets,
		boost::shared_ptr<Slice>                           slice) {

	LOG_ALL(resultevaluatorlog) << "inserting slice " << slice->getId() << " to set of section " << slice->getSection() << std::endl;

	// Add the slice to its section set.
	sliceSets[slice->getSection()].insert(slice);
}

void
ResultEvaluator::createMappings(
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

Errors
ResultEvaluator::getErrors(
		const Mapping& mapping,
		const Mapping& previousMapping,
		unsigned int section) {

	// Compute the errors for the given mappings of 'section' and the previous 
	// section.

	// Get intra-section errors.

	Errors intraErrors = getIntraErrors(mapping, section);

	// Get inter-section errors.

	Errors interErrors = getInterErrors(mapping, previousMapping, section);

	return intraErrors + interErrors;
}

Errors
ResultEvaluator::getIntraErrors(const Mapping& mapping, unsigned int section) {

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

	Errors errors;

	// Remaining result slice ids are false positives.

	errors.falsePositives() = resultIds;

	// Remaining ground-truth slice ids are false negatives.

	errors.falseNegatives() = groundTruthIds;

	LOG_ALL(resultevaluatorlog)
			<< errors.numFalsePositives() << " false positives, "
			<< errors.numFalseNegatives() << " false negatives" << std::endl;

	return errors;
}

Errors
ResultEvaluator::getInterErrors(
		const Mapping& mapping,
		const Mapping& previousMapping,
		unsigned int section) {

	Errors interErrors;

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
	interErrors.falseMerges() = resultLinks;

	// Remaining ground-truth links are false splits.

	foreach (boost::tie(a, b), foundGroundTruthLinks)
		groundTruthLinks.erase(std::make_pair(a, b));
	interErrors.falseSplits() = groundTruthLinks;

	return interErrors;
}

