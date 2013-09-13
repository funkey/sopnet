#ifndef SOPNET_SEGMENTS_SPLIT_MERGE_H__
#define SOPNET_SEGMENTS_SPLIT_MERGE_H__

#include <gui/MouseSignals.h>
#include <gui/KeySignals.h>
#include <pipeline/SimpleProcessNode.h>

#include <sopnet/gui/SplitMergePainter.h>

class SplitMerge : public pipeline::SimpleProcessNode<> {

public:

	SplitMerge();

private:

	struct Link {

		Link(boost::shared_ptr<Slice> l, boost::shared_ptr<Slice> r) : left(l), right(r) {}

		bool operator<(const Link& other) const {

			return distance() < other.distance();
		}

		double distance() const {

			return
					pow(left->getComponent()->getCenter().x - right->getComponent()->getCenter().x, 2) +
					pow(left->getComponent()->getCenter().y - right->getComponent()->getCenter().y, 2);
		}

		boost::shared_ptr<Slice> left;
		boost::shared_ptr<Slice> right;
	};

	void updateOutputs();

	void onInputSet(const pipeline::InputSetBase& signal);

	void onKeyDown(const gui::KeyDown& signal);
	void onMouseDown(const gui::MouseDown& signal);

	void removeSegments(std::vector<boost::shared_ptr<Slice> >& slices, Direction direction, unsigned int interval);
	void mergeSlices(std::vector<boost::shared_ptr<Slice> >& prevSlices, std::vector<boost::shared_ptr<Slice> >& currSlices);

	void probe(std::vector<boost::shared_ptr<EndSegment> >& ends, const util::point<double>& position);
	void probe(std::vector<boost::shared_ptr<ContinuationSegment> >& continuation, const util::point<double>& position);
	void probe(std::vector<boost::shared_ptr<BranchSegment> >& branch, const util::point<double>& position);

	pipeline::Input<Segments>           _initialSegments;
	pipeline::Input<int>                _section;
	pipeline::Output<Segments>          _segments;
	pipeline::Output<SplitMergePainter> _painter;

	boost::shared_ptr<Segments> _selection;

	bool _initialSegmentsProcessed;
};

#endif // SOPNET_SEGMENTS_SPLIT_MERGE_H__

