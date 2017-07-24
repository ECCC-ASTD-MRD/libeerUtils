/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie Tcl de fichiers standards.
 * Fichier      : Triangle.h
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

#ifndef _Triangle_h
#define _Triangle_h

#include "Vector.h"

#define BARY_PREC 1e-5

#define Bary_InterpV(B,V)       (B[0]*V[0]+B[1]*V[1]+B[2]*V[2])
#define Bary_Interp(B,V0,V1,V2) (B[0]*V0+B[1]*V1+B[2]*V2);
#define Bary_Nearest(B)         (B[0]>B[1]?(B[0]>B[2]?0:2):(B[1]>B[2]?1:2))

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <Bary_Get>
 * Creation     : Janvier 2014 J.P. Gauthier - CMC/CMOE
 *
 * But          : Get barycentric coordinate within a triangle of an xy coordinate
 *
 * Parametres    :
 *   <B>         : Barycentric coordinare
 *   <W>         : Barycentric weight
 *   <X>         : X coordinate to interpolate
 *   <Y>         : Y coordinate to interpolate
 *   <X0>        : Triangle 1st point X
 *   <Y0>        : Triangle 1st point Y
 *   <X1>        : Triangle 2nd point X
 *   <Y1>        : Triangle 2nd point Y
 *   <X2>        : Triangle 3rd point X
 *   <Y2>        : Triangle 3rd point Y
 *
 * Retour       : Inside (1 if within triangle).
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
static inline int Bary_Get(Vect3d B,double W,double X,double Y,double X0,double Y0,double X1,double Y1,double X2,double Y2) {

   double x0,x1,x2,y0,y1,y2;

   x0=X0-X; y0=Y0-Y;
   x1=X1-X; y1=Y1-Y;
   x2=X2-X; y2=Y2-Y;

   if (W==0.0) 
      W=1.0/((X1-X0)*(Y2-Y0)-(X2-X0)*(Y1-Y0));
   
   B[0]=(x1*y2-x2*y1)*W;
   B[1]=(x2*y0-x0*y2)*W;
   B[2]=1.0-B[0]-B[1];

   return(B[0]>=-BARY_PREC && B[1]>=-BARY_PREC && B[2]>=-BARY_PREC);
}

/*--------------------------------------------------------------------------------------------------------------
 * Nom          : <Bary_InterpPos>
 * Creation     : Janvier 2014 J.P. Gauthier - CMC/CMOE
 *
 * But          : interpolate value at barycentric coordinate
 *
 * Parametres    :
 *   <B>         : Barycentric coordinare
 *   <P>         : X coordinate to interpolate
 *   <P0>        : Triangle 1st point coordinates
 *   <P1>        : Triangle 2nd point coordinates
 *   <P2>        : Triangle 3rd point coordinates
 *
 * Retour       : Inside (1 if within triangle).
 *
 * Remarques   :
 *
 *---------------------------------------------------------------------------------------------------------------
*/
static inline int Bary_InterpPos(Vect3d B,Vect3d P,Vect3d P0,Vect3d P1,Vect3d P2) {

   P[0]=Bary_Interp(B,P0[0],P1[0],P2[0]);
   P[1]=Bary_Interp(B,P0[1],P1[1],P2[1]);
   P[2]=Bary_Interp(B,P0[2],P1[2],P2[2]);

   return(1);
}

#endif
