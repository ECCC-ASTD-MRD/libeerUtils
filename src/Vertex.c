/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie Tcl de fichiers standards.
 * Fichier      : Vertex.c
 * Creation     : Aout 2002 - J.P. Gauthier - CMC/CMOE
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

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <Vertex_Map>
 * Creation     : Janvier 2014 J.P. Gauthier - CMC/CMOE
 *
 * But          : Arbitrary quadrilateral location interpolation
 *
 * Parametres    :
 *   <Pt>        : Cell coordinates
 *   <LX>        : Output X grid coordinate 
 *   <LY>        : Output Y grid coordinate 
 *   <WX>        : Longitude
 *   <WY>        : Latitude
 *
 * Retour       : Inside (1 si a l'interieur du domaine).
 *
 * Remarques   :  
 *     The unit cell L(x,y) is oriented as:
 *     L0(x=0,y=0),L1(0,1), L2(1,1), L3(1,0).  The order matters.
 * 
 *     ref: http://math.stackexchange.com/questions/13404/mapping-irregular-quadrilateral-to-a-rectangle
 *---------------------------------------------------------------------------------------------------------------
*/
void Vertex_Map(Vect2d P[4],double *LX,double *LY,double WX,double WY) {

    double ax,a2x,a3x,ay,a1y,a2y,bx,b2x,b3x,by,b1y,b2y;
    double wydy,y3dy,y2dy,wxdx,x1dx,x2dx,wxy,xy0,xy1,xy2,xy3;
    
    if (P[0][1]==P[1][1]) P[0][1]-=1e-5;
    if (P[0][0]==P[3][0]) P[0][0]-=1e-5;
        
    wydy=(WY      - P[0][1]) / (P[0][1] - P[1][1]);
    y3dy=(P[3][1] - P[0][1]) / (P[0][1] - P[1][1]);
    y2dy=(P[2][1] - P[0][1]) / (P[0][1] - P[1][1]);
    wxdx=(WX      - P[0][0]) / (P[0][0] - P[3][0]);
    x1dx=(P[1][0] - P[0][0]) / (P[0][0] - P[3][0]);
    x2dx=(P[2][0] - P[0][0]) / (P[0][0] - P[3][0]);
    wxy=WX * WY;
    xy0=P[0][0] * P[0][1];
    xy1=P[1][0] * P[1][1];
    xy2=P[2][0] * P[2][1];
    xy3=P[3][0] * P[3][1];
    
    ax  = (WX      - P[0][0]) + (P[1][0] - P[0][0]) * wydy;
    a3x = (P[3][0] - P[0][0]) + (P[1][0] - P[0][0]) * y3dy;
    a2x = (P[2][0] - P[0][0]) + (P[1][0] - P[0][0]) * y2dy;
    ay  = (WY      - P[0][1]) + (P[3][1] - P[0][1]) * wxdx;
    a1y = (P[1][1] - P[0][1]) + (P[3][1] - P[0][1]) * x1dx;
    a2y = (P[2][1] - P[0][1]) + (P[3][1] - P[0][1]) * x2dx;
    bx  = wxy - xy0 + (xy1 - xy0) * wydy;
    b3x = xy3 - xy0 + (xy1 - xy0) * y3dy;
    b2x = xy2 - xy0 + (xy1 - xy0) * y2dy;
    by  = wxy - xy0 + (xy3 - xy0) * wxdx;
    b1y = xy1 - xy0 + (xy3 - xy0) * x1dx;
    b2y = xy2 - xy0 + (xy3 - xy0) * x2dx;

    ax /=a3x;
    ay /=a1y;
    a2x/=a3x;
    a2y/=a1y;
    
    *LX = ax + (1 - a2x) * (bx - b3x * ax) / (b2x - b3x * a2x);
    *LY = ay + (1 - a2y) * (by - b1y * ay) / (b2y - b1y * a2y);
}

/*----------------------------------------------------------------------------
 * Nom      : <VertexGradient>
 * Creation : Septembre 2001 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir le gradient d'un point tridimentionne
 *            a l'interieur d'un voxel
 *
 * Parametres :
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
void VertexGradient(TDef *Def,Vect3d Nr) {

   Vect3d v;

   Vect_Assign(v,Nr);
   Nr[0]=VertexVal(Def,-1,v[0]-0.5,v[1],v[2])-VertexVal(Def,-1,v[0]+0.5,v[1],v[2]);
   Nr[1]=VertexVal(Def,-1,v[0],v[1]-0.5,v[2])-VertexVal(Def,-1,v[0],v[1]+0.5,v[2]);
   Nr[2]=VertexVal(Def,-1,v[0],v[1],v[2]-0.5)-VertexVal(Def,-1,v[0],v[1],v[2]+0.5);

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
int VertexLoc(Vect3d **Pos,TDef *Def,Vect3d Vr,double X,double Y,double Z) {

   Vect3d        v00,v01,v10,v11,v0,v1;
   unsigned long i,j,k,k1;
   unsigned long idx0,idx1,idx2,idx3;

   if (X>Def->NI-1 || Y>Def->NJ-1 || Z>Def->NK-1 || X<0 || Y<0 || Z<0) {
      return(0);
   }

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

      Vect_InterpC(v00,Pos[k][idx0],Pos[k1][idx0],Z);
      Vect_InterpC(v10,Pos[k][idx1],Pos[k1][idx1],Z);
      Vect_InterpC(v11,Pos[k][idx2],Pos[k1][idx2],Z);
      Vect_InterpC(v01,Pos[k][idx3],Pos[k1][idx3],Z);
   } else {
      Vect_Assign(v00,Pos[k][idx0]);
      Vect_Assign(v10,Pos[k][idx1]);
      Vect_Assign(v01,Pos[k][idx3]);
      Vect_Assign(v11,Pos[k][idx2]);
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
 * Nom      : <VertexAvg>
 * Creation : Avril 2016 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Moyenne la valeur d'un point de grille masque avec ses voisins
 *
 * Parametres :
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
static inline double VertexAvg(TDef *Def,int Idx,int X,int Y,int Z) {
   
   unsigned long n,i,j,idx,k;
   double        v,val;
   
   n=0;
   val=0.0;
   k=Z*Def->NIJ;
   
   for(j=Y-1;j<=Y+1;j++) {
      for(i=X-1;i<=X+1;i++) {
         idx=j*Def->NI+i;

         if (idx>=0 && idx<Def->NIJ && Def->Mask[k+idx]) {
            if (Idx==-1) {
               Def_GetMod(Def,k+idx,v);
            } else {
               Def_Get(Def,Idx,k+idx,v)
            }
            if (DEFVALID(Def,v)) {
               val+=v;
               n++;
            }
         }
      }
   }
   return(n?val/n:Def->NoData);
}

/*----------------------------------------------------------------------------
 * Nom      : <VertexVal>
 * Creation : Septembre 2001 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Interpoler lineairement la valeur d'un point
 *            a l'interieur d'un voxel pour une les deux composante (UV=vitesse)
 *
 * Parametres :
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
float VertexVal(TDef *Def,int Idx,double X,double Y,double Z) {

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

   // If a mask exists average the masked points with the ones around
   if (Def->Mask) {
      if (!Def->Mask[idx[0]]) cube[0][0]=VertexAvg(Def,Idx,i,  j,  k);
      if (!Def->Mask[idx[1]]) cube[0][1]=VertexAvg(Def,Idx,i+1,j,  k);
      if (!Def->Mask[idx[2]]) cube[0][2]=VertexAvg(Def,Idx,i+1,j+1,k);
      if (!Def->Mask[idx[3]]) cube[0][3]=VertexAvg(Def,Idx,i,  j+1,k);
   }
   
   // If either value is nodata then interpolation will be nodata as well
   if (!DEFVALID(Def,cube[0][0]) || !DEFVALID(Def,cube[0][1]) || !DEFVALID(Def,cube[0][2]) || !DEFVALID(Def,cube[0][3])) {
      return(Def->NoData);
   }
     
   // 3D Interpolation case
   if (Z>TINY_VALUE) {

      idx[0]+=idxk;
      idx[1]+=idxk;
      idx[3]+=idxk;
      idx[2]+=idxk;
      k++;
      
      if (Idx==-1) {
         Def_GetQuadMod(Def,idx,cube[1]);
      } else {
         Def_GetQuad(Def,Idx,idx,cube[1]);
      }
      
      // Check for masked values
      if (Def->Mask) {
         if (!Def->Mask[idx[0]]) cube[1][0]=VertexAvg(Def,Idx,i,  j,  k);
         if (!Def->Mask[idx[1]]) cube[1][1]=VertexAvg(Def,Idx,i+1,j,  k);
         if (!Def->Mask[idx[2]]) cube[1][2]=VertexAvg(Def,Idx,i+1,j+1,k);
         if (!Def->Mask[idx[3]]) cube[1][3]=VertexAvg(Def,Idx,i,  j+1,k);
      }
      
      // If either value is nodata then interpolation will be nodata as well
      if (!DEFVALID(Def,cube[1][0]) || !DEFVALID(Def,cube[1][1]) || !DEFVALID(Def,cube[1][2]) || !DEFVALID(Def,cube[1][3])) {
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

double VertexValV(TDef *Def,double X,double Y,double Z,Vect3d V) {

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
   if (!DEFVALID(Def,cube[0][0][0]) || !DEFVALID(Def,cube[0][0][1]) || !DEFVALID(Def,cube[0][0][2]) || !DEFVALID(Def,cube[0][0][3])) {
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
      if (!DEFVALID(Def,cube[0][1][0]) || !DEFVALID(Def,cube[0][1][1]) || !DEFVALID(Def,cube[0][1][2]) || !DEFVALID(Def,cube[0][1][3])) {
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

static inline float VertexAvgS(float *Data,char *Mask,int NI,int NJ,int X,int Y) {
   
   unsigned long n=0,i,j,idx;
   float         val=0.0;
   
   i=X;j=Y;
   j-=1;i-=1; idx=j*NI+i; if (j>=0 && i>=0 && Mask[idx]) { val+=Data[idx]; n++; }
   i++;       idx++;      if (j>=0         && Mask[idx]) { val+=Data[idx]; n++; }
   i++;       idx++;      if (j>=0 && i<NI && Mask[idx]) { val+=Data[idx]; n++; }
   j++;       idx+=NI;    if (        i<NI && Mask[idx]) { val+=Data[idx]; n++; }
   i-=2;      idx-=2;     if (        i>=0 && Mask[idx]) { val+=Data[idx]; n++; }
   j++;       idx+=NI;    if (j<NJ && i>=0 && Mask[idx]) { val+=Data[idx]; n++; }
   i++;       idx++;      if (j<NJ         && Mask[idx]) { val+=Data[idx]; n++; }
   i++;       idx++;      if (j<NJ && i<NI && Mask[idx]) { val+=Data[idx]; n++; }
   
   return(n?val/n:0.0);
}

float Vertex_ValS(float *Data,char *Mask,int NI,int NJ,double X,double Y,char Geo) {
   
   double        cell[4],d;
   unsigned long i,j,idx[4];

   if (!Data || X>NI-1 || Y>NJ-1 || X<0 || Y<0) {
      return(0);
   }

   i=X;X-=i;
   j=Y;Y-=j;

   // Get gridpoint indexes
   idx[0]=j*NI+i;
   idx[1]=idx[0]+1;
   idx[3]=(j==NJ-1)?idx[0]:idx[0]+NI;
   idx[2]=idx[3]+1;

   cell[0]=Data[idx[0]];
   cell[1]=Data[idx[1]];
   cell[2]=Data[idx[2]];
   cell[3]=Data[idx[3]];
      
   if (Mask) {
      if (!Mask[idx[0]]) cell[0]=VertexAvgS(Data,Mask,NI,NJ,i,  j);
      if (!Mask[idx[1]]) cell[1]=VertexAvgS(Data,Mask,NI,NJ,i+1,j);
      if (!Mask[idx[2]]) cell[2]=VertexAvgS(Data,Mask,NI,NJ,i+1,j+1);
      if (!Mask[idx[3]]) cell[3]=VertexAvgS(Data,Mask,NI,NJ,i,  j+1);      
   }
   
   // Interpolate over X
   if (X>TINY_VALUE) {
      if (Geo) {
         // If interpolation on coordinates, check for wrap-around cell[0] and cell[3] contain longitude
         d=cell[0]-cell[1];
         if (d>=180) cell[1]+=360.0;
         if (d<-180) cell[1]-=360.0;
         d=cell[3]-cell[2];
         if (d>=180) cell[2]+=360.0;
         if (d<-180) cell[2]-=360.0;
      }
      cell[0]=ILIN(cell[0],cell[1],X);
      cell[3]=ILIN(cell[3],cell[2],X);
   }

   // Interpolate over Y
   if (Y>TINY_VALUE) {
      if (Geo) {
         // If interpolation on coordinates, check for wrap-around cell[0] and cell[3] contain longitude
         d=cell[0]-cell[3];
         if (d>=180) cell[3]+=360.0;
         if (d<-180) cell[3]-=360.0;
      }
      cell[0]=ILIN(cell[0],cell[3],Y);
   }

   return(cell[0]);
}
