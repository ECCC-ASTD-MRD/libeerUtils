/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie Tcl de fichiers standards.
 * Fichier      : Vertex.c
 * Creation     : Aout 2002 - J.P. Gauthier - CMC/CMOE
 * Revision     : $Id$
 *
 * Description: Fonctions de manipulations et de traitements des vertex.
 *
 * Remarques :
 *
 * License   :
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
 *==============================================================================
 */

#include "Vertex.h"

int Bary_Get(Vect3d B,double X,double Y,double X0,double Y0,double X1,double Y1,double X2,double Y2) {

   double b,x0,x1,x2,y0,y1,y2;

   x0=X0-X; y0=Y0-Y;
   x1=X1-X; y1=Y1-Y;
   x2=X2-X; y2=Y2-Y;

   b=1.0/((X1-X0)*(Y2-Y0)-(X2-X0)*(Y1-Y0));
   B[0]=(x1*y2-x2*y1)*b;
   B[1]=(x2*y0-x0*y2)*b;
   B[2]=(x0*y1-x1*y0)*b;

   return(B[0]>=0 && B[1]>=0 && B[2]>=0);
}

int Bary_Interp(Vect3d B,Vect3d P,Vect3d P0,Vect3d P1,Vect3d P2) {

   P[0]=B[0]*P0[0]+B[1]*P1[0]+B[2]*P2[0];
   P[1]=B[0]*P0[1]+B[1]*P1[1]+B[2]*P2[1];
   P[2]=B[0]*P0[2]+B[1]*P1[2]+B[2]*P2[2];

   return(1);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <Vertex_Map>
 * Creation     : Janvier 2014 J.P. Gauthier - CMC/CMOE
 *
 * But          : Arbitrary quadrilateral location interpolation
 *
 * Parametres    :
 *   <X>         : Cell longitude coordinate vector
 *   <Y>         : Cell latitude coordinate vector
 *   <LX>        : Output X grid coordinate 
 *   <LY>        : Output Y grid coordinate 
 *   <WX>        : Longitude
 *   <WY>        : Latitude
 *
 * Retour       : Inside (1 si a l'interieur du domaine).
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
void Vertex_Map(double X[4] ,double Y[4],double *LX,double *LY,double WX,double WY) {

   double a,b,c,d,u,v;
   
   // Quadratic equation coeffs
   c = (Y[0]-WY) * (X[3]-WX) - (X[0]-WX) * (Y[3]-WY);
   b = (Y[0]-WY) * (X[2]-X[3]) + (Y[1]-Y[0]) * (X[3]-WX) - (X[0]-WX) * (Y[2]-Y[3]) - (X[1]-X[0]) * (Y[3]-WY);
   a = (Y[1]-Y[0]) * (X[2]-X[3]) - (X[1]-X[0]) * (Y[2]-Y[3]);

   // Compute u = (-b+sqrt(b^2-4ac))/(2a)
   d = b*b-4*a*c;
   u = (-b-sqrt(d))/(2*a);

   // Compute v
   a = X[0]+(X[1]-X[0])*u;
   b = X[3]+(X[2]-X[3])*u;
   v = (WX-a)/(b-a);
                        
   *LX=u;
   *LY=v;
}

void Vertex_Map2(double X[4] ,double Y[4],double *LX,double *LY,double WX,double WY) {

   double M[4][4]={{1.0,-1.0,-1.0,1.0},{0.0,1.0,0.0,-1.0},{0.0,0.0,0.0,1.0},{0.0,0.0,1.0,-1.0}};
   double aa,bb,cc,m,l,a[4],b[4];
   
   a[0]=M[0][0]*X[0]+M[1][0]*X[1]+M[2][0]*X[2]+M[3][0]*X[3];
   a[1]=M[0][1]*X[0]+M[1][1]*X[1]+M[2][1]*X[2]+M[3][1]*X[3];
   a[2]=M[0][2]*X[0]+M[1][2]*X[1]+M[2][2]*X[2]+M[3][2]*X[3];
   a[3]=M[0][3]*X[0]+M[1][3]*X[1]+M[2][3]*X[2]+M[3][3]*X[3];
   
   b[0]=M[0][0]*Y[0]+M[1][0]*Y[1]+M[2][0]*Y[2]+M[3][0]*Y[3];
   b[1]=M[0][1]*Y[0]+M[1][1]*Y[1]+M[2][1]*Y[2]+M[3][1]*Y[3];
   b[2]=M[0][2]*Y[0]+M[1][2]*Y[1]+M[2][2]*Y[2]+M[3][2]*Y[3];
   b[3]=M[0][3]*Y[0]+M[1][3]*Y[1]+M[2][3]*Y[2]+M[3][3]*Y[3];

   //quadratic equation coeffs, aa*mm^2+bb*m+cc=0
    aa = a[3]*b[2] - a[2]*b[3];
    bb = a[3]*b[0] -a[0]*b[3] + a[1]*b[2] - a[2]*b[1] + WX*b[3] - WY*a[3];
    cc = a[1]*b[0] -a[0]*b[1] + WX*b[1] - WY*a[1];
 
    //compute m = (-b+sqrt(b^2-4ac))/(2a)
    m = (-bb+sqrt(bb*bb - 4*aa*cc))/(2*aa);
 
    //compute l
    l = (WX-a[0]-a[2]*m)/(a[1]+a[3]*m);

   *LX=l;
   *LY=m;
}

/*----------------------------------------------------------------------------
 * Nom      : <VertexGradient>
 * Creation : Septembre 2001 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir le gradient d'un point tridimentionne
 *            a l'interieur d'un voxel
 *
 * Parametres :
 *   <Ref>    : Georeference
 *   <Def>    : Definitions des donnees
 *   <Nr>     : Point a l'interieur du voxel
 *
 * Retour:
 *   <Nr>     : Gradient normalise
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
void VertexGradient(TGeoRef *Ref,TDef *Def,Vect3d Nr) {

   Vect3d v;

   Vect_Assign(v,Nr);
   Nr[0]=VertexVal(Ref,Def,-1,v[0]-0.5,v[1],v[2])-VertexVal(Ref,Def,-1,v[0]+0.5,v[1],v[2]);
   Nr[1]=VertexVal(Ref,Def,-1,v[0],v[1]-0.5,v[2])-VertexVal(Ref,Def,-1,v[0],v[1]+0.5,v[2]);
   Nr[2]=VertexVal(Ref,Def,-1,v[0],v[1],v[2]-0.5)-VertexVal(Ref,Def,-1,v[0],v[1],v[2]+0.5);

//   Vect_Mul(Nr,Nr,v);
}

/*----------------------------------------------------------------------------
 * Nom      : <VertexInterp>
 * Creation : Septembre 2001 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Interpoler lineairement la position de coupe d'une isosurface
 *            sur un cote entre deux vertex
 *
 * Parametres :
 *   <Pi>     : Point resultant
 *   <P0>     : Point 1
 *   <P1>     : Point 2
 *   <V0>     : Valeur au point P0
 *   <V1>     : Valeur au point P1
 *   <Level>  : Niveau
 *
 * Retour:
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
void VertexInterp(Vect3d Pi,Vect3d P0,Vect3d P1,double V0,double V1,double Level) {

   double mu;

   if (ABS(Level-V0) < TINY_VALUE) {
      Vect_Assign(Pi,P0);
      return;
   }

   if (ABS(Level-V1) < TINY_VALUE) {
      Vect_Assign(Pi,P1);
      return;
   }
   if (ABS(V0-V1) < TINY_VALUE) {
      Vect_Assign(Pi,P0);
      return;
   }

   mu = (Level-V0)/(V1-V0);

   Pi[0]=P0[0] + mu*(P1[0] - P0[0]);
   Pi[1]=P0[1] + mu*(P1[1] - P0[1]);
   Pi[2]=P0[2] + mu*(P1[2] - P0[2]);
}

/*----------------------------------------------------------------------------
 * Nom      : <VertexLoc>
 * Creation : Mars 2002 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Interpoler la position d'un point de grille en X,Y,Z.
 *
 * Parametres :
 *  <Ref>     : Georeference
 *  <Def>     : Definition des donnees
 *  <Vr>      : Vertex resultant
 *  <X>       : Coordonnee en X ([0 NI-1])
 *  <Y>       : Coordonnee en Y ([0 NJ-1])
 *  <Z>       : Coordonnee en Z ([0 NK-1])
 *
 * Retour:
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
int VertexLoc(TGeoRef *Ref,TDef *Def,Vect3d Vr,double X,double Y,double Z) {

   Vect3d        v00,v01,v10,v11,v0,v1,**pos;
   unsigned long i,j,k,k1;
   unsigned long idx0,idx1,idx2,idx3;

   if (X>Def->NI-1 || Y>Def->NJ-1 || Z>Def->NK-1 || X<0 || Y<0 || Z<0) {
      return(0);
   }

   pos=Ref->Pos;
   i=X;X-=i;
   j=Y;Y-=j;
   k=Z;Z-=k;

   /*Get gridpoint indexes*/
   idx0=Def->Idx+j*Def->NI+i;
   idx1=idx0+1;
   idx3=(j==Def->NJ-1)?idx0:idx0+Def->NI;
   idx2=idx3+1;

   /*3D Interpolation case*/
   if (Z>TINY_VALUE) {

      k1=k+1;

      Vect_InterpC(v00,pos[k][idx0],pos[k1][idx0],Z);
      Vect_InterpC(v10,pos[k][idx1],pos[k1][idx1],Z);
      Vect_InterpC(v11,pos[k][idx2],pos[k1][idx2],Z);
      Vect_InterpC(v01,pos[k][idx3],pos[k1][idx3],Z);
   } else {
      Vect_Assign(v00,pos[k][idx0]);
      Vect_Assign(v10,pos[k][idx1]);
      Vect_Assign(v01,pos[k][idx3]);
      Vect_Assign(v11,pos[k][idx2]);
   }

   /*Interpolate over X*/
   if (X>TINY_VALUE) {
      Vect_InterpC(v0,v00,v10,X);
      Vect_InterpC(v1,v01,v11,X);
   }  else {
      Vect_Assign(v0,v00);
      Vect_Assign(v1,v01);
   }

   /*Interpolate over Y*/
   if (Y>TINY_VALUE) {
      Vect_InterpC(Vr,v0,v1,Y);
   } else {
      Vect_Assign(Vr,v0);
   }

   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <VertexVal>
 * Creation : Septembre 2001 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Interpoler lineairement la valeur d'un point
 *            a l'interieur d'un voxel pour une les deux composante (UV=vitesse)
 *
 * Parametres :
 *   <Ref>    : Georeference
 *   <Def>    : Definition des donnees
 *   <Idx>    : Composantes (-1=mode)
 *   <X>      : Position en X
 *   <Y>      : Position en Y
 *   <Z>      : Position en Z (z<0, Interpolation 2D seulement)
 *
 * Retour:
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
float VertexVal(TGeoRef *Ref,TDef *Def,int Idx,double X,double Y,double Z) {

   double        cube[2][4];
   unsigned long i,j,k,idx[4],idxk;

   if (X>Def->NI-1 || Y>Def->NJ-1 || Z>Def->NK-1 || X<0 || Y<0 || Z<0) {
      return(0);
   }

   i=X;X-=i;
   j=Y;Y-=j;
   k=Z;Z-=k;

   // Get gridpoint indexes
   idxk=Def->NIJ;

   idx[0]=(k?idxk*k:0)+j*Def->NI+i;
   idx[1]=idx[0]+1;
   idx[3]=(j==Def->NJ-1)?idx[0]:idx[0]+Def->NI;
   idx[2]=idx[3]+1;
   if (Idx==-1) {
      Def_GetQuadMod(Def,idx,cube[0]);
   } else {
      Def_GetQuad(Def,Idx,idx,cube[0]);
   }
   
   // If either value is nodata then interpolation will be nodata as well
   if (cube[0][0]==Def->NoData || cube[0][1]==Def->NoData || cube[0][2]==Def->NoData || cube[0][3]==Def->NoData) {
      return(Def->NoData);
   }
   
   // 3D Interpolation case
   if (Z>TINY_VALUE) {

      idx[0]+=idxk;
      idx[1]+=idxk;
      idx[3]+=idxk;
      idx[2]+=idxk;
      if (Idx==-1) {
         Def_GetQuadMod(Def,idx,cube[1]);
      } else {
         Def_GetQuad(Def,Idx,idx,cube[1]);
      }
      // If either value is nodata then interpolation will be nodata as well
      if (cube[1][0]==Def->NoData || cube[1][1]==Def->NoData || cube[1][2]==Def->NoData || cube[1][3]==Def->NoData) {
         return(Def->NoData);
      }

      cube[0][0]=ILIN(cube[0][0],cube[1][0],Z);
      cube[0][1]=ILIN(cube[0][1],cube[1][1],Z);
      cube[0][2]=ILIN(cube[0][2],cube[1][2],Z);
      cube[0][3]=ILIN(cube[0][3],cube[1][3],Z);
   }

   /*Interpolate over X*/
   if (X>TINY_VALUE) {
      cube[0][0]=ILIN(cube[0][0],cube[0][1],X);
      cube[0][3]=ILIN(cube[0][3],cube[0][2],X);
   }

   /*Interpolate over Y*/
   if (Y>TINY_VALUE) {
      cube[0][0]=ILIN(cube[0][0],cube[0][3],Y);
   }

   return(cube[0][0]);
}

double VertexValV(TGeoRef *Ref,TDef *Def,double X,double Y,double Z,Vect3d V) {

   double        cube[3][2][4];
   unsigned long i,j,k,idx[4],idxk;

   if (X>Def->NI-1 || Y>Def->NJ-1 || Z>Def->NK-1 || X<0 || Y<0 || Z<0) {
      return(0);
   }

   i=X;X-=i;
   j=Y;Y-=j;
   k=Z;Z-=k;

   // Get gridpoint indexes
   idxk=Def->NIJ;

   idx[0]=(k?idxk*k:0)+j*Def->NI+i;
   idx[1]=idx[0]+1;
   idx[3]=(j==Def->NJ-1)?idx[0]:idx[0]+Def->NI;
   idx[2]=idx[3]+1;
   
   Def_GetQuad(Def,0,idx,cube[0][0]);
   Def_GetQuad(Def,1,idx,cube[1][0]);
   if (Def->Data[2]) Def_GetQuad(Def,2,idx,cube[2][0]);
   
   // If either value is nodata then interpolation will be nodata as well
   if (cube[0][0][0]==Def->NoData || cube[0][0][1]==Def->NoData || cube[0][0][2]==Def->NoData || cube[0][0][3]==Def->NoData) {
      return(Def->NoData);
   }
   
   // 3D Interpolation case
   if (Z>TINY_VALUE) {

      idx[0]+=idxk;
      idx[1]+=idxk;
      idx[3]+=idxk;
      idx[2]+=idxk;
      Def_GetQuad(Def,0,idx,cube[0][1]);
      Def_GetQuad(Def,1,idx,cube[1][1]);
      if (Def->Data[2]) Def_GetQuad(Def,2,idx,cube[2][1]);
      
      // If either value is nodata then interpolation will be nodata as well
      if (cube[0][1][0]==Def->NoData || cube[0][1][1]==Def->NoData || cube[0][1][2]==Def->NoData || cube[0][1][3]==Def->NoData) {
         return(Def->NoData);
      }

      cube[0][0][0]=ILIN(cube[0][0][0],cube[0][1][0],Z);
      cube[0][0][1]=ILIN(cube[0][0][1],cube[0][1][1],Z);
      cube[0][0][2]=ILIN(cube[0][0][2],cube[0][1][2],Z);
      cube[0][0][3]=ILIN(cube[0][0][3],cube[0][1][3],Z);
      cube[1][0][0]=ILIN(cube[1][0][0],cube[1][1][0],Z);
      cube[1][0][1]=ILIN(cube[1][0][1],cube[1][1][1],Z);
      cube[1][0][2]=ILIN(cube[1][0][2],cube[1][1][2],Z);
      cube[1][0][3]=ILIN(cube[1][0][3],cube[1][1][3],Z);
      if (Def->Data[2]) {
         cube[2][0][0]=ILIN(cube[2][0][0],cube[2][1][0],Z);
         cube[2][0][1]=ILIN(cube[2][0][1],cube[2][1][1],Z);
         cube[2][0][2]=ILIN(cube[2][0][2],cube[2][1][2],Z);
         cube[2][0][3]=ILIN(cube[2][0][3],cube[2][1][3],Z);
      }
   }
   
   V[0]=cube[0][0][0];
   V[1]=cube[1][0][0];
   V[2]=Def->Data[2]?cube[2][0][0]:0.0;
   
   /*Interpolate over X*/
   if (X>TINY_VALUE) {
      V[0]=cube[0][0][0]=ILIN(cube[0][0][0],cube[0][0][1],X);
           cube[0][0][3]=ILIN(cube[0][0][3],cube[0][0][2],X);
      V[1]=cube[1][0][0]=ILIN(cube[1][0][0],cube[1][0][1],X);
           cube[1][0][3]=ILIN(cube[1][0][3],cube[1][0][2],X);
      if (Def->Data[2]) {
         V[2]=cube[2][0][0]=ILIN(cube[2][0][0],cube[2][0][1],X);
              cube[2][0][3]=ILIN(cube[2][0][3],cube[2][0][2],X);
      }
   }

   /*Interpolate over Y*/
   if (Y>TINY_VALUE) {
      V[0]=ILIN(cube[0][0][0],cube[0][0][3],Y);
      V[1]=ILIN(cube[1][0][0],cube[1][0][3],Y);
      if (Def->Data[2]) {
         V[2]=ILIN(cube[2][0][0],cube[2][0][3],Y);
      }
   }
   
   return(0);
}
