#include <util/Logger.h>
#include "SplitMerge.h"

logger::LogChannel splitmergelog("splitmergelog", "[SplitMerge] ");

SplitMerge::SplitMerge() :
	_segments(boost::make_shared<Segments>()),
	_painter(boost::make_shared<SplitMergePainter>()),
	_selection(boost::make_shared<std::set<boost::shared_ptr<Slice> > >()),
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
		std::set<boost::shared_ptr<Slice> > firstSlices;
		unsigned int numNewEnds = 0;
		foreach (boost::shared_ptr<Segment> segment, _segments->getSegments(1)) {

			if (segment->getDirection() == Right)
				foreach (boost::shared_ptr<Slice> slice, segment->getSourceSlices())
					firstSlices.insert(slice);
			else
				foreach (boost::shared_ptr<Slice> slice, segment->getTargetSlices())
					firstSlices.insert(slice);
		}
		foreach (boost::shared_ptr<Slice> slice, firstSlices) {

			boost::shared_ptr<EndSegment> newEnd = boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Left, slice);
			_segments->add(newEnd);

			numNewEnds++;
		}

		LOG_DEBUG(splitmergelog) << "added " << numNewEnds << " new ends" << std::endl;

		_initialSegmentsProcessed = true;
	}

	_painter->setSection(*_section);
	_painter->updateSize();
}

void
SplitMerge::onInputSet(const pipeline::InputSetBase& /*signal*/) {

	_initialSegmentsProcessed = false;

	LOG_DEBUG(splitmergelog) << "initial segments have been set" << std::endl;
}

void
SplitMerge::onKeyDown(const gui::KeyDown& signal) {

	if (signal.key == gui::keys::M || signal.key == gui::keys::S) {

		// merge! or split!
		bool doMerge = (signal.key == gui::keys::M);

		std::vector<boost::shared_ptr<Slice> > prevSlices;
		std::vector<boost::shared_ptr<Slice> > currSlices;

		// compare each section with the previous section
		for (unsigned int interval = 0; interval < _segments->getNumInterSectionIntervals(); interval++) {

			foreach (boost::shared_ptr<Slice> slice, *_selection) {

				// slices to the right
				if (slice->getSection() == interval)
					currSlices.push_back(slice);
			}

			LOG_ALL(splitmergelog) << "merging in inter-section interval " << interval << std::endl;
			LOG_ALL(splitmergelog) << "previous slices: " << prevSlices.size() << ", current slices " << currSlices.size() << std::endl;

			// is there something to merge?
			if (prevSlices.size()*currSlices.size() > 0) {

				std::vector<Link> oldLinks;

				removeSegments(prevSlices, Right, interval, oldLinks);
				removeSegments(currSlices, Left, interval, oldLinks);

				if (doMerge) {

					// enumerate all possible links between the slices
					std::vector<Link> links(oldLinks);
					foreach(boost::shared_ptr<Slice> p, prevSlices)
						foreach (boost::shared_ptr<Slice> c, currSlices)
							links.push_back(Link(p, c));

					mergeSlices(interval, links);

				} else { // do a split

					std::set<boost::shared_ptr<Slice> > keptPrevSlices;
					std::set<boost::shared_ptr<Slice> > keptCurrSlices;

					// get all links that should be kept, i.e., not both of 
					// their slices are selected
					std::vector<Link> keptLinks;
					foreach (Link& link, oldLinks) {

						if (std::count(prevSlices.begin(), prevSlices.end(), link.left) == 0) {
							keptPrevSlices.insert(link.left);
							keptLinks.push_back(link);
						}
						
						if (std::count(currSlices.begin(), currSlices.end(), link.right) == 0) {

							keptCurrSlices.insert(link.right);
							keptLinks.push_back(link);
						}
					}

					// re-merge the slices of the links that should be kept
					mergeSlices(interval, keptLinks);

					// end all slices that are not in the kept links
					std::vector<boost::shared_ptr<Slice> > delPrevSlices;
					std::vector<boost::shared_ptr<Slice> > delCurrSlices;

					foreach (boost::shared_ptr<Slice> slice, prevSlices)
						if (keptPrevSlices.count(slice) == 0)
							delPrevSlices.push_back(slice);
					foreach (boost::shared_ptr<Slice> slice, currSlices)
						if (keptCurrSlices.count(slice) == 0)
							delCurrSlices.push_back(slice);

					endSlices(delPrevSlices, delCurrSlices);
				}
			}

			std::swap(prevSlices, currSlices);
			currSlices.clear();
		}

		setDirty(_segments);
	}
}

void
SplitMerge::removeSegments(std::vector<boost::shared_ptr<Slice> >& slices, Direction direction, unsigned int interval, std::vector<Link>& oldLinks) {

	// remove all segments in the given interval that are starting from any of 
	// the given slices in the given direction

	// remember the links that have been represented by these segments -- they 
	// should be maintained

	std::vector<boost::shared_ptr<Segment> > segments = _segments->getSegments(interval);

	foreach (boost::shared_ptr<Segment> segment, segments) {
		if (segment->getDirection() == direction) {
			foreach (boost::shared_ptr<Slice> slice, slices) {
				foreach (boost::shared_ptr<Slice> segmentSlice, segment->getSourceSlices()) {
					if (segmentSlice->getId() == slice->getId()) {

						_segments->remove(segment);

						LOG_DEBUG(splitmergelog) << "removed a segment using slice " << slice->getId() << " in interval " << interval << std::endl;

						foreach (boost::shared_ptr<Slice> p, (direction == Right ? segment->getSourceSlices() : segment->getTargetSlices()))
							foreach (boost::shared_ptr<Slice> c, (direction == Right ? segment->getTargetSlices() : segment->getSourceSlices())) {

								LOG_ALL(splitmergelog) << "remembering link between " << p->getId() << " and " << c->getId() << std::endl;
								oldLinks.push_back(Link(p, c));
							}
					}
				}
			}
		}
	}
}

