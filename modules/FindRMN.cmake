# TODO We should have code to check the version
set(RMN_LIBRARIES "")

# [[DOC]] for find_package (lists variables that are set automatically by CMake)
# https://cmake.org/cmake/help/v3.0/command/find_package.html

# [[DOC]] https://cmake.org/cmake/help/v3.14/manual/cmake-developer.7.html
set(RMN_VERSION ${RMN_FIND_VERSION})
#string(REPLACE " " ":" ECCC_INCLUDE_PATH $ENV{EC_INCLUDE_PATH})
#find_path(RMN_INCLUDE_DIR
#   NAMES rpnmacros.h
#   PATHS ENV CPATH NO_DEFAULT_PATH)
find_path(RMN_INCLUDE_DIR
   NAMES rpn_macros_arch.h
   PATHS ENV CPATH NO_DEFAULT_PATH)

# [[DOC]] for find_library https://cmake.org/cmake/help/latest/command/find_library.html
if("shared" IN_LIST RMN_FIND_COMPONENTS)
    if("threaded" IN_LIST RMN_FIND_COMPONENTS)
        find_library(RMN_LIBRARIES
           NAMES rmnsharedPTHRD
           PATHS ENV LIBPATH NO_DEFAULT_PATH)
    else()
       find_library(RMN_LIBRARIES
           NAMES rmnshared
           PATHS ENV LIBPATH NO_DEFAULT_PATH)
    endif()
else()
    find_library(RMN_LIBRARIES
        NAMES rmn
        PATHS ENV LIBPATH NO_DEFAULT_PATH)
endif()

if(NOT ${RMN_LIBRARIES} STREQUAL "RMN_LIBRARY-NOTFOUND")
    set(RMN_FOUND true)
endif()

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
    
