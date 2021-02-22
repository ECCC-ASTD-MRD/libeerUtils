/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Lecture et traitements de fichiers raster
 * Fichier      : OGR.c
 * Creation     : Juillet 2005 - J.P. Gauthier
 *
 * Description  : Fonctions de calculs sur les objets geometriques.
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
 *
 *=========================================================
 */

#include "App.h"
#include "OGR.h"
#include "DynArray.h"

static  __thread Vect3d*  OGM_Geom[2];
static  __thread Vect3d** OGM_Ptr;
static  __thread unsigned int OGM_GeomNb=0;

#ifdef HAVE_GDAL

void OGM_ClearVect3d(void) {

   if (OGM_GeomNb>OGR_BUFFER) {
      if (OGM_Geom[0]) free(OGM_Geom[0]); OGM_Geom[0]=OGM_Geom[1]=NULL;
      if (OGM_Ptr)  free(OGM_Ptr); OGM_Ptr=NULL;
      OGM_GeomNb=0;
   }
}

Vect3d* OGM_GetVect3d(unsigned int Size,unsigned int No) {

   if (Size>OGM_GeomNb) {
      OGM_GeomNb=Size<OGR_BUFFER?OGR_BUFFER:Size;
      OGM_Geom[0]=(Vect3d*)realloc(OGM_Geom[0],OGM_GeomNb*2*sizeof(Vect3d));
      OGM_Geom[1]=&OGM_Geom[0][OGM_GeomNb];
      OGM_Ptr=(Vect3d**)realloc(OGM_Ptr,OGM_GeomNb*sizeof(Vect3d*));

      if (!OGM_Geom[0] || !OGM_Geom[1] || !OGM_Ptr) {
         App_Log(ERROR,"%s: Could not allocate GPC buffers\n",__func__);
         return(NULL);
      }
#ifdef DEBUG
         App_Log(DEBUG,"%s: Increasing size to %i\n",__func__,OGM_GeomNb);
#endif
   }
   return(No==2?(Vect3d*)OGM_Ptr:OGM_Geom[No]);
}

static inline int OGM_ToVect3d(OGRGeometryH Geom,unsigned int No) {

   unsigned int n;

   n=OGR_G_GetPointCount(Geom);
   if (OGM_GetVect3d(n,No)) {
      OGR_G_GetPoints(Geom,&OGM_Geom[No][0][0],sizeof(Vect3d),&OGM_Geom[No][0][1],sizeof(Vect3d),&OGM_Geom[No][0][2],sizeof(Vect3d));
      return(n);
   }
   return(0);
}

