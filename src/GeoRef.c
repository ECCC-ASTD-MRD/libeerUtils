/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Lecture et traitements de fichiers raster
 * Fichier      : GeoRef.c
 * Creation     : Mars 2005 - J.P. Gauthier
 * Revision     : $Id$
 *
 * Description  : Fonctions de manipulations de projections.
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
#include "App.h"
#include "Def.h"
#include "RPN.h"

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoScan_Clear>
 * Creation     : Fevrier 2008 J.P. Gauthier - CMC/CMOE
 *
 * But          : Reinitialiser le buffer de reprojection
 *
 * Parametres   :
 *  <Scan>      : Buffer de reprojection
 *
 * Retour       :
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
void GeoScan_Clear(TGeoScan *Scan) {

   if (Scan) {
      if (Scan->X) free(Scan->X);
      if (Scan->Y) free(Scan->Y);
      if (Scan->V) free(Scan->V);
      if (Scan->D) free(Scan->D);

      Scan->X=Scan->Y=NULL;
      Scan->V=NULL;
      Scan->D=NULL;
      Scan->N=Scan->S=Scan->DX=Scan->DY=0;
   }
}

void GeoScan_Init(TGeoScan *Scan) {

   if (Scan) {
      Scan->X=Scan->Y=NULL;
      Scan->V=NULL;
      Scan->D=NULL;
      Scan->N=Scan->S=Scan->DX=Scan->DY=0;
   }
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoScan_Get>
 * Creation     : Fevrier 2008 J.P. Gauthier - CMC/CMOE
 *
 * But          : Reprojeter et extraire les valeurs a ces points
 *
 * Parametres   :
 *  <Scan>      : Buffer de reprojection
 *  <ToRef>     : Georeference destination
 *  <ToDef>     : Data definition destination
 *  <FromRef>   : Georeference source
 *  <FromDef>   : Data definition source
 *  <X0>        : Limite inferieure en X
 *  <Y0>        : Limite inferieure en Y
 *  <X1>        : Limite superieure en X
 *  <Y1>        : Limite superieure en Y
 *  <Dim>       : Dimension dee cellules de grilles (1=point, 2=area)
 *  <Degree>    : Interpolation degree
 *  <To>        : Georeference destination
 *  <From>      : Georeference source
 *
 * Retour       : Dimension des resultats
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoScan_Get(TGeoScan *Scan,TGeoRef *ToRef,TDef *ToDef,TGeoRef *FromRef,TDef *FromDef,int X0,int Y0,int X1,int Y1,int Dim,char *Degree) {

   register int idx,x,y,n=0;
   int          d=0,sz,dd;
   double       x0,y0,v;

   if (!Scan || !ToRef || !FromRef) {
      return(0);
   }

   // Check limits
   X0=FMAX(X0,FromRef->X0);
   Y0=FMAX(Y0,FromRef->Y0);
   X1=FMIN(X1,FromRef->X1);
   Y1=FMIN(Y1,FromRef->Y1);

   /*Adjust scan buffer sizes*/
   Scan->DX=X1-X0+1;
   Scan->DY=Y1-Y0+1;
   dd=(Scan->DX+1)*(Scan->DY+1);
   sz=Scan->DX*Scan->DY;

   if (Scan->S<sz) {
      if (!(Scan->X=(double*)realloc(Scan->X,dd*sizeof(double))))
         return(0);
      if (!(Scan->Y=(double*)realloc(Scan->Y,dd*sizeof(double))))
         return(0);
      if (!(Scan->V=(unsigned int*)realloc(Scan->V,sz*sizeof(unsigned int))))
         return(0);
      if (!(Scan->D=(float*)realloc(Scan->D,sz*sizeof(float))))
         return(0);
      Scan->S=sz;
   }

   dd=Dim-1;
   Scan->N=0;

   /*WKT grid type*/
   if (FromRef->Grid[0]=='W') {
#ifdef HAVE_GDAL
      for(y=Y0;y<=Y1+dd;y++) {
         idx=(y-FromRef->Y0)*FromDef->NI+(X0-FromRef->X0);
         for(x=X0;x<=X1+dd;x++,idx++,n++) {
            if (x<=X1 && y<=Y1) {
               Scan->V[Scan->N++]=idx;
            }

            x0=dd?x-0.5:x;
            y0=dd?y-0.5:y;
            if (FromRef->Transform) {
               Scan->X[n]=FromRef->Transform[0]+FromRef->Transform[1]*x0+FromRef->Transform[2]*y0;
               Scan->Y[n]=FromRef->Transform[3]+FromRef->Transform[4]*x0+FromRef->Transform[5]*y0;
            } else {
               Scan->X[n]=x0;
               Scan->Y[n]=y0;
            }
         }
      }

      if (FromRef->Function) {
         OCTTransform(FromRef->Function,n,Scan->X,Scan->Y,NULL);
      }
#endif
      d=dd?2:1;
      sz=8;

   /*Y Grid type*/
   } else if (FromRef->Grid[0]=='Y') {
      for(y=Y0;y<=Y1;y++) {
         idx=(y-FromRef->Y0)*FromDef->NI+(X0-FromRef->X0);
         for(x=X0;x<=X1;x++,idx++,n++) {
            if (x<=X1 && y<=Y1) {
               Scan->V[Scan->N++]=idx;
            }
            ((float*)Scan->X)[n]=FromRef->Lon[idx];
            ((float*)Scan->Y)[n]=FromRef->Lat[idx];
         }
      }
      d=1;
      sz=4;

   /*Other RPN grids*/
   } else {
#ifdef HAVE_RMN
      for(y=Y0;y<=Y1+dd;y++) {
         idx=(y-FromRef->Y0)*FromDef->NI+(X0-FromRef->X0);
         for(x=X0;x<=X1+dd;x++,idx++,n++) {
            if (x<=X1 && y<=Y1) {
               Scan->V[Scan->N++]=idx;
            }
            ((float*)Scan->X)[n]=dd?x+0.5:x+1.0;
            ((float*)Scan->Y)[n]=dd?y+0.5:y+1.0;
         }
      }
//      RPN_IntLock();
      c_gdllfxy(FromRef->Ids[FromRef->NId],(float*)Scan->Y,(float*)Scan->X,(float*)Scan->X,(float*)Scan->Y,n);
//      RPN_IntUnlock();

      d=dd?2:1;
      sz=4;
#else
      App_ErrorSet("RMNLIB support not included.");
#endif
   }

   /*Project to destination grid*/
   if (ToRef->Grid[0]=='W') {
#ifdef HAVE_GDAL
      for(x=n-1;x>=0;x--) {
         if (sz==4) {
            x0=(double)((float*)Scan->X)[x];
            y0=(double)((float*)Scan->Y)[x];
         } else {
            x0=Scan->X[x];
            y0=Scan->Y[x];

         }
         if (ToDef) {
            Scan->D[x]=ToDef->NoData;
         }

         if (ToRef->UnProject(ToRef,&Scan->X[x],&Scan->Y[x],y0,x0,0,1)) {

            if (ToDef) {
              ToRef->Value(ToRef,ToDef,Degree?Degree[0]:'L',0,Scan->X[x],Scan->Y[x],0,&v,NULL);
              Scan->D[x]=v;
            }
         }
      }

/*
         if (sz==4) {
            for(x=n-1;x>=0;x--) {
               Scan->X[x]=(double)((float*)Scan->X)[x];
               Scan->Y[x]=(double)((float*)Scan->Y)[x];
            }
         }

         if (ToRef->Function)
            OCTTransform(ToRef->InvFunction,n,Scan->X,Scan->Y,NULL);

         if (ToRef->InvTransform) {
            for(x=0;x<n;x++) {
               x0=ToRef->InvTransform[0]+ToRef->InvTransform[1]*Scan->X[x]+ToRef->InvTransform[2]*Scan->Y[x];
               y0=ToRef->InvTransform[3]+ToRef->InvTransform[4]*Scan->X[x]+ToRef->InvTransform[5]*Scan->Y[x];
               Scan->X[x]=x0;
               Scan->Y[x]=y0;
            }
         }
*/
#endif
   } else {
#ifdef HAVE_RMN
      if (sz==8) {
         for(x=0;x<n;x++) {
            /*RPN functions go from 0 to 360 instead of -180 to 180*/
            x0=Scan->X[x]<0?Scan->X[x]+360:Scan->X[x];
            y0=Scan->Y[x];
            ((float*)Scan->X)[x]=x0;
            ((float*)Scan->Y)[x]=y0;
         }
      }

//      RPN_IntLock();
      c_gdxyfll(ToRef->Ids[ToRef->NId],(float*)Scan->X,(float*)Scan->Y,(float*)Scan->Y,(float*)Scan->X,n);
//      RPN_IntUnlock();
//EZFIX
      /*If we have the data of source, get it's values right now*/
      if (ToDef) {
         if (Degree)
            c_ezsetopt("INTERP_DEGREE",Degree);
         c_gdxysval(ToRef->Ids[ToRef->NId],Scan->D,(float*)ToDef->Mode,(float*)Scan->X,(float*)Scan->Y,n);
      }

      /*Cast back to double (Start from end since type is double, not to overlap values*/
      for(x=n-1;x>=0;x--) {
         Scan->X[x]=(double)((float*)Scan->X)[x]-1.0;
         Scan->Y[x]=(double)((float*)Scan->Y)[x]-1.0;

         /*If we're outside, set to nodata*/
         if (ToDef && (!FIN2D(ToDef,Scan->X[x],Scan->Y[x]) || (ToDef->Mask && !ToDef->Mask[FIDX2D(ToDef,lrint(Scan->X[x]),lrint(Scan->Y[x]))]))) {
            Scan->D[x]=ToDef->NoData;
         }
      }
#else
      App_ErrorSet("RMNLIB support not included.");
#endif
   }
   return(d);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoFunc_RadialPointRatio>
 * Creation     : Fevrier 2008 J.P. Gauthier - CMC/CMOE
 *
 * But          : Interpoler la position d'un point sur un grand cercle
 *
 * Parametres   :
 *  <C1>        : Coordonne du premier point
 *  <C2>        : Coordonne du deuxieme point
 *  <C3>        : Coordonne du point a localier sur le grand cercle
 *
 * Retour       : Ratio de distance
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
double GeoFunc_RadialPointRatio(Coord C1,Coord C2,Coord C3) {

   Coord cr;
   double d0,d1,d2;

   GeoFunc_RadialPointOn(C1,C2,C3,&cr);

   d0=DIST(0,C1.Lat,C1.Lon,C2.Lat,C2.Lon);
   d1=DIST(0,C1.Lat,C1.Lon,cr.Lat,cr.Lon);
   d2=DIST(0,C2.Lat,C2.Lon,cr.Lat,cr.Lon);

   if(d2>d0) {
      return(-(d2-d0)/d0);
   } else {
      return(d1/d0);
   }
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoFunc_RadialPointOn>
 * Creation     : Fevrier 2008 J.P. Gauthier - CMC/CMOE
 *
 * But          : Calculer le point d'intersection en tracant un angle droit d'un point sur un grand cercle
 *
 * Parametres   :
 *  <C1>        : Coordonne du premier point
 *  <C2>        : Coordonne du deuxieme point
 *  <C3>        : Coordonne du point a localier sur le grand cercle
 *  <CR>        : Coordonne du point localise
 *
 * Retour       : Intersection existe
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoFunc_RadialPointOn(Coord C1,Coord C2,Coord C3,Coord *CR) {

   double crs12,crs13,crs3x;

   /*Calculates 90 degree course crossing*/
   crs12=COURSE(C1.Lat,C1.Lon,C2.Lat,C2.Lon);
   crs13=COURSE(C1.Lat,C1.Lon,C3.Lat,C3.Lon);
   crs3x=crs13>crs12?crs12-M_PI2:crs12+M_PI2;

   return(GeoFunc_RadialIntersect(C1,C3,crs12,crs3x,CR));
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoFunc_RadialIntersect>
 * Creation     : Fevrier 2008 J.P. Gauthier - CMC/CMOE
 *
 * But          : Calculer le point d'intersection de deux grand cercle
 *
 * Parametres   :
 *  <C1>        : Coordonne du premier point
 *  <C2>        : Coordonne du deuxieme point
 *  <CRS13>     : Direction entre le premier et le troisieme point
 *  <CRS23>     : Direction entre le deuxieme et troisieme point
 *  <C3>        : Point d'intersection
 *
 * Retour       : Intersection existe
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoFunc_RadialIntersect(Coord C1,Coord C2,double CRS13,double CRS23,Coord *C3) {

   double dst13,dst12,crs12,crs21,ang1,ang2,ang3;

   Coord sinc2,cosc2,sinc1,cosc1;

   sinc1.Lat=sin(C1.Lat);sinc1.Lon=sin(C1.Lon);
   cosc1.Lat=cos(C1.Lat);cosc1.Lon=cos(C1.Lon);
   sinc2.Lat=sin(C2.Lat);sinc2.Lon=sin(C2.Lon);
   cosc2.Lat=cos(C2.Lat);cosc2.Lon=cos(C2.Lon);

   dst12=2*asin(sqrt(pow((sin((C1.Lat-C2.Lat)/2)),2) + cosc1.Lat*cosc2.Lat*pow(sin((C1.Lon-C2.Lon)/2),2)));

   if (sin(C2.Lon-C1.Lon)<0) {
      crs12=acos((sinc2.Lat-sinc1.Lat*cos(dst12))/(sin(dst12)*cosc1.Lat));
   } else {
      crs12=2.0*M_PI-acos((sinc2.Lat-sinc1.Lat*cos(dst12))/(sin(dst12)*cosc1.Lat));
   }

   if (sin(C1.Lon-C2.Lon)<0) {
      crs21=acos((sinc1.Lat-sinc2.Lat*cos(dst12))/(sin(dst12)*cosc2.Lat));
   } else {
      crs21=M_2PI-acos((sinc1.Lat-sinc2.Lat*cos(dst12))/(sin(dst12)*cosc2.Lat));
   }

   ang1=fmod(CRS13-crs12+M_PI,M_2PI)-M_PI;
   ang2=fmod(crs21-CRS23+M_PI,M_2PI)-M_PI;

   if (sin(ang1)*sin(ang2)<=sqrt(10e-15)) {
      /*No intersection*/
      return(0);
   } else {
      ang1=fabs(ang1);
      ang2=fabs(ang2);
      ang3=acos(-cos(ang1)*cos(ang2)+sin(ang1)*sin(ang2)*cos(dst12));
      dst13=asin(sin(ang2)*sin(dst12)/sin(ang3));
      C3->Lat=asin(sinc1.Lat*cos(dst13)+cosc1.Lat*sin(dst13)*cos(CRS13));
      C3->Lon=fmod(C1.Lon-asin(sin(CRS13)*sin(dst13)/cos(C3->Lat))+M_PI,M_2PI)-M_PI;
   }

   return(1);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_Size>
 * Creation     : Juillet 2005 J.P. Gauthier
 *
 * But          : Initialiser les limites d'un georef.
 *
 * Parametres   :
 *   <Ref>      : Pointeur sur la reference geographique
 *   <X0>       : Coordonnee X minimale
 *   <Y0>       : Coordonnee Y minimale
 *   <Z0>       : Coordonnee Z minimale
 *   <X1>       : Coordonnee X maximale
 *   <Y1>       : Coordonnee Y maximale
 *   <Z1>       : Coordonnee Z maximale
 *   <BD>       : Bordure
 *
 * Retour       : Code de retour standard TCL
 *
 * Remarques :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
void GeoRef_Size(TGeoRef *Ref,int X0,int Y0,int Z0,int X1,int Y1,int Z1,int BD) {

   Ref->X0=X0;
   Ref->X1=X1;
   Ref->Y0=Y0;
   Ref->Y1=Y1;
   Ref->Z0=Z0;
   Ref->Z1=Z1;
   Ref->BD=BD;
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_Free>
 * Creation     : Juillet 2005 J.P. Gauthier
 *
 * But          : Liberer les resources alloeur par un georef.
 *
 * Parametres   :
 *   <Ref>      : Pointeur sur la reference geographique
 *
 * Retour       : Code de retour standard TCL
 *
 * Remarques :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoRef_Free(TGeoRef *Ref) {

  if (!Ref)
      return(0);

   if (__sync_sub_and_fetch(&Ref->NRef,1)>0) {
      return(0);
   }

   if (Ref->RefFrom) {
//      GeoRef_Free(Ref->RefFrom);
      __sync_sub_and_fetch(&Ref->RefFrom->NRef,1);
   }

   GeoRef_Clear(Ref,1);
   free(Ref);

   return(1);
}

int GeoRef_Incr(TGeoRef *Ref) {

   if (Ref) {
      return(__sync_add_and_fetch(&Ref->NRef,1));
   } else {
      return(0);
   }
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_Clear>
 * Creation     : Mars 2005 J.P. Gauthier - CMC/CMOE
 *
 * But          : Liberer une structure de projection WKT.
 *
 * Parametres  :
 *   <Ref>     : Pointeur sur la reference geographique
 *   <New>     : Nouveau georef
 *
 * Retour       : Un code d'erreur Tcl standard.
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
void GeoRef_Clear(TGeoRef *Ref,int New) {

   int n;

   if (Ref->String)       free(Ref->String);       Ref->String=NULL;
   if (Ref->Transform)    free(Ref->Transform);    Ref->Transform=NULL;
   if (Ref->InvTransform) free(Ref->InvTransform); Ref->InvTransform=NULL;
   if (Ref->Lat)          free(Ref->Lat);          Ref->Lat=NULL;
   if (Ref->Lon)          free(Ref->Lon);          Ref->Lon=NULL;
   if (Ref->Hgt)          free(Ref->Hgt);          Ref->Hgt=NULL;
   if (Ref->Idx)          free(Ref->Idx);          Ref->Idx=NULL; Ref->NIdx=0;
   if (Ref->AX)           free(Ref->AX);           Ref->AX=NULL;
   if (Ref->AY)           free(Ref->AY);           Ref->AY=NULL;

   Ref->IG1=Ref->IG2=Ref->IG3=Ref->IG4=0;

   if (Ref->Pos) {
      for(n=0;n<Ref->ZRef.LevelNb;n++) {
         if (Ref->Pos[n]) free(Ref->Pos[n]);
      }
      free(Ref->Pos);
      Ref->Pos=NULL;
   }

   if (New) {
      if (Ref->Name)      free(Ref->Name);         Ref->Name=NULL;
      ZRef_Free(&Ref->ZRef);
   }

#ifdef HAVE_RMN
   // Release ezscint sub-grid
   if (Ref->Ids) {
      for(n=0;n<Ref->NbId+1;n++) {
         if (Ref->Ids[n]>-1)
            RPN_IntIdFree(Ref->Ids[n]);
      }
      free(Ref->Ids);  Ref->Ids=NULL;
   }
#endif

#ifdef HAVE_GDAL
   if (Ref->GCPTransform) {
      GDALDestroyGCPTransformer(Ref->GCPTransform);
      Ref->GCPTransform=NULL;
   }
   if (Ref->TPSTransform) {
      GDALDestroyTPSTransformer(Ref->TPSTransform);
      Ref->TPSTransform=NULL;
   }
   if (Ref->RPCTransform) {
      GDALDestroyRPCTransformer(Ref->RPCTransform);
      Ref->RPCTransform=NULL;
   }

   if (Ref->Spatial) {
      OSRDestroySpatialReference(Ref->Spatial);
   }

   if (Ref->Function) {
      OCTDestroyCoordinateTransformation(Ref->Function);
      Ref->Function=NULL;
   }

   if (Ref->InvFunction) {
      OCTDestroyCoordinateTransformation(Ref->InvFunction);
      Ref->InvFunction=NULL;
   }
#endif

   Ref->RefFrom=NULL;
   Ref->Project=NULL;
   Ref->UnProject=NULL;
   Ref->Value=NULL;
   Ref->Distance=NULL;
   Ref->Height=NULL;
}

void GeoRef_Qualify(TGeoRef *Ref) {

   Coord co[2];
   double d[2];

   if (Ref) {
      Ref->Type=GRID_NONE;

      if (Ref->Grid[0]=='X') {
         return;
      }

      if (Ref->Grid[0]=='M' || Ref->Grid[0]=='Y' || Ref->Grid[1]=='X' || Ref->Grid[1]=='Y' || Ref->Grid[1]=='Z') {
         Ref->Type|=GRID_SPARSE;
      } else {
         Ref->Type|=GRID_REGULAR;
      }

      if (Ref->Grid[0]=='#') {
         Ref->Type|=GRID_TILE;
      }

      if (Ref->Grid[0]=='A' || Ref->Grid[0]=='G') {
         Ref->Type|=GRID_WRAP;
      } else if (Ref->Grid[0]!='V' && Ref->X0!=Ref->X1 && Ref->Y0!=Ref->Y1) {
         /*Get size of a gridpoint*/
         Ref->Project(Ref,Ref->X0+(Ref->X1-Ref->X0)/2.0,Ref->Y0+(Ref->Y1-Ref->Y0)/2.0,&co[0].Lat,&co[0].Lon,1,1);
         Ref->Project(Ref,Ref->X0+(Ref->X1-Ref->X0)/2.0+1.0,Ref->Y0+(Ref->Y1-Ref->Y0)/2.0,&co[1].Lat,&co[1].Lon,1,1);
         d[0]=DIST(0.0,DEG2RAD(co[0].Lat),DEG2RAD(co[0].Lon),DEG2RAD(co[1].Lat),DEG2RAD(co[1].Lon));

         /*Get distance between first and lat point*/
         Ref->Project(Ref,Ref->X0,Ref->Y0+(Ref->Y1-Ref->Y0)/2.0,&co[0].Lat,&co[0].Lon,1,1);
         Ref->Project(Ref,Ref->X1,Ref->Y0+(Ref->Y1-Ref->Y0)/2.0,&co[1].Lat,&co[1].Lon,1,1);
         d[1]=DIST(0.0,DEG2RAD(co[0].Lat),DEG2RAD(co[0].Lon),DEG2RAD(co[1].Lat),DEG2RAD(co[1].Lon));

         /*If we're within 1.5 grid point, we wrap*/
         if (d[1]<=(d[0]*1.5)) {
            Ref->Type|=GRID_WRAP;
         }

         /*If we're within 0.25 grid point, we repeat*/
         if (d[1]<=(d[0]*0.25)) {
            Ref->Type|=GRID_REPEAT;
         }
      }

      if (Ref->Grid[0]=='V') {
         Ref->Type|=GRID_VERTICAL;
      }

      if (Ref->Grid[0]=='R') {
         Ref->Type|=GRID_RADIAL;
      }
   }
}

int GeoRef_Equal(TGeoRef *Ref0,TGeoRef *Ref1,int Dim) {

   if (!Ref0 || !Ref1) {
      return(0);
   }

   if (Ref0->IG1!=Ref1->IG1 || Ref0->IG2!=Ref1->IG2 || Ref0->IG3!=Ref1->IG3 || Ref0->IG4!=Ref1->IG4)
     return(0);

   /*Pacth temporaire du au lagrangien qui doivent avoir des GeoRef differents*/
   if (Ref0->Grid[0]=='M' || Ref0->Grid[0]=='X' || Ref0->Grid[0]=='Y' || Ref0->Grid[1]=='Y' || Ref0->Grid[0]=='#')
      return(0);

   if (Ref1->Grid[0]=='M' || Ref1->Grid[0]=='X' || Ref1->Grid[0]=='Y' || Ref1->Grid[1]=='Y' || Ref1->Grid[0]=='#')
      return(0);

   /*Test for limits but only for ther refs with transforms (bands)*/
   if (Ref0->Transform || Ref1->Transform)
      if (Ref0->BD!=Ref1->BD || Ref0->X0!=Ref1->X0 || Ref0->X1!=Ref1->X1 || Ref0->Y0!=Ref1->Y0 || Ref0->Y1!=Ref1->Y1 || Ref0->Z0!=Ref1->Z0 || Ref0->Z1!=Ref1->Z1)
         return(0);

   if (Ref0->Grid[0]!=Ref1->Grid[0] || Ref0->Grid[1]!=Ref1->Grid[1])
      return(0);

   if (Dim==3 && !ZRef_Equal(&Ref0->ZRef,&Ref1->ZRef))
      return(0);

   if (Ref0->Ids && Ref1->Ids && Ref0->Ids[Ref0->NId]!=Ref1->Ids[Ref1->NId])
      return(0);

   if (Ref0->R!=Ref1->R || Ref0->ResR!=Ref1->ResR || Ref0->ResA!=Ref1->ResA || Ref0->Loc.Lat!=Ref1->Loc.Lat || Ref0->Loc.Lon!=Ref1->Loc.Lon || Ref0->Loc.Elev!=Ref1->Loc.Elev)
      return(0);

   if ((Ref0->Spatial && !Ref1->Spatial) || (!Ref0->Spatial && Ref1->Spatial))
      return(0);

#ifdef HAVE_GDAL
   if (Ref0->Spatial && Ref1->Spatial && !OSRIsSame(Ref0->Spatial,Ref1->Spatial))
      return(0);
#endif

   if ((Ref0->Transform && !Ref1->Transform) || (!Ref0->Transform && Ref1->Transform))
      return(0);

   if (Ref0->Transform && Ref1->Transform)
      if (memcmp(Ref0->Transform,Ref1->Transform,6*sizeof(double))!=0)
         return(0);

   return(1);
}

TGeoRef *GeoRef_Copy(TGeoRef *Ref) {

   GeoRef_Incr(Ref);
   return(Ref);
}

TGeoRef *GeoRef_Reference(TGeoRef *Ref) {

   TGeoRef *ref;

   ref=GeoRef_New();

   GeoRef_Incr(Ref);
   ref->RefFrom=Ref;
   ref->Project=Ref->Project;
   ref->UnProject=Ref->UnProject;
   ref->Value=Ref->Value;
   ref->Distance=Ref->Distance;

   ref->Loc.Lat=Ref->Loc.Lat;
   ref->Loc.Lon=Ref->Loc.Lon;
   ref->Loc.Elev=Ref->Loc.Elev;
   ref->R=Ref->R;
   ref->ResR=Ref->ResR;
   ref->ResA=Ref->ResA;
   ref->NbId=1;
   ref->NId=0;

   return(ref);
}

TGeoRef *GeoRef_HardCopy(TGeoRef *Ref) {

   TGeoRef *ref;
   int      i;

   ref=GeoRef_New();
   GeoRef_Size(ref,Ref->X0,Ref->Y0,Ref->Z0,Ref->X1,Ref->Y1,Ref->Z1,Ref->BD);

   ref->Grid[0]=Ref->Grid[0];
   ref->Grid[1]=Ref->Grid[1];
   ref->Project=Ref->Project;
   ref->UnProject=Ref->UnProject;
   ref->Value=Ref->Value;
   ref->Distance=Ref->Distance;
   ref->Type=Ref->Type;
   ref->NbId=Ref->NbId;
   ref->NId=Ref->NId;

#ifdef HAVE_RMN
   if (Ref->Ids) {
      ref->Ids=(int*)malloc(Ref->NbId*sizeof(int));
      memcpy(ref->Ids,Ref->Ids,Ref->NbId*sizeof(int));
      for(i=0;i<ref->NbId;i++)
         c_ez_refgrid(ref->Ids[i]);
   }
#endif

   ref->IG1=Ref->IG1;
   ref->IG2=Ref->IG2;
   ref->IG3=Ref->IG3;
   ref->IG4=Ref->IG4;

   ZRef_Copy(&ref->ZRef,&Ref->ZRef,-1);

   switch(ref->Grid[0]) {
      case 'R' :
         ref->Loc.Lat=Ref->Loc.Lat;
         ref->Loc.Lon=Ref->Loc.Lon;
         ref->Loc.Elev=Ref->Loc.Elev;
         ref->R=Ref->R;
         ref->ResR=Ref->ResR;
         ref->ResA=Ref->ResA;
      case 'W' :
         GeoRef_WKTSet(ref,Ref->String,Ref->Transform,Ref->InvTransform,Ref->Spatial);
  }

  return(ref);
}

TGeoRef *GeoRef_Resize(TGeoRef *Ref,int NI,int NJ,int NK,int Type,float *Levels) {

   TGeoRef *ref;

   if (!Ref) {
      ref=GeoRef_New();
   } else {
      ref=GeoRef_HardCopy(Ref);
   }
   GeoRef_Size(ref,0,0,0,NI-1,NJ-1,NK-1,0);

   ref->ZRef.LevelNb=(Ref && Ref->Grid[0]=='V')?NJ:NK;
   ref->ZRef.Type=Type;
   ref->ZRef.Levels=(float*)realloc(ref->ZRef.Levels,ref->ZRef.LevelNb*sizeof(float));
   if (Levels)
      memcpy(ref->ZRef.Levels,Levels,ref->ZRef.LevelNb*sizeof(float));

   return(ref);
}

int GeoRef_Project(TGeoRef *Ref,double X,double Y,double *Lat,double *Lon,int Extrap,int Transform) {

   if (X>Ref->X1 || Y>Ref->Y1 || X<Ref->X0 || Y<Ref->Y0) {
      if (!Extrap) {
         *Lon=-999.0;
         *Lat=-999.0;
         return(0);
      }
   }
   *Lon=X/(Ref->X1-Ref->X0);
   *Lat=Y/(Ref->Y1-Ref->Y0);

   return(1);
}

int GeoRef_UnProject(TGeoRef *Ref,double *X,double *Y,double Lat,double Lon,int Extrap,int Transform) {

   *X=Lon*(Ref->X1-Ref->X0);
   *Y=Lat*(Ref->Y1-Ref->Y0);

   /*Check the grid limits*/
   if (*X>Ref->X1 || *Y>Ref->Y1 || *X<Ref->X0 || *Y<Ref->Y0) {
      if (!Extrap) {
         *X=-1.0;
         *Y=-1.0;
      }
      return(0);
   }
   return(1);
}

TGeoRef* GeoRef_New() {

   TGeoRef *ref=malloc(sizeof(TGeoRef));

   GeoRef_Size(ref,0,0,0,0,0,0,0);

   /*General*/
   ref->Name=NULL;
   ref->NbId=0;
   ref->NId=0;
   ref->Ids=NULL;
   ref->Type=GRID_NONE;
   ref->NRef=1;
   ref->NIdx=0;
   ref->Pos=NULL;
   ref->Lat=NULL;
   ref->Lon=NULL;
   ref->Hgt=NULL;
   ref->Idx=NULL;
   ref->AX=NULL;
   ref->AY=NULL;
   ref->RefFrom=NULL;
   ref->Grid[0]='X';
   ref->Grid[1]='\0';
   ref->Grid[2]='\0';
   ref->IG1=ref->IG2=ref->IG3=ref->IG4=0;

   /*WKT Specific*/
   ref->String=NULL;
   ref->Spatial=NULL;
   ref->Function=NULL;
   ref->InvFunction=NULL;
   ref->Transform=NULL;
   ref->InvTransform=NULL;
   ref->GCPTransform=NULL;
   ref->TPSTransform=NULL;
   ref->RPCTransform=NULL;
   ref->LLExtent.MinX=1e32;
   ref->LLExtent.MinY=1e32;
   ref->LLExtent.MaxX=-1e32;
   ref->LLExtent.MaxY=-1e32;

   /*RDR Specific*/
   ref->Loc.Lat=-999;
   ref->Loc.Lon=-999;
   ref->Loc.Elev=-999;
   ref->R=0;
   ref->CTH=0;
   ref->STH=0;
   ref->ResR=0;
   ref->ResA=0;

   /*General functions*/
   ref->Project=GeoRef_Project;
   ref->UnProject=GeoRef_UnProject;
   ref->Value=NULL;
   ref->Distance=NULL;
   ref->Height=NULL;

   /*Vertical reference*/
   ZRef_Init(&ref->ZRef);

   return(ref);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_Intersect>
 * Creation     : Aout 2006 J.P. Gauthier - CMC/CMOE
 *
 * But          : Verifier l'intersection de deux georeference, Ref0 dans Ref1.
 *
 * Parametres  :
 *   <Ref0>     : Pointeur sur la reference geographique 1
 *   <Ref1>     : Pointeur sur la reference geographique 2
 *   <X0,Y0,...>: Limites dans le Ref 1 du Ref 0
 *   <BD>       : Include border
 *
 * Retour       :
 *   <Inter>    : Booleen indiquant l'intersection
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoRef_Intersect(TGeoRef *Ref0,TGeoRef *Ref1,int *X0,int *Y0,int *X1,int *Y1,int BD) {

   double lat,lon,di,dj,in=0;
   double x0,y0,x1,y1;
   int    x,y;

   /*Source grid Y*/
   if (Ref1->Grid[0]=='Y') {
      *X0=Ref1->X0; *Y0=Ref1->Y0;
      *X1=Ref1->X1; *Y1=Ref1->Y1;
      return(1);
   }

   /*If destination is global*/
   if (Ref0->Type&GRID_WRAP) {
      *X0=Ref1->X0; *Y0=Ref1->Y0;
      *X1=Ref1->X1; *Y1=Ref1->Y1;
      in=1;
   }

   /*Test for limit source inclusion into destination*/
   x0=y0=1e32;
   x1=y1=-1e32;
   x=0;

   if (!in) {
      Ref1->Project(Ref1,Ref1->X0,Ref1->Y0,&lat,&lon,0,1);
      if (Ref0->UnProject(Ref0,&di,&dj,lat,lon,0,1)) {
         x0=Ref1->X0;y0=Ref1->Y0;
         x++;
      }
      Ref1->Project(Ref1,Ref1->X0,Ref1->Y1,&lat,&lon,0,1);
      if (Ref0->UnProject(Ref0,&di,&dj,lat,lon,0,1)) {
         x0=Ref1->X0;y1=Ref1->Y1;
         x++;
      }
      Ref1->Project(Ref1,Ref1->X1,Ref1->Y0,&lat,&lon,0,1);
      if (Ref0->UnProject(Ref0,&di,&dj,lat,lon,0,1)) {
         x1=Ref1->X1;y0=Ref1->Y0;
         x++;
      }
      Ref1->Project(Ref1,Ref1->X1,Ref1->Y1,&lat,&lon,0,1);
      if (Ref0->UnProject(Ref0,&di,&dj,lat,lon,0,1)) {
         x1=Ref1->X1;y1=Ref1->Y1;
      }
      *X0=x0; *Y0=y0;
      *X1=x1; *Y1=y1;

      if (x>=3) {
         in=1;
      }
   }

   if (!in) {

      /*Project Ref0 within Ref1 and get limits*/
      for(x=Ref0->X0;x<=Ref0->X1;x++) {
         Ref0->Project(Ref0,x,Ref0->Y0,&lat,&lon,0,1);
         Ref1->UnProject(Ref1,&di,&dj,lat,lon,1,1);
         x0=FMIN(x0,di); y0=FMIN(y0,dj);
         x1=FMAX(x1,di); y1=FMAX(y1,dj);

         Ref0->Project(Ref0,x,Ref0->Y1,&lat,&lon,0,1);
         Ref1->UnProject(Ref1,&di,&dj,lat,lon,1,1);
         x0=FMIN(x0,di); y0=FMIN(y0,dj);
         x1=FMAX(x1,di); y1=FMAX(y1,dj);
      }

      for(y=Ref0->Y0;y<=Ref0->Y1;y++) {
         Ref0->Project(Ref0,Ref0->X0,y,&lat,&lon,0,1);
         Ref1->UnProject(Ref1,&di,&dj,lat,lon,1,1);
         x0=FMIN(x0,di); y0=FMIN(y0,dj);
         x1=FMAX(x1,di); y1=FMAX(y1,dj);

         Ref0->Project(Ref0,Ref0->X1,y,&lat,&lon,0,1);
         Ref1->UnProject(Ref1,&di,&dj,lat,lon,1,1);
         x0=FMIN(x0,di); y0=FMIN(y0,dj);
         x1=FMAX(x1,di); y1=FMAX(y1,dj);
      }

      /*Test for north and south pole including grid*/
      if (Ref0->UnProject(Ref0,&di,&dj,89.9,0.0,0,1) && dj>Ref0->Y0+2 && dj<Ref0->Y1-2 && di>Ref0->X0+2 && di<Ref0->X1-2) {
         Ref1->UnProject(Ref1,&di,&dj,89.9,0.0,1,1);
         x0=FMIN(x0,di); y0=FMIN(y0,dj);
         x1=FMAX(x1,di); y1=FMAX(y1,dj);
      }
      if (Ref0->UnProject(Ref0,&di,&dj,-89.9,0.0,0,1) && dj>Ref0->Y0+2 && dj<Ref0->Y1-2 && di>Ref0->X0+2 && di<Ref0->X1-2) {
         Ref1->UnProject(Ref1,&di,&dj,-89.9,0.0,1,1);
         x0=FMIN(x0,di); y0=FMIN(y0,dj);
         x1=FMAX(x1,di); y1=FMAX(y1,dj);
      }

      *X0=floor(x0); *Y0=floor(y0);
      *X1=ceil(x1);  *Y1=ceil(y1);

      if (!VOUT(*X0,*X1,Ref1->X0,Ref1->X1) && !VOUT(*Y0,*Y1,Ref1->Y0,Ref1->Y1)) {
         in=1;
      }
   }

   /*Clamp the coordinates*/
   if (BD) {
      REFCLAMP(Ref1,*X0,*Y0,*X1,*Y1);
   } else {
      REFCLAMPBD(Ref1,*X0,*Y0,*X1,*Y1);
   }

   return(in);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_Limits>
 * Creation     : Aout 2006 J.P. Gauthier - CMC/CMOE
 *
 * But          : Calculer les limites en latlon de la couverture d'une georeference.
 *
 * Parametres  :
 *   <Ref>     : Pointeur sur la reference geographique
 *   <Lat0>    : Latitude inferieure
 *   <Lon0>    : Longitude inferieure
 *   <Lat1>    : Latitude superieure
 *   <Lon1>    : Longitude superieure
 *
 * Retour       :
 *   <valid>    : Booleen indiquant la validite
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoRef_Limits(TGeoRef *Ref,double *Lat0,double *Lon0,double *Lat1,double *Lon1) {

   int x,y;
   double di,dj,lat,lon;

   *Lat0=*Lon0=1e32;
   *Lat1=*Lon1=-1e32;

   /*Source grid Y*/
   if (Ref->Grid[0]=='Y' || Ref->Grid[1]=='Y' || Ref->Grid[0]=='X' || Ref->Grid[1]=='X') {
      for(x=0;x<((Ref->X1-Ref->X0)+1)*((Ref->Y1-Ref->Y0)+1);x++) {
         *Lat0=FMIN(*Lat0,Ref->Lat[x]); *Lon0=FMIN(*Lon0,Ref->Lon[x]);
         *Lat1=FMAX(*Lat1,Ref->Lat[x]); *Lon1=FMAX(*Lon1,Ref->Lon[x]);
      }
      return(1);
   }

   /*If destination is global*/
   if (Ref->Type&GRID_WRAP) {
      *Lat0=-90.0;  *Lat1=90.0;
      *Lon0=-180.0; *Lon1=180.0;
      return(1);
   }

   /*Project Ref0 Border within Ref1 and get limits*/
   for(x=Ref->X0,y=Ref->Y0;x<=Ref->X1;x++) {
      Ref->Project(Ref,x,y,&lat,&lon,0,1);
      *Lat0=FMIN(*Lat0,lat); *Lon0=FMIN(*Lon0,lon);
      *Lat1=FMAX(*Lat1,lat); *Lon1=FMAX(*Lon1,lon);
   }

   for(x=Ref->X0,y=Ref->Y1;x<=Ref->X1;x++) {
      Ref->Project(Ref,x,y,&lat,&lon,0,1);
      *Lat0=FMIN(*Lat0,lat); *Lon0=FMIN(*Lon0,lon);
      *Lat1=FMAX(*Lat1,lat); *Lon1=FMAX(*Lon1,lon);
   }

   for(y=Ref->Y0,x=Ref->X0;y<=Ref->Y1;y++) {
      Ref->Project(Ref,x,y,&lat,&lon,0,1);
      *Lat0=FMIN(*Lat0,lat); *Lon0=FMIN(*Lon0,lon);
      *Lat1=FMAX(*Lat1,lat); *Lon1=FMAX(*Lon1,lon);
   }

   for(y=Ref->Y0,x=Ref->X1;y<=Ref->Y1;y++) {
      Ref->Project(Ref,x,y,&lat,&lon,0,1);
      *Lat0=FMIN(*Lat0,lat); *Lon0=FMIN(*Lon0,lon);
      *Lat1=FMAX(*Lat1,lat); *Lon1=FMAX(*Lon1,lon);
   }

   /*Test for north and south pole including grid*/
   if (Ref->UnProject(Ref,&di,&dj,90.0,0.0,0,1) && dj>Ref->Y0+2 && dj<Ref->Y1-2 && di>Ref->X0+2 && di<Ref->X1-2) {
      *Lat1=90.0;
   }
   if (Ref->UnProject(Ref,&di,&dj,-90.0,0.0,0,1) && dj>Ref->Y0+2 && dj<Ref->Y1-2 && di>Ref->X0+2 && di<Ref->X1-2) {
      *Lat0=-90.0;
   }
   return(1);
}

int GeoRef_Within(TGeoRef *Ref0,TGeoRef *Ref1) {

   double lat,lon,di,dj;
   int    x,y;

   /*Project Ref0 Border within Ref1 and get limits*/
   for(x=Ref0->X0,y=Ref0->Y0;x<=Ref0->X1;x++) {
      Ref0->Project(Ref0,x,y,&lat,&lon,0,1);
      if (!Ref1->UnProject(Ref1,&di,&dj,lat,lon,0,1)) {
         return(0);
      }
   }

   for(x=Ref0->X0,y=Ref0->Y1;x<=Ref0->X1;x++) {
      Ref0->Project(Ref0,x,y,&lat,&lon,0,1);
      if (!Ref1->UnProject(Ref1,&di,&dj,lat,lon,0,1)) {
         return(0);
      }
   }

   for(y=Ref0->Y0,x=Ref0->X0;y<=Ref0->Y1;y++) {
      Ref0->Project(Ref0,x,y,&lat,&lon,0,1);
      if (!Ref1->UnProject(Ref1,&di,&dj,lat,lon,0,1)) {
         return(0);
      };
   }

   for(y=Ref0->Y0,x=Ref0->X1;y<=Ref0->Y1;y++) {
      Ref0->Project(Ref0,x,y,&lat,&lon,0,1);
      if (!Ref1->UnProject(Ref1,&di,&dj,lat,lon,0,1)) {
         return(0);
      }
   }
   return(1);
}

int GeoRef_WithinRange(TGeoRef *Ref,double Lat0,double Lon0,double Lat1,double Lon1,int In) {

   double lat[4],lon[4],dl;
   int    d0,d1,d2,d3;

   if (Lon0*Lon1<0) {
      dl=Lon1-Lon0;
   } else {
      dl=0;
   }

   if (Lat0>Lat1) {
      dl=Lat0;
      Lat0=Lat1;
      Lat1=dl;
   }

   if (Lon0>Lon1) {
      dl=Lon0;
      Lon0=Lon1;
      Lon1=dl;
   }

   /* Check image within range */
   Ref->Project(Ref,Ref->X0,Ref->Y0,&lat[0],&lon[0],0,1);
   d0=FWITHIN(dl,Lat0,Lon0,Lat1,Lon1,lat[0],lon[0]);
   if (!In && d0) return(1);

   Ref->Project(Ref,Ref->X1,Ref->Y0,&lat[1],&lon[1],0,1);
   d1=FWITHIN(dl,Lat0,Lon0,Lat1,Lon1,lat[1],lon[1]);
   if (!In && d1) return(1);

   Ref->Project(Ref,Ref->X1,Ref->Y1,&lat[2],&lon[2],0,1);
   d2=FWITHIN(dl,Lat0,Lon0,Lat1,Lon1,lat[2],lon[2]);
   if (!In && d2) return(1);

   Ref->Project(Ref,Ref->X0,Ref->Y1,&lat[3],&lon[3],0,1);
   d3=FWITHIN(dl,Lat0,Lon0,Lat1,Lon1,lat[3],lon[3]);
   if (!In && d3) return(1);

   /* Check for all contained */
   if (In) {
      if (d0 && d1 && d2 && d3) {
         return(1);
      } else {
         return(0);
      }
   }

   /* Check range within image */
   lat[0]=FMIN(FMIN(FMIN(lat[0],lat[1]),lat[2]),lat[3]);
   lat[1]=FMAX(FMAX(FMAX(lat[0],lat[1]),lat[2]),lat[3]);
   lon[0]=FMIN(FMIN(FMIN(lon[0],lon[1]),lon[2]),lon[3]);
   lon[1]=FMAX(FMAX(FMAX(lon[0],lon[1]),lon[2]),lon[3]);

   if (FWITHIN(dl,lat[0],lon[0],lat[1],lon[1],Lat0,Lon0)) return(1);
   if (FWITHIN(dl,lat[0],lon[0],lat[1],lon[1],Lat0,Lon1)) return(1);
   if (FWITHIN(dl,lat[0],lon[0],lat[1],lon[1],Lat1,Lon1)) return(1);
   if (FWITHIN(dl,lat[0],lon[0],lat[1],lon[1],Lat1,Lon0)) return(1);

   return(0);
}

int GeoRef_BoundingBox(TGeoRef *Ref,double Lat0,double Lon0,double Lat1,double Lon1,double *I0,double *J0,double *I1,double *J1) {

   double di,dj;


   *I0=*J0=1000000;
   *I1=*J1=-1000000;

   Ref->UnProject(Ref,&di,&dj,Lat0,Lon0,1,1);
   *I0=*I0<di?*I0:di;
   *J0=*J0<dj?*J0:dj;
   *I1=*I1>di?*I1:di;
   *J1=*J1>dj?*J1:dj;

   Ref->UnProject(Ref,&di,&dj,Lat0,Lon1,1,1);
   *I0=*I0<di?*I0:di;
   *J0=*J0<dj?*J0:dj;
   *I1=*I1>di?*I1:di;
   *J1=*J1>dj?*J1:dj;

   Ref->UnProject(Ref,&di,&dj,Lat1,Lon1,1,1);
   *I0=*I0<di?*I0:di;
   *J0=*J0<dj?*J0:dj;
   *I1=*I1>di?*I1:di;
   *J1=*J1>dj?*J1:dj;

   Ref->UnProject(Ref,&di,&dj,Lat1,Lon0,1,1);
   *I0=*I0<di?*I0:di;
   *J0=*J0<dj?*J0:dj;
   *I1=*I1>di?*I1:di;
   *J1=*J1>dj?*J1:dj;

   *I0=*I0<Ref->X0?Ref->X0:*I0;
   *J0=*J0<Ref->Y0?Ref->Y0:*J0;
   *I1=*I1>Ref->X1?Ref->X1:*I1;
   *J1=*J1>Ref->Y1?Ref->Y1:*J1;

   if (fabs(Lon0-Lon1)>=359.99999999) {
      *I0=Ref->X0;
      *I1=Ref->X1;
   }

   if (fabs(Lat0-Lat1)>=179.99999999) {
      *J0=Ref->Y0;
      *J1=Ref->Y1;
   }
   return(1);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_Valid>
 * Creation     : Fevrier 2009. Gauthier - CMC/CMOE
 *
 * But          : Verifier la validite d'une georeference.
 *
 * Parametres  :
 *   <Ref>      : Pointeur sur la reference
 *
 * Retour       :
 *   <valid>    : Booleen indiquant la validite
 *
 * Remarques   :
 *    - On projete la bounding box et si les latitudes sont en dehors de -90 90 alors c'est pas bon
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoRef_Valid(TGeoRef *Ref) {

   Coord co[2];

   Ref->Project(Ref,Ref->X0,Ref->Y0,&co[0].Lat,&co[0].Lon,1,1);
   Ref->Project(Ref,Ref->X1,Ref->Y1,&co[1].Lat,&co[1].Lon,1,1);

   if (co[0].Lat<-91 || co[0].Lat>91.0 || co[1].Lat<-91 || co[1].Lat>91.0) {
      return(0);
   }
   return(1);
}


/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <GeoRef_Positional>
 * Creation     : Mars 2010 J.P. Gauthier - CMC/CMOE
 *
 * But          : Assigner des vecteurs de positions X et Y
 *
 * Parametres   :
 *   <Ref>      : Pointeur sur la reference
 *   <XDef>     : Data definition des positions en X
 *   <YDef>     : Data definition des positions en Y
 *
 * Retour       : Dimension des resultats
 *
 * Remarques    :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
int GeoRef_Positional(TGeoRef *Ref,TDef *XDef,TDef *YDef) {

   int d,dx,dy,nx,ny;

   if (!Ref) {
      return(0);
   }

   /* Check the dimensions */
   nx=FSIZE2D(XDef);
   ny=FSIZE2D(YDef);
   dx=(Ref->X1-Ref->X0+1);
   dy=(Ref->Y1-Ref->Y0+1);

   if (Ref->Grid[0]=='Y' || Ref->Grid[0]=='W') {
      if (nx!=ny || nx!=dx*dy) {
         return(0);
      }
   } else if (Ref->Grid[0]=='V') {
      if (nx!=ny || nx!=dx) {
         return(0);
      }
   } else if (nx!=dx || ny!=dy) {
      return(0);
   }

   /*Clear arrays*/
   if (Ref->Lat) free(Ref->Lat);
   if (Ref->Lon) free(Ref->Lon);

   Ref->Lat=(float*)malloc(ny*sizeof(float));
   Ref->Lon=(float*)malloc(nx*sizeof(float));

   if (!Ref->Lat || !Ref->Lon) {
      return(0);
   }

   /*Assign positionals, if size is float, just memcopy otherwise, assign*/
   if (XDef->Type==TD_Float32) {
      memcpy(Ref->Lon,XDef->Data[0],nx*sizeof(float));
   } else {
      for(d=0;d<nx;d++) {
         Def_Get(XDef,0,d,Ref->Lon[d]);
      }
   }

   if (YDef->Type==TD_Float32) {
      memcpy(Ref->Lat,YDef->Data[0],ny*sizeof(float));
   } else {
      for(d=0;d<ny;d++) {
         Def_Get(YDef,0,d,Ref->Lat[d]);
      }
   }

#ifdef HAVE_GDAL
   /*Get rid of transforms and projection functions if the positionnale are already in latlon (GDAL case)*/
   if (Ref->Grid[1]=='\0') {
      if (Ref->Transform)    free(Ref->Transform);    Ref->Transform=NULL;
      if (Ref->InvTransform) free(Ref->InvTransform); Ref->InvTransform=NULL;
      if (Ref->Function) {
         OCTDestroyCoordinateTransformation(Ref->Function);
         Ref->Function=NULL;
      }
      if (Ref->InvFunction) {
         OCTDestroyCoordinateTransformation(Ref->InvFunction);
         Ref->InvFunction=NULL;
      }
      OSRDestroySpatialReference(Ref->Spatial);
      Ref->Spatial=NULL;
   }
#endif

   /*Set secondary gridtype to Y for the project/unproject functions to work correctly*/
   if (Ref->Grid[0]=='W' && Ref->Grid[1]=='\0') {
      Ref->Grid[1]=nx>dx?'Y':'Z';
   }

   return(1);
}
