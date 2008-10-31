/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de gestion des champs RPN en tuiles
 * Creation  : Janvier 2008
 * Auteur    : Jean-Philippe Gauthier
 *
 * Description:
 *
 * License:
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

#ifndef _EZTile_h
#define _EZTile_h

#include "rpnmacros.h"
#include "eerUtils.h"

#define GRIDCACHEMAX 4096
#define TILEMAX      1024

#define EZGrid_IsLoaded(TILE,K) (TILE->Data && !isnan(TILE->Data[K*TILE->NI*TILE->NJ]))

typedef struct {
   int    GID;                      /*EZSCINT Tile grid id (for interpolation)*/
   int    I,J;                      /*Tile starting point within master grid*/
   int    NO;                       /*Tile number*/
   int    KBurn;                    /*Index estampille*/
   int    NI,NJ,NIJ;                /*Tile dimensions*/
   float *Data;                     /*Data pointer*/
} TGridTile;

typedef struct {
   TRPNHeader    H;                 /*RPN Standard file header*/
   int           GID;               /*EZSCINT Tile grid id (for interpolation)*/
   int           IP1,IP2,Master;    /*Grid template identifier*/
   int           Incr;              /*Increasing sorting*/
   int          *Levels;            /*List of encoded levels (Ordered bottom-up)*/
   float        *Data;              /*Data pointer*/
   unsigned long NTI,NTJ;           /*Number of tiles in I and J*/

   unsigned int  NbTiles;           /*Number of tiles*/
   TGridTile    *Tiles;             /*Array of tiles*/
} TGrid;

TGrid* EZGrid_CacheFind(TGrid *Grid);
int    EZGrid_CacheAdd(TGrid *Grid);
int    EZGrid_CacheDel(TGrid *Grid);
int    EZGrid_CopyDesc(int FIdTo,TGrid *Grid);
int    EZGrid_Tile(int FIdTo,int NI, int NJ,int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2);
int    EZGrid_UnTile(int FIdTo,int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2);
int    EZGrid_Free(TGrid *Grid);
TGrid* EZGrid_Get(TGrid *Grid);
TGrid* EZGrid_Read(int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2,int Incr);
TGrid *EZGrid_ReadIdx(int FId,int Key,int Incr);
int    EZGrid_Load(TGrid *Grid,int I0,int J0,int K0,int I1,int J1,int K1);
int    EZGrid_IJGetValue(TGrid *Grid,int I,int J,int K,float *Value);
int    EZGrid_IJGetValues(TGrid *Grid,int I0,int J0,int K0,int I1,int J1,int K1,float *Value);
int    EZGrid_LLGetValue(TGrid *Grid,float Lat,float Lon,int K0,int K1,float *Value);
int    EZGrid_LLGetUVValue(TGrid *GridU,TGrid *GridV,float Lat,float Lon,int K0,int K1,float *UU,float *VV);
int    EZGrid_GetLevelNb(TGrid *Grid);
int    EZGrid_GetLevels(TGrid *Grid,float *Levels,int *Type);
TGrid *EZGrid_TimeInterp(TGrid *Grid0,TGrid *Grid1,int Date);

float* EZGrid_TileBurn(TGrid *Grid,TGridTile *Tile,int K);
float* EZGrid_TileBurnAll(TGrid *Grid,int K);

void EZLock_RPNFile();
void EZUnLock_RPNFile();
void EZLock_RPNField();
void EZUnLock_RPNField();
void EZLock_RPNInt();
void EZUnLock_RPNInt();

int cs_fnomid();
int cs_fstfrm(int Unit);
int cs_fstouv(char *Path,char *Mode);
int cs_fstinl(int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,int *List,int *Nb,int Max);
int cs_fstinf(int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar);
int cs_fstprm(int Unit,int *DateO,int *Deet,int *NPas,int *NI,int *NJ,int *NK,int *NBits,int *Datyp,int *IP1,int *IP2,int *IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int *IG1,int *IG2,int *IG3,int *IG4,int *Swa,int *Lng,int *DLTF,int *UBC,int *EX1,int *EX2,int *EX3);
int cs_fstluk(float *Data,int Idx,int *NI,int *NJ,int *NK);
int cs_fstecr(float *Data,int NPak,int Unit, int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over);

#endif
