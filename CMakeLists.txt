cmake_minimum_required(VERSION 3.12)

#----- Append EC specific module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_rpn)

include(ec_init)           # Initialise compilers and ec specific functions
ec_git_version()           # Get version from git state
ec_parse_manifest()        # Parse MANIFEST file

project(${NAME} DESCRIPTION "${DESCRIPTION}")
set(PROJECT_VERSION ${VERSION})

ec_build_info()              # Generate build include file
ec_install_prefix(${NAME} ${VERSION} ec_prefix) # Define install prefix  
set(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${ec_prefix})

enable_language(C)

include(ec_compiler_presets)

#----- Specific tests
if(DEFINED ENV{CRAYPE_VERSION})
   set(ENV{CRAYPE_LINK_TYPE} dynamic)
endif()

#TODO define elsewhere
set(TCL_SRC_DIR "$ENV{SSM_DEV}/SPI-External/tcl${TCL_REQ_VERSION}")

#----- Look for non system default gdal
execute_process(
      COMMAND gdal-config --prefix
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      RESULT_VARIABLE GDAL_RESULT
      OUTPUT_VARIABLE GDAL_ROOT
      ERROR_VARIABLE GDAL_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
   )

#----- Packages that create imported targets
find_package(RMN ${RMN_REQ_VERSION} COMPONENTS beta SHARED OPTIONAL)
find_package(VGRID ${VGRID_REQ_VERSION} COMPONENTS SHARED OPTIONAL)
find_package(LibXml2 REQUIRED)

find_package(GDAL ${GDAL_REQ_VERSION})
if(GDAL_FOUND)
   target_compile_definitions(GDAL::GDAL INTERFACE HAVE_GDAL)
endif()

if(DEFINED TCL_SRC_DIR)
   add_compile_definitions(HAVE_TCL TCL_THREADS)
   include_directories(${TCL_SRC_DIR}/unix ${TCL_SRC_DIR}/generic)
endif()

find_package(MPI REQUIRED COMPONENTS C)
find_package(OpenMP REQUIRED COMPONENTS C)

add_compile_definitions(_BUILD_EERUTILS HAVE_GPC _${CMAKE_SYSTEM_NAME}_ _GNU_SOURCE STDC_HEADERS)

add_subdirectory(src src)
add_subdirectory(util util)

set(ENV_CMDS_EXT_VERSION $ENV{CMDS_EXT_VERSION})
set(ENV_INTELCOMP_HOME $ENV{INTEL_COMP_HOME})

ec_build_config()          # Create build configuration script
ec_prepare_ssm()           # Prepare ssm packaging files

set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_VENDOR "ECCC")
set(CPACK_PACKAGE_CONTACT "${MAINTAINER}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/package")
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CPACK_PACKAGE_FILE_NAME "${ec_prefix}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${NAME}")
include(CPack)
