cmake_minimum_required(VERSION 3.16)
project(libeerUtils VERSION $ENV{VERSION} DESCRIPTION $ENV{DESCRIPTION})

list(APPEND CMAKE_MODULE_PATH $ENV{EC_CMAKE_MODULE_PATH})
set(CMAKE_INSTALL_PREFIX "" CACHE PATH "..." FORCE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

message(STATUS "Generating libeerUtils package")
message(STATUS "Build architecture ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

set(CMAKE_BUILD_TYPE "Release")
include("$ENV{EC_CMAKE_MODULE_PATH}/CMake_compiler.txt")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
add_compile_definitions(HAVE_GPC)

#find_package(RMN $ENV{RMN_VERSION} COMPONENTS shared threaded OPTIONAL)
find_package(RMN $ENV{RMN_VERSION} COMPONENTS beta shared OPTIONAL)
if (RMN_FOUND) 
   add_compile_definitions(HAVE_RMN)
   include_directories(${RMN_INCLUDE_DIR})
endif()
#message(${RMN_INCLUDE_DIR})
#message(${RMN_LIBRARIES})

find_package(VGRID $ENV{VGRID_VERSION} COMPONENTS shared OPTIONAL)
if(VGRID_FOUND)
   add_compile_definitions(HAVE_VGRID)
   include_directories(${VGRID_INCLUDE_DIR})
endif()

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

find_package(GDAL)
if(GDAL_FOUND)
   add_compile_definitions(HAVE_GDAL)
   include_directories(${GDAL_INCLUDE_DIR})
endif()

if(DEFINED TCL_SRC_DIR)
   add_compile_definitions(HAVE_TCL)
   include_directories(${TCL_SRC_DIR}/unix ${TCL_SRC_DIR}/generic)
endif()

if(${CMAKE_SYSTEM_PROCESSOR} EQUAL x86_64)
   add_compile_options(-fPIC -DSTDC_HEADERS)
endif()

if(DEFINED ENV{OMPI})
   message(STATUS "OMPI build enabled")


   find_package(MPI REQUIRED)
   include_directories(${MPI_INCLUDE_PATH})
   add_compile_definitions(_MPI)

   find_package(OpenMP REQUIRED) 
   add_compile_options(${OpenMP_C_FLAGS})
endif()

add_compile_definitions(VERSION=\"$ENV{VERSION}-r$ENV{BUILDINFO}\" _${CMAKE_SYSTEM_NAME}_ TCL_THREADS _GNU_SOURCE)

add_subdirectory(src src)
if(NOT DEFINED ENV{OMPI})
   add_subdirectory(util util)
   #add_dependencies(util src)
endif()
