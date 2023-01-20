message(STATUS "Generating libeerUtils librairie")

file(GLOB PROJECT_INCLUDE_FILES *.h)
file(GLOB PROJECT_C_FILES *.c)

if(DEFINED OMPI) 
   set(target eerUtils-ompi)
else()
   set(target eerUtils)
endif()

#----- target dependencies
add_library(${target} STATIC ${PROJECT_INCLUDE_FILES} ${PROJECT_C_FILES})

#----- Common target properties
set_target_properties(${target} PROPERTIES
   VERSION                    ${PROJECT_VERSION}
   PUBLIC_HEADER              "${PROJECT_INCLUDE_FILES}"
   POSITION_INDEPENDENT_CODE  ON
)   

add_dependencies(${target} ${PROJECT_NAME}_build_info)
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${target} PUBLIC LibXml2::LibXml2)
if(rmn_FOUND)
   target_link_libraries(${target} PUBLIC rmn)
endif()
if(vgrid_FOUND)
   target_link_libraries(${target} PUBLIC vgrid)
endif()
if(GDAL_FOUND)
   target_link_libraries(${target} PUBLIC GDAL::GDAL)
endif()
if(${CMAKE_Fortran_COMPILER} STREQUAL "gfortran")
   target_link_libraries(${target} PUBLIC -lgfortran)
endif()

#----- Specific target dependencies
if(DEFINED OMPI)
   target_compile_definitions(eerUtils-ompi PUBLIC HAVE_MPI)
   target_link_libraries(eerUtils-ompi PUBLIC MPI::MPI_C OpenMP::OpenMP_C)
endif()

#----- Install instructions
install(TARGETS ${target})
