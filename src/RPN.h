/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Fonctions et definitions relatives aux fichiers standards et rmnlib
 * Fichier      : RPN.h
 * Creation     : Avril 2006 - J.P. Gauthier
 * Revision     : $Id$
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
#ifndef _RPN_h
#define _RPN_h

#define RPNMAX 2048

struct TGeoRef;

typedef struct TRPNFile {
   char *CId;              // Identificateur du fichier
   char *Name;             // Path complet du fichier
   char Mode;              // Mode d'ouverture du fichier (r,w,a)
   int  Open;              // Etat du fichier
   unsigned int Id;        // Numero d'unite du fichier
   int  NRef;              // Nombre de reference
} TRPNFile;

typedef struct TRPNHeader {
   TRPNFile *File;         // Fichier dont provient le champs
   int  FID;               // FID dont provient le champs
   int  KEY;               // Cle du champs
   int  DATEO;             // Date d'origine du champs
   int  DATEV;             // Date de validitee du champs
   int  DEET;              // Duree d'un pas de temps
   int  NPAS;              // Pas de temps
   int  NBITS;             // Nombre de bits du champs
   int  DATYP;             // Type de donnees
   int  IP1,IP2,IP3;       // Specificateur du champs
   int  NI,NJ,NK,NIJ;      // Dimensions
   int  IG1,IG2,IG3,IG4;   // Descripteur de grille
   int  SWA;
   int  LNG;
   int  DLTF;
   int  UBC;
   int  EX1,EX2,EX3;
   char TYPVAR[3];         // Type de variable
   char NOMVAR[5];         // Nom de la variable
   char ETIKET[13];        // Etiquette du champs
   char GRTYP[2];          // Type de grilles
}  TRPNHeader;

typedef struct TRPNField {
   TRPNHeader     Head;    // Entete du champs
   struct TDef    *Def;    // Definition des donnees
   struct TGeoRef *GRef;   // Reference geographique horizontale
   struct TZRef   *ZRef;   // Reference geographique verticale
} TRPNField;

int  RPN_CopyDesc(int FIdTo,TRPNHeader* const H);
int  RPN_IsDesc(char *Var);
void RPN_FileLock(void);
void RPN_FileUnlock(void);
void RPN_FieldLock(void);
void RPN_FieldUnlock(void);
void RPN_IntLock(void);
void RPN_IntUnlock(void);

TRPNField* RPN_FieldNew();
void       RPN_FieldFree(TRPNField *Fld);
TRPNField* RPN_FieldReadIndex(int FileId,int Index,TRPNField *Fld);
TRPNField* RPN_FieldRead(int FileId,int DateV,char *Eticket,int IP1,int IP2,int IP3,char *TypVar,char *NomVar);
int        RPN_FieldWrite(int FileId,TRPNField *Field);
void       RPN_CopyHead(TRPNHeader *To,TRPNHeader *From);
int        RPN_FieldTile(int FID,struct TDef *Def,TRPNHeader *Head,struct TGeoRef *GRef,struct TZRef *ZRef,int Comp,int NI,int NJ,int Halo,int DATYP,int NPack,int Rewrite,int Compress);

int RPN_IntIdNew(int NI,int NJ,char* GRTYP,int IG1,int IG2,int IG3, int IG4,int FID);
int RPN_IntIdFree(int Id);
int RPN_IntIdIncr(int Id);

#ifdef HAVE_RMN
#include "rpnmacros.h"
#include "rpn_macros_arch.h"

// EER threadsafe fstd functions
int cs_fstunlockid(int Unit);
int cs_fstlockid();
int cs_fstfrm(int Unit);
int cs_fstouv(char *Path,char *Mode);
int cs_fstflush(int Unit);
int cs_fstinl(int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,int *List,int *Nb,int Max);
int cs_fstinf(int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar);
int cs_fstprm(int Unit,int *DateO,int *Deet,int *NPas,int *NI,int *NJ,int *NK,int *NBits,int *Datyp,int *IP1,int *IP2,int *IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int *IG1,int *IG2,int *IG3,int *IG4,int *Swa,int *Lng,int *DLTF,int *UBC,int *EX1,int *EX2,int *EX3);
int cs_fstlir(void *Buf,int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar);
int cs_fstluk(void *Data,int Idx,int *NI,int *NJ,int *NK);
int cs_fstsui(int Unit,int *NI,int *NJ,int *NK);
int cs_fstlukt(void *Data,int Unit,int Idx,char *GRTYP,int *NI,int *NJ,int *NK);
int cs_fstecr(void *Data,int NPak,int Unit, int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over);

