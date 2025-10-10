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

#define EZGRID_CACHEMAX 64

#define EZGrid_IsSame(GRID0,GRID1)     (GRID0 && GRID1 && GRID0->GDef->GID==GRID1->GDef->GID)
#define EZGrid_IsLoaded(GRID,Z)        (GRID->Data && GRID->Data[Z] && !ISNAN(GRID->Data[Z][0]))
#define EZGrid_HasIJ(GRID)             (GRID->GDef->GRTYP[0]!='M' && GRID->GDef->GRTYP[0]!='Y')
#define EZGrid_IsInside(GRID,X,Y)      (!EZGrid_HasIJ(GRID) || Y>=0 && Y<=GRID->GDef->NJ-1 && (GRID->GDef->Wrap || X>=0 && X<=GRID->GDef->NI-1))
#define EZGrid_IsMesh(GRID)            (GRID->GDef->GRTYP[0]=='M')
#define EZGrid_IsRegular(GRID)         (GRID->GDef->GRTYP[0]!='M' && GRID->GDef->GRTYP[0]!='Y' && GRID->GDef->GRTYP[0]!='O' && GRID->GDef->GRTYP[0]!='X')
#define EZGrid_Is3D(GRID)              (GRID->GDef->ZRef->LevelNb>1?GRID->GDef->ZRef->LevelNb:0)
#define EZGrid_Size(GRID)              (GRID->GDef->NIJ)

// This checks for wraps around longitude and flips over poles
#define EZGrid_WrapFlip(GRID,X,Y) {\
   if (GRID->GDef->Wrap) { \
      if( Y > GRID->GDef->NJ-1 ) { \
         Y = GRID->GDef->NJ-(Y-GRID->GDef->NJ+2); \
         X = X<(GRID->GDef->NI>>1) ? X+(GRID->GDef->NI>>1) : X-(GRID->GDef->NI>>1); \
      } else if( Y < 0.0f ) { \
         Y = -Y; \
         X = X<(GRID->GDef->NI>>1) ? X+(GRID->GDef->NI>>1) : X-(GRID->GDef->NI>>1); \
      } \
      if( X >= (GRID->GDef->NI-GRID->GDef->Wrap+1) ) { \
         X -= (GRID->GDef->NI-GRID->GDef->Wrap+1); \
      } else if( X<0.0f ) { \
         X += (GRID->GDef->NI-GRID->GDef->Wrap+1); \
         if( X == GRID->GDef->NI ) X-=1e-4; \
      } \
   } \
}

typedef enum { EZ_NEAREST=0, EZ_LINEAR=1 }  TGridInterpMode;
typedef enum { EZ_CRESSMAN=0, EZ_BARNES=1 } TGridYInterpMode;

typedef struct TGridDef {
   TZRef          *ZRef;               // Vertical referential
   TGeoRef        *GRef;               // Geographic referential

   int            NI,NJ,NIJ;           // Grid dimensions
   int            IG1,IG2,IG3,IG4;     // Grid descriptors

   int            CIdx;                // Cache id of the GridDef
   int            GID;                 // EZSCINT Tile grid id (for interpolation)
   int            Wrap;                // Flag indicating grid globe wrap-around (global grids)
   float          Pole[2];             // Pole coverage

   // For tile support
   unsigned int    NbTiles;            // Number of tiles (0 if not tiled)
   int             Halo;               // Halo size
   int             NTI,NTJ;            // Number of tiles in I and J (0 if not tiled)

   char           GRTYP[2];            // Grid type

   // Temporary for ZE grids
   float          *AX,*AY;             // X and Y descriptors
   TLookup        LUX,LUY;             // X and Y lookup for the above descriptors
   float          Rot[3][3];           // Rotation matrix
} TGridDef;

typedef struct TGrid TGrid;
typedef struct TGrid {
   pthread_mutex_t Mutex;                // Per grid mutex for IO
   TGridDef       *GDef;                 // Grid definition
   TRPNHeader      H;                    // RPN Standard file header
   float         **Data;                 // Data pointer
   char          **Mask;                 // Mask pointer

   float           Factor;               // Increasing sorting

   TGrid          *T0,*T1;               // Time interpolation strat and end grid
   float           FT0,FT1;              // Time interpolation factor

} TGrid;

