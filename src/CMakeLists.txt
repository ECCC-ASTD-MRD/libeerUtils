message(STATUS "Generating libeerUtils librairie")

set(PROJECT_INCLUDE_FILES 
   Array.h
   BinaryFile.h
   BitStuff.h
   Def.h
   Dict.h
   DynArray.h
   eerUtils.h
   EZGrid.h
   FPCompressD.h
   FPCompressF.h
   FPFC.h
   gdal_safe.h
   GeoRef.h
   gpc_ext.h
   gpc.h
   List.h
   Lookup.h
   Matrix.h
   OGR.h
   ogr_stub.h
   OMP_Utils.h
   QSM.h
   QTree.h
   RPNC.h
   RPN.h
   SM.h
   tclUtils.h
   TimeRef.h
   Traj.h
   Triangle.h
   Vector.h
   Vertex.h
   ZRef.h
   ZRefInterp.h)

 set(PROJECT_C_FILES
   Array.c
   Astro.c
   BinaryFile.c
   Def.c
   Dict.c
   DynArray.c
   eerUtils.c
   EZGrid.c
   FPCompressD.c
   FPCompressF.c
   FPFC.c
   GeoRef.c
   GeoRef_RDR.c
   GeoRef_RPN.c
   GeoRef_WKT.c
   gpc.c
   gpc_ext.c
   List.c
   Lookup.c
   Matrix.c
   OGR.c
   QSM.c
   QTree.c
   RPN.c
   SM.c
   tclUtils.c
   Triangle.c
   Vector.c
   Vertex.c
   ZRef.c
   ZRefInterp.c)

if(WITH_OMPI AND OpenMP_FOUND AND MPI_FOUND)
   set(targets eerUtils eerUtils-ompi)
else()
   set(targets eerUtils)
endif()

foreach(target IN ITEMS ${targets})
   string(REGEX MATCH "-ompi\$" pfix "${target}")
   add_library(${target} STATIC ${PROJECT_INCLUDE_FILES} ${PROJECT_C_FILES})

   set_target_properties(${target} PROPERTIES
      VERSION                       ${PROJECT_VERSION}
      PUBLIC_HEADER                 "${PROJECT_INCLUDE_FILES}"
      POSITION_INDEPENDENT_CODE     ON
   )
   target_compile_definitions(${target} INTERFACE HAVE_EER=${PROJECT_VERSION} HAVE_GPC=TRUE)
   add_library(eerUtils::${target} ALIAS ${target})

   add_dependencies(${target} ${PROJECT_NAME}_build_info)
   target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
   target_link_libraries(${target} PUBLIC LibXml2::LibXml2)
   if(rmn_FOUND)
      target_link_libraries(${target} PUBLIC rmn::rmn${pfix})
   endif()
   if(vgrid_FOUND)
      target_link_libraries(${target} PUBLIC vgrid::vgrid)
   endif()
   if(GDAL_FOUND)
      target_link_libraries(${target} PUBLIC GDAL::GDAL)
   endif()
   if(${CMAKE_Fortran_COMPILER} STREQUAL "gfortran")
      target_link_libraries(${target} PUBLIC -lgfortran)
   endif()
endforeach()

if(WITH_OMPI AND OpenMP_FOUND AND MPI_FOUND)
   target_compile_definitions(eerUtils-ompi PUBLIC _MPI HAVE_MPI HAVE_OPENMP)
   target_link_libraries(eerUtils-ompi PUBLIC MPI::MPI_C OpenMP::OpenMP_C)
   target_link_libraries(eerUtils-ompi PUBLIC -lrt)
endif()

#----- Install instructions
install(TARGETS ${targets}
   EXPORT eerUtils-static-targets
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
   PUBLIC_HEADER DESTINATION include
   INCLUDES DESTINATION include)
