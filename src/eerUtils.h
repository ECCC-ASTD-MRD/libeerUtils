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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <malloc.h>

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
#define FCLAMP(R,PX0,PY0,PX1,PY1)         if (PX0<R->X0) PX0=R->X0; if (PY0<R->Y0) PY0=R->Y0; if (PX1>R->X1) PX1=R->X1; if (PY1>R->Y1) PY1=R->Y1;

/*Geographical related constants and functions*/
//#define EARTHRADIUS          6378140.0                          /*Rayon de la terre en metres*/
#define EARTHRADIUS          6371000.0                          /*Rayon de la terre en metres (Utilise par RPN)*/

#define DIST(E,A0,O0,A1,O1)  ((E+EARTHRADIUS)*acos(sin(A0)*sin(A1)+cos(O0-O1)*cos(A0)*cos(A1)))
#define COURSE(A0,O0,A1,O1)  (fmod(atan2(sin(O0-O1)*cos(A1),cos(A0)*sin(A1)-sin(A0)*cos(A1)*cos(O0-O1)),M_2PI))
#define RAD2DEG(R)           ((double)(R)*57.295779513082322864647721871734)
#define DEG2RAD(D)           ((double)(D)*0.017453292519943295474371680598)
#define M2RAD(M)             ((double)(M)*0.00000015706707756635)
#define RAD2M(R)             ((double)(R)*6.36670701949370745569e+06)
#define CLAMPLAT(LAT)        (LAT=LAT>90.0?90.0:(LAT<-90.0?-90.0:LAT))
#define CLAMPLON(LON)        (LON=LON>180?LON-360:(LON<-180?LON+360:LON))

#define RPNMAX 1024

/*Structure pour les coordonees latlon*/
typedef struct TGridCoord {
   float Lat,Lon,I,J;
} TGridCoord;

typedef struct TGridPoint {
   float I,J;
} TGridPoint;

typedef struct TCoord {
   double Lon,Lat,Elev;
} TCoord;


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

#define System_IsStamp(S) (S<999999999)

int    System_IsBigEndian(void);
int    System_ByteOrder(void);
int    System_BitCount(unsigned int N);
time_t System_DateTime2Seconds(int YYYYMMDD,int HHMMSS,int GMT);
double System_Date2Julian(int Year,int Month,int Day);
time_t System_Seconds2DateTime(time_t Sec,int *YYYYMMDD,int *HHMMSS,int GMT);
int    System_Julian2Stamp(int Year,int Day,int Time);
long   System_Stamp2Seconds(int Stamp);
int    System_Seconds2Stamp(long Sec);
void   System_StampDecode(int Stamp,int *YYYY,int *MM,int *DD,int *H,int *M,int *S);
void   System_StampEncode(int *Stamp,int YYYY,int MM,int DD,int H,int M,int S);
int    System_TimeValSubtract(struct timeval *Result,struct timeval *T0,struct timeval *T1);

void Astro_SunPos(time_t Sec,double *Lat,double *Lon);
void Astro_MoonPos(time_t ssue,float *lat,float *lon);

#ifdef HAVE_RMN
#include "rpnmacros.h"

/*EER external Fortran functions*/
extern int f77name(rmnlib_version)(char *rmn,wordint *print,wordint len);
extern int f77name(r8ipsort)(wordint *ip,double *a,wordint *n);
extern int f77name(binarysearchfindlevel2)(ftnfloat *hybvl,ftnfloat *hyb,wordint *size,wordint *ikk,wordint *ikn);

