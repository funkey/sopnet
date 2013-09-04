# This module finds an installed Vigra package.
#
# It sets the following variables:
#  Vigra_FOUND         - True, if vigra was found.
#  Vigra_INCLUDE_DIR   - The include directory.
#  Vigra_IMPEX_LIBRARY - Vigra impex library.
#  Vigra_NUMPY_LIBRARY - Vigra numpy library.
#  Vigra_LIBRARIES     - All vigra libraries.

set (Vigra_BUILD_DIR "" CACHE PATH "The Vigra build directory")

if (Vigra_BUILD_DIR)

  include(${Vigra_BUILD_DIR}/VigraConfig.cmake)

  set(Vigra_INCLUDE_DIR ${Vigra_INCLUDE_DIRS})

  find_library(Vigra_IMPEX_LIBRARY vigraimpex     ${Vigra_BUILD_DIR}/src/impex)
  find_library(Vigra_NUMPY_LIBRARY vigranumpycore ${Vigra_BUILD_DIR}/vigranumpy/vigra)

else()

  find_path(Vigra_INCLUDE_DIR vigra/matrix.hxx)

  find_library(Vigra_IMPEX_LIBRARY vigraimpex)

endif()

if (Vigra_INCLUDE_DIR AND Vigra_IMPEX_LIBRARY)
        set(Vigra_FOUND true)
endif()

set(Vigra_LIBRARIES ${Vigra_IMPEX_LIBRARY})
