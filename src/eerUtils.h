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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <malloc.h>

#include "rpnmacros.h"

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

/*Mathematical related constants and functions*/
#ifndef M_PI
#define M_PI        3.141592653589793115997963468544        /*Pi*/
#endif
#ifndef M_2PI
#define M_2PI       6.283185307179586231995926937088        /*Deux fois Pix*/
#endif
#ifndef M_PI2
#define M_PI2       1.570796326794896557998981734272        /*Pi sur deux*/
#endif
#ifndef M_PI4
#define M_PI4       0.785398163397448278999490867136        /*Pi sur quatre*/
#endif

#define TINY_VALUE 1e-25
#define HUGE_VALUE 1e-300

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
#define LOG2(V)                           (floor(log10(V)/log10(2)))

#define DSIZE(D)                          (D[0]?(D[1]?(D[2]?3:2):1):0)
#define FSIZE2D(D)                        (D->NI*D->NJ)
#define FSIZE3D(D)                        (D->NI*D->NJ*D->NK)
#define FSIZECHECK(D0,D1)                 (D0->NI==D1->NI && D0->NJ==D1->NJ && D0->NK==D1->NK)
#define FIDX2D(D,I,J)                     ((J)*D->NI+(I))
#define FIDX3D(D,I,J,K)                   ((K)*D->NI*D->NJ+(J)*D->NI+(I))
#define FIN2D(D,I,J)                      (J>=0 && J<D->NJ && I>=0 && I<D->NI)
#define FIN25D(D,I,J)                     (J>-0.5 && J<D->NJ-0.5 && I>-0.5 && I<D->NI-0.5)

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
#define FCLAMP(R,PX0,PY0,PX1,PY1)         if (PX0<R->X0) PX0=R->X0; if (PY0<R->Y0) PY0=R->Y0; if (PX1>R->X1) PX1=R->X1; if (PY1>R->Y1) PY1=R->Y1;

/*Level related constants and functions*/
#define LVL_MASL         0  /* Meters above sea level */
#define LVL_SIGMA        1  /* P/Ps */
#define LVL_PRES         2  /* Pressure mb */
#define LVL_UNDEF        3  /* units are user defined */
#define LVL_MAGL         4  /* Meters above ground level */
#define LVL_HYBRID       5  /* Hybrid levels*/
#define LVL_THETA        6  /* ? */
#define LVL_ETA          7  /* (Pt-P)/(Pt-Ps) -not in convip */
#define LVL_GALCHEN      8  /* Original Gal-Chen -not in convip */
#define LVL_ANGLE        9  /* Radar angles */

#define PRESS2METER(LVL) (-8409.1*log((LVL==0?1e-31:LVL)/1200.0))
#define SIGMA2METER(LVL) (-8409.1*log(LVL==0?1e-31:LVL))

/*Geographical related constants and functions*/
#define EARTHRADIUS          6378140.0                          /*Rayon de la terre en metres*/

#define DIST(E,A0,O0,A1,O1)  ((E+EARTHRADIUS)*acos(sin(A0)*sin(A1)+cos(O0-O1)*cos(A0)*cos(A1)))
#define COURSE(A0,O0,A1,O1)  (fmod(atan2(sin(O0-O1)*cos(A1),cos(A0)*sin(A1)-sin(A0)*cos(A1)*cos(O0-O1)),M_2PI))
#define RAD2DEG(R)           ((R)*57.295779513082322864647721871734)
#define DEG2RAD(D)           ((D)*0.017453292519943295474371680598)
#define M2RAD(M)             ((M)*0.00000015706707756635)
#define RAD2M(R)             ((R)*6.36670701949370745569e+06)
#define CLAMPLAT(LAT)        (LAT=LAT>90.0?90.0:(LAT<-90.0?-90.0:LAT))
#define CLAMPLON(LON)        (LON=LON>180?LON-360:(LON<-180?LON+360:LON))

/*Vertical referential definition*/
typedef struct TZRef {
   float *Levels;       /*Levels list*/
   int    LevelType;    /*Type of levels*/
   int    LevelNb;      /*Number of Levels*/
   float  PTop;         /*Pressure at top of atmosphere*/
   float  PRef;         /*Reference pressure*/
   float  RCoef[2];     /*Hybrid level coefficient*/
   float  ETop;         /*Eta coordinate a top*/
   float  *A,*B;        /*Pressure calculation factors*/
} TZRef;

/*Standard struct to read an RPN Field*/
typedef struct TRPNHeader {
   int  FID;               /*Fichier dont provient le champs*/
   int  KEY;               /*Cle du champs*/
   int  DATEO;             /*Date d'origine du champs*/
   int  DATEV;             /*Date de validitee du champs*/
   int  DEET;
   int  NPAS;
   int  NBITS;
   int  DATYP;             /*Type de donnees*/
   int  IP1,IP2,IP3;       /*Specificateur du champs*/
   int  NI,NJ,NK,NIJ;      /*Dimensions*/
   char TYPVAR[3];         /*Type de variable*/
   char NOMVAR[5];         /*Nom de la variable*/
   char ETIKET[13];        /*Etiquette du champs*/
   char GRTYP[2];          /*Type de grilles*/
   int  IG1,IG2,IG3,IG4;   /*Descripteur de grille*/
   int  SWA;
   int  LNG;
   int  DLTF;
   int  UBC;
   int  EX1,EX2,EX3;
}  TRPNHeader;

