/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Manipulation des fichiers xml dictionnaire des variables
 * Fichier      : Dict.h
 * Creation     : Mai 2014 - J.P. Gauthier
 *
 * Description  : Basé fortement sur le code d'Yves Chartier afin de convertir
 *                Le binaire un fonctions de librairies.
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
#include "eerUtils.h"
#include "eerStruct.h"

#define DICT_SHORT  0
#define DICT_LONG   1
#define DICT_NOTSET 1e+300

#define DICT_ALL      0x00
#define DICT_OBSOLETE 0x01
#define DICT_CURRENT  0x02
#define DICT_FUTURE   0x04

#define DICT_INTEGER  0x08
#define DICT_REAL     0x10
#define DICT_LOGICAL  0x20
#define DICT_CODE     0x40

#define DICT_EXACT    0
#define DICT_GLOB     1

#define DICT_ASCII     0
#define DICT_UTF8      1 
#define DICT_ISO8859_1 2

typedef struct {
   char   Origin[32];           // Origin ot the variable
   char   Name[8];              // NOMVAR
   int    Nature;               // Mask for state and nature of variable
   char   Short[2][128];        // Short description in both language
   char   Long[2][128];         // Long description in both language
   char   Units[32];            // Units
   double Min,Max,Magnitude;    // Range of values and applied factor
   int    Codes[64];            // List of codes for coded variable
   char   Meanings[64][64];     // List of associated meanings
   int    NCodes;               // Number of codes
   int    IP1,IP2,IP3;          // Specific IP values
} TDictVar;

typedef struct {
   char Origin[32];             // Origin ot the variable
   char Name[3];                // TYPVAR
   char Short[2][128];          // Short description in both language
   char Long[2][128];           // Long description in both language 
} TDictType;

int        Dict_Parse(char *Filename);
void       Dict_SetEncoding(int Encoding);
void       Dict_SetSearch(int SearchMode,int SearchState,char *SearchOrigin,int SearchIP1,int SearchIP2,int SearchIP3);
TDictVar  *Dict_GetVar(char *Var);
TDictType *Dict_GetType(char *Type);
TDictVar  *Dict_IterateVar(TList **Iterator,char *Var);
TDictType *Dict_IterateType(TList **Iterator,char *Type);
void       Dict_PrintVar(TDictVar *DVar,int Format,char *Language);
void       Dict_PrintVars(char *Var,int Format,char *Language);
void       Dict_PrintType(TDictType *DType,int Format,char *Language);
void       Dict_PrintTypes(char *Type,int Format,char *Language);
int        Dict_SortVar(void *Data0,void *Data1);
int        Dict_SortType(void *Data0,void *Data1);
int        Dict_CheckVar(void *Data0,void *Data1);
int        Dict_CheckType(void *Data0,void *Data1);