// EER external Fortran functions
extern int f77name(rmnlib_version)(char *rmn,int *print,int len);
extern int f77name(r8ipsort)(int *ip,double *a,int *n);

// RPN external C && Fortran functions
extern int f77name(newdate)(int *dat1,int *dat2,int *dat3,int *mode);
extern int f77name(incdatr)(int *dat1,int *dat2,double *nhours);
extern int f77name(difdatr)(int *dat1,int *dat2,double *nhours);
extern int f77name(convip) (int *ip,ftnfloat *p,int *kind,int *mode,char *string,int *flag);
extern int f77name(sort)   (ftnfloat *work,int *n);
extern int f77name(ipsort) (int *ip,ftnfloat *a,int *n);
extern int f77name(fd1)    (ftnfloat *gd1,ftnfloat *f,ftnfloat *h);
extern int f77name(fdm)    (ftnfloat *gdm,ftnfloat *f,ftnfloat *h,int *m);
extern int f77name(int1d1) (ftnfloat *fi,ftnfloat *f,ftnfloat *xi,ftnfloat *x,ftnfloat *fx,ftnfloat *h,int *m,int *mi,ftnfloat *cmu1,ftnfloat *c1,ftnfloat *clmdam,ftnfloat *cm,ftnfloat *a,ftnfloat *c,ftnfloat *d);
extern int f77name(xyfll)  (ftnfloat *x,ftnfloat *y,ftnfloat *dlat,ftnfloat *dlon,ftnfloat *d60,ftnfloat *dgrw,int *nhem);
extern int f77name(llfxy)  (ftnfloat *dlat,ftnfloat *dlon,ftnfloat *x,ftnfloat *y,ftnfloat *d60,ftnfloat *dgrw,int *nhem);
extern int f77name(cigaxg) (char *igtyp,ftnfloat *xg1,ftnfloat *xg2,ftnfloat *xg3,ftnfloat *xg4,int *ig1,int *ig2,int *ig3,int *ig4);
extern int f77name(cxgaig) (char *igtyp,int *ig1,int *ig2,int *ig3,int *ig4,ftnfloat *xg1,ftnfloat *xg2,ftnfloat *xg3,ftnfloat *xg4);
extern int f77name(mscale) (ftnfloat *r,ftnfloat *d60,ftnfloat *pi,ftnfloat *pj,int *ni,int *nj);
extern int f77name(wkoffit)(char *filename,int size);
extern int f77name(fstlnk) (int *list,int *size);
extern int f77name(fstunl) (int *list,int *size);

// RPN 1d interpolation functions
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
extern int c_fsteff();

