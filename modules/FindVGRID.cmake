# TODO We should have code to check the version

# [[DOC]] for find_package (lists variables that are set automatically by CMake)
# https://cmake.org/cmake/help/v3.0/command/find_package.html
# [[DOC]] https://cmake.org/cmake/help/v3.14/manual/cmake-developer.7.html
set(VGRID_VERSION ${VGRID_FIND_VERSION})
    find_path(VGRID_INCLUDE_DIR
       NAMES vgrid.h
       PATHS ENV CPATH NO_DEFAULT_PATH)
       
    # [[DOC]] for find_library https://cmake.org/cmake/help/latest/command/find_library.html
    if("shared" IN_LIST VGRID_FIND_COMPONENTS)
      find_library(VGRID_LIBRARIES
            NAMES vgridshared
            PATHS ENV LIBPATH NO_DEFAULT_PATH)
     else()
        find_library(VGRID_LIBRARIES
            NAMES vgrid
            PATHS ENV LIBPATH NO_DEFAULT_PATH)
       endif()

    if(NOT ${VGRID_LIBRARIES} STREQUAL "VGRID_LIBRARY-NOTFOUND")
        set(VGRID_FOUND true)
        message("-- Found libvgrid: ${VGRIDLIBRARIES} \(found version \"${VGRID_VERSION}\"\)")
    endif()