int   QSort_Double(const void *A,const void *B);
int   QSort_Float(const void *A,const void *B);
int   QSort_Int(const void *A,const void *B);

double InterpCubic(double X0,double X1,double X2, double X3,double F);
double InterpHermite(double X0,double X1,double X2, double X3,double F,double T,double B);

double HCentile(double *M,int N,int K);

extern char *strdup(const char *str);
char* strpath(char *Path,char *File);
char* strcatalloc(char *StrTo,char *StrFrom);
void  strtrim(char* Str,char Tok);
void  strrep(char *Str,char Tok,char Rep);
int   strfind(char *Str,char Tok);
int   strrindex(char *Str);

#define System_IsStamp(S) (S<999999999)

int    System_IsBigEndian(void);
int    System_ByteOrder(void);
time_t System_DateTime2Seconds(int YYYYMMDD,int HHMMSS,int GMT);
double System_Date2Julian(int Year,int Month,int Day);
time_t System_Seconds2DateTime(time_t Sec,int *YYYYMMDD,int *HHMMSS,int GMT);
int    System_Julian2Stamp(int Year,int Day,int Time);
long   System_Stamp2Seconds(int Stamp);
int    System_Seconds2Stamp(long Sec);
void   System_StampDecode(int Stamp,int *YYYY,int *MM,int *DD,int *H,int *M,int *S);
void   System_StampEncode(int *Stamp,int YYYY,int MM,int DD,int H,int M,int S);

void Astro_SunPos(time_t Sec,double *Lat,double *Lon);
void Astro_MoonPos(time_t ssue,float *lat,float *lon);

/*EER external Fortran functions*/
extern int f77name(r8ipsort)(wordint *ip,double *a,wordint *n);
extern int f77name(binarysearchfindlevel2)(ftnfloat *hybvl,ftnfloat *hyb,wordint *size,wordint *ikk,wordint *ikn);

/*RPN external C && Fortran functions*/
extern int f77name(newdate)(wordint *dat1,wordint *dat2,wordint *dat3,wordint *mode);
extern int f77name(incdatr)(wordint *dat1,wordint *dat2,double *nhours);
extern int f77name(difdatr)(wordint *dat1,wordint *dat2,double *nhours);
extern int f77name(convip) (wordint *ip,ftnfloat *p,wordint *kind,wordint *mode,char *string,wordint *flag);
extern int f77name(sort)   (ftnfloat *work,wordint *n);
extern int f77name(fd1)    (ftnfloat *gd1,ftnfloat *f,ftnfloat *h);
extern int f77name(fdm)    (ftnfloat *gdm,ftnfloat *f,ftnfloat *h,wordint *m);
extern int f77name(int1d1) (ftnfloat *fi,ftnfloat *f,ftnfloat *xi,ftnfloat *x,ftnfloat *fx,ftnfloat *h,wordint *m,wordint *mi,ftnfloat *cmu1,ftnfloat *c1,ftnfloat *clmdam,ftnfloat *cm,ftnfloat *a,ftnfloat *c,ftnfloat *d);
extern int f77name(xyfll)  (ftnfloat *x,ftnfloat *y,ftnfloat *dlat,ftnfloat *dlon,ftnfloat *d60,ftnfloat *dgrw,wordint *nhem);
extern int f77name(llfxy)  (ftnfloat *dlat,ftnfloat *dlon,ftnfloat *x,ftnfloat *y,ftnfloat *d60,ftnfloat *dgrw,wordint *nhem);
extern int f77name(cigaxg) (char *igtyp,ftnfloat *xg1,ftnfloat *xg2,ftnfloat *xg3,ftnfloat *xg4,wordint *ig1,wordint *ig2,wordint *ig3,wordint *ig4);
extern int f77name(cxgaig) (char *igtyp,wordint *ig1,wordint *ig2,wordint *ig3,wordint *ig4,ftnfloat *xg1,ftnfloat *xg2,ftnfloat *xg3,ftnfloat *xg4);
extern int f77name(mscale) (ftnfloat *r,ftnfloat *d60,ftnfloat *pi,ftnfloat *pj,wordint *ni,wordint *nj);
extern int f77name(wkoffit)(char *filename);

extern int f77name(rmnlib_version) (char *rmn,wordint *print,wordint *len);

extern int c_fnom();
extern int c_fclos();
extern int c_fstouv();
extern int c_fstfrm();
extern int c_fstecr();
extern int c_fstinf();
extern int c_fstprm();
extern int c_fstluk();
extern int c_fstinl();
extern int c_fstopc();

/*RPN external EZscint functions*/
extern int c_ezqkdef();
extern int c_ezgdef_fmem();
extern int c_ezdefset();
extern int c_gdxysval();
extern int c_gdxywdval();
extern int c_gdxyfll();
extern int c_gdllfxy();
extern int c_gdrls();
extern int c_ezsint();
extern int c_ezuvint();
extern int c_ezsetval();
extern int c_ezsetopt();
extern int c_gdll();
extern int c_gdllsval();
extern int c_gdllwdval();
extern int c_gdllvval();
extern int c_gdaxes();

/*RPN external BURP functions*/
extern int c_mrfopc();
extern int c_mrfopr();
extern int c_mrfopn();
extern int c_mrfmxl();
extern int c_mrfcls();
extern int c_mrfloc();
extern int c_mrfget();
extern int c_mrbhdr();
extern int c_mrbloc();
extern int c_mrbprm();
extern int c_mrbxtr();
extern int c_mrbcvt();
extern int c_mrbdcl();

#endif
