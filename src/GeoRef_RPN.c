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
 * Description  : Fonctions de manipulations de projections aux standard RPN.
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
#include "RPN.h"
#include "Vertex.h"
#include "EZGrid.h"

void     GeoRef_Expand(TGeoRef *GRef);
double   GeoRef_RPNDistance(TGeoRef *GRef,double X0,double Y0,double X1, double Y1);
int      GeoRef_RPNValue(TGeoRef *GRef,TDef *Def,char Mode,int C,double X,double Y,double Z,double *Length,double *ThetaXY);
int      GeoRef_RPNProject(TGeoRef *GRef,double X,double Y,double *Lat,double *Lon,int Extrap,int Transform);
int      GeoRef_RPNUnProject(TGeoRef *GRef,double *X,double *Y,double Lat,double Lon,int Extrap,int Transform);

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_Expand>
 * Creation     : Mars 2005 J.P. Gauthier - CMC/CMOE
 *
 * But          : Effectuer l'expansion des axes de la grilles selon les >> ^^.
 *
 * Parametres    :
 *   <GRef>       : Pointeur sur la reference geographique
 *
 * Retour       :
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
void GeoRef_Expand(TGeoRef *GRef) {

#ifdef HAVE_RMN
   double lat,lon;
   int    i;

   if (GRef->Ids && !GRef->AX) {
      GRef->AX=(float*)calloc((int)GRef->X1+1,sizeof(float));
      GRef->AY=(float*)calloc((int)GRef->Y1+1,sizeof(float));

      if (GRef->AX && GRef->AY) {
         if (GRef->Grid[0]=='Z') {
//            RPN_IntLock();
            c_gdgaxes(GRef->Ids[GRef->NId],GRef->AX,GRef->AY);
//            RPN_IntUnlock();
         } else {
            for(i=0;i<=GRef->X1;i++) {
               GRef->Project(GRef,i,0,&lat,&lon,1,1);
               GRef->AX[i]=lon;
            }
            for(i=0;i<=GRef->Y1;i++) {
               GRef->Project(GRef,0,i,&lat,&lon,1,1);
               GRef->AY[i]=lat;
            }
         }
      }
   }
#endif
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RPNDistance>
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
double GeoRef_RPNDistance(TGeoRef *GRef,double X0,double Y0,double X1, double Y1) {

#ifdef HAVE_RMN
   float i[2],j[2],lat[2],lon[2];

   if (GRef->Ids) {
      i[0]=X0+1.0;
      j[0]=Y0+1.0;
      i[1]=X1+1.0;
      j[1]=Y1+1.0;

//      RPN_IntLock();
      c_gdllfxy(GRef->Ids[GRef->NId],lat,lon,i,j,2);
//      RPN_IntUnlock();

      X0=DEG2RAD(lon[0]);
      X1=DEG2RAD(lon[1]);
      Y0=DEG2RAD(lat[0]);
      Y1=DEG2RAD(lat[1]);

      return(DIST(0.0,Y0,X0,Y1,X1));
   }
#endif
   return(hypot(X1-X0,Y1-Y0));
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RPNValue>
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
 * Retour       : Inside (1 si a l'interieur du domaine).
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/

int GeoRef_RPNValue(TGeoRef *GRef,TDef *Def,char Mode,int C,double X,double Y,double Z,double *Length,double *ThetaXY) {

   Vect3d       b,v;
   float        x,y,valf,valdf;
   void        *p0,*p1;
   int          mem,ix,iy,n;
   unsigned int idx;

   *Length=Def->NoData;

#ifdef HAVE_RMN
   // In case of triangle meshe
   if (GRef->Grid[0]=='M') {
      if (C<Def->NC && X>=0 && Y>=0) {
         b[0]=X-(int)X;
         b[1]=Y-(int)Y;
         b[2]=1.0-b[0]-b[1];
         ix=(int)X;

         if (Mode=='N') {
            n=(b[0]>b[1]?(b[0]>b[2]?0:2):(b[1]>b[2]?1:2));
            Def_Get(Def,C,GRef->Idx[ix+n],x);
         } else {
            Def_Get(Def,C,GRef->Idx[ix],v[0]);
            Def_Get(Def,C,GRef->Idx[ix+1],v[1]);
            Def_Get(Def,C,GRef->Idx[ix+2],v[2]);

            x=Bary_InterpV(b,v);
         }
         *Length=x;
         
         if (Def->Data[1] && !C) {
            if (Mode=='N') {
               n=(b[0]>b[1]?(b[0]>b[2]?0:2):(b[1]>b[2]?1:2));
               Def_Get(Def,1,GRef->Idx[ix+n],y);
            } else {
               Def_Get(Def,1,GRef->Idx[ix],v[0]);
               Def_Get(Def,1,GRef->Idx[ix+1],v[1]);
               Def_Get(Def,1,GRef->Idx[ix+2],v[2]);

               y=Bary_InterpV(b,v);
            }
            *Length=hypot(x,y);
            *ThetaXY=180+RAD2DEG(atan2(x,y));
         }
         return(TRUE);
      } else {
         return(FALSE);
      }
   }

   // Si on est a l'interieur de la grille ou que l'extrapolation est activee
   if (C<Def->NC && X>=(GRef->X0-0.5) && Y>=(GRef->Y0-0.5) && Z>=0 && X<(GRef->X1+0.5) && Y<(GRef->Y1+0.5) && Z<=Def->NK-1) {

      // Index memoire du niveau desire
      mem=Def->NIJ*(int)Z;

      ix=lrint(X);
      iy=lrint(Y);
      idx=iy*Def->NI+ix;
      
      // Check for mask
      if (Def->Mask && !Def->Mask[mem+idx]) {
         return(FALSE);
      }

      // Point cloud
      if (GRef->Grid[0]=='Y' || GRef->Grid[0]=='P' || Def->NI==1 || Def->NJ==1) {
         mem+=idx;
         Def_Get(Def,C,mem,x);
         if (Def->Data[1] && !C) {
            Def_Get(Def,1,mem,y);
            if (ThetaXY) *ThetaXY=180+RAD2DEG(atan2(x,y));
            *Length=hypot(x,y);
         } else {
            *Length=x;   
         }
         return(TRUE);
      } 

      // Check for nodata in linear interpolation 
      if (Def->Type>=TD_Int64 && Mode!='N') {
         int idxs[4],dx,dy;
         double vals[4];
         
         dx=X;
         dy=Y;
         idxs[0]=mem+dy*Def->NI+dx;
         idxs[1]=(dx<GRef->X1)?idxs[0]+1:idxs[0];
         idxs[2]=(dy<GRef->Y1)?idxs[0]+Def->NI:idxs[0];
         idxs[3]=(dy<GRef->Y1 && dx<GRef->X1)?idxs[0]+Def->NI+1:idxs[0];
         
         Def_GetQuad(Def,C,idxs,vals);

         // If either value is nodata then interpolation will be nodata as well
         if (!DEFVALID(Def,vals[0]) || !DEFVALID(Def,vals[1]) || !DEFVALID(Def,vals[2]) || !DEFVALID(Def,vals[3])) 
            return(FALSE);        
      }
     
      // XSection
      if (GRef->Grid[0]=='V') {
         if (Def->Data[1]) {
            Def_GetMod(Def,FIDX2D(Def,ix,iy),*Length);
         } else {
            *Length=VertexVal(Def,-1,X,Y,0.0);
         }
         return(TRUE);
      }

      // Unstructured or not referenced
      if (GRef->Grid[0]=='X') {
         if (Def->Type<=TD_Int64 || Mode=='N' || (X==ix && Y==iy)) {
            mem+=idx;

           // Pour un champs vectoriel
            if (Def->Data[1] && !C) {
               Def_Get(Def,0,mem,x);
               Def_Get(Def,1,mem,y);
               if (ThetaXY) *ThetaXY=180+RAD2DEG(atan2(x,y)-GeoRef_GeoDir(GRef,X,Y));
               *Length=hypot(x,y);
            } else {
               Def_GetMod(Def,mem,*Length);              
            }
         } else {
            // Pour un champs vectoriel
            if (Def->Data[1] && !C) {
               x=VertexVal(Def,0,X,Y,Z);
               y=VertexVal(Def,1,X,Y,Z);
               if (ThetaXY) *ThetaXY=180+RAD2DEG(atan2(x,y)-GeoRef_GeoDir(GRef,X,Y));
               *Length=hypot(x,y);
            } else {
               *Length=VertexVal(Def,-1,X,Y,Z);               
            }
         }
         return(TRUE);
      }
   
      // RPN grid
      if (GRef->Ids) {         
         if (Def->Type==TD_Float32 && Def->Data[1] && !C) { 
            x=X+1.0;
            y=Y+1.0;

            if (Mode=='N') {
               x=lrint(x);
               y=lrint(y);
            }
            
            Def_Pointer(Def,0,mem,p0);
            Def_Pointer(Def,1,mem,p1);
            c_gdxywdval(GRef->Ids[GRef->NId],&valf,&valdf,p0,p1,&x,&y,1);

            // If it's 3D, use the mode for speed since c_gdxywdval only uses 2D
            if (Def->Data[2])
               c_gdxysval(GRef->Ids[GRef->NId],&valf,(float*)&Def->Mode[mem],&x,&y,1);
               *Length=valf;
            if (ThetaXY)
               *ThetaXY=valdf;
         } else {            
            if (Mode=='N') {
               mem+=idx;
               Def_Get(Def,C,mem,*Length);
            } else {
               *Length=VertexVal(Def,C,X,Y,Z);
//            Def_Pointer(Def,0,mem,p0);
//            x=X+=1.0;y=Y+=1.0;
//                c_gdxysval(GRef->Ids[GRef->NId],&valf,p0,&x,&y,1);
//               fprintf(stderr,"----- %.10e %.10e   ---> %.10e\n",*Length,valf,*Length-valf);
            }
            if (ThetaXY)
               *ThetaXY=0.0;
         }
      }
      return(TRUE);
   }
#endif
   return(FALSE);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RPNProject>
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
int GeoRef_RPNProject(TGeoRef *GRef,double X,double Y,double *Lat,double *Lon,int Extrap,int Transform) {

   float i,j,lat=-999.0,lon=-999.0;
   int   idx;
   double d,dx,dy;
   int    sx,sy,s;

#ifdef HAVE_RMN
   if (X<(GRef->X0-0.5) || Y<(GRef->Y0-0.5) || X>(GRef->X1+0.5) || Y>(GRef->Y1+0.5)) {
      if (!Extrap) {
         *Lat=-999.0;
         *Lon=-999.0;
         return(0);
      }
   }

   if (GRef->Type&GRID_SPARSE) {
      if (GRef->AX && GRef->AY) {
         if (GRef->Grid[0]=='Y') {
            idx=Y*(GRef->X1-GRef->X0)+X;
            Y=GRef->AY[idx];
            X=GRef->AX[idx];
         } else {
            dx=Vertex_ValS(GRef->AX,NULL,GRef->NX,GRef->NY,X,Y,TRUE);
            dy=Vertex_ValS(GRef->AY,NULL,GRef->NX,GRef->NY,X,Y,FALSE);
            
            X=dx;
            Y=dy;
         }
      } else {
         return(0);
      }
   }
   
   if (!GRef->Ids || GRef->Type&GRID_SPARSE) {
      *Lat=Y;
      *Lon=X;
      return(1);
   }

   i=X+1.0;
   j=Y+1.0;

//   RPN_IntLock();
   c_gdllfxy(GRef->Ids[(GRef->NId==0&&GRef->Grid[0]=='U'?1:GRef->NId)],&lat,&lon,&i,&j,1);
//   RPN_IntUnlock();
#endif
   
   *Lat=lat;
   *Lon=lon>180?lon-=360:lon;

   return(1);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RPNUnProject>
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
int GeoRef_RPNUnProject(TGeoRef *GRef,double *X,double *Y,double Lat,double Lon,int Extrap,int Transform) {

   float   i,j,lat,lon;
   int     x,y,n,nd,dx,dy,idx,idxs[8];
   double  dists[8];
   Vect2d  pts[4],pt;
   Vect3d  b;
   TQTree *node;

   *X=-1.0;
   *Y=-1.0;

   // Invalid coordinates ?
   if (Lat>90.0 || Lat<-90.0 || Lon==-999.0) 
     return(FALSE);

   Lon=GeoRef_Lon(GRef,Lon);
   
#ifdef HAVE_RMN
   if (GRef->Type&GRID_SPARSE) {      
      if (GRef->AX && GRef->AY) {
         if (GRef->Grid[0]=='M') {
 
            if (GRef->QTree) {
               // If there's an index use it
               if ((node=QTree_Find(GRef->QTree,Lon,Lat)) && node->NbData) {
                  
                  // Loop on this nodes data payload
                  for(n=0;n<node->NbData;n++) {
                     idx=(int)node->Data[n].Ptr-1; // Remove false pointer increment

                     if (Bary_Get(b,GRef->Wght?GRef->Wght[idx/3]:0.0,Lon,Lat,GRef->AX[GRef->Idx[idx]],GRef->AY[GRef->Idx[idx]],
                        GRef->AX[GRef->Idx[idx+1]],GRef->AY[GRef->Idx[idx+1]],GRef->AX[GRef->Idx[idx+2]],GRef->AY[GRef->Idx[idx+2]])) {
                        
                        // Return coordinate as triangle index + barycentric coefficient
                        *X=idx+b[0];
                        *Y=idx+b[1];
                        return(TRUE);
                     }
                  }
               }
            } else {
               // Otherwise loop on all
               for(idx=0;idx<GRef->NIdx-3;idx+=3) {
                  if (Bary_Get(b,GRef->Wght?GRef->Wght[idx/3]:0.0,Lon,Lat,GRef->AX[GRef->Idx[idx]],GRef->AY[GRef->Idx[idx]],
                     GRef->AX[GRef->Idx[idx+1]],GRef->AY[GRef->Idx[idx+1]],GRef->AX[GRef->Idx[idx+2]],GRef->AY[GRef->Idx[idx+2]])) {

                     // Return coordinate as triangle index + barycentric coefficient
                     *X=idx+b[0];
                     *Y=idx+b[1];
                     return(TRUE);
                  }
               }
            }            
         } else if (GRef->Grid[0]=='Y') {
            // Get nearest point
            if (GeoRef_Nearest(GRef,Lon,Lat,&idx,dists,1)) {
               if (dists[0]<1.0) {
                  *Y=(int)(idx/GRef->NX);
                  *X=idx-(*Y)*GRef->NX;
                  return(TRUE);
               }         
            }
         } else if (GRef->Grid[0]=='X') {
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
//                  fprintf(stderr,"nananan %f %f----- %f %f %i\n",Lat,Lon, *X,*Y,idx);
                     *X=-1,0;
                     *Y=-1.0;
                     return(FALSE);
                  }
               } else {
//                 fprintf(stderr,"-11111 %f %f----- %i\n",Lat,Lon,idx);
               }

               // Si on est a l'interieur de la grille
               if (*X>(GRef->X1+0.5) || *Y>(GRef->Y1+0.5) || *X<(GRef->X0-0.5) || *Y<(GRef->Y0-0.5)) {
                  *X=-1.0;
                  *Y=-1.0;
                  return(FALSE);
               }
               return(TRUE);
            } 
         }
      } 
      return(FALSE);
   }

   if (!GRef->Ids) {
      *Y=Lat;
      *X=Lon;
      return(TRUE);
   } else {

      lon=Lon;
      lat=Lat;

      // Extraire la valeur du point de grille
//      RPN_IntLock();
      c_gdxyfll(GRef->Ids[GRef->NId],&i,&j,&lat,&lon,1);
//      RPN_IntUnlock();

      *X=i-1.0;
      *Y=j-1.0;

      // Fix for G grid 0-360 1/5 gridpoint problem
      if (GRef->Grid[0]=='G' && *X>GRef->X1+0.5) *X-=(GRef->X1+1);

      // Si on est a l'interieur de la grille
      if (*X>(GRef->X1+0.5) || *Y>(GRef->Y1+0.5) || *X<(GRef->X0-0.5) || *Y<(GRef->Y0-0.5)) {
         if (!Extrap) {
            *X=-1.0;
            *Y=-1.0;
         }
         return(FALSE);
      }
   }
#endif
   return(TRUE);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RPNSetup>
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
 *    <FID>     : Identificateur du fichier
 *
 * Retour       :
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
TGeoRef* GeoRef_RPNSetup(int NI,int NJ,char *GRTYP,int IG1,int IG2,int IG3,int IG4,int FID) {

   TGeoRef *ref;
   int      id;
   char     grtyp[2];

   ref=GeoRef_New();
   GeoRef_Size(ref,0,0,NI-1,NJ-1,0);

   // If not specified, type is X
   if (GRTYP[0]==' ') GRTYP[0]='X';

   if ((NI>1 || NJ>1) && GRTYP[0]!='X' && GRTYP[0]!='P' && GRTYP[0]!='M' && GRTYP[0]!='V' && ((GRTYP[0]!='Z' && GRTYP[0]!='Y') || FID!=-1)) {
      grtyp[0]=GRTYP[0];
      grtyp[1]='\0';

#ifdef HAVE_RMN
      // Create master gridid
      if (GRTYP[1]=='#') {
         // For tiled grids (#) we have to fudge the IG3 ang IG4 to 0 since they're used for tile limit
         id=RPN_IntIdNew(NI,NJ,grtyp,IG1,IG2,0,0,FID);
      } else {
         id=RPN_IntIdNew(NI,NJ,grtyp,IG1,IG2,IG3,IG4,FID);
      }
      // Check for sub-grids (U grids can have sub grids)
      ref->NbId=GRTYP[0]=='U'?c_ezget_nsubgrids(id):1;
//      ref->NbId=1;
      if ((ref->Ids=(int*)malloc((ref->NbId>1?ref->NbId+1:1)*sizeof(int)))) {
         ref->Ids[0]=id;
         if (ref->NbId>1) {
            c_ezget_subgridids(id,&ref->Ids[1]);
         }
      }
#endif
   }

   ref->IG1=IG1;
   ref->IG2=IG2;
   ref->IG3=IG3;
   ref->IG4=IG4;
   ref->Grid[0]=GRTYP[0];
   ref->Grid[1]=GRTYP[1];
   ref->Project=GeoRef_RPNProject;
   ref->UnProject=GeoRef_RPNUnProject;
   ref->Value=(TGeoRef_Value*)GeoRef_RPNValue;
   ref->Distance=GeoRef_RPNDistance;
   ref->Height=NULL;

   return(ref);
}

int GEM_grid_param(int *F_bsc_base,int *F_bsc_ext1,int *F_extension ,int F_maxcfl,float *F_lonr,float *F_latr,int *F_ni,int *F_nj,float *F_dx,float *F_dy,double *F_x0_8,double *F_y0_8,double *F_xl_8,double *F_yl_8,int F_overlap,int F_yinyang_L) {

   double delta_8;
   int iref,jref;
  
   // basic global lateral boundary conditions width
   *F_bsc_base = 5;
   if (F_yinyang_L) *F_bsc_base=*F_bsc_base+1;

   // added points for proper de-staggering of u,v at physics interface
   *F_bsc_ext1 = 2;

   // total extension to user specified grid configuration
   *F_extension= F_maxcfl + *F_bsc_base + *F_bsc_ext1;

   if (F_yinyang_L) {

      *F_x0_8 =   45.0 - 3.0*F_overlap;
      *F_xl_8 =  315.0 + 3.0*F_overlap;
      *F_y0_8 = -45.0  -     F_overlap;
      *F_yl_8 =  45.0  +     F_overlap;
      
      delta_8  = ((*F_xl_8)-(*F_x0_8))/(*F_ni-1);
      *F_dx   = delta_8;
      *F_x0_8 = *F_x0_8 - (*F_extension)*delta_8;
      *F_xl_8 = *F_xl_8 + (*F_extension)*delta_8;
      
      delta_8  = ((*F_yl_8)-(*F_y0_8))/(*F_nj-1);
      *F_dy   = delta_8;
      *F_y0_8 = *F_y0_8 - (*F_extension)*delta_8;
      *F_yl_8 = *F_yl_8 + (*F_extension)*delta_8;
      
      *F_ni   = *F_ni + 2  *(*F_extension);
      *F_nj   = *F_nj + 2  *(*F_extension);

   } else {

      iref = *F_ni / 2 + (*F_extension);
      if ((*F_ni)%2==0) {
         *F_lonr = *F_lonr - (*F_dx)/2.0;
      } else {
         iref = iref + 1;
      }
      jref = *F_nj / 2 + (*F_extension);
      if ((*F_nj)%2==0) {
         *F_latr = *F_latr - (*F_dy)/2.0;
      } else {
         jref = *F_nj / 2 + (*F_extension) + 1;
      }
      
      *F_ni   = *F_ni + 2*(*F_extension);
      *F_nj   = *F_nj + 2*(*F_extension);
      *F_x0_8 = *F_lonr - (iref-1) * (*F_dx);
      *F_y0_8 = *F_latr - (jref-1) * (*F_dy);
      *F_xl_8 = *F_x0_8 + (*F_ni  -1) * (*F_dx);
      *F_yl_8 = *F_y0_8 + (*F_nj  -1) * (*F_dy);
      if (*F_x0_8 < 0.) *F_x0_8=*F_x0_8+360.0;
      if (*F_xl_8 < 0.) *F_xl_8=*F_xl_8+360.0;

      if (*F_x0_8 < 0.) {
         fprintf(stderr,"Longitude of WEST %f < 0.0\n",*F_x0_8);
         return(0);
      }
      if (*F_y0_8 < -90.) {
         fprintf(stderr,"Latitude of SOUTH %f < 0.0\n",*F_y0_8);
         return(0);
      }
      if (*F_xl_8 > 360.) {
         fprintf(stderr,"Longitude of EAST %f < 0.0\n",*F_xl_8);
         return(0);
      }
      if (*F_yl_8 > 90.) {
         fprintf(stderr,"Latitude of NORTH %f < 0.0\n",*F_yl_8);
         return(0);
      }
   }
}

void GEM_hgrid4(float *F_xgi_8,float *F_ygi_8,int F_Grd_ni,int F_Grd_nj,float *F_Grd_dx,float *F_Grd_dy,double F_Grd_x0_8,double F_Grd_xl_8,double F_Grd_y0_8,double F_Grd_yl_8, int F_Grd_yinyang_L){

   int i;
   double delta_8;

   delta_8 = (F_Grd_xl_8-F_Grd_x0_8)/(F_Grd_ni-1);
   F_xgi_8[0] = F_Grd_x0_8;
   F_xgi_8[F_Grd_ni-1] = F_Grd_xl_8;
   for(i=1;i<F_Grd_ni-1;i++) F_xgi_8[i]= F_Grd_x0_8 + i*delta_8;

   delta_8 = (F_Grd_yl_8-F_Grd_y0_8)/(F_Grd_nj-1);
   F_ygi_8[0] = F_Grd_y0_8;
   F_ygi_8[F_Grd_nj-1] = F_Grd_yl_8;
   for(i=1;i<F_Grd_nj-1;i++) F_ygi_8[i]= F_Grd_y0_8 + i*delta_8;

   if (F_Grd_yinyang_L) {
      *F_Grd_dx   = fabs(F_xgi_8[1]-F_xgi_8[0]);
      *F_Grd_dy   = fabs(F_ygi_8[1]-F_ygi_8[0]);
   }
}

/*-------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RPNGridZE>
 * Creation     : Avril 2005 J.P. Gauthier - CMC/CMOE
 *
 * But          : Definir le referentiel de type RPN ZE
 *
 * Parametres   :
 *    <GRef>    : Georef definition
 *    <NI>      : Dimension en X
 *    <NJ>      : Dimension en Y
 *    <DX>      : Resolution en X
 *    <DY>      : Resolution en Y
 *    <LatR>    : 
 *    <LonR>    : 
 *    <MaxCFL>  : 
 *    <XLat1>   : Latitude centrale
 *    <XLon1>   : Longitude centrale
 *    <XLat2>   : Latitude de l'axe de rotation
 *    <XLon2>   : Longitude de l'axe de rotation
 *
 * Retour       :
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
TGeoRef* GeoRef_RPNGridZE(TGeoRef *GRef,int NI,int NJ,float DX,float DY,float LatR,float LonR,int MaxCFL,float XLat1,float XLon1,float XLat2,float XLon2) {

   int    ig1,ig2,ig3,ig4;
   char   gxtyp='E';
   int    bsc_base,bsc_ext1,extension,err;
   int    maxcfl;
   double x0,x1,y0,y1;
   float latr,lonr;

   if (!GRef) {
      return(NULL);
   }
 
   f77name(cxgaig)("E",&GRef->IG1,&GRef->IG2,&GRef->IG3,&GRef->IG4,&XLat1,&XLon1,&XLat2,&XLon2);
   f77name(cigaxg)("E",&XLat1,&XLon1,&XLat2,&XLon2,&GRef->IG1,&GRef->IG2,&GRef->IG3,&GRef->IG4);
   
   GEM_grid_param(&bsc_base,&bsc_ext1,&extension,MaxCFL,&LonR,&LatR,&NI,&NJ,&DX,&DY,&x0,&y0,&x1,&y1,-1,FALSE);
 
   if (NI!=GRef->NX+1 || NJ!=GRef->NY+1) {
      GRef->AX=realloc(GRef->AX,NI*sizeof(float));
      GRef->AY=realloc(GRef->AY,NJ*sizeof(float));

      GeoRef_Size(GRef,0,0,NI-1,NJ-1,0);
   }

   //   f77name(set_gemhgrid4)(GRef->AX,GRef->AY,&NI,&NJ,&DX,&DY,&x0,&x1,&y0,&y1,FALSE);
   GEM_hgrid4(GRef->AX,GRef->AY,NI,NJ,&DX,&DY,x0,x1,y0,y1,FALSE);
        
   if (!GRef->Ids && !(GRef->Ids=(int*)malloc(sizeof(int)))) {
      return(NULL);
   }
   
   RPN_IntLock();
   GRef->Ids[0]=c_ezgdef_fmem(NI,NJ,"Z","E",GRef->IG1,GRef->IG2,GRef->IG3,GRef->IG4,GRef->AX,GRef->AY);
   RPN_IntUnlock();
   
   GRef->NbId=1;
   GRef->Grid[0]='Z';
   GRef->Grid[1]='E';
   GRef->Grid[2]='\0';
   GRef->Project=GeoRef_RPNProject;
   GRef->UnProject=GeoRef_RPNUnProject;
   GRef->Value=(TGeoRef_Value*)GeoRef_RPNValue;
   GRef->Distance=GeoRef_RPNDistance;
   GRef->Height=NULL;
  
   return(GRef);
}
