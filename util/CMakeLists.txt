message(STATUS "Generating libeerUtils binaries")

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
   if(COMPILER_SUITE MATCHES intel)
#      target_link_options(${target} PRIVATE "LINKER:-rpath,$ENV{INTELCOMP_HOME}/lib/intel64_lin")
      target_link_libraries(${target} -lintlc -lifcore -lifport -limf -lsvml -liomp5 -lifcoremt)
   endif()
endforeach()

install(TARGETS ${targets}
   RUNTIME DESTINATION bin)

ec_install_symlink(./Dict ${CMAKE_INSTALL_PREFIX}/bin/o.dict)