// RPN external EZscint functions
extern int  c_ezfreegridset(int gdid, int index);
extern int  c_ezdefset(int gdout, int gdin);
extern int  c_ezgdef(int ni, int nj, char *grtyp, char *grref,int ig1, int ig2, int ig3, int ig4, ftnfloat *ax, ftnfloat *ay);
extern int  c_ezgdef_ffile(int ni, int nj, char *grtyp,int ig1, int ig2, int ig3, int ig4, int iunit);
extern int  c_ezgdef_fll(int ni, int nj,ftnfloat *lat, ftnfloat *lon);
extern int  c_ezgdef_fmem(int ni, int nj, char *grtyp, char *grref,int ig1, int ig2, int ig3, int ig4, ftnfloat *ax, ftnfloat *ay);
extern int  c_ezgprm(int gdid, char *grtyp, int *ni, int *nj, int *ig1, int *ig2, int *ig3, int *ig4);
extern int  c_ezgenpole(ftnfloat *vpolnor, ftnfloat *vpolsud, ftnfloat *fld,int ni, int nj, int vecteur,char *grtyp, int hem);
extern int  c_ezgetopt(char *option, char *value);
extern int  c_ezgetval(char *option, ftnfloat *value);
extern int  c_ezget_nsubgrids(int id);
extern int  c_ezget_subgridids(int id,int *subid);
extern int  c_gdll(int gdid, ftnfloat *lat, ftnfloat *lon);
extern int  c_ezqkdef(int ni, int nj, char *grtyp,int ig1, int ig2, int ig3, int ig4, int iunit);
extern int  c_ezquickdef(int ni, int nj, char *grtyp,int ig1, int ig2, int ig3, int ig4, int iunit);
extern int  c_gdrls(int gdin);
extern int  c_ezsetopt(char *option, char *value);
extern int  c_ezsetval(char *option, ftnfloat fvalue);
extern int  c_ezsint(ftnfloat *zout, ftnfloat *zin);
extern int  c_ezuvint(ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin);
extern int  c_ezwdint(ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin);
extern int  c_gdgaxes(int gdid, ftnfloat *ax, ftnfloat *ay);
extern int  c_gdgxpndaxes(int gdid, ftnfloat *ax, ftnfloat *ay);
extern int  c_gdllfxy(int gdid, ftnfloat *lat, ftnfloat *lon, ftnfloat *x, ftnfloat *y, int n);
extern int  c_gdllfxyz(int gdid, ftnfloat *lat, ftnfloat *lon, ftnfloat *x, ftnfloat *y, int n);
extern int  c_gdllsval(int gdid, ftnfloat *zout, ftnfloat *zin, ftnfloat *lat, ftnfloat *lon, int n);
extern int  c_gdllvval(int gdid, ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin,ftnfloat *lat, ftnfloat *lon, int n);
extern int  c_gdllwdval(int gdid, ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin,ftnfloat *lat, ftnfloat *lon, int n);
extern int  c_gdxpncf(int gdin, int *i1, int *i2, int *j1, int *j2);
extern int  c_gdxysval(int gdin, ftnfloat *zout, ftnfloat *zin, ftnfloat *x, ftnfloat *y, int n);
extern int  c_gdxywdval(int gdin, ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin, ftnfloat *x, ftnfloat *y, int n);
extern int  c_gdxyvval(int gdin, ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin, ftnfloat *vvin, ftnfloat *x, ftnfloat *y, int n);
extern int  c_gduvfwd(int gdid,  ftnfloat *uugdout, ftnfloat *vvgdout, ftnfloat *uullin, ftnfloat *vvllin,ftnfloat *latin, ftnfloat *lonin, int npts);
extern int  c_gdwdfuv(int gdid, ftnfloat *uullout, ftnfloat *vvllout, ftnfloat *uuin, ftnfloat *vvin,ftnfloat *latin, ftnfloat *lonin, int npts);
extern int  c_gdxpngd(int gdin, ftnfloat *zxpnded, ftnfloat *zin);
extern int  c_gdxyfll(int gdid, ftnfloat *x, ftnfloat *y, ftnfloat *lat, ftnfloat *lon, int n);
extern int  c_gdxyzfll(int gdid, ftnfloat *x, ftnfloat *y, ftnfloat *lat, ftnfloat *lon, int n);
extern int  c_guval(int gdin, ftnfloat *uuout, ftnfloat *vvout, ftnfloat *uuin,  ftnfloat *vvin, ftnfloat *x, ftnfloat *y, int n);
extern void c_ezgfllfxy(ftnfloat *lonp, ftnfloat *latp,ftnfloat *lon, ftnfloat *lat,ftnfloat *r, ftnfloat *ri, int *npts,ftnfloat *xlat1, ftnfloat *xlon1, ftnfloat *xlat2, ftnfloat *xlon2);
extern void c_ezgfxyfll(ftnfloat *lonp, ftnfloat *latp,ftnfloat *lon, ftnfloat *lat,ftnfloat *r, ftnfloat *ri, int *npts,ftnfloat *xlat1, ftnfloat *xlon1, ftnfloat *xlat2, ftnfloat *xlon2);
extern void c_ezgfwfllw(ftnfloat *uullout, ftnfloat *vvllout, ftnfloat *latin, ftnfloat *lonin,ftnfloat *xlatingf, ftnfloat *xloningf,int *ni, int *nj,char *grtyp, int *ig1, int *ig2, int *ig3, int *ig4);
extern void c_ezllwfgfw(ftnfloat *uullout, ftnfloat *vvllout, ftnfloat *latin, ftnfloat *lonin,ftnfloat *xlatingf, ftnfloat *xloningf,int *ni,int *nj,char *grtyp,int *ig1,int *ig2,int *ig3,int *ig4);
extern void c_ezdefxg(int gdid);
extern void c_ezdefaxes(int gdid, ftnfloat *ax, ftnfloat *ay);
extern int  c_gdinterp(ftnfloat *zout, ftnfloat *zin, int gdin, ftnfloat *x, ftnfloat *y, int npts);
extern int  c_gdsetmask(int gdid, int *mask);
extern int  c_gdgetmask(int gdid, int *mask);
extern int  c_ezsint_m(float *zout, float *zin);
extern int  c_ezuvint_m(float *uuout, float *vvout, float *uuin, float *vvin);
extern int  c_ezsint_mdm(float *zout, int *mask_out, float *zin, int *mask_in);
extern int  c_ezuvint_mdm(float *uuout, float *vvout, int *mask_out, float *uuin, float *vvin, int *mask_in);
extern int  c_ezsint_mask(int *mask_out, int *mask_in);
extern int  c_ez_refgrid(int gdid);
extern int  c_fst_data_length(int size);

// RPN external BURP functions
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
