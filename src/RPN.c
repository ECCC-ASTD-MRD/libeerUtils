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
#ifdef HAVE_RMN

#include <pthread.h>

#include "App.h"
#include "GeoRef.h"
#include "Def.h"
#include "EZTile.h"
#include "fnom.h"

static const char *RPN_Desc[]={ ">>","^^","^>","!!","HY","PROJ","MTRX",NULL };

static char FGFDTLock[MAXFILES];

static pthread_mutex_t RPNFieldMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t RPNFileMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t RPNIntMutex=PTHREAD_MUTEX_INITIALIZER;

void RPN_FileLock() {
   pthread_mutex_lock(&RPNFileMutex);
}
void RPN_FileUnlock() {
   pthread_mutex_unlock(&RPNFileMutex);
}

void RPN_FieldLock() {
   pthread_mutex_lock(&RPNFieldMutex);
}
void RPN_FieldUnlock() {
   pthread_mutex_unlock(&RPNFieldMutex);
}

void RPN_IntLock() {
   pthread_mutex_lock(&RPNIntMutex);
}
void RPN_IntUnlock() {
   pthread_mutex_unlock(&RPNIntMutex);
}

int cs_fstunlockid(int Unit) {
   pthread_mutex_lock(&RPNFileMutex);
   FGFDTLock[Unit-1]=0;
   pthread_mutex_unlock(&RPNFileMutex);
}

int cs_fstlockid() {

   int id;

   pthread_mutex_lock(&RPNFileMutex);
   for (id=0;id<MAXFILES;id++) {
      /*Patch pour rmn008 (Unit 6 = bad)*/
      if (id!=5 && FGFDT[id].iun==0 && FGFDTLock[id]==0) {
         FGFDTLock[id]=1;
         id++;
         break;
      }
   }
   pthread_mutex_unlock(&RPNFileMutex);
   return(id);
}

int cs_fstouv(char *Path,char *Mode) {

   int err=-1,id=-1;
   char mode[32];

   if (Path) {
      id=cs_fstlockid();
      pthread_mutex_lock(&RPNFileMutex);
      if (index(Path,':') && Path[0]!=':') {
         strcpy(mode,Mode);
         strcat(mode,"+REMOTE");
         err=c_fnom(&id,Path,mode,0);
      } else {
         err=c_fnom(&id,Path,Mode,0);
      }
      if (err>=0) {
         err=c_fstouv(id,"RND");
      }
      pthread_mutex_unlock(&RPNFileMutex);
   }
   return(err<0?err:id);
}

int cs_fstflush(int Unit) {
   int err;

   pthread_mutex_lock(&RPNFileMutex);
   err=c_fstfrm(Unit);
   err=c_fstouv(Unit,"RND");
   pthread_mutex_unlock(&RPNFileMutex);

   return(err);
}

int cs_fstfrm(int Unit) {
   int err;

   pthread_mutex_lock(&RPNFileMutex);
   err=c_fstfrm(Unit);
   err=c_fclos(Unit);
   FGFDTLock[Unit-1]=0;
   pthread_mutex_unlock(&RPNFileMutex);

   return(err);
}

int cs_fstlir(void *Buf,int Unit,int *NI,int *NJ,int *NK,int DateV,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar) {

   int err;

   pthread_mutex_lock(&RPNFieldMutex);
   err=c_fstlir(Buf,Unit,NI,NJ,NK,DateV,Etiket,IP1,IP2,IP3,TypVar,NomVar);
   pthread_mutex_unlock(&RPNFieldMutex);

   return(err);
}

int cs_fstinf(int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar) {

   int err;

   pthread_mutex_lock(&RPNFieldMutex);
   err=c_fstinf(Unit,NI,NJ,NK,DateO,Etiket,IP1,IP2,IP3,TypVar,NomVar);
   pthread_mutex_unlock(&RPNFieldMutex);

   return(err);
}

