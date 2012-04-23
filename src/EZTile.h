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
#include <pthread.h>

#define GRIDCACHEMAX 4096
#define TILEMAX      1024

#define GRID_CENTER 0x0
#define GRID_LEFT   0x1
#define GRID_RIGHT  0x2
#define GRID_BOTTOM 0x4
#define GRID_TOP    0x8

#define EZGrid_IsSame(GRID0,GRID1)     (GRID0->GID==GRID1->GID)
#define EZGrid_IsLoaded(TILE,Z)        (TILE->Data && TILE->Data[Z])
#define EZGrid_IsInside(GRID,X,Y)      (X>=0 && Y>=0 && (GRID->Wrap || (X<GRID->H.NI-1 && Y<GRID->H.NJ-1)))
#define EZGrid_Size(GRID)              (GRID->H.NJ*GRID->H.NI)
#define EZGrid_TileValue(TILE,X,Y,Z)   (TILE->Data[Z][((int)Y-TILE->HJ)*TILE->HNI+((int)X-TILE->HI)])

// This checks for wraps around longitude and flips over poles
#define EZGrid_WrapFlip(GRID,X,Y) {\
   if (GRID->Wrap) { \
      if (Y>GRID->H.NJ-1) { \
         Y=GRID->H.NJ-(Y-GRID->H.NJ+2); \
         X=X<(GRID->H.NI>>1)?X+(GRID->H.NI>>1):X-(GRID->H.NI>>1); \
      } else if (Y<0) { \
         Y=-Y; \
         X=X<(GRID->H.NI>>1)?X+(GRID->H.NI>>1):X-(GRID->H.NI>>1); \
      } \
      if (X>=GRID->H.NI) { \
         X-=GRID->H.NI; \
      } else if (X<0) { \
         X+=GRID->H.NI;\
         if (X==GRID->H.NI) X-=1e-4;\
      } \
   } \
}

typedef struct TGridTile {
   int     GID;                         /*EZSCINT Tile grid id (for interpolation)*/
   char    Side;                        /*Side flag indicator*/
   int     I,J;                         /*Tile starting point within master grid*/
   int     NO;                          /*Tile number*/
   int     KBurn;                       /*Index estampille*/
   int     NI,NJ,NIJ;                   /*Tile dimensions without halo)*/
   int     HI,HJ,HDI,HDJ,HNI,HNJ,HNIJ;  /*Tile dimensions with halo*/
   float **Data;                        /*Data pointer*/
   pthread_mutex_t Mutex;               /*Per tile mutex for IO*/
} TGridTile;

struct TGrid;

typedef struct TGrid {
   TRPNHeader      H;                    /*RPN Standard file header*/
   TZRef          *ZRef;                 /*Vertical referential*/
   int             Wrap;                 /*Flag indicating grid globe wrap-around (global grids)*/
   float           Pole[2];              /*Pole coverage*/

   int             GID;                  /*EZSCINT Tile grid id (for interpolation)*/
   int             IP1,IP2,IP3,Master;   /*Grid template identifier*/
   int             Incr;                 /*Increasing sorting*/
   int             Factor;               /*Increasing sorting*/
   float          *Data;                 /*Data pointer*/
   unsigned int    NTI,NTJ;              /*Number of tiles in I and J*/
   unsigned int    Halo;                 /*Halo width*/

   float           FT0,FT1;              /*Time interpolation factor*/
   struct TGrid   *T0,*T1;               /*Time interpolation strat and end grid*/

   unsigned int    NbTiles;              /*Number of tiles*/
   TGridTile      *Tiles;                /*Array of tiles*/
   pthread_mutex_t Mutex;                /*Per grid mutex for IO*/
} TGrid;

int    EZGrid_CopyDesc(const int FIdTo,TGrid* restrict const Grid);
TGrid *EZGrid_New();
TGrid *EZGrid_Copy(TGrid *Master,int Level);
void   EZGrid_Free(TGrid* restrict const Grid);
TGrid* EZGrid_Get(TGrid* restrict const Grid);
TZRef* EZGrid_GetZRef(const TGrid* restrict const Grid);
TGrid* EZGrid_Read(int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2,int Incr);
TGrid *EZGrid_ReadIdx(int FId,int Key,int Incr);
int    EZGrid_Load(const TGrid* restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1);
int    EZGrid_LoadAll(const TGrid* restrict const Grid);
int    EZGrid_GetLevelNb(const TGrid* restrict const Grid);
int    EZGrid_GetLevels(const TGrid* restrict const Grid,float* restrict Levels,int* restrict Type);
int    EZGrid_GetLevelType(const TGrid* restrict const Grid);
float  EZGrid_GetLevel(const TGrid* restrict const Grid,float Pressure,float P0);
float  EZGrid_GetPressure(const TGrid* restrict const Grid,float Level,float P0);
int    EZGrid_BoundaryCopy(TGrid* restrict const Grid,int Width);
int    EZGrid_Write(int FId,TGrid* restrict const Grid,int NBits,int Overwrite);

