#include <util/Logger.h>
#include "SplitMerge.h"

logger::LogChannel splitmergelog("splitmergelog", "[SplitMerge] ");

SplitMerge::SplitMerge() :
	_segments(boost::make_shared<Segments>()),
	_painter(boost::make_shared<SplitMergePainter>()),
	_selection(boost::make_shared<Segments>()),
	_initialSegmentsProcessed(false) {

	registerInput(_initialSegments, "initial segments");
	registerInput(_section, "section");
	registerOutput(_segments, "segments");
	registerOutput(_painter, "painter");

	setDependency(_section, _painter);

	_initialSegments.registerBackwardCallback(&SplitMerge::onInputSet, this);
	_painter.registerForwardCallback(&SplitMerge::onMouseDown, this);
	_painter.registerForwardCallback(&SplitMerge::onKeyDown, this);
	_painter->setSelection(_selection);
}

void
SplitMerge::updateOutputs() {

	if (!_initialSegmentsProcessed) {

		*_segments = *_initialSegments;

		// create end segments for the first section (so that we can select slices 
		// in there)
		unsigned int numNewEnds = 0;
		foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds(1)) {

			if (end->getDirection() == Right) {

				boost::shared_ptr<EndSegment> newEnd = boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Left, end->getSlice());
				_segments->add(newEnd);

				numNewEnds++;
			}
		}

		LOG_DEBUG(splitmergelog) << "added " << numNewEnds << " new ends" << std::endl;

		_initialSegmentsProcessed = true;
	}

	_painter->setSection(*_section);
}

void
SplitMerge::onInputSet(const pipeline::InputSetBase& signal) {

	_initialSegmentsProcessed = false;

	LOG_DEBUG(splitmergelog) << "initial segments have been set" << std::endl;
}

void
SplitMerge::onKeyDown(const gui::KeyDown& signal) {

	if (signal.key == gui::keys::M) {

		// merge!

		std::vector<boost::shared_ptr<Slice> > prevSlices;
		std::vector<boost::shared_ptr<Slice> > currSlices;

		// compare each section with the previous section
		for (int section = 0; section < _segments->getNumInterSectionIntervals(); section++) {

			foreach (boost::shared_ptr<Segment> segment, _selection->getSegments(section)) {

				if (segment->getDirection() == Left)
					foreach (boost::shared_ptr<Slice> slice, segment->getSourceSlices())
						currSlices.push_back(slice);
				else
					foreach (boost::shared_ptr<Slice> slice, segment->getTargetSlices())
						currSlices.push_back(slice);
			}

			LOG_ALL(splitmergelog) << "merging in inter-section interval " << section << std::endl;
			LOG_ALL(splitmergelog) << "previous slices: " << prevSlices.size() << ", current slices " << currSlices.size() << std::endl;

			// is there something to merge?
			if (prevSlices.size()*currSlices.size() > 0) {

				removeSegments(prevSlices, Right, section);
				removeSegments(currSlices, Left, section);
				mergeSlices(prevSlices, currSlices);
			}

			std::swap(prevSlices, currSlices);
			currSlices.clear();
		}

		setDirty(_segments);
	}
}

void
SplitMerge::removeSegments(std::vector<boost::shared_ptr<Slice> >& slices, Direction direction, unsigned int interval) {

	// remove all segments in the given interval that are starting from any of 
	// the given slices in the given direction

	std::vector<boost::shared_ptr<Segment> > segments = _segments->getSegments(interval);

	foreach (boost::shared_ptr<Segment> segment, segments) {
		if (segment->getDirection() == direction) {
			foreach (boost::shared_ptr<Slice> slice, slices) {
				foreach (boost::shared_ptr<Slice> segmentSlice, segment->getSourceSlices()) {
					if (segmentSlice->getId() == slice->getId()) {

						_segments->remove(segment);

						LOG_DEBUG(splitmergelog) << "removed a segment using slice " << slice->getId() << " in interval " << interval << std::endl;
					}
				}
			}
		}
	}
}

