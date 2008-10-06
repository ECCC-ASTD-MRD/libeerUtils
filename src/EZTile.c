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

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <pthread.h>

#include "EZTile.h"
#include "rpn_macros_arch.h"
#include "/usr/local/env/armnlib/include/fnom.h"

static TGrid *GridCache[GRIDCACHEMAX];
static pthread_mutex_t CacheMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t RPNFieldMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t RPNFileMutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t RPNIntMutex=PTHREAD_MUTEX_INITIALIZER;

void EZLock_RPNFile() {
   pthread_mutex_lock(&RPNFileMutex);
}
void EZUnLock_RPNFile() {
   pthread_mutex_unlock(&RPNFileMutex);
}

void EZLock_RPNField() {
   pthread_mutex_lock(&RPNFieldMutex);
}
void EZUnLock_RPNField() {
   pthread_mutex_unlock(&RPNFieldMutex);
}

void EZLock_RPNInt() {
   pthread_mutex_lock(&RPNIntMutex);
}
void EZUnLock_RPNInt() {
   pthread_mutex_unlock(&RPNIntMutex);
}

int cs_fnomid() {

   int id;

   pthread_mutex_lock(&RPNFileMutex);

   for (id=0;id<MAXFILES;id++) {
      /*Patch pour rmn008 (Unit 6 = bad)*/
      if (FGFDT[id].iun==0) {
         id++;
         if (id==6) id=32767;
         break;
      }
   }
   pthread_mutex_unlock(&RPNFileMutex);

   return(id);
}

