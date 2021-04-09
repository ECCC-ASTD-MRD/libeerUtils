cmake_minimum_required(VERSION 3.16)

#----- Append EC specific module path
foreach(PATH $ENV{EC_CMAKE_MODULE_PATH})
   list(APPEND CMAKE_MODULE_PATH ${PATH})
endforeach()

include(ec_init)      # Include EC specific cmake utils
ec_parse_manifest()   # Parse MANIFEST file
ec_build_info()       # Generate build include file
ec_git_version()      # Get version from git 

project(${NAME} DESCRIPTION "${DESCRIPTION}")
set(PROJECT_VERSION ${VERSION})

set(CMAKE_INSTALL_PREFIX "" CACHE PATH "..." FORCE)
#set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_SKIP_BUILD_RPATH FALSE)
#set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

enable_language(C)

include(ec_compiler_presets)

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

configure_file(config.in ${CMAKE_BINARY_DIR}/${NAME}-config @ONLY)
install(PROGRAMS ${CMAKE_BINARY_DIR}/${NAME}-config DESTINATION bin)
