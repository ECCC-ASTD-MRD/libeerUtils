/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Lecture et traitements de fichiers raster
 * Fichier      : GeoRef_RPN.c
 * Creation     : Mars 2005 - J.P. Gauthier
 *
 * Description  : Fonctions de manipulations de projections aux standard WKT.
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

#include "App.h"
#include "GeoRef.h"
#include "Def.h"
#include "Vertex.h"

double   GeoRef_WKTDistance(TGeoRef *GRef,double X0,double Y0,double X1, double Y1);
int      GeoRef_WKTValue(TGeoRef *GRef,TDef *Def,char Mode,int C,double X,double Y,double Z,double *Length,double *ThetaXY);
int      GeoRef_WKTProject(TGeoRef *GRef,double X,double Y,double *Lat,double *Lon,int Extrap,int Transform);
int      GeoRef_WKTUnProject(TGeoRef *GRef,double *X,double *Y,double Lat,double Lon,int Extrap,int Transform);

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_WKTDistance>
 * Creation     : Mars 2007 J.P. Gauthier - CMC/CMOE
 *
 * But          : Calculer la distance entre deux points.
 *
 * Parametres    :
 *   <GRef>      : Pointeur sur la reference geographique
 *   <X0>        : coordonnee en X dans la projection/grille
 *   <Y0>        : coordonnee en Y dans la projection/grille
 *   <X0>        : coordonnee en X dans la projection/grille
 *   <Y0>        : coordonnee en Y dans la projection/grille
 *
 * Retour       : Distance
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
double GeoRef_WKTDistance(TGeoRef *GRef,double X0,double Y0,double X1, double Y1) {

#ifdef HAVE_GDAL
   double i[2],j[2],lat[2],lon[2],u;
   char *unit,geo;
   
   X0+=GRef->X0;
   X1+=GRef->X0;
   Y0+=GRef->Y0;
   Y1+=GRef->Y0;
   
   // Check for unit type 
   geo=0;
   if (GRef->Grid[0]=='Z' || GRef->Grid[1]=='Z') {
      geo=1;
   } else {
      if (GRef->Spatial) {
         u=OSRGetLinearUnits(GRef->Spatial,&unit);
         geo=(unit[0]!='M' && unit[0]!='m');
      }
   }

   if (geo) {
      GeoRef_WKTProject(GRef,X0,Y0,&lat[0],&lon[0],1,1);
      GeoRef_WKTProject(GRef,X1,Y1,&lat[1],&lon[1],1,1);
      return(DIST(0.0,DEG2RAD(lat[0]),DEG2RAD(lon[0]),DEG2RAD(lat[1]),DEG2RAD(lon[1])));
   } else {
      if (GRef->Transform) {
         i[0]=GRef->Transform[0]+GRef->Transform[1]*X0+GRef->Transform[2]*Y0;
         j[0]=GRef->Transform[3]+GRef->Transform[4]*X0+GRef->Transform[5]*Y0;
         i[1]=GRef->Transform[0]+GRef->Transform[1]*X1+GRef->Transform[2]*Y1;
         j[1]=GRef->Transform[3]+GRef->Transform[4]*X1+GRef->Transform[5]*Y1;
      } else {
         i[0]=X0;
         j[0]=Y0;
         i[1]=X1;
         j[1]=Y1;
      }
      return(hypot(j[1]-j[0],i[1]-i[0])*u);
   }
#else
   Lib_Log(APP_LIBEER,APP_ERROR,"Function %s is not available, needs to be built with GDAL\n",__func__);
   return(0.0);
#endif
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_WKTValue>
 * Creation     : Mars 2005 J.P. Gauthier - CMC/CMOE
 *
 * But          : Extraire la valeur d'une matrice de donnees.
 *
 * Parametres    :
 *   <GRef>      : Pointeur sur la reference geographique
 *   <Def>       : Pointeur sur la definition de la donnee
 *   <Mode>      : Mode d'interpolation (N=NEAREST,L=LINEAR);
 *   <C>         : Composante
 *   <X>         : coordonnee en X dans la projection/grille
 *   <Y>         : coordonnee en Y dans la projection/grille
 *   <Z>         : coordonnee en Z dans la projection/grille
 *   <Length>    : Module interpolee
 *   <ThetaXY>   : Direction interpolee
 *
 * Retour        : Memory index of nearest point + 1, 0 if oustide or masked
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoRef_WKTValue(TGeoRef *GRef,TDef *Def,char Mode,int C,double X,double Y,double Z,double *Length,double *ThetaXY){

   double       x,y,d,ddir=0.0;
   int          mem,ix,iy;
   unsigned int idx;

  *Length=Def->NoData;
   d=1.0;

   //i on est a l'interieur de la grille ou que l'extrapolation est activee
   if (C<Def->NC && X>=(GRef->X0-d) && Y>=(GRef->Y0-d) && Z>=0 && X<=(GRef->X1+d) && Y<=(GRef->Y1+d) && Z<=Def->NK-1) {

      X-=GRef->X0;
      Y-=GRef->Y0;
      DEFCLAMP(Def,X,Y);

      // Index memoire du niveau desire
      mem=Def->NI*Def->NJ*(int)Z;

      ix=lrint(X);
      iy=lrint(Y);
      idx=iy*Def->NI+ix;

      // Check for mask
      if (Def->Mask && !Def->Mask[idx]) {
         return(FALSE);
      }
      
      // Reproject vector orientation by adding grid projection's north difference
      if (Def->Data[1] && GRef->Type&GRID_NUNORTH) { 
         ddir=GeoRef_GeoDir(GRef,X,Y);
      }

      mem+=idx;
      if (Def->Type<=9 || Mode=='N' || (X==ix && Y==iy)) {
         Def_GetMod(Def,mem,*Length);

         // Pour un champs vectoriel
         if (Def->Data[1] && ThetaXY) {
            Def_Get(Def,0,mem,x);
            Def_Get(Def,1,mem,y);
            *ThetaXY=180+RAD2DEG(atan2(x,y)-ddir);
         }
      } else {
         *Length=VertexVal(Def,-1,X,Y,Z);
         // Pour un champs vectoriel
         if (Def->Data[1] && ThetaXY) {
            x=VertexVal(Def,0,X,Y,Z);
            y=VertexVal(Def,1,X,Y,Z);
            *ThetaXY=180+RAD2DEG(atan2(x,y)-ddir);
         }
      }
      if (DEFVALID(Def,*Length)) {
         return(mem+1);
      }
   }
   
   return(FALSE);
}

static inline int GeoRef_WKTRotate(TRotationTransform *T,double *Lat,double *Lon) {

   double lat,lon,x,y,z,xr,yr,zr;
   
   lon = DEG2RAD(*Lon);
   lat = DEG2RAD(*Lat);

   // Convert from spherical to cartesian coordinates
   x = cos(lon)*cos(lat); 
   y = sin(lon)*cos(lat);
   z = sin(lat);

   xr = T->CosTheta*T->CosPhi*x + T->CosTheta*T->SinPhi*y + T->SinTheta*z;
   yr = -T->SinPhi*x + T->CosPhi*y;
   zr = -T->SinTheta*T->CosPhi*x - T->SinTheta*T->SinPhi*y + T->CosTheta*z;
      
   // Convert cartesian back to spherical coordinates
   *Lon = RAD2DEG(atan2(yr,xr)); 
   *Lat = RAD2DEG(asin(zr));
   
   return(TRUE);
}

static inline int GeoRef_WKTUnRotate(TRotationTransform *T,double *Lat,double *Lon) {

   double lat,lon,x,y,z,xr,yr,zr;
   
   lon = DEG2RAD(*Lon);
   lat = DEG2RAD(*Lat);

   // Convert from spherical to cartesian coordinates
   x = cos(lon)*cos(lat); 
   y = sin(lon)*cos(lat);
   z = sin(lat);

   xr = T->CosTheta*T->CosPhi*x + -T->SinPhi*y + -T->SinTheta*T->CosPhi*z;
   yr = -T->CosTheta*-T->SinPhi*x + T->CosPhi*y - -T->SinTheta*-T->SinPhi*z;
   zr = T->SinTheta*x + T->CosTheta*z;
      
   // Convert cartesian back to spherical coordinates
   *Lon = RAD2DEG(atan2(yr,xr)); 
   *Lat = RAD2DEG(asin(zr));

   return(TRUE);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_WKTProject>
 * Creation     : Mars 2005 J.P. Gauthier - CMC/CMOE
 *
 * But          : Projeter une coordonnee de projection en latlon.
 *
 * Parametres    :
 *   <GRef>      : Pointeur sur la reference geographique
 *   <X>         : coordonnee en X dans la projection/grille
 *   <Y>         : coordonnee en Y dans la projection/grille
 *   <Lat>       : Latitude
 *   <Lon>       : Longitude
 *   <Extrap>    : Extrapolation hors grille
 *   <Transform> : Appliquer la transformation
 *
 * Retour       : Inside (1 si a l'interieur du domaine).
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoRef_WKTProject(TGeoRef *GRef,double X,double Y,double *Lat,double *Lon,int Extrap,int Transform) {

#ifdef HAVE_GDAL
   double d,dx,dy,x,y,z=0.0;
   int    sx,sy,s,ok,gidx;

   d=1.0;

   if( !Extrap && (X>(GRef->X1+d) || Y>(GRef->Y1+d) || X<(GRef->X0-d) || Y<(GRef->Y0-d)) ) {
      *Lon=-999.0;
      *Lat=-999.0;
      return(0);
   }

   // Grid cell are corner defined 
   if (GRef->Type&GRID_CORNER) {
      X+=0.5;
      Y+=0.5;
   }

   // Because some grids are defined as WZ and others as ZW, this makes sure we catch the other letter
   gidx = GRef->Grid[0]=='W' ? 1 : 0;

   // In case of non-uniform grid, figure out where in the position vector we are
   if (GRef->Grid[gidx]=='Z') {
      if (GRef->AX && GRef->AY) {
         // X
         if( X < GRef->X0 )      { sx=GRef->X0; X=GRef->AX[sx]-(GRef->AX[sx]-GRef->AX[sx+1])*(X-sx); }
         else if( X > GRef->X1 ) { sx=GRef->X1; X=GRef->AX[sx]+(GRef->AX[sx]-GRef->AX[sx-1])*(X-sx); }
         else                    { sx=floor(X); X=sx==X?GRef->AX[sx]:ILIN(GRef->AX[sx],GRef->AX[sx+1],X-sx); }

         // Y
         s=GRef->NX;
         if( Y < GRef->Y0 )      { sy=GRef->Y0; Y=GRef->AY[sy*s]-(GRef->AY[sy*s]-GRef->AY[(sy+1)*s])*(Y-sy); }
         else if( Y > GRef->Y1 ) { sy=GRef->Y1; Y=GRef->AY[sy*s]+(GRef->AY[sy*s]-GRef->AY[(sy-1)*s])*(Y-sy); }
         else                    { sy=floor(Y); Y=sy==Y?GRef->AY[sy*s]:ILIN(GRef->AY[sy*s],GRef->AY[(sy+1)*s],Y-sy); }
      }
   } else if (GRef->Grid[gidx]=='X' || GRef->Grid[gidx]=='Y') {
      if (GRef->AX && GRef->AY) {
         sx=floor(X);sx=CLAMP(sx,GRef->X0,GRef->X1);
         sy=floor(Y);sy=CLAMP(sy,GRef->Y0,GRef->Y1);
         dx=X-sx;;
         dy=Y-sy;

         s=sy*GRef->NX+sx;
         X=GRef->AX[s];
         Y=GRef->AY[s];

         if (++sx<=GRef->X1) {
            s=sy*GRef->NX+sx;
            X+=(GRef->AX[s]-X)*dx;
         }

         if (++sy<=GRef->Y1) {
            s=sy*GRef->NX+(sx-1);
            Y+=(GRef->AY[s]-Y)*dy;
         }
      }
   }

   // Transform the point into georeferenced coordinates 
   x=X;
   y=Y;
   if (Transform) {
      if (GRef->Transform) {
         x=GRef->Transform[0]+GRef->Transform[1]*X+GRef->Transform[2]*Y;
         y=GRef->Transform[3]+GRef->Transform[4]*X+GRef->Transform[5]*Y;
      } else if (GRef->GCPTransform) {
         GDALGCPTransform(GRef->GCPTransform,FALSE,1,&x,&y,&z,&ok);
      } else if (GRef->TPSTransform) {
         GDALGCPTransform(GRef->TPSTransform,FALSE,1,&x,&y,&z,&ok);
      } else if (GRef->RPCTransform) {
         GDALGCPTransform(GRef->RPCTransform,FALSE,1,&x,&y,&z,&ok);
      }
   }
   
   // Transform to latlon
   if (GRef->Function) {
      if (!OCTTransform(GRef->Function,1,&x,&y,NULL)) {
         *Lon=-999.0;
         *Lat=-999.0;
         return(0);
      }
   }

   *Lon=x;
   *Lat=y;
   
   if (GRef->RotTransform) 
      GeoRef_WKTUnRotate(GRef->RotTransform,Lat,Lon);
   
   return(1);
#else
   Lib_Log(APP_LIBEER,APP_ERROR,"Function %s is not available, needs to be built with GDAL\n",__func__);
   return(0);
#endif
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_WKTUnProject>
 * Creation     : Mars 2005 J.P. Gauthier - CMC/CMOE
 *
 * But          : Projeter une latlon en position grille.
 *
 * Parametres    :
 *   <GRef>      : Pointeur sur la reference geographique
 *   <X>         : coordonnee en X dans la projection/grille
 *   <Y>         : coordonnee en Y dans la projection/grille
 *   <Lat>       : Latitude
 *   <Lon>       : Longitude
 *   <Extrap>    : Extrapolation hors grille
 *   <Transform> : Appliquer la transformation
 *
 * Retour       : Inside (1 si a l'interieur du domaine).
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoRef_WKTUnProject(TGeoRef *GRef,double *X,double *Y,double Lat,double Lon,int Extrap,int Transform) {

#ifdef HAVE_GDAL
   double x,y,z=0.0,d=1e32,sd;
   int    n,nd,s,dx,dy,ok,idx,idxs[8],gidx;
   double dists[8];
   Vect2d pts[4],pt;
   Vect3d b;
   
   if (GRef->RotTransform) 
      GeoRef_WKTRotate(GRef->RotTransform,&Lat,&Lon);

   if (Lat<=90.0 && Lat>=-90.0 && Lon!=-999.0) {

      // Longitude from -180 to 180
      Lon=Lon>180?Lon-360:Lon;

      x=Lon;
      y=Lat;

      // Transform from latlon 
      if (GRef->InvFunction) {
         if (!OCTTransform(GRef->InvFunction,1,&x,&y,NULL)) {
            *X=-1.0;
            *Y=-1.0;
            return(0);
         }
      }

      // Transform from georeferenced coordinates
      *X=x;
      *Y=y;
      if (Transform) {
         if (GRef->InvTransform) {
            *X=GRef->InvTransform[0]+GRef->InvTransform[1]*x+GRef->InvTransform[2]*y;
            *Y=GRef->InvTransform[3]+GRef->InvTransform[4]*x+GRef->InvTransform[5]*y;
         } else if (GRef->GCPTransform) {
            GDALGCPTransform(GRef->GCPTransform,TRUE,1,X,Y,&z,&ok);
         } else if (GRef->TPSTransform) {
            GDALTPSTransform(GRef->TPSTransform,TRUE,1,X,Y,&z,&ok);
         } else if (GRef->RPCTransform) {
            GDALRPCTransform(GRef->RPCTransform,TRUE,1,X,Y,&z,&ok);
         }

         // Because some grids are defined as WZ and others as ZW, this makes sure we catch the other letter
         gidx = GRef->Grid[0]=='W' ? 1 : 0;

         // In case of non-uniform grid, figure out where in the position vector we are 
         if (GRef->Grid[gidx]=='Z') {
            if (GRef->AX && GRef->AY) {
               s=GRef->X0;
               // Check if vector is increasing
               if (GRef->AX[s]<GRef->AX[s+1]) {
                  while(s<=GRef->X1 && *X>GRef->AX[s]) s++;
               } else {
                  while(s<=GRef->X1 && *X<GRef->AX[s]) s++;
               }
               if (s>GRef->X0) {
                  // We're in so interpolate postion
                  if (s<=GRef->X1) {
                     *X=(*X-GRef->AX[s-1])/(GRef->AX[s]-GRef->AX[s-1])+s-1;
                  } else {
                     *X=(*X-GRef->AX[GRef->X1])/(GRef->AX[GRef->X1]-GRef->AX[GRef->X1-1])+s-1;
                  }
               } else {
                  // We're out so extrapolate position
                  *X=GRef->X0+(*X-GRef->AX[0])/(GRef->AX[1]-GRef->AX[0]);
               }

               s=GRef->Y0;dx=GRef->NX;
               // Check if vector is increasing
               if (GRef->AY[s*GRef->NX]<GRef->AY[(s+1)*GRef->NX]) {
                  while(s<=GRef->Y1 && *Y>GRef->AY[s*GRef->NX]) s++;
               } else {
                  while(s<=GRef->Y1 && *Y<GRef->AY[s*GRef->NX]) s++;
               }
               if (s>GRef->Y0) {
                  // We're in so interpolate postion
                  if (s<=GRef->Y1) {
                     *Y=(*Y-GRef->AY[(s-1)*GRef->NX])/(GRef->AY[s*GRef->NX]-GRef->AY[(s-1)*GRef->NX])+s-1;
                  } else {
                     *Y=(*Y-GRef->AY[GRef->Y1*GRef->NX])/(GRef->AY[GRef->Y1*GRef->NX]-GRef->AY[(GRef->Y1-1)*GRef->NX])+s-1;
                  }
               } else {
                  // We're out so extrapolate position
                  *Y=GRef->Y0+(*Y-GRef->AY[0])/(GRef->AY[GRef->NX]-GRef->AY[0]);
               }
            }
         } else if (GRef->Grid[gidx]=='Y') {
            // Get nearest point
            if (GeoRef_Nearest(GRef,Lon,Lat,&idx,dists,1)) {
               if (dists[0]<1.0) {
                  *Y=(int)(idx/GRef->NX);
                  *X=idx-(*Y)*GRef->NX;
                  return(TRUE);
               }
            }
         } else if (GRef->Grid[gidx]=='X') {
            // Get nearest points
            if ((nd=GeoRef_Nearest(GRef,Lon,Lat,idxs,dists,8))) {
               
               pt[0]=Lon;
               pt[1]=Lat;

               // Find which cell includes coordinates
               for(n=0;n<nd;n++) {
                  idx=idxs[n];

                  // Find within which quad
                  dx=-1;dy=-1;
                  if (!GeoRef_WithinCell(GRef,pt,pts,idx-GRef->NX-1,idx-1,idx,idx-GRef->NX)) {
                 
                     dx=0;dy=-1;
                     if (!GeoRef_WithinCell(GRef,pt,pts,idx-GRef->NX,idx,idx+1,idx-GRef->NX+1)) {
                        
                        dx=-1;dy=0;
                        if (!GeoRef_WithinCell(GRef,pt,pts,idx-1,idx+GRef->NX-1,idx+GRef->NX,idx)) {
                     
                           dx=0;dy=0;
                           if (!GeoRef_WithinCell(GRef,pt,pts,idx,idx+GRef->NX,idx+GRef->NX+1,idx+1)) {
                              idx=-1;
                           }
                        }
                     }
                  }
                  
                  // If found, exit loop
                  if (idx!=-1) {
                     break;
                  }
               }
               
               if (idx!=-1) {
                  // Map coordinates to grid
                  Vertex_Map(pts,X,Y,Lon,Lat);
                  
                  if (!ISNAN(*X) && !ISNAN(*Y)) {
                     y=idx/GRef->NX;
                     x=idx-y*GRef->NX;
                     *Y+=y+dy;
                     *X+=x+dx; 
                  } else {
                     *X=-1,0;
                     *Y=-1.0;
                     return(FALSE);
                  }
               }
            }
         }

         // Check the grid limits
         d=1.0;
         if (*X>(GRef->X1+d) || *Y>(GRef->Y1+d) || *X<(GRef->X0-d) || *Y<(GRef->Y0-d)) {
            if (!Extrap) {
               *X=-1.0;
               *Y=-1.0;
            }
            return(0);
         }
      }
      // Grid cell are corner defined 
      if (GRef->Type&GRID_CORNER) {
         *X-=0.5;
         *Y-=0.5;
      }
      
   } else {
      *X=-1.0;
      *Y=-1.0;
      return(0);
   }
   return(1);
#else
   Lib_Log(APP_LIBEER,APP_ERROR,"Function %s is not available, needs to be built with GDAL\n",__func__);
   return(0);
#endif
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_WKTSet>
 * Creation     : Juin 2004 J.P. Gauthier - CMC/CMOE
 *
 * But          : Definir les fonctions de transformations WKT
 *
 * Parametres   :
 *   <GRef>     : Pointeur sur la reference geographique
 *   <String>   : Description de la projection
 *   <Geometry> : Geometrie d'ou extraire la reference spatiale (optionel=NULL)
 *
 * Retour       :
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoRef_WKTSet(TGeoRef *GRef,char *String,double *Transform,double *InvTransform,OGRSpatialReferenceH Spatial) {

#ifdef HAVE_GDAL
   static OGRSpatialReferenceH llref=NULL;
   char                      *string=NULL;

   if (String && String[0]!='\0') {
      string=strdup(String);
      strtrim(string,' ');
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Projection string: %s\n",__func__,String);
   }

   GeoRef_Clear(GRef,0);

   if (Transform || InvTransform) {
      if (!GRef->Transform)
         GRef->Transform=(double*)calloc(6,sizeof(double));
      if (!GRef->InvTransform)
         GRef->InvTransform=(double*)calloc(6,sizeof(double));
   }
   
   if (Transform) {
      memcpy(GRef->Transform,Transform,6*sizeof(double));
   } else {
      if (!InvTransform || !GDALInvGeoTransform(InvTransform,GRef->Transform)) {
         if (GRef->Transform) {
            free(GRef->Transform);
            GRef->Transform=NULL;
         }
      }
   }

   if (InvTransform) {
      memcpy(GRef->InvTransform,InvTransform,6*sizeof(double));
   } else {
      if (!Transform || !GDALInvGeoTransform(Transform,GRef->InvTransform)) {
         if (GRef->InvTransform) {
            free(GRef->InvTransform);
            GRef->InvTransform=NULL;
         }
      }
   } 

   if (Spatial) {
      GRef->Spatial=OSRClone(Spatial);
      OSRExportToWkt(GRef->Spatial,&string);
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Projection from spatial:%p\n",__func__,string);
   } else if (string) {
      GRef->Spatial=OSRNewSpatialReference(NULL);
      if (OSRSetFromUserInput(GRef->Spatial,string)==OGRERR_FAILURE) {
        Lib_Log(APP_LIBEER,APP_WARNING,"%s: Unable to create spatial reference\n",__func__);
        return(0);
      }
   } else {
      string=strdup(REFDEFAULT);
      GRef->Spatial=OSRNewSpatialReference(string);
   }

   if (GRef->String)
      free(GRef->String);
   GRef->String=string;

   if (GRef->Spatial) {
      if (!llref) {
         // Create global latlon reference on perfect sphere
         llref=OSRNewSpatialReference(NULL);
         OSRSetFromUserInput(llref,"EPSG:4047");
#if defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 3
         OSRSetAxisMappingStrategy(llref,OAMS_TRADITIONAL_GIS_ORDER);
#endif
      }

      if (llref) {
         // Create forward/backward tranformation functions
         GRef->Function=OCTNewCoordinateTransformation(GRef->Spatial,llref);
         GRef->InvFunction=OCTNewCoordinateTransformation(llref,GRef->Spatial);
         if (!GRef->Function || !GRef->InvFunction)
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to create transformation functions\n",__func__);
      }
   } else {
      Lib_Log(APP_LIBEER,APP_WARNING,"%s: Unable to get spatial reference\n",__func__);
      return(0);
   }

   GRef->Project=GeoRef_WKTProject;
   GRef->UnProject=GeoRef_WKTUnProject;
   GRef->Value=(TGeoRef_Value*)GeoRef_WKTValue;
   GRef->Distance=GeoRef_WKTDistance;
   GRef->Height=NULL;

   return(1);
#else
   Lib_Log(APP_LIBEER,APP_ERROR,"Function %s is not available, needs to be built with GDAL\n",__func__);
   return(0);
#endif
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_WKTSetup>
 * Creation     : Avril 2005 J.P. Gauthier - CMC/CMOE
 *
 * But          : Definir le referetiel de type RPN
 *
 * Parametres   :
 *    <NI>      : Dimension en X
 *    <NJ>      : Dimension en Y
 *    <GRTYP>   : Type de grille
 *    <IG1>     : Descripteur IG1
 *    <IG2>     : Descripteur IG2
 *    <IG3>     : Descripteur IG3
 *    <IG4>     : Descripteur IG4
 *
 * Retour       :
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
TGeoRef *GeoRef_WKTSetup(int NI,int NJ,char *GRTYP,int IG1,int IG2,int IG3,int IG4,char *String,double *Transform,double *InvTransform,OGRSpatialReferenceH Spatial) {

   TGeoRef *ref;

   ref=GeoRef_New();
   GeoRef_Size(ref,0,0,NI>0?NI-1:0,NJ>0?NJ-1:0,0);
   if (!GeoRef_WKTSet(ref,String,Transform,InvTransform,Spatial)) {
      return(NULL);
   }
   
   if (GRTYP) {
      ref->Grid[0]=GRTYP[0];
      ref->Grid[1]=GRTYP[1];
   } else {
      ref->Grid[0]='W';
      ref->Grid[1]='\0';
   }
   ref->IG1=IG1;
   ref->IG2=IG2;
   ref->IG3=IG3;
   ref->IG4=IG4;

   return(ref);
}
