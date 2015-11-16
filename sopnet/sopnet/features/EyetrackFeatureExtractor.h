#ifndef SOPNET_EYE_TRACK_FEATURE_EXTRACTOR_H__
#define SOPNET_EYE_TRACK_FEATURE_EXTRACTOR_H__

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <pipeline/all.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/features/Features.h>

class EyetrackFeatureExtractor : public pipeline::SimpleProcessNode<> {

public:

	EyetrackFeatureExtractor(const std::string& eyetrackDirectory);

private:

	typedef boost::numeric::ublas::vector<double> Vec;
	typedef boost::numeric::ublas::matrix<double> Mat;

	void updateOutputs();

	void readDistributions();

	void readNeuriteDistribution(boost::filesystem::path neuriteDir);

	void getFeatures(const EndSegment& end, std::vector<double>& slice);

	void getFeatures(const ContinuationSegment& continuation, std::vector<double>& slice);

	void getFeatures(const BranchSegment& branch, std::vector<double>& slice);

	pipeline::Input<Segments> _segments;

	pipeline::Output<Features> _features;

	std::string _eyetrackDirectory;

	/**
	 * A 2D distribution of the tracks of one neurite in one section.
	 */
	class Distribution {

	public:

		Distribution() : mean(2), cov(2, 2), _inv(2, 2) {

			mean[0] = 0; mean[1] = 0;
			cov(0, 0) = 0; cov(0, 1) = 0;
			cov(1, 0) = 0; cov(1, 1) = 0;
			_inv(0, 0) = 1; _inv(0, 1) = 0;
			_inv(1, 0) = 0; _inv(1, 1) = 1;
		}

		Vec mean;
		Mat cov;

		/**
		 * Update the inverse and determinant of the covariance matrix, as well 
		 * as the normalization constant for the pdf.
		 */
		void update();

		/**
		 * Square of the mahalanobis distance.
		 */
		double mahalanobis2(const Vec& x) const;

		/**
		 * Density function.
		 */
		double pdf(const Vec& x) const;

	private:

		// inverse and determinant of the covariance matrix
		Mat    _inv;
		double _det;
		double _z;
	};

	/**
	 * A distribution per section.
	 */
	struct Track {

		std::map<int, Distribution> distributions;
	};

	double getLinkScore(const Slice& a, const Slice& b);

	double getSliceScore(const Track& track, const Slice& slice);

	// a track per neurite
	std::map<int, Track> _tracks;

	int _currentNeurite;
};

#endif // SOPNET_EYE_TRACK_FEATURE_EXTRACTOR_H__

