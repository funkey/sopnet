#include <fstream>
#include <util/exceptions.h>
#include <util/Logger.h>
#include "GoldStandardFileReader.h"

logger::LogChannel goldstandardfilereaderlog("goldstandardfilereaderlog", "[GoldStandardFileReader] ");

GoldStandardFileReader::GoldStandardFileReader(const std::string& filename) :
		_filename(filename) {

	registerInput(_allSegments, "all segments");
	registerOutput(_goldStandard, "gold standard");
}

void
GoldStandardFileReader::updateOutputs() {

	// get the hashes of all gold-standard segments

	std::set<SegmentHash> goldStandardSegmentHashes;
	std::set<SegmentHash> otherSegmentHashes;

	std::ifstream file(_filename.c_str());

	if (!file.good()) {

		UTIL_THROW_EXCEPTION(
				UsageError,
				"could not open " << _filename << " to read the gold-standard hashes -- please make sure you created them beforehand!");
	}

	while (file.good()) {

		std::string line;
		std::getline(file, line);

		if (line.empty())
			continue;

		std::size_t hashStart = line.find_first_of('#');

		// is it a gold-standard segment? it is a gold-standard segment if there 
		// is a '1' before the '#'
		std::size_t firstOne = line.find_first_of('1');
		bool isGoldStandard = (firstOne < hashStart);

		if (hashStart != std::string::npos) {

			std::string hashString = line.substr(hashStart);

			// strip any leading non-number
			std::size_t firstDigit = hashString.find_first_of("0123456789");
			hashString = hashString.substr(firstDigit);

			SegmentHash hash = boost::lexical_cast<SegmentHash>(hashString);

			if (isGoldStandard)
				goldStandardSegmentHashes.insert(hash);
			else
				otherSegmentHashes.insert(hash);

		} else {

			UTIL_THROW_EXCEPTION(
					UsageError,
					_filename << " does not contain a hash value in line '" << line << "'");
		}
	}

	// collect all gold-standard segments

	if (!_goldStandard)
		_goldStandard = new Segments();
	else
		_goldStandard->clear();

	foreach (boost::shared_ptr<EndSegment> segment, _allSegments->getEnds())
		if (goldStandardSegmentHashes.count(segment->hashValue()))
			_goldStandard->add(segment);
		else if (!otherSegmentHashes.count(segment->hashValue())) {

			LOG_ERROR(goldstandardfilereaderlog)
					<< "end segment " << segment->getId()
					<< " in ISI " << segment->getInterSectionInterval()
					<< " is not contained in "
					<< _filename << std::endl;
		}
	foreach (boost::shared_ptr<ContinuationSegment> segment, _allSegments->getContinuations())
		if (goldStandardSegmentHashes.count(segment->hashValue()))
			_goldStandard->add(segment);
		else if (!otherSegmentHashes.count(segment->hashValue())) {

			LOG_ERROR(goldstandardfilereaderlog)
					<< "continuation segment " << segment->getId()
					<< " in ISI " << segment->getInterSectionInterval()
					<< " is not contained in "
					<< _filename << std::endl;
		}
	foreach (boost::shared_ptr<BranchSegment> segment, _allSegments->getBranches())
		if (goldStandardSegmentHashes.count(segment->hashValue()))
			_goldStandard->add(segment);
		else if (!otherSegmentHashes.count(segment->hashValue())) {

			LOG_ERROR(goldstandardfilereaderlog)
					<< "branch segment " << segment->getId()
					<< " in ISI " << segment->getInterSectionInterval()
					<< " is not contained in "
					<< _filename << std::endl;
		}

	LOG_USER(goldstandardfilereaderlog)
			<< "collected " << _goldStandard->size()
			<< " segments in gold-standard" << std::endl;
}
