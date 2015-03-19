#include "DistanceToleranceFunction.h"
#include <util/Logger.h>
#include <vigra/multi_distance.hxx>
//#include <vigra/multi_impex.hxx>

logger::LogChannel distancetolerancelog("distancetolerancelog", "[DistanceToleranceFunction] ");

DistanceToleranceFunction::DistanceToleranceFunction(
		float distanceThreshold,
		bool haveBackgroundLabel,
		float backgroundLabel) :
	_haveBackgroundLabel(haveBackgroundLabel),
	_backgroundLabel(backgroundLabel),
	_maxDistanceThreshold(distanceThreshold) {

	_resolutionX = 4.0;
	_resolutionY = 4.0;
	_resolutionZ = 40.0;
}

void
DistanceToleranceFunction::extractCells(
		unsigned int numCells,
		const vigra::MultiArray<3, unsigned int>& cellLabels,
		const ImageStack& recLabels,
		const ImageStack& gtLabels) {

	_depth  = gtLabels.size();
	_width  = gtLabels.width();
	_height = gtLabels.height();

	createBoundaryMap(recLabels);
	createBoundaryDistanceMap();

	//vigra::exportVolume(cellLabels, vigra::VolumeExportInfo("cell_labels/cell_labels", ".tif").setPixelType("FLOAT"));
	//vigra::exportVolume(_boundaryMap, vigra::VolumeExportInfo("boundaries/boundaries", ".tif").setPixelType("FLOAT"));
	//vigra::exportVolume(_boundaryDistance2, vigra::VolumeExportInfo("distances/boundary_distance2", ".tif").setPixelType("FLOAT"));

	// create a cell for each found connected component in cellLabels
	_cells->resize(numCells);

	// the maximum boundary distance of any location for each cell
	std::vector<float> maxBoundaryDistances(numCells, 0);

	std::set<unsigned int> foundCells;
	for (unsigned int z = 0; z < _depth; z++) {

		boost::shared_ptr<const Image> gt  = gtLabels[z];
		boost::shared_ptr<const Image> rec = recLabels[z];

		for (unsigned int x = 0; x < _width; x++)
			for (unsigned int y = 0; y < _height; y++) {

				float gtLabel  = (*gt)(x, y);
				float recLabel = (*rec)(x, y);

				// argh, vigra starts counting at 1!
				unsigned int cellIndex = cellLabels(x, y, z) - 1;

				(*_cells)[cellIndex].add(cell_t::Location(x, y, z));
				(*_cells)[cellIndex].setReconstructionLabel(recLabel);
				(*_cells)[cellIndex].setGroundTruthLabel(gtLabel);

				maxBoundaryDistances[cellIndex] = std::max(maxBoundaryDistances[cellIndex], _boundaryDistance2(x, y, z));

				if (foundCells.count(cellIndex) == 0) {

					registerPossibleMatch(gtLabel, recLabel);
					foundCells.insert(cellIndex);
				}
			}
	}

	findRelabelCandidates(maxBoundaryDistances);

	enumerateCellLabels(recLabels);
}

void
DistanceToleranceFunction::findRelabelCandidates(const std::vector<float>& maxBoundaryDistances) {

	_relabelCandidates.clear();
	for (unsigned int cellIndex = 0; cellIndex < maxBoundaryDistances.size(); cellIndex++)
		if (maxBoundaryDistances[cellIndex] <= _maxDistanceThreshold*_maxDistanceThreshold)
			_relabelCandidates.push_back(cellIndex);
}

void
DistanceToleranceFunction::createBoundaryMap(const ImageStack& recLabels) {

	vigra::Shape3 shape(_width, _height, _depth);
	_boundaryMap.reshape(shape);

	// create boundary map
	LOG_DEBUG(distancetolerancelog) << "creating boundary map of size " << shape << std::endl;
	_boundaryMap = 0.0f;
	for (unsigned int x = 0; x < _width; x++)
		for (unsigned int y = 0; y < _height; y++)
			for (unsigned int z = 0; z < _depth; z++)
				if (isBoundaryVoxel(x, y, z, recLabels))
					_boundaryMap(x, y, z) = 1.0f;
}

