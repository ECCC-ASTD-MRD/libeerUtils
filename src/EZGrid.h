/* ==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie de gestion des champs RPN en tuiles
 * Creation     : Janvier 2008
 * Auteur       : Jean-Philippe Gauthier
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

#ifndef _EZGrid_h
#define _EZGrid_h

#include <pthread.h>
#include "eerUtils.h"
#include "RPN.h"
#include "GeoRef.h"
#include "ZRef.h"
#include "QTree.h"
#include "Triangle.h"

#define EZGRID_CACHEMAX 2048
#define EZGRID_CENTER 0x0
#define EZGRID_LEFT   0x1
#define EZGRID_RIGHT  0x2
#define EZGRID_BOTTOM 0x4
#define EZGRID_TOP    0x8

#define EZGrid_IsSame(GRID0,GRID1)     (GRID0 && GRID1 && GRID0->GID==GRID1->GID)
#define EZGrid_IsLoaded(TILE,Z)        (TILE->Data && TILE->Data[Z] && !ISNAN(TILE->Data[Z][TILE->HNIJ-1]))
#define EZGrid_IsInside(GRID,X,Y)      (GRID->H.GRTYP[0]=='M' || GRID->H.GRTYP[0]=='Y' || Y>=0 && Y<=GRID->H.NJ-1 && (GRID->Wrap || X>=0 && X<=GRID->H.NI-1))
#define EZGrid_IsMesh(GRID)            (GRID->H.GRTYP[0]=='M')
#define EZGrid_IsRegular(GRID)         (GRID->H.GRTYP[0]!='M' && GRID->H.GRTYP[0]!='Y' && GRID->H.GRTYP[0]!='O' && GRID->H.GRTYP[0]!='X')
#define EZGrid_Is3D(GRID)              (GRID->H.NK>1?GRID->H.NK:0)

#define EZGrid_Size(GRID)              (GRID->H.NJ*GRID->H.NI)
#define EZGrid_TileValue(TILE,X,Y,Z)   (TILE->Data[Z][((int)Y-TILE->HJ)*TILE->HNI+((int)X-TILE->HI)])

// This checks for wraps around longitude and flips over poles
#define EZGrid_WrapFlip(GRID,X,Y) {\
   if (GRID->Wrap) { \
      if( Y > GRID->H.NJ-1 ) { \
         Y = GRID->H.NJ-(Y-GRID->H.NJ+2); \
         X = X<(GRID->H.NI>>1) ? X+(GRID->H.NI>>1) : X-(GRID->H.NI>>1); \
      } else if( Y < 0.0f ) { \
         Y = -Y; \
         X = X<(GRID->H.NI>>1) ? X+(GRID->H.NI>>1) : X-(GRID->H.NI>>1); \
      } \
      if( X >= (GRID->H.NI-GRID->Wrap+1) ) { \
         X -= (GRID->H.NI-GRID->Wrap+1); \
      } else if( X<0.0f ) { \
         X += (GRID->H.NI-GRID->Wrap+1); \
         if( X == GRID->H.NI ) X-=1e-4; \
      } \
   } \
}

typedef enum { EZ_NEAREST=0, EZ_LINEAR=1 }  TGridInterpMode;
typedef enum { EZ_CRESSMAN=0, EZ_BARNES=1 } TGridYInterpMode;

typedef struct TGridTile {
   pthread_mutex_t Mutex;               // Per tile mutex for IO
   float **Data;                        // Data pointer
   char  **Mask;                        // Mask pointer
   int     GID;                         // EZSCINT Tile grid id (for interpolation)
   int     I,J;                         // Tile starting point within master grid
   int     NO;                          // Tile number
   int     KBurn;                       // Index estampille
   int     NI,NJ,NIJ;                   // Tile dimensions without halo)
   int     HI,HJ,HDI,HDJ,HNI,HNJ,HNIJ;  // Tile dimensions with halo
   char    Side;                        // Side flag indicator
} TGridTile;

struct TGrid;

typedef struct TGrid {
   pthread_mutex_t Mutex;                // Per grid mutex for IO
   TRPNHeader      H;                    // RPN Standard file header
   TZRef          *ZRef;                 // Vertical referential
   TGeoRef        *GRef;                 // Geographic referential
   float          *Data;                 // Data pointer
   char           *Mask;                 // Mask pointer
   int             Wrap;                 // Flag indicating grid globe wrap-around (global grids)
   float           Pole[2];              // Pole coverage

   int             GID;                  // EZSCINT Tile grid id (for interpolation)
   int             IP1,IP2,IP3,Master;   // Grid template identifier
   int             Incr;                 // Increasing sorting
   float           Factor;               // Increasing sorting
   unsigned int    NTI,NTJ;              // Number of tiles in I and J
   unsigned int    Halo;                 // Halo width

   unsigned int    NbTiles;              // Number of tiles
   TGridTile      *Tiles;                // Array of tiles

   const struct TGrid   *T0,*T1;         // Time interpolation strat and end grid
   float                FT0,FT1;         // Time interpolation factor

} TGrid;

#ifndef EZGRID_BUILD
extern int              EZGRID_YLINEARCOUNT;
extern int              EZGRID_YQTREESIZE;
extern int              EZGRID_MQTREEDEPTH;
extern TGridYInterpMode EZGRID_YINTERP;
#endif

TGrid *EZGrid_New();
TGrid *EZGrid_Copy(TGrid *Master,int Level);
void   EZGrid_Free(TGrid* restrict const Grid);
void   EZGrid_Clear(TGrid* restrict const Grid);
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
float  EZGrid_GetPressure(const TGrid* restrict const Grid,float Level,float P0,float P0LS);
int    EZGrid_BoundaryCopy(TGrid* restrict const Grid,int Width);
int    EZGrid_Write(int FId,TGrid* restrict const Grid,int NBits,int Overwrite);

int    EZGrid_Wrap(TGrid* restrict const Grid);
void   EZGrid_Factor(TGrid* restrict Grid,float Factor);
int    EZGrid_GetValue(const TGrid* restrict const Grid,int I,int J,int K0,int K1,float* restrict Value);
int    EZGrid_GetValues(const TGrid* restrict const Grid,int Nb,float* restrict const I,float* restrict const J,float* restrict const K,float* restrict Value);
int    EZGrid_IJGetValue(TGrid* restrict const Grid,TGridInterpMode Mode,float I,float J,int K0,int K1,float* restrict Value);
int    EZGrid_IJGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,float I,float J,int K0,int K1,float *UU,float* restrict VV,float Conv);
int    EZGrid_LLGetValue(TGrid* restrict const Grid,TGridInterpMode Mode,float Lat,float Lon,int K0,int K1,float* restrict Value);
int    EZGrid_LLGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,float Lat,float Lon,int K0,int K1,float* restrict UU,float* restrict VV,float Conv);
int    EZGrid_LLGetValueO(TGrid* __restrict const GridU,TGrid* __restrict const GridV,TGridInterpMode Mode,float Lat,float Lon,int K0,int K1,float* __restrict UU,float* __restrict VV,float Conv);
int    EZGrid_LLGetValueY(TGrid* __restrict const GridU,TGrid* __restrict const GridV,TGridInterpMode Mode,float Lat,float Lon,int K0,int K1,float* __restrict UU,float* __restrict VV,float Conv);
int    EZGrid_LLGetValueM(TGrid* __restrict const GridU,TGrid* __restrict const GridV,TGridInterpMode Mode,float Lat,float Lon,int K0,int K1,float* __restrict UU,float* __restrict VV,float Conv);
int    EZGrid_GetArray(TGrid* restrict const Grid,int K,float* restrict Value);
float* EZGrid_GetArrayPtr(TGrid* restrict const Grid,int K);
int    EZGrid_GetRange(const TGrid* restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1,float* restrict Value);
int    EZGrid_GetDims(TGrid* restrict const Grid,int Invert,float* DX,float* DY,float* DA);
int    EZGrid_GetLL(TGrid* restrict const Grid,float* Lat,float* Lon,float* I,float* J,int Nb);
int    EZGrid_GetIJ(TGrid* restrict const Grid,float* Lat,float* Lon,float* I,float* J,int Nb);
int    EZGrid_GetBary(TGrid* __restrict const Grid,float Lat,float Lon,Vect3d Bary,Vect3i Index);

int    EZGrid_TileGrid(int FIdTo,int NI, int NJ,int Halo,TGrid* restrict const Grid);
int    EZGrid_UnTile(int FIdTo,int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2);

int    EZGrid_Interp(TGrid* restrict const To,TGrid* restrict const From);
TGrid *EZGrid_InterpTime(TGrid* restrict Grid,const TGrid* restrict Grid0,const TGrid* restrict Grid1,int Date);
TGrid *EZGrid_InterpFactor(TGrid* restrict Grid,const TGrid* restrict Grid0,const TGrid* restrict Grid1,float Factor0,float Factor1);

float* EZGrid_TileBurn(TGrid* restrict const Grid,TGridTile* restrict const Tile,int K,float* restrict Data);
float* EZGrid_TileBurnAll(TGrid* restrict const Grid,int K,float* restrict Data);

int EZGrid_IdNew(int NI,int NJ,char* GRTYP,int IG1,int IG2,int IG3, int IG4,int FID);
int EZGrid_IdFree(int Id);
int EZGrid_IdIncr(int Id);

#endif
