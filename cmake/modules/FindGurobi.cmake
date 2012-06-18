SET(Gurobi_ROOT_DIR "" CACHE PATH "Gurobi root directory")

FIND_PATH(Gurobi_INCLUDE_DIR
  gurobi_c++.h
  HINTS ${Gurobi_ROOT_DIR}/include
  )

#FIND_LIBRARY(Gurobi_CPP_LIBRARY
  #gurobi_c++
  #HINTS ${Gurobi_ROOT_DIR}/lib
  #)

FIND_PATH(Gurobi_LIBRARY_DIR
  gurobi
  NAMES libgurobi_c++.a libgurobi_c++.dll
  HINTS ${Gurobi_ROOT_DIR}/lib
  )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Gurobi DEFAULT_MSG Gurobi_LIBRARY_DIR Gurobi_INCLUDE_DIR)

IF(GUROBI_FOUND)
  SET(Gurobi_INCLUDE_DIRS ${Gurobi_INCLUDE_DIR})
  SET(Gurobi_LIBRARIES gurobi_c++ gurobi50)
ENDIF(GUROBI_FOUND)

MARK_AS_ADVANCED(Gurobi_INCLUDE_DIR Gurobi_LIBRARY Gurobi_INCLUDE_DIRS Gurobi_LIBRARIES)