void
DistanceToleranceFunction::createBoundaryDistanceMap() {

	vigra::Shape3 shape(_width, _height, _depth);
	_boundaryDistance2.reshape(shape);

	float pitch[3];
	pitch[0] = _resolutionX;
	pitch[1] = _resolutionY;
	pitch[2] = _resolutionZ;

	// compute l2 distance for each pixel to boundary
	LOG_DEBUG(distancetolerancelog) << "computing boundary distances" << std::endl;
	vigra::separableMultiDistSquared(
			_boundaryMap,
			_boundaryDistance2,
			true /* background */,
			pitch);
}

void
DistanceToleranceFunction::enumerateCellLabels(const ImageStack& recLabels) {

	_maxDistanceThresholdX = std::min((float)_width,  _maxDistanceThreshold/_resolutionX);
	_maxDistanceThresholdY = std::min((float)_height, _maxDistanceThreshold/_resolutionY);
	_maxDistanceThresholdZ = std::min((float)_depth,  _maxDistanceThreshold/_resolutionZ);

	LOG_DEBUG(distancetolerancelog) << "there are " << _relabelCandidates.size() << " cells that can be relabeled" << std::endl;

	if (_relabelCandidates.size() == 0)
		return;

	LOG_DEBUG(distancetolerancelog) << "creating distance threshold neighborhood" << std::endl;

	// list of all location offsets within threshold distance
	std::vector<cell_t::Location> neighborhood = createNeighborhood();

	LOG_DEBUG(distancetolerancelog) << "there are " << neighborhood.size() << " pixels in the neighborhood for a threshold of " << _maxDistanceThreshold << std::endl;

	// for each cell
	foreach (unsigned int index, _relabelCandidates) {

		cell_t& cell = (*_cells)[index];

		LOG_ALL(distancetolerancelog) << "processing cell " << index << " (label " << cell.getReconstructionLabel() << ")" << std::flush;

		std::set<float> alternativeLabels = getAlternativeLabels(cell, neighborhood, recLabels);

		LOG_ALL(distancetolerancelog) << "; can map to ";

		// for each alternative label
		foreach (float recLabel, alternativeLabels) {

			LOG_ALL(distancetolerancelog) << recLabel << " ";

			// register possible match
			cell.addAlternativeLabel(recLabel);
			registerPossibleMatch(cell.getGroundTruthLabel(), recLabel);
		}
		LOG_ALL(distancetolerancelog) << std::endl;

		// DEBUG
		index++;
	}
}

bool
DistanceToleranceFunction::isBoundaryVoxel(int x, int y, int z, const ImageStack& stack) {

	// voxels at the volume borders are always boundary voxels
	if (x == 0 || x == (int)_width - 1)
		return true;
	if (y == 0 || y == (int)_height - 1)
		return true;
	// in z only if there are multiple sections
	if (_depth > 1 && (z == 0 || z == (int)_depth - 1))
		return true;

	float center = (*stack[z])(x, y);

	if (x > 0)
		if ((*stack[z])(x - 1, y) != center)
			return true;
	if (x < (int)_width - 1)
		if ((*stack[z])(x + 1, y) != center)
			return true;
	if (y > 0)
		if ((*stack[z])(x, y - 1) != center)
			return true;
	if (y < (int)_height - 1)
		if ((*stack[z])(x, y + 1) != center)
			return true;
	if (z > 0)
		if ((*stack[z - 1])(x, y) != center)
			return true;
	if (z < (int)_depth - 1)
		if ((*stack[z + 1])(x, y) != center)
			return true;

	return false;
}

