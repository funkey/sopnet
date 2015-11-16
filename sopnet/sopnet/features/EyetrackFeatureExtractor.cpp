#include <fstream>
#include <boost/filesystem.hpp>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include <util/foreach.h>
#include <util/Logger.h>
#include "EyetrackFeatureExtractor.h"

logger::LogChannel eyetrackfeatureextractorlog("eyetrackfeatureextractorlog", "[EyetrackFeatureExtractor] ");

namespace ublas = boost::numeric::ublas;

EyetrackFeatureExtractor::EyetrackFeatureExtractor(const std::string& eyetrackDirectory) :
	_features(new Features()),
	_eyetrackDirectory(eyetrackDirectory) {

	registerInput(_segments, "segments");
	registerOutput(_features, "features");

	// read tracks
	readDistributions();

	// compute per-neurite track distribution for each section
}

void
EyetrackFeatureExtractor::readDistributions() {

	boost::filesystem::path dir(_eyetrackDirectory);

	std::vector<boost::filesystem::path> neuriteDirs;
	std::copy(
			boost::filesystem::directory_iterator(dir),
			boost::filesystem::directory_iterator(),
			back_inserter(neuriteDirs));
	std::sort(neuriteDirs.begin(), neuriteDirs.end());

	_currentNeurite = 0;
	foreach (boost::filesystem::path neuriteDir, neuriteDirs) {

		readNeuriteDistribution(neuriteDir);
		_currentNeurite++;
	}
}

void
EyetrackFeatureExtractor::readNeuriteDistribution(boost::filesystem::path neuriteDir) {

	std::vector<boost::filesystem::path> trackFiles;
	std::copy(
			boost::filesystem::directory_iterator(neuriteDir),
			boost::filesystem::directory_iterator(),
			back_inserter(trackFiles));
	std::sort(trackFiles.begin(), trackFiles.end());

	Track track;
	std::set<int> sections;
	std::map<int, std::vector<Vec> > sectionPoints;

	foreach (boost::filesystem::path file, trackFiles) {

		std::ifstream in(file.native().c_str());

		int section;
		Vec point(2);
		while (in.good()) {

			in >> section;
			in >> point[0];
			in >> point[1];

			if (!in.good())
				break;

			if (!sections.count(section)) {

				track.distributions[section].mean[0] = 0;
				track.distributions[section].mean[1] = 0;
			}

			sections.insert(section);
			sectionPoints[section].push_back(point);
			track.distributions[section].mean += point;
		}
	}

	foreach (int section, sections) {

		Vec mean = track.distributions[section].mean / sectionPoints[section].size();
		track.distributions[section].mean = mean;

		LOG_ALL(eyetrackfeatureextractorlog) << "\t\tsection " << section << ",\tmean: " << track.distributions[section].mean[0] << ", " << track.distributions[section].mean[1] << std::endl;

		foreach (Vec point, sectionPoints[section]) {

			Vec diff = mean - point;
			track.distributions[section].cov += ublas::outer_prod(diff, diff);

			LOG_ALL(eyetrackfeatureextractorlog) << "\t\t             \tadd : " << diff[0] << ", " << diff[1] << std::endl;
			LOG_ALL(eyetrackfeatureextractorlog) << "\t\t             \top  : " << ublas::outer_prod(diff, diff)(0,0) << ", " << ublas::outer_prod(diff, diff)(0,1) << std::endl;
			LOG_ALL(eyetrackfeatureextractorlog) << "\t\t             \t      " << ublas::outer_prod(diff, diff)(1,0) << ", " << ublas::outer_prod(diff, diff)(1,1) << std::endl;
			LOG_ALL(eyetrackfeatureextractorlog) << "\t\t             \tcov : " << track.distributions[section].cov(0,0) << ", " << track.distributions[section].cov(0,1) << std::endl;
			LOG_ALL(eyetrackfeatureextractorlog) << "\t\t             \t      " << track.distributions[section].cov(1,0) << ", " << track.distributions[section].cov(1,1) << std::endl;
		}

		if (sectionPoints[section].size() > 1) {

			track.distributions[section].cov /= (sectionPoints[section].size() - 1);
			track.distributions[section].update();

		} else {

			// for single-track "distributions", we just take the identity as 
			// covariance matrix to get isotropic mahalanobis distances
			track.distributions[section].cov(0, 0) = 1.0;
			track.distributions[section].cov(0, 1) = 0.0;
			track.distributions[section].cov(1, 0) = 0.0;
			track.distributions[section].cov(1, 1) = 1.0;
			track.distributions[section].update();
		}
	}

	LOG_DEBUG(eyetrackfeatureextractorlog) << "found a track with:" << std::endl;
	foreach (int section, sections) {
		LOG_DEBUG(eyetrackfeatureextractorlog) << "\tsection " << section << ",\tmean: " << track.distributions[section].mean[0] << ", " << track.distributions[section].mean[1] << std::endl;
		LOG_DEBUG(eyetrackfeatureextractorlog) << "\t             \tcov : " << track.distributions[section].cov(0,0) << ", " << track.distributions[section].cov(0,1) << std::endl;
		LOG_DEBUG(eyetrackfeatureextractorlog) << "\t             \t      " << track.distributions[section].cov(1,0) << ", " << track.distributions[section].cov(1,1) << std::endl;
	}

	_tracks[_currentNeurite] = track;
}

