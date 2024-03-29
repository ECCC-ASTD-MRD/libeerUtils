/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Lecture et traitements de fichiers raster
 * Fichier      : GeoRef_RDR.c
 * Creation     : Mars 2005 - J.P. Gauthier
 *
 * Description  : Fonctions de manipulations de projections aux standard RADAR
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

double   GeoRef_RDRHeight(TGeoRef *GRef,TZRef *ZRef,double Azimuth,double Bin,double Sweep);
double   GeoRef_RDRDistance(TGeoRef *GRef,double X0,double Y0,double X1, double Y1);
int      GeoRef_RDRValue(TGeoRef *GRef,TDef *Def,char Mode,int C,double Azimuth,double Bin,double Sweep,double *Length,double *ThetaXY);
int      GeoRef_RDRProject(TGeoRef *GRef,double X,double Y,double *Lat,double *Lon,int Extrap,int Transform);
int      GeoRef_RDRUnProject(TGeoRef *GRef,double *X,double *Y,double Lat,double Lon,int Extrap,int Transform);
TGeoRef* GeoRef_RDRSetup(double Lat,double Lon,double Height,int R,double ResR,double ResA);

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RDRHeight>
 * Creation     : Decembre 2009 J.P. Gauthier - CMC/CMOE
 *
 * But          : Calculer la hauteur en MAGL d<une coordonnee RADAR.
 *
 * Parametres    :
 *   <GRef>      : Pointeur sur la reference geographique
 *   <ZRef>      : Pointeur sur la reference verticale
 *   <Azimuth>   : coordonnee en X dans la projection/grille
 *   <Bin>       : coordonnee en Y dans la projection/grille
 *   <Sweep>     : coordonnee en Z dans la projection/grille
 *
 * Retour       : Hauteur
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
double GeoRef_RDRHeight(TGeoRef *GRef,TZRef *ZRef,double Azimuth,double Bin,double Sweep) {

   if (Bin>=0 && Bin<GRef->R && Sweep>=0 && Sweep<ZRef->LevelNb) {
      return(GRef->Loc.Elev+sin(DEG2RAD(ZRef->Levels[(int)Sweep]))*((int)Bin*GRef->ResR));
   } else {
      return(0);
   }
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RDRDistance>
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
double GeoRef_RDRDistance(TGeoRef *GRef,double X0,double Y0,double X1, double Y1) {

   return(0.0);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RDRValue>
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
int GeoRef_RDRValue(TGeoRef *GRef,TDef *Def,char Mode,int C,double Azimuth,double Bin,double Sweep,double *Length,double *ThetaXY){

   int      valid=0,mem,ix,iy;

   *Length=Def->NoData;

   /*Si on est a l'interieur de la grille ou que l'extrapolation est activee*/
   if (C<Def->NC && Bin<=GRef->R) {

      mem=Def->NIJ*(int)Sweep;

      ix=ROUND(Azimuth);
      iy=ROUND(Bin);

      mem+=iy*Def->NI+ix;

      if (Def->Type<=9 || Mode=='N' || (Azimuth==ix && Bin==iy)) {
         Def_Get(Def,C,mem,*Length);
      } else {
         *Length=VertexVal(Def,C,Azimuth,Bin,Sweep);
      }
      valid=mem+1;
   }
   return(valid);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RDRProject>
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
int GeoRef_RDRProject(TGeoRef *GRef,double X,double Y,double *Lat,double *Lon,int Extrap,int Transform) {

   Coord loc0;
   double x,d;

   if (Y>GRef->R && !Extrap) {
      *Lat=-999.0;
      *Lon=-999.0;
      return(0);
   }

   loc0.Lat=DEG2RAD(GRef->Loc.Lat);
   loc0.Lon=DEG2RAD(GRef->Loc.Lon);

   X*=GRef->ResA;
   Y*=GRef->ResR;

   x=DEG2RAD(X);
   d=M2RAD(Y*GRef->CTH);

   if (Transform) {
      *Lat=asin(sin(loc0.Lat)*cos(d)+cos(loc0.Lat)*sin(d)*cos(x));
      *Lon=fmod(loc0.Lon+(atan2(sin(x)*sin(d)*cos(loc0.Lat),cos(d)-sin(loc0.Lat)*sin(*Lat)))+M_PI,M_2PI)-M_PI;
      *Lat=RAD2DEG(*Lat);
      *Lon=RAD2DEG(*Lon);
   } else {
      *Lat=d;
      *Lon=x;
   }

   return(1);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RDRUnProject>
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
int GeoRef_RDRUnProject(TGeoRef *GRef,double *X,double *Y,double Lat,double Lon,int Extrap,int Transform) {

   Coord loc0;
   double x,d;

   loc0.Lat=DEG2RAD(GRef->Loc.Lat);
   loc0.Lon=DEG2RAD(GRef->Loc.Lon);
   Lat=DEG2RAD(Lat);
   Lon=DEG2RAD(Lon);

   d=fabs(DIST(0.0,loc0.Lat,loc0.Lon,Lat,Lon));
   x=-RAD2DEG(COURSE(loc0.Lat,loc0.Lon,Lat,Lon));
   *X=x<0.0?x+360.0:x;
   *Y=d/GRef->CTH;

   if (Transform) {
      *X/=GRef->ResA;
      *Y/=GRef->ResR;
   }

   if (*Y>GRef->Y1) {
      if (!Extrap) {
         *X=-1.0;
         *Y=-1.0;
      }
      return(0);
   }
   return(1);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_RDRSetup>
 * Creation     : Avril 2006 J.P. Gauthier - CMC/CMOE
 *
 * But          : Definir le referetiel de type Radar
 *
 * Parametres   :
 *    <Lat>     : Latitude du centre
 *    <Lon>     : Longitude du centre
 *    <Height>  : Altitude du centre
 *    <NBin>    : Nombre de bin
 *    <ResR>    : Resolution en distance
 *    <ResA>    : Resolution en azimuth
 *
 * Retour       :
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
TGeoRef* GeoRef_RDRSetup(double Lat,double Lon,double Height,int R,double ResR,double ResA) {

   TGeoRef *ref;

   ref=GeoRef_New();

   ref->Grid[0]='R';
   ref->Loc.Lat=Lat;
   ref->Loc.Lon=Lon;
   ref->Loc.Elev=Height;
   ref->R=R;
   ref->ResR=ResR;
   ref->ResA=ResA;
   ref->CTH=cos(0.0);
   ref->STH=sin(0.0);

   GeoRef_Size(ref,0,0,360/ResA,R-1,0);

   ref->Project=GeoRef_RDRProject;
   ref->UnProject=GeoRef_RDRUnProject;
   ref->Value=(TGeoRef_Value*)GeoRef_RDRValue;
   ref->Distance=GeoRef_RDRDistance;
   ref->Height=GeoRef_RDRHeight;

   return(ref);
}
