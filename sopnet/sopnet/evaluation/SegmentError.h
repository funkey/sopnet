#ifndef SOPNET_EVALUATION_SEGMENT_ERROR_H__
#define SOPNET_EVALUATION_SEGMENT_ERROR_H__

#include <pipeline/SimpleProcessNode.h>
#include <sopnet/segments/Segments.h>

/**
 * Given two sets of segments (a gold standard and a reconstruction), provides 
 * the number of missclassified segments.
 */
class SegmentError : public pipeline::SimpleProcessNode<> {

public:

	SegmentError() {

		registerInput(_goldStandard, "gold standard");
		registerInput(_reconstruction, "reconstruction");
		registerOutput(_missclassified, "missclassified");
	}

private:

	void updateOutputs() {

		_missclassified = new unsigned int();

		*_missclassified = 0;
		foreach (boost::shared_ptr<Segment> segment, _reconstruction->getSegments())
			if (!_goldStandard->contains(segment))
				(*_missclassified)++;
		foreach (boost::shared_ptr<Segment> segment, _goldStandard->getSegments())
			if (!_reconstruction->contains(segment))
				(*_missclassified)++;
		
	}

	pipeline::Input<Segments> _goldStandard;
	pipeline::Input<Segments> _reconstruction;

	pipeline::Output<unsigned int> _missclassified;
};

#endif // SOPNET_EVALUATION_SEGMENT_ERROR_H__

