cmake_minimum_required(VERSION 3.20)

#----- Append EC specific module path
list(APPEND CMAKE_MODULE_PATH $ENV{EC_CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake_rpn/modules)

include(ec_init)           # Initialise compilers and ec specific functions
ec_git_version()           # Get version from git state
ec_parse_manifest()        # Parse MANIFEST file

project(${NAME} DESCRIPTION "${DESCRIPTION}")
set(PROJECT_VERSION ${VERSION})

ec_build_info()            # Generate build include file

enable_language(C Fortran)

include(ec_compiler_presets)

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
option(WITH_OMPI "Compile with MPI support" TRUE)
if (WITH_OMPI)
   find_package(MPI REQUIRED)
   find_package(OpenMP REQUIRED)
endif()

find_package(rmn ${rmn_REQ_VERSION} COMPONENTS shared)
find_package(vgrid ${vgrid_REQ_VERSION} COMPONENTS shared)
find_package(LibXml2 REQUIRED)

find_package(GDAL ${GDAL_REQ_VERSION})
if(GDAL_FOUND)
   target_compile_definitions(GDAL::GDAL INTERFACE HAVE_GDAL)
endif()

if(DEFINED TCL_SRC_DIR)
   message(STATUS "Found TCL source: ${TCL_SRC_DIR}")
   add_compile_definitions(HAVE_TCL TCL_THREADS)
   include_directories(${TCL_SRC_DIR}/unix ${TCL_SRC_DIR}/generic)
endif()

add_compile_definitions(_BUILD_EERUTILS HAVE_GPC _${CMAKE_SYSTEM_NAME}_ _GNU_SOURCE STDC_HEADERS)

#----- Install paths
set(INCLUDE_INSTALL_DIR include)
set(LIB_INSTALL_DIR     lib)
set(CONFIG_INSTALL_DIR  "${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}")

install(EXPORT ${PROJECT_NAME}-static-targets
   NAMESPACE ${PROJECT_NAME}::
   DESTINATION ${CONFIG_INSTALL_DIR})

link_libraries("-Wl,--disable-new-dtags")
add_subdirectory(src src)
if(NOT DEFINED OMPI) 
   add_subdirectory(util util)
endif()

set(ENV_CMDS_EXT_VERSION $ENV{CMDS_EXT_VERSION})
set(ENV_INTELCOMP_HOME $ENV{CMPLR_ROOT})

#----- Generate the config file for the project to be usable via cmake's find_package command
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
    PATH_VARS           INCLUDE_INSTALL_DIR LIB_INSTALL_DIR
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)
install(FILES   "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CONFIG_INSTALL_DIR}"
)

#----- Packaging
ec_build_config()                 # Create build configuration script
ec_prepare_ssm()                  # Prepare ssm packaging files

set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_VENDOR "ECCC")
set(CPACK_PACKAGE_CONTACT "${MAINTAINER}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/package")
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
include(CPack)
