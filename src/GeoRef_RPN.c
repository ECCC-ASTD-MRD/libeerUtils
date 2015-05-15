/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Lecture et traitements de fichiers raster
 * Fichier      : GeoRef_RPN.c
 * Creation     : Mars 2005 - J.P. Gauthier
 * Revision     : $Id$
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

#include "GeoRef.h"
#include "Def.h"
#include "RPN.h"
#include "Vertex.h"
#include "EZTile.h"

void     GeoRef_Expand(TGeoRef *Ref);
double   GeoRef_RPNDistance(TGeoRef *Ref,double X0,double Y0,double X1, double Y1);
int      GeoRef_RPNValue(TGeoRef *Ref,TDef *Def,char Mode,int C,double X,double Y,double Z,double *Length,double *ThetaXY);
int      GeoRef_RPNProject(TGeoRef *Ref,double X,double Y,double *Lat,double *Lon,int Extrap,int Transform);
int      GeoRef_RPNUnProject(TGeoRef *Ref,double *X,double *Y,double Lat,double Lon,int Extrap,int Transform);

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_Expand>
 * Creation     : Mars 2005 J.P. Gauthier - CMC/CMOE
 *
 * But          : Effectuer l'expansion des axes de la grilles selon les >> ^^.
 *
 * Parametres    :
 *   <Ref>       : Pointeur sur la reference geographique
 *
 * Retour       :
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
void GeoRef_Expand(TGeoRef *Ref) {

#ifdef HAVE_RMN
   double lat,lon;
   int    i;

   if (Ref->Ids && !Ref->AX) {
      Ref->AX=(float*)calloc((int)Ref->X1+1,sizeof(float));
      Ref->AY=(float*)calloc((int)Ref->Y1+1,sizeof(float));

      if (Ref->AX && Ref->AY) {
         if (Ref->Grid[0]=='Z') {
//            RPN_IntLock();
            c_gdgaxes(Ref->Ids[Ref->NId],Ref->AX,Ref->AY);
//            RPN_IntUnlock();
         } else {
            for(i=0;i<=Ref->X1;i++) {
               Ref->Project(Ref,i,0,&lat,&lon,1,1);
               Ref->AX[i]=lon;
            }
            for(i=0;i<=Ref->Y1;i++) {
               Ref->Project(Ref,0,i,&lat,&lon,1,1);
               Ref->AY[i]=lat;
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
 *   <Ref>       : Pointeur sur la reference geographique
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
double GeoRef_RPNDistance(TGeoRef *Ref,double X0,double Y0,double X1, double Y1) {

#ifdef HAVE_RMN
   float i[2],j[2],lat[2],lon[2];

   if (Ref->Ids) {
      i[0]=X0+1.0;
      j[0]=Y0+1.0;
      i[1]=X1+1.0;
      j[1]=Y1+1.0;

//      RPN_IntLock();
      c_gdllfxy(Ref->Ids[Ref->NId],lat,lon,i,j,2);
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
 *   <Ref>       : Pointeur sur la reference geographique
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

int GeoRef_RPNValue(TGeoRef *Ref,TDef *Def,char Mode,int C,double X,double Y,double Z,double *Length,double *ThetaXY) {

   Vect3d       b,v;
   float        x,y,valf,valdf;
   void        *p0,*p1;
   int          valid=0,mem,ix,iy,n;
   unsigned int idx;

   *Length=Def->NoData;

#ifdef HAVE_RMN
   /*In case of triangle meshe*/
   if (Ref->Grid[0]=='M') {
      if (C<Def->NC && X>=0 && Y>=0) {
         b[0]=X-(int)X;
         b[1]=Y-(int)Y;
         b[2]=1.0-b[0]-b[1];
         ix=(int)X;

         if (Mode=='N') {
            n=(b[0]>b[1]?(b[0]>b[2]?0:2):(b[1]>b[2]?1:2));
            Def_Get(Def,C,Ref->Idx[ix+n],v[0]);
            *Length=v[0];
         } else {
            Def_Get(Def,C,Ref->Idx[ix],v[0]);
            Def_Get(Def,C,Ref->Idx[ix+1],v[1]);
            Def_Get(Def,C,Ref->Idx[ix+2],v[2]);

            *Length=Bary_Interp1D(b,v);
         }
         return(1);
      } else {
         return(0);
      }
   }

   /*Si on est a l'interieur de la grille ou que l'extrapolation est activee*/
   if (C<Def->NC && X>=(Ref->X0-0.5) && Y>=(Ref->Y0-0.5) && Z>=0 && X<(Ref->X1+0.5) && Y<(Ref->Y1+0.5) && Z<=Def->NK-1) {

      /*Index memoire du niveau desire*/
      mem=Def->NIJ*(int)Z;

      x=X+1.0;
      y=Y+1.0;

      ix=lrint(X);
      iy=lrint(Y);
      idx=iy*Def->NI+ix;

      // Check for mask
      if (Def->Mask && !Def->Mask[idx]) {
         return(valid);
      }

      valid=1;

      if (Ref && Ref->Grid[0]=='V') {
         if (Def->Data[1]) {
            Def_GetMod(Def,FIDX2D(Def,ix,iy),*Length);
         } else {
            *Length=VertexVal(Ref,Def,-1,X,Y,0.0);
         }
         return(valid);
      }

      if (Ref->Grid[0]!='X' && Ref->Grid[0]!='Y' && Ref->Grid[0]!='P' && Def->Data[1] && !C) {
         if (Ref && Ref->Ids) {
            Def_Pointer(Def,0,mem,p0);
            Def_Pointer(Def,1,mem,p1);
//            RPN_IntLock();
            c_gdxywdval(Ref->Ids[Ref->NId],&valf,&valdf,p0,p1,&x,&y,1);

            /*If it's 3D, use the mode for speed since c_gdxywdval only uses 2D*/
            if (Def->Data[2])
               c_gdxysval(Ref->Ids[Ref->NId],&valf,(float*)&Def->Mode[mem],&x,&y,1);
            *Length=valf;
            if (ThetaXY)
               *ThetaXY=valdf;
//            RPN_IntUnlock();
         }
      } else {
         if (Ref->Grid[0]=='Y' || Ref->Grid[0]=='P') {
            X=ix;
            Y=iy;
         }

         // G grids have something weire which measn we have to use ezscint
         if ((Def->Type<TD_Float32 || Mode=='N' || (X==ix && Y==iy)) && Ref->Grid[0]!='G') {
            mem+=idx;
            Def_Get(Def,C,mem,*Length);
            if (Def->Data[1] && ThetaXY && !C)
               Def_Get(Def,1,mem,*ThetaXY);
         } else {
            if (Def->Type==TD_Float32 && Ref && Ref->Ids) {
               Def_Pointer(Def,C,mem,p0);

               // If either value is nodata then interpolation will be nodata as well
               ix=trunc(X);
               iy=trunc(Y);
               mem=idx;
               if (              ((float*)p0)[mem]==Def->NoData)                         { return(valid); }
               if (ix<Ref->X1 && ((float*)p0)[mem+1]==Def->NoData)                       { return(valid); }
               if (iy<Ref->Y1 && ((float*)p0)[mem+Def->NI]==Def->NoData)                 { return(valid); }
               if (iy<Ref->Y1 && ix<Ref->X1 && ((float*)p0)[mem+Def->NI+1]==Def->NoData) { return(valid); }

//               RPN_IntLock();
               c_gdxysval(Ref->Ids[Ref->NId],&valf,p0,&x,&y,1);
               *Length=valf;
//               RPN_IntUnlock();
            } else {
               *Length=VertexVal(Ref,Def,C,X,Y,Z);
            }
         }
      }
   }
#endif
   return(valid);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RPNProject>
 * Creation     : Mars 2005 J.P. Gauthier - CMC/CMOE
 *
 * But          : Projeter une coordonnee de projection en latlon.
 *
 * Parametres    :
 *   <Ref>       : Pointeur sur la reference geographique
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
int GeoRef_RPNProject(TGeoRef *Ref,double X,double Y,double *Lat,double *Lon,int Extrap,int Transform) {

   float i,j,lat,lon;
   int   idx;

   if (X<(Ref->X0-0.5) || Y<(Ref->Y0-0.5) || X>(Ref->X1+0.5) || Y>(Ref->Y1+0.5)) {
      if (!Extrap) {
         *Lat=-999.0;
         *Lon=-999.0;
         return(0);
      }
   }

   if (!Ref->Ids) {
      *Lat=Y;
      *Lon=X;
      return(1);
   }

   if (Ref->Type&GRID_SPARSE) {
      if (Ref->Lon && Ref->Lat) {
         idx=Y*(Ref->X1-Ref->X0)+X;
         *Lat=Ref->Lat[idx];
         *Lon=Ref->Lon[idx];
         return(1);
      } else {
         return(0);
      }
   }

   i=X+1.0;
   j=Y+1.0;

//   RPN_IntLock();
   c_gdllfxy(Ref->Ids[(Ref->NId==0&&Ref->Grid[0]=='U'?1:Ref->NId)],&lat,&lon,&i,&j,1);
//   RPN_IntUnlock();

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
 *   <Ref>       : Pointeur sur la reference geographique
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
int GeoRef_RPNUnProject(TGeoRef *Ref,double *X,double *Y,double Lat,double Lon,int Extrap,int Transform) {

   float  i,j,lat,lon,d=1e32,dx=1.0;
   int    n,di,dj,idx,ni,nj;
   Vect3d b;

   *X=-1.0;
   *Y=-1.0;

#ifdef HAVE_RMN
   if (Ref->Type&GRID_SPARSE) {
      if (Ref->Lon && Ref->Lat) {
         if (Ref->Grid[0]=='M') {
            for(n=0;n<Ref->NIdx-3;n+=3) {
               if (Bary_Get(b,Lon,Lat,Ref->Lon[Ref->Idx[n]],Ref->Lat[Ref->Idx[n]],
                  Ref->Lon[Ref->Idx[n+1]],Ref->Lat[Ref->Idx[n+1]],Ref->Lon[Ref->Idx[n+2]],Ref->Lat[Ref->Idx[n+2]])) {

                  *X=n+b[0];
                  *Y=n+b[1];
                  return(1);
               }
            }
            return(0);
         } else {
            ni=Ref->X1-Ref->X0;
            nj=Ref->Y1-Ref->Y0;
            idx=0;

            for(dj=0;dj<=nj;dj++) {
               for(di=0;di<=ni;di++) {

                  dx=hypot(Lon-Ref->Lon[idx],Lat-Ref->Lat[idx]);
                  idx++;

                  if (dx<0.1 && dx<d) {
                     *X=di;*Y=dj;d=dx;
                  }
               }
            }

            if (d<1.0) {
               return(1);
            } else {
               return(0);
            }
         }
      } else {
         return(0);
      }
   }

   if (!Ref->Ids) {
      *Y=Lat;
      *X=Lon;
      return(1);
   } else if (Lat<=90.0 && Lat>=-90.0 && Lon!=-999.0) {

//      lon=Lon<0?Lon+360.0:Lon;
      lon=Lon;
      lat=Lat;

      /*Extraire la valeur du point de grille*/
//      RPN_IntLock();
      c_gdxyfll(Ref->Ids[Ref->NId],&i,&j,&lat,&lon,1);
//      RPN_IntUnlock();

      *X=i-1.0;
      *Y=j-1.0;

      // Fix for G grid 0-360 1/5 gridpoint problem
      if (Ref->Grid[0]=='G' && *X>Ref->X1+0.5) *X-=(Ref->X1+1);

      /*Si on est a l'interieur de la grille*/
      if (*X>(Ref->X1+0.5) || *Y>(Ref->Y1+0.5) || *X<(Ref->X0-0.5) || *Y<(Ref->Y0-0.5)) {
         if (!Extrap) {
            *X=-1.0;
            *Y=-1.0;
         }
         return(0);
      }
   } else {
     *X=-1.0;
     *Y=-1.0;
     return(0);
   }
#endif
   return(1);
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
 *    <NK>      : Diemnsion en Z
 *    <Type>    : Type de niveaux
 *    <Levels>  : Liste des niveaux
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
TGeoRef* GeoRef_RPNSetup(int NI,int NJ,int NK,int Type,float *Levels,char *GRTYP,int IG1,int IG2,int IG3,int IG4,int FID) {

   TGeoRef *ref;
   int      id;
   char     grtyp[2];

   ref=GeoRef_New();
   GeoRef_Size(ref,0,0,0,NI-1,NJ-1,NK-1,0);

   // If not specified, type is X
   if (GRTYP[0]==' ') GRTYP[0]='X';

   if ((NI>1 || NJ>1) && GRTYP[0]!='X' && GRTYP[0]!='P' && GRTYP[0]!='M' && GRTYP[0]!='V' && ((GRTYP[0]!='Z' && GRTYP[0]!='Y') || FID!=-1)) {
      grtyp[0]=GRTYP[0];
      grtyp[1]='\0';

      // Create master gridid
      if (GRTYP[1]=='#') {
         // For tiled grids (#) we have to fudge the IG3 ang IG4 to 0 since they're used for tile limit
         id=RPN_IntIdNew(NI,NJ,grtyp,IG1,IG2,0,0,FID);
      } else {
         id=RPN_IntIdNew(NI,NJ,grtyp,IG1,IG2,IG3,IG4,FID);
      }
#ifdef HAVE_RMN
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
   ref->ZRef.Type=Type;
   ref->ZRef.LevelNb=NK;
   ref->ZRef.Levels=(float*)calloc(ref->ZRef.LevelNb+1,sizeof(float));
   if (Levels && ref->ZRef.Levels)
      memcpy(ref->ZRef.Levels,Levels,ref->ZRef.LevelNb*sizeof(float));

   ref->Grid[0]=GRTYP[0];
   ref->Grid[1]=GRTYP[1];
   ref->Project=GeoRef_RPNProject;
   ref->UnProject=GeoRef_RPNUnProject;
   ref->Value=(TGeoRef_Value*)GeoRef_RPNValue;
   ref->Distance=GeoRef_RPNDistance;
   ref->Height=NULL;

//TODO:Dont't forget ot add the GeoRef_Find to SPI's call
   return(ref);
}