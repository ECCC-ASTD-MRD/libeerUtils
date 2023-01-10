/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Fonctions et definitions relatives aux fichiers standards et rmnlib
 * Fichier      : RPN.h
 * Creation     : Avril 2006 - J.P. Gauthier
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

#include "Def.h"
#include "rmn.h"

#define RPNMAX 2048

struct TGeoRef;

typedef struct TRPNFile {
   char *CId;              // Identificateur du fichier
   char *Name;             // Path complet du fichier
   int  Open;              // Etat du fichier
   unsigned int Id;        // Numero d'unite du fichier
   int  NRef;              // Nombre de reference
   char Mode;              // Mode d'ouverture du fichier (r,w,a)
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
} TRPNHeader;

typedef struct TRPNField {
   TRPNHeader     Head;    // Entete du champs
   struct TDef    *Def;    // Definition des donnees
   struct TGeoRef *GRef;   // Reference geographique horizontale
   struct TZRef   *ZRef;   // Reference geographique verticale
} TRPNField;

int  RPN_CopyDesc(int FIdTo,TRPNHeader* const H);
int  RPN_IsDesc(const char* restrict Var);
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

int RPN_GetAllFields(int FID,int DateV,char *Etiket,int Ip1,int Ip2,int Ip3,char *Typvar,char *Nomvar,int **Arr,int *Size);
int RPN_GetAllDates(int *Flds,int NbFlds,int Uniq,int **DateV,int *NbDateV);
int RPN_GetAllIps(int *Flds,int NbFlds,int IpN,int Uniq,int **Ips,int *NbIp);

int RPN_GenerateIG(int *IG1,int *IG2,int *IG3);
int RPN_LinkFiles(char **Files,int N);
int RPN_UnLinkFiles(int FID);
int RPN_LinkPattern(const char* Pattern);
int RPN_ReadData(void *Data,TDef_Type Type,int Key);
int RPN_sReadData(void *Data,TDef_Type Type,int Key);
int RPN_Read(void *Data,TDef_Type Type,int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar);
int RPN_sRead(void *Data,TDef_Type Type,int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar);

#ifdef HAVE_RMN
#include "rmn/rpnmacros.h"

// EER threadsafe fstd functions
void cs_fstunlockid(int Unit);
int  cs_fstlockid();
int  cs_fstfrm(int Unit);
int  cs_fstouv(char *Path,char *Mode);
int  cs_fstflush(int Unit);
int  cs_fstinl(int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,int *List,int *Nb,int Max);
int  cs_fstinf(int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar);
int  cs_fstprm(int Unit,int *DateO,int *Deet,int *NPas,int *NI,int *NJ,int *NK,int *NBits,int *Datyp,int *IP1,int *IP2,int *IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int *IG1,int *IG2,int *IG3,int *IG4,int *Swa,int *Lng,int *DLTF,int *UBC,int *EX1,int *EX2,int *EX3);
int  cs_fstlir(void *Buf,int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar);
int  cs_fstluk(void *Data,int Idx,int *NI,int *NJ,int *NK);
int  cs_fstsui(int Unit,int *NI,int *NJ,int *NK);
int  cs_fstlukt(void *Data,int Unit,int Idx,char *GRTYP,int *NI,int *NJ,int *NK);
int  cs_fstecr(void *Data,int NPak,int Unit, int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over);

// EER external Fortran functions
extern int f77name(rmnlib_version)(char *rmn,int *print,int len);
extern int f77name(r8ipsort)(int *ip,double *a,int *n);

// RPN external C && Fortran functions
extern int f77name(convip)      (int *ip,float *p,int *kind,int *mode,char *string,int *flag);
extern int f77name(sort)        (float *work,int *n);
extern int f77name(ipsort)      (int *ip,float *a,int *n);
extern int f77name(fd1)         (float *gd1,float *f,float *h);
extern int f77name(fdm)         (float *gdm,float *f,float *h,int *m);
extern int f77name(int1d1)      (float *fi,float *f,float *xi,float *x,float *fx,float *h,int *m,int *mi,float *cmu1,float *c1,float *clmdam,float *cm,float *a,float *c,float *d);
extern int f77name(xyfll)       (float *x,float *y,float *dlat,float *dlon,float *d60,float *dgrw,int *nhem);
extern int f77name(llfxy)       (float *dlat,float *dlon,float *x,float *y,float *d60,float *dgrw,int *nhem);
extern int f77name(mscale)      (float *r,float *d60,float *pi,float *pj,int *ni,int *nj);
extern int f77name(wkoffit)     (char *filename,int size);
extern int f77name(fstlnk)      (int *list,int *size);
extern int f77name(fstunl)      (int *list,int *size);
extern int f77name(hyb_to_pres) (float *pres,float *hyb,float *ptop,float *rcoef,float *pref,int *kind,float *ps,int *NI,int *NJ,int *NK);

// RPN 1d interpolation functions
extern void f77name (interp1d_findpos) ();
extern void f77name (interp1d_nearestneighbour) ();
extern void f77name (interp1d_linear) ();
extern void f77name (interp1d_cubicwithderivs) ();
extern void f77name (interp1d_cubiclagrange) ();
extern void f77name (extrap1d_lapserate) ();

#endif
#endif