int cs_fstprm(int Idx,int *DateO,int *Deet,int *NPas,int *NI,int *NJ,int *NK,int *NBits,int *Datyp,int *IP1,int *IP2,int *IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int *IG1,int *IG2,int *IG3,int *IG4,int *Swa,int *Lng,int *DLTF,int *UBC,int *EX1,int *EX2,int *EX3) {

   int err;

   pthread_mutex_lock(&RPNFieldMutex);
   err=c_fstprm(Idx,DateO,Deet,NPas,NI,NJ,NK,NBits,Datyp,IP1,IP2,IP3,TypVar,NomVar,Etiket,GrTyp,IG1,IG2,IG3,IG4,Swa,Lng,DLTF,UBC,EX1,EX2,EX3);
   pthread_mutex_unlock(&RPNFieldMutex);

   return(err);
}

int cs_fstinl(int Unit,int *NI,int *NJ,int *NK,int DateO,char *Etiket,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,int *List,int *Nb,int Max) {

   int err;

   pthread_mutex_lock(&RPNFieldMutex);
   err=c_fstinl(Unit,NI,NJ,NK,DateO,Etiket,IP1,IP2,IP3,TypVar,NomVar,List,Nb,Max);
   pthread_mutex_unlock(&RPNFieldMutex);

   return(err);
}

int cs_fstluk(void *Data,int Idx,int *NI,int *NJ,int *NK) {

   int err;

   pthread_mutex_lock(&RPNFieldMutex);
   err=c_fstluk(Data,Idx,NI,NJ,NK);
   pthread_mutex_unlock(&RPNFieldMutex);

   return(err);
}

int cs_fstsui(int Unit,int *NI,int *NJ,int *NK) {

   int key;

   pthread_mutex_lock(&RPNFieldMutex);
   key=c_fstsui(Unit,NI,NJ,NK);
   pthread_mutex_unlock(&RPNFieldMutex);

   return(key);
}

int cs_fstlukt(void *Data,int Unit,int Idx,char *GRTYP,int *NI,int *NJ,int *NK) {

   int    err=-1;
   TGrid *grid;

   if (GRTYP[0]=='#') {
      if ((grid=EZGrid_ReadIdx(Unit,Idx,0))) {
         if (EZGrid_TileBurnAll(grid,0,Data)) {
            err=0;
         }
         EZGrid_Free(grid);
      }
      *NI=grid->H.NI;
      *NJ=grid->H.NJ;
      *NK=grid->H.NK;
   } else {
      pthread_mutex_lock(&RPNFieldMutex);
      err=c_fstluk(Data,Idx,NI,NJ,NK);
      pthread_mutex_unlock(&RPNFieldMutex);
   }

   return(err);
}

int cs_fstecr(void *Data,int NPak,int Unit, int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over) {
   int err;

   pthread_mutex_lock(&RPNFieldMutex);
   err=c_fstecr(Data,NULL,NPak,Unit,DateO,Deet,NPas,NI,NJ,NK,IP1,IP2,IP3,TypVar,NomVar,Etiket,GrTyp,IG1,IG2,IG3,IG4,DaTyp,Over);
   pthread_mutex_unlock(&RPNFieldMutex);

   return(err);
}

