#file (GLOB_RECURSE sopnetfiles
  #external/dgc/*.cpp
  #sopnet/*.cpp
  #gui/*.cpp
  #imageprocessing/*.cpp
  #io/*.cpp
  #inference/*.cpp
  #toys/*.cpp)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR})
define_module(allsopnet LIBRARY LINKS pipeline gui imageprocessing boost hdf5 gurobi cplex)