int    EZGrid_Wrap(TGrid* restrict const Grid);
void   EZGrid_Factor(TGrid* restrict Grid,float Factor);
int    EZGrid_GetValue(const TGrid* restrict const Grid,int I,int J,int K0,int K1,float* restrict Value);
int    EZGrid_GetValues(const TGrid* restrict const Grid,int Nb,float* restrict const I,float* restrict const J,float* restrict const K,float* restrict Value);
int    EZGrid_IJGetValue(TGrid* restrict const Grid,float I,float J,int K0,int K1,float* restrict Value);
int    EZGrid_IJGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,float I,float J,int K0,int K1,float *UU,float* restrict VV);
int    EZGrid_LLGetValue(TGrid* restrict const Grid,float Lat,float Lon,int K0,int K1,float* restrict Value);
int    EZGrid_LLGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,float Lat,float Lon,int K0,int K1,float* restrict UU,float* restrict VV);
int    EZGrid_GetArray(TGrid* restrict const Grid,int K,float* restrict Value);
float* EZGrid_GetArrayPtr(TGrid* restrict const Grid,int K);
int    EZGrid_GetRange(const TGrid* restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1,float* restrict Value);
int    EZGrid_GetDelta(TGrid* restrict const Grid,int Invert,float* DX,float* DY,float* DA);
int    EZGrid_GetLL(TGrid* restrict const Grid,float* Lat,float* Lon,float* I,float* J,int Nb);
int    EZGrid_GetIJ(TGrid* restrict const Grid,float* Lat,float* Lon,float* I,float* J,int Nb);

int    EZGrid_TileGrid(int FIdTo,int NI, int NJ,int Halo,TGrid* restrict const Grid);
int    EZGrid_Tile(int FIdTo,int NI, int NJ,int Halo,int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2);
int    EZGrid_UnTile(int FIdTo,int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2);

int    EZGrid_Interp(TGrid* restrict const To,TGrid* restrict const From);
TGrid *EZGrid_InterpTime(const TGrid* restrict const Grid0,const TGrid* restrict const Grid1,int Date);
TGrid *EZGrid_InterpFactor(TGrid* restrict const Grid0,TGrid* restrict const Grid1,float Factor0,float Factor1);

float* EZGrid_TileBurn(TGrid* restrict const Grid,TGridTile* restrict const Tile,int K);
float* EZGrid_TileBurnAll(TGrid* restrict const Grid,int K);

void EZLock_RPNFile();
void EZUnLock_RPNFile();
void EZLock_RPNField();
void EZUnLock_RPNField();
void EZLock_RPNInt();
void EZUnLock_RPNInt();

int EZGrid_IdNew(int NI,int NJ,char* GRTYP,int IG1,int IG2,int IG3, int IG4,int FID);
int EZGrid_IdFree(int Id);
int EZGrid_IdIncr(int Id);


int cs_fstunlockid(int Unit);
int cs_fstlockid();
int cs_fstfrm(int Unit);
int cs_fstouv(char *Path,char *Mode);
int cs_fstflush(int Unit);
int cs_fstinl(int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,int *List,int *Nb,int Max);
int cs_fstinf(int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar);
int cs_fstprm(int Unit,int *DateO,int *Deet,int *NPas,int *NI,int *NJ,int *NK,int *NBits,int *Datyp,int *IP1,int *IP2,int *IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int *IG1,int *IG2,int *IG3,int *IG4,int *Swa,int *Lng,int *DLTF,int *UBC,int *EX1,int *EX2,int *EX3);
int cs_fstlir(float *Buf,int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar);
int cs_fstluk(float *Data,int Idx,int *NI,int *NJ,int *NK);
int cs_fstecr(float *Data,int NPak,int Unit, int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over);

#endif