int OGM_QSortInter(const void *A,const void *B){

   if ((*(const Vect3d*)A)[2]<(*(const Vect3d*)B)[2]) {
      return(-1);
   } else if ((*(const Vect3d*)A)[2]>(*(const Vect3d*)B)[2]) {
      return(1);
   } else {
      return(0);
   }
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <OGM_OGRProject>
 * Creation     : Novembre 2005 J.P. Gauthier - CMC/CMOE
 *
 * But          : Transforme les coordonnees d'un object vectoriel OGR dans une autre referentiel
 *
 * Parametres   :
 *   <Geom>     : Object geometrique OGR
 *   <FromRef>  : Reference source
 *   <ToRef>    : Reference destination
 *
 * Retour       : Code d'erreur standard TCL
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
void OGM_OGRProject(OGRGeometryH Geom,TGeoRef *FromRef,TGeoRef *ToRef) {

   OGRGeometryH geom;
   Vect3d       vr;
   Coord        co;
   int          n;

   if (FromRef!=ToRef) {
      for(n=0;n<OGR_G_GetGeometryCount(Geom);n++) {
         geom=OGR_G_GetGeometryRef(Geom,n);
         OGM_OGRProject(geom,FromRef,ToRef);
      }

      for(n=0;n<OGR_G_GetPointCount(Geom);n++) {
         OGR_G_GetPoint(Geom,n,&vr[0],&vr[1],&vr[2]);
         FromRef->Project(FromRef,vr[0],vr[1],&co.Lat,&co.Lon,1,1);
         ToRef->UnProject(ToRef,&vr[0],&vr[1],co.Lat,co.Lon,1,1);
         OGR_G_SetPoint(Geom,n,vr[0],vr[1],vr[2]);
      }
   }
}

OGRGeometryH OGM_Clip(OGRGeometryH Line,OGRGeometryH Poly) {

   OGRGeometryH clip=NULL;

   if (!(OGR_G_GetGeometryCount(Poly))) {
      return(NULL);
   }

   clip=OGR_G_CreateGeometry(wkbMultiLineString);
   OGM_ClipSegment(Line,Poly,clip);

   return(clip);
}

int OGM_ClipSegment(OGRGeometryH Line,OGRGeometryH Poly,OGRGeometryH Clip) {

   OGRGeometryH line,point,ring;
   Vect3d       pt0,pt1,ppt0,ppt1,inter[16];
   int          n,np,nr,nb,nbinter,in=0;

   for (n=0;n<OGR_G_GetGeometryCount(Line);n++) {
      line=OGR_G_GetGeometryRef(Line,n);
      OGM_ClipSegment(line,Poly,Clip);
   }

   line=NULL;
   ring=OGR_G_GetGeometryRef(Poly,0);
   point=OGR_G_CreateGeometry(wkbPoint);
   OGR_G_AddPoint_2D(point,0.0,0.0);

   if ((nb=OGR_G_GetPointCount(Line))) {
      OGR_G_GetPoint(Line,0,&pt0[0],&pt0[1],&pt0[2]);
      OGR_G_SetPoint_2D(point,0,pt0[0],pt0[1]);
      in=OGM_PointPolyIntersect(point,ring,0);
      /*Add the current point if inside*/
      if (in) {
         if (!line) line=OGR_G_CreateGeometry(wkbLineString);
         OGR_G_AddPoint_2D(line,pt0[0],pt0[1]);
      }

      /*Loop on the line segments*/
      for (n=1;n<OGR_G_GetPointCount(Line);n++) {
         OGR_G_GetPoint(Line,n,&pt1[0],&pt1[1],&pt1[2]);
         nbinter=0;

         /*Loop on the polygon rings*/
         for (nr=0;nr<OGR_G_GetGeometryCount(Poly);nr++) {
            ring=OGR_G_GetGeometryRef(Poly,nr);

            /*Loop on the ring segments*/
            if (!(nb=OGR_G_GetPointCount(ring)))
               continue;

            OGR_G_GetPoint(ring,0,&ppt0[0],&ppt0[1],&ppt0[2]);
            for (np=1;np<nb;np++) {
               OGR_G_GetPoint(ring,np,&ppt1[0],&ppt1[1],&ppt1[2]);

               /*If intersect, add point*/
               if ((OGM_SegmentIntersect(pt0,pt1,ppt0,ppt1,inter[nbinter]))==1) {
                  nbinter++;
               }
               Vect_Assign(ppt0,ppt1);
            }
         }

         /*Order the intersections*/
         if (nbinter>1)
            qsort(inter,nbinter,sizeof(Vect3d),OGM_QSortInter);

         /*Add intersections to the linestring*/
         for (nr=0;nr<nbinter;nr++) {
            if (!line) line=OGR_G_CreateGeometry(wkbLineString);
            OGR_G_AddPoint_2D(line,inter[nr][0],inter[nr][1]);
            /*Flip inside/outside flag*/
            in=!in;
            if (!in && line) {
               OGR_G_AddGeometry(Clip,line);
               line=NULL;
            }
         }

         /*If still in, add the point*/
         if (in) {
            OGR_G_SetPoint_2D(point,0,pt1[0],pt1[1]);
            if (OGM_PointPolyIntersect(point,ring,0)) {
               OGR_G_AddPoint_2D(line,pt1[0],pt1[1]);
            }
         }
         Vect_Assign(pt0,pt1);
      }
      if (line) {
         if (OGR_G_GetPointCount(line)>1) {
            OGR_G_AddGeometry(Clip,line);
         }
      }
   }
   OGR_G_DestroyGeometry(point);
   return(1);
}

void OGM_GPCFromOGR(gpc_polygon *Poly,OGRGeometryH *Geom) {

#ifdef HAVE_GPC
   OGRGeometryH       geom;
   OGRwkbGeometryType type;
   gpc_vertex_list   *gpc;
   unsigned int       nb,g,nc;

   type=wkbFlatten(OGR_G_GetGeometryType(Geom));

   if (type==wkbMultiPolygon || type==wkbGeometryCollection) {
      for (g=0;g<OGR_G_GetGeometryCount(Geom);g++) {
         geom=OGR_G_GetGeometryRef(Geom,g);
         OGM_GPCFromOGR(Poly,geom);
      }
   } else if (type==wkbPolygon) {
      if ((nc=OGR_G_GetGeometryCount(Geom))) {
         Poly->hole=(int*)realloc(Poly->hole,(Poly->num_contours+nc)*(sizeof(int)));
         Poly->contour=(gpc_vertex_list*)realloc(Poly->contour,(Poly->num_contours+nc)*(sizeof(gpc_vertex_list)));

         for (g=0;g<nc;g++) {
            geom=OGR_G_GetGeometryRef(Geom,g);
            nb=OGR_G_GetPointCount(geom);
            Poly->hole[Poly->num_contours+g]=(g==0?FALSE:TRUE);
            gpc=&Poly->contour[Poly->num_contours+g];
            gpc->num_vertices=nb;
            gpc->vertex=(nb?(gpc_vertex*)malloc(nb*sizeof(gpc_vertex)):NULL);

            OGR_G_GetPoints(geom,&(gpc->vertex[0].x),sizeof(gpc_vertex),&(gpc->vertex[0].y),sizeof(gpc_vertex),NULL,0);
         }
         Poly->num_contours+=nc;
      }
   } else {
      App_Log(ERROR,"%s: Unsupported geometry type\n",__func__);
   }
#else
   App_Log(ERROR,"%s: Library not built with GPC\n",__func__);
#endif
}

void OGM_GPCToOGR(gpc_polygon *Poly,OGRGeometryH *Geom) {

#ifdef HAVE_GPC
   OGRGeometryH     geom,ring,poly=NULL,multi=NULL;
   gpc_vertex_list *gpc;
   unsigned int     n,g,nb=0,in;

   /*Check for multiple polygon (more thant 1 non-hole contour)*/
   for(n=0;n<Poly->num_contours;n++) {
      if (!Poly->hole[n]) {
         nb++;
      }
   }
   if (nb>1) {
      multi=OGR_G_CreateGeometry(wkbMultiPolygon);
   }

   /*Process external rings*/
   for(g=0;g<Poly->num_contours;g++) {
      if (!Poly->hole[g]) {
         gpc=&Poly->contour[g];
         ring=OGR_G_CreateGeometry(wkbLinearRing);
         //OGR_G_SetPoints(ring,gpc->num_vertices,&(gpc->vertex[0].x),sizeof(gpc_vertex),&(gpc->vertex[0].y),sizeof(gpc_vertex),NULL,0);
         for (n=0;n<gpc->num_vertices;n++) {
            OGR_G_AddPoint_2D(ring,gpc->vertex[n].x,gpc->vertex[n].y);
         }

         /*Make sure polygon is closed*/
         if (gpc->vertex[n-1].x!=gpc->vertex[0].x || gpc->vertex[n-1].y!=gpc->vertex[0].y) {
            OGR_G_AddPoint_2D(ring,gpc->vertex[0].x,gpc->vertex[0].y);
         }
         poly=OGR_G_CreateGeometry(wkbPolygon);
         OGR_G_AddGeometryDirectly(poly,ring);
         if (multi)
            OGR_G_AddGeometryDirectly(multi,poly);
      }
   }

   /*In case of no rings, create empty geometry*/
   if (!poly) {
      poly=OGR_G_CreateGeometry(wkbPolygon);
   } else {
      /*Process internal rings (holes)*/
      for(g=0;g<Poly->num_contours;g++) {
         if (Poly->hole[g]) {
            gpc=&Poly->contour[g];
            ring=OGR_G_CreateGeometry(wkbLinearRing);
            //OGR_G_SetPoints(ring,gpc->num_vertices,&(gpc->vertex[0].x),sizeof(gpc_vertex),&(gpc->vertex[0].y),sizeof(gpc_vertex),NULL,0);
            for (n=0;n<gpc->num_vertices;n++) {
               OGR_G_AddPoint_2D(ring,gpc->vertex[n].x,gpc->vertex[n].y);
            }

            /*Make sure polygon is closed*/
            if (gpc->vertex[n-1].x!=gpc->vertex[0].x || gpc->vertex[n-1].y!=gpc->vertex[0].y) {
               OGR_G_AddPoint_2D(ring,gpc->vertex[0].x,gpc->vertex[0].y);
            }
            if (multi) {
               in=0;

               /*Look for hole's parent*/
               for (n=0;n<OGR_G_GetGeometryCount(multi);n++) {
                  geom=OGR_G_GetGeometryRef(multi,n);
                  if (OGM_Intersect(geom,ring,NULL,NULL)) {
                     OGR_G_AddGeometryDirectly(geom,ring);
                     in=1;
                     break;
                  }
               }
               if (!in) {
                  App_Log(ERROR,"%s: Found a hole without parent\n",__func__);
               }
            } else {
               OGR_G_AddGeometryDirectly(poly,ring);
            }
         }
      }
   }
   *Geom=multi?multi:poly;
#else
      App_Log(ERROR,"%s: Library not built with GPC\n",__func__);
#endif
}

void OGM_GPCNew(gpc_polygon *Poly) {

#ifdef HAVE_GPC
   Poly->num_contours=0;
   Poly->hole=NULL;
   Poly->contour=NULL;
#else
   App_Log(ERROR,"%s: Library not built with GPC\n",__func__);
#endif
}

OGRGeometryH OGM_GPCOnOGR(gpc_op Op,OGRGeometryH Geom0,OGRGeometryH Geom1) {

#ifdef HAVE_GPC
   gpc_polygon  poly0,poly1,poly;
   OGRGeometryH geom=NULL;

   OGM_GPCNew(&poly0);
   OGM_GPCNew(&poly1);

   OGM_GPCFromOGR(&poly0,Geom0);
   OGM_GPCFromOGR(&poly1,Geom1);

   gpc_polygon_clip(Op,&poly0,&poly1,&poly);

   OGM_GPCToOGR(&poly,&geom);

   gpc_free_polygon(&poly);
   gpc_free_polygon(&poly0);
   gpc_free_polygon(&poly1);

   return(geom);
#else
   App_Log(ERROR,"%s: Library not built with GPC\n",__func__);
#endif
}

OGRGeometryH OGM_GPCOnOGRLayer(gpc_op Op,OGR_Layer *Layer) {

#ifdef HAVE_GPC
   gpc_polygon  poly0,poly1,result,*r,*p,*t;
   OGRGeometryH geom=NULL;
   unsigned int f;

   OGM_GPCNew(&poly0);
   OGM_GPCNew(&poly1);
   OGM_GPCNew(&result);

   p=&poly0;
   r=&result;
   t=NULL;

   for(f=0;f<Layer->NFeature;f++) {
      if (Layer->Select[f] && Layer->Feature[f]) {
         if ((geom=OGR_F_GetGeometryRef(Layer->Feature[f]))) {

            OGM_GPCFromOGR((t?&poly1:&result),geom);
            if (t) {
               gpc_polygon_clip(Op,p,&poly1,r);
               gpc_free_polygon(p);
            }
            t=p; p=r; r=t;

            gpc_free_polygon(&poly1);
         }
      }
   }

   OGM_GPCToOGR(p,&geom);

   gpc_free_polygon(&result);
   gpc_free_polygon(&poly0);
   gpc_free_polygon(&poly1);

   return(geom);
#else
   App_Log(ERROR,"%s: Library not built with GPC\n",__func__);
#endif
}

OGRGeometryH OGM_GPCOnOGRGeometry(gpc_op Op,OGRGeometryH *Geom) {

#ifdef HAVE_GPC
   gpc_polygon  poly0,poly1,result,*r,*p,*t;
   OGRGeometryH geom=NULL;
   unsigned int g;

   OGM_GPCNew(&poly0);
   OGM_GPCNew(&poly1);
   OGM_GPCNew(&result);

   p=&poly0;
   r=&result;
   t=NULL;
 
   for(g=0;g<OGR_G_GetGeometryCount(Geom);g++) {
      if ((geom=OGR_G_GetGeometryRef(Geom,g))) {
         OGM_GPCFromOGR((t?&poly1:&result),geom);
         if (t) {
            gpc_polygon_clip(Op,p,&poly1,r);
            gpc_free_polygon(p);
         }
         t=p; p=r; r=t;

         gpc_free_polygon(&poly1);
      }
   }

   OGM_GPCToOGR(p,&geom);

   gpc_free_polygon(&result);
   gpc_free_polygon(&poly0);
   gpc_free_polygon(&poly1);

   return(geom);
#else
   App_Log(ERROR,"%s: Library not built with GPC\n",__func__);
#endif
}

int OGM_Within(OGRGeometryH Geom0,OGRGeometryH Geom1,OGREnvelope *Env0,OGREnvelope *Env1) {

   int          n0,n1,npt=0;
   OGRGeometryH pt,geom;
   OGREnvelope  env;

   /*Check if enveloppe overlap*/
   if (Env0 && Env1) {
      if (!OGR_G_EnvelopeIntersect((*Env0),(*Env1))) {
         return(0);
      }
   }

   /*It can't be within if it's not a within polygon*/
   if (OGR_G_GetGeometryType(Geom1)!=wkbPolygon && OGR_G_GetGeometryType(Geom1)!=wkbMultiPolygon && OGR_G_GetGeometryType(Geom1)!=wkbLinearRing) {
      return(0);
   }

   /*Boucle recursive sur les sous geometrie*/
   if (OGR_G_GetGeometryType(Geom0)!=wkbPolygon && (n0=OGR_G_GetGeometryCount(Geom0))) {
      for(n0=0;n0<OGR_G_GetGeometryCount(Geom0);n0++) {
         geom=OGR_G_GetGeometryRef(Geom0,n0);
         OGR_G_GetEnvelope(geom,&env);
         if (!OGM_Within(geom,Geom1,&env,Env1)) {
            return(0);
         } else {
            npt++;
         }
      }
      return(npt==OGR_G_GetGeometryCount(Geom0));
   }

   if (OGR_G_GetGeometryType(Geom1)!=wkbPolygon && (n1=OGR_G_GetGeometryCount(Geom1))) {
     for(n1=0;n1<OGR_G_GetGeometryCount(Geom1);n1++) {
         geom=OGR_G_GetGeometryRef(Geom1,n1);
         OGR_G_GetEnvelope(geom,&env);
         if (OGM_Within(Geom0,geom,Env0,&env)) {
            return(1);
         }
      }
   }

   /*Verifier l'inclusion dans les trous pour les polygones*/
   if (OGR_G_GetGeometryType(Geom1)==wkbPolygon && OGR_G_GetGeometryCount(Geom1)>1) {
      pt=OGR_G_CreateGeometry(wkbPoint);
      for(n0=1;n0<OGR_G_GetGeometryCount(Geom1);n0++) {
         npt=0;
         for(n1=0;n1<OGR_G_GetPointCount(Geom0);n1++) {
            OGR_G_SetPoint(pt,0,OGR_G_GetX(Geom0,n1),OGR_G_GetY(Geom0,n1),0);
            if (OGM_PointPolyIntersect(pt,OGR_G_GetGeometryRef(Geom1,n0),0)) {
               OGR_G_DestroyGeometry(pt);
               return(0);
            }
         }
      }
      OGR_G_DestroyGeometry(pt);
   }

   /*Pour les polygones tester seulement le contour externe*/
   if (OGR_G_GetGeometryType(Geom0)==wkbPolygon) {
      Geom0=OGR_G_GetGeometryRef(Geom0,0);
   }
   if (OGR_G_GetGeometryType(Geom1)==wkbPolygon) {
      Geom1=OGR_G_GetGeometryRef(Geom1,0);
   }

//   return(OGM_PointPolyIntersect(Geom0,Geom1,1));

   /*Demarrer les tests selon les type de geometrie*/
   if (OGM_LinePolyIntersect(Geom0,Geom1)) {
      return(0);
   } else {
      return(OGM_PointPolyIntersect(Geom0,Geom1,0));
   }
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <OGM_IntersectionPts>
 * Creation     : Mai 2018 - E. Legault-Ouellet - CMC/CMOE
 *
 * But          : Retourne les points d'intersection entre un segment et une géométrie
 *
 * Parametres   :
 *   <Geom>     : Géométrie OGR
 *   <X0>       : Coordonnée en X du premier point du segment
 *   <Y0>       : Coordonnée en Y du premier point du segment
 *   <X1>       : Coordonnée en X du second point du segment
 *   <Y1>       : Coordonnée en Y du second point du segment
 *
 * Retour       : Une géométrie OGR de type MultiPoint
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
static int OGM_QSortSegIntersectionPts(const void *A,const void *B){
   const double *a=A,*b=B,a2=a[0]*a[0]+a[1]*a[1],b2=b[0]*b[0]+b[1]*b[1];

   if( a2 < b2 )
      return -1;
   else if( a2 > b2 )
      return 1;
   else
      return 0;
}
static int OGM_SegIntersectionPts_(OGRGeometryH Geom,double X0,double Y0,double X1,double Y1,DynArray *restrict Pts) {
   int n,i,npt=0;

   // Loop on the points in the current geometry
   if( (n=OGR_G_GetPointCount(Geom)) > 1 ) {
      Vect3d *restrict  v;
      double            s12x,s13x,s34x,s12y,s13y,s34y,a,b,c;

      // These values won't change
      s12x = X1-X0;
      s12y = Y1-Y0;

      // Loop on the segments
      OGM_ToVect3d(Geom,0);
      v = OGM_Geom[0];
      for(i=1; i<n; ++i) {
         // Calculate the intersection point of the two segments
         s34x = v[i][0]-v[i-1][0];
         s34y = v[i][1]-v[i-1][1];
         c = s34x*s12y - s34y*s12x;
         // Note that if c==0.0, the lines are parallel, which means that we either have
         // no intersection or an infinity of it (if both lines overlap). The special case where
         // both segments are parallel and have points in common (overlap or share an end) is not
         // taken into account (it is not considered an intersection)
         if( c != 0.0 ) {
            // The lines intersects somewhere
            s13x = X0-v[i-1][0];
            s13y = v[i-1][1]-Y0;

            a = (s13x*s34y + s13y*s34x) / c;
            b = (s13x*s12y + s13y*s12x) / c;

            // Check if the intersection is inside both segments
            if( 0.0<=a && a<=1.0 && 0.0<=b && b<=1.0 ) {
               // Calculate and add the intersecting point
               DynArray_Pushd(Pts,X0+a*s12x);
               DynArray_Pushd(Pts,Y0+a*s12y);
               ++npt;
            }
         }
      }
   }

   // Recursive loop in all the sub-geometry
   n=OGR_G_GetGeometryCount(Geom);
   for(i=0; i<n; ++i) {
      npt += OGM_SegIntersectionPts_(OGR_G_GetGeometryRef(Geom,i),X0,Y0,X1,Y1,Pts);
   }

   return npt;
}
OGRGeometryH OGM_SegIntersectionPts(OGRGeometryH Geom,double X0,double Y0,double X1,double Y1) {
   OGRGeometryH   pts,pt;
   DynArray       da;
   double         *arr;
   int            n,i;

   DynArray_Init(&da,0);

   // Process the geometry recursively and fill the array with X,Y pairs of points
   n = OGM_SegIntersectionPts_(Geom,X0,Y0,X1,Y1,&da);

   // Sort the points if we have more than one
   if( n > 1 ) {
      // Make the position relative to the first point of the segment
      for(i=n,arr=da.Arr; i; --i) {
         *arr++ -= X0;
         *arr++ -= Y0;
      }
      // Sort the points in terms of distance from the first point of the segment
      qsort(da.Arr,n,2*sizeof(double),OGM_QSortSegIntersectionPts);
      // Make the position absolute again
      for(i=n,arr=da.Arr; i; --i) {
         *arr++ += X0;
         *arr++ += Y0;
      }
   }

   // Create points for each coords and add it to our set
   pts = OGR_G_CreateGeometry(wkbMultiPoint);
   for(arr=da.Arr; n; --n,arr+=2) {
      pt = OGR_G_CreateGeometry(wkbPoint);
      OGR_G_AddPoint_2D(pt,arr[0],arr[1]);
      OGR_G_AddGeometryDirectly(pts,pt);
   }

   DynArray_Free(&da);
   return pts;
}

int OGM_Intersect(OGRGeometryH Geom0,OGRGeometryH Geom1,OGREnvelope *Env0,OGREnvelope *Env1) {

   int          n0,n1,t0,t1,npt;
   Vect3d       v0,v1;
   OGRGeometryH pt,geom;
   OGREnvelope  env;

   /*Check if enveloppe overlap*/
   if (Env0 && Env1) {
      if (!OGR_G_EnvelopeIntersect((*Env0),(*Env1))) {
         return(0);
      }
   }

   /*Boucle recursive sur les sous geometrie*/
   if (OGR_G_GetGeometryType(Geom0)!=wkbPolygon && (n0=OGR_G_GetGeometryCount(Geom0))) {
      for(n0=0;n0<OGR_G_GetGeometryCount(Geom0);n0++) {
         geom=OGR_G_GetGeometryRef(Geom0,n0);
         OGR_G_GetEnvelope(geom,&env);
         if (OGM_Intersect(geom,Geom1,&env,Env1)) {
            return(1);
         }
      }
   }

   if (OGR_G_GetGeometryType(Geom1)!=wkbPolygon && (n1=OGR_G_GetGeometryCount(Geom1))) {
      for(n1=0;n1<OGR_G_GetGeometryCount(Geom1);n1++) {
         geom=OGR_G_GetGeometryRef(Geom1,n1);
         OGR_G_GetEnvelope(geom,&env);
         if (OGM_Intersect(Geom0,geom,Env0,&env)) {
            return(1);
         }
      }
   }

   /*Verifier l'inclusion dans les trous pour les polygones*/
   if (OGR_G_GetGeometryType(Geom0)==wkbPolygon && OGR_G_GetGeometryCount(Geom0)>1) {
      pt=OGR_G_CreateGeometry(wkbPoint);
      for(n0=1;n0<OGR_G_GetGeometryCount(Geom0);n0++) {
         npt=0;

         /*Verifier seulement pour le contour exterieur*/
         if (OGR_G_GetGeometryCount(Geom1)) {
            geom=OGR_G_GetGeometryRef(Geom1,0);
         } else {
            geom=Geom1;
         }
         for(n1=0;n1<OGR_G_GetPointCount(geom);n1++) {
            OGR_G_SetPoint(pt,0,OGR_G_GetX(geom,n1),OGR_G_GetY(geom,n1),0);
            npt+=OGM_PointPolyIntersect(pt,OGR_G_GetGeometryRef(Geom0,n0),0);
         }
         if (npt==OGR_G_GetPointCount(geom)) {
            return(0);
         }
      }
      OGR_G_DestroyGeometry(pt);
  }

   /*Recuperer la dimension des geometries*/
   t0=OGR_G_GetDimension(Geom0);
   t1=OGR_G_GetDimension(Geom1);

   /*Pour les polygones tester seulement le contour externe*/
   if (OGR_G_GetGeometryType(Geom0)==wkbPolygon) {
      Geom0=OGR_G_GetGeometryRef(Geom0,0);
   }
   if (OGR_G_GetGeometryType(Geom1)==wkbPolygon) {
      Geom1=OGR_G_GetGeometryRef(Geom1,0);
   }

   /*Forcer les lignes referme comme des polygones*/
   if ((n0=OGR_G_GetPointCount(Geom0))>2) {
      OGR_G_GetPoint(Geom0,0,&v0[0],&v0[1],&v0[2]);
      OGR_G_GetPoint(Geom0,n0-1,&v1[0],&v1[1],&v1[2]);
      if (Vect_Equal(v0,v1)) t0=2;
   }
   if ((n1=OGR_G_GetPointCount(Geom1))>2) {
      OGR_G_GetPoint(Geom1,0,&v0[0],&v0[1],&v0[2]);
      OGR_G_GetPoint(Geom1,n1-1,&v1[0],&v1[1],&v1[2]);
      if (Vect_Equal(v0,v1)) t1=2;
   }

   /*Demarrer les tests selon les type de geometrie*/
   if (n0 && n1) {
      if (t0==0) {
         if (t1==0) {
            return(OGM_PointPointIntersect(Geom0,Geom1,0));
         } else if (t1==1){
            return(OGM_PointLineIntersect(Geom0,Geom1,0));
         } else {
            return(OGM_PointPolyIntersect(Geom0,Geom1,0));
         }
      } else if (t0==1) {
         if (t1==0) {
            return(OGM_PointLineIntersect(Geom1,Geom0,0));
         } else if (t1==1) {
            return(OGM_LinePolyIntersect(Geom0,Geom1));
         } else {
            return(OGM_PolyPolyIntersect(Geom0,Geom1));
         }
      } else {
         if (t1==0) {
            return(OGM_PointPolyIntersect(Geom1,Geom0,0));
         } else if (t1==1) {
            return(OGM_LinePolyIntersect(Geom1,Geom0));
         } else {
            if (OGM_PolyPolyIntersect(Geom0,Geom1)) {
               return(1);
            }
            return(OGM_PointPolyIntersect(Geom1,Geom0,0));
         }
      }
   }
   return(0);
}

int OGM_PointPointIntersect(OGRGeometryH Geom0,OGRGeometryH Geom1,int All) {

   unsigned int n0,n1,g0,g1,t=0;

   g0=OGM_ToVect3d(Geom0,OGM_ARRAY0);
   g1=OGM_ToVect3d(Geom1,OGM_ARRAY1);

   for(n0=0;n0<g0;n0++) {
      for(n1=0;n1<g1;n1++) {
         if (Vect_Equal(OGM_Geom[0][n0],OGM_Geom[1][n1])) {
            t++;
            if (!All)
               return(1);
         }
      }
   }
   return(All?t==g0:t);
}

int OGM_PointLineIntersect(OGRGeometryH Geom0,OGRGeometryH Geom1,int All) {

   unsigned int n0,n1,g0,g1,t=0,i;
   Vect3d       v0,v1[2];

   g0=OGM_ToVect3d(Geom0,OGM_ARRAY0);
   g1=OGM_ToVect3d(Geom1,OGM_ARRAY1);

   for(n0=0;n0<g0;n0++) {
      Vect_Assign(v0,OGM_Geom[0][n0]);

      for(n1=0;n1<g1-1;n1++) {
         Vect_Assign(v1[0],OGM_Geom[1][n1]);
         Vect_Assign(v1[1],OGM_Geom[1][n1+1]);

         i=OGM_SegmentIntersect(v0,v0,v1[0],v1[1],NULL);
         if (i==1 || i==4 || i==5) {
            t++;
            if (!All)
               return(1);
         }
      }
   }
   return(All?t==g0:t);
}

int OGM_PointPolyIntersect(OGRGeometryH Geom0,OGRGeometryH Geom1,int All) {

   unsigned int n0,n1,g0,g1,n11,t=0;
   int          c=0;
   Vect3d       v0,v1[2];

   g0=OGM_ToVect3d(Geom0,OGM_ARRAY0);
   g1=OGM_ToVect3d(Geom1,OGM_ARRAY1);

   if (!g0 || !g1)
      return(0);

   for(n0=0;n0<g0;n0++) {
      Vect_Assign(v0,OGM_Geom[0][n0]);

      c=0;

      for(n1=0,n11=g1-1;n1<g1;n11=n1++) {
         Vect_Assign(v1[0],OGM_Geom[1][n1]);
         Vect_Assign(v1[1],OGM_Geom[1][n11]);

         /*Check for point insidness*/
         if (OGR_PointInside(v0,v1[0],v1[1])) {
            c=!c;
         }
      }
      if (c) {
         t++;
         if (!All) {
            break;
         }
      }
   }
   return(All?t==g0:t);
}

int OGM_PolyPolyIntersect(OGRGeometryH Geom0,OGRGeometryH Geom1) {

   unsigned int n0,n1,g0,g1,n11;
   int          c;
   Vect3d       v0[2],v1[2];

   g0=OGM_ToVect3d(Geom0,OGM_ARRAY0);
   g1=OGM_ToVect3d(Geom1,OGM_ARRAY1);

   if (!g0 || !g1)
      return(0);

   for(n0=0;n0<(g0-1);n0++) {

      Vect_Assign(v0[0],OGM_Geom[0][n0]);
      Vect_Assign(v0[1],OGM_Geom[0][n0+1]);
      c=0;

      for(n1=0,n11=g1-2;n1<(g1-1);n11=n1++) {

         Vect_Assign(v1[0],OGM_Geom[1][n1]);
         Vect_Assign(v1[1],OGM_Geom[1][n11]);

         /*Check for segment intersection*/
         if ((OGM_SegmentIntersect(v0[0],v0[1],v1[0],v1[1],NULL)==1)) {
            return(1);
         }

         /*Check for point insidness*/
         if (OGR_PointInside(v0[0],v1[0],v1[1])) {
            c=!c;
         }
      }
      if (c) {
         return(1);
      }
   }
   return(0);
}

int OGM_LinePolyIntersect(OGRGeometryH Geom0,OGRGeometryH Geom1) {

   unsigned int n0,n1,g0,g1;
   Vect3d       v0[2],v1[2];

   g0=OGM_ToVect3d(Geom0,OGM_ARRAY0);
   g1=OGM_ToVect3d(Geom1,OGM_ARRAY1);

   if (!g0 || !g1)
      return(0);

   for(n0=0;n0<g0-1;n0++) {
      Vect_Assign(v0[0],OGM_Geom[0][n0]);
      Vect_Assign(v0[1],OGM_Geom[0][n0+1]);

      for(n1=0;n1<g1-1;n1++) {
         Vect_Assign(v1[0],OGM_Geom[1][n1]);
         Vect_Assign(v1[1],OGM_Geom[1][n1+1]);

         /*Check for segment intersection*/
         if ((OGM_SegmentIntersect(v0[0],v0[1],v1[0],v1[1],NULL)==1)) {
            return(1);
         }
      }
   }
   return(0);
}

double OGM_CoordLimit(OGRGeometryH Geom,int Coord,int Mode) {

   register unsigned int n=0;
   int                   g=0;
   double                val=0,valg;

   if (Coord>=0 && Coord<=2) {

      if (Mode==0) {         /*Minimum*/
         val=1e32;
      } else if (Mode==1) {  /*Maximum*/
         val=-1e32;
      } else {               /*Average*/
         val=0.0;
      }

      /*Boucle recursive sur les sous geometrie*/
      for(g=0;g<OGR_G_GetGeometryCount(Geom);g++) {
         valg=OGM_CoordLimit(OGR_G_GetGeometryRef(Geom,g),Coord,Mode);
         if (Mode==0) {
            val=valg<val?valg:val;
         } else if (Mode==1) {
            val=valg>val?valg:val;
         } else {
            val+=valg;
         }
      }

      for(n=0;n<OGM_ToVect3d(Geom,OGM_ARRAY0);n++) {
         if (Mode==0) {
            val=OGM_Geom[0][n][Coord]<val?OGM_Geom[0][n][Coord]:val;
         } else if (Mode==1) {
            val=OGM_Geom[0][n][Coord]>val?OGM_Geom[0][n][Coord]:val;
         } else {
            val+=OGM_Geom[0][n][Coord];
         }
      }
   }
   return(Mode==2?val/(n+g):val);
}

double OGM_Length(OGRGeometryH Geom) {

   double length=0;
   int    g;

   /*Boucle recursive sur les sous geometrie*/
   for(g=0;g<OGR_G_GetGeometryCount(Geom);g++) {
      length+=OGM_Length(OGR_G_GetGeometryRef(Geom,g));
   }

   return(length+=OGM_SegmentLength(Geom));
}

double OGM_SegmentLength(OGRGeometryH Geom) {

   register int n;
   double       length=0;

   for(n=0;n<OGM_ToVect3d(Geom,OGM_ARRAY0)-1;n++) {
      Vect_Substract(OGM_Geom[0][n],OGM_Geom[0][n+1],OGM_Geom[0][n]);
      length+=Vect_Norm(OGM_Geom[0][n]);
   }
   return(length);
}

double OGM_PointClosest(OGRGeometryH Geom,OGRGeometryH Pick,Vect3d Vr) {

   Vect3d vr;
   double d,dist=1e32;
   int    n,g;

   /*Boucle recursive sur les sous geometrie*/
   for(g=0;g<OGR_G_GetGeometryCount(Geom);g++) {
      d=OGM_PointClosest(OGR_G_GetGeometryRef(Geom,g),Pick,vr);
      if (d<dist) {
         dist=d;
         Vect_Assign(Vr,vr);
      }
   }

   for(g=0;g<OGM_ToVect3d(Geom,OGM_ARRAY1);g++) {
      for(n=0;n<OGM_ToVect3d(Pick,OGM_ARRAY0);n++) {
         d=hypot(OGM_Geom[1][g][0]-OGM_Geom[0][n][0],OGM_Geom[1][g][1]-OGM_Geom[0][n][1]);
         if (d<dist) {
            dist=d;
            Vect_Assign(Vr,OGM_Geom[1][g]);
         }
      }
   }

   return(dist);
}

int OGM_PointInside(OGRGeometryH Geom,OGRGeometryH Pick,Vect3d Vr) {

   OGRGeometryH pt;
   Vect3d       vr;
   int          n,g;

   pt=OGR_G_CreateGeometry(wkbPoint);
   for(n=0;n<OGR_G_GetPointCount(Geom);n++) {
      OGR_G_GetPoint(Geom,n,&vr[0],&vr[1],&vr[2]);
      OGR_G_SetPoint(pt,0,vr[0],vr[1],vr[2]);
      if (OGM_Intersect(pt,Pick,NULL,NULL)) {
         Vect_Assign(Vr,vr);
         return(n);
      }
   }
   OGR_G_DestroyGeometry(pt);

   /*Boucle recursive sur les sous geometrie*/
   for(g=0;g<OGR_G_GetGeometryCount(Geom);g++) {
      n=OGM_PointInside(OGR_G_GetGeometryRef(Geom,g),Pick,Vr);
      return(n);
   }

   return(-1);
}

/* 0----Intersection doesn't exists                                          */
/* 1----Intersection exists.                                                 */
/* 2----two line segments are parallel.                                      */
/* 3----two line segments are collinear, but not overlap.                    */
/* 4----two line segments are collinear, and share one same end point.       */
/* 5----two line segments are collinear, and overlap.                        */

int OGM_SegmentIntersect(Vect3d PointA,Vect3d PointB,Vect3d PointC,Vect3d PointD,Vect3d Inter) {

   double u,v,delta;
   double t1,t2;
   double a,b,c,d;
   double xba,yba,xdc,ydc,xca,yca;

   xba=PointB[0]-PointA[0];  yba=PointB[1]-PointA[1];
   xdc=PointD[0]-PointC[0];  ydc=PointD[1]-PointC[1];
   xca=PointC[0]-PointA[0];  yca=PointC[1]-PointA[1];

   delta=xba*ydc-yba*xdc;
   t1=xca*ydc-yca*xdc;
   t2=xca*yba-yca*xba;

   if (delta!=0.0) {

      u=t1/delta;
      v=t2/delta;

      /*two segments intersect (including intersect at end points)*/
      if (u<=1 && u>=0 && v<=1 && v>=0) {
         if (Inter) {
            Inter[0]=PointA[0]+u*(PointB[0]-PointA[0]);
            Inter[1]=PointA[1]+u*(PointB[1]-PointA[1]);
            Inter[2]=u;
         }
         return(1);
      } else {
         return(0);
      }
   } else {

      /* AB & CD are parallel. */
      if ((t1!=0) && (t2!=0))
         return(2);

      /* when AB & CD are collinear */
      /*if AB isn't a vertical line segment, project to x-axis */
      if (PointA[0]!=PointB[0]) {
         a=MIN(PointA[0],PointB[0]);
         b=MAX(PointA[0],PointB[0]);
         c=MIN(PointC[0],PointD[0]);
         d=MAX(PointC[0],PointD[0]);

         if (d<a || c>b) {
            return(3);
         } else if (d==a || c==b) {
            return(4);
         } else {
            return(5);
         }
      } else {
         /*if AB is a vertical line segment, project to y-axis*/
         a=MIN(PointA[1],PointB[1]);
         b=MAX(PointA[1],PointB[1]);
         c=MIN(PointC[1],PointD[1]);
         d=MAX(PointC[1],PointD[1]);

         if (d<a || c>b) {
            return(3);
         } else if (d==a || c==b) {
            return(4);
         } else {
            return(5);
         }
      }
   }
}

double OGM_SegmentDist(Vect3d SegA,Vect3d SegB,Vect3d Point) {

   double dx = SegB[0] - SegA[0];
   double dy = SegB[1] - SegA[1];

   // Line segment is a point
   if ((dx==0.0) && (dy==0.0)) {
      dx = Point[0] - SegA[0];
      dy = Point[1] - SegA[1];
      return(hypot(dx,dy));
   }

   double t = ((Point[0]-SegA[0])*dx+(Point[1]-SegA[1])*dy)/(dx*dx+dy*dy);

   if (t<0) {
      // Point is nearest to the first point
      dx = Point[0] - SegA[0];
      dy = Point[1] - SegA[1];
   } else if (t>1) {
      // Point is nearest to the end point
      dx = Point[0] - SegB[0];
      dy = Point[1] - SegB[1];
   } else {
      // Perpendicular line intersect the line segment.
      dx = Point[0] - (SegA[0] + t * dx);
      dy = Point[1] - (SegA[1] + t * dy);
   }

   //returning shortest distance
   return(hypot(dx,dy));
}

double OGM_Centroid2DProcess(OGRGeometryH Geom,double *X,double *Y) {

   int    i,g,n,i1;
   double area=0,mid;

   /* Process current geometry */
   n=OGM_ToVect3d(Geom,OGM_ARRAY0);

   if (n==1) {
      /* Proccess point */
      *X=OGM_Geom[0][0][0];
      *Y=OGM_Geom[0][0][1];
      return(0.0);
   } else if (n==2) {
      /* Process line */
      *X=OGM_Geom[0][0][0];
      *Y=OGM_Geom[0][0][1];
      *X+=(OGM_Geom[0][1][0]-*X)/2.0;
      *Y+=(OGM_Geom[0][1][1]-*Y)/2.0;
      return(0.0);
   }

   /* Process polygon/polyline */
   for(i=0;i<n;i++) {
      i1=(i+1)%n;

      area+=mid=OGM_Geom[0][i][0]*OGM_Geom[0][i1][1]-OGM_Geom[0][i][1]*OGM_Geom[0][i1][0];
      *X+=(OGM_Geom[0][i][0]+OGM_Geom[0][i1][0])*mid;
      *Y+=(OGM_Geom[0][i][1]+OGM_Geom[0][i1][1])*mid;
   }
   area*=0.5;

   /* Process sub geometry */
   for(g=0;g<OGR_G_GetGeometryCount(Geom);g++) {
      area+=OGM_Centroid2DProcess(OGR_G_GetGeometryRef(Geom,g),X,Y);
   }

   return(area);
}

double OGM_Centroid2D(OGRGeometryH Geom,double *X,double *Y) {

   OGREnvelope env;
   OGRSpatialReferenceH srs;
   double area,d;

   *X=0.0;
   *Y=0.0;

   area=OGM_Centroid2DProcess(Geom,X,Y);

   if (area!=0.0) {
      d=1.0/(6.0*area);
      *X*=d;
      *Y*=d;
   }

   if (!(srs=OGR_G_GetSpatialReference(Geom)) || OSRIsGeographic(srs)) {
      OGR_G_GetEnvelope(Geom,&env);
      if (-180.0<=env.MinX && 180.0>=env.MaxX && (env.MaxX-env.MinX)>180.0) {
         *X-=180.0;
      }
   }
   return(area<0?-area:area);
}

// Copyright 2002, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.
//
//  OGM_SimplifyDP():
//  This is the Douglas-Peucker recursive simplification routine
//  It just marks vertices that are part of the simplified polyline
//  for approximating the polyline subchain v[j] to v[k].
//    Input:  tol = approximation tolerance
//            v[] = polyline array of vertex points
//            j,k = indices for the subchain v[j] to v[k]
//    Output: mk[] = array of markers matching vertex array v[]
int OGM_SimplifyDP(double Tolerance,Vect3d *Pt,int J,int K,int *Markers) {

   /*There is nothing to simplify*/
   if (K<=J+1)
      return(0);

   int      i,n=0;
   int      maxi=J;                    // index of vertex farthest from S
   double   maxd2=0.0;                 // distance squared of farthest vertex
   double   tol2=Tolerance*Tolerance;  // tolerance squared
   double  cu;                         // segment length squared
   Vect3d  w,u,su,p;
   double  b,cw,dv2;                   // dv2 = distance v[i] to S squared

   // test each vertex v[i] for max distance from S
   // compute using the Feb 2001 Algorithm's dist_Point_to_Segment()
   // Note: this works in any dimension (2D, 3D, ...)

   Vect_Substract(u,Pt[K],Pt[J]);
   cu=Vect_DotProduct(u,u);

   for (i=J+1;i<K;i++) {
      // compute distance squared
      Vect_Substract(w,Pt[i],Pt[J]);
      cw=Vect_DotProduct(w,u);
      if (cw<=0) {
         dv2=Vect_Dist2(Pt[i],Pt[J]);
      } else if (cu<=cw) {
         dv2=Vect_Dist2(Pt[i],Pt[K]);
      } else {
         b=cw/cu;
         Vect_SMul(su,u,b);
         Vect_Add(p,Pt[J],su);
         dv2=Vect_Dist2(Pt[i],p);
      }
      // test with current max distance squared
      if (dv2<=maxd2)
         continue;

      // v[i] is a new max vertex
      maxi=i;
      maxd2=dv2;
   }

   /*Error is worse than the tolerance*/
   if (maxd2>tol2) {
      /*split the polyline at the farthest vertex from S*/
      Markers[maxi]=1;      // mark v[maxi] for the simplified polyline
      n++;

      /*Recursively simplify the two subpolylines at v[maxi]*/
      n+=OGM_SimplifyDP(Tolerance,Pt,J,maxi,Markers);  // polyline v[j] to v[maxi]
      n+=OGM_SimplifyDP(Tolerance,Pt,maxi,K,Markers);  // polyline v[maxi] to v[k]
   }
   /*Else the approximation is OK, so ignore intermediate vertices*/
   return(n);
}

// OGM_Simplify():
//    Input:  tol = approximation tolerance
//            V[] = polyline array of vertex points
//            n   = the number of points in V[]
//    Output: sV[]= simplified polyline vertices (max is n)
//    Return: m   = the number of points in sV[]
int OGM_Simplify(double Tolerance,OGRGeometryH Geom) {

   int    i,k,pv,n=-1,m=0;         // Misc counters
   double tol2=Tolerance*Tolerance;  // Tolerance squared
   int    *mk;                       // Marker buffer

   /*Simplify sub-geometry*/
   for(i=0;i<OGR_G_GetGeometryCount(Geom);i++) {
      m=OGM_Simplify(Tolerance,OGR_G_GetGeometryRef(Geom,i));
   }

   if ((n=OGM_ToVect3d(Geom,OGM_ARRAY0))>2) {
      mk=(int*)calloc(n,sizeof(int));
      if (!mk) {
         App_Log(ERROR,"%s: Unable to allocate buffers\n",__func__);
         return(0);
      }

      /*STAGE 1: Vertex Reduction within tolerance of prior vertex cluster*/
      for(i=k=1,pv=0;i<n;i++) {
         if (Vect_Dist2(OGM_Geom[0][i],OGM_Geom[0][pv])<tol2)
            continue;
         Vect_Assign(OGM_Geom[1][k],OGM_Geom[0][i]);
         k++;
         pv=i;
      }

      /*Start at beginning and finish at the end*/
      OGR_G_GetPoint(Geom,0,&OGM_Geom[1][0][0],&OGM_Geom[1][0][1],&OGM_Geom[1][0][2]);
      if (pv<n-1) {
         OGR_G_GetPoint(Geom,n-1,&OGM_Geom[1][k][0],&OGM_Geom[1][k][1],&OGM_Geom[1][k][2]);
         k++;
      }

      /*STAGE 2: Douglas-Peucker polyline simplification*/
      mk[0]=mk[k-1]=1;       // mark the first and last vertices
      m=2;
      if (k>2) m=OGM_SimplifyDP(Tolerance,OGM_Geom[1],0,k-1,mk);

      // copy marked vertices to the output simplified polyline
      OGR_G_Empty(Geom);
      if (m>=2) {
         for (i=m=0;i<k;i++) {
            if (mk[i]) {
               OGR_G_AddPoint_2D(Geom,OGM_Geom[1][i][0],OGM_Geom[1][i][1]);
               m++;
            }
         }
      } else {
         OGR_G_AddPoint_2D(Geom,OGM_Geom[1][0][0],OGM_Geom[1][0][1]);
         OGR_G_AddPoint_2D(Geom,OGM_Geom[1][k-1][0],OGM_Geom[1][k-1][1]);
      }
      free(mk);
   }
   return(m); // m vertices in simplified polyline
}

double OGM_AngleMin(OGRGeometryH Geom) {

   int    i,vi,n;
   double a,ma=M_2PI;
   Vect3d pt[3],v[2];
   
   // Parse subgeometry
   for(i=0;i<OGR_G_GetGeometryCount(Geom);i++) {
      a=OGM_AngleMin(OGR_G_GetGeometryRef(Geom,i));
      ma=fmin(DEG2RAD(a),ma);
   }
   
   // On polygons, don't use the last point as it's a repeat of the first. on other, don't use the last 2
   n=OGR_G_GetPointCount(Geom);
   for(i=0;i<n-(OGR_G_GetGeometryType(Geom)!=wkbLinearRing?2:1);i++) {
      vi=i;   OGR_G_GetPoint(Geom,vi<n?vi:vi-n,&pt[0][0],&pt[0][1],&pt[0][2]);
      vi=i+1; OGR_G_GetPoint(Geom,vi<n?vi:vi-n,&pt[1][0],&pt[1][1],&pt[1][2]);
      vi=i+2; OGR_G_GetPoint(Geom,vi<n?vi:vi-n,&pt[2][0],&pt[2][1],&pt[2][2]);
      
      // Use dot product method to get the angle
      Vect_Init(v[0],pt[1][0]-pt[0][0],pt[1][1]-pt[0][1],0.0);
      Vect_Init(v[1],pt[1][0]-pt[2][0],pt[1][1]-pt[2][1],0.0);
      
      a=fabs(acos(Vect_DotProduct(v[0],v[1])/Vect_Norm(v[0])/Vect_Norm(v[1])));    
      ma=fmin(a,ma);
   }
   
   return(RAD2DEG(ma));
}

int OGM_Clean(OGRGeometryH Geom) {

   unsigned int g;
   int    i,v,r=0;
   
   // Get vertices into a temporary vector array
   g=OGM_ToVect3d(Geom,OGM_ARRAY0);

   // Check for contiguous vextex repeat
   for(i=0;i<g-1;i++) {
      if (Vect_Equal(OGM_Geom[0][i],OGM_Geom[0][i+1])) {
         for(v=i;v<g-1;v++) {
            Vect_Assign(OGM_Geom[0][v],OGM_Geom[0][v+1]);
            r=1;
         }
         g--;
         i--;
      }
   }
   
   // If found any, rebuild geometry without the repeats
   if (r) {
      v=OGR_G_GetCoordinateDimension(Geom);
      OGR_G_SetPoints(Geom,g,&OGM_Geom[0][0][0],sizeof(Vect3d),&OGM_Geom[0][0][1],sizeof(Vect3d),&OGM_Geom[0][0][2],sizeof(Vect3d));
      OGR_G_SetCoordinateDimension(Geom,v);
   }
   
   // Parse subgeometry
   for(i=0;i<OGR_G_GetGeometryCount(Geom);i++) {
      r+=OGM_Clean(OGR_G_GetGeometryRef(Geom,i));
   }
   
   return(r);
}
#endif