int cs_fstouv(char *Path,char *Mode) {

   int err=-1,id=-1;
   char *path,mode[32];

   if (Path) {
      id=cs_fnomid();
      pthread_mutex_lock(&RPNFileMutex);
      if (Path[strlen(Path)-1]==':') {
         /*In remote mode, the nice fnom changes the content of the variable, so we have to make a copy*/
         path=strdup(Path);
         strcpy(mode,Mode);
         strcat(mode,"+REMOTE");
         err=c_fnom(&id,path,mode,0);
         free(path);
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

int cs_fstfrm(int Unit) {
   int err;

   pthread_mutex_lock(&RPNFileMutex);
   err=c_fstfrm(Unit);
   err=c_fclos(Unit);
   pthread_mutex_unlock(&RPNFileMutex);

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

int cs_fstluk(float *Data,int Idx,int *NI,int *NJ,int *NK) {
   int err;

   pthread_mutex_lock(&RPNFieldMutex);
   err=c_fstluk(Data,Idx,NI,NJ,NK);
   pthread_mutex_unlock(&RPNFieldMutex);

   return(err);
}

int cs_fstecr(float *Data,int NPak,int Unit, int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,char* TypVar,char *NomVar,char *Etiket,char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over) {
   int err;

   pthread_mutex_lock(&RPNFieldMutex);
   err=c_fstecr(Data,NULL,NPak,Unit,DateO,Deet,NPas,NI,NJ,NK,IP1,IP2,IP3,TypVar,NomVar,Etiket,GrTyp,IG1,IG2,IG3,IG4,DaTyp,Over);
   pthread_mutex_unlock(&RPNFieldMutex);

   return(err);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_TileGet>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir la tuile contenant le point specifie
 *
 * Parametres :
 *   <Grid>      : Grille maitre
 *   <I>         : Coordonnee du point de grille en I
 *   <J>         : Coordonnee du point de grille en J
 *
 * Retour:
 *  <TGridTile*> : Tuile (ou NULL si non existante)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
static inline TGridTile* EZGrid_TileGet(TGrid *Grid,int I,int J) {

   TGridTile    *tile=NULL;
   register int  idx=0;

   while((tile=&Grid->Tiles[idx])) {
      if (I>=tile->I+tile->NI) {
         idx++;
         continue;
      }
      if (J>=tile->J+tile->NJ) {
         idx+=Grid->NTI;
         continue;
      }
      break;
   }
   return(tile);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_TileGetData>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Lire les donnees d'une tuile dans le fichier standard
 *
 * Parametres :
 *   <Grid>      : Grille maitre
 *   <Tile>      : Tuile pour laquelle lire les donnees
 *
 * Retour:
 *  <float*>     : Pointeur sur les donnees lues (ou NULL si erreur)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
static inline float *EZGrid_TileGetData(TGrid *Grid,TGridTile *Tile) {

   int    key;
   int    ni,nj,nk,k;

   if (Grid->H.FID<0) {
      return(NULL);
   }

   if (!(Tile->Data=(float*)malloc(Tile->NI*Tile->NJ*Grid->H.NK*sizeof(float)))) {
      fprintf(stderr,"EZGrid_TileGetData: Unable to allocate memory for tile data\n");
      return(NULL);
   }

   pthread_mutex_lock(&RPNFieldMutex);
   for(k=0;k<Grid->H.NK;k++) {
      key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,Grid->H.DATEV,Grid->H.ETIKET,Grid->Levels[k],Grid->H.IP2,Tile->NO,Grid->H.TYPVAR,Grid->H.NOMVAR);
      if (key<0) {
         fprintf(stderr,"EZGrid_TileGetData: Could not find tile data at level %i\n",Grid->Levels[k]);
      } else {
         c_fstluk(&Tile->Data[k*Tile->NI*Tile->NJ],key,&ni,&nj,&nk);
      }
   }
   pthread_mutex_unlock(&RPNFieldMutex);
   return(Tile->Data);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_TileFind>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Trouver la tuile incluant le point specifie et en lire les donnees
 *
 * Parametres :
 *   <Grid>      : Grille maitre
 *   <I>         : Coordonnee du point de grille en I
 *   <J>         : Coordonnee du point de grille en J
 *
 * Retour:
 *  <tile*>      : Pointeur sur la tuile (ou NULL si erreur)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
static inline TGridTile* EZGrid_TileFind(TGrid *Grid,int i,int j) {

   TGridTile *tile;

   /*Figure out the right tile*/
   if (!(tile=EZGrid_TileGet(Grid,i,j))) {
      return(NULL);
   }

   /*Check for tile data*/
   if (!tile->Data) {
      if (!EZGrid_TileGetData(Grid,tile)) {
         fprintf(stderr,"EZGrid_TileFind: Unable to get tile data\n");
         return(NULL);
      }
   }
   return(tile);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_TileBurn>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Estampiller la tuile de donnees dans la grille maitre
 *
 * Parametres :
 *   <Grid>      : Grille maitre
 *   <Tile>      : Tuile pour laquelle lire les donnees
 *   <K>         : Niveau vertical a tuiler
 *
 * Retour:
 *  <float*>     : Pointeur sur les donnees lues (ou NULL si erreur)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
float* EZGrid_TileBurn(TGrid *Grid,TGridTile *Tile,int K) {

   int j,k;

   if (Tile->KBurn!=-1 && Tile->KBurn==K) {
      return(Grid->Data);
   }

   if (!Tile || !Tile->Data) {
      fprintf(stderr,"EZGrid_TileBurn: Invalid tile\n");
      return(NULL);
   }

   if (!Grid->Data) {
      if (!(Grid->Data=(float*)malloc(Grid->H.NI*Grid->H.NJ*sizeof(float)))) {
         fprintf(stderr,"EZGrid_TileBurn: Unable to allocate memory for grid data\n");
         return(NULL);
      }
   }

   k=K*(Tile->NI*Tile->NJ);
   for(j=0;j<Tile->NJ;j++) {
      memcpy(&Grid->Data[(Tile->J+j)*Grid->H.NI+Tile->I],&Tile->Data[k+j*Tile->NI],Tile->NI*sizeof(float));
   }
   Tile->KBurn=K;

   return(Grid->Data);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_TileBurnAll>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Estampiller la tuile de donnees dans la grille maitre
 *
 * Parametres :
 *   <Grid>      : Grille maitre
 *   <K>         : Niveau vertical a tuiler
 *
 * Retour:
 *  <float*>     : Pointeur sur les donnees lues (ou NULL si erreur)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
float* EZGrid_TileBurnAll(TGrid *Grid,int K) {

   int t;

   for(t=0;t<Grid->NbTiles;t++) {
      if (!Grid->Tiles[t].Data) {
         EZGrid_TileGetData(Grid,&Grid->Tiles[t]);
      }
      if (!EZGrid_TileBurn(Grid,&Grid->Tiles[t],K)) {
         return(NULL);
      }
   }
   return(Grid->Data);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_CacheFind>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir la grille "template" correspondant a la grille specifiee
 *
 * Parametres :
 *   <Grid>      : Grille
 *
 * Retour:
 *  <TGrid*>     : Grille "template" (ou NULL si non existante)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
TGrid* EZGrid_CacheFind(TGrid *Grid) {

   register int n;

   if (Grid) {
      pthread_mutex_lock(&CacheMutex);
      for(n=0;n<GRIDCACHEMAX;n++) {
         if (GridCache[n] && GridCache[n]->IP1==Grid->H.IG1 && GridCache[n]->IP2==Grid->H.IG2 && GridCache[n]->H.NK==Grid->H.NK) {
            pthread_mutex_unlock(&CacheMutex);
            return(GridCache[n]);
         }
      }
      pthread_mutex_unlock(&CacheMutex);
   }
   return(NULL);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_CacheIdx>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir l'index de la grille specifiee dans le tableau "cache"
 *
 * Parametres :
 *   <Grid>      : Grille
 *
 * Retour:
 *  <int>        : Index (ou -1 si non existante)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_CacheIdx(TGrid *Grid) {

   register int n,i=-1;

   if (Grid) {
      pthread_mutex_lock(&CacheMutex);
      for(n=0;n<GRIDCACHEMAX;n++) {
         if (GridCache[n]==Grid) {
            i=n;
            break;
         }
      }
      pthread_mutex_unlock(&CacheMutex);
   }
   return(i);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_CacheAdd>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Ajoute la grille specifiee au tableau de grilles en "cache"
 *
 * Parametres :
 *   <Grid>      : Grille
 *
 * Retour:
 *  <int>        : Index de l'emplacement de l'ajout (ou -1 si erreur)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_CacheAdd(TGrid *Grid) {

   register int n,i=-1;

   if (Grid) {
      pthread_mutex_lock(&CacheMutex);
      for(n=0;n<GRIDCACHEMAX;n++) {
         if (!GridCache[n]) {
            GridCache[n]=Grid;
            i=n;
            break;
         }
      }
      pthread_mutex_unlock(&CacheMutex);
   }
   return(i);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_CacheDel>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Supprimer la grille specifiee du tableau de grille "cache"
 *
 * Parametres :
 *   <Grid>      : Grille
 *
 * Retour:
 *  <int>        : Index de la grille supprime (ou -1 si non existante)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_CacheDel(TGrid *Grid) {

   register int n=-1;

   if (Grid) {
      pthread_mutex_lock(&CacheMutex);
      for(n=0;n<GRIDCACHEMAX;n++) {
         if (GridCache[n]==Grid) {
            GridCache[n]=NULL;
            break;
         }
      }
      pthread_mutex_unlock(&CacheMutex);
   }
   return(n);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_CopyDesc>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Copier les descripteur de grille d'un fichier dans un autre
 *
 * Parametres :
 *   <FidTo>     : Fichier dans lequel copier
 *   <Grid>      : Grille dont on veut copier les descripteurs
 *
 * Retour:
 *  <int>        : Code de reussite (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_CopyDesc(int FIdTo,TGrid *Grid) {

   TRPNHeader h;
   char      *data=NULL;
   int        ni,nj,nk;
   int        key;

   if (EZGrid_CacheFind(Grid)) {
      return(1);
   }

   pthread_mutex_lock(&RPNFieldMutex);
   key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->H.IG1,Grid->H.IG2,-1,"",">>");
   if (key<0) {
      fprintf(stderr,"EZGrid_CopyDesc: Could not find master grid descriptor >>\n");
      pthread_mutex_unlock(&RPNFieldMutex);
      return(0);
   }
   data=(char*)malloc((Grid->H.NI>Grid->H.NJ?Grid->H.NI:Grid->H.NJ)*sizeof(float));
   c_fstluk(data,key,&ni,&nj,&nk);

   strcpy(h.NOMVAR,"    ");
   strcpy(h.TYPVAR,"  ");
   strcpy(h.ETIKET,"            ");
   strcpy(h.GRTYP,"  ");
   key=c_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,
      &h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,h.GRTYP,&h.IG1,
      &h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);
   key=c_fstecr(data,NULL,-32,FIdTo,h.DATEO,h.DEET,h.NPAS,h.NI,h.NJ,h.NK,h.IP1,
      h.IP2,h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,h.DATYP,1);

  key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->H.IG1,Grid->H.IG2,-1,"","^^");
   if (key<0) {
      fprintf(stderr,"EZGrid_CopyDesc: Could not find master grid descriptor ^^\n");
      pthread_mutex_unlock(&RPNFieldMutex);
      return(0);
   }
   c_fstluk(data,key,&ni,&nj,&nk);

   key=c_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,
      &h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,h.GRTYP,&h.IG1,
      &h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);
   key=c_fstecr(data,NULL,-32,FIdTo,h.DATEO,h.DEET,h.NPAS,h.NI,h.NJ,h.NK,h.IP1,
      h.IP2,h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,h.DATYP,1);
   pthread_mutex_unlock(&RPNFieldMutex);

   EZGrid_CacheAdd(Grid);
   free(data);
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Tile>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Sauvegarder un champs en tuiles
 *
 * Parametres :
 *   <FidTo>     : Fichier dans lequel copier
 *   <NI>        : Dimension des tuiles en I
 *   <NJ>        : Dimension des tuiles en J
 *   <FId>       : Fichier source
 *   <Var>       : Variable ("" pour toutes)
 *   <TypVar>    : Type de variable  ("" pour toutes)
 *   <Etiket>    : Etiquette  ("" pour toutes)
 *   <DateV>     : Date de validite (-1 pour toutes)
 *   <IP1>       : Niveau (-1 pout tous)
 *   <IP2>       : Heure ou whatever else (-1 pout tous)
 *
 * Retour:
 *  <int>        : Code de reussite (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_Tile(int FIdTo,int NI, int NJ,int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2) {

   TGrid *new;
   int    n,no,i,j,ni,nj,nk,szd=0,pj;
   int    key;
   float *data=NULL,*tile=NULL;
   int    idlst[TILEMAX],nid;

   /*Get the number of fields*/
   cs_fstinl(FId,&ni,&nj,&nk,DateV,Etiket,IP1,IP2,-1,TypVar,Var,idlst,&nid,TILEMAX);

   if (nid<=0) {
      fprintf(stderr,"EZGrid_Read: Specified fields do not exist\n");
      return(0);
   }
   tile=(float*)malloc(NI*NJ*sizeof(float));

   /*Loop on all found fields*/
   for(n=0;n<nid;n++) {
      new=(TGrid*)malloc(sizeof(TGrid));
      new->H.FID=FId;

      strcpy(new->H.NOMVAR,"    ");
      strcpy(new->H.TYPVAR,"  ");
      strcpy(new->H.ETIKET,"            ");
      strcpy(new->H.GRTYP,"  ");
      key=cs_fstprm(idlst[n],&new->H.DATEO,&new->H.DEET,&new->H.NPAS,&new->H.NI,&new->H.NJ,&new->H.NK,&new->H.NBITS,&new->H.DATYP,
         &new->H.IP1,&new->H.IP2,&new->H.IP3,new->H.TYPVAR,new->H.NOMVAR,new->H.ETIKET,new->H.GRTYP,&new->H.IG1,&new->H.IG2,
         &new->H.IG3,&new->H.IG4,&new->H.SWA,&new->H.LNG,&new->H.DLTF,&new->H.UBC,&new->H.EX1,&new->H.EX2,&new->H.EX3);

      /*Reallocate buffer if too small*/
      if (new->H.NI*new->H.NJ>szd) {
         szd=new->H.NI*new->H.NJ;
         data=(float*)realloc(data,szd*sizeof(float));
      }

      new->IP1=new->H.IG1;
      new->IP2=new->H.IG2;
      EZGrid_CopyDesc(FIdTo,new);

      /*Read the field data*/
      cs_fstluk(data,idlst[n],&ni,&nj,&nk);

      /*Build and save the tiles, we adjust the tile size if it is too big*/
      no=0;
      for(j=0;j<new->H.NJ;j+=NJ) {
         nj=(j+NJ>new->H.NJ)?(new->H.NJ-j):NJ;
         for(i=0;i<new->H.NI;i+=NI) {
            no++;
            ni=(i+NI>new->H.NI)?(new->H.NI-i):NI;
            for(pj=0;pj<nj;pj++) {
               memcpy(&tile[pj*ni],&data[(j+pj)*new->H.NI+i],ni*sizeof(float));
            }
            key=cs_fstecr(tile,-new->H.NBITS,FIdTo,new->H.DATEO,new->H.DEET,new->H.NPAS,ni,nj,1,new->H.IP1,new->H.IP2,
               no,new->H.TYPVAR,new->H.NOMVAR,new->H.ETIKET,"#",new->H.IG1,new->H.IG2,i+1,j+1,new->H.DATYP,1);
         }
      }
   }
   free(data);
   free(tile);
   return(nid);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_UnTile>
 * Creation : Mai 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Sauvegarder un champs reconstitue de tuiles
 *
 * Parametres :
 *   <FidTo>     : Fichier dans lequel copier
 *   <FId>       : Fichier source
 *   <Var>       : Variable ("" pour toutes)
 *   <TypVar>    : Type de variable  ("" pour toutes)
 *   <Etiket>    : Etiquette  ("" pour toutes)
 *   <DateV>     : Date de validite (-1 pour toutes)
 *   <IP1>       : Niveau (-1 pout tous)
 *   <IP2>       : Heure ou whatever else (-1 pout tous)
 *
 * Retour:
 *  <int>        : Code de reussite (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_UnTile(int FIdTo,int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2) {

   TGrid *new;
   int    n,ni,nj,nk;
   int    idlst[TILEMAX],nid;

   /*Get the number of fields (tile number=1)*/
   cs_fstinl(FId,&ni,&nj,&nk,DateV,Etiket,IP1,IP2,1,TypVar,Var,idlst,&nid,TILEMAX);

   if (nid<=0) {
      fprintf(stderr,"EZGrid_Read: Specified fields do not exist\n");
      return(0);
   }

   /*Loop on all found fields*/
   for(n=0;n<nid;n++) {
      new=EZGrid_ReadIdx(FId,idlst[n],0);
      EZGrid_TileBurnAll(new,0);
   }
   return(nid);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Get>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Lire la definition de grille dans le fichier
 *
 * Parametres :
 *   <Grid>      : Grille
 *
 * Retour:
 *  <Grid>      : Grille (NULL=erreur)
 *
 * Remarques :
 *
 *    - On construit la structure de donnee de la grille sans lire les donnees elle-meme
 *    - On lit toutes les descriptions de tuiles et on en deduit le nombre en I et J
 *    - On lit tous les niveau disponibles
 *    - La procedure fonctionne aussi pour les champs non tuile. Dans ce cas on a une seule tuile.
 *----------------------------------------------------------------------------
*/
TGrid* EZGrid_Get(TGrid *Grid) {

   TRPNHeader h;
   int        n,r,i,j,k,k2,idump;
   int        ip3,key;
   int        l,idlst[TILEMAX];
   char       cdump[16];

   /*Check for master grid descriptor*/
   pthread_mutex_lock(&RPNFieldMutex);
   key=c_fstinf(Grid->H.FID,&Grid->H.NI,&h.NJ,&h.NK,-1,"",Grid->H.IG1,Grid->H.IG2,-1,"",">>");
   key=c_fstinf(Grid->H.FID,&h.NI,&Grid->H.NJ,&h.NK,-1,"",Grid->H.IG1,Grid->H.IG2,-1,"","^^");
   if (key<0) {
      fprintf(stderr,"EZGrid_Get: Could not find master grid descriptor\n");
      pthread_mutex_unlock(&RPNFieldMutex);
      return(NULL);
   }

   /*Get the number of levels*/
   ip3=Grid->H.GRTYP[0]=='#'?1:-1;
   c_fstinl(Grid->H.FID,&h.NI,&h.NJ,&h.NK,Grid->H.DATEV,Grid->H.ETIKET,-1,Grid->H.IP2,ip3,Grid->H.TYPVAR,Grid->H.NOMVAR,idlst,&Grid->H.NK,TILEMAX);
   Grid->Levels=(int*)malloc(Grid->H.NK*sizeof(int));

   /*Get the levels*/
   for(k=0;k<Grid->H.NK;k++) {
      key=c_fstprm(idlst[k],&idump,&idump,&idump,&idump,&idump,&idump,&idump,
         &idump,&Grid->Levels[k],&idump,&idump,cdump,cdump,cdump,cdump,&idump,&idump,
         &idump,&idump,&idump,&idump,&idump,&idump,&idump,&idump,&idump);
   }

   /*Sort the levels from ground up*/
   for(k=0;k<Grid->H.NK;k++) {
      for(k2=0;k2<Grid->H.NK;k2++) {
         if (Grid->Incr==1) {
            if (Grid->Levels[k]<Grid->Levels[k2]) {
               l=Grid->Levels[k];
               Grid->Levels[k]=Grid->Levels[k2];
               Grid->Levels[k2]=l;
            }
         } else {
            if (Grid->Levels[k]>Grid->Levels[k2]) {
               l=Grid->Levels[k];
               Grid->Levels[k]=Grid->Levels[k2];
               Grid->Levels[k2]=l;
            }
         }
      }
   }

   /*Get the number of tiles*/
   c_fstinl(Grid->H.FID,&h.NI,&h.NJ,&h.NK,Grid->H.DATEV,Grid->H.ETIKET,Grid->H.IP1,Grid->H.IP2,-1,Grid->H.TYPVAR,Grid->H.NOMVAR,idlst,&Grid->NbTiles,TILEMAX);
   Grid->Tiles=(TGridTile*)malloc(Grid->NbTiles*sizeof(TGridTile));
   Grid->Data=NULL;
   Grid->NTI=Grid->NTJ=0;
   Grid->IP1=Grid->H.IG1;
   Grid->IP2=Grid->H.IG2;
   i=j=-1;

   /*Parse the tiles to get the tile limits and structure*/
   for(n=0;n<Grid->NbTiles;n++) {
      key=c_fstprm(idlst[n],&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,
            &h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
            h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,
            &h.UBC,&h.EX1,&h.EX2,&h.EX3);
      if (key<0) {
         fprintf(stderr,"EZGrid_Get: Missing subgrid number %i\n",n);
         pthread_mutex_unlock(&RPNFieldMutex);
         return(NULL);
      }

      k=h.IP3>0?h.IP3-1:h.IP3;

      /*Create tile grid*/
/*
      if (k==0)
         Grid->Tiles[k].GID=c_ezqkdef(h.NI,h.NJ,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,Grid->H.FID);
*/
      Grid->Tiles[k].NO=h.IP3;
      Grid->Tiles[k].NI=h.NI;
      Grid->Tiles[k].NJ=h.NJ;
      Grid->Tiles[k].Data=NULL;
      Grid->Tiles[k].KBurn=-1;

      /*Check for tiled data or not*/
      if (Grid->H.GRTYP[0]=='#') {
         Grid->Tiles[k].I=h.IG3-1;
         Grid->Tiles[k].J=h.IG4-1;

         /*Count the number of tiles in I,J*/
         if ((r=(h.NI+h.IG3))>i) { i=r; Grid->NTI++; }
         if ((r=(h.NJ+h.IG4))>j) { j=r; Grid->NTJ++; }
      } else {
         Grid->Tiles[k].I=0;
         Grid->Tiles[k].J=0;
         Grid->NTI++;
         Grid->NTJ++;
      }
   }

   /*Create master grid*/
   if (Grid->H.GRTYP[0]=='#') {
      h.GRTYP[0]='Z';
      h.IG3=h.IG4=0;
   } else {
      h.GRTYP[0]=Grid->H.GRTYP[0];
   }
   h.GRTYP[1]='\0';

   /*c_ezqkdef uses fstd functions to get grid def so we need to keep the RPNFieldMutex on*/
   pthread_mutex_lock(&RPNIntMutex);
   Grid->GID=c_ezqkdef(Grid->H.NI,Grid->H.NJ,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,Grid->H.FID);
   pthread_mutex_unlock(&RPNIntMutex);
   pthread_mutex_unlock(&RPNFieldMutex);

   return(Grid);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Free>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Liberer la memoire de la grille specifie
 *
 * Parametres :
 *   <Grid>      : Grille
 *
 * Retour:
 *  <int>        : Code de reussite (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_free)(wordint *gdid) {
   return(EZGrid_Free(GridCache[*gdid]));
}
int EZGrid_Free(TGrid *Grid) {

   int n;

   /*Cleanup tile data*/
   for(n=0;n<Grid->NbTiles;n++) {
      if (Grid->Tiles[n].Data) {
         free(Grid->Tiles[n].Data);
         Grid->Tiles[n].Data=NULL;
      }
   }
   if (Grid->Data) {
      free(Grid->Data);
      Grid->Data=NULL;
   }

   /*If this is a master, keep in memory*/
   if (!Grid->Master) {
      if (Grid->Tiles) {
         free(Grid->Tiles);
         Grid->Tiles=NULL;
      }
      if (Grid->Levels) {
         free(Grid->Levels);
         Grid->Levels=NULL;
      }
      EZGrid_CacheDel(Grid);
   }
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Read>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Lire un champs tuile par selection en mettant en attente la lecture des donnees
 *            jusqu'a ce qu'elles soit accedees.
 *
 * Parametres :
 *   <FId>       : Fichier source
 *   <Var>       : Variable ("" pour toutes)
 *   <TypVar>    : Type de variable  ("" pour toutes)
 *   <Etiket>    : Etiquette  ("" pour toutes)
 *   <DateV>     : Date de validite (-1 pour toutes)
 *   <IP1>       : Niveau (-1 pout tous)
 *   <IP2>       : Heure ou whatever else (-1 pout tous)
 *   <Incr>      : Ordre de tri des niveaux (IP1) (-1=decroissant, 1=croissant)
 *
 * Retour:
 *   <Grid>      : Grille
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_read)(wordint *fid,char *var,char *typvar,char *etiket,wordint *datev,wordint *ip1,wordint *ip2,wordint *incr) {
   char cvar[5],ctypvar[3],cetiket[13];

   strcpy(cvar,"    ");
   strcpy(ctypvar,"  ");
   strcpy(cetiket,"            ");

   memcpy(cvar,var,4);
   memcpy(ctypvar,typvar,2);
   memcpy(cetiket,etiket,12);

   return(EZGrid_CacheIdx(EZGrid_Read(*fid,cvar,ctypvar,cetiket,*datev,*ip1,*ip2,*incr)));
}

TGrid *EZGrid_Read(int FId,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2,int Incr) {

   int    key;
   int    ni,nj,nk;

   /*Get field info*/
   key=cs_fstinf(FId,&ni,&nj,&nk,DateV,Etiket,IP1,IP2,-1,TypVar,Var);

   if (key<0) {
      fprintf(stderr,"EZGrid_Read: Specified field does not exist\n");
      return(NULL);
   }
   return(EZGrid_ReadIdx(FId,key,Incr));
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_ReadIdx>
 * Creation : Mai 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Lire un champs tuile par l'index en mettant en attente la lecture des donnees
 *            jusqu'a ce qu'elles soit accedees.
 *
 * Parametres :
 *   <Key>       : Cle de l'enregistrement
 *   <Incr>      : Ordre de tri des niveaux (IP1) (-1=decroissant, 1=croissant)
 *
 * Retour:
 *   <Grid>      : Grille
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_readidx)(wordint *fid,wordint *key,wordint *incr) {

   return(EZGrid_CacheIdx(EZGrid_ReadIdx(*fid,*key,*incr)));
}

TGrid *EZGrid_ReadIdx(int FId,int Key,int Incr) {

   TRPNHeader h;
   TGrid     *mst,*new;
   int        n;
   double     nh;
   int        idlst[TILEMAX];

   new=(TGrid*)malloc(sizeof(TGrid));
   new->H.FID=FId;
   new->Incr=Incr;

   strcpy(new->H.NOMVAR,"    ");
   strcpy(new->H.TYPVAR,"  ");
   strcpy(new->H.ETIKET,"            ");
   strcpy(new->H.GRTYP,"  ");

   pthread_mutex_lock(&RPNFieldMutex);
   Key=c_fstprm(Key,&new->H.DATEO,&new->H.DEET,&new->H.NPAS,&new->H.NI,&new->H.NJ,&new->H.NK,&new->H.NBITS,
         &new->H.DATYP,&new->H.IP1,&new->H.IP2,&new->H.IP3,new->H.TYPVAR,new->H.NOMVAR,new->H.ETIKET,
         new->H.GRTYP,&new->H.IG1,&new->H.IG2,&new->H.IG3,&new->H.IG4,&new->H.SWA,&new->H.LNG,&new->H.DLTF,
         &new->H.UBC,&new->H.EX1,&new->H.EX2,&new->H.EX3);

   nh=(new->H.NPAS*new->H.DEET)/3600.0;
   f77name(incdatr)(&new->H.DATEV,&new->H.DATEO,&nh);

   c_fstinl(new->H.FID,&h.NI,&h.NJ,&h.NK,new->H.DATEV,new->H.ETIKET,-1,new->H.IP2,1,new->H.TYPVAR,new->H.NOMVAR,idlst,&new->H.NK,TILEMAX);
   pthread_mutex_unlock(&RPNFieldMutex);

   /*Check previous master grid existence*/
   if ((mst=EZGrid_CacheFind(new))) {
      new->Master=0;
      new->Data=NULL;
      new->GID=mst->GID;
      new->H.NI=mst->H.NI;
      new->H.NJ=mst->H.NJ;
      new->IP1=mst->IP1;
      new->IP2=mst->IP2;
      new->NTI=mst->NTI;
      new->NTJ=mst->NTJ;
      new->NbTiles=mst->NbTiles;
      new->Levels=(int*)malloc(mst->H.NK*sizeof(int));
      memcpy(new->Levels,mst->Levels,mst->H.NK*sizeof(int));
      new->Tiles=(TGridTile*)malloc(mst->NbTiles*sizeof(TGridTile));
      memcpy(new->Tiles,mst->Tiles,mst->NbTiles*sizeof(TGridTile));
      for(n=0;n<mst->NbTiles;n++) {
         new->Tiles[n].Data=NULL;
         new->Tiles[n].KBurn=-1;
      }
   } else {
      if (!EZGrid_Get(new)) {
         fprintf(stderr,"EZGrid_Read: Could not create master grid\n");
         free(new);
         return(NULL);
      } else {
         new->Master=1;
      }
   }
   EZGrid_CacheAdd(new);
   return(new);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Load>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Forcer la lecture des donnees pour une region 3D de la grille
 *
 * Parametres :
 *   <Grid>      : Grille
 *   <I0>        : Coordonnee inferieure en I
 *   <J0>        : Coordonnee inferieure en J
 *   <K0>        : Coordonnee inferieure en K
 *   <I1>        : Coordonnee superieure en I
 *   <J1>        : Coordonnee superieure en J
 *   <K1>        : Coordonnee superieure en K
 *
 * Retour:
 *  <int>        : Code de reussite (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_load)(wordint *gdid,wordint *i0,wordint *j0,wordint *k0,wordint *i1,wordint *j1,wordint *k1) {
   return(EZGrid_Load(GridCache[*gdid],*i0,*j0,*k0,*i1,*j1,*k1));
}
int EZGrid_Load(TGrid *Grid,int I0,int J0,int K0,int I1,int J1,int K1) {

   TGridTile *tile;
   int i,j,k;

   if (!Grid) {
      fprintf(stderr,"EZGrid_Load: Invalid grid\n");
      return(0);
   }

   /*Check inclusion in master grid limits*/
   if (I0<0 || J0<0 || K0<0 || I0>=Grid->H.NI || J0>=Grid->H.NJ || K0>=Grid->H.NK ||
       I1<0 || J1<0 || K1<0 || I1>=Grid->H.NI || J1>=Grid->H.NJ || K1>=Grid->H.NK) {
      fprintf(stderr,"EZGrid_Load: Coordinates out of range\n");
      return(0);
   }

   /*Loop on coverage*/
   for(k=K0;k<=K1;k++) {
      for(j=J0;j<=J1;j++) {
         for(i=I0;i<=I1;i++) {
            /*Figure out the right tile*/
            if (!(tile=EZGrid_TileFind(Grid,i,j))) {
               return(0);
            }
         }
      }
   }
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_TimeInterp>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Interpolation temporelle enre deux grilles
 *
 * Parametres :
 *   <Grid0>      : Grille au temps 0
 *   <Grid1>      : Grille au temps 1
 *   <Date>       : Date stamps du temps desire (entre T0 et T1)
 *
 * Retour:
 *   <Grid>      : Grille interpolee (NULL=erreur)
 *
 * Remarques :
 *
 *   - Seule les tuiles avec des donnees sont interpolees. Il faut donc s'assurer
 *     que les donnees y sont en utilisant la fonction EZGrid_Load
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_timeinterp)(wordint *gdid0,wordint *gdid1,wordint *date) {
   return(EZGrid_CacheIdx(EZGrid_TimeInterp(GridCache[*gdid0],GridCache[*gdid1],*date)));
}
TGrid *EZGrid_TimeInterp(TGrid *Grid0,TGrid *Grid1,int Date) {

   TGrid *new;
   double delay,dt;
   int    i,n;

   /*Figure out the delai between the 2 fields*/
   f77name(difdatr)(&Grid0->H.DATEV,&Grid1->H.DATEV,&delay);
   f77name(difdatr)(&Grid0->H.DATEV,&Date,&dt);

   dt/=delay;

   /*Allocate new tile*/
   new=(TGrid*)malloc(sizeof(TGrid));
   memcpy(new,Grid0,sizeof(TGrid));
   new->Master=0;
   new->H.FID=-1;
   new->Tiles=(TGridTile*)malloc(Grid0->NbTiles*sizeof(TGridTile));
   memcpy(new->Tiles,Grid0->Tiles,Grid0->NbTiles*sizeof(TGridTile));

   for(n=0;n<Grid0->NbTiles;n++) {
      /*Interpolate data if exists*/
      if (Grid0->Tiles[n].Data && Grid1->Tiles[n].Data) {
         new->Tiles[n].Data=(float*)malloc(new->H.NI*new->H.NJ*new->H.NK*sizeof(float));
         for(i=0;i<new->H.NI*new->H.NJ*new->H.NK;i++) {
            new->Tiles[n].Data[i]=ILIN(Grid0->Tiles[n].Data[i],Grid1->Tiles[n].Data[i],dt);
         }
      } else {
         new->Tiles[n].Data=NULL;
      }
   }
   EZGrid_CacheAdd(new);
   return(new);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetLevelNb>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir le nombre de niveau disponible pour la grille specifiee
 *
 * Parametres :
 *   <Grid>       : Grille
 *
 * Retour:
 *   <int>       : Nombre de niveau
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_getlevelnb)(wordint *gdid) {
   return(EZGrid_GetLevelNb(GridCache[*gdid]));
}
int EZGrid_GetLevelNb(TGrid *Grid) {
   return(Grid->H.NK);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetLevels>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les niveaux disponible pour la grille specifiee
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Levels>     : Liste des niveaux
 *   <Type>       : Type de niveaux
 *
 * Retour:
 *   <int>       : Nombre de niveau
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_getlevels)(wordint *gdid,ftnfloat *levels,wordint *type) {
   return(EZGrid_GetLevels(GridCache[*gdid],levels,type));
}
int EZGrid_GetLevels(TGrid *Grid,float *Levels,int *Type) {

   int  k;
   int  mode=-1,flag=0;
   char format;

   if (!Grid) {
      fprintf(stderr,"EZGrid_Load: Invalid grid\n");
      return(0);
   }

   for(k=0;k<Grid->H.NK;k++) {
      f77name(convip)(&Grid->Levels[k],&Levels[k],Type,&mode,&format,&flag);
   }
   return(k);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_LLGetValue>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeur a une lat-lon pour un range de niveaux
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Lat>        : Latitude
 *   <Lon>        : Longitude
 *   <K0>         : Index du niveau 0
 *   <K1>         : Index du niveau K
 *   <Value>      : Valeur a la latlon du profile
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *
 *   - On effectue une interpolation lineaire
 *   - Cette fonction permet de recuperer un profile
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_llgetvalue)(wordint *gdid,ftnfloat *lat,ftnfloat *lon,wordint *k0,wordint *k1,ftnfloat *val) {
   return(EZGrid_LLGetValue(GridCache[*gdid],*lat,*lon,*k0,*k1,val));
}
int EZGrid_LLGetValue(TGrid *Grid,float Lat,float Lon,int K0,int K1,float *Value) {

   TGridTile *tile[4];
   float      i,j,ii,jj;
   int        ik,k,n,in[4],jn[4];

   if (!Grid) {
      fprintf(stderr,"EZGrid_LLGetValue: Invalid grid\n");
      return(0);
   }

   pthread_mutex_lock(&RPNIntMutex);
   c_gdxyfll(Grid->GID,&i,&j,&Lat,&Lon,1);
   pthread_mutex_unlock(&RPNIntMutex);
   ii=i-1.0;jj=j-1.0;

   /*Check inclusion in master grid limits*/
   if (ii<0 || jj<0 || K0<0 || K1<0 || ii>=Grid->H.NI || jj>=Grid->H.NJ || K0>=Grid->H.NK || K1>=Grid->H.NK) {
      fprintf(stderr,"EZGrid_LLGetValue: Coordinates out of range\n");
      return(0);
   }

   in[0]=in[2]=floor(ii);
   jn[0]=jn[1]=floor(jj);
   in[1]=in[3]=in[0]+1;
   jn[2]=jn[3]=jn[0]+1;

   n=4;
   while(n--) {
      if (!(tile[n]=EZGrid_TileFind(Grid,in[n],jn[n]))) {
         return(0);
      }
   }

   pthread_mutex_lock(&RPNIntMutex);
   for(k=K0;k<=K1;k++) {
      ik=k-K0;
      if (Grid->NbTiles>1) {
         n=4;
         while(n--) {
            EZGrid_TileBurn(Grid,tile[n],k);
         }
         c_gdxysval(Grid->GID,&Value[ik],Grid->Data,&i,&j,1);
      } else {
         c_gdxysval(Grid->GID,&Value[ik],&tile[0]->Data[k*Grid->H.NI*Grid->H.NJ],&i,&j,1);
      }
   }
   pthread_mutex_unlock(&RPNIntMutex);
   return(1);
}

wordint f77name(ezgrid_llgetuvvalue)(wordint *gdidu,wordint *gdidv,ftnfloat *lat,ftnfloat *lon,wordint *k0,wordint *k1,ftnfloat *uu,ftnfloat *vv) {
   return(EZGrid_LLGetUVValue(GridCache[*gdidu],GridCache[*gdidv],*lat,*lon,*k0,*k1,uu,vv));
}
int EZGrid_LLGetUVValue(TGrid *GridU,TGrid *GridV,float Lat,float Lon,int K0,int K1,float *UU,float *VV) {

   TGridTile *tile[2][4];
   float      i,j,ii,jj,d,v;
   int        ik,k,n,in[4],jn[4];

   if (!GridU || !GridV) {
      fprintf(stderr,"EZGrid_LLGetWindValue: Invalid grid\n");
      return(0);
   }

   pthread_mutex_lock(&RPNIntMutex);
   c_gdxyfll(GridU->GID,&i,&j,&Lat,&Lon,1);
   pthread_mutex_unlock(&RPNIntMutex);
   ii=i-1.0;jj=j-1.0;

   /*Check inclusion in master grid limits*/
   if (ii<0 || jj<0 || K0<0 || K1<0 || ii>=GridU->H.NI || jj>=GridU->H.NJ || K0>=GridU->H.NK || K1>=GridU->H.NK) {
      fprintf(stderr,"EZGrid_LLGetValue: Coordinates out of range\n");
      return(0);
   }

   in[0]=in[2]=floor(ii);
   jn[0]=jn[1]=floor(jj);
   in[1]=in[3]=in[0]+1;
   jn[2]=jn[3]=jn[0]+1;

   n=4;
   while(n--) {
      if (!(tile[0][n]=EZGrid_TileFind(GridU,in[n],jn[n]))) {
         return(0);
      }
      if (!(tile[1][n]=EZGrid_TileFind(GridV,in[n],jn[n]))) {
         return(0);
      }
   }

   pthread_mutex_lock(&RPNIntMutex);
   for(k=K0;k<=K1;k++) {
      ik=k-K0;
      if (GridU->NbTiles>1) {
         n=4;
         while(n--) {
            EZGrid_TileBurn(GridU,tile[0][n],k);
            EZGrid_TileBurn(GridV,tile[1][n],k);
         }
         c_gdxywdval(GridU->GID,&UU[ik],&VV[ik],GridU->Data,GridV->Data,&i,&j,1);
      } else {
         c_gdxywdval(GridU->GID,&UU[ik],&VV[ik],&tile[0][0]->Data[k*GridU->H.NI*GridU->H.NJ],&tile[1][0]->Data[k*GridV->H.NI*GridV->H.NJ],&i,&j,1);
      }
      d=DEG2RAD(VV[ik]);
      v=UU[ik]*0.515f;

      UU[ik]=-v*sinf(d);
      VV[ik]=-v*cosf(d);
   }
   pthread_mutex_unlock(&RPNIntMutex);
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_IJGetValue>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeur a une point de grille
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <I>          : Coordonnee en I
 *   <J>          : Coordonnee en J
 *   <K>          : Coordonnee en K
 *   <Val>        : Valeur au point de grille
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *
 *   - On effectue aucune interpolation
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_ijgetvalue)(wordint *gdid,wordint *i,wordint *j,wordint *k,ftnfloat *val) {
   return(EZGrid_IJGetValue(GridCache[*gdid],*i,*j,*k,val));
}
int EZGrid_IJGetValue(TGrid *Grid,int I,int J,int K,float *Value) {

   TGridTile *tile;

   if (!Grid) {
      fprintf(stderr,"EZGrid_IJGetValue: Invalid grid\n");
      return(0);
   }

   /*Check inclusion in master grid limits*/
   if (I<0 || J<0 || K<0 || I>=Grid->H.NI || J>=Grid->H.NJ || K>=Grid->H.NK) {
      fprintf(stderr,"EZGrid_IJGetValue: Coordinates out of range\n");
      return(0);
   }

   if (!(tile=EZGrid_TileFind(Grid,I,J))) {
      return(0);
   }

   /*Get the value*/
   *Value=tile->Data[(tile->NI*tile->NJ*K)+(J-tile->J)*tile->NI+(I-tile->I)];
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_IJGetValues>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeurs pour un range de points de grille
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <I0>         : Coordonnee inferieure en I
 *   <J0>         : Coordonnee inferieure en J
 *   <K0>         : Coordonnee inferieure en K
 *   <I1>         : Coordonnee superieure en I
 *   <J1>         : Coordonnee superieure en J
 *   <K1>         : Coordonnee superieure en K
 *   <Value>      : Valeurs du range
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *
 *   - On effectue aucune interpolation
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_ijgetvalues)(wordint *gdid,wordint *i0,wordint *j0,wordint *k0,wordint *i1,wordint *j1,wordint *k1,ftnfloat *val) {
   return(EZGrid_IJGetValues(GridCache[*gdid],*i0,*j0,*k0,*i1,*j1,*k1,val));
}
int EZGrid_IJGetValues(TGrid *Grid,int I0,int J0,int K0,int I1,int J1,int K1,float *Value) {

   TGridTile *tile;
   int        n=0;
   int        i,j,k;

   if (!Grid) {
      fprintf(stderr,"EZGrid_IJGetValues: Invalid grid\n");
      return(0);
   }

   /*Check inclusion in master grid limits*/
   if (I0<0 || J0<0 || K0<0 || I0>=Grid->H.NI || J0>=Grid->H.NJ || K0>=Grid->H.NK ||
       I1<0 || J1<0 || K1<0 || I1>=Grid->H.NI || J1>=Grid->H.NJ || K1>=Grid->H.NK) {
      fprintf(stderr,"EZGrid_IJGetValues: Coordinates out of range\n");
      return(0);
   }

   /*Loop on coverage*/
   for(k=K0;k<=K1;k++) {
      for(j=J0;j<=J1;j++) {
         for(i=I0;i<=I1;i++) {
            if (!(tile=EZGrid_TileFind(Grid,i,j))) {
               return(0);
            }

            /*Get the value*/
            Value[n++]=tile->Data[(tile->NI*tile->NJ*k)+(j-tile->J)*tile->NI+(i-tile->I)];
         }
      }
   }
   return(1);
}
