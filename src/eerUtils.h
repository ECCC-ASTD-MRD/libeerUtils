/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Lecture et traitements de divers fichiers de donnees
 * Fichier      : eerUtils.h
 * Creation     : Avril 2006 - J.P. Gauthier
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
 * Modification :
 *
 *    Nom         :
 *    Date        :
 *    Description :
 *
 *=========================================================
 */
#ifndef _eerUtils_h
#define _eerUtils_h

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <malloc.h>

// $@#! AIX does not have timersub macro (pasted from /sys/time.h)
#ifndef timersub
#define timersub(a, b, result)                       \
  do {                                 \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;              \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;              \
    if ((result)->tv_usec < 0) {                   \
      --(result)->tv_sec;                       \
      (result)->tv_usec += 1000000;                   \
    }                               \
  } while (0)
#endif

/*System related constants and functions*/
#define SYS_BIG_ENDIAN     0
#define SYS_LITTLE_ENDIAN  1

#define SYS_FIX2(x) ((((x) & 0xff)<<8) | ((unsigned short)(x)>>8))
#define SYS_FIX4(x) (((x)<<24) | (((x)<<8) & 0x00ff0000) | (((x)>>8) & 0x0000ff00) | ((x)>>24))
#define SYS_FIX8(x) (((long long)(SYS_SWAP4((unsigned int)((x<<32)>>32)))<<32) | (unsigned int)SYS_SWAP4(((int)(x>>32))))

#define SYS_SWAP2(x) (*(unsigned short*)&(x)=SYS_FIX2(*(unsigned short*)&(x)))
#define SYS_SWAP4(x) (*(unsigned int*)&(x)=SYS_FIX4(*(unsigned int*)&(x)))
#define SYS_SWAP8(x) (*(unsigned long long*)&(x)=SYS_FIX8(*(unsigned long long*)&(x)))

#define SYS_IFSWAP2(i,t,x) if (i!=t) SYS_SWAP2(x)
#define SYS_IFSWAP4(i,t,x) if (i!=t) SYS_SWAP4(x)
#define SYS_IFSWAP8(i,t,x) if (i!=t) SYS_SWAP8(x)

#define SYS_IOTHREAD_STACKSIZE 83886080

/*Mathematical related constants and functions*/
#ifndef M_PI
#define M_PI        3.141592653589793115997963468544        /*Pi*/
#endif
#ifndef M_2PI
#define M_2PI       6.283185307179586231995926937088        /*Deux fois Pi*/
#endif
#ifndef M_PI2
#define M_PI2       1.570796326794896557998981734272        /*Pi sur deux*/
#endif
#ifndef M_4PI
#define M_4PI       12.566370614359172463991853874176       /*Quatre foir Pi*/
#endif
#ifndef M_PI4
#define M_PI4       0.785398163397448278999490867136        /*Pi sur quatre*/
#endif

#define TINY_VALUE 1e-300
#define HUGE_VALUE 1e+300

#define TRUE   1
#define FALSE  0

#define CLOSE(V)                          ((V-(int)V)<0.5?(V-(int)V):(V-(int)V)-0.5)
#ifndef ROUND
#define ROUND(V)                          ((int)(V+0.5))
#endif
#ifndef ABS
#define ABS(V)                            ((V)<0.0?-(V):(V))
#endif
#define ILIN(X,Y,Z)                       (X+(Y-X)*(Z))
#define ILFAC(Level,V0,V1)                ((V1==V0)?0.5:(Level-V0)/(V1-V0))
#define ILDF(Level,V0,V1)                 ((Level-V0)/(V1-V0))
#define ILVIN(VAL,A,B)                    ((A!=B) && ((VAL>=A && VAL<=B) || (VAL<=A && VAL>=B)))
#define ILADD(SIDE,F)                     (SIDE?1.0f-F:F)
#define FARENOUGH(DT,X0,Y0,X1,Y1)         (hypot((Y1-Y0),(X1-X0))>DT)
#define LOG2(V)                           (log10(V)/0.301029995663981198017)

#define DSIZE(D)                          (D[0]?(D[1]?(D[2]?3:2):1):0)
#define FSIZE2D(D)                        ((unsigned long)(D->NI)*D->NJ)
#define FSIZE3D(D)                        ((unsigned long)(D->NI)*D->NJ*D->NK)
#define FSIZECHECK(D0,D1)                 (D0->NI==D1->NI && D0->NJ==D1->NJ && D0->NK==D1->NK)
#define FIDX2D(D,I,J)                     ((unsigned long)(J)*D->NI+(I))
#define FIDX3D(D,I,J,K)                   ((unsigned long)(K)*D->NI*D->NJ+(J)*D->NI+(I))
#define FIN2D(D,I,J)                      (J>=0 && J<D->NJ && I>=0 && I<D->NI)
#define FIN25D(D,I,J)                     (J>-0.5 && J<D->NJ+0.5 && I>-0.5 && I<D->NI+0.5)

