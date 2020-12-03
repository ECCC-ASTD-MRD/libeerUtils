message(STATUS "Generating libeerUtils librairie")

file(GLOB PROJECT_INCLUDE_FILES *.h)
file(GLOB PROJECT_C_FILES *.c)

add_library(eerUtils STATIC ${PROJECT_INCLUDE_FILES} ${PROJECT_C_FILES})
add_library(eerUtils-ompi STATIC ${PROJECT_INCLUDE_FILES} ${PROJECT_C_FILES})

#----- Common target properties
set_target_properties(eerUtils eerUtils-ompi PROPERTIES
   VERSION                    ${PROJECT_VERSION}
   PUBLIC_HEADER              "${PROJECT_INCLUDE_FILES}"
   POSITION_INDEPENDENT_CODE  ON
)

#----- Common target dependencies
foreach(target IN ITEMS eerUtils eerUtils-ompi)
   message(WARNING "Adding dep to ${target} (${CMAKE_CURRENT_SOURCE_DIR})")
   add_dependencies(${target} build_info)
   target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
   target_link_libraries(${target} PUBLIC LibXml2::LibXml2)
   if(RMN_FOUND)
      target_link_libraries(${target} PUBLIC RMN::RMN)
   endif()
   if(VGRID_FOUND)
      target_link_libraries(${target} PUBLIC VGRID::VGRID)
   endif()
   if(GDAL_FOUND)
      target_link_libraries(${target} PUBLIC GDAL::GDAL)
   endif()
endforeach()

#----- Specific target dependencies
target_compile_definitions(eerUtils-ompi PUBLIC _MPI)
target_link_libraries(eerUtils-ompi PUBLIC MPI::MPI_C OpenMP::OpenMP_C)

install(TARGETS eerUtils eerUtils-ompi
   LIBRARY        DESTINATION lib
   ARCHIVE        DESTINATION lib
   PUBLIC_HEADER  DESTINATION include)
