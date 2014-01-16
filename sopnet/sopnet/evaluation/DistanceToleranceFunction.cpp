#include <vigra/multi_distance.hxx>
#include <vigra/impex.hxx>

#include <util/Logger.h>
#include "DistanceToleranceFunction.h"

logger::LogChannel distancetolerancelog("distancetolerancelog", "[DistanceToleranceFunction] ");

int DistanceToleranceFunction::covered_from::resX2;
int DistanceToleranceFunction::covered_from::resY2;
int DistanceToleranceFunction::covered_from::resZ2;
int DistanceToleranceFunction::covered_from::r2;

DistanceToleranceFunction::DistanceToleranceFunction(float distanceThreshold) :
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

				if (_boundaryMap(x, y, z))
					(*_cells)[cellIndex].addBoundary(cell_t::Location(x, y, z));
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

	for (unsigned int cellIndex = 0; cellIndex < numCells; cellIndex++)
		if (maxBoundaryDistances[cellIndex] < _maxDistanceThreshold*_maxDistanceThreshold)
			_relabelCandidates.push_back(cellIndex);

	enumerateCellLabels(recLabels);
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

		LOG_ALL(distancetolerancelog) << "processing cell " << index << std::endl;

		std::set<float> alternativeLabels = getAlternativeLabels(cell, neighborhood, recLabels);

		// for each alternative label
		foreach (float recLabel, alternativeLabels) {

			// register possible match
			cell.addAlternativeLabel(recLabel);
			registerPossibleMatch(cell.getGroundTruthLabel(), recLabel);
		}

		// DEBUG
		index++;
	}
}

bool
DistanceToleranceFunction::isBoundaryVoxel(int x, int y, int z, const ImageStack& stack) {

	float center = (*stack[z])(x, y);

	for (int dz = -1; dz <= 1; dz++)
		for (int dy = -1; dy <= 1; dy++)
			for (int dx = -1; dx <= 1; dx++)
				if (
						!(dx == 0 && dy == 0 && dz == 0) &&
						(x + dx) >= 0 && (x + dx) < (int)_width  &&
						(y + dy) >= 0 && (y + dy) < (int)_height &&
						(z + dz) >= 0 && (z + dz) < (int)_depth)

					if ((*stack[z + dz])(x + dx, y + dy) != center)
						return true;

	return false;
}

std::vector<DistanceToleranceFunction::cell_t::Location>
DistanceToleranceFunction::createNeighborhood() {

	std::vector<cell_t::Location> thresholdOffsets;

	for (int z = -_maxDistanceThresholdZ; z <= _maxDistanceThresholdZ; z++)
		for (int y = -_maxDistanceThresholdY; y <= _maxDistanceThresholdY; y++)
			for (int x = -_maxDistanceThresholdX; x <= _maxDistanceThresholdX; x++)
				if (
						x*_resolutionX*x*_resolutionX +
						y*_resolutionY*y*_resolutionY +
						z*_resolutionZ*z*_resolutionZ <= _maxDistanceThreshold*_maxDistanceThreshold)

					thresholdOffsets.push_back(cell_t::Location(x, y, z));

	return thresholdOffsets;
}

std::set<float>
DistanceToleranceFunction::getAlternativeLabels(
		const cell_t& cell,
		const std::vector<cell_t::Location>& neighborhood,
		const ImageStack& recLabels) {

	std::set<float> alternativeLabels;

	float cellLabel = cell.getReconstructionLabel();

	// get the extended bounding box for this cell

	cell_t::Location min = cell.getBoundingBoxMin();
	cell_t::Location max = cell.getBoundingBoxMax();

	min.x -= _maxDistanceThresholdX;
	min.y -= _maxDistanceThresholdY;
	min.z -= _maxDistanceThresholdZ;
	max.x += _maxDistanceThresholdX;
	max.y += _maxDistanceThresholdY;
	max.z += _maxDistanceThresholdZ;

	// list of all still uncovered locations in cell for each possible label
	std::map<float, std::list<cell_t::Location> > uncovered;

	// all boundary locations of the cell
	std::list<cell_t::Location> boundaryLocations(cell.getBoundary().begin(), cell.getBoundary().end());

	// prepare coverage functor
	covered_from::resX2 = (int)(_resolutionX*_resolutionX);
	covered_from::resY2 = (int)(_resolutionY*_resolutionY);
	covered_from::resZ2 = (int)(_resolutionZ*_resolutionZ);
	covered_from::r2    = (int)(_maxDistanceThreshold*_maxDistanceThreshold);

	// for each location in extended bounding box of cell
	for (int z = min.z; z < max.z; z++)
		for (int y = min.y; y < max.y; y++)
			for (int x = min.x; x < max.x; x++) {

				// within volume?
				if (x < 0 || x >= (int)_width || y < 0 || y >= (int)_height || z < 0 || z >= (int)_depth)
					continue;

				// process only boundary pixels
				if (!_boundaryMap(x, y, z))
					continue;

				// process only other labels
				float k = (*recLabels[z])(x, y);
				if (k == cellLabel)
					continue;

				// process only labels that haven't been tested positively
				if (alternativeLabels.count(k) == 1)
					continue;

				// first time we see this label?
				if (uncovered.count(k) == 0)
					uncovered[k] = boundaryLocations;

				// remove all locations within threshold of (x, y, z)
				uncovered[k].remove_if(covered_from(x, y, z));

				// completely covered?
				if (uncovered[k].size() == 0)
					alternativeLabels.insert(k);
			}

	return alternativeLabels;
}
