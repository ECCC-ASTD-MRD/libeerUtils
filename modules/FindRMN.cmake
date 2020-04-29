# TODO We should have code to check the version

# [[DOC]] for find_package (lists variables that are set automatically by CMake)
# https://cmake.org/cmake/help/v3.0/command/find_package.html

# [[DOC]] https://cmake.org/cmake/help/v3.14/manual/cmake-developer.7.html
set(RMN_VERSION ${RMN_FIND_VERSION})

#find_path(RMN_INCLUDE_DIR
#   NAMES rpnmacros.h
#   PATHS ENV CPATH NO_DEFAULT_PATH)
find_path(RMN_INCLUDE_DIR
   NAMES rpn_macros_arch.h
   PATHS ${ECCC_INCLUDE_PATH} NO_DEFAULT_PATH)

# [[DOC]] for find_library https://cmake.org/cmake/help/latest/command/find_library.html
if("shared" IN_LIST RMN_FIND_COMPONENTS)
    if("threaded" IN_LIST RMN_FIND_COMPONENTS)
        find_library(RMN_LIBRARY
           NAMES rmnsharedPTHRD
           PATHS ${ECCC_LD_LIBRARY_PATH} NO_DEFAULT_PATH)
    else()
       find_library(RMN_LIBRARY
           NAMES rmnshared
           PATHS ${ECCC_LD_LIBRARY_PATH} NO_DEFAULT_PATH)
    endif()
else()
    find_library(RMN_LIBRARY
        NAMES rmn
        PATHS ${ECCC_LD_LIBRARY_PATH} NO_DEFAULT_PATH)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RMN DEFAULT_MSG RMN_LIBRARY RMN_INCLUDE_DIR)
mark_as_advanced(RMN_INCLUDE_DIR RMN_LIBRARY)

set(RMN_INCLUDE_DIRS ${RMN_INCLUDE_DIR})
set(RMN_LIBRARIES ${RMN_LIBRARY})

if("rpnpy" IN_LIST RMN_FIND_COMPONENTS)
    message("Component rpnpy requested")
    # Let's say we don't find it
    set(RMN_rpnpy_FOUND)
    if(${RMN_FIND_REQUIRED_rpnpy})
        message(FATAL_ERROR "Could not find required component rpnpy")
    else()
        message("Could not find component rpnpy")
    endif()
endif()
    
