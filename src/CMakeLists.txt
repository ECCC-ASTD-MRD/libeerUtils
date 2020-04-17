message(STATUS "Generating libeerUtils librairie")

file(GLOB PROJECT_INCLUDE_FILES *.h)
file(GLOB PROJECT_SOURCE_FILES *.c)

add_library(eerUtils$ENV{OMPI} STATIC ${PROJECT_INCLUDE_FILES} ${PROJECT_SOURCE_FILES})

set_target_properties(eerUtils$ENV{OMPI} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(eerUtils$ENV{OMPI} PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
set_target_properties(eerUtils$ENV{OMPI} PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
set_target_properties(eerUtils$ENV{OMPI} PROPERTIES PUBLIC_HEADER "${PROJECT_INCLUDE_FILES}")

add_dependencies(eerUtils$ENV{OMPI} timestamp)

target_link_libraries(eerUtils$ENV{OMPI} ${RMN_LIBRARIES} ${VGRID_LIBRARIES})
target_link_libraries(eerUtils$ENV{OMPI} ${LIBXML2_LIBRARIES})
if(GDAL_FOUND)
   target_link_libraries(eerUtils$ENV{OMPI} ${GDAL_LIBRARIES})
endif()

install(TARGETS eerUtils$ENV{OMPI} 
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
   PUBLIC_HEADER DESTINATION include)
