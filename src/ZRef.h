/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairies de fonctions utiles
 * Fichier      : ZRef.h
 * Creation     : Octobre 2011 - J.P. Gauthier
 *
 * Description  : Fonctions relectives a la coordonnee verticale.
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
 * Modification :
 *
 *    Nom         :
 *    Date        :
 *    Description :
 *
 *=========================================================
 */

#ifndef _ZRef_h
#define _ZRef_h

// Level related constants and functions
#define LVL_NIL         -1  //  No conversion
#define LVL_MASL         0  //  Meters above sea level
#define LVL_SIGMA        1  //  P/Ps
#define LVL_PRES         2  //  Pressure mb
#define LVL_UNDEF        3  //  units are user defined
#define LVL_MAGL         4  //  Meters above ground level
#define LVL_HYBRID       5  //  Hybrid levels
#define LVL_THETA        6  //  ?
#define LVL_ETA          7  //  (Pt-P)/(Pt-Ps) -not in convip
#define LVL_GALCHEN      8  //  Original Gal-Chen -not in convip (JP Defined)
#define LVL_NBR          9  //  Nombre d'elements
#define LVL_HOUR        10  //  Hours
#define LVL_ANGLE       11  //  Radar angles (JP defined)
#define LVL_INT         15  //  Entiers (reserve)
#define LVL_IDX         17  //  Index de matrice
#define LVL_MPRES       21  //  Metres-pression

#define MB2PA           100.0f           // Constant for converting pressure from [mb] to [Pa]
#define PA2MB           0.01f            // Constant for converting pressure from [Pa] to [mb]

#define PRESS2METER(LVL) (LVL>0?-8409.1*log(LVL)/1200.0:0)
#define SIGMA2METER(LVL) (LVL>0?-8409.1*log(LVL):0)
#define ETA2METER(LVL)   (-8409.1*log(LVL+1e-32))

struct TRPNHeader;

typedef enum { DEFAULT=1,NEW=2,OLD=3 } TZRef_IP1Mode;

// Vertical referential definition
typedef struct TZRef {
   char  *Name;           // Reference name
   int    NRef;           // Reference count
   int    Version;        // Version
   int    Type;           // Type of levels
   float *Levels;         // Levels list
   int    LevelNb;        // Number of Levels
   float  POff;           // Pressure offset from level
   float  PTop;           // Pressure at top of atmosphere
   float  PRef;           // Reference pressure
   float  RCoef[2];       // Hybrid level coefficient
   float  ETop;           // Eta coordinate a top
   float  *A,*B;          // Pressure calculation factors
   float  *P0;            // Pressure at surface
   float  *PCube;         // 3D Pressure cube

   char  *VGD; // VGrid descriptor
   TZRef_IP1Mode Style;   // IP style

#ifdef HAVE_RPNC   
   int NC_Id,NC_NZDimId;  // netCDF identifiers
#endif
} TZRef;

#define ZRef_Incr(ZREF) __sync_add_and_fetch(&ZREF->NRef,1)
#define ZRef_Decr(ZREF) __sync_sub_and_fetch(&ZREF->NRef,1)

TZRef* ZRef_New(void);
TZRef* ZRef_Define(int Type,int NbLevels,float *Levels);
int    ZRef_Free(TZRef *ZRef);
int    ZRef_Equal(TZRef *Zref0,TZRef *ZRef1);
TZRef* ZRef_Copy(TZRef *ZRef);
TZRef* ZRef_HardCopy(TZRef *ZRef);
int    ZRef_DecodeRPN(TZRef *ZRef,int Unit);
int    ZRef_SetRestrictLevels(float *Levels,int NbLevels);
int    ZRef_AddRestrictLevel(float Level);
int    ZRef_GetLevels(TZRef *ZRef,const struct TRPNHeader* restrict const H,int Invert);
double ZRef_K2Pressure(TZRef* restrict const ZRef,double P0,int K);
int    ZRef_KCube2Pressure(TZRef* restrict const ZRef,float *P0,int NIJ,int Log,float *Pres);
int    ZRef_KCube2Meter(TZRef* restrict const ZRef,float *GZ,const int NIJ,float *Height);
double ZRef_Level2Pressure(TZRef* restrict const ZRef,double P0,double Level);
double ZRef_Pressure2Level(TZRef* restrict const ZRef,double P0,double Pressure);
double ZRef_IP2Meter(int IP);
double ZRef_Level2Meter(double Level,int Type);
double ZRef_IP2Level(int IP,int *Type);
int    ZRef_Level2IP(float Level,int Type,TZRef_IP1Mode Mode);
int    ZRef_IPFormat(char *Buf,int IP,int Interval);

const char** ZRef_LevelNames();
const char*  ZRef_LevelName(int Type);
const char** ZRef_LevelUnits();
const char*  ZRef_LevelUnit(int Type);

#endif
