#ifndef SOPNET_EVALUATION_CELL_H__
#define SOPNET_EVALUATION_CELL_H__

#include <set>

/**
 * A cell is a set of connected locations build by intersecting a connected 
 * component of the ground truth with a connected component of the 
 * reconstruction.
 *
 * Cells are annotated with their original reconstruction label, as well as 
 * possible alternative reconstruction labels according to an external tolerance 
 * criterion.
 */
template <typename LabelType>
class Cell {

public:

	/**
	 * A 3D location in the volume.
	 */
	struct Location {

		Location(int x_, int y_, int z_):
			x(x_), y(y_), z(z_) {}

		int x, y, z;

		bool operator<(const Location& other) const {

			if (z < other.z)
				return true;
			else if (z > other.z)
				return false;
			else {

				if (y < other.y)
					return true;
				else if (y > other.y)
					return false;
				else {

					return x < other.x;
				}
			}
		}
	};

	/**
	 * Set the original reconstruction label of this cell.
	 */
	void setReconstructionLabel(LabelType k) {

		_label = k;
	}

	/**
	 * Get the original reconstruction label of this cell.
	 */
	LabelType getReconstructionLabel() const {

		return _label;
	}

	/**
	 * Set the ground truth label of this cell.
	 */
	void setGroundTruthLabel(LabelType k) {

		_groundTruthLabel = k;
	}

	/**
	 * Get the ground truth label of this cell.
	 */
	LabelType getGroundTruthLabel() const {

		return _groundTruthLabel;
	}

	/**
	 * Add an alternative label for this cell.
	 */
	void addAlternativeLabel(LabelType k) {

		_alternativeLabels.insert(k);
	}

	/**
	 * Get the current list of alternative labels for this cell.
	 */
	const std::set<LabelType>& getAlternativeLabels() const {

		return _alternativeLabels;
	}

	/**
	 * Add a location to this cell.
	 */
	void add(const Location& l) {

		_content.insert(l);
	}

	/**
	 * Remove a location from this cell. Returns false, if the location was not 
	 * part of this cell.
	 */
	bool remove(const Location& l) {

		iterator i = _content.find(l);

		if (i != _content.end())
			_content.erase(i);
		else
			return false;

		return true;
	}

	/**
	 * Get the number of locations in this cell.
	 */
	unsigned int size() const {

		return _content.size();
	}

	/**
	 * Iterator access to the locations of the cell.
	 */
	typedef typename std::set<Location>::iterator       iterator;
	typedef typename std::set<Location>::const_iterator const_iterator;

	iterator begin() { return _content.begin(); }
	iterator end() { return _content.end(); }
	const_iterator begin() const { return _content.begin(); }
	const_iterator end() const { return _content.end(); }

private:

	// the original reconstruction label of this cell
	LabelType _label;

	// the real label of this cell
	LabelType _groundTruthLabel;

	// possible other reconstruction labels, according to the tolerance 
	// criterion
	std::set<LabelType> _alternativeLabels;

	// the volume locations that constitute this cell
	std::set<Location> _content;
};

#endif // SOPNET_EVALUATION_CELL_H__

