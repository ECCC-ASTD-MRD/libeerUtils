/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie Tcl de fichiers standards.
 * Fichier      : Vertex.h
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

#ifndef _Vertex_h
#define _Vertex_h

#include "GeoRef.h"
#include "Def.h"

void   Vertex_Map(Vect2d P[4],double *LX,double *LY,double WX,double WY);
void   VertexGradient(TDef *Def,Vect3d Nr);
float  VertexVal(TDef *Def,int Idx,double X,double Y,double Z);
double VertexValV(TDef *Def,double X,double Y,double Z,Vect3d V);
float  Vertex_ValS(float *Data,char *Mask,int NI,int NJ,double X,double Y,char Geo);
int    VertexLoc(Vect3d **Pos,TDef *Def,Vect3d Vr,double X,double Y,double Z);
void   VertexInterp(Vect3d Pi,Vect3d P0,Vect3d P1,double V0,double V1,double Level);

#endif
