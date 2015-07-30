/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie de gestion des champs RPN en tuiles
 * Creation     : Janvier 2008
 * Auteur       : Jean-Philippe Gauthier
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

#include "App.h"
#include "EZTile.h"

static pthread_mutex_t CacheMutex=PTHREAD_MUTEX_INITIALIZER;
static TGrid          *GridCache[GRIDCACHEMAX];

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Wrap>
 * Creation : Mai 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Tester si une grille est globale (wrap-around)
 *
 * Parametres :
 *   <Grid>   : Grille maitre
 *
 * Retour:
 *  <Bool>   : (0:no wrap, 1: wrap)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_Wrap(TGrid* __restrict const Grid) {

   float i,j,lat,lon;

   Grid->Wrap=0;
   Grid->Pole[0]=Grid->Pole[1]=0.0f;

   if (Grid->H.GRTYP[0]!='M' && Grid->H.GRTYP[0]!='W') {
   
   //   RPN_IntLock();
      // Check for south pole coverage
      i=1.0;j=1.0;
      c_gdllfxy(Grid->GID,&lat,&lon,&i,&j,1);
      if (lat==-90.0) {
         i=1.0;j=1.5;
         c_gdllfxy(Grid->GID,&Grid->Pole[0],&lon,&i,&j,1);
      }

      // Check for north pole coverage
      i=1.0;j=Grid->H.NJ;
      c_gdllfxy(Grid->GID,&lat,&lon,&i,&j,1);
      if (lat==90.0) {
         i=1.0;j=Grid->H.NJ-0.5;
         c_gdllfxy(Grid->GID,&Grid->Pole[1],&lon,&i,&j,1);
      }

      i=Grid->H.NI+1.0f;
      j=Grid->H.NJ/2.0f;
      c_gdllfxy(Grid->GID,&lat,&lon,&i,&j,1);
      c_gdxyfll(Grid->GID,&i,&j,&lat,&lon,1);

   //   RPN_IntUnlock();

      if (Grid->H.GRTYP[0]=='A' || Grid->H.GRTYP[0]=='B' || Grid->H.GRTYP[0]=='G') {
         Grid->Wrap=1;
      } else {
         // If the grid wraps
         if (i<Grid->H.NI) {
            // check if the last gridpoint is a repeat of the first
            if (rintf(i)==1.0f) {
               Grid->Wrap=1;
            } else {
               Grid->Wrap=2;
            }
         }
      }
   }
   
   return(Grid->Wrap);
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
static inline TGridTile* EZGrid_TileGet(const TGrid* __restrict const Grid,int I,int J) {

   TGridTile *tile=NULL;

   if (Grid->NbTiles>1) {
      I/=Grid->Tiles[0].NI;
      J/=Grid->Tiles[0].NJ;

      if (I<Grid->NTI && J<Grid->NTJ) {
         tile=&Grid->Tiles[J*Grid->NTI+I];
      }
   } else {
      tile=&Grid->Tiles[0];
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
 *      - On utilise des variables (data et datak) temporaire pour les allocations
 *        afin de limiter le nombre de mutex lock
 *----------------------------------------------------------------------------
*/
static float **EZGrid_TileGetData(const TGrid* __restrict const Grid,TGridTile* __restrict const Tile,int K,int Safe) {

   TGridTile *t0,*t1;
   int        key;
   int        ni,nj,nk,t=0,k;
   int        flag=0,ip1=0,mode=2,type;
   char       format;
   float    **data,*datak;
   
   if (!Safe) pthread_mutex_lock(&Tile->Mutex);

   if (!EZGrid_IsLoaded(Tile,K)) {

      /*Allocate Tile data if not already done*/
      if (!Tile->Data) {
         if (!(data=(float**)calloc(Grid->H.NK,sizeof(float*)))) {
            App_Log(ERROR,"%s: Unable to allocate memory for tile data levels (%s)",__func__,Grid->H.NOMVAR);
            if (!Safe) pthread_mutex_unlock(&Tile->Mutex);
            return(NULL);
         }
      } else {
         data=Tile->Data;
      }

      k=K;

      if (!data[k]) {
         if (!(datak=(float*)calloc(Tile->HNIJ,sizeof(float)))) {
            App_Log(ERROR,"%s: Unable to allocate memory for tile data slices (%s)\n",__func__,Grid->H.NOMVAR);
            if (!Safe) pthread_mutex_unlock(&Tile->Mutex);
            return(NULL);
         }
      } else {
         datak=data[k];
      }

      if (Grid->H.FID>=0) {                                       /*Check for data to read*/
         RPN_FieldLock();
         mode=2;
         type=Grid->ZRef->Type==LVL_ETA?LVL_SIGMA:Grid->ZRef->Type;
         f77name(convip)(&ip1,&Grid->ZRef->Levels[k],&type,&mode,&format,&flag);
         key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,Grid->H.DATEV,Grid->H.ETIKET,ip1,Grid->H.IP2,Tile->NO,Grid->H.TYPVAR,Grid->H.NOMVAR);
//         c_fst_data_length((Grid->H.DATYP==1 || Grid->H.DATYP==5)?Grid->H.NBITS>32?8:4)?(Grid->H.NBITS>8?(Grid->H.NBITS>16?(Grid->H.NBITS>32?8:4):2):1));
         if (key<0) {
            mode=3;
            f77name(convip)(&ip1,&Grid->ZRef->Levels[k],&type,&mode,&format,&flag);
            key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,Grid->H.DATEV,Grid->H.ETIKET,ip1,Grid->H.IP2,Tile->NO,Grid->H.TYPVAR,Grid->H.NOMVAR);
            if (key<0) {
               App_Log(WARNING,"%s: Could not find tile data (%s) at level %f (%i)\n",__func__,Grid->H.NOMVAR,Grid->ZRef->Levels[k],ip1);
            } else {
               c_fstluk(datak,key,&ni,&nj,&nk);
            }
         } else {
            c_fstluk(datak,key,&ni,&nj,&nk);
         }
         RPN_FieldUnlock();
      } else if (Grid->T0 && Grid->T1) {  /*Check for time interpolation needs*/
         /*Figure out T0 and T1 tile to use*/
         if (Grid->NbTiles>1) {
            while(&Grid->Tiles[t]!=Tile) t++;
         }
         t0=&(Grid->T0->Tiles[t]);
         t1=&(Grid->T1->Tiles[t]);

         /*Make sure the data form the needed tile is loaded*/
         if (!EZGrid_IsLoaded(t0,k)) EZGrid_TileGetData(Grid->T0,t0,k,0);
         if (!EZGrid_IsLoaded(t1,k)) EZGrid_TileGetData(Grid->T1,t1,k,0);

         /*Interpolate between by applying factors*/
         for(ni=0;ni<Tile->HNIJ;ni++) {
            datak[ni]=t0->Data[k][ni]*Grid->FT0+t1->Data[k][ni]*Grid->FT1;
         }
      }

      /*Apply Factor if needed*/
      if (Grid->Factor!=0.0) {
         for(ni=0;ni<Tile->HNIJ;ni++) {
            datak[ni]*=Grid->Factor;
         }
      }
      data[k]=datak;
      Tile->Data=data;
   }

   if (!Safe) pthread_mutex_unlock(&Tile->Mutex);

   return(Tile->Data);
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
 *   <Data>      : Buffer to burn to, for external to package untiling (NULL= use the internal one)
 *
 * Retour:
 *  <float*>     : Pointeur sur les donnees lues (ou NULL si erreur)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
float* EZGrid_TileBurn(TGrid* __restrict const Grid,TGridTile* __restrict const Tile,int K,float* __restrict Data) {

   int j,dj;

   pthread_mutex_lock(&Grid->Mutex);
   if (Tile->KBurn!=-1 && Tile->KBurn==K) {
      pthread_mutex_unlock(&Grid->Mutex);
      return(Grid->Data);
   }

   if (!Tile || !Tile->Data) {
      App_Log(ERROR,"%s: Invalid tile (%s)",__func__,Grid->H.NOMVAR);
      pthread_mutex_unlock(&Grid->Mutex);
      return(NULL);
   }

   // If a buffer array is passed, use it otherwise use the internal one
   if (!Data) {
      if (!Grid->Data) {
         if (!(Grid->Data=(float*)malloc(Grid->H.NIJ*sizeof(float)))) {
            App_Log(ERROR,"%s: Unable to allocate memory for grid data (%s)",__func__,Grid->H.NOMVAR);
            pthread_mutex_unlock(&Grid->Mutex);
            return(NULL);
         }
      }
      Data=Grid->Data;
   }
   dj=Tile->J*Grid->H.NI+Tile->I;

   for(j=0;j<Tile->NJ;j++) {
      memcpy(&Data[dj],&Tile->Data[K][(j+Tile->HDJ)*Tile->HNI+Tile->HDI],Tile->NI*sizeof(float));
      dj+=Grid->H.NI;
   }
   Tile->KBurn=K;

   pthread_mutex_unlock(&Grid->Mutex);

   return(Data);
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
 *   <Data>      : Buffer to burn to, for external to package untiling (NULL= use the internal one)
 *
 * Retour:
 *  <float*>     : Pointeur sur les donnees lues (ou NULL si erreur)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
float* EZGrid_TileBurnAll(TGrid* __restrict const Grid,int K,float* __restrict Data) {

   int t;

   if (Grid) {
      if (Grid->NbTiles>1) {
         for(t=0;t<Grid->NbTiles;t++) {
            if (!Grid->Tiles[t].Data || isnan(Grid->Tiles[t].Data[K][0])) {
               EZGrid_TileGetData(Grid,&Grid->Tiles[t],K,0);
            }
            if (!EZGrid_TileBurn(Grid,&Grid->Tiles[t],K,Data)) {
               return(NULL);
            }
         }
         return(Data?Data:Grid->Data);
      } else {
         EZGrid_TileGetData(Grid,&Grid->Tiles[0],K,0);
         return(Grid->Tiles[0].Data[K]);
      }
   } else {
      return(NULL);
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
static TGrid* EZGrid_CacheFind(TGrid *Grid) {

   register int n;

   int     type;
   float   level;
   char    format;

   if (Grid) {

      pthread_mutex_lock(&CacheMutex);
      for(n=0;n<GRIDCACHEMAX;n++) {
         if (GridCache[n]) {

            // Check for same level type and definitions
            level=ZRef_IP2Level(Grid->H.IP1,&type);
            type=type==LVL_SIGMA?LVL_ETA:type;
            if (type!=GridCache[n]->ZRef->Type) {
               continue;
            }
            if (GridCache[n]->H.NK!=Grid->H.NK || GridCache[n]->ZRef->Levels[GridCache[n]->Incr<0?Grid->H.NK-1:0]!=level) {
//            if (GridCache[n]->H.NK!=Grid->H.NK) {
               continue;
            }

            // Check for same grid
            if (Grid->H.GRTYP[0]=='#') {
               if (GridCache[n]->IP1==Grid->H.IG1 && GridCache[n]->IP2==Grid->H.IG2) {
                  pthread_mutex_unlock(&CacheMutex);
                  return(GridCache[n]);
               }
            } else if (Grid->H.GRTYP[0]=='Z' || Grid->H.GRTYP[0]=='M') {
               if (GridCache[n]->IP1==Grid->H.IG1 && GridCache[n]->IP2==Grid->H.IG2 && GridCache[n]->IP3==Grid->H.IG3) {
                  pthread_mutex_unlock(&CacheMutex);
                  return(GridCache[n]);
               }
            } else if (GridCache[n]->H.GRTYP[0]==Grid->H.GRTYP[0] && GridCache[n]->H.IG1==Grid->H.IG1 && GridCache[n]->H.IG2==Grid->H.IG2 && GridCache[n]->H.IG3==Grid->H.IG3) {
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
static inline int EZGrid_CacheIdx(const TGrid* __restrict const Grid) {

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
static inline int EZGrid_CacheAdd(TGrid* __restrict const Grid) {

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
static inline int EZGrid_CacheDel(const TGrid* __restrict const Grid) {

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
 * Nom      : <EZGrid_TileGrid>
 * Creation : Janvier 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Sauvegarder un champs en tuiles
 *
 * Parametres :
 *   <FidTo>     : Fichier dans lequel copier
 *   <NI>        : Dimension des tuiles en I (0= pas de tuiles)
 *   <NJ>        : Dimension des tuiles en J (0= pas de tuiles)
 *   <Halo>      : Tile halo size
 *   <Grid>      : Champs
 *
 * Retour:
 *  <int>        : Code de reussite (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_TileGrid(int FIdTo,int NI, int NJ,int Halo,TGrid* __restrict const Grid) {

   char   format;
   int    i,j,k,ni,nj,di,dj,pj,no,key;
   float *tile=NULL,*data;
   int    ip1=0;

   if (!Grid)
      return(FALSE);

   tile=(float*)malloc((NI+Halo*2)*(NJ+Halo*2)*sizeof(float));

   RPN_CopyDesc(FIdTo,&Grid->H);

   ip1=Grid->H.IP1;

   /*Build and save the tiles*/
   for(k=0;k<Grid->H.NK;k++) {
      no=0;
      data=EZGrid_TileBurnAll(Grid,k,NULL);

      if (Grid->H.NK>1) {
         ip1=ZRef_Level2IP(Grid->ZRef->Levels[k],Grid->ZRef->Type,DEFAULT);
      }

      /*Check if dimensions allow tiling*/
      if (!NI || !NJ || Grid->H.NI==1 || Grid->H.NJ==1 || (Grid->H.NI<NI && Grid->H.NJ<NJ)) {
         key=cs_fstecr(data,-Grid->H.NBITS,FIdTo,Grid->H.DATEO,Grid->H.DEET,Grid->H.NPAS,Grid->H.NI,Grid->H.NJ,1,ip1,Grid->H.IP2,
            Grid->H.IP3,Grid->H.TYPVAR,Grid->H.NOMVAR,Grid->H.ETIKET,Grid->H.GRTYP,Grid->H.IG1,Grid->H.IG2,Grid->H.IG3,Grid->H.IG4,Grid->H.DATYP,0);
      } else {
         /*Build and save the tiles, we adjust the tile size if it is too big*/
         for(j=0;j<Grid->H.NJ;j+=NJ) {
            nj=((j+NJ>Grid->H.NJ)?(Grid->H.NJ-j):NJ)+Halo*2;
            dj=j-Halo;

            if (dj<0)             { dj+=Halo; nj-=Halo; }
            if (dj+nj>Grid->H.NJ) { nj-=Halo; }

            for(i=0;i<Grid->H.NI;i+=NI) {
               no++;
               ni=((i+NI>Grid->H.NI)?(Grid->H.NI-i):NI)+Halo*2;
               di=i-Halo;

               if (di<0)             { di+=Halo; ni-=Halo; }
               if (di+ni>Grid->H.NI) { ni-=Halo; }

               for(pj=0;pj<nj;pj++) {
                  memcpy(&tile[pj*ni],&data[(dj+pj)*Grid->H.NI+di],ni*sizeof(float));
               }
               key=cs_fstecr(tile,-Grid->H.NBITS,FIdTo,Grid->H.DATEO,Grid->H.DEET,Grid->H.NPAS,ni,nj,1,ip1,Grid->H.IP2,
                  no,Grid->H.TYPVAR,Grid->H.NOMVAR,Grid->H.ETIKET,"#",Grid->H.IG1,Grid->H.IG2,di+1,dj+1,Grid->H.DATYP,0);
            }
         }
      }
   }
   free(tile);

   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Write>
 * Creation : Janvier 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Sauvegarder un champs
 *
 * Parametres :
 *   <Fid>       : Fichier dans lequel copier
 *   <Grid>      : Champs
 *   <NBits>     : Packing
 *   <Overwrite> : Reecrie sur les champs existant
 *
 * Retour:
 *  <int>        : Code de reussite (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_Write(int FId,TGrid* __restrict const Grid,int NBits,int Overwrite) {

   int        k,tidx,key,ok=0;
   char       format;
   int        ip1;
   TGridTile *tile;

   if (!Grid)
      return(FALSE);

   ip1=Grid->H.IP1;

   for(k=0;k<Grid->ZRef->LevelNb;k++) {
      for (tidx=0;tidx<Grid->NbTiles;tidx++) {
         tile=&Grid->Tiles[tidx];

         if (Grid->H.NK>1) {
            ip1=ZRef_Level2IP(Grid->ZRef->Levels[k],Grid->ZRef->Type,DEFAULT);
         }
         if (Grid->NbTiles>1) {
            key=cs_fstecr(tile->Data[k],-NBits,FId,Grid->H.DATEO,Grid->H.DEET,Grid->H.NPAS,tile->HNI,tile->HNJ,1,ip1,Grid->H.IP2,
                 tile->NO,Grid->H.TYPVAR,Grid->H.NOMVAR,Grid->H.ETIKET,"#",Grid->H.IG1,Grid->H.IG2,tile->I+1,tile->J+1,Grid->H.DATYP,Overwrite);
         } else {
            key=cs_fstecr(tile->Data[k],-NBits,FId,Grid->H.DATEO,Grid->H.DEET,Grid->H.NPAS,tile->NI,tile->NJ,1,ip1,Grid->H.IP2,
                Grid->H.IP3,Grid->H.TYPVAR,Grid->H.NOMVAR,Grid->H.ETIKET,Grid->H.GRTYP,Grid->H.IG1,Grid->H.IG2,Grid->H.IG3,Grid->H.IG4,Grid->H.DATYP,Overwrite);
         }
         ok+=key;
      }
   }
   return(!key);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_UnTile>
 * Creation : Mai 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Sauvegarder un champs reconstitue de tuiles
 *
 * Parametres :
 *   <FidTo>     : Fichier dans lequel copier
 *   <FIdFrom>   : Fichier source
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
int EZGrid_UnTile(int FIdTo,int FIdFrom,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2) {

   TGrid *new;
   int    n,ni,nj,nk;
   int    idlst[RPNMAX],nid;

   /*Get the number of fields (tile number=1)*/
   cs_fstinl(FIdFrom,&ni,&nj,&nk,DateV,Etiket,IP1,IP2,1,TypVar,Var,idlst,&nid,RPNMAX);

   if (nid<=0) {
      App_Log(ERROR,"%s: Specified fields do not exist",__func__);
      return(FALSE);
   }

   /*Loop on all found fields*/
   for(n=0;n<nid;n++) {
      new=EZGrid_ReadIdx(FIdFrom,idlst[n],0);
      EZGrid_TileBurnAll(new,0,NULL);
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
 *    - On lit toutes les descriptions de tuiles et on en deduit le nombre en I et J et les halos
 *    - On lit tous les niveau disponibles
 *    - La procedure fonctionne aussi pour les champs non tuile. Dans ce cas on a une seule tuile.
 *----------------------------------------------------------------------------
*/
TQTree* EZGrid_BuildIndex(TGrid* __restrict const Grid) {

   unsigned int  n,nt,res=0;
   unsigned int *idx;
   double        lat0,lon0,lat1,lon1;
   Vect2d        tr[3];
   
   // Check data limits   
   lat0=lon0=1e10;
   lat1=lon1=-1e10;
   
   for(n=0;n<Grid->GRef->NX;n++) {
      lat0=FMIN(lat0,Grid->GRef->Lat[n]);
      lon0=FMIN(lon0,Grid->GRef->Lon[n]);
      lat1=FMAX(lat1,Grid->GRef->Lat[n]);
      lon1=FMAX(lon1,Grid->GRef->Lon[n]);
   }
      
   // Create the tree on the data limits
   if (!(Grid->QTree=QTree_New(lat0,lon0,lat1,lon1,NULL))) {
      App_Log(ERROR,"%s: failed to create QTree index\n",__func__);
      return(NULL);
   }

   // Loop on triangles
   idx=Grid->GRef->Idx;
   for(nt=0;nt<Grid->GRef->NIdx-3;nt+=3) {     
      
      tr[0][0]=Grid->GRef->Lat[idx[nt]];     tr[0][1]=Grid->GRef->Lon[idx[nt]];
      tr[1][0]=Grid->GRef->Lat[idx[nt+1]];   tr[1][1]=Grid->GRef->Lon[idx[nt+1]];
      tr[2][0]=Grid->GRef->Lat[idx[nt+2]];   tr[2][1]=Grid->GRef->Lon[idx[nt+2]];
      
      // Put it in the quadtree, in any child nodes intersected
      res=8;
      if (!QTree_AddTriangle(Grid->QTree,tr,res,&Grid->GRef->Idx[nt])) {
         App_Log(ERROR,"%s: failed to add node\n",__func__);
         return(NULL);
      }      
   }
   
   return(Grid->QTree);
}

TGrid* EZGrid_Get(TGrid* __restrict const Grid) {

   TRPNHeader h;
   TGridTile *tile;
   int        n,i,j,k,nt,ni,nj,nk;
   int        ip3,key;
   int        l,idlst[RPNMAX];
   
   /*Check for master grid descriptor*/
   if (Grid->H.GRTYP[0]=='#') {
      key=cs_fstinf(Grid->H.FID,&ni,&h.NJ,&h.NK,-1,"",Grid->H.IG1,Grid->H.IG2,-1,"",">>");
      key=cs_fstinf(Grid->H.FID,&h.NI,&nj,&h.NK,-1,"",Grid->H.IG1,Grid->H.IG2,-1,"","^^");
      if (key<0) {
         App_Log(WARNING,"%s: Could not find master grid descriptor (>>,^^)\n",__func__);
         RPN_FieldUnlock();
         return(NULL);
      }
   }

   /*Get vertical reference*/
   Grid->ZRef=EZGrid_GetZRef(Grid);

   /*Get the number of tiles*/
   cs_fstinl(Grid->H.FID,&h.NI,&h.NJ,&h.NK,Grid->H.DATEV,Grid->H.ETIKET,Grid->H.IP1,Grid->H.IP2,-1,Grid->H.TYPVAR,Grid->H.NOMVAR,idlst,&Grid->NbTiles,RPNMAX);
   Grid->Tiles=(TGridTile*)malloc(Grid->NbTiles*sizeof(TGridTile));
   Grid->Data=NULL;
   Grid->QTree=NULL;
   Grid->GRef=NULL;
   Grid->Halo=0;
   Grid->NTI=Grid->NTJ=0;
   Grid->IP1=Grid->H.IG1;
   Grid->IP2=Grid->H.IG2;
   Grid->IP3=Grid->H.IG3;
   Grid->H.NIJ=Grid->H.NI*Grid->H.NJ;
   h.GRTYP[1]='\0';
   i=j=-1;
   ni=0;
   nt=Grid->NbTiles;

   /*Parse the tiles to get the tile limits and structure*/
   for(n=0;n<Grid->NbTiles;n++) {
      key=cs_fstprm(idlst[n],&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,
            &h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
            h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,
            &h.UBC,&h.EX1,&h.EX2,&h.EX3);
      if (key<0) {
         App_Log(ERROR,"%s: Missing subgrid number %i",__func__,n);
         RPN_FieldUnlock();
         return(NULL);
      }

      tile=&Grid->Tiles[Grid->H.GRTYP[0]=='#'?h.IP3-1:0];

      /*Create tile grid*/
      tile->NO    = h.IP3;
      tile->HNI   = tile->NI  = h.NI;
      tile->HNJ   = tile->NJ  = h.NJ;
      tile->HNIJ  = tile->NIJ = h.NI*h.NJ;
      tile->HDI   = 0;
      tile->HDJ   = 0;
      tile->Data  = NULL;
      tile->KBurn = -1;
      tile->Side  = GRID_CENTER;
      pthread_mutex_init(&tile->Mutex,NULL);

      /*c_ezqkdef uses fstd functions to get grid def so we need to keep the RPNFieldMutex on*/
      tile->GID =RPN_IntIdNew(h.NI,h.NJ,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,Grid->H.FID);

      /*Check for tiled data or not*/
      if (Grid->H.GRTYP[0]=='#') {
         tile->HI=tile->I=h.IG3-1;
         tile->HJ=tile->J=h.IG4-1;

         /*Set the border tile flags and count the number of tiles in I,J*/
         if (h.IG3==1)                { tile->Side|=GRID_LEFT;   Grid->NTJ++; }
         if (h.IG3+h.NI+2>Grid->H.NI) { tile->Side|=GRID_RIGHT; }
         if (h.IG4==1)                { tile->Side|=GRID_BOTTOM; Grid->NTI++; ni+=h.NI; }
         if (h.IG4+h.NJ+2>Grid->H.NJ) { tile->Side|=GRID_TOP; }
      } else {
         tile->HI=tile->I=0;
         tile->HJ=tile->J=0;
         tile->Side|=GRID_LEFT|GRID_RIGHT|GRID_BOTTOM|GRID_TOP;
         Grid->NTI=Grid->NTJ=Grid->NbTiles=1;
         break;
      }
   }

   /*Is there a halo around the tiles*/
   if (ni>Grid->H.NI) {
      /*Calculate halo width*/
      Grid->Halo=ceil((ni-Grid->H.NI)/Grid->NTI/2.0);

      /*Adjust dimensions with halo, we keep coordinates reference without halo*/
      for(n=0;n<Grid->NbTiles;n++) {
         tile=&Grid->Tiles[n];

         tile->HDI = (tile->Side&GRID_LEFT?0:Grid->Halo);
         tile->HDJ = (tile->Side&GRID_BOTTOM?0:Grid->Halo);
         tile->I  += tile->HDI;
         tile->J  += tile->HDJ;
         tile->NI -= tile->HDI+(tile->Side&GRID_RIGHT?0:Grid->Halo);
         tile->NJ -= tile->HDJ+(tile->Side&GRID_TOP?0:Grid->Halo);
         tile->NIJ = tile->NI*tile->NJ;
      }
   }

   /*Create master grid*/
   if (Grid->H.GRTYP[0]=='#') {
      h.GRTYP[0]='Z';
      h.IG3=h.IG4=0;
   } else {
      h.GRTYP[0]=Grid->H.GRTYP[0];
   }

   if (Grid->H.GRTYP[0]=='M') {
      Grid->GRef=GeoRef_New();
  
      cs_fstinf(Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->IP1,Grid->IP2,Grid->IP3,"","##");
      
      Grid->GRef->NIdx=ni*nj*nk;
      Grid->GRef->NX=Grid->H.NI;
      Grid->GRef->NY=Grid->H.NJ;

      Grid->GRef->Idx=(unsigned int*)malloc(Grid->GRef->NIdx*sizeof(unsigned int));
      Grid->GRef->Lat=(float*)malloc(Grid->GRef->NX*sizeof(float));
      Grid->GRef->Lon=(float*)malloc(Grid->GRef->NX*sizeof(float));

      cs_fstlir(Grid->GRef->Lat,Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->IP1,Grid->IP2,Grid->IP3,"","^^");
      cs_fstlir(Grid->GRef->Lon,Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->IP1,Grid->IP2,Grid->IP3,"",">>");
      cs_fstlir(Grid->GRef->Idx,Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->IP1,Grid->IP2,Grid->IP3,"","##");
      
      EZGrid_BuildIndex(Grid);
   } else {
      /*c_ezqkdef uses fstd functions to get griddef so we need to keep the RPNFieldMutex on*/
      Grid->GID =RPN_IntIdNew(Grid->H.NI,Grid->H.NJ,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,Grid->H.FID);
      Grid->Wrap=EZGrid_Wrap(Grid);
   }

   return(Grid);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_BoundaryCopy>
 * Creation : Fevrierr 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Etendre les valeurs internes vers la bordure d'un champs
 *
 * Parametres :
 *   <Grid>   : Grille
 *   <Width>  : Largeur de la bordure (1 ou 2)
 *
 * Retour:
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int EZGrid_BoundaryCopy(TGrid* __restrict const Grid,int Width) {

   TGridTile    *tile;
   unsigned long idx,i,j,n,k;

   if (Width!=1 && Width!=2)
      return(FALSE);

   for(n=0;n<Grid->NbTiles;n++) {
      tile=&Grid->Tiles[n];

      for(k=0;k<Grid->ZRef->LevelNb;k++) {
         if (tile->Side&GRID_BOTTOM) {
            for(i=0,idx=0;i<tile->HNI;i++,idx++) {
               if (Width==2)
                  tile->Data[k][idx+tile->HNI]=tile->Data[k][idx+tile->HNI+tile->HNI];
               tile->Data[k][idx]=tile->Data[k][idx+tile->HNI];
            }
         }
         if (tile->Side&GRID_TOP) {
            for(i=0,idx=tile->HNIJ-tile->HNI-1;i<tile->HNI;i++,idx++) {
               if (Width==2)
                  tile->Data[k][idx-tile->HNI]=tile->Data[k][idx-tile->HNI-tile->HNI];
               tile->Data[k][idx]=tile->Data[k][idx-tile->HNI];
            }
         }
         if (tile->Side&GRID_LEFT) {
            for(j=0,idx=0;j<tile->HNJ;j++,idx+=tile->HNI) {
               if (Width==2)
                  tile->Data[k][idx+1]=tile->Data[k][idx+2];
               tile->Data[k][idx]=tile->Data[k][idx+1];
            }
         }
         if (tile->Side&GRID_RIGHT) {
            for(j=0,idx=tile->HNI-1;j<tile->HNJ;j++,idx+=tile->HNI) {
               if (Width==2)
                  tile->Data[k][idx-1]=tile->Data[k][idx-2];
               tile->Data[k][idx]=tile->Data[k][idx-1];
            }
         }
      }
   }
   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetZRef>
 * Creation : Octobre 2009 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Lire la definition de la coordonnee verticale dans le fichier
 *
 * Parametres :
 *   <Grid>   : Grille
 *
 * Retour:
 *  <ZRef>    : Referential vertical
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
TZRef* EZGrid_GetZRef(const TGrid* __restrict const Grid) {

   TZRef *zref;

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

   if (!(zref=ZRef_New())) {
      return(NULL);
   }

   // Get levels
   ZRef_GetLevels(zref,&Grid->H,Grid->Incr);

   // Decode vertical coordinate parameters
   ZRef_DecodeRPN(zref,Grid->H.FID);
   
   // Force sigma to eta
   zref->Type=zref->Type==LVL_SIGMA?LVL_ETA:zref->Type;

   return(zref);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_New>
 * Creation : Novembre 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Allouer une nouvelle grille
 *
 * Parametres :
 *
 * Retour:
 *  <TGrid>   : Nouvelle Grille (NULL=Error)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
TGrid *EZGrid_New(void) {

   TGrid *new=NULL;
   int    n;

   if ((new=(TGrid*)malloc(sizeof(TGrid)))) {
      pthread_mutex_init(&new->Mutex,NULL);
      new->Master=0;
      new->Data=NULL;
      new->T0=new->T1=NULL;
      new->FT0=new->FT1=0.0f;
      new->Factor=0.0f;
      new->Incr=0;
      new->GID=-1;
      new->ZRef=NULL;
      new->GRef=NULL;
      new->QTree=NULL;
      new->Wrap=0;
      new->Halo=0;
      new->H.NI=0;
      new->H.NJ=0;
      new->H.NK=0;
      new->H.NIJ=0;
      new->IP1=new->IP2=new->IP3=0;
      new->NTI=new->NTJ=0;
      new->NbTiles=0;
      new->Tiles=NULL;
      new->H.FID=-1;
   }
   return(new);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Copy>
 * Creation : Fevrier 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Copier une grille
 *
 * Parametres :
 *  <Master>  : Copy from master (NULL=new empty)
 *  <Level>   : (Level to copy or -1 for all)
 *
 * Retour:
 *  <TGrid>   : Nouvelle Grille (NULL=Error)
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
TGrid *EZGrid_Copy(TGrid *Master,int Level) {

   TGrid *new=NULL;
   int    n;

   if (Master && (new=(TGrid*)malloc(sizeof(TGrid)))) {
      pthread_mutex_init(&new->Mutex,NULL);
      new->Master=0;
      new->Data=NULL;
      new->T0=new->T1=NULL;
      new->FT0=new->FT1=0.0f;
      new->Factor=0.0f;
      new->Incr=0;

      new->GID=Master->GID;
      new->Wrap=Master->Wrap;
      new->Halo=Master->Halo;
      memcpy(&new->H,&Master->H,sizeof(TRPNHeader));
      new->H.FID=-1;

      new->IP1=Master->IP1;
      new->IP2=Master->IP2;
      new->IP3=Master->IP3;
      new->NTI=Master->NTI;
      new->NTJ=Master->NTJ;
      new->NbTiles=Master->NbTiles;
      new->Tiles=(TGridTile*)malloc(Master->NbTiles*sizeof(TGridTile));
      memcpy(new->Tiles,Master->Tiles,Master->NbTiles*sizeof(TGridTile));
      for(n=0;n<Master->NbTiles;n++) {
         memcpy(&new->Tiles[n],&Master->Tiles[n],sizeof(TGridTile));
         new->Tiles[n].Data=NULL;
         new->Tiles[n].KBurn=-1;
         pthread_mutex_init(&new->Tiles[n].Mutex,NULL);
      }
      new->ZRef=ZRef_Copy(Master->ZRef);
      new->GRef=GeoRef_Copy(Master->GRef);
      new->QTree=Master->QTree;
      new->H.NK=new->ZRef->LevelNb;

      // Force memory allocation of all tiles
      EZGrid_LoadAll(new);
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
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/

wordint f77name(ezgrid_free)(wordint *gdid) {
   EZGrid_Free(GridCache[*gdid]);
   return(TRUE);
}

void EZGrid_Free(TGrid* __restrict const Grid) {

   int n,k;

   if (Grid) {
      /*Cleanup tile data*/
      for(n=0;n<Grid->NbTiles;n++) {
         if (Grid->Tiles[n].Data) {
            for(k=0;k<Grid->H.NK;k++) {
               if (Grid->Tiles[n].Data[k]) {
                  free(Grid->Tiles[n].Data[k]);
                  Grid->Tiles[n].Data[k]=NULL;
               }
            }
            pthread_mutex_destroy(&Grid->Tiles[n].Mutex);
            free(Grid->Tiles[n].Data);
            Grid->Tiles[n].Data=NULL;
         }
      }
      if (Grid->Data) {
         free(Grid->Data);
         Grid->Data=NULL;
      }

      pthread_mutex_destroy(&Grid->Mutex);

      /*If this is a master, keep in memory*/
      if (!Grid->Master) {
         if (Grid->Tiles) {
            free(Grid->Tiles);
            Grid->Tiles=NULL;
         }
   //      ZRef_Free(Grid->ZRef);
         EZGrid_CacheDel(Grid);
      }
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Clear>
 * Creation : Avril 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Marquer la memoire comme non-innitialisee en assignant NAN
 *            au premier index
 *
 * Parametres :
 *   <Grid>      : Grille
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void EZGrid_Clear(TGrid* __restrict const Grid) {

   int n,k;
   float f=nanf("NaN");

   /*Cleanup tile data*/
   for(n=0;n<Grid->NbTiles;n++) {
      if (Grid->Tiles[n].Data) {
         for(k=0;k<Grid->H.NK;k++) {
            if (Grid->Tiles[n].Data[k])
               Grid->Tiles[n].Data[k][0]=f;
         }
      }
      Grid->Tiles[n].KBurn=-1;
   }
   Grid->T0=Grid->T1=0;
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
 *   <Incr>      : Ordre de tri des niveaux (IP1) (-1=decroissant, 0=1 seul niveau, 1=croissant)
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
   int        idlst[RPNMAX];

   new=EZGrid_New();
   new->H.FID=FId;
   new->Incr=Incr;

   strcpy(new->H.NOMVAR,"    ");
   strcpy(new->H.TYPVAR,"  ");
   strcpy(new->H.ETIKET,"            ");
   strcpy(new->H.GRTYP," ");

   RPN_FieldLock();
   Key=c_fstprm(Key,&new->H.DATEO,&new->H.DEET,&new->H.NPAS,&new->H.NI,&new->H.NJ,&new->H.NK,&new->H.NBITS,
         &new->H.DATYP,&new->H.IP1,&new->H.IP2,&new->H.IP3,new->H.TYPVAR,new->H.NOMVAR,new->H.ETIKET,
         new->H.GRTYP,&new->H.IG1,&new->H.IG2,&new->H.IG3,&new->H.IG4,&new->H.SWA,&new->H.LNG,&new->H.DLTF,
         &new->H.UBC,&new->H.EX1,&new->H.EX2,&new->H.EX3);

   if (new->H.DATEO==0 && new->H.NPAS==0 && new->H.DEET==0) {
      new->H.DATEV=0;
   } else {
      nh=(new->H.NPAS*new->H.DEET)/3600.0;
      f77name(incdatr)(&new->H.DATEV,&new->H.DATEO,&nh);
   }

   /*Get the number of levels (if Incr!=0)*/
   /*In case of # grid, set IP3 to 1 to get NK just for the first tile*/
   if (Incr) {
      ip3=new->H.GRTYP[0]=='#'?1:-1;
      c_fstinl(new->H.FID,&h.NI,&h.NJ,&h.NK,new->H.DATEV,new->H.ETIKET,-1,new->H.IP2,ip3,new->H.TYPVAR,new->H.NOMVAR,idlst,&new->H.NK,RPNMAX);
   }
   RPN_FieldUnlock();

   // Check previous master grid existence
   // mst is no in a critical sectin but its veryyyyyyyyyy unlikelyyyyyy that it will cause problems
   if ((mst=EZGrid_CacheFind(new))) {
      new->GID=mst->GID;
      new->ZRef=mst->ZRef;
      new->GRef=mst->GRef;
      new->QTree=mst->QTree;
      new->Wrap=mst->Wrap;
      new->Halo=mst->Halo;
      new->H.NI=mst->H.NI;
      new->H.NJ=mst->H.NJ;
      new->H.NK=mst->H.NK;
      new->H.NIJ=mst->H.NIJ;
//      new->IP1=mst->IP1;
//      new->IP2=mst->IP2;
//      new->IP3=mst->IP3;
      new->NTI=mst->NTI;
      new->NTJ=mst->NTJ;
      new->NbTiles=mst->NbTiles;
      new->Tiles=(TGridTile*)malloc(mst->NbTiles*sizeof(TGridTile));
      memcpy(new->Tiles,mst->Tiles,mst->NbTiles*sizeof(TGridTile));
      for(n=0;n<mst->NbTiles;n++) {
         new->Tiles[n].Data=NULL;
         new->Tiles[n].KBurn=-1;
         pthread_mutex_init(&new->Tiles[n].Mutex,NULL);
      }
   } else {
      if (!EZGrid_Get(new)) {
         App_Log(ERROR,"%s: Could not create master grid (%s)",__func__,new->H.NOMVAR);
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
int EZGrid_Load(const TGrid* __restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1) {

   TGridTile *tile;
   int        i,j,k;

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

   /*Check inclusion in master grid limits*/
   if (I0<0 || J0<0 || K0<0 || I0>=Grid->H.NI || J0>=Grid->H.NJ || K0>=Grid->H.NK ||
       I1<0 || J1<0 || K1<0 || I1>=Grid->H.NI || J1>=Grid->H.NJ || K1>=Grid->H.NK) {
      App_Log(WARNING,"%s: Coordinates out of range (%s): I(%i,%i) J(%i,%i) K(%i,%i)\n",__func__,Grid->H.NOMVAR,I0,I1,J0,J1,K0,K1);
      return(FALSE);
   }

   /*Loop on coverage*/
   for(k=K0;k<=K1;k++) {
      for(j=J0;j<=J1;j++) {
         for(i=I0;i<=I1;i++) {
            /*Figure out the right tile*/
            if (!(tile=EZGrid_TileGet(Grid,i,j))) {
               App_Log(WARNING,"%s: Tile not found (%s) I(%i) J(%i)\n",__func__,Grid->H.NOMVAR,i,j);
               return(FALSE);
            }

            /*Check for tile data*/
            if (!EZGrid_TileGetData(Grid,tile,k,0)) {
               return(FALSE);
            }
         }
      }
   }
   return(TRUE);
}

wordint f77name(ezgrid_loadall)(wordint *gdid) {
   return(EZGrid_LoadAll(GridCache[*gdid]));
}
int EZGrid_LoadAll(const TGrid* __restrict const Grid) {

   int k=0,idx;

   if (Grid) {
      /*Loop on coverage*/
      for(k=0;k<Grid->H.NK;k++) {
         idx=0;
         while(idx<Grid->NTI*Grid->NTJ) {

            /*Check for tile data*/
            if (!EZGrid_TileGetData(Grid,&Grid->Tiles[idx],k,0)) {
               App_Log(ERROR,"%s: Unable to get tile data (%s) at level %i",__func__,Grid->H.NOMVAR,k);
               return(FALSE);
            }
            idx++;
         }
      }
   }
   return(k);
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

TGrid *EZGrid_InterpTime(const TGrid* __restrict const Grid0,const TGrid* __restrict const Grid1,int Date) {

   TGrid *new;
   double delay,dt;
   int    i,n,k;

   /*Figure out the delai between the 2 fields*/
   f77name(difdatr)((int*)&Grid0->H.DATEV,(int*)&Grid1->H.DATEV,&delay);
   f77name(difdatr)((int*)&Grid0->H.DATEV,&Date,&dt);

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
   return(TRUE);
}

void EZGrid_Factor(TGrid* __restrict Grid,const float Factor) {

   Grid->Factor=Factor;
}

wordint f77name(ezgrid_interpfactor)(wordint *gdid0,wordint *gdid1,ftnfloat *f0,ftnfloat *f1) {
   return(EZGrid_CacheIdx(EZGrid_InterpFactor(NULL,GridCache[*gdid0],GridCache[*gdid1],*f0,*f1)));
}

TGrid *EZGrid_InterpFactor(TGrid* __restrict const Grid,TGrid* __restrict const Grid0,TGrid* __restrict const Grid1,float Factor0,float Factor1) {

   TGrid *new;
   int    i;

   /*Allocate new tile*/
   if (Grid) {
      new=Grid;
   } else {
      new=EZGrid_New();
      memcpy(new,Grid0,sizeof(TGrid));

      new->ZRef=Grid0->ZRef;
      new->Data=NULL;
      new->Master=0;
      new->H.FID=-1;
      new->Tiles=(TGridTile*)malloc(Grid0->NbTiles*sizeof(TGridTile));
      memcpy(new->Tiles,Grid0->Tiles,Grid0->NbTiles*sizeof(TGridTile));
      for(i=0;i<Grid0->NbTiles;i++) {
         new->Tiles[i].Data=NULL;
      }
      EZGrid_CacheAdd(new);
   }

   new->T0=Grid0;
   new->T1=Grid1;
   new->FT0=Factor0;
   new->FT1=Factor1;

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
int EZGrid_GetLevelNb(const TGrid* __restrict const Grid) {
   return(Grid->ZRef->LevelNb);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetLevelType>
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
wordint f77name(ezgrid_getleveltype)(wordint *gdid) {
   return(EZGrid_GetLevelType(GridCache[*gdid]));
}
int EZGrid_GetLevelType(const TGrid* __restrict const Grid) {
   return(Grid->ZRef->Type);
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
int EZGrid_GetLevels(const TGrid* __restrict const Grid,float* __restrict Levels,int* __restrict Type) {

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

   memcpy(Levels,Grid->ZRef->Levels,Grid->ZRef->LevelNb*sizeof(float));
   *Type=Grid->ZRef->Type;

   return(Grid->ZRef->LevelNb);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetLevel>
 * Creation : Octobre 2009 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Calculer la pression a un niveaux specifique
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Pressure>   : Pression du niveau
 *   <P0>         : Pression au sol
 *
 * Retour:
 *   <Level>      : Niveau du modele
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
ftnfloat f77name(ezgrid_getlevel)(wordint *gdid,ftnfloat *pressure,ftnfloat *p0) {
   return(EZGrid_GetLevel(GridCache[*gdid],*pressure,*p0));
}
float EZGrid_GetLevel(const TGrid* __restrict const Grid,float Pressure,float P0) {

   double  level=-1.0;
   float  *pres;
   int     z;
   TZRef  *zref;

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(level);
   }

   return(ZRef_Pressure2Level(Grid->ZRef,P0,Pressure));
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetPressure>
 * Creation : Octobre 2009 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Calculer la pression a un niveaux specifique
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Level>      : Liste des niveaux
 *
 * Retour:
 *   <Pressure>  : Pression
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
ftnfloat f77name(ezgrid_getpressure)(wordint *gdid,ftnfloat *level,ftnfloat *p0) {
   return(EZGrid_GetPressure(GridCache[*gdid],*level,*p0));
}
float EZGrid_GetPressure(const TGrid* __restrict const Grid,float Level,float P0) {

   double  pres=-1.0;
   TZRef  *zref;

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(pres);

   }

   return(ZRef_Level2Pressure(Grid->ZRef,P0,Level));
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
int EZGrid_LLGetValue(TGrid* __restrict const Grid,float Lat,float Lon,int K0,int K1,float* __restrict Value) {

   TGridTile *t;
   TQTree *node;
   Vect3d  bary;
   float i,j;
   int   v,n,m;
   unsigned int *idx;
   
   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

   if (Grid->H.GRTYP[0]=='M') {
      // This is a triangle mesh
      idx=Grid->GRef->Idx;
      t=&Grid->Tiles[0];
      EZGrid_TileGetData(Grid,t,K0,0);
      CLAMPLON(Lon);
      
      // Find enclosing triangle
      if ((node=QTree_Find(Grid->QTree,Lat,Lon)) && node->NbData) {
         
         // Loop on this nodes data payload
         for(n=0;n<node->NbData;n++) {
            idx=(unsigned int*)QTree_GetData(node,n);
            
            // if the Barycentric coordinates are within this triangle, get its interpolated value
            if (Bary_Get(bary,Lat,Lon,Grid->GRef->Lat[*idx],Grid->GRef->Lon[*idx],Grid->GRef->Lat[*(idx+1)],Grid->GRef->Lon[*(idx+1)],Grid->GRef->Lat[*(idx+2)],Grid->GRef->Lon[*(idx+2)])) {
               *Value=Bary_Interp(bary,t->Data[K0][*idx],t->Data[K0][*(idx+1)],t->Data[K0][*(idx+2)]);                    
               return(TRUE);
            }
         }
      }
      // We must be out of the tin
      return(FALSE);
   } else {
      // This is a regular RPN grid
   //   RPN_IntLock();
      c_gdxyfll(Grid->GID,&i,&j,&Lat,&Lon,1);
   //   RPN_IntUnlock();

      return(EZGrid_IJGetValue(Grid,i-1.0f,j-1.0f,K0,K1,Value));
   }
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
int EZGrid_LLGetUVValue(TGrid* __restrict const GridU,TGrid* __restrict const GridV,float Lat,float Lon,int K0,int K1,float* __restrict UU,float* __restrict VV) {

   float i,j;

   if (!GridU || !GridV) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

//   RPN_IntLock();
   c_gdxyfll(GridU->GID,&i,&j,&Lat,&Lon,1);
//   RPN_IntUnlock();

   return(EZGrid_IJGetUVValue(GridU,GridV,i-1.0f,j-1.0f,K0,K1,UU,VV));
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
int EZGrid_IJGetValue(TGrid* __restrict const Grid,float I,float J,int K0,int K1,float* __restrict Value) {

   TGridTile *t,*tw=NULL;
   int        i,j,k,ik=0,idx,idxj,idxw,idxwj,wrap=0;
   float      dx,dy,d[4];

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

   /*Check inclusion in master grid limits*/
   if (I<0 || J<0 || K0<0 || K1<0 || J>=Grid->H.NJ || K0>=Grid->H.NK || K1>=Grid->H.NK) {
      App_Log(ERROR,"%s: Coordinates out of range (%s): I(%f) J(%f) K(%i,%i)",__func__,Grid->H.NOMVAR,I,J,K0,K1);
      return(FALSE);
   }

   if (I>=Grid->H.NI-1) {
      if (Grid->Wrap) {
         wrap=1;
      } else {
         App_Log(ERROR,"%s: Coordinates out of range (%s): I(%f) J(%f) K(%i,%i)",__func__,Grid->H.NOMVAR,I,J,K0,K1);
         return(FALSE);
      }
   }

   if (!(t=EZGrid_TileGet(Grid,I,J))) {
      App_Log(ERROR,"%s: Tile not found (%s) I(%.f) J(%f)",__func__,Grid->H.NOMVAR,I,J);
      return(FALSE);
   }

   // Calculate in-tile indexes and limits
   i=I;
   j=J;
   dx=I-i;
   dy=J-j;
   i=i-t->HI;
   j=j-t->HJ;
   //   I+=1.0f;J+=1.0f;

   idx =j*t->HNI+i;
   idxj=j<t->HNJ-1?idx+t->HNI:idx;

   // Get the wrapping tile
   if (wrap) {
      if (!(tw=EZGrid_TileGet(Grid,0,J))) {
         App_Log(ERROR,"%s: Wrapped tile not found (%s) I(%.f) J(%f)",__func__,Grid->H.NOMVAR,I,J);
         return(FALSE);
      }
      // If we wrap, index of first point within wrapped tile
      idxw=j*tw->HNI;
      idxwj=idxw+(idx==idxj?0:tw->HNI);
   }

   k=K0;
   do {
      if (!EZGrid_IsLoaded(t,k)) {
         EZGrid_TileGetData(Grid,t,k,0);
      }

      d[0]=t->Data[k][idx];
      d[2]=t->Data[k][idxj];

      if (tw) {
         if (!EZGrid_IsLoaded(tw,k))
            EZGrid_TileGetData(Grid,tw,k,0);

         d[1]=tw->Data[k][idxw];
         d[3]=tw->Data[k][idxwj];
      } else {
         d[1]=t->Data[k][idx+1];
         d[3]=t->Data[k][idxj+1];
      }

      Value[ik]=d[0] + (d[1]-d[0])*dx + (d[2]-d[0])*dy + (d[3]-d[1]-d[2]+d[0])*dx*dy;

      ik++;
//         RPN_IntLock();
//         c_gdxysval(t->GID,&Value[ik++],t->Data[k],&I,&J,1);
//         RPN_IntUnlock();
   } while ((K0<=K1?k++:k--)!=K1);

   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Interp>
 * Creation : Mai 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Interpoler un champs dans une autre grille
 *
 * Parametres :
 *   <To>     : Destination
 *   <From>   : Source
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
wordint f77name(ezgrid_interp)(wordint *to,wordint *from) {
   return(EZGrid_Interp(GridCache[*to],GridCache[*from]));
}

int EZGrid_Interp(TGrid* __restrict const To, TGrid* __restrict const From) {

   int n,ok;
   float *from,*to;

   if (!From) {
      App_Log(ERROR,"%s: Invalid input grid (%s)",__func__,From->H.NOMVAR);
      return(FALSE);
   }

   if (!To) {
      App_Log(ERROR,"%s: Invalid output grid (%s)",__func__,To->H.NOMVAR);
      return(FALSE);
   }

   if (!(from=EZGrid_TileBurnAll(From,0,NULL))) {
      App_Log(ERROR,"%s: Problems with input grid",__func__);
      return(FALSE);
   }
   if (!(to=EZGrid_TileBurnAll(To,0,NULL))) {
      App_Log(ERROR,"%s: Problems with output grid",__func__);
      return(FALSE);
   }

   RPN_IntLock();
   ok=c_ezdefset(To->GID,From->GID);
   ok=c_ezsint(to,from);
   RPN_IntUnlock();
   if (ok<0)  {
      App_Log(ERROR,"%s: Unable to do interpolation (c_ezscint (%i))",__func__,ok);
      return(FALSE);
   }

   return(TRUE);
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
int EZGrid_IJGetUVValue(TGrid* __restrict const GridU,TGrid* __restrict const GridV,float I,float J,int K0,int K1,float *UU,float* __restrict VV) {

   TGridTile *tu,*tv;
   float      d,v,i,j;
   int        ik=0,k;

   if (!GridU || !GridV) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

   /*Check inclusion in master grid limits*/
   if (I<0 || J<0 || K0<0 || K1<0 || I>=GridU->H.NI || J>=GridU->H.NJ || K0>=GridU->H.NK || K1>=GridU->H.NK) {
      App_Log(ERROR,"%s: Coordinates out of range (%s,%s): I(%f) J(%f) K(%i,%i)",__func__,GridU->H.NOMVAR,GridV->H.NOMVAR,I,J,K0,K1);
      return(FALSE);
   }

   if (!(tu=EZGrid_TileGet(GridU,I,J))) {
      App_Log(ERROR,"%s: Tile not found (%s) I(%f) J(%f)",__func__,GridU->H.NOMVAR,I,J);
      return(FALSE);
   }
   if (!(tv=EZGrid_TileGet(GridV,I,J))) {
      App_Log(ERROR,"%s: Tile not found (%s) I(%f) J(%f)",__func__,GridV->H.NOMVAR,I,J);
      return(FALSE);
   }

   /*Have to readjust coordinate within tile for ezscint*/
   I=I-tu->HI+1.0;
   J=J-tu->HJ+1.0;

   k=K0;
   do {
      if (!EZGrid_IsLoaded(tu,k))
         EZGrid_TileGetData(GridU,tu,k,0);
      if (!EZGrid_IsLoaded(tv,k))
         EZGrid_TileGetData(GridV,tv,k,0);

//      RPN_IntLock();
      c_gdxywdval(tu->GID,&UU[ik],&VV[ik],tu->Data[k],tv->Data[k],&I,&J,1);
//      RPN_IntUnlock();

      d=DEG2RAD(VV[ik]);
      v=UU[ik]*0.515f;

      UU[ik]=-v*sinf(d);
      VV[ik]=-v*cosf(d);
      ik++;
   } while ((K0<=K1?k++:k--)!=K1);

   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetValue>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeur a un point de grille
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
int EZGrid_GetValue(const TGrid* __restrict const Grid,int I,int J,int K0,int K1,float* __restrict Value) {

   TGridTile *t;
   int        k,ik=0;

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

   /*Check inclusion in master grid limits*/
   if (I<0 || J<0 || K0<0 || K1<0 || I>=Grid->H.NI || J>=Grid->H.NJ || K0>=Grid->H.NK || K1>=Grid->H.NK) {
      App_Log(ERROR,"%s: Coordinates out of range (%s) I(%i) J(%i) K(%i,%i)",__func__,Grid->H.NOMVAR,I,J,K0,K1);
      return(FALSE);
   }

   if (!(t=EZGrid_TileGet(Grid,I,J))) {
      App_Log(ERROR,"%s: Tile not found (%s) I(%i) J(%i)",__func__,Grid->H.NOMVAR,I,J);
      return(FALSE);
   }

   k=K0;
   do {
      if (!EZGrid_IsLoaded(t,k))
         EZGrid_TileGetData(Grid,t,k,0);

      Value[ik++]=EZGrid_TileValue(t,I,J,k);
   } while ((K0<=K1?k++:k--)!=K1);

   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetValues>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeurs a plusieurs points de grilles
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Nb>         : Nombre de points
 *   <I>          : Coordonnees en I
 *   <J>          : Coordonnees en J
 *   <K>          : Coordonnees en K
 *   <Value>      : Valeurs au points de grilles
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
int EZGrid_GetValues(const TGrid* __restrict const Grid,int Nb,float* __restrict const I,float* __restrict const J,float* __restrict const K,float* __restrict Value) {

   TGridTile *t;
   int        n,i,j,k;

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
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
         App_Log(ERROR,"%s: Coordinates out of range (%s): I(%i) J(%i) K(%i)",__func__,Grid->H.NOMVAR,i,j,k);
         return(FALSE);
      }

      if (!(t=EZGrid_TileGet(Grid,i,j))) {
         App_Log(ERROR,"%s: Tile not found (%s) I(%i) J(%i)",__func__,Grid->H.NOMVAR,i,j);
         return(FALSE);
      }
      if (!EZGrid_IsLoaded(t,k))
         EZGrid_TileGetData(Grid,t,k,0);

      Value[n]=EZGrid_TileValue(t,i,j,k);
   }

   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetArray>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir un pointeur sur les valeurs
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
int EZGrid_GetArray(TGrid* __restrict const Grid,int K,float* __restrict Value) {

   float *data;

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

   /*Check inclusion in master grid limits*/
   if (K<0 || K>=Grid->H.NK) {
      App_Log(ERROR,"%s: Coordinates out of range (%s): K(%i)",__func__,Grid->H.NOMVAR,K);
      return(FALSE);
   }
   data=EZGrid_TileBurnAll(Grid,K,NULL);
   memcpy(Value,data,Grid->H.NIJ*sizeof(float));

   return(TRUE);
}

float* EZGrid_GetArrayPtr(TGrid* __restrict const Grid,int K) {

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

   /*Check inclusion in master grid limits*/
   if (K<0 || K>=Grid->H.NK) {
      App_Log(ERROR,"%s: Coordinates out of range (%s): K(%i)",__func__,Grid->H.NOMVAR,K);
      return(FALSE);
   }
   return(EZGrid_TileBurnAll(Grid,K,NULL));
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
int EZGrid_GetRange(const TGrid* __restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1,float* __restrict Value) {

   TGridTile *t;
   int        ik=0;
   int        i,j,k;

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

   /*Check inclusion in master grid limits*/
   if (I0<0 || J0<0 || K0<0 || I0>=Grid->H.NI || J0>=Grid->H.NJ || K0>=Grid->H.NK ||
       I1<0 || J1<0 || K1<0 || I1>=Grid->H.NI || J1>=Grid->H.NJ || K1>=Grid->H.NK) {
      App_Log(ERROR,"%s: Coordinates out of range (%s): I(%i,%i) J(%i,%i) K(%i,%i)",__func__,Grid->H.NOMVAR,I0,I1,J0,J1,K0,K1);
      return(FALSE);
   }

   /*Loop on coverage*/
   for(k=K0;k<=K1;k++) {
      for(j=J0;j<=J1;j++) {
         for(i=I0;i<=I1;i++) {
            if (!(t=EZGrid_TileGet(Grid,i,j))) {
               App_Log(ERROR,"%s: Tile not found (%s) I(%i) J(%i)",__func__,Grid->H.NOMVAR,i,j);
               return(FALSE);
            }
            if (!EZGrid_IsLoaded(t,k))
               EZGrid_TileGetData(Grid,t,k,0);

            Value[ik++]=EZGrid_TileValue(t,i,j,k);
         }
      }
   }
   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetDelta>
 * Creation : Avril 2010 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeurs de distance en X et Y ainsi que l'aire
 *            pour chaque cellule de la grille
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Invert>     : Invert (1/area)
 *   <DX>         : Valeurs de distance en X
 *   <DY>         : Valeurs de distance en X
 *   <DA>         : Valeurs de l'aire
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *    - Si un des tableau est NULL, il ne sera pas remplie
 *----------------------------------------------------------------------------
*/
int f77name(ezgrid_getdelta)(wordint *gdid,wordint *k,ftnfloat *dx,ftnfloat *dy,ftnfloat *da) {
   return(EZGrid_GetDelta(GridCache[*gdid],*k-1,dx,dy,da));
}

int EZGrid_GetDelta(TGrid* __restrict const Grid,int Invert,float* DX,float* DY,float* DA) {

   unsigned int i,gi,j,gj,idx;
   float        di[4],dj[4],dlat[4],dlon[4];
   double       fx,fy,dx[4],dy[4];

   if (!Grid) {
      App_Log(ERROR,"%s: Invalid grid",__func__);
      return(FALSE);
   }

//   RPN_IntLock();

   for(j=0,gj=1;j<Grid->H.NJ;j++,gj++) {
      idx=j*Grid->H.NI;
      for(i=0,gi=1;i<Grid->H.NI;i++,idx++,gi++) {
         di[0]=gi-0.5; dj[0]=gj;
         di[1]=gi+0.5; dj[1]=gj;
         di[2]=gi;     dj[2]=gj-0.5;
         di[3]=gi;     dj[3]=gj+0.5;

         // Reproject gridpoint length coordinates os segments crossing center of cell
         c_gdllfxy(Grid->GID,dlat,dlon,di,dj,4);
         dx[0]=DEG2RAD(dlon[0]); dy[0]=DEG2RAD(dlat[0]);
         dx[1]=DEG2RAD(dlon[1]); dy[1]=DEG2RAD(dlat[1]);

         dx[2]=DEG2RAD(dlon[2]); dy[2]=DEG2RAD(dlat[2]);
         dx[3]=DEG2RAD(dlon[3]); dy[3]=DEG2RAD(dlat[3]);

         // Get distance in meters
         fx=DIST(0.0,dy[0],dx[0],dy[1],dx[1]);
         fy=DIST(0.0,dy[2],dx[2],dy[3],dx[3]);

         // If x distance is null, we crossed the pole
         if (fx==0.0)
            fx=(M_PI*fy)/Grid->H.NI;

         if (DX) DX[idx]=(Invert?1.0/fx:fx);
         if (DY) DY[idx]=(Invert?1.0/fy:fy);
         if (DA) DA[idx]=(Invert?1.0/(fx*fy):(fx*fy));
      }
   }
//   RPN_IntUnlock();

   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetLL>
 * Creation : Avril 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les latlon a partir des I J
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Lat>        : Latitudes
 *   <Lon>        : Longitudes
 *   <I>          : I grille
 *   <J>          : J Grille
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *    - Ceci n'est qu'un wrapper sur c_gdllfxy pour le rendre threadsafe
 *----------------------------------------------------------------------------
*/
int EZGrid_GetLL(TGrid* __restrict const Grid,float* Lat,float* Lon,float* I,float* J,int Nb) {

   int i,ok=0;
   float fi,fj;

   if (Grid->GID>=0) {
   //   RPN_IntLock();
      for(i=0;i<Nb;i++) {
         fi=I[i]+1.0;
         fj=J[i]+1.0;
         if ((ok=c_gdllfxy(Grid->GID,Lat,Lon,&fi,&fj,1))<0) {
            break;
         }
      }
   //   RPN_IntUnlock();
   }
   return(ok==0);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetIJ>
 * Creation : Avril 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les IJs a partir des latlon
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Lat>        : Latitudes
 *   <Lon>        : Longitudes
 *   <I>          : I grille
 *   <J>          : J Grille
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *    - Ceci n'est qu'un wrapper sur c_gdxyfll pour le rendre threadsafe
 *----------------------------------------------------------------------------
*/
int EZGrid_GetIJ(TGrid* __restrict const Grid,float* Lat,float* Lon,float* I,float* J,int Nb) {

   int i,ok=0;

   if (Grid->GID>=0) {
   //   RPN_IntLock();
      ok=c_gdxyfll(Grid->GID,I,J,Lat,Lon,Nb);
   //   RPN_IntUnlock();

      for(i=0;i<Nb;i++) {
         I[i]-=1.0;
         J[i]-=1.0;
      }
   }
   return(ok==0);
}

#endif