/*RPN external C && Fortran functions*/
extern wordint f77name(newdate)(wordint *dat1,wordint *dat2,wordint *dat3,wordint *mode);
extern wordint f77name(incdatr)(wordint *dat1,wordint *dat2,double *nhours);
extern wordint f77name(difdatr)(wordint *dat1,wordint *dat2,double *nhours);
extern wordint f77name(convip) (wordint *ip,ftnfloat *p,wordint *kind,wordint *mode,char *string,wordint *flag);
extern wordint f77name(sort)   (ftnfloat *work,wordint *n);
extern wordint f77name(ipsort) (wordint *ip,ftnfloat *a,wordint *n);
extern wordint f77name(fd1)    (ftnfloat *gd1,ftnfloat *f,ftnfloat *h);
extern wordint f77name(fdm)    (ftnfloat *gdm,ftnfloat *f,ftnfloat *h,wordint *m);
extern wordint f77name(int1d1) (ftnfloat *fi,ftnfloat *f,ftnfloat *xi,ftnfloat *x,ftnfloat *fx,ftnfloat *h,wordint *m,wordint *mi,ftnfloat *cmu1,ftnfloat *c1,ftnfloat *clmdam,ftnfloat *cm,ftnfloat *a,ftnfloat *c,ftnfloat *d);
extern wordint f77name(xyfll)  (ftnfloat *x,ftnfloat *y,ftnfloat *dlat,ftnfloat *dlon,ftnfloat *d60,ftnfloat *dgrw,wordint *nhem);
extern wordint f77name(llfxy)  (ftnfloat *dlat,ftnfloat *dlon,ftnfloat *x,ftnfloat *y,ftnfloat *d60,ftnfloat *dgrw,wordint *nhem);
extern wordint f77name(cigaxg) (char *igtyp,ftnfloat *xg1,ftnfloat *xg2,ftnfloat *xg3,ftnfloat *xg4,wordint *ig1,wordint *ig2,wordint *ig3,wordint *ig4);
extern wordint f77name(cxgaig) (char *igtyp,wordint *ig1,wordint *ig2,wordint *ig3,wordint *ig4,ftnfloat *xg1,ftnfloat *xg2,ftnfloat *xg3,ftnfloat *xg4);
extern wordint f77name(mscale) (ftnfloat *r,ftnfloat *d60,ftnfloat *pi,ftnfloat *pj,wordint *ni,wordint *nj);
extern wordint f77name(wkoffit)(char *filename,int size);

/*RPN 1d interpolation functions*/
extern void f77name (interp1d_findpos) ();
extern void f77name (interp1d_nearestneighbour) ();
extern void f77name (interp1d_linear) ();
extern void f77name (interp1d_cubicwithderivs) ();
extern void f77name (interp1d_cubiclagrange) ();
extern void f77name (extrap1d_lapserate) ();

extern int c_fnom();
extern int c_fclos();
extern int c_fstouv();
extern int c_fstfrm();
extern int c_fstecr();
extern int c_fstlir();
extern int c_fstinf();
extern int c_fstprm();
extern int c_fstluk();
extern int c_fstsui();
extern int c_fstinl();
extern int c_fstopc();

