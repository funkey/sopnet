cmake_minimum_required(VERSION 2.6)
include(${CMAKE_SOURCE_DIR}/cmake/include/define_module.cmake)

#######################
# project information #
#######################

project(sopnet)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

#####################
# required packages #
#####################


###############
# config file #
###############

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/config.h.in
  ${PROJECT_BINARY_DIR}/config.h
)

#######################
# project directories #
#######################

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(modules)
add_subdirectory(binaries)

#####################
# compiler settings #
#####################

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wextra -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -Wall -Wextra")

###############
# main target #
###############

#add_library(allsopnet ${sopnetfiles})
#target_link_libraries(allsopnet signals pipeline imageprocessing gui util ${CORELIBS})

#add_executable(sopnet sopnet.cpp)
#target_link_libraries(sopnet allsopnet)

#add_executable(graphcut graphcut.cpp)
#target_link_libraries(graphcut allsopnet)

#add_executable(problemdump problemdump.cpp)
#target_link_libraries(problemdump allsopnet)

#add_executable(solve_subproblems solve_subproblems.cpp)
#target_link_libraries(solve_subproblems allsopnet)
