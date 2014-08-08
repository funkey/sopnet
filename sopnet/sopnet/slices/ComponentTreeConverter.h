#ifndef SOPNET_COMPONENT_TREE_CONVERTER_H__
#define SOPNET_COMPONENT_TREE_CONVERTER_H__

#include <deque>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <pipeline/all.h>
#include <imageprocessing/ComponentTree.h>
#include "ConflictSets.h"
#include "Slices.h"

/**
 * Converts a component tree into a set of Slices and creates conflict sets for 
 * conflicting slices.
 *
 * Input:
 *
 * <table>
 * <tr>
 *   <td>"component tree"</td>
 *   <td>(ComponentTree)</td>
 *   <td>The input component tree</td>
 * </tr>
 * </table>
 *
 * Outputs:
 *
 * <table>
 * <tr>
 *   <td>"slices"</td>
 *   <td>(Slices)</td>
 *   <td>All slices found in the component tree.</td>
 * </tr>
 * <tr>
 *   <td>"conflict sets"</td>
 *   <td>(ConflictSets)</td>
 *   <td>Conflict sets preventing conflicting slices to be picked at the
 *   same time.</td>
 * </tr>
 * </table>
 */
class ComponentTreeConverter : public pipeline::SimpleProcessNode<>, public ComponentTree::Visitor {

public:

	/**
	 * Create a new component tree converter that creates slices from a 
	 * component tree. The produced set of slices will be annotated to be part 
	 * of the given section, coming from images having the given resolution.
	 */
	ComponentTreeConverter(unsigned int section, float resX, float resY, float resZ);

	void visitNode(boost::shared_ptr<ComponentTree::Node> node);

	void leaveNode(boost::shared_ptr<ComponentTree::Node> node);

private:

	void addConflictSet();

	static unsigned int getNextSliceId();

	static unsigned int NextSliceId;

	static boost::mutex SliceIdMutex;

	void updateOutputs();

	void convert();

	pipeline::Input<ComponentTree> _componentTree;
	pipeline::Output<Slices>       _slices;
	pipeline::Output<ConflictSets> _conflictSets;

	// the path to the currently visited component
	std::deque<unsigned int> _path;

	unsigned int _section;

	float _resX, _resY, _resZ;
};

#endif // SOPNET_COMPONENT_TREE_CONVERTER_H__