/*RPN external EZscint functions*/
extern wordint c_ezfreegridset(wordint gdid, wordint index);
extern wordint c_ezdefset(wordint gdout, wordint gdin);
extern wordint c_ezgdef(wordint ni, wordint nj, char *grtyp, char *grref,wordint ig1, wordint ig2, wordint ig3, wordint ig4, ftnfloat *ax, ftnfloat *ay);
extern wordint c_ezgdef_ffile(wordint ni, wordint nj, char *grtyp,wordint ig1, wordint ig2, wordint ig3, wordint ig4, wordint iunit);
extern wordint c_ezgdef_fll(wordint ni, wordint nj,ftnfloat *lat, ftnfloat *lon);
extern wordint c_ezgdef_fmem(wordint ni, wordint nj, char *grtyp, char *grref,wordint ig1, wordint ig2, wordint ig3, wordint ig4, ftnfloat *ax, ftnfloat *ay);
extern wordint c_ezgenpole(ftnfloat *vpolnor, ftnfloat *vpolsud, ftnfloat *fld,wordint ni, wordint nj, wordint vecteur,char *grtyp, wordint hem);
extern wordint c_ezgetopt(char *option, char *value);
extern wordint c_ezgetval(char *option, ftnfloat *value);
extern wordint c_gdll(wordint gdid, ftnfloat *lat, ftnfloat *lon);
extern wordint c_ezqkdef(wordint ni, wordint nj, char *grtyp,wordint ig1, wordint ig2, wordint ig3, wordint ig4, wordint iunit);
extern wordint c_ezquickdef(wordint ni, wordint nj, char *grtyp,wordint ig1, wordint ig2, wordint ig3, wordint ig4, wordint iunit);
extern wordint c_gdrls(wordint gdin);
extern wordint c_ezsetopt(char *option, char *value);
extern wordint c_ezsetval(char *option, ftnfloat fvalue);
extern wordint c_ezsint(ftnfloat *zout, ftnfloat *zin);
extern wordint c_ezuvint(ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin);
extern wordint c_ezwdint(ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin);
extern wordint c_gdgaxes(wordint gdid, ftnfloat *ax, ftnfloat *ay);
extern wordint c_gdgxpndaxes(wordint gdid, ftnfloat *ax, ftnfloat *ay);
extern wordint c_gdllfxy(wordint gdid, ftnfloat *lat, ftnfloat *lon, ftnfloat *x, ftnfloat *y, wordint n);
extern wordint c_gdllfxyz(wordint gdid, ftnfloat *lat, ftnfloat *lon, ftnfloat *x, ftnfloat *y, wordint n);
extern wordint c_gdllsval(wordint gdid, ftnfloat *zout, ftnfloat *zin, ftnfloat *lat, ftnfloat *lon, wordint n);
extern wordint c_gdllvval(wordint gdid, ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin,ftnfloat *lat, ftnfloat *lon, wordint n);
extern wordint c_gdllwdval(wordint gdid, ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin,ftnfloat *lat, ftnfloat *lon, wordint n);
extern wordint c_gdxpncf(wordint gdin, wordint *i1, wordint *i2, wordint *j1, wordint *j2);
extern wordint c_gdxysval(wordint gdin, ftnfloat *zout, ftnfloat *zin, ftnfloat *x, ftnfloat *y, wordint n);
extern wordint c_gdxywdval(wordint gdin, ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin, ftnfloat *x, ftnfloat *y, wordint n);
extern wordint c_gdxyvval(wordint gdin, ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin, ftnfloat *x, ftnfloat *y, wordint n);
extern wordint c_gduvfwd(wordint gdid,  ftnfloat *uugdout, ftnfloat *vvgdout, ftnfloat *uullin, ftnfloat *vvllin,ftnfloat *latin, ftnfloat *lonin, wordint npts);
extern wordint c_gdwdfuv(wordint gdid, ftnfloat *uullout, ftnfloat *vvllout, ftnfloat *uuin, ftnfloat *vvin,ftnfloat *latin, ftnfloat *lonin, wordint npts);
extern wordint c_gdxpngd(wordint gdin, ftnfloat *zxpnded, ftnfloat *zin);
extern wordint c_gdxyfll(wordint gdid, ftnfloat *x, ftnfloat *y, ftnfloat *lat, ftnfloat *lon, wordint n);
extern wordint c_gdxyzfll(wordint gdid, ftnfloat *x, ftnfloat *y, ftnfloat *lat, ftnfloat *lon, wordint n);
extern wordint c_guval(wordint gdin, ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin,  ftnfloat *vvin, ftnfloat *x, ftnfloat *y, wordint n);
extern void    c_ezgfllfxy(ftnfloat *lonp, ftnfloat *latp,ftnfloat *lon, ftnfloat *lat,ftnfloat *r, ftnfloat *ri, wordint *npts,ftnfloat *xlat1, ftnfloat *xlon1, ftnfloat *xlat2, ftnfloat *xlon2);
extern void    c_ezgfxyfll(ftnfloat *lonp, ftnfloat *latp,ftnfloat *lon, ftnfloat *lat,ftnfloat *r, ftnfloat *ri, wordint *npts,ftnfloat *xlat1, ftnfloat *xlon1, ftnfloat *xlat2, ftnfloat *xlon2);
extern void    c_ezgfwfllw(ftnfloat *uullout, ftnfloat *vvllout, ftnfloat *latin, ftnfloat *lonin,ftnfloat *xlatingf, ftnfloat *xloningf,wordint *ni, wordint *nj,char *grtyp, wordint *ig1, wordint *ig2, wordint *ig3, wordint *ig4);
extern void    c_ezllwfgfw(ftnfloat *uullout, ftnfloat *vvllout, ftnfloat *latin, ftnfloat *lonin,ftnfloat *xlatingf, ftnfloat *xloningf,wordint *ni,wordint *nj,char *grtyp,wordint *ig1,wordint *ig2,wordint *ig3,wordint *ig4);
extern void    c_ezdefxg(wordint gdid);
extern void    c_ezdefaxes(wordint gdid, ftnfloat *ax, ftnfloat *ay);
extern wordint c_gdinterp(ftnfloat *zout, ftnfloat *zin, wordint gdin, ftnfloat *x, ftnfloat *y, wordint npts);
extern int     c_gdsetmask(int gdid, int *mask);
extern int     c_gdgetmask(int gdid, int *mask);
extern int     c_ezsint_m(float *zout, float *zin);
extern int     c_ezuvint_m(float *uuout, float *vvout, float *uuin, float *vvin);
extern int     c_ezsint_mdm(float *zout, int *mask_out, float *zin, int *mask_in);
extern int     c_ezuvint_mdm(float *uuout, float *vvout, int *mask_out, float *uuin, float *vvin, int *mask_in);
extern int     c_ezsint_mask(int *mask_out, int *mask_in);

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

#endif
