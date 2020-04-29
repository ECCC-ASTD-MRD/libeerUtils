# TODO We should have code to check the version

# [[DOC]] for find_package (lists variables that are set automatically by CMake)
# https://cmake.org/cmake/help/v3.0/command/find_package.html
# [[DOC]] https://cmake.org/cmake/help/v3.14/manual/cmake-developer.7.html
set(VGRID_VERSION ${VGRID_FIND_VERSION})
find_path(VGRID_INCLUDE_DIR
   NAMES vgrid.h
   PATHS ${ECCC_INCLUDE_PATH} NO_DEFAULT_PATH)
       
# [[DOC]] for find_library https://cmake.org/cmake/help/latest/command/find_library.html
if("shared" IN_LIST VGRID_FIND_COMPONENTS)
   find_library(VGRID_LIBRARY
      NAMES vgridshared
      PATHS ${ECCC_LD_LIBRARY_PATH} NO_DEFAULT_PATH)
else()
   find_library(VGRID_LIBRARY
      NAMES vgrid
      PATHS ${ECCC_LD_LIBRARY_PATH} NO_DEFAULT_PATH)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VGRID DEFAULT_MSG VGRID_LIBRARY VGRID_INCLUDE_DIR)
mark_as_advanced(VGRID_INCLUDE_DIR VGRID_LIBRARY)

set(VGRID_INCLUDE_DIRS ${VGRID_INCLUDE_DIR})
set(VGRID_LIBRARIES ${VGRID_LIBRARY})
