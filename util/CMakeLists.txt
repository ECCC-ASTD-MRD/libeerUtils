message(STATUS "Generating libeerUtils binaries")

#add_link_options(-Wl,-rpath=${CMAKE_INSTALL_PREFIX}/lib)
add_link_options(-lpthread)

IF(DEFINED ENV{INTELCOMP_HOME})
#CMAKE_CXX_COMPILER_ID MATCHES "Intel" )
   add_link_options(-Wl,-rpath $ENV{INTELCOMP_HOME}/lib/intel64_lin) 
   add_link_options(-lintlc -lifcore -lifport)
endif()

add_executable(CodeInfo CodeInfo.c)
add_executable(Dict Dict.c)
add_executable(EZTiler EZTiler.c)
add_executable(ReGrid ReGrid.c)

set(targets CodeInfo Dict EZTiler ReGrid)

#----- Set common target properties
set_target_properties(${targets} PROPERTIES
   VERSION                       ${PROJECT_VERSION}
   INSTALL_RPATH_USE_LINK_PATH   TRUE
   BUILD_RPATH_USE_ORIGIN        TRUE
)

#----- Set common link libraries
foreach(target IN LISTS targets)
   target_link_libraries(${target} eerUtils -lm)
endforeach()

install(TARGETS ${targets}
   RUNTIME DESTINATION bin)