void
EyetrackFeatureExtractor::updateOutputs() {

	_features->clear();
	_features->addName("eyetrack score");
	_features->resize(_segments->size(), 1);

	foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds())
		getFeatures(*segment, _features->get(segment->getId()));
	foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations())
		getFeatures(*segment, _features->get(segment->getId()));
	foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches())
		getFeatures(*segment, _features->get(segment->getId()));
}

void
EyetrackFeatureExtractor::getFeatures(const EndSegment& /*end*/, std::vector<double>& features) {

	// we don't have an opinion on ends
	features[0] = 0;
}

void
EyetrackFeatureExtractor::getFeatures(const ContinuationSegment& continuation, std::vector<double>& features) {

	// the score is a number between 0 and 1, make it a reward
	features[0] = -getLinkScore(*continuation.getSourceSlice(), *continuation.getTargetSlice());
}

void
EyetrackFeatureExtractor::getFeatures(const BranchSegment& branch, std::vector<double>& features) {

	// the score is a number between 0 and 1, make it a reward
	features[0] = -std::max(
			getLinkScore(*branch.getSourceSlice(), *branch.getTargetSlice1()),
			getLinkScore(*branch.getSourceSlice(), *branch.getTargetSlice1()));
}

double
EyetrackFeatureExtractor::getLinkScore(const Slice& a, const Slice& b) {

	double bestScore = 0;

	std::map<int, Track>::const_iterator i;
	for (i = _tracks.begin(); i != _tracks.end(); i++) {

		/* For the score, let's take the product of the probability density of 
		 * the centroids of the involved slices in the track distribution.
		 */
		double score = getSliceScore(i->second, a)*getSliceScore(i->second, b);
		bestScore = std::max(bestScore, score);
	}

	return bestScore;
}

double
EyetrackFeatureExtractor::getSliceScore(const Track& track, const Slice& slice) {

	int section = slice.getSection();
	if (!track.distributions.count(section))
		return 0.5; // default score of "no observation"

	Vec x(2);
	x[0] = slice.getComponent()->getCenter().x;
	x[1] = slice.getComponent()->getCenter().y;
	return track.distributions.at(section).pdf(x);
}

void
EyetrackFeatureExtractor::Distribution::update() {

	using namespace boost::numeric::ublas;

	typedef permutation_matrix<std::size_t> pmatrix;

	// create a working copy of the input
	Mat A(cov);

	// create a permutation matrix for the LU-factorization
	pmatrix pm(A.size1());

	// perform LU-factorization
	int res = lu_factorize(A, pm);
	if(res != 0)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"covariance matrix not invertible " << cov(0, 0) << ", " << cov(0, 1) << ", " << cov(1, 0) << " " << cov(1, 1));

	// create identity matrix of "inverse"
	_inv.assign(identity_matrix<double>(A.size1()));

	// backsubstitute to get the inverse
	lu_substitute(A, pm, _inv);

	// get the determant
	_det = 1.0;
	for (std::size_t i = 0; i < pm.size(); ++i) {

		if (pm(i) != i)
			_det *= -1;

		_det *= A(i, i);
	}

	_z = sqrt(pow(2*M_PI,2)*_det);
}

double
EyetrackFeatureExtractor::Distribution::mahalanobis2(const Vec& x) const {

	Vec a = (mean - x);
	Vec b = boost::numeric::ublas::prod(_inv, a);
	return boost::numeric::ublas::inner_prod(a, b);
}

double
EyetrackFeatureExtractor::Distribution::pdf(const Vec& x) const {

	return exp(-0.5*mahalanobis2(x))/_z;
}
