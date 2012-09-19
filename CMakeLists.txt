cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#######################
# project information #
#######################

project(sopnet)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

#####################
# required packages #
#####################

message(STATUS ${CMAKE_MODULE_PATH})

find_package(Boost 1.42 COMPONENTS filesystem program_options serialization signals system thread REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIR})
  message(STATUS "Boost found.")
else()
  message(STATUS "Boost *NOT* found.")
endif()

enable_language(Fortran)
find_package(LAPACK REQUIRED)
if (LAPACK_FOUND)
  message(STATUS "LAPACK found")
else()
  message(STATUS "LAPACK *NOT* found")
endif()

find_package(X11 REQUIRED)
if (X11_Xrandr_FOUND)
  include_directories(${X11_INCLUDE_DIR})
  message(STATUS "Xrandr found")
else()
  message(STATUS "Xrandr *NOT* found")
endif()

find_package(OpenGL)
if(OPENGL_FOUND)
  message(STATUS "OpenGL found.")
else()
  message(STATUS "OpenGL *NOT* found.")
endif()

find_package(GLEW)
if (GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_DIR})
  message(STATUS "GLEW found.")
else()
  message(STATUS "GLEW *NOT* found.")
endif()

find_package(GLUT)
if (GLUT_FOUND)
  include_directories(${GLUT_INCLUDE_DIR})
  message(STATUS "GLUT found.")
else()
  message(STATUS "GLUT *NOT* found.")
endif()

find_package(Vigra REQUIRED)
if (Vigra_FOUND)
  include_directories(${Vigra_INCLUDE_DIR})
  message(STATUS "Vigra found.")
else()
  message(STATUS "Vigra *NOT* found.")
endif()

find_package(HDF5 1.8.5)
if (HDF5_FOUND)
  find_package(HDF5 1.8.5 COMPONENTS CXX HL)
  include_directories(${HDF5_INCLUDE_DIRS})
  message(STATUS "HDF5 found.")
  set(HAVE_HDF5 1)
else()
  # try to find older version without HL
  find_package(HDF5 COMPONENTS CXX)
  if (HDF5_FOUND)
    include_directories(${HDF5_INCLUDE_DIRS})
    message(STATUS "HDF5 found.")
    set(HAVE_HDF5 1)
  else()
    message(STATUS "HDF5 *NOT* found.")
    set(HAVE_HDF5 0)
  endif()
endif()

#####################
# optional packages #
#####################

find_package(CPLEX)
if(CPLEX_FOUND)
  include_directories(${CPLEX_INCLUDE_DIRS})
  add_definitions(-DHAVE_CPLEX)
  add_definitions(-DIL_STD)
  message(STATUS "CPLEX found")
else()
  message(STATUS "CPLEX not found")
endif()

find_package(Gurobi)
if(GUROBI_FOUND)
  include_directories(${Gurobi_INCLUDE_DIRS})
  link_directories(${Gurobi_LIBRARY_DIR})
  add_definitions(-DHAVE_GUROBI)
  message(STATUS "Gurobi found " ${Gurobi_LIBRARY_DIR})
else()
  message(STATUS "Gurobi *NOT* found")
endif()

find_package(Cairo)
if(Cairo_FOUND)
  include_directories(${Cairo_INCLUDE_DIR})
  message(STATUS "Cairo found.")
  set(HAVE_CAIRO 1)
else()
  message(STATUS "Cairo *NOT* found.")
  set(HAVE_CAIRO 0)
endif()

find_package(PNG)
if(PNG_FOUND)
  message(STATUS "PNG found.")
  set(HAVE_PNG 1)
else()
  message(STATUS "PNG *NOT* found.")
  set(HAVE_PNG 0)
endif()

find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/doxygen.in
  ${PROJECT_BINARY_DIR}/doxygen.conf
  )
  add_custom_target(
  doc
  ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/doxygen.conf
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT "Generating Doxygen documentation"
  VERBATIM
  )
  message(STATUS "Doxygen found.")
else()
  message(STATUS "Doxygen *NOT* found.")
endif()

###############
# config file #
###############

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/config.h.in
  ${PROJECT_BINARY_DIR}/config.h
)

############
# all libs #
############

set(CORELIBS
  ${LAPACK_LIBRARIES}
  ${X11_X11_LIB}
  ${X11_Xrandr_LIB}
  ${OPENGL_LIBRARY}
  ${GLEW_LIBRARY}
  ${GLUT_LIBRARIES}
  ${PNG_LIBRARY}
  ${Boost_LIBRARIES}
  ${Cairo_LIBRARY}
  ${CPLEX_LIBRARIES}
  ${Gurobi_LIBRARIES}
  ${Vigra_LIBRARIES}
  ${HDF5_LIBRARIES}
)

#######################
# project directories #
#######################

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/modules)

add_subdirectory(modules/util)
add_subdirectory(modules/signals)
add_subdirectory(modules/pipeline)
add_subdirectory(modules/imageprocessing)
add_subdirectory(modules/gui)

#####################
# compiler settings #
#####################

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fomit-frame-pointer -Wno-deprecated -Wno-invalid-offsetof")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -Wno-deprecated -Wno-invalid-offsetof")

###############
# main target #
###############

file (GLOB_RECURSE sopnetfiles
  external/dgc/*.cpp
  sopnet/*.cpp
  gui/*.cpp
  imageprocessing/*.cpp
  io/*.cpp
  inference/*.cpp
  toys/*.cpp)

add_library(allsopnet ${sopnetfiles})
target_link_libraries(allsopnet util signals pipeline imageprocessing gui ${CORELIBS})

add_executable(sopnet sopnet.cpp)
target_link_libraries(sopnet allsopnet)

add_executable(graphcut graphcut.cpp)
target_link_libraries(graphcut allsopnet)
