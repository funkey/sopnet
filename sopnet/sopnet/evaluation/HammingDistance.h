#ifndef SOPNET_EVALUATION_HAMMING_DISTANCE_H__
#define SOPNET_EVALUATION_HAMMING_DISTANCE_H__

#include <pipeline/SimpleProcessNode.h>
#include <sopnet/segments/Segments.h>
#include "HammingDistanceErrors.h"

/**
 * Given two sets of segments (a gold standard and a reconstruction), provides 
 * the number of missclassified segments.
 */
class HammingDistance : public pipeline::SimpleProcessNode<> {

public:

	HammingDistance() {

		registerInput(_goldStandard, "gold standard");
		registerInput(_reconstruction, "reconstruction");
		registerOutput(_errors, "errors");
	}

private:

	void updateOutputs() {

		_errors = new HammingDistanceErrors();

		unsigned int fp = 0;
		unsigned int fn = 0;

		foreach (boost::shared_ptr<Segment> segment, _reconstruction->getSegments())
			if (!_goldStandard->contains(segment))
				fp++;
		foreach (boost::shared_ptr<Segment> segment, _goldStandard->getSegments())
			if (!_reconstruction->contains(segment))
				fn++;

		_errors->setNumFalsePositives(fp);
		_errors->setNumFalseNegatives(fn);
	}

	pipeline::Input<Segments> _goldStandard;
	pipeline::Input<Segments> _reconstruction;

	pipeline::Output<HammingDistanceErrors> _errors;
};

#endif // SOPNET_EVALUATION_HAMMING_DISTANCE_H__

