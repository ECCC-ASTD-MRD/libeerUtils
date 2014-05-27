/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Lecture et traitements de divers fichiers de donnees
 * Fichier      : tclUtils.h
 * Creation     : Avril 2007 - J.P. Gauthier
 *
 * Description  : Fonctions generales d'utilites courantes.
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
#ifndef _tclUtils_h
#define _tclUtils_h

#include "tcl.h"

#ifndef _AIX_

//#include "tclPort.h"

typedef int (TclY_HashFreeEntryDataFunc) (ClientData EntryData);

int   TclY_Get0IntFromObj(Tcl_Interp *Interp,Tcl_Obj *Obj,int *Var);
int   TclY_Get0LongFromObj(Tcl_Interp *Interp,Tcl_Obj *Obj,long *Var);
int   TclY_Get0DoubleFromObj(Tcl_Interp *Interp,Tcl_Obj *Obj,double *Var);
FILE* TclY_ChannelOrSocketOpen(Tcl_Interp *Interp,Tcl_Obj *Obj,char *Mode);
int   TclY_SocketTimeOut(Tcl_Interp *Interp,Tcl_Obj *Obj,int *Receive,int *Send);
int   TclY_ListObjFind(Tcl_Interp *Interp,Tcl_Obj *List,Tcl_Obj *Item);

Tcl_HashEntry* TclY_CreateHashEntry(Tcl_HashTable *Table,const char *Name,int *new);
Tcl_HashEntry* TclY_FindHashEntry(Tcl_HashTable *Table,const char *Name);
void  TclY_DeleteHashEntry(Tcl_HashEntry *Entry);
int   TclY_HashAll(Tcl_Interp *Interp,Tcl_HashTable *Table);
void* TclY_HashGet(Tcl_HashTable *Table,const char *Name);
void* TclY_HashDel(Tcl_HashTable *Table,const char *Name);
void* TclY_HashPut(Tcl_Interp *Interp,Tcl_HashTable *Table,const char *Name,unsigned int Size);
void* TclY_HashReplace(Tcl_Interp *Interp,Tcl_HashTable *Table,const char *Name,void *Data);
int   TclY_HashSet(Tcl_Interp *Interp,Tcl_HashTable *Table,const char *Name,void *Data);
void  TclY_HashWipe(Tcl_HashTable *Table,TclY_HashFreeEntryDataFunc *TclY_HashFreeEntryData);

void TclY_LockHash();
void TclY_UnlockHash();

#endif

#endif
