/*=============================================================================
* Environnement Canada
* Centre Meteorologique Canadian
* 2100 Trans-Canadienne
* Dorval, Quebec
*
* Projet    : Projection diverses de la carte vectorielle.
* Fichier   : eerStruct.h
* Creation  : Janvier 2011 - J.P. Gauthier
*
* Description: Fichier de definition de structures de donnees
*
* Remarques :
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
*==============================================================================
*/

#ifndef _eerStruct_h
#define _eerStruct_h

#include "Vector.h"

typedef struct T3DArray {
   double  Value;
   int     Size;
   Vect3d *Data;
} T3DArray;

typedef struct TList {
   void         *Data;
   struct TList *Next;
   struct TList *Prev;
} TList;

T3DArray *T3DArray_Alloc(double Value,int Size);
void      T3DArray_Free(T3DArray *Array);

typedef int (TList_FindProc)(void *Data0,void *Data1);
typedef int (TList_FreeProc)(void *Data0);

TList* TList_Add(TList *List,void *Data);
TList* TList_Del(TList *List,void *Data);
TList* TList_Find(TList *List,TList_FindProc *Proc,void *Data);
void   TList_Clear(TList *List,TList_FreeProc *Proc);

#endif