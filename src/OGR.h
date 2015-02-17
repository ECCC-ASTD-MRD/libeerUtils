/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Lecture et traitements de fichiers raster
 * Fichier      : OGR.h
 * Creation     : Juillet 2004 - J.P. Gauthier
 *
 * Description  : Fonctions de manipulations et d'affichage de fichiers vectoriel.
 *
 * Remarques    :
 *
 * License      :
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation,
 *    version 2.1 of the License.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the
 *    Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *    Boston, MA 02111-1307, USA.
 *
 *=========================================================
 */

#ifndef _OGR_h
#define _OGR_h

#include "GeoRef.h"

#include "ogr_api.h"
#include "ogr_srs_api.h"
#include "gpc.h"

#define OGR_G_EnvelopeIntersect(ENV0,ENV1) (!(ENV0.MaxX<ENV1.MinX || ENV0.MinX>ENV1.MaxX || ENV0.MaxY<ENV1.MinY || ENV0.MinY>ENV1.MaxY))
#define OGR_PointInside(V,V0,V1)           (((V0[1]<=V[1] && V[1]<V1[1]) || (V1[1]<=V[1] && V[1]<V0[1])) && (V[0]<((V1[0]-V0[0])*(V[1]-V0[1])/(V1[1]-V0[1])+V0[0])))

#define OGR_GEOMTYPES "Point,3D Point,Line String,3D Line String,Polygon,3D Polygon,Multi Point,3D Multi Point,Multi Line String,3D Multi Line String,Multi Polygon,3D Multi Polygon,Geometry Collection,3D Geometry Collection,Linear Ring"
#define OGR_BUFFER    4096

typedef struct OGR_Sort {
   int           Field,Type,Order;       // Sorting parameters
   unsigned int  Nb,*Table;              // Sorted table of featurre index
} OGR_Sort;

typedef struct OGR_File {
   OGRDataSourceH  Data;                 // OGR internal file datasource object
   OGRSFDriverH    Driver;               // OGR driver used for this file
   char           *Id;                   // File identifier
   char           *Name;                 // File path
   char            Mode;                 // File access mode
} OGR_File;

typedef struct OGR_Layer {
#ifdef _TK_SOURCE
   Tcl_Obj         *Tag;                 // (Tcl_Obj type)   Tcl identifier
   TDataSpec       *Spec;                // (TDataSpec type) Specification de rendue des donnees
#else
   char            *Tag;                 // (Tcl_Obj type)   Tcl identifier
   char            *Spec;                // (TDataSpec type) Specification de rendue des donnees
#endif

   TGeoRef         *Ref;                 // GeoReference 
   OGRLayerH        Layer;               // OGR internal layer object
   OGRFeatureH     *Feature;             // List of OGR internal layer featuret
   OGRFeatureDefnH  Def;                 // OGR internal feature definition object
   OGRDataSourceH   Data;                // OGR internal layer datasource object

   OGR_File        *File;                // Layer's file provenance
   OGR_Sort         Sort;                // Sorting parameters
   char            *Select;              // List of features selection flag
   char             Changed;             // Is the layer changed
   int              Update;              // Do we need to update the internale OGR
   int              Mask,FMask;          // Is this layer used as a mask
   unsigned int     NFeature;            // Number of features in the layer
   unsigned int     LFeature;            // List of feature's rendered display list
   unsigned int     GFeature;            // Number of feature processes as display list
   unsigned int    *SFeature;            // List of of highlighted features
   unsigned int     NSFeature;           // Number of highligted features
   int              CFeature;            // Cleared feature (to be re-rendered)
   int              Topo,Extrude,Space;  // Positional parameters
   double           Min,Max;             // Layer's min-max of the currently mapped field
   Vect3d           Vr[2];               // Layer extent in projected coordinates
   Coord           *Loc;                 // List of feature's centroid
} OGR_Layer;

#define GPC_ARRAY0   0
#define GPC_ARRAY1   1
#define GPC_ARRAYPTR 2

Vect3d*      GPC_GetVect3d(unsigned int Size,unsigned int No);
void         GPC_ClearVect3d(void);
void         GPC_OGRProject(OGRGeometryH Geom,TGeoRef *FromRef,TGeoRef *ToRef);
void         GPC_FromOGR(gpc_polygon* Poly,OGRGeometryH *Geom);
void         GPC_ToOGR(gpc_polygon *Poly,OGRGeometryH *Geom);
OGRGeometryH GPC_OnOGR(gpc_op Op,OGRGeometryH Geom0,OGRGeometryH Geom1);
OGRGeometryH GPC_OnOGRLayer(gpc_op Op,OGR_Layer *Layer);
void         GPC_New(gpc_polygon *Poly);
int          GPC_QSortInter(const Vect3d *A,const Vect3d *B);
int          GPC_Within(OGRGeometryH Geom0,OGRGeometryH Geom1,OGREnvelope *Env0,OGREnvelope *Env1);
int          GPC_Intersect(OGRGeometryH Geom0,OGRGeometryH Geom1,OGREnvelope *Env0,OGREnvelope *Env1);
int          GPC_PointPointIntersect(OGRGeometryH Geom0,OGRGeometryH Geom1,int All);
int          GPC_PointLineIntersect(OGRGeometryH Geom0,OGRGeometryH Geom1,int All);
int          GPC_PointPolyIntersect(OGRGeometryH Geom0,OGRGeometryH Geom1,int All);
int          GPC_PolyPolyIntersect(OGRGeometryH Geom0,OGRGeometryH Geom1);
int          GPC_LinePolyIntersect(OGRGeometryH Geom0,OGRGeometryH Geom1);
int          GPC_SegmentIntersect(Vect3d PointA,Vect3d PointB,Vect3d PointC,Vect3d PointD,Vect3d Inter);
double       GPC_Length(OGRGeometryH Geom);
double       GPC_SegmentLength(OGRGeometryH Geom);
double       GPC_SegmentDist(Vect3d SegA,Vect3d SegB,Vect3d Point);
double       GPC_PointClosest(OGRGeometryH Geom,OGRGeometryH Pick,Vect3d Vr);
int          GPC_PointInside(OGRGeometryH Geom,OGRGeometryH Pick,Vect3d Vr);
double       GPC_CoordLimit(OGRGeometryH Geom,int Coord,int Mode);
OGRGeometryH GPC_Clip(OGRGeometryH Line,OGRGeometryH Poly);
int          GPC_ClipSegment(OGRGeometryH Line,OGRGeometryH Poly,OGRGeometryH Clip);
double       GPC_Centroid2D(OGRGeometryH Geom,double *X,double *Y);
double       GPC_Centroid2DProcess(OGRGeometryH Geom,double *X,double *Y);
int          GPC_Simplify(double Tolerance,OGRGeometryH Geom);
int          GPC_SimplifyDP(double Tolerance,Vect3d *Pt,int J,int K,int *Markers);

#endif