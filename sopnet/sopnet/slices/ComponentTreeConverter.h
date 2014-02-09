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

	ComponentTreeConverter(unsigned int _section);

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
};

#endif // SOPNET_COMPONENT_TREE_CONVERTER_H__

