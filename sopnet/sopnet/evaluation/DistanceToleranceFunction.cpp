#include <vigra/multi_distance.hxx>
#include <vigra/impex.hxx>

#include <external/nanoflann/nanoflann.hpp>

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

	enumerateCellLabels(recLabels, cellLabels);
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
DistanceToleranceFunction::enumerateCellLabels(const ImageStack& recLabels, const vigra::MultiArray<3, unsigned int>& cellLabels) {

	_maxDistanceThresholdX = std::min((float)_width,  _maxDistanceThreshold/_resolutionX);
	_maxDistanceThresholdY = std::min((float)_height, _maxDistanceThreshold/_resolutionY);
	_maxDistanceThresholdZ = std::min((float)_depth,  _maxDistanceThreshold/_resolutionZ);

	LOG_DEBUG(distancetolerancelog) << "there are " << _relabelCandidates.size() << " cells that can be relabeled" << std::endl;

	if (_relabelCandidates.size() == 0)
		return;

	// create kd-tree of relabel candidate boundary locations

	BoundaryLocations boundaryLocations;
	boundaryLocations.resX = (int)_resolutionX;
	boundaryLocations.resY = (int)_resolutionY;
	boundaryLocations.resZ = (int)_resolutionZ;
	foreach (unsigned int cellIndex, _relabelCandidates)
		foreach (const cell_t::Location& b, (*_cells)[cellIndex].getBoundary())
			boundaryLocations.locations.push_back(b);

	typedef nanoflann::KDTreeSingleIndexAdaptor<
		nanoflann::L2_Simple_Adaptor<int, BoundaryLocations>,
		BoundaryLocations,
		3 /* dim */
		> boundary_kd_tree_t;

	boundary_kd_tree_t index(3 /*dim*/, boundaryLocations, nanoflann::KDTreeSingleIndexAdaptorParams(10 /* max leaf */) );
	index.buildIndex();

	// map from cell indices to all alternative labels it can assume
	std::map<unsigned int, std::set<float> > mapped;

	// map from cell index X reconstruction label to covered locations
	std::map<unsigned int, std::map<float, std::set<cell_t::Location> > > covered;

	LOG_DEBUG(distancetolerancelog) << "determining label coverage" << std::endl;

	// for each location i
	int i[3];
	int& x = i[0];
	int& y = i[1];
	int& z = i[2];
	for (z = 0; z < (int)_depth; z++) {

		LOG_DEBUG(distancetolerancelog) << "processing section " << z << std::endl;

		for (y = 0; y < (int)_height; y++) {
			for (x = 0; x < (int)_width; x++) {

				// is boundary location?
				if (!_boundaryMap(x, y, z))
					continue;

				// get label k
				float k = (*recLabels[z])(x, y);


				// get all relabel candidate boundary points within threshold distance
				std::vector<std::pair<size_t, int> > points;
				nanoflann::SearchParams params(0 /* ignored parameter */);
				index.radiusSearch(&i[0], (int)(_maxDistanceThreshold*_maxDistanceThreshold), points, params);

				LOG_ALL(distancetolerancelog) << "found " << points.size() << " boundary points around (" << x << ", " << y << ", " << z << ")" << std::endl;
				size_t index;
				double dist;
				foreach (boost::tie(index, dist), points) {

					cell_t::Location& l = boundaryLocations.locations[index];

					// get cell label A
					unsigned int A = cellLabels(l.x, l.y, l.z);

					// A->k already mapped?
					if (mapped[A].count(k))
						continue;

					// add j to covered[A][k]
					covered[A][k].insert(l);

					// number of covered == boundary size of A?
					if (covered[A][k].size() == (*_cells)[A].getBoundarySize())
						mapped[A].insert(k);
				}
			}
		}
	}

	LOG_DEBUG(distancetolerancelog) << "setting alternative cell labels" << std::endl;


	typedef std::map<unsigned int, std::set<float> >::value_type mapping_t;
	foreach (mapping_t& r, mapped)
		foreach (float k, r.second)
			(*_cells)[r.first].addAlternativeLabel(k);
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
		const std::vector<cell_t::Location>& /*neighborhood*/,
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
