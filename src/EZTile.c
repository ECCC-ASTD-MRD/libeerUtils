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

#include "EZTile.h"

#include <pthread.h>

#include "rpn_macros_arch.h"
#include "/usr/local/env/armnlib/include/fnom.h"

static char FGFDTLock[MAXFILES];
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

int cs_fstfrm(int Unit) {
   int err;

   pthread_mutex_lock(&RPNFileMutex);
   err=c_fstfrm(Unit);
   err=c_fclos(Unit);
   FGFDTLock[Unit-1]=0;
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
static inline TGridTile* EZGrid_TileGet(const TGrid* restrict const Grid,int I,int J) {

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
 * But      : Recuperer les donnees d'une tuile
 *
 * Parametres :
 *   <Grid>      : Grille maitre
 *   <Tile>      : Tuile pour laquelle lire les donnees
 *   <K>         : Niveau
 *
 * Retour:
 *  <float*>     : Pointeur sur les donnees lues (ou NULL si erreur)
 *
 * Remarques :
 *      - Cette fonction implemente le read on demand et interpolate on demand
 *      - Les donnees sont lues dans le fichier standard ou interpollee
 *        entre deux champs si necessaire
 *----------------------------------------------------------------------------
*/
static float **EZGrid_TileGetData(const TGrid* restrict const Grid,TGridTile* restrict const Tile,int K) {

   TGridTile *t0,*t1;
   int        key;
   int        ni,nj,nk,t=0;

   if (!EZGrid_IsLoaded(Tile,K)) {
      /*Allocate Tile data if not already done*/
      if (!Tile->Data) {
         if (!(Tile->Data=(float**)calloc(Grid->H.NK,sizeof(float*)))) {
            fprintf(stderr,"(ERROR) EZGrid_TileGetData: Unable to allocate memory for tile data levels\n");
            return(NULL);
         }
      }
      if (!Tile->Data[K]) {
         if (!(Tile->Data[K]=(float*)malloc(Grid->H.NIJ*sizeof(float*)))) {
            fprintf(stderr,"(ERROR) EZGrid_TileGetData: Unable to allocate memory for tile data slices\n");
            return(NULL);
         }
      }

      if (Grid->H.FID>=0) {                                       /*Check for data to read*/
         pthread_mutex_lock(&RPNFieldMutex);
         key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,Grid->H.DATEV,Grid->H.ETIKET,Grid->Levels[K],Grid->H.IP2,Tile->NO,Grid->H.TYPVAR,Grid->H.NOMVAR);
         if (key<0) {
            fprintf(stderr,"(WARNING) EZGrid_TileGetData: Could not find tile data at level %i\n",Grid->Levels[K]);
         } else {
            c_fstluk(Tile->Data[K],key,&ni,&nj,&nk);
         }
         pthread_mutex_unlock(&RPNFieldMutex);
      } else if (Grid->T0 && Grid->T1) {  /*Check for time interpolation needs*/
         /*Figure out T0 and T1 tile to use*/
         if (Grid->NbTiles>1) {
            while(&Grid->Tiles[t]!=Tile) t++;
         }
         t0=&(Grid->T0->Tiles[t]);
         t1=&(Grid->T1->Tiles[t]);

         /*Make sure the data form the needed tile is loaded*/
         EZGrid_TileGetData(Grid->T0,t0,K);
         EZGrid_TileGetData(Grid->T1,t1,K);

         /*Interpolate between by applying factors*/
         if (Grid->FT0==0.0) {
            if (Grid->FT1==1.0) {
               memcpy(Tile->Data[K],t1->Data[K],Tile->NIJ*sizeof(float));
            } else {
               for(ni=0;ni<Tile->NIJ;ni++) {
                  Tile->Data[K][ni]=t1->Data[K][ni]*Grid->FT1;
               }
            }
         } else if (Grid->FT1==0.0) {
            if (Grid->FT0==1.0) {
               memcpy(Tile->Data[K],t0->Data[K],Tile->NIJ*sizeof(float));
            } else {
               for(ni=0;ni<Tile->NIJ;ni++) {
                  Tile->Data[K][ni]=t0->Data[K][ni]*Grid->FT0;
               }
            }
         } else {
            for(ni=0;ni<Tile->NIJ;ni++) {
               Tile->Data[K][ni]=t0->Data[K][ni]*Grid->FT0+t1->Data[K][ni]*Grid->FT1;
            }
         }
      }

      /*Apply Factor if needed*/
      if (Grid->Factor!=0.0) {
         for(ni=0;ni<Tile->NIJ;ni++) {
            Tile->Data[K][ni]*=Grid->Factor;
         }
      }
   }
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
 *   <K>         : Coordonnee du point de grille en K
 *
 * Retour:
 *  <tile*>      : Pointeur sur la tuile (ou NULL si erreur)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
static inline TGridTile* EZGrid_TileFind(const TGrid* restrict const Grid,int I,int J,int K) {

   TGridTile *tile;

   /*Figure out the right tile*/
   if (!(tile=EZGrid_TileGet(Grid,I,J))) {
      return(NULL);
   }

   /*Check for tile data*/
   if (!EZGrid_TileGetData(Grid,tile,K)) {
      fprintf(stderr,"(ERROR) EZGrid_TileFind: Unable to get tile data\n");
      return(NULL);
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
float* EZGrid_TileBurn(TGrid* restrict const Grid,TGridTile* restrict const Tile,int K) {

   int j;

   if (Tile->KBurn!=-1 && Tile->KBurn==K) {
      return(Grid->Data);
   }

   if (!Tile || !Tile->Data) {
      fprintf(stderr,"(ERROR) EZGrid_TileBurn: Invalid tile\n");
      return(NULL);
   }

   if (!Grid->Data) {
      if (!(Grid->Data=(float*)malloc(Grid->H.NIJ*sizeof(float)))) {
         fprintf(stderr,"(ERROR) EZGrid_TileBurn: Unable to allocate memory for grid data\n");
         return(NULL);
      }
   }

   for(j=0;j<Tile->NJ;j++) {
      memcpy(&Grid->Data[(Tile->J+j)*Grid->H.NI+Tile->I],&Tile->Data[K][j*Tile->NI],Tile->NI*sizeof(float));
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
float* EZGrid_TileBurnAll(TGrid* restrict const Grid,int K) {

   int t;

   if (Grid->NbTiles>1) {
      for(t=0;t<Grid->NbTiles;t++) {
         if (!Grid->Tiles[t].Data) {
            EZGrid_TileGetData(Grid,&Grid->Tiles[t],K);
         }
         if (!EZGrid_TileBurn(Grid,&Grid->Tiles[t],K)) {
            return(NULL);
         }
      }
      return(Grid->Data);
   } else {
      EZGrid_TileGetData(Grid,&Grid->Tiles[0],K);
      return(Grid->Tiles[0].Data[K]);
   }
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
static TGrid* EZGrid_CacheFind(const TGrid* restrict const Grid) {

   register int n;

   if (Grid) {
      pthread_mutex_lock(&CacheMutex);
      for(n=0;n<GRIDCACHEMAX;n++) {
         if (GridCache[n]) {
            if (Grid->H.GRTYP[0]=='#') {
               if (GridCache[n]->IP1==Grid->H.IG1 && GridCache[n]->IP2==Grid->H.IG2 && GridCache[n]->H.NK==Grid->H.NK) {
                  pthread_mutex_unlock(&CacheMutex);
                  return(GridCache[n]);
               }
            } else if (Grid->H.GRTYP[0]=='Z') {
               if (GridCache[n]->IP1==Grid->H.IG1 && GridCache[n]->IP2==Grid->H.IG2 && GridCache[n]->IP3==Grid->H.IG3 && GridCache[n]->H.NK==Grid->H.NK) {
                  pthread_mutex_unlock(&CacheMutex);
                  return(GridCache[n]);
               }
            } else if (GridCache[n]->H.GRTYP[0]==Grid->H.GRTYP[0] && GridCache[n]->H.IG1==Grid->H.IG1 && GridCache[n]->H.IG2==Grid->H.IG2 && GridCache[n]->H.IG3==Grid->H.IG3 && GridCache[n]->H.NK==Grid->H.NK) {
               pthread_mutex_unlock(&CacheMutex);
               return(GridCache[n]);
            }
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
static inline int EZGrid_CacheIdx(const TGrid* restrict const Grid) {

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
static inline int EZGrid_CacheAdd(TGrid* restrict const Grid) {

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
static inline int EZGrid_CacheDel(const TGrid* restrict const Grid) {

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
int EZGrid_CopyDesc(int FIdTo,TGrid* restrict const Grid) {

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
      fprintf(stderr,"(WARNING) EZGrid_CopyDesc: Could not find master grid descriptor >>\n");
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
      fprintf(stderr,"(ERROR) EZGrid_CopyDesc: Could not find master grid descriptor ^^\n");
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
   int    key,n,no,i,j,ni,nj,nk,szd=0,pj;
   float *data=NULL,*tile=NULL;
   int    idlst[TILEMAX],nid;

   /*Get the number of fields*/
   cs_fstinl(FId,&ni,&nj,&nk,DateV,Etiket,IP1,IP2,-1,TypVar,Var,idlst,&nid,TILEMAX);

   if (nid<=0) {
      fprintf(stderr,"(ERROR) EZGrid_Tile: Specified fields do not exist\n");
      return(0);
   }
   tile=(float*)malloc(NI*NJ*sizeof(float));

   /*Loop on all found fields*/
   for(n=0;n<nid;n++) {
      new=EZGrid_New();
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
      fprintf(stderr,"(ERROR) EZGrid_UnTile: Specified fields do not exist\n");
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
TGrid* EZGrid_Get(TGrid* restrict const Grid) {

   TRPNHeader h;
   int        n,r,i,j,k,k2,idump;
   int        ip3,key;
   int        l,idlst[TILEMAX];
   char       cdump[16];

   /*Check for master grid descriptor*/
   pthread_mutex_lock(&RPNFieldMutex);
   if (Grid->H.GRTYP[0]=='#') {
      key=c_fstinf(Grid->H.FID,&Grid->H.NI,&h.NJ,&h.NK,-1,"",Grid->H.IG1,Grid->H.IG2,-1,"",">>");
      key=c_fstinf(Grid->H.FID,&h.NI,&Grid->H.NJ,&h.NK,-1,"",Grid->H.IG1,Grid->H.IG2,-1,"","^^");
      if (key<0) {
         fprintf(stderr,"(WARNING) EZGrid_Get: Could not find master grid descriptor (>>,^^)\n");
         pthread_mutex_unlock(&RPNFieldMutex);
         return(NULL);
      }
   }

   /*Get the number of levels*/
   /*In case of # grid, set IP1 to 1 to get NK just for the first tile*/
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
   Grid->IP3=Grid->H.IG3;
   Grid->H.NIJ=Grid->H.NI*Grid->H.NJ;
   i=j=-1;

   /*Parse the tiles to get the tile limits and structure*/
   for(n=0;n<Grid->NbTiles;n++) {
      key=c_fstprm(idlst[n],&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,
            &h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
            h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,
            &h.UBC,&h.EX1,&h.EX2,&h.EX3);
      if (key<0) {
         fprintf(stderr,"(ERROR) EZGrid_Get: Missing subgrid number %i\n",n);
         pthread_mutex_unlock(&RPNFieldMutex);
         return(NULL);
      }

      k=h.IP3>0?h.IP3-1:h.IP3;

      /*Create tile grid*/
      Grid->Tiles[k].NO=h.IP3;
      Grid->Tiles[k].NI=h.NI;
      Grid->Tiles[k].NJ=h.NJ;
      Grid->Tiles[k].NIJ=h.NI*h.NJ;
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
 * Nom      : <EZGrid_New>
 * Creation : Novembre 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Allouer une nouvell grille
 *
 * Parametres :
 *
 * Retour:
 *  <TGrid>   : Nouvelle Grille (NULL=Error)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
TGrid *EZGrid_New() {

   TGrid *new=NULL;

   if ((new=(TGrid*)malloc(sizeof(TGrid)))) {
      new->GID=-1;
      new->Master=0;
      new->IP1=new->IP2=new->IP3=0;
      new->Factor=0.0f;
      new->Incr=0;
      new->Levels=NULL;
      new->Data=NULL;
      new->NTI=new->NTJ=0;
      new->FT0=new->FT1=0.0f;
      new->T0=new->T1=NULL;
      new->NbTiles=0;
      new->Tiles=NULL;
   }
   return(new);
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
int EZGrid_Free(TGrid* restrict const Grid) {

   int n,k;

   /*Cleanup tile data*/
   for(n=0;n<Grid->NbTiles;n++) {
      if (Grid->Tiles[n].Data) {
         for(k=0;k<Grid->H.NK;k++) {
            if (Grid->Tiles[n].Data[k])
               free(Grid->Tiles[n].Data[k]);
         }
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
wordint f77name(ezgrid_free)(wordint *gdid) {
   return(EZGrid_Free(GridCache[*gdid]));
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
      fprintf(stderr,"(ERROR) EZGrid_Read: Specified field does not exist\n");
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
   int        n,ip3;
   double     nh;
   int        idlst[TILEMAX];

   new=EZGrid_New();
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

   /*Get the number of levels*/
   /*In case of # grid, set IP1 to 1 to get NK just for the first tile*/
   ip3=new->H.GRTYP[0]=='#'?1:-1;
   c_fstinl(new->H.FID,&h.NI,&h.NJ,&h.NK,new->H.DATEV,new->H.ETIKET,-1,new->H.IP2,ip3,new->H.TYPVAR,new->H.NOMVAR,idlst,&new->H.NK,TILEMAX);
   pthread_mutex_unlock(&RPNFieldMutex);

   /*Check previous master grid existence*/
   if ((mst=EZGrid_CacheFind(new))) {
      new->Master=0;
      new->Data=NULL;
      new->GID=mst->GID;
      new->H.NI=mst->H.NI;
      new->H.NJ=mst->H.NJ;
      new->H.NK=mst->H.NK;
      new->H.NIJ=mst->H.NIJ;
      new->IP1=mst->IP1;
      new->IP2=mst->IP2;
      new->IP3=mst->IP3;
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
         fprintf(stderr,"(ERROR) EZGrid_Read: Could not create master grid\n");
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
int EZGrid_Load(const TGrid* restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1) {

   int i,j,k;

   if (!Grid) {
      fprintf(stderr,"(ERROR) EZGrid_Load: Invalid grid\n");
      return(0);
   }

   /*Check inclusion in master grid limits*/
   if (I0<0 || J0<0 || K0<0 || I0>=Grid->H.NI || J0>=Grid->H.NJ || K0>=Grid->H.NK ||
       I1<0 || J1<0 || K1<0 || I1>=Grid->H.NI || J1>=Grid->H.NJ || K1>=Grid->H.NK) {
      fprintf(stderr,"(WARNING) EZGrid_Load: Coordinates out of range\n");
      return(0);
   }

   /*Loop on coverage*/
   for(k=K0;k<=K1;k++) {
      for(j=J0;j<=J1;j++) {
         for(i=I0;i<=I1;i++) {
            /*Figure out the right tile*/
            if (!(EZGrid_TileFind(Grid,i,j,k))) {
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
wordint f77name(ezgrid_interptime)(wordint *gdid0,wordint *gdid1,wordint *date) {
   return(EZGrid_CacheIdx(EZGrid_InterpTime(GridCache[*gdid0],GridCache[*gdid1],*date)));
}

TGrid *EZGrid_InterpTime(const TGrid* restrict const Grid0,const TGrid* restrict const Grid1,int Date) {

   TGrid *new;
   double delay,dt;
   int    i,n,k;

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
         new->Tiles[n].Data=(float**)calloc(new->H.NK,sizeof(float*));
         for(k=0;k<new->H.NK;k++) {
            new->Tiles[n].Data[k]=(float*)malloc(new->H.NIJ*sizeof(float));
            for(i=0;i<new->H.NIJ;i++) {
               new->Tiles[n].Data[k][i]=ILIN(Grid0->Tiles[n].Data[k][i],Grid1->Tiles[n].Data[k][i],dt);
            }
         }
      } else {
         new->Tiles[n].Data=NULL;
      }
   }
   EZGrid_CacheAdd(new);
   return(new);
}

wordint f77name(ezgrid_factor)(wordint *gdid,ftnfloat *f) {
   EZGrid_Factor(GridCache[*gdid],*f);
   return(1);
}

void EZGrid_Factor(TGrid* restrict Grid,const float Factor) {

   Grid->Factor=Factor;
}

wordint f77name(ezgrid_interp)(wordint *gdid0,wordint *gdid1,ftnfloat *f0,ftnfloat *f1) {
   return(EZGrid_CacheIdx(EZGrid_Interp(GridCache[*gdid0],GridCache[*gdid1],*f0,*f1)));
}

TGrid *EZGrid_Interp(TGrid* restrict const Grid0,TGrid* restrict const Grid1,float Factor0,float Factor1) {

   TGrid *new;
   int    i;

   /*Allocate new tile*/
   new=EZGrid_New();
   memcpy(new,Grid0,sizeof(TGrid));

   new->Levels=NULL;
   new->Data=NULL;
   new->Master=0;
   new->H.FID=-1;
   new->Tiles=(TGridTile*)malloc(Grid0->NbTiles*sizeof(TGridTile));
   memcpy(new->Tiles,Grid0->Tiles,Grid0->NbTiles*sizeof(TGridTile));
   for(i=0;i<Grid0->NbTiles;i++) {
      new->Tiles[i].Data=NULL;
   }

   new->T0=Grid0;
   new->T1=Grid1;
   new->FT0=Factor0;
   new->FT1=Factor1;

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
int EZGrid_GetLevelNb(const TGrid* restrict const Grid) {
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
int EZGrid_GetLevels(const TGrid* restrict const Grid,float* restrict Levels,int* restrict Type) {

   int  k;
   int  mode=-1,flag=0;
   char format;

   if (!Grid) {
      fprintf(stderr,"(ERROR) EZGrid_GetLevels Invalid grid\n");
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
   return(EZGrid_LLGetValue(GridCache[*gdid],*lat,*lon,*k0-1,*k1-1,val));
}
int EZGrid_LLGetValue(TGrid* restrict const Grid,float Lat,float Lon,int K0,int K1,float* restrict Value) {

   float i,j;

   if (!Grid) {
      fprintf(stderr,"(ERROR) EZGrid_LLGetValue: Invalid grid\n");
      return(0);
   }

   pthread_mutex_lock(&RPNIntMutex);
   c_gdxyfll(Grid->GID,&i,&j,&Lat,&Lon,1);
   pthread_mutex_unlock(&RPNIntMutex);

   return(EZGrid_IJGetValue(Grid,i-1.0,j-1.0,K0,K1,Value));
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_LLGetUVValue>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeur des vents a une lat-lon pour un range de niveaux
 *
 * Parametres :
 *   <GridU>      : Grille de la composante U
 *   <GridV>      : Grille de la composante V
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
wordint f77name(ezgrid_llgetuvvalue)(wordint *gdidu,wordint *gdidv,ftnfloat *lat,ftnfloat *lon,wordint *k0,wordint *k1,ftnfloat *uu,ftnfloat *vv) {
   return(EZGrid_LLGetUVValue(GridCache[*gdidu],GridCache[*gdidv],*lat,*lon,*k0-1,*k1-1,uu,vv));
}
int EZGrid_LLGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,float Lat,float Lon,int K0,int K1,float* restrict UU,float* restrict VV) {

   float i,j;

   if (!GridU || !GridV) {
      fprintf(stderr,"(ERROR) EZGrid_LLGetUVValue: Invalid grid\n");
      return(0);
   }

   pthread_mutex_lock(&RPNIntMutex);
   c_gdxyfll(GridU->GID,&i,&j,&Lat,&Lon,1);
   pthread_mutex_unlock(&RPNIntMutex);

   return(EZGrid_IJGetUVValue(GridU,GridV,i-1.0,j-1.0,K0,K1,UU,VV));
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_IJGetValue>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeur a une coordonnee IJ en point de grille pour un range de niveaux
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <I>          : Coordonnee en X
 *   <J>          : Coordonnee en Y
 *   <K0>         : Index du niveau 0
 *   <K1>         : Index du niveau K
 *   <Value>      : Valeur a la position
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
wordint f77name(ezgrid_ijgetvalue)(wordint *gdid,ftnfloat *i,ftnfloat *j,wordint *k0,wordint *k1,ftnfloat *val) {
   return(EZGrid_IJGetValue(GridCache[*gdid],*i-1,*j-1,*k0-1,*k1-1,val));
}
int EZGrid_IJGetValue(TGrid* restrict const Grid,float I,float J,int K0,int K1,float* restrict Value) {

   TGridTile *tile[4];
   int        k,n,ik=0,in[4],jn[4];

   if (!Grid) {
      fprintf(stderr,"(ERROR) EZGrid_IJGetValue: Invalid grid\n");
      return(0);
   }

   /*Check inclusion in master grid limits*/
   if (I<0 || J<0 || K0<0 || K1<0 || I>=Grid->H.NI || J>=Grid->H.NJ || K0>=Grid->H.NK || K1>=Grid->H.NK) {
      fprintf(stderr,"(WARNING) EZGrid_IJGetValue: Coordinates out of range\n");
      return(0);
   }

   if (Grid->NbTiles>1) {
      in[0]=in[2]=floor(I);
      jn[0]=jn[1]=floor(J);
      in[1]=in[3]=in[0]+1;
      jn[2]=jn[3]=jn[0]+1;

      n=4;
      while(n--) {
         k=K0;
         do {
            if (!(tile[n]=EZGrid_TileFind(Grid,in[n],jn[n],k))) {
               return(0);
            }
         } while ((K0<=K1?k++:k--)!=K1);
      }
   }

   I++;J++;

   k=K0;
   do {
      if (Grid->NbTiles>1) {
         n=4;
         while(n--) {
            EZGrid_TileBurn(Grid,tile[n],k);
         }
         pthread_mutex_lock(&RPNIntMutex);
         c_gdxysval(Grid->GID,&Value[ik++],Grid->Data,&I,&J,1);
         pthread_mutex_unlock(&RPNIntMutex);
      } else {
         EZGrid_TileGetData(Grid,&Grid->Tiles[0],k);
         pthread_mutex_lock(&RPNIntMutex);
         c_gdxysval(Grid->GID,&Value[ik++],Grid->Tiles[0].Data[k],&I,&J,1);
         pthread_mutex_unlock(&RPNIntMutex);
      }
   } while ((K0<=K1?k++:k--)!=K1);
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_IJGetUVValue>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeur des vents a une coordonnee IJ en point de
 *            grille pour un range de niveaux
 *
 * Parametres :
 *   <GridU>      : Grille de la composante U
 *   <GridV>      : Grille de la composante V
 *   <I>          : Coordonnee en X
 *   <J>          : Coordonnee en Y
 *   <K0>         : Index du niveau 0
 *   <K1>         : Index du niveau K
 *   <Value>      : Valeur a la position
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
wordint f77name(ezgrid_ijgetuvvalue)(wordint *gdidu,wordint *gdidv,ftnfloat *i,ftnfloat *j,wordint *k0,wordint *k1,ftnfloat *uu,ftnfloat *vv) {
   return(EZGrid_IJGetUVValue(GridCache[*gdidu],GridCache[*gdidv],*i-1,*j-1,*k0-1,*k1-1,uu,vv));
}
int EZGrid_IJGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,float I,float J,int K0,int K1,float *UU,float* restrict VV) {

   TGridTile *tile[2][4];
   float      d,v;
   int        ik=0,k,n,in[4],jn[4];

   if (!GridU || !GridV) {
      fprintf(stderr,"(ERROR) EZGrid_IJGetUVValue: Invalid grid\n");
      return(0);
   }

   /*Check inclusion in master grid limits*/
   if (I<0 || J<0 || K0<0 || K1<0 || I>=GridU->H.NI || J>=GridU->H.NJ || K0>=GridU->H.NK || K1>=GridU->H.NK) {
      fprintf(stderr,"(WARNING) EZGrid_IJGetUVValue: Coordinates out of range\n");
      return(0);
   }

   if (GridU->NbTiles>1) {
      in[0]=in[2]=floor(I);
      jn[0]=jn[1]=floor(J);
      in[1]=in[3]=in[0]+1;
      jn[2]=jn[3]=jn[0]+1;

      n=4;
      while(n--) {
         k=K0;
         do {
            if (!(tile[0][n]=EZGrid_TileFind(GridU,in[n],jn[n],k))) {
               return(0);
            }
            if (!(tile[1][n]=EZGrid_TileFind(GridV,in[n],jn[n],k))) {
               return(0);
            }
         } while ((K0<=K1?k++:k--)!=K1);
      }
   }

   I++;J++;

   k=K0;
   do {
      if (GridU->NbTiles>1) {
         n=4;
         while(n--) {
            EZGrid_TileBurn(GridU,tile[0][n],k);
            EZGrid_TileBurn(GridV,tile[1][n],k);
         }
         pthread_mutex_lock(&RPNIntMutex);
         c_gdxywdval(GridU->GID,&UU[ik],&VV[ik],GridU->Data,GridV->Data,&I,&J,1);
         pthread_mutex_unlock(&RPNIntMutex);
      } else {
         EZGrid_TileGetData(GridU,&GridU->Tiles[0],k);
         EZGrid_TileGetData(GridV,&GridV->Tiles[0],k);
         pthread_mutex_lock(&RPNIntMutex);
         c_gdxywdval(GridU->GID,&UU[ik],&VV[ik],GridU->Tiles[0].Data[k],GridV->Tiles[0].Data[k],&I,&J,1);
         pthread_mutex_unlock(&RPNIntMutex);
      }
      d=DEG2RAD(VV[ik]);
      v=UU[ik]*0.515f;

      UU[ik]=-v*sinf(d);
      VV[ik]=-v*cosf(d);
      ik++;
   } while ((K0<=K1?k++:k--)!=K1);
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetValue>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeur a une point de grille
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <I>          : Coordonnee en I
 *   <J>          : Coordonnee en J
 *   <K0>         : Index du niveau 0
 *   <K1>         : Index du niveau K
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
wordint f77name(ezgrid_getvalue)(wordint *gdid,wordint *i,wordint *j,wordint *k0,wordint *k1,ftnfloat *val) {
   return(EZGrid_GetValue(GridCache[*gdid],*i-1,*j-1,*k0-1,*k1-1,val));
}
int EZGrid_GetValue(const TGrid* restrict const Grid,int I,int J,int K0,int K1,float* restrict Value) {

   TGridTile *tile;
   int        k,ik=0;

   if (!Grid) {
      fprintf(stderr,"(ERROR) EZGrid_GetValue: Invalid grid\n");
      return(0);
   }

   /*Check inclusion in master grid limits*/
   if (I<0 || J<0 || K0<0 || K1<0 || I>=Grid->H.NI || J>=Grid->H.NJ || K0>=Grid->H.NK || K1>=Grid->H.NK) {
      fprintf(stderr,"(WARNING) EZGrid_GetValue: Coordinates out of range\n");
      return(0);
   }

   k=K0;
   do {
      if (Grid->NbTiles>1) {
         if (!(tile=EZGrid_TileFind(Grid,I,J,k))) {
            return(0);
         }
      } else {
         tile=&Grid->Tiles[0];
         if (!EZGrid_IsLoaded(tile,k)) {
            EZGrid_TileGetData(Grid,tile,k);
         }
      }
      Value[ik++]=EZGrid_TileValue(tile,I,J,k);
   } while ((K0<=K1?k++:k--)!=K1);

   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetValues>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeurs a plusieurs points de grilles
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <I>          : Coordonnees en I
 *   <J>          : Coordonnees en J
 *   <K>          : Coordonnees en K
 *   <Val>        : Valeurs au points de grilles
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *
 *   - On effectue aucune interpolation
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_getvalues)(wordint *gdid,wordint *nb,ftnfloat *i,ftnfloat *j,ftnfloat *k,ftnfloat *val) {
   return(EZGrid_GetValues(GridCache[*gdid],-(*nb),i,j,k,val));
}
int EZGrid_GetValues(const TGrid* restrict const Grid,int Nb,float* restrict const I,float* restrict const J,float* restrict const K,float* restrict Value) {

   TGridTile *tile;
   int        n,i,j,k;

   if (!Grid) {
      fprintf(stderr,"(ERROR) EZGrid_GetValue: Invalid grid\n");
      return(0);
   }

   for(n=0;n<ABS(Nb);n++) {

      if (Nb<1) {
         i=I[n]-1;
         j=J[n]-1;
         k=Grid->H.NK>1?K[n]-1:1;
      } else {
         i=I[n];
         j=J[n];
         k=Grid->H.NK>1?K[n]:0;
      }

      /*Check inclusion in master grid limits*/
      if (i<0 || j<0 || k<0 || i>=Grid->H.NI || j>=Grid->H.NJ || k>=Grid->H.NK) {
         fprintf(stderr,"(WARNING) EZGrid_GetValue: Coordinates out of range\n");
         return(0);
      }

      if (Grid->NbTiles>1) {
         if (!(tile=EZGrid_TileFind(Grid,i,j,k))) {
            return(0);
         }
      } else {
         tile=&Grid->Tiles[0];
         if (!EZGrid_IsLoaded(tile,k)) {
            EZGrid_TileGetData(Grid,tile,k);
         }
      }
      Value[n]=EZGrid_TileValue(tile,i,j,k);
   }

   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetArray>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir un pointeru sur les valeurs
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <K>          : Niveau
 *   <Value>      : Valeurs
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *
 *   - On effectue aucune interpolation
 *----------------------------------------------------------------------------
*/
int f77name(ezgrid_getarray)(wordint *gdid,wordint *k,ftnfloat *vals) {
   return(EZGrid_GetArray(GridCache[*gdid],*k-1,vals));
}
int EZGrid_GetArray(TGrid* restrict const Grid,int K,float* restrict Value) {

   float *data;

   if (!Grid) {
      fprintf(stderr,"(ERROR) EZGrid_IJGetValues: Invalid grid\n");
      return(0);
   }

   /*Check inclusion in master grid limits*/
   if (K<0 || K>=Grid->H.NK) {
      fprintf(stderr,"(WARNING) EZGrid_IJGetValues: Coordinates out of range\n");
      return(0);
   }
   data=EZGrid_TileBurnAll(Grid,K);
   memcpy(Value,data,Grid->H.NI*Grid->H.NI*sizeof(float));

   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetRange>
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
wordint f77name(ezgrid_getrange)(wordint *gdid,wordint *i0,wordint *j0,wordint *k0,wordint *i1,wordint *j1,wordint *k1,ftnfloat *val) {
   return(EZGrid_GetRange(GridCache[*gdid],*i0-1,*j0-1,*k0-1,*i1-1,*j1-1,*k1-1,val));
}
int EZGrid_GetRange(const TGrid* restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1,float* restrict Value) {

   TGridTile *tile;
   int        ik=0;
   int        i,j,k;

   if (!Grid) {
      fprintf(stderr,"(ERROR) EZGrid_IJGetValues: Invalid grid\n");
      return(0);
   }

   /*Check inclusion in master grid limits*/
   if (I0<0 || J0<0 || K0<0 || I0>=Grid->H.NI || J0>=Grid->H.NJ || K0>=Grid->H.NK ||
       I1<0 || J1<0 || K1<0 || I1>=Grid->H.NI || J1>=Grid->H.NJ || K1>=Grid->H.NK) {
      fprintf(stderr,"(WARNING) EZGrid_IJGetValues: Coordinates out of range\n");
      return(0);
   }

   /*Loop on coverage*/
   for(k=K0;k<=K1;k++) {
      for(j=J0;j<=J1;j++) {
         for(i=I0;i<=I1;i++) {
            if (Grid->NbTiles>1) {
               if (!(tile=EZGrid_TileFind(Grid,i,j,k))) {
                  return(0);
               }
            } else {
               EZGrid_TileGetData(Grid,&Grid->Tiles[0],k);
               tile=&Grid->Tiles[0];
            }
            /*Get the value*/
            Value[ik++]=EZGrid_TileValue(tile,i,j,k);
         }
      }
   }
   return(1);
}
