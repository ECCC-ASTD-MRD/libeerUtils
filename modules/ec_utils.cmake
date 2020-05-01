#----- Compiler selection
if(DEFINED ENV{INTEL_LICENSE_FILE})
   set(CMAKE_C_COMPILER icc)
   set(CMAKE_CXX_COMPILER icpc)
   set(CMAKE_Fortran_COMPILER ifort)
endif()

if(DEFINED ENV{CRAYPE_VERSION})
   set(CMAKE_SYSTEM_NAME CrayLinuxEnvironment)
endif()

#----- Prepare some variables
string(REPLACE " " ";" ECCC_INCLUDE_PATH $ENV{EC_INCLUDE_PATH})
string(REPLACE " " ";" ECCC_LD_LIBRARY_PATH $ENV{EC_LD_LIBRARY_PATH})

#----- Build ISO 8601 build timestamp target
# How to use:
#   - add as a target to a build
#   - add_dependencies(eerUtils$ENV{OMPI} timestamp)
#   - and include the file timestamp. to have the BUILD_TIMESTAMP variable #defined
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

function(ec_build_include)
   if(DEFINED ENV{ORDENV_SETUP})
      execute_process(COMMAND git describe --always
         OUTPUT_VARIABLE BUILD_INFO)
      string(STRIP ${BUILD_INFO} BUILD_INFO)
   endif()

   FILE (WRITE ${CMAKE_BINARY_DIR}/timestamp.cmake "string(TIMESTAMP BUILD_TIMESTAMP UTC)\n")
   FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "file(WRITE build.h \"#ifndef _BUILD_H\\n\")\n")
   FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "file(APPEND build.h \"#define _BUILD_H\\n\\n\")\n")
   FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "file(APPEND build.h \"#define BUILD_TIMESTAMP \\\"\${BUILD_TIMESTAMP}\\\"\\n\")\n")
   FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "file(APPEND build.h \"#define BUILD_INFO      \\\"${BUILD_INFO}\\\"\\n\\n\")\n")
   FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "file(APPEND build.h \"#define VERSION         \\\"${VERSION}\\\"\\n\")\n")
   FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "file(APPEND build.h \"#define DESCRIPTION     \\\"${DESCRIPTION}\\\"\\n\\n\")\n")
   FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "file(APPEND build.h \"#endif // _BUILD_H\\n\")\n")
   ADD_CUSTOM_TARGET (
      timestamp
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/timestamp.cmake
      ADD_DEPENDENCIES ${CMAKE_BINARY_DIR}/timestamp.cmake)
   include_directories(${CMAKE_BINARY_DIR})
endfunction()

#----- Parse a VERSION file
macro(ec_parse_version)
   file(STRINGS VERSION dependencies)
   foreach(line ${dependencies})
      string(REGEX MATCH "([A-Z,a-z]+)[ ]*([<,>,=,~,:]+)[ ]*(.*)" null ${line})
#      message("${CMAKE_MATCH_1}..${CMAKE_MATCH_2}..${CMAKE_MATCH_3}")
      set(LBL1 ${CMAKE_MATCH_1})
      set(LBL2 ${CMAKE_MATCH_2})
      set(LBL3 ${CMAKE_MATCH_3})
      string(TOUPPER ${LBL1} LBL1)

      if (${CMAKE_MATCH_2} MATCHES ":")
         set(${LBL1} ${LBL3}) 
      else()
         set(${LBL1}_REQ_VERSION_CHECK ${LBL2})

         string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" null ${LBL3})
         set(${LBL1}_REQ_VERSION_MAJOR ${CMAKE_MATCH_1})
         set(${LBL1}_REQ_VERSION_MINOR ${CMAKE_MATCH_2})
         set(${LBL1}_REQ_VERSION_PATCH ${CMAKE_MATCH_3})
         set(${LBL1}_REQ_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3})
#         message("${LBL1} ${CMAKE_MATCH_1}..${CMAKE_MATCH_2}..${CMAKE_MATCH_3}")
      endif()
  endforeach()

  set(CMAKE_BUILD_TYPE ${BUILD})
  message(STATUS "Generating ${NAME} package")
endmacro()

function(ec_check_version DEPENDENCY)
   message("${${DEPENDENCY}_REQ_VERSION} ${${DEPENDENCY}_REQ_VERSION_CHECK} ${${DEPENDENCY}_VERSION}")

   set(STATUS YES)

   if (${$DEPENDENCY}_REQ_VERSION_CHECK} MATCHES "=")
      if (NOT ${${DEPENDENCY}_REQ_VERSION} VERSION_EQUAL ${$DEPENDENCY}_VERSION})
         set(STATUS NO)
         message(SEND_ERROR "Found version is different")
      endif()
   elseif (${$DEPENDENCY}_REQ_VERSION_CHECK} MATCHES "<=")
      if (NOT ${${DEPENDENCY}_REQ_VERSION} VERSION_LESS_EQUAL ${$DEPENDENCY}_VERSION})
         set(STATUS NO)
         message(SEND_ERROR "Found version greather than ${$DEPENDENCY}_VERSION}")
      endif()
   elseif (${${DEPENDENCY}_REQ_VERSION_CHECK} MATCHES ">=")
      set(STATUS NO)
      if (NOT ${${DEPENDENCY}_REQ_VERSION} VERSION_GREATER_EQUAL ${$DEPENDENCY}_VERSION})
         message(SEND_ERROR "Found version is less than ${$DEPENDENCY}_VERSION}")
      endif()
   endif()

   if (NOT ${STATUS})
      if (NOT ${${DEPENDENCY}_REQ_VERSION_CHECK} MATCHES "~")
         message(FATAL_ERROR "package is required")
      endif()
   endif()
endfunction()