void
SplitMerge::mergeSlices(std::vector<boost::shared_ptr<Slice> >& prevSlices, std::vector<boost::shared_ptr<Slice> >& currSlices) {

	unsigned int numSlices = prevSlices.size() + currSlices.size();

	std::vector<Link> links;

	foreach(boost::shared_ptr<Slice> p, prevSlices)
		foreach (boost::shared_ptr<Slice> c, currSlices)
			links.push_back(Link(p, c));

	std::sort(links.begin(), links.end());

	std::map<boost::shared_ptr<Slice>, std::set<boost::shared_ptr<Slice> > > partners;
	foreach (Link& link, links) {

		if (partners.size() == numSlices)
			break;

		boost::shared_ptr<Slice> p = link.left;
		boost::shared_ptr<Slice> c = link.right;

		// partners to themselves
		partners[p].insert(p);
		partners[c].insert(c);

		// get all partners of c and add them to the partners of p
		foreach (boost::shared_ptr<Slice> s, partners[c])
			partners[p].insert(s);

		// p is now the only one who knows all its partners

		// the partners of p might not know about c or any of its partners

		// for each partner of p, add all partners of p to the partner list
		foreach (boost::shared_ptr<Slice> pp, partners[p]) {

			foreach (boost::shared_ptr<Slice> ppp, partners[p])
				partners[pp].insert(ppp);
		}
	}

	std::set<boost::shared_ptr<Slice> > linkedSlices;

	boost::shared_ptr<Slice>            s;
	std::set<boost::shared_ptr<Slice> > ps;
	foreach (boost::tie(s, ps), partners) {

		if (linkedSlices.size() == numSlices)
			break;

		if (linkedSlices.count(s))
			continue;

		// remember all slices that have been handled so far
		foreach (boost::shared_ptr<Slice> p, ps)
			linkedSlices.insert(p);

		typename std::set<boost::shared_ptr<Slice> >::iterator i = ps.begin();

		if (ps.size() == 2) {

			boost::shared_ptr<Slice> left = *i;
			i++;
			boost::shared_ptr<Slice> right = *i;

			if (left->getSection() > right->getSection())
				std::swap(left, right);

			boost::shared_ptr<ContinuationSegment> continuation = boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, left, right);

			_segments->add(continuation);

			LOG_DEBUG(splitmergelog) << "added a continuation between slice " << left->getId() << " and " << right->getId() << std::endl;

		} else if (ps.size() == 3) {

			boost::shared_ptr<Slice> s1 = *i; i++;
			boost::shared_ptr<Slice> s2 = *i; i++;
			boost::shared_ptr<Slice> s3 = *i;

			double section;
			double offset;
			offset = modf((double)(s1->getSection()+s2->getSection()+s3->getSection())/3.0, &section);
			Direction direction = (offset > 0.5 ? Right : Left);

			boost::shared_ptr<BranchSegment> branch;
			if (s1->getSection() == s2->getSection())
				branch = boost::make_shared<BranchSegment>(Segment::getNextSegmentId(), direction, s3, s1, s2);
			else if (s1->getSection() == s3->getSection())
				branch = boost::make_shared<BranchSegment>(Segment::getNextSegmentId(), direction, s2, s1, s3);
			else
				branch = boost::make_shared<BranchSegment>(Segment::getNextSegmentId(), direction, s1, s2, s3);

			_segments->add(branch);

			LOG_USER(splitmergelog) << "added a branch between slices " << s1->getId() << ", " << s2->getId() << ", and " << s3->getId() << std::endl;

		} else {

			LOG_ERROR(splitmergelog) << "something wants to merge " << ps.size() << " slices -- don't know what to do." << std::endl;
		}
	}
}

void
SplitMerge::onMouseDown(const gui::MouseDown& signal) {

	LOG_ALL(splitmergelog) << "mouse down" << std::endl;

	if (signal.button == gui::buttons::Left) {

		// find closest segments in intersection interval to the left
		std::vector<boost::shared_ptr<EndSegment> >          ends          = _segments->findEnds(signal.position, *_section, 1000000);
		std::vector<boost::shared_ptr<ContinuationSegment> > continuations = _segments->findContinuations(signal.position, *_section, 1000000);
		std::vector<boost::shared_ptr<BranchSegment> >       branches      = _segments->findBranches(signal.position, *_section, 1000000);

		probe(ends, signal.position);
		probe(continuations, signal.position);
		probe(branches, signal.position);

	}

	if (signal.button == gui::buttons::Right) {

		_selection->clear();
		setDirty(_painter);
	}
}

void
SplitMerge::probe(std::vector<boost::shared_ptr<EndSegment> >& ends, const util::point<double>& position) {

	foreach (boost::shared_ptr<EndSegment> end, ends) {

		// we are only interested in ends having the slice on the right
		if (end->getDirection() == Right)
			continue;

		LOG_ALL(splitmergelog) << "found an end segment" << std::endl;

		if (end->getSlice()->getComponent()->getBoundingBox().contains(position)) {

			if (_selection->contains(end)) {

				LOG_ALL(splitmergelog) << "remove it from selection" << std::endl;
				_selection->remove(end);

			} else {

				LOG_ALL(splitmergelog) << "add it to the selection" << std::endl;
				_selection->add(end);
			}

			setDirty(_painter);

		} else {

			LOG_ALL(splitmergelog) << "...but it doesn't contain the click position" << std::endl;
		}

		return;
	}

	LOG_ALL(splitmergelog) << "couldn't find an end segment" << std::endl;
}

void
SplitMerge::probe(std::vector<boost::shared_ptr<ContinuationSegment> >& continuations, const util::point<double>& position) {

	foreach (boost::shared_ptr<ContinuationSegment> continuation, continuations) {

		LOG_ALL(splitmergelog) << "found a continuation segment" << std::endl;

		boost::shared_ptr<Slice> slice = (continuation->getDirection() == Left ? continuation->getSourceSlice() : continuation->getTargetSlice());

		if (slice->getComponent()->getBoundingBox().contains(position)) {

			if (_selection->contains(continuation)) {

				LOG_ALL(splitmergelog) << "remove it from selection" << std::endl;
				_selection->remove(continuation);

			} else {

				LOG_ALL(splitmergelog) << "add it to the selection" << std::endl;
				_selection->add(continuation);
			}

			setDirty(_painter);

		} else {

			LOG_ALL(splitmergelog) << "...but it doesn't contain the click position" << std::endl;
		}

		return;
	}

	LOG_ALL(splitmergelog) << "couldn't find a continuation segment" << std::endl;
}

void
SplitMerge::probe(std::vector<boost::shared_ptr<BranchSegment> >& branches, const util::point<double>& position) {}
