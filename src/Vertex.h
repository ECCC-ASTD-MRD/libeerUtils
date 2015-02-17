/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie Tcl de fichiers standards.
 * Fichier   : Vertex.h
 * Creation  : Aout 2002 - J.P. Gauthier - CMC/CMOE
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

#ifndef _Vertex_h
#define _Vertex_h

#include "GeoRef.h"
#include "Def.h"

#define Bary_Interp1D(B,V)  (B[0]*V[0]+B[1]*V[1]+B[2]*V[2])

int Bary_Get(Vect3d B,double X,double Y,double X0,double Y0,double X1,double Y1,double X2,double Y2);
int Bary_Interp(Vect3d B,Vect3d P,Vect3d P0,Vect3d P1,Vect3d P2);

void Vertex_Map(double X[4] ,double Y[4],double *LX,double *LY,double WX,double WY);

void   VertexGradient(TGeoRef *Ref,TDef *Def,Vect3d Nr);
int    VertexLoc(TGeoRef *Ref,TDef *Def,Vect3d Vr,double X,double Y,double Z);
float  VertexVal(TGeoRef *Ref,TDef *Def,int Idx,double X,double Y,double Z);
void   VertexInterp(Vect3d Pi,Vect3d P0,Vect3d P1,double V0,double V1,double Level);
double VertexValV(TGeoRef *Ref,TDef *Def,double X,double Y,double Z,Vect3d V);

#endif