std::vector<DistanceToleranceFunction::cell_t::Location>
DistanceToleranceFunction::createNeighborhood() {

	std::vector<cell_t::Location> thresholdOffsets;

	// quick check first: test on all three axes -- if they contain all covering
	// labels already, we can abort iterating earlier in getAlternativeLabels()

	for (int z = 1; z <= _maxDistanceThresholdZ; z++) {

		thresholdOffsets.push_back(cell_t::Location(0, 0,  z));
		thresholdOffsets.push_back(cell_t::Location(0, 0, -z));
	}
	for (int y = 1; y <= _maxDistanceThresholdY; y++) {

		thresholdOffsets.push_back(cell_t::Location(0,  y, 0));
		thresholdOffsets.push_back(cell_t::Location(0, -y, 0));
	}
	for (int x = 1; x <= _maxDistanceThresholdX; x++) {

		thresholdOffsets.push_back(cell_t::Location( x, 0, 0));
		thresholdOffsets.push_back(cell_t::Location(-x, 0, 0));
	}

	for (int z = -_maxDistanceThresholdZ; z <= _maxDistanceThresholdZ; z++)
		for (int y = -_maxDistanceThresholdY; y <= _maxDistanceThresholdY; y++)
			for (int x = -_maxDistanceThresholdX; x <= _maxDistanceThresholdX; x++) {

				// axis locations have been added already, center is not needed
				if ((x == 0 && y == 0) || (x == 0 && z == 0) || (y == 0 && z == 0))
					continue;

				// is it within threshold distance?
				if (
						x*_resolutionX*x*_resolutionX +
						y*_resolutionY*y*_resolutionY +
						z*_resolutionZ*z*_resolutionZ <= _maxDistanceThreshold*_maxDistanceThreshold)

					thresholdOffsets.push_back(cell_t::Location(x, y, z));
			}

	return thresholdOffsets;
}

std::set<float>
DistanceToleranceFunction::getAlternativeLabels(
		const cell_t& cell,
		const std::vector<cell_t::Location>& neighborhood,
		const ImageStack& recLabels) {

	float cellLabel = cell.getReconstructionLabel();

	// counts for each neighbor label, how often it was found while iterating 
	// over the cells locations
	std::map<float, unsigned int> counts;

	// the number of cell locations visited so far
	unsigned int numVisited = 0;

	// the maximal number of alternative labels, starts with number of labels 
	// found at first location and decreases whenever one label was not found
	unsigned int maxAlternativeLabels = 0;

	// for each location i in that cell
	foreach (const cell_t::Location& i, cell) {

		// all the labels in the neighborhood of i
		std::set<float> neighborhoodLabels;

		numVisited++;

		// the number of complete neighbor labels that have been seen at the 
		// current location
		unsigned int numComplete = 0;

		// for all locations within the neighborhood, get alternative labels
		foreach (const cell_t::Location& n, neighborhood) {

			cell_t::Location j(i.x + n.x, i.y + n.y, i.z + n.z);

			// are we leaving the image?
			if (j.x < 0 || j.x >= (int)_width || j.y < 0 || j.y >= (int)_height || j.z < 0 || j.z >= (int)_depth)
				continue;

			// is this a boundary?
			if (!_boundaryMap(j.x, j.y, j.z))
				continue;

			// now we have found a boundary pixel within our neighborhood
			float label = (*(recLabels)[j.z])(j.x, j.y);

			// count how often we see a neighbor label the first time
			if (label != cellLabel) {

				bool firstTime = neighborhoodLabels.insert(label).second;

				// seen the first time for the current location i
				if (firstTime)
					// is a potential alternative label (covers all visited 
					// locations so far)
					if (++counts[label] == numVisited) {

						numComplete++;

						// if we have seen all the possible complete labels 
						// already, there is no need to search further for the 
						// current location i
						if (numComplete == maxAlternativeLabels)
							break;
					}
			}
		}

		// the number of labels that we have seen for each location visited so 
		// far is the maximal possible number of alternative labels
		maxAlternativeLabels = numComplete;

		// none of the neighbor labels covers the cell
		if (maxAlternativeLabels == 0)
			break;
	}

	std::set<float> alternativeLabels;

	// collect all neighbor labels that we have seen for every location of the 
	// cell
	float label;
	unsigned int count;
	foreach (boost::tie(label, count), counts)
		if (count == cell.size())
			alternativeLabels.insert(label);

	return alternativeLabels;
}
