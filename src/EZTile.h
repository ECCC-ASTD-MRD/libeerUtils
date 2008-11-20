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

#include "eerUtils.h"

#define GRIDCACHEMAX 4096
#define TILEMAX      1024

#define EZGrid_IsLoaded(TILE,Z)      (TILE->Data && TILE->Data[(int)Z])
#define EZGrid_TileValue(TILE,X,Y,Z) (TILE->Data[(int)Z][((int)Y-TILE->J)*TILE->NI+((int)X-TILE->I)])

typedef struct {
   int     GID;                      /*EZSCINT Tile grid id (for interpolation)*/
   int     I,J;                      /*Tile starting point within master grid*/
   int     NO;                       /*Tile number*/
   int     KBurn;                    /*Index estampille*/
   int     NI,NJ,NIJ;                /*Tile dimensions*/
   float **Data;                     /*Data pointer*/
} TGridTile;

struct TGrid;

typedef struct TGrid {
   TRPNHeader    H;                  /*RPN Standard file header*/
   int           GID;                /*EZSCINT Tile grid id (for interpolation)*/
   int           IP1,IP2,IP3,Master; /*Grid template identifier*/
   int           Incr;               /*Increasing sorting*/
   int           Factor;             /*Increasing sorting*/
   int          *Levels;             /*List of encoded levels (Ordered bottom-up)*/
   float        *Data;               /*Data pointer*/
   unsigned long NTI,NTJ;            /*Number of tiles in I and J*/

   float         FT0,FT1;            /*Time interpolation factor*/
   struct TGrid *T0,*T1;             /*Time interpolation strat and end grid*/

   unsigned int  NbTiles;            /*Number of tiles*/
   TGridTile    *Tiles;              /*Array of tiles*/
} TGrid;

int    EZGrid_CopyDesc(const int FIdTo,TGrid* restrict const Grid);
TGrid *EZGrid_New();
int    EZGrid_Free(TGrid* restrict const Grid);
TGrid* EZGrid_Get(TGrid* restrict const Grid);
TGrid* EZGrid_Read(int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2,int Incr);
TGrid *EZGrid_ReadIdx(int FId,int Key,int Incr);
int    EZGrid_Load(const TGrid* restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1);
int    EZGrid_GetLevelNb(const TGrid* restrict const Grid);
int    EZGrid_GetLevels(const TGrid* restrict const Grid,float* restrict Levels,int* restrict Type);

void   EZGrid_Factor(TGrid* restrict Grid,float Factor);
int    EZGrid_GetValue(const TGrid* restrict const Grid,int I,int J,int K0,int K1,float* restrict Value);
int    EZGrid_GetValues(const TGrid* restrict const Grid,int Nb,float* restrict const I,float* restrict const J,float* restrict const K,float* restrict Value);
int    EZGrid_IJGetValue(TGrid* restrict const Grid,float I,float J,int K0,int K1,float* restrict Value);
int    EZGrid_IJGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,float I,float J,int K0,int K1,float *UU,float* restrict VV);
int    EZGrid_LLGetValue(TGrid* restrict const Grid,float Lat,float Lon,int K0,int K1,float* restrict Value);
int    EZGrid_LLGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,float Lat,float Lon,int K0,int K1,float* restrict UU,float* restrict VV);
int    EZGrid_GetArray(TGrid* restrict const Grid,int K,float* restrict Value);
int    EZGrid_GetRange(const TGrid* restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1,float* restrict Value);

int    EZGrid_Tile(int FIdTo,int NI, int NJ,int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2);
int    EZGrid_UnTile(int FIdTo,int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2);

TGrid *EZGrid_InterpTime(const TGrid* restrict const Grid0,const TGrid* restrict const Grid1,int Date);
TGrid *EZGrid_Interp(TGrid* restrict const Grid0,TGrid* restrict const Grid1,float Factor0,float Factor1);

float* EZGrid_TileBurn(TGrid* restrict const Grid,TGridTile* restrict const Tile,int K);
float* EZGrid_TileBurnAll(TGrid* restrict const Grid,int K);

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