void
SplitMerge::mergeSlices(unsigned int interval, std::vector<Link>& links) {

	// get all slices that are involved
	std::set<boost::shared_ptr<Slice> > slices;
	foreach (Link& link, links) {
		slices.insert(link.left);
		slices.insert(link.right);
	}
	unsigned int numSlices = slices.size();

	LOG_ALL(splitmergelog) << "merging " << numSlices << " slices" << std::endl;

	std::sort(links.begin(), links.end());

	std::map<boost::shared_ptr<Slice>, std::set<boost::shared_ptr<Slice> > > partners;

	// process links from closest to furthest to identify partner sets, stop as 
	// soon as all slices are in one set
	foreach (Link& link, links) {

		// if we found partners for each slice
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

		// all slices have been linked by segments
		if (linkedSlices.size() == numSlices)
			break;

		// this slice has been linked already
		if (linkedSlices.count(s))
			continue;

		// now we connect s to all its partners ps with segments

		// remember all slices that have been handled so far
		foreach (boost::shared_ptr<Slice> p, ps)
			linkedSlices.insert(p);

		// if there is only one partner, it's the slice itself, so this has to 
		// become an end
		if (ps.size() == 0) {

			boost::shared_ptr<Slice> slice = *ps.begin();
			if (slice->getSection() == interval)
				_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Left, slice));
			else
				_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Right, slice));

			continue;
		}

		// for each pair of partners, introduce a continuation segment
		typename std::set<boost::shared_ptr<Slice> >::iterator i, j;
		for (i = ps.begin(); i != ps.end(); i++)
			for (j = i; j != ps.end(); j++) {

				// if both are in the same section
				if ((*i)->getSection() == (*j)->getSection())
					continue;

				boost::shared_ptr<Slice> left;
				boost::shared_ptr<Slice> right;

				if ((*i)->getSection() < (*j)->getSection()) {

					left  = *i;
					right = *j;

				} else {

					left  = *j;
					right = *i;
				}

				boost::shared_ptr<ContinuationSegment> continuation = boost::make_shared<ContinuationSegment>(Segment::getNextSegmentId(), Right, left, right);

				_segments->add(continuation);

				LOG_DEBUG(splitmergelog) << "added a continuation between slice " << left->getId() << " and " << right->getId() << std::endl;
			}
	}
}

void
SplitMerge::endSlices(std::vector<boost::shared_ptr<Slice> >& prevSlices, std::vector<boost::shared_ptr<Slice> >& currSlices) {

	foreach (boost::shared_ptr<Slice> slice, prevSlices)
		_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Right, slice));

	foreach (boost::shared_ptr<Slice> slice, currSlices)
		_segments->add(boost::make_shared<EndSegment>(Segment::getNextSegmentId(), Left, slice));
}

void
SplitMerge::onMouseDown(const gui::MouseDown& signal) {

	if (signal.modifiers & gui::keys::ControlDown)
		return;

	LOG_ALL(splitmergelog) << "mouse down" << std::endl;

	if (signal.button == gui::buttons::Left) {

		// find closest segments in intersection interval to the left and right
		std::vector<boost::shared_ptr<Segment> > closestSegments;
		for (int section = *_section; section <= *_section + 1; section++) {

			std::vector<boost::shared_ptr<EndSegment> >          ends          = _segments->findEnds(signal.position, *_section, 1000000);
			std::vector<boost::shared_ptr<ContinuationSegment> > continuations = _segments->findContinuations(signal.position, *_section, 1000000);

			std::copy(ends.begin(),          ends.end(),          std::back_inserter(closestSegments));
			std::copy(continuations.begin(), continuations.end(), std::back_inserter(closestSegments));
		}

		// get all the slices that are in the current section
		std::vector<boost::shared_ptr<Slice> > closestSlices;

		foreach (boost::shared_ptr<Segment> segment, closestSegments) {

			foreach (boost::shared_ptr<Slice> slice, segment->getSourceSlices())
				if (slice->getSection() == (unsigned int)*_section)
					closestSlices.push_back(slice);
			foreach (boost::shared_ptr<Slice> slice, segment->getTargetSlices())
				if (slice->getSection() == (unsigned int)*_section)
					closestSlices.push_back(slice);
		}

		if (closestSlices.size() == 0) {

			LOG_ALL(splitmergelog) << "found no slices in this section" << std::endl;
			return;
		}

		// find the closest of all slices
		boost::shared_ptr<Slice> closestSlice;
		double distance = 0;
		for (unsigned int i = 0; i < closestSlices.size(); i++) {

			util::point<double> center = closestSlices[i]->getComponent()->getCenter();
			double d = pow(center.x - signal.position.x, 2) + pow(center.y - signal.position.y,2);

			if (!closestSlice || distance > d) {

				closestSlice = closestSlices[i];
				distance = d;
			}
		}

		LOG_ALL(splitmergelog) << "slice " << closestSlice->getId() << " is closest to click" << std::endl;

		if (_selection->count(closestSlice)) {

			LOG_ALL(splitmergelog) << "remove it from selection" << std::endl;
			_selection->erase(_selection->find(closestSlice));

		} else {

			LOG_ALL(splitmergelog) << "add it to selection" << std::endl;
			_selection->insert(closestSlice);
		}

		setDirty(_painter);
	}

	if (signal.button == gui::buttons::Right) {

		_selection->clear();
		setDirty(_painter);
	}
}

