message(STATUS "Generating libeerUtils binaries")

#add_link_options(-Wl,-rpath=$(INSTALL_DIR)/lib)
add_link_options(-lpthread)

IF(CMAKE_CXX_COMPILER_ID MATCHES "Intel" )
   add_link_options(-Wl,-rpath $ENV{INTELCOMP_HOME}/lib/intel64_lin) 
   add_link_options(-lintlc -lifcore -lifport)
endif()

add_executable(CodeInfo CodeInfo.c)
add_executable(Dict Dict.c)
add_executable(EZTiler EZTiler.c)
add_executable(ReGrid ReGrid.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

#add_dependencies(CodeInfo eerUtils)
target_link_libraries(CodeInfo eerUtils ${RMN_LIBRARIES} ${VGRID_LIBRARIES})
set_target_properties(CodeInfo PROPERTIES VERSION ${PROJECT_VERSION})

#add_dependencies(Dict eerUtils)
target_link_libraries(Dict eerUtils ${RMN_LIBRARIES} ${VGRID_LIBRARIES})
set_target_properties(Dict PROPERTIES VERSION ${PROJECT_VERSION})

#add_dependencies(EZTiler eerUtils)
target_link_libraries(EZTiler eerUtils ${RMN_LIBRARIES} ${VGRID_LIBRARIES})
set_target_properties(EZTiler PROPERTIES VERSION ${PROJECT_VERSION})

#add_dependencies(ReGrid eerUtils)
target_link_libraries(ReGrid eerUtils ${RMN_LIBRARIES} ${VGRID_LIBRARIES})
set_target_properties(ReGrid PROPERTIES VERSION ${PROJECT_VERSION})

install(TARGETS CodeInfo Dict EZTiler ReGrid 
   RUNTIME DESTINATION bin)