/*----------------------------------------------------------------------------
 * Nom      : <RPN_IntIdNew>
 * Creation : Janvier 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Wrapper autour de c_ezqkdef pour garder un compte du nombre
 *           d'allocation de chaque grille ezscint
 *
 * Parametres :
 *
 * Retour:
 *  <Id>      : Identificateur de grille ezscint
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int RPN_IntIdNew(int NI,int NJ,char* GRTYP,int IG1,int IG2,int IG3, int IG4,int FID) {

   int id;

   RPN_IntLock();
   id=c_ezqkdef(NI,NJ,GRTYP,IG1,IG2,IG3,IG4,FID);
   RPN_IntUnlock();

   return(id);
}

/*----------------------------------------------------------------------------
 * Nom      : <RPN_IntIdFree>
 * Creation : Janvier 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Wrapper autour de c_gdrls pour garder un compte du nombre
 *           d'allocation de chaque grille ezscint
 *
 * Parametres :
 *  <Id>      : Identificateur de grille ezscint
 *
 * Retour:
 *  <n>      : Compte du nobre d'allocation de la grille ezscint
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int RPN_IntIdFree(int Id) {

   int n=-1;

   if (Id<0)
      return(n);
//TODO:Check on grid cache
//  RPN_IntLock();
//         c_gdrls(Id);
//   RPN_IntUnlock();

   return(n);
}


TRPNField* RPN_FieldNew(int NI,int NJ,int NK,int NC,TDef_Type Type) {

   TRPNField  *fld;

   fld=(TRPNField*)calloc(1,sizeof(TRPNField));
   if (!(fld->Def=Def_New(NI,NJ,NK,NC,Type))) {
      App_ErrorSet("RPN_FieldNew: Could not allocate memory");
      return(NULL);
   }

   fld->Ref=NULL;
   fld->Head.NI=NI;
   fld->Head.NJ=NJ;
   fld->Head.NK=NK;

   return(fld);
}

void RPN_FieldFree(TRPNField *Fld) {

   if (Fld->Ref) GeoRef_Free(Fld->Ref);
   if (Fld->Def) Def_Free(Fld->Def);

   free(Fld);
}

TRPNField* RPN_FieldReadIndex(int FileId,int Index,TRPNField *Fld) {

   TRPNField  *fld;
   TRPNHeader  h;
   int         ok,type;
   float       lvl;

   h.FID=FileId;
   h.KEY=Index;
   h.GRTYP[0]=h.GRTYP[1]=h.GRTYP[2]='\0';

   strcpy(h.NOMVAR,"    ");
   strcpy(h.TYPVAR,"  ");
   strcpy(h.ETIKET,"            ");
   ok=cs_fstprm(h.KEY,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,
         &h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
         h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,
         &h.UBC,&h.EX1,&h.EX2,&h.EX3);

   if (ok<0) {
      App_ErrorSet("RPN_FieldReadIndex: Could not get field information (c_fstprm failed)");
      return(NULL);
   }

   // Supprimer les espaces inutiles
   strtrim(h.NOMVAR,' ');
   strtrim(h.TYPVAR,' ');
   strtrim(h.ETIKET,' ');

   // If a TRPNField is passed, fill it instead of new
   if (Fld) {
      fld=Fld;
   } else {
      fld=(TRPNField*)malloc(sizeof(TRPNField));
      if (!(fld->Def=Def_New(h.NI,h.NJ,h.NK,1,TD_Float32))) {
         App_ErrorSet("RPN_FieldReadIndex: Could not allocate memory for fld");
         return(NULL);
      }
   }

   // Recuperer les donnees du champs
   c_fst_data_length(TDef_Size[fld->Def->Type]);
   if ((ok=cs_fstlukt(fld->Def->Data[0],h.FID,h.KEY,h.GRTYP,&h.NI,&h.NJ,&h.NK))<0) {
      App_ErrorSet("RPN_FieldReadIndex: Could not read field data (c_fstluk failed)");
      return(NULL);
   }

   // If a TRPNField is passed, we assume this has alredy been done
   if (!Fld) {
      // Recuperer les type de niveaux et forcer ETA pour SIGMA
      lvl=ZRef_IP2Level(h.IP1,&type);
      type=type==LVL_SIGMA?LVL_ETA:type;

      if (h.GRTYP[0]!='W') {
         fld->Ref=GeoRef_RPNSetup(h.NI,h.NJ,h.NK,type,&lvl,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,h.FID);
      }

   //   if (grtyp[0]=='U') {
   //      FSTD_FieldSubBuild(field);
   //   }

      GeoRef_Qualify(fld->Ref);
   }
   memcpy(&fld->Head,&h,sizeof(TRPNHeader));

   return(fld);
}

TRPNField* RPN_FieldRead(int FileId,int DateV,char *Eticket,int IP1,int IP2,int IP3,char *TypVar,char *NomVar) {

   TRPNField  *fld;
   TRPNHeader  h;

   // Rechercher et lire l'information de l'enregistrement specifie
   h.KEY=cs_fstinf(FileId,&h.NI,&h.NJ,&h.NK,DateV,Eticket,IP1,IP2,IP3,TypVar,NomVar);

   if (h.KEY<0) {
      App_ErrorSet("RPN_FieldRead: Specified field does not exist (c_fstinf failed)");
      return(NULL);
   }

   fld=RPN_FieldReadIndex(FileId,h.KEY,NULL);

   // Recheck for fstsui to be ok
   cs_fstinf(FileId,&h.NI,&h.NJ,&h.NK,DateV,Eticket,IP1,IP2,IP3,TypVar,NomVar);

   return(fld);
}

int RPN_FieldWrite(int FileId,TRPNField *Field) {

   int  ok;

   c_fst_data_length(TDef_Size[Field->Def->Type]);
   ok=cs_fstecr(Field->Def->Data[0],-Field->Head.NBITS,FileId,Field->Head.DATEO,Field->Head.DEET,Field->Head.NPAS,
      Field->Head.NI,Field->Head.NJ,Field->Head.NK,Field->Head.IP1,Field->Head.IP2,Field->Head.IP3,Field->Head.TYPVAR,
      Field->Head.NOMVAR,Field->Head.ETIKET,Field->Head.GRTYP,Field->Head.IG1,Field->Head.IG2,Field->Head.IG3,Field->Head.IG4,Field->Head.DATYP,FALSE);

   if (ok<0) {
      App_ErrorSet("RPN_FieldWrite: Could not write field data (c_fstecr failed)");
      return(0);
   }
   return(1);
}

void RPN_CopyHead(TRPNHeader *To,TRPNHeader *From) {

   strncpy(To->NOMVAR,From->NOMVAR,5);
   strncpy(To->TYPVAR,From->TYPVAR,3);
   strncpy(To->ETIKET,From->ETIKET,13);
   strncpy(To->NOMVAR,From->NOMVAR,5);
   To->DATEO=From->DATEO;
   To->DATEV=From->DATEV;
   To->DEET=From->DEET;
   To->NPAS=From->NPAS;
   To->DATYP=From->DATYP;
   To->IP1=From->IP1;
   To->IP2=From->IP2;
   To->IP3=From->IP3;
}

/*----------------------------------------------------------------------------
 * Nom      : <RPN_CopyDesc>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Copier les descripteur de grille d'un fichier dans un autre
 *
 * Parametres :
 *   <FidTo>  : Fichier dans lequel copier
 *   <H>      : RPN Header
 *
 * Retour:
 *  <int>        : Code de reussite (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int RPN_CopyDesc(int FIdTo,TRPNHeader* const H) {

   TRPNHeader  h;
   char       *data=NULL;
   const char *desc;
   int         d=0,ni,nj,nk,sz=0,ip1,ip2;
   int         key;

   if (H->FID>-1) {
      strcpy(h.NOMVAR,"    ");
      strcpy(h.TYPVAR,"  ");
      strcpy(h.ETIKET,"            ");
      strcpy(h.GRTYP," ");

      pthread_mutex_lock(&RPNFieldMutex);

      while(desc=RPN_Desc[d++]) {
         if (strncmp(desc,"HY",2)==0) {
            ip1=-1;ip2=-1;
         } else {
            ip1=H->IG1;
            ip2=H->IG2;
         }
         key=c_fstinf(FIdTo,&ni,&nj,&nk,-1,"",ip1,ip2,-1,"",desc);
         if (key<0) {
            key=c_fstinf(H->FID,&ni,&nj,&nk,-1,"",ip1,ip2,-1,"",desc);
            if (key>=0) {
               if (ni*ni>sz) {
                  data=(char*)realloc(data,ni*nj*sizeof(float));
                  sz=ni*nj;
               }
               c_fstluk(data,key,&ni,&nj,&nk);

               key=c_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,
                  &h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,h.GRTYP,&h.IG1,
                  &h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);
               key=c_fstecr(data,NULL,-h.NBITS,FIdTo,h.DATEO,h.DEET,h.NPAS,h.NI,h.NJ,h.NK,h.IP1,
                  h.IP2,h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,h.DATYP,1);
            }
         }
      }

      pthread_mutex_unlock(&RPNFieldMutex);
      free(data);
   }

   return(TRUE);
}

int RPN_IsDesc(char *Var) {

   const char *desc;
   int         d=0;

   while(desc=RPN_Desc[d++]) {
      if (!strncmp(Var,desc,4)) {
         return(TRUE);
      }
   }

   return(FALSE);
}

/*----------------------------------------------------------------------------
 * Nom      : <RPN_FieldTile>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Save an RPN field as tiles
 *
 * Parametres  :
 *   <FID>     : File id
 *   <Def>     : Data definition
 *   <Head>    : RPN field Header
 *   <Ref>     : Georeference
 *   <Comp>    : Component into data array
 *   <NI>      : Horizontal tile size
 *   <NJ>      : Vertical tile size
 *   <Halo>    : Width of the alo
 *   <DATYP>   : Data type (RPN value)
 *   <NPack>   : Facteur de compaction
 *   <Rewrite> : Reecrire le champs ou pas
 *
 * Retour:
 *  <int>        : Code de reussite (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int RPN_FieldTile(int FID,TDef *Def,TRPNHeader *Head,TGeoRef *Ref,int Comp,int NI,int NJ,int Halo,int DATYP,int NPack,int Rewrite,int Compress) {

   char        *tile=NULL,*data=NULL;;
   int          i,j,k,ip1,ni,nj,di,dj,pj,no,sz,key=0;
   unsigned int idx;

   // Allocate temp tile
   sz=TDef_Size[Def->Type];
   if (!(tile=(char*)malloc((NI+Halo*2)*(NJ+Halo*2)*sz))) {
      return(0);
   }

   pthread_mutex_lock(&RPNFieldMutex);

   for(k=0;k<Def->NK;k++) {
      idx=k*FSIZE2D(Def);

      Def_Pointer(Def,Comp,idx,data);

      // If IP1 is set, use it otherwise, convert it from levels array
      if ((ip1=Head->IP1)==-1 || Def->NK>1) {
         ip1=ZRef_Level2IP(Ref->ZRef.Levels[k],Ref->ZRef.Type,DEFAULT);
      }

      // Check if tiling asked and if dimensions allow tiling
      if (!NI || !NJ || (Def->NI<NI && Def->NJ<NJ)) {
         c_fst_data_length(TDef_Size[Def->Type]);
         key=c_fstecr(data,NULL,NPack,FID,Head->DATEO,Head->DEET,Head->NPAS,Def->NI,Def->NJ,1,ip1,Head->IP2,Head->IP3,Head->TYPVAR,
            Head->NOMVAR,Head->ETIKET,(Ref?(Ref->Grid[1]!='\0'?&Ref->Grid[1]:Ref->Grid):"X"),Head->IG1,Head->IG2,Head->IG3,Head->IG4,DATYP,Rewrite);
      } else {

         // Build and save the tiles, we adjust the tile size if it is too big
         no=0;
         for(j=0;j<Def->NJ;j+=NJ) {
            nj=((j+NJ>Def->NJ)?(Def->NJ-j):NJ)+Halo*2;
            dj=j-Halo;

            if (dj<0)          { dj+=Halo; nj-=Halo; }
            if (dj+nj>Def->NJ) { nj-=Halo; }

            for(i=0;i<Def->NI;i+=NI) {
               no++;
               ni=((i+NI>Def->NI)?(Def->NI-i):NI)+Halo*2;
               di=i-Halo;

               if (di<0)          { di+=Halo; ni-=Halo; }
               if (di+ni>Def->NI) { ni-=Halo; }

               for(pj=0;pj<nj;pj++) {
                  memcpy(tile+(pj*ni*sz),data+((dj+pj)*Def->NI+di)*sz,ni*sz);
               }
               c_fst_data_length(TDef_Size[Def->Type]);
               key=c_fstecr(tile,NULL,NPack,FID,Head->DATEO,Head->DEET,Head->NPAS,ni,nj,1,ip1,Head->IP2,no,Head->TYPVAR,
                  Head->NOMVAR,Head->ETIKET,"#",Head->IG1,Head->IG2,di+1,dj+1,DATYP,Rewrite);
            }
         }
      }
   }

   pthread_mutex_unlock(&RPNFieldMutex);

   free(tile);

   return(key>=0);
}

#endif