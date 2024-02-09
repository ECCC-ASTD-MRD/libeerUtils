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

set(targets eerUtils)
add_library(eerUtils STATIC ${PROJECT_INCLUDE_FILES} ${PROJECT_C_FILES})
set_target_properties(eerUtils PROPERTIES
   VERSION                       ${PROJECT_VERSION}
   PUBLIC_HEADER                 "${PROJECT_INCLUDE_FILES}"
   POSITION_INDEPENDENT_CODE     ON
)   
target_compile_definitions(eerUtils INTERFACE HAVE_EER=${PROJECT_VERSION} HAVE_GPC=TRUE)
add_library(eerUtils::eerUtils ALIAS eerUtils)

add_dependencies(eerUtils ${PROJECT_NAME}_build_info)
target_include_directories(eerUtils PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(eerUtils PUBLIC LibXml2::LibXml2)
if(rmn_FOUND)
   target_link_libraries(eerUtils PUBLIC rmn::rmn)
endif()
if(vgrid_FOUND)
   target_link_libraries(eerUtils PUBLIC vgrid::vgrid)
endif()
if(GDAL_FOUND)
   target_link_libraries(eerUtils PUBLIC GDAL::GDAL)
endif()
if(${CMAKE_Fortran_COMPILER} STREQUAL "gfortran")
   target_link_libraries(eerUtils PUBLIC -lgfortran)
endif()

if(WITH_OMPI AND OpenMP_FOUND AND MPI_FOUND) 
   set(targets eerUtils eerUtils-ompi)
   add_library(eerUtils-ompi STATIC ${PROJECT_INCLUDE_FILES} ${PROJECT_C_FILES})
   set_target_properties(eerUtils-ompi PROPERTIES
      VERSION                       ${PROJECT_VERSION}
      PUBLIC_HEADER                 "${PROJECT_INCLUDE_FILES}"
      POSITION_INDEPENDENT_CODE     ON
   )   
   add_library(eerUtils::eerUtils-ompi ALIAS eerUtils-ompi)
   
   add_dependencies(eerUtils-ompi ${PROJECT_NAME}_build_info)
   target_include_directories(eerUtils-ompi PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
   target_compile_definitions(eerUtils-ompi PUBLIC HAVE_MPI)
   target_compile_definitions(eerUtils-ompi INTERFACE HAVE_EER=${PROJECT_VERSION} HAVE_GPC=TRUE)
   target_link_libraries(eerUtils-ompi PUBLIC MPI::MPI_C OpenMP::OpenMP_C)
   target_link_libraries(eerUtils-ompi PUBLIC LibXml2::LibXml2)
   if(rmn_FOUND)
      target_link_libraries(eerUtils-ompi PUBLIC rmn::rmn-ompi)
   endif()
   if(vgrid_FOUND)
      target_link_libraries(eerUtils-ompi PUBLIC vgrid::vgrid)
   endif()
   if(GDAL_FOUND)
      target_link_libraries(eerUtils-ompi PUBLIC GDAL::GDAL)
   endif()
   if(${CMAKE_Fortran_COMPILER} STREQUAL "gfortran")
      target_link_libraries(eerUtils-ompi PUBLIC -lgfortran)
   endif()
   target_link_libraries(eerUtils-ompi PUBLIC -lrt)
endif()

#----- Install instructions
install(TARGETS ${targets}
   EXPORT eerUtils-static-targets
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
   PUBLIC_HEADER DESTINATION include
   INCLUDES DESTINATION include)