#ifndef EZGRID_BUILD
extern int              EZGRID_YLINEARCOUNT;
extern int              EZGRID_YQTREESIZE;
extern int              EZGRID_MQTREEDEPTH;
extern TGridYInterpMode EZGRID_YINTERP;
#endif

TGrid *EZGrid_New();
TGrid *EZGrid_CopyGrid(const TGrid *Master,int Level,int Alloc);
void   EZGrid_Free(TGrid* restrict const Grid);
void   EZGrid_Clear(TGrid* restrict const Grid);
TGrid* EZGrid_Read(int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2,int Incr);
TGrid *EZGrid_ReadIdx(int FId,int Key,int Incr);
int    EZGrid_Update(TGrid* restrict const Grid,int FId,int DateV);
int    EZGrid_GetData(TGrid* restrict Grid,int K);
int    EZGrid_LoadAll(TGrid* restrict const Grid);
int    EZGrid_GetLevelNb(const TGrid* restrict const Grid);
int    EZGrid_GetLevels(const TGrid* restrict const Grid,float* restrict Levels,int* restrict Type);
int    EZGrid_GetLevelType(const TGrid* restrict const Grid);
float  EZGrid_GetLevel(const TGrid* restrict const Grid,float Pressure,float P0);
float  EZGrid_GetPressure(const TGrid* restrict const Grid,float Level,float P0,float P0LS);
int    EZGrid_Write(int FId,TGrid* restrict const Grid,int NBits,int Overwrite);

void   EZGrid_Factor(TGrid* restrict Grid,float Factor);
int    EZGrid_IJGetValue(TGrid* restrict const Grid,TGridInterpMode Mode,float I,float J,int K0,int K1,float* restrict Value);
int    EZGrid_IJGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,float I,float J,int K0,int K1,float *UU,float* restrict VV,float Conv);
int    EZGrid_LLGetValue(TGrid* restrict const Grid,TGridInterpMode Mode,double Lat,double Lon,int K0,int K1,float* restrict Value);
int    EZGrid_LLGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,double Lat,double Lon,int K0,int K1,float* restrict UU,float* restrict VV,float Conv);
int    EZGrid_LLGetValueO(TGrid* __restrict const GridU,TGrid* __restrict const GridV,TGridInterpMode Mode,double Lat,double Lon,int K0,int K1,float* __restrict UU,float* __restrict VV,float Conv);
int    EZGrid_LLGetValueY(TGrid* __restrict const GridU,TGrid* __restrict const GridV,TGridInterpMode Mode,double Lat,double Lon,int K0,int K1,float* __restrict UU,float* __restrict VV,float Conv);
int    EZGrid_LLGetValueM(TGrid* __restrict const GridU,TGrid* __restrict const GridV,TGridInterpMode Mode,double Lat,double Lon,int K0,int K1,float* __restrict UU,float* __restrict VV,float Conv);
float* EZGrid_GetArrayPtr(TGrid* restrict const Grid,int K);
int    EZGrid_GetDims(const TGrid* restrict const Grid,int Invert,float* DX,float* DY,float* DA);
int    EZGrid_GetLL(TGrid* restrict const Grid,double* Lat,double* Lon,float* I,float* J,int Nb);
int    EZGrid_GetIJ(TGrid* restrict const Grid,double* Lat,double* Lon,float* I,float* J,int Nb);
int    EZGrid_GetBary(TGrid* __restrict const Grid,double Lat,double Lon,Vect3d Bary,Vect3i Index);

int    EZGrid_Interp(TGrid* restrict const To,TGrid* restrict const From);
TGrid *EZGrid_InterpTime(TGrid* restrict Grid,TGrid* restrict Grid0,TGrid* restrict Grid1,int Date);
TGrid *EZGrid_InterpFactor(TGrid* restrict Grid,TGrid* restrict Grid0,TGrid* restrict Grid1,float Factor0,float Factor1);

int EZGrid_IdNew(int NI,int NJ,char* GRTYP,int IG1,int IG2,int IG3, int IG4,int FID);
int EZGrid_IdFree(int Id);
int EZGrid_IdIncr(int Id);

#endif
