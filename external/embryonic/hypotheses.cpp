#include <cassert>
#include <utility>
#include <vector>
#include <boost/tuple/tuple.hpp>
#include "hypotheses.h"
#include "log.h"

using namespace std;

namespace Tracking {
  ////
  //// class HypothesesGraph
  ////
  HypothesesGraph::Node HypothesesGraph::add_node(node_timestep_map::Value timestep) {
      node_timestep_map& timestep_m = get(node_timestep());

      HypothesesGraph::Node node = addNode();
      timestep_m.set(node, timestep);
      timesteps_.insert( timestep );
      return node;
  }

  const std::set<HypothesesGraph::node_timestep_map::Value>& HypothesesGraph::timesteps() const {
    return timesteps_;
  }

  HypothesesGraph::node_timestep_map::Value HypothesesGraph::earliest_timestep() const {
    return *(timesteps_.begin());
  }

  HypothesesGraph::node_timestep_map::Value HypothesesGraph::latest_timestep() const {
    return *(timesteps_.rbegin());
  }

} /* namespace Tracking */
