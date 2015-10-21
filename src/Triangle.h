/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie Tcl de fichiers standards.
 * Fichier      : Triangle.h
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

#ifndef _Triangle_h
#define _Triangle_h

#include "Vector.h"

#define Bary_InterpV(B,V)       (B[0]*V[0]+B[1]*V[1]+B[2]*V[2])
#define Bary_Interp(B,V0,V1,V2) (B[0]*V0+B[1]*V1+B[2]*V2);
#define Bary_Nearest(B)         (B[0]>B[1]?(B[0]>B[2]?0:2):(B[1]>B[2]?1:2))

int Bary_Get(Vect3d B,double X,double Y,double X0,double Y0,double X1,double Y1,double X2,double Y2);
int Bary_InterpPos(Vect3d B,Vect3d P,Vect3d P0,Vect3d P1,Vect3d P2);

#endif