#define CLAMP(A,MIN,MAX)                  (A>MAX?MAX:A<MIN?MIN:A)
#define ORDER(VAL)                        (VAL==0.0?1.0:floor(log10(ABS(VAL))))
#define RANGE_ORDER(VAL)                  (VAL==0.0?1.0:ceil(log10(ABS(VAL))-0.25))
#define RANGE_INCR(VAL)                   (pow(10,VAL-1))

#define VOUT(C0,C1,MI,MA)                 ((C0<MI && C1<MI) || (C0>MA && C1>MA))
#define VIN(VAL,MIN,MAX)                  ((VAL>MIN && VAL<MAX))
#define INSIDE(PT,X0,Y0,X1,Y1)            (PT[0]<=X1 && PT[0]>=X0 && PT[1]<=Y1 && PT[1]>=Y0)
#define FMIN(X,Y)                         (X<Y?X:Y)
#define FMAX(X,Y)                         (X>Y?X:Y)
#define FWITHIN(DL,LA0,LO0,LA1,LO1,LA,LO) ((LA>=LA0 && LA<=LA1)?((DL<=180)?(LO>=LO0 && LO<=LO1):((LO<=LO0 && DL>-180) || (LO>=LO1 && DL<180))):0)

/*Geographical related constants and functions*/
//#define EARTHRADIUS          6378140.0                          /*Rayon de la terre en metres*/
#define EARTHRADIUS          6371000.0                          /*Rayon de la terre en metres (Utilise par RPN)*/

#define DIST(E,A0,O0,A1,O1)  ((E+EARTHRADIUS)*acos(sin(A0)*sin(A1)+cos(O0-O1)*cos(A0)*cos(A1)))
#define COURSE(A0,O0,A1,O1)  (fmod(atan2(sin(O0-O1)*cos(A1),cos(A0)*sin(A1)-sin(A0)*cos(A1)*cos(O0-O1)),M_2PI))
#define RAD2DEG(R)           ((double)(R)*57.295779513082322864647721871734)
#define DEG2RAD(D)           ((double)(D)*0.017453292519943295474371680598)
#define M2RAD(M)             ((double)(M)*0.00000015706707756635)
#define M2DEG(M)             ((double)(M)*8.9992806450057884399546578634955e-06)
#define RAD2M(R)             ((double)(R)*6.36670701949370745569e+06)
#define CLAMPLAT(LAT)        (LAT=LAT>90.0?90.0:(LAT<-90.0?-90.0:LAT))
#define CLAMPLON(LON)        (LON=LON>180?LON-360:(LON<-180?LON+360:LON))
#define CLAMPLONRAD(LON)     (LON=(LON>M_PI?(fmod(LON+M_PI,M_2PI)-M_PI):(LON<=-M_PI?(fmod(LON-M_PI,M_2PI)+M_PI):LON)))

#define COORD_CLEAR(C)       (C.Lat=C.Lon=C.Elev=-999.0)

/*Structure pour les coordonees*/
typedef struct TGridCoord {
   float Lat,Lon,I,J;
} TGridCoord;

typedef struct TGridPoint {
   float I,J;
} TGridPoint;

typedef struct TCoord {
   double Lon,Lat,Elev;
} TCoord;

typedef struct TPoint2D {
  double X;
  double Y;
} TPoint2D;

int QSort_Double(const void *A,const void *B);
int QSort_Float(const void *A,const void *B);
int QSort_Int(const void *A,const void *B);

double InterpCubic(double X0,double X1,double X2, double X3,double F);
double InterpHermite(double X0,double X1,double X2, double X3,double F,double T,double B);

double HCentile(double *M,int N,int K);

char* strpath(char *Path,char *File);
char* strcatalloc(char *StrTo,char *StrFrom);
void  strtrim(char* Str,char Tok);
void  strrep(char *Str,char Tok,char Rep);
int   strfind(char *Str,char Tok);
int   strrindex(char *Str);
int   strmatch(const char *Str,char *Pattern);

#define System_IsStamp(S) (S<999999999)

int    System_IsBigEndian(void);
int    System_ByteOrder(void);
int    System_BitCount(unsigned int N);
time_t System_DateTime2Seconds(int YYYYMMDD,int HHMMSS,int GMT);
double System_Date2Julian(int Year,int Month,int Day);
time_t System_Seconds2DateTime(time_t Sec,int *YYYYMMDD,int *HHMMSS,int GMT);
int    System_Julian2Stamp(int Year,int Day,int Time);
long   System_Stamp2Seconds(int Stamp);
char*  System_StampFormat(int Stamp,char *Buf,char *Format);
int    System_Seconds2Stamp(long Sec);
void   System_StampDecode(int Stamp,int *YYYY,int *MM,int *DD,int *H,int *M,int *S);
void   System_StampEncode(int *Stamp,int YYYY,int MM,int DD,int H,int M,int S);
double System_TimeValSubtract(struct timeval *Result,struct timeval *T0,struct timeval *T1);

void Astro_SunPos(time_t Sec,double *Lat,double *Lon);
void Astro_MoonPos(time_t ssue,float *lat,float *lon);

#endif
