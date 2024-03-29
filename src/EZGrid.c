/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie de gestion des champs RPN en tuiles
 * Creation     : Janvier 2008
 * Auteur       : Jean-Philippe Gauthier
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

#define EZGRID_BUILD

#include "App.h"
#include "EZGrid.h"
#include "Vertex.h"
#include "OMP_Utils.h"

TGridYInterpMode EZGRID_YINTERP      = EZ_BARNES;               // Type of linear interpolation for Y grids
int              EZGRID_YLINEARCOUNT = 4;                       // Number of points to use for point cloud interpolation
int              EZGRID_YQTREESIZE   = 1000;                    // Size of the QTree index for Y grids
int              EZGRID_MQTREEDEPTH  = 8;                       // Depth of the QTree index for M grids

static pthread_mutex_t CacheMutex=PTHREAD_MUTEX_INITIALIZER;    // Grid cache mutex
static TGrid          *GridCache[EZGRID_CACHEMAX];              // Grid cache list
static __thread int    MIdx=-1;                                 // Cached previously used mesh index

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
int EZGrid_Wrap(TGrid* restrict const Grid) {

   float i,j,lat,lon;

   Grid->Wrap=0;
   Grid->Pole[0]=Grid->Pole[1]=0.0f;

   if (Grid->H.GRTYP[0]!='M' && Grid->H.GRTYP[0]!='W' && Grid->H.GRTYP[1]!='W' && Grid->H.GRTYP[0]!='Y') {
   
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
            if (lrintf(i)==1) {
               // No repeat
               Grid->Wrap=1;
            } else {
               // First gridpoint is repeated at end
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
static inline TGridTile* EZGrid_TileGet(const TGrid* restrict const Grid,int I,int J) {

   TGridTile *tile=NULL;

   if (Grid->NbTiles==1) {
      tile=&Grid->Tiles[0];
   } else {
      I/=Grid->Tiles[0].NI;
      J/=Grid->Tiles[0].NJ;

      if (I<Grid->NTI && J<Grid->NTJ) {
         tile=&Grid->Tiles[J*Grid->NTI+I];
      }
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
static float **EZGrid_TileGetData(const TGrid* restrict const Grid,TGridTile* restrict const Tile,int K) {

   TGridTile *t0,*t1;
   int        key;
   int        i,ni,nj,nk,t=0;
   int       *tmpi,flag=0,ip1=0,mode=2,type;
   char       format;
   float    **data,*datak,val;
   
   pthread_mutex_lock(&Tile->Mutex);

   if (!EZGrid_IsLoaded(Tile,K)) {

      // Allocate Tile data if not already done
      if (!Tile->Data) {
         if (!(data=(float**)calloc(Grid->H.NK,sizeof(float*)))) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for tile data levels (%s)\n",__func__,Grid->H.NOMVAR);
            pthread_mutex_unlock(&Tile->Mutex);
            return(NULL);
         }
      } else {
         data=Tile->Data;
      }
      
      if (!data[K]) {
         if (!(datak=(float*)calloc(Tile->HNIJ,sizeof(float)))) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for tile data slices (%s)\n",__func__,Grid->H.NOMVAR);
            pthread_mutex_unlock(&Tile->Mutex);
            return(NULL);
         }
      } else {
         datak=data[K];
      }

      if (Grid->H.FID>=0) {                                      
         RPN_FieldLock();
         mode=2;
         type=Grid->ZRef->Type==LVL_ETA?LVL_SIGMA:Grid->ZRef->Type;
         f77name(convip_plus)(&ip1,&Grid->ZRef->Levels[K],&type,&mode,&format,&flag,1);
         key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,Grid->H.DATEV,Grid->H.ETIKET,ip1,Grid->H.IP2,Tile->NO,Grid->H.TYPVAR,Grid->H.NOMVAR);
//         c_fst_data_length((Grid->H.DATYP==1 || Grid->H.DATYP==5)?Grid->H.NBITS>32?8:4)?(Grid->H.NBITS>8?(Grid->H.NBITS>16?(Grid->H.NBITS>32?8:4):2):1));
         if (key<0) {
            mode=3;
            f77name(convip_plus)(&ip1,&Grid->ZRef->Levels[K],&type,&mode,&format,&flag,1);
            key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,Grid->H.DATEV,Grid->H.ETIKET,ip1,Grid->H.IP2,Tile->NO,Grid->H.TYPVAR,Grid->H.NOMVAR);
            if (key<0) {
               Lib_Log(APP_LIBEER,APP_WARNING,"%s: Could not find tile data (%s) at level %f (%i)\n",__func__,Grid->H.NOMVAR,Grid->ZRef->Levels[K],ip1);
            } else {
               RPN_ReadData(datak,TD_Float32,key);
            }
         } else {
            RPN_ReadData(datak,TD_Float32,key);
         }
        
         // Last gridpoint is tile load marker (nan) so it is already nan, use something else
//         if (datak[Tile->HNIJ-1]!=datak[Tile->HNIJ-1]) datak[Tile->HNIJ-1]=-999.0;
         
         // Apply Factor if needed (TODO: ok for now but need to fix concurent access and might conflict with time interp)
         if (Grid->Factor!=1.0f) {
            for(ni=0;ni<Tile->HNIJ;ni++) datak[ni]*=Grid->Factor;
         }
         
         // Check for mask (TYPVAR==@@) 
         if (Grid->H.TYPVAR[1]=='@') {
            key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,Grid->H.DATEV,Grid->H.ETIKET,ip1,Grid->H.IP2,Tile->NO,"@@",Grid->H.NOMVAR);
            if (key>0) {
               // Allocate Tile data if not already done
               if (!Tile->Mask) {
                  if (!(Tile->Mask=(char**)calloc(Grid->H.NK,sizeof(char*)))) {
                     Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for tile mask levels (%s)\n",__func__,Grid->H.NOMVAR);
                     RPN_FieldUnlock();
                     pthread_mutex_unlock(&Tile->Mutex);
                     return(NULL);
                  }
               }

               if (!Tile->Mask[K]) {
                  if ((Tile->Mask[K]=(char*)malloc(ni*nj))) {
                     if ((tmpi=(int*)malloc(ni*nj*sizeof(int)))) {
                        RPN_ReadData(tmpi,TD_Float32,key);
                        for(i=0;i<ni*nj;i++) {
                           Tile->Mask[K][i]=tmpi[i]!=0x0;
                        }
                        free(tmpi);
                     } else {
                        free(Tile->Mask[K]);
                        Tile->Mask[K]=NULL;
                        Lib_Log(APP_LIBEER,APP_WARNING,"%s: Unable to allocate memory to read mask\n",__func__);
                     }
                  } else {
                     Lib_Log(APP_LIBEER,APP_WARNING,"%s: Unable to allocate memory for mask\n",__func__);
                  }
               }          
            } else {
               Lib_Log(APP_LIBEER,APP_WARNING,"%s: Unable to find associated field mask (%s)\n",__func__,Grid->H.NOMVAR);
            }
         }
         
         RPN_FieldUnlock();
      } else if (Grid->T0 && Grid->T1) {  // Check for time interpolation needs
         // Figure out T0 and T1 tile to use
         if (Grid->NbTiles>1) {
            while(&Grid->Tiles[t]!=Tile) t++;
         }
         t0=&(Grid->T0->Tiles[t]);
         t1=&(Grid->T1->Tiles[t]);

         // Make sure the data from the needed tile is loaded
         if (!EZGrid_IsLoaded(t0,K)) EZGrid_TileGetData(Grid->T0,t0,K);
         if (!EZGrid_IsLoaded(t1,K)) EZGrid_TileGetData(Grid->T1,t1,K);

         // Interpolate between by applying factors
         Tile->Mask=t0->Mask;
         for(ni=0;ni<Tile->HNIJ;ni++) {
            datak[ni]=(t0->Data[K][ni]*Grid->FT0+t1->Data[K][ni]*Grid->FT1)*Grid->Factor;
         }
      }

      data[K]=datak;
      Tile->Data=data;
   }

   pthread_mutex_unlock(&Tile->Mutex);
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
float* EZGrid_TileBurn(TGrid* restrict const Grid,TGridTile* restrict const Tile,int K,float* restrict Data) {

   int j,dj;

   pthread_mutex_lock(&Grid->Mutex);
   if (Tile->KBurn!=-1 && Tile->KBurn==K) {
      pthread_mutex_unlock(&Grid->Mutex);
      return(Grid->Data);
   }

   if (!Tile || !Tile->Data) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid tile (%s)\n",__func__,Grid->H.NOMVAR);
      pthread_mutex_unlock(&Grid->Mutex);
      return(NULL);
   }

   // If a buffer array is passed, use it otherwise use the internal one
   if (!Data) {
      if (!Grid->Data) {
         if (!(Grid->Data=(float*)malloc(Grid->H.NIJ*sizeof(float)))) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for grid data (%s)\n",__func__,Grid->H.NOMVAR);
            pthread_mutex_unlock(&Grid->Mutex);
            return(NULL);
         }
      }
      Data=Grid->Data;
      if (!Grid->Mask && Tile->Mask && Tile->Mask[K]) {
         if (!(Grid->Mask=(char*)malloc(Grid->H.NIJ*sizeof(char)))) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for grid mask (%s)\n",__func__,Grid->H.NOMVAR);
            pthread_mutex_unlock(&Grid->Mutex);
            return(NULL);
         }
      }
   }
   dj=Tile->J*Grid->H.NI+Tile->I;

   for(j=0;j<Tile->NJ;j++) {
      memcpy(&Data[dj],&Tile->Data[K][(j+Tile->HDJ)*Tile->HNI+Tile->HDI],Tile->NI*sizeof(float));
      if (Grid->Mask)
         memcpy(&Grid->Mask[dj],&Tile->Mask[K][(j+Tile->HDJ)*Tile->HNI+Tile->HDI],Tile->NI*sizeof(char));
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
float* EZGrid_TileBurnAll(TGrid* restrict const Grid,int K,float* restrict Data) {

   int t;
   TGridTile *tile;
   
   if (Grid) {
      if (Grid->NbTiles>1) {
         for(t=0;t<Grid->NbTiles;t++) {
            tile=&Grid->Tiles[t];
            if (!EZGrid_IsLoaded(tile,K)) {
               EZGrid_TileGetData(Grid,tile,K);
            }
            if (!EZGrid_TileBurn(Grid,tile,K,Data)) {
               return(NULL);
            }
         }
         return(Data?Data:Grid->Data);
      } else {
         EZGrid_TileGetData(Grid,&Grid->Tiles[0],K);
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
 *   <Master>    : Set to 1 for a master grid, 0 for a normal grid and -1 for any grid
 *
 * Retour:
 *  <TGrid*>     : Grille "template" (ou NULL si non existante)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
static TGrid* EZGrid_CacheFind(const TGrid *Grid,int Master) {

   register int n,k;

   int     type;
   float   level;

   if (Grid) {

      pthread_mutex_lock(&CacheMutex);
      for(n=0;n<EZGRID_CACHEMAX;n++) {
         if (GridCache[n]) {
            // Check for master grid
            if( Master!=-1 && Master!=GridCache[n]->Master ) {
               continue;
            }

            // Check for same level type and definitions
            level=ZRef_IP2Level(Grid->H.IP1,&type);
            type=type==LVL_SIGMA?LVL_ETA:type;
            if (type!=GridCache[n]->ZRef->Type) {
               continue;
            }
            if (GridCache[n]->H.NK!=Grid->H.NK) {
               continue;
            }
            for(k=0;k<GridCache[n]->H.NK;k++) if (GridCache[n]->ZRef->Levels[k]==level) break;
            if (k==GridCache[n]->H.NK)
               continue;

            // Check for same grid
            if (Grid->H.GRTYP[0]=='#') {
               if (GridCache[n]->IP1==Grid->H.IG1 && GridCache[n]->IP2==Grid->H.IG2) {
                  pthread_mutex_unlock(&CacheMutex);
                  return(GridCache[n]);
               }
            } else if (Grid->H.GRTYP[0]=='Z' || Grid->H.GRTYP[0]=='M' || Grid->H.GRTYP[0]=='Y' || Grid->H.GRTYP[0]=='O' || Grid->H.GRTYP[0]=='X') {
               if (GridCache[n]->IP1==Grid->H.IG1 && GridCache[n]->IP2==Grid->H.IG2 && GridCache[n]->IP3==Grid->H.IG3) {
                  pthread_mutex_unlock(&CacheMutex);
                  return(GridCache[n]);
               }
            } else if (GridCache[n]->H.GRTYP[0]==Grid->H.GRTYP[0] && GridCache[n]->H.IG1==Grid->H.IG1 && GridCache[n]->H.IG2==Grid->H.IG2 && GridCache[n]->H.IG3==Grid->H.IG3 && GridCache[n]->H.IG4==Grid->H.IG4) {
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
      for(n=0;n<EZGRID_CACHEMAX;n++) {
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
static int EZGrid_CacheAdd(TGrid* restrict const Grid) {

   register int n,i=-1;

   if (Grid) {
      pthread_mutex_lock(&CacheMutex);
      for(n=0;n<EZGRID_CACHEMAX;n++) {
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
      for(n=0;n<EZGRID_CACHEMAX;n++) {
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
int EZGrid_TileGrid(int FIdTo,int NI, int NJ,int Halo,TGrid* restrict const Grid) {

   char   format;
   int    i,j,k,ni,nj,di,dj,pj,no;
   float *tile=NULL,*data;
   int    ip1=0;

   if (!Grid)
      return(FALSE);

   tile=(float*)malloc((NI+Halo*2)*(NJ+Halo*2)*sizeof(float));

   ip1=Grid->H.IP1;

   // Build and save the tiles
   for(k=0;k<Grid->H.NK;k++) {
      no=0;
      data=EZGrid_TileBurnAll(Grid,k,NULL);

      if (Grid->H.NK>1) {
         ip1=ZRef_Level2IP(Grid->ZRef->Levels[k],Grid->ZRef->Type,DEFAULT);
      }

      // Check if dimensions allow tiling
      if (!NI || !NJ || Grid->H.NI==1 || Grid->H.NJ==1 || (Grid->H.NI<NI && Grid->H.NJ<NJ)) {
         cs_fstecr(data,-Grid->H.NBITS,FIdTo,Grid->H.DATEO,Grid->H.DEET,Grid->H.NPAS,Grid->H.NI,Grid->H.NJ,1,ip1,Grid->H.IP2,
            Grid->H.IP3,Grid->H.TYPVAR,Grid->H.NOMVAR,Grid->H.ETIKET,Grid->H.GRTYP,Grid->H.IG1,Grid->H.IG2,Grid->H.IG3,Grid->H.IG4,Grid->H.DATYP,0);
      } else {
         // Build and save the tiles, we adjust the tile size if it is too big
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
               cs_fstecr(tile,-Grid->H.NBITS,FIdTo,Grid->H.DATEO,Grid->H.DEET,Grid->H.NPAS,ni,nj,1,ip1,Grid->H.IP2,
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
int EZGrid_Write(int FId,TGrid* restrict const Grid,int NBits,int Overwrite) {

   int        k,tidx,key,ok=0;
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
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Specified fields do not exist\n",__func__);
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
TGrid* EZGrid_Get(TGrid* restrict const Grid) {

   TRPNHeader h;
   TGridTile *tile;
   int        n,ni,nj,nk,key;
   int        idlst[RPNMAX];
   
   memset(&h,0,sizeof(TRPNHeader));

   // Check for master grid descriptor
   if (Grid->H.GRTYP[0]=='#') {
      key=cs_fstinf(Grid->H.FID,&Grid->H.NI,&h.NJ,&h.NK,-1,"",Grid->H.IG1,Grid->H.IG2,-1,"",">>");
      key=cs_fstinf(Grid->H.FID,&h.NI,&Grid->H.NJ,&h.NK,-1,"",Grid->H.IG1,Grid->H.IG2,-1,"","^^");
      if (key<0) {
         Lib_Log(APP_LIBEER,APP_WARNING,"%s: Could not find master grid descriptor (>>,^^)\n",__func__);
         RPN_FieldUnlock();
         return(NULL);
      }
   }

   // Get vertical reference
   Grid->ZRef=EZGrid_GetZRef(Grid);

   // Get the number of tiles
   cs_fstinl(Grid->H.FID,&h.NI,&h.NJ,&h.NK,Grid->H.DATEV,Grid->H.ETIKET,Grid->H.IP1,Grid->H.IP2,-1,Grid->H.TYPVAR,Grid->H.NOMVAR,idlst,&Grid->NbTiles,RPNMAX);
   Grid->Tiles=(TGridTile*)malloc(Grid->NbTiles*sizeof(TGridTile));
   Grid->Data=NULL;
   Grid->Mask=NULL;
   Grid->GRef=NULL;
   Grid->Halo=0;
   Grid->NTI=Grid->NTJ=0;
   Grid->IP1=Grid->H.IG1;
   Grid->IP2=Grid->H.IG2;
   Grid->IP3=Grid->H.IG3;
   Grid->H.NIJ=Grid->H.NI*Grid->H.NJ;
   h.GRTYP[1]='\0';
   ni=0;

   // Parse the tiles to get the tile limits and structure
   for(n=0;n<Grid->NbTiles;n++) {
      key=cs_fstprm(idlst[n],&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,
            &h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
            h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,
            &h.UBC,&h.EX1,&h.EX2,&h.EX3);
      if (key<0) {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: Missing subgrid number %i\n",__func__,n);
         RPN_FieldUnlock();
         return(NULL);
      }

      tile=&Grid->Tiles[Grid->H.GRTYP[0]=='#'?h.IP3-1:0];

      // Create tile grid
      tile->NO    = Grid->H.GRTYP[0]=='#'?h.IP3:-1;
      tile->HNI   = tile->NI  = h.NI;
      tile->HNJ   = tile->NJ  = h.NJ;
      tile->HNIJ  = tile->NIJ = h.NI*h.NJ;
      tile->HDI   = 0;
      tile->HDJ   = 0;
      tile->Data  = NULL;
      tile->Mask  = NULL;
      tile->KBurn = -1;
      tile->Side  = EZGRID_CENTER;
      pthread_mutex_init(&tile->Mutex,NULL);
      tile->GID =RPN_IntIdNew(h.NI,h.NJ,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,Grid->H.FID);

      // Check for tiled data or not
      if (Grid->H.GRTYP[0]=='#') {
         tile->HI=tile->I=h.IG3-1;
         tile->HJ=tile->J=h.IG4-1;

         // Set the border tile flags and count the number of tiles in I,J
         if (h.IG3==1)                { tile->Side|=EZGRID_LEFT;   Grid->NTJ++; }
         if (h.IG3+h.NI+2>Grid->H.NI) { tile->Side|=EZGRID_RIGHT; }
         if (h.IG4==1)                { tile->Side|=EZGRID_BOTTOM; Grid->NTI++; ni+=h.NI; }
         if (h.IG4+h.NJ+2>Grid->H.NJ) { tile->Side|=EZGRID_TOP; }
      } else {
         tile->HI=tile->I=0;
         tile->HJ=tile->J=0;
         tile->Side|=EZGRID_LEFT|EZGRID_RIGHT|EZGRID_BOTTOM|EZGRID_TOP;
         Grid->NTI=Grid->NTJ=Grid->NbTiles=1;
         break;
      }
   }

   // Is there a halo around the tiles
   if (ni>Grid->H.NI) {
      // Calculate halo width
      Grid->Halo=ceil((ni-Grid->H.NI)/Grid->NTI/2.0);

      // Adjust dimensions with halo, we keep coordinates reference without halo
      for(n=0;n<Grid->NbTiles;n++) {
         tile=&Grid->Tiles[n];

         tile->HDI = (tile->Side&EZGRID_LEFT?0:Grid->Halo);
         tile->HDJ = (tile->Side&EZGRID_BOTTOM?0:Grid->Halo);
         tile->I  += tile->HDI;
         tile->J  += tile->HDJ;
         tile->NI -= tile->HDI+(tile->Side&EZGRID_RIGHT?0:Grid->Halo);
         tile->NJ -= tile->HDJ+(tile->Side&EZGRID_TOP?0:Grid->Halo);
         tile->NIJ = tile->NI*tile->NJ;
      }
   }

   // Create master grid
   if (Grid->H.GRTYP[0]=='#') {
      h.GRTYP[0]='Z';
      h.IG3=h.IG4=0;
   } else {
      h.GRTYP[0]=Grid->H.GRTYP[0];
   }
   

   switch(Grid->H.GRTYP[0]) {
      case 'M':
         Grid->GRef=GeoRef_RPNSetup(Grid->H.NI,Grid->H.NJ,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,Grid->H.FID);

         cs_fstinf(Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->IP1,Grid->IP2,Grid->IP3,"","##");
         Grid->GRef->NIdx=ni*nj*nk;
         Grid->GRef->Idx=(unsigned int*)malloc(Grid->GRef->NIdx*sizeof(unsigned int));
         Grid->GRef->AY=(float*)malloc(Grid->GRef->NX*sizeof(float));
         Grid->GRef->AX=(float*)malloc(Grid->GRef->NX*sizeof(float));

         RPN_sRead(Grid->GRef->AY,TD_Float32,Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->IP1,Grid->IP2,Grid->IP3,"","^^");
         RPN_sRead(Grid->GRef->AX,TD_Float32,Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->IP1,Grid->IP2,Grid->IP3,"",">>");
         RPN_sRead(Grid->GRef->Idx,TD_UInt32,Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->IP1,Grid->IP2,Grid->IP3,"","##");
         
         GeoRef_BuildIndex(Grid->GRef);
         break;
   
      case 'X':
      case 'Y':
      case 'O':
         Grid->GRef=GeoRef_RPNSetup(Grid->H.NI,Grid->H.NJ,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,Grid->H.FID);

         Grid->GRef->AY=(float*)malloc(Grid->H.NIJ*sizeof(float));
         Grid->GRef->AX=(float*)malloc(Grid->H.NIJ*sizeof(float));

         RPN_sRead(Grid->GRef->AY,TD_Float32,Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->IP1,Grid->IP2,Grid->IP3,"","^^");
         RPN_sRead(Grid->GRef->AX,TD_Float32,Grid->H.FID,&ni,&nj,&nk,-1,"",Grid->IP1,Grid->IP2,Grid->IP3,"",">>");
         
         GeoRef_BuildIndex(Grid->GRef);
         break;

      case 'Z':
         {
            int tmpi,desc,key,ig1,ig2,ig3,ni,nj,nk;
            char grtyp[2]={'\0'},tmpc[13]={'\0'};

            // Get the X descriptor
            if( (desc=cs_fstinf(Grid->H.FID,&ni,&nj,&tmpi,-1,"",h.IG1,h.IG2,h.IG3,"",">>")) <=0 ) {
               Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not find grid descriptor (>>) of Z grid\n",__func__);
               RPN_FieldUnlock();
               return(NULL);
            }
            if( cs_fstprm(desc,&ni,&nj,&nk,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,
                  tmpc,tmpc,tmpc,grtyp,&ig1,&ig2,&ig3,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi) ) {
               Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not stat grid descriptor (>>) of Z grid\n",__func__);
               RPN_FieldUnlock();
               return(NULL);
            }

            // If we have a W grtyp, then we are dealing with a WKT projection
            if( grtyp[0] == 'W' ) {
               double transform[6];
               char *str=NULL;

               Grid->H.GRTYP[1] = 'W';

               // Read the transformation matrix
               if( (key=cs_fstinf(Grid->H.FID,&ni,&nj,&nk,-1,"",ig1,ig2,ig3,"","MTRX")) <=0 ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not find grid transformation matrix (MTRX) of Z grid\n",__func__);
                  goto werr;
               }
               if( ni*nj*nk != 6 ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: MTRX field should have a dimension of exactly 6, but has %d*%d*%d=%d instead\n",__func__,ni,nj,nk,ni*nj*nk);
                  goto werr;
               }
               if( RPN_sReadData(transform,TD_Float64,key) != APP_OK ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not read grid transformation matrix (MTRX) of Z grid\n",__func__);
                  goto werr;
               }

               // Read the projection string
               if( (key=cs_fstinf(Grid->H.FID,&ni,&nj,&nk,-1,"",ig1,ig2,ig3,"","PROJ")) <=0 ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not find grid projection string (PROJ) of Z grid\n",__func__);
                  goto werr;
               }
               if( !(str=malloc((ni*nj*nk+1)*sizeof(*str))) ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not allocate memory for projection string (PROJ) of Z grid\n",__func__);
                  goto werr;
               }
               if( RPN_sReadData(str,TD_Byte,key) != APP_OK ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not read grid projection string (PROJ) of Z grid\n",__func__);
                  goto werr;
               }
               str[ni*nj*nk] = '\0';

               // Create the WKT georef
               Grid->GRef=GeoRef_WKTSetup(Grid->H.NI,Grid->H.NJ,Grid->H.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,str,transform,NULL,NULL);

               // Memory for the descriptors
               if( !(Grid->GRef->AX=malloc(Grid->H.NIJ*sizeof(*Grid->GRef->AX))) ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not allocate memory for descriptor (>>) of Z grid\n",__func__);
                  goto werr;
               }
               if( !(Grid->GRef->AY=malloc(Grid->H.NIJ*sizeof(*Grid->GRef->AY))) ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not allocate memory for descriptor (^^) of Z grid\n",__func__);
                  goto werr;
               }

               // Read the descriptors
               if( RPN_sReadData(Grid->GRef->AX,TD_Float32,desc) != APP_OK ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not read grid descriptor (>>) of Z grid\n",__func__);
                  goto werr;
               }
               if( RPN_sRead(Grid->GRef->AY,TD_Float32,Grid->H.FID,&ni,&nj,&nk,-1,"",h.IG1,h.IG2,h.IG3,"","^^") != APP_OK ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not find grid descriptor (^^) of Z grid\n",__func__);
                  goto werr;
               }

               // No need to do the default steps
               free(str);
               break;
werr:
               free(str);
               GeoRef_Free(Grid->GRef); Grid->GRef=NULL;
               RPN_FieldUnlock();
               return(NULL);
            }
            // *** NO BREAK : Fall through to default condition if our Z grid is not on a W
         }

      default:
         Grid->GRef=GeoRef_RPNSetup(Grid->H.NI,Grid->H.NJ,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,Grid->H.FID);

         Grid->GID=RPN_IntIdNew(Grid->H.NI,Grid->H.NJ,h.GRTYP,h.IG1,h.IG2,h.IG3,h.IG4,Grid->H.FID);
         Grid->Wrap=EZGrid_Wrap(Grid);
   }
   GeoRef_Qualify(Grid->GRef);

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
int EZGrid_BoundaryCopy(TGrid* restrict const Grid,int Width) {

   TGridTile    *tile;
   unsigned long idx,i,j,n,k;

   if (Width!=1 && Width!=2)
      return(FALSE);

   for(n=0;n<Grid->NbTiles;n++) {
      tile=&Grid->Tiles[n];

      for(k=0;k<Grid->ZRef->LevelNb;k++) {
         if (tile->Side&EZGRID_BOTTOM) {
            for(i=0,idx=0;i<tile->HNI;i++,idx++) {
               if (Width==2)
                  tile->Data[k][idx+tile->HNI]=tile->Data[k][idx+tile->HNI+tile->HNI];
               tile->Data[k][idx]=tile->Data[k][idx+tile->HNI];
            }
         }
         if (tile->Side&EZGRID_TOP) {
            for(i=0,idx=tile->HNIJ-tile->HNI-1;i<tile->HNI;i++,idx++) {
               if (Width==2)
                  tile->Data[k][idx-tile->HNI]=tile->Data[k][idx-tile->HNI-tile->HNI];
               tile->Data[k][idx]=tile->Data[k][idx-tile->HNI];
            }
         }
         if (tile->Side&EZGRID_LEFT) {
            for(j=0,idx=0;j<tile->HNJ;j++,idx+=tile->HNI) {
               if (Width==2)
                  tile->Data[k][idx+1]=tile->Data[k][idx+2];
               tile->Data[k][idx]=tile->Data[k][idx+1];
            }
         }
         if (tile->Side&EZGRID_RIGHT) {
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
TZRef* EZGrid_GetZRef(const TGrid* restrict const Grid) {

   TZRef *zref;

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
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

   if ((new=(TGrid*)malloc(sizeof(TGrid)))) {
      pthread_mutex_init(&new->Mutex,NULL);
      new->Master=0;
      new->Data=NULL;
      new->Mask=NULL;
      new->T0=new->T1=NULL;
      new->FT0=new->FT1=0.0f;
      new->Factor=1.0f;
      new->Incr=0;
      new->GID=-1;
      new->ZRef=NULL;
      new->GRef=NULL;
      new->Wrap=0;
      new->Halo=0;
      new->IP1=new->IP2=new->IP3=0;
      new->NTI=new->NTJ=0;
      new->NbTiles=0;
      new->Tiles=NULL;
      
      memset(&new->H,0,sizeof(TRPNHeader));
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

   if (Master && (Level<0 || Level<Master->ZRef->LevelNb) && (new=(TGrid*)malloc(sizeof(TGrid)))) {
      pthread_mutex_init(&new->Mutex,NULL);
      new->Master=0;
      new->Data=NULL;
      new->Mask=NULL;
      new->T0=new->T1=NULL;
      new->FT0=new->FT1=0.0f;
      new->Factor=1.0f;
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
      
      new->GRef=GeoRef_Copy(Master->GRef);
      new->ZRef=ZRef_Copy(Master->ZRef);

      if (Level>=0) {
         new->H.IP1=ZRef_Level2IP(new->ZRef->Levels[Level],new->ZRef->Type,new->ZRef->Style);
         new->H.NK=1;
      } else {         
         new->H.NK=new->ZRef->LevelNb;
      }
      
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

int32_t f77name(ezgrid_free)(int32_t *gdid) {
   EZGrid_Free(GridCache[*gdid]);
   return(TRUE);
}

void EZGrid_Free(TGrid* restrict const Grid) {

   int        n,k;
   TGridTile *tile;

   if (Grid) {
      // Cleanup tile data
      for(n=0;n<Grid->NbTiles;n++) {
         tile=&Grid->Tiles[n];
         
         if (tile->Data) {
            for(k=0;k<Grid->H.NK;k++) {
               if (tile->Data[k]) {
                  free(tile->Data[k]);
                  tile->Data[k]=NULL;
               }
               if (!Grid->T0 && tile->Mask && tile->Mask[k] && Grid->H.FID!=-1) {
                  free(tile->Mask[k]);
                  tile->Mask[k]=NULL;
               }  
            }
            free(tile->Data);
            tile->Data=NULL;
            pthread_mutex_destroy(&tile->Mutex);
            
            // Free mask only on fields with associated file, otherwise it's referenced
            if (!Grid->T0 && Grid->H.FID!=-1 && tile->Mask) {
               free(tile->Mask);
            }
            tile->Mask=NULL;
         }
      }
      if (Grid->Data) {
         free(Grid->Data);
         Grid->Data=NULL;
      }
      if (Grid->Mask) {
         free(Grid->Mask);
         Grid->Mask=NULL;
      }

      pthread_mutex_destroy(&Grid->Mutex);

      // If this is a master, keep in memory
      if (!Grid->Master) {
         if (Grid->Tiles) {
            free(Grid->Tiles);
            Grid->Tiles=NULL;
         }
   //      ZRef_Free(Grid->ZRef);
         EZGrid_CacheDel(Grid);
         free(Grid);
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
void EZGrid_Clear(TGrid* restrict const Grid) {

   int        n,k;
   float      f=nanf("NaN");
   TGridTile *tile;
   
   // Cleanup tile data
   for(n=0;n<Grid->NbTiles;n++) {
      tile=&Grid->Tiles[n];
      if (tile->Data) {
         for(k=0;k<Grid->H.NK;k++) {
            if (tile->Data[k])
               tile->Data[k][tile->HNIJ-1]=f;
         }
      }
      // If tile is interpolated, mask is a reference
      if (Grid->T0) tile->Mask=NULL;
      
      tile->KBurn=-1;
   }
   Grid->T0=Grid->T1=NULL;
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
int32_t f77name(ezgrid_read)(int32_t *fid,char *var,char *typvar,char *etiket,int32_t *datev,int32_t *ip1,int32_t *ip2,int32_t *incr) {
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

   TRPNHeader h;
   int        key;
   int       ni,nj,nk;

   // Get field info
   key=cs_fstinf(FId,&ni,&nj,&nk,DateV,Etiket,IP1,IP2,-1,TypVar,Var);

   // If we're searching for any nomvar, make sure we don't get a descriptor
   // Likewise, if we're searching for any typvar, make sure we don't get a mask
   if( key>=0 && (Var[0]=='\0' || TypVar[0]=='\0') ) {
      strcpy(h.NOMVAR,"    ");
      strcpy(h.TYPVAR,"  ");
      c_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
         h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);
      while( key>=0 && (Var[0]=='\0'&&RPN_IsDesc(h.NOMVAR) || TypVar[0]=='\0'&&h.TYPVAR[0]=='@'&&h.TYPVAR[1]=='@')) {
         key=cs_fstsui(FId,&ni,&nj,&nk);
         strcpy(h.NOMVAR,"    ");
         strcpy(h.TYPVAR,"  ");
         c_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
            h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);
      }
   }

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
int32_t f77name(ezgrid_readidx)(int32_t *fid,int32_t *key,int32_t *incr) {
   TGrid *fld = EZGrid_ReadIdx(*fid,*key,*incr);
   return(fld->Master ? EZGrid_CacheIdx(fld) : EZGrid_CacheAdd(fld));
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
   // mst is not in a critical sectin but its veryyyyyyyyyy unlikelyyyyyy that it will cause problems
   if ((mst=EZGrid_CacheFind(new,1))) {
      new->GID=mst->GID;
      new->ZRef=mst->ZRef;
      new->GRef=mst->GRef;
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
         new->Tiles[n].Mask=NULL;
         new->Tiles[n].KBurn=-1;
         pthread_mutex_init(&new->Tiles[n].Mutex,NULL);
      }
   } else {
      if (!EZGrid_Get(new)) {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not create master grid (%s)\n",__func__,new->H.NOMVAR);
         free(new);
         return(NULL);
      } else {
         new->Master=1;
         EZGrid_CacheAdd(new);
      }
   }

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
int32_t f77name(ezgrid_load)(int32_t *gdid,int32_t *i0,int32_t *j0,int32_t *k0,int32_t *i1,int32_t *j1,int32_t *k1) {
   return(EZGrid_Load(GridCache[*gdid],*i0,*j0,*k0,*i1,*j1,*k1));
}
int EZGrid_Load(const TGrid* restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1) {

   TGridTile *tile;
   int        i,j,k;

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   /*Check inclusion in master grid limits*/
   if (I0<0 || J0<0 || K0<0 || I0>=Grid->H.NI || J0>=Grid->H.NJ || K0>=Grid->H.NK ||
       I1<0 || J1<0 || K1<0 || I1>=Grid->H.NI || J1>=Grid->H.NJ || K1>=Grid->H.NK) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): I(%i,%i) J(%i,%i) K(%i,%i)\n",__func__,Grid->H.NOMVAR,I0,I1,J0,J1,K0,K1);
      return(FALSE);
   }

   /*Loop on coverage*/
   for(k=K0;k<=K1;k++) {
      for(j=J0;j<=J1;j++) {
         for(i=I0;i<=I1;i++) {
            /*Figure out the right tile*/
            if (!(tile=EZGrid_TileGet(Grid,i,j))) {
               Lib_Log(APP_LIBEER,APP_WARNING,"%s: Tile not found (%s) I(%i) J(%i)\n",__func__,Grid->H.NOMVAR,i,j);
               return(FALSE);
            }

            /*Check for tile data*/
            if (!EZGrid_TileGetData(Grid,tile,k)) {
               return(FALSE);
            }
         }
      }
   }
   return(TRUE);
}

int32_t f77name(ezgrid_loadall)(int32_t *gdid) {
   return(EZGrid_LoadAll(GridCache[*gdid]));
}
int EZGrid_LoadAll(const TGrid* restrict const Grid) {

   int k=0,idx;

   if (Grid) {
      /*Loop on coverage*/
      for(k=0;k<Grid->H.NK;k++) {
         idx=0;
         while(idx<Grid->NTI*Grid->NTJ) {

            /*Check for tile data*/
            if (!EZGrid_TileGetData(Grid,&Grid->Tiles[idx],k)) {
               Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to get tile data (%s) at level %i\n",__func__,Grid->H.NOMVAR,k);
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
 *----------------------------------------------------------------------------
*/
int32_t f77name(ezgrid_interptime)(int32_t *gdid0,int32_t *gdid1,int32_t *date) {
   return(EZGrid_CacheAdd(EZGrid_InterpTime(NULL,GridCache[*gdid0],GridCache[*gdid1],*date)));
}

TGrid *EZGrid_InterpTime(TGrid* restrict Grid,const TGrid* restrict Grid0,const TGrid* restrict Grid1,int Date) {

   TGrid *new;
   double delay,dt;
   int    i,n,k;

   /*Figure out the delai between the 2 fields*/
   f77name(difdatr)((int*)&Grid0->H.DATEV,(int*)&Grid1->H.DATEV,&delay);
   f77name(difdatr)((int*)&Grid0->H.DATEV,&Date,&dt);

   dt/=delay;

   return(EZGrid_InterpFactor(Grid,Grid0,Grid1,1.0-dt,dt));
}

int32_t f77name(ezgrid_factor)(int32_t *gdid,float *f) {
   EZGrid_Factor(GridCache[*gdid],*f);
   return(TRUE);
}

void EZGrid_Factor(TGrid* restrict Grid,const float Factor) {

   Grid->Factor=Factor;
}

int32_t f77name(ezgrid_interpfactor)(int32_t *gdid0,int32_t *gdid1,float *f0,float *f1) {
   return(EZGrid_CacheAdd(EZGrid_InterpFactor(NULL,GridCache[*gdid0],GridCache[*gdid1],*f0,*f1)));
}

TGrid *EZGrid_InterpFactor(TGrid* restrict Grid,const TGrid* restrict Grid0,const TGrid* restrict Grid1,float Factor0,float Factor1) {

   TGrid *new;
   int    i;

   /*Allocate new tile*/
   if (Grid) {
      new=Grid;
   } else {
      new=EZGrid_New();
      memcpy(new,Grid0,sizeof(TGrid));

//      new->GRef=GeoRef_Copy(Master->GRef);
//      new->ZRef=ZRef_Copy(Master->ZRef);
      new->GRef=Grid0->GRef;
      new->ZRef=Grid0->ZRef;
      new->Data=NULL;
      new->Mask=NULL;
      new->Master=0;
      new->H.FID=-1;
      new->Tiles=(TGridTile*)malloc(Grid0->NbTiles*sizeof(TGridTile));
      memcpy(new->Tiles,Grid0->Tiles,Grid0->NbTiles*sizeof(TGridTile));
      for(i=0;i<Grid0->NbTiles;i++) {
         new->Tiles[i].Data=NULL;
         new->Tiles[i].Mask=NULL;
         new->Tiles[i].KBurn=-1;
      }
   }

   new->T0=Grid0;
   new->T1=Grid1;
   new->FT0=Factor0;
   new->FT1=Factor1;

   return(new);
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
int32_t f77name(ezgrid_interp)(int32_t *to,int32_t *from) {
   return(EZGrid_Interp(GridCache[*to],GridCache[*from]));
}

int EZGrid_Interp(TGrid* restrict const To, TGrid* restrict const From) {

   int    ok;
   float *from,*to;

   if (!From) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid input grid (%s)\n",__func__,From->H.NOMVAR);
      return(FALSE);
   }

   if (!To) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid output grid (%s)\n",__func__,To->H.NOMVAR);
      return(FALSE);
   }

   if (!(from=EZGrid_TileBurnAll(From,0,NULL))) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Problems with input grid\n",__func__);
      return(FALSE);
   }
   if (!(to=EZGrid_TileBurnAll(To,0,NULL))) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Problems with output grid\n",__func__);
      return(FALSE);
   }

   RPN_IntLock();
   ok=c_ezdefset(To->GID,From->GID);
   ok=c_ezsint(to,from);
   RPN_IntUnlock();
   if (ok<0)  {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to do interpolation (c_ezscint (%i))\n",__func__,ok);
      return(FALSE);
   }

   return(TRUE);
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
int32_t f77name(ezgrid_getlevelnb)(int32_t *gdid) {
   return(EZGrid_GetLevelNb(GridCache[*gdid]));
}
int EZGrid_GetLevelNb(const TGrid* restrict const Grid) {
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
int32_t f77name(ezgrid_getleveltype)(int32_t *gdid) {
   return(EZGrid_GetLevelType(GridCache[*gdid]));
}
int EZGrid_GetLevelType(const TGrid* restrict const Grid) {
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
int32_t f77name(ezgrid_getlevels)(int32_t *gdid,float *levels,int32_t *type) {
   return(EZGrid_GetLevels(GridCache[*gdid],levels,type));
}
int EZGrid_GetLevels(const TGrid* restrict const Grid,float* restrict Levels,int* restrict Type) {

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
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
float f77name(ezgrid_getlevel)(int32_t *gdid,float *pressure,float *p0) {
   return(EZGrid_GetLevel(GridCache[*gdid],*pressure,*p0));
}
float EZGrid_GetLevel(const TGrid* restrict const Grid,float Pressure,float P0) {

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(-1.0);
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
float f77name(ezgrid_getpressure)(int32_t *gdid,float *level,float *p0,float *p0ls) {
   return(EZGrid_GetPressure(GridCache[*gdid],*level,*p0,*p0ls));
}
float EZGrid_GetPressure(const TGrid* restrict const Grid,float Level,float P0,float P0LS) {

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(-1.0);

   }

   //TODO: needs p0ls
   return(ZRef_Level2Pressure(Grid->ZRef,P0,P0LS,Level));
}


/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_LLGetValue>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeur a une lat-lon pour un range de niveaux
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Mode>       : Interpolarion mode (EZ_NEAREST,EZ_LINEAR)
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
 *   - Gere les grille Y (Cloud point) et M (Triangle meshes)
 *----------------------------------------------------------------------------
*/
int32_t f77name(ezgrid_llgetvalue)(int32_t *gdid,int32_t *mode,float *lat,float *lon,int32_t *k0,int32_t *k1,float *val) {
   return(EZGrid_LLGetValue(GridCache[*gdid],*mode,*lat,*lon,*k0-1,*k1-1,val));
}

int EZGrid_LLGetValue(TGrid* restrict const Grid,TGridInterpMode Mode,float Lat,float Lon,int K0,int K1,float* restrict Value) {

   float      i,j;

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   if (K0<0 || K0>=Grid->H.NK || K1<0 || K1>=Grid->H.NK) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): K(%i,%i)\n",__func__,Grid->H.NOMVAR,K0,K1);
      return(FALSE);
   }

   switch(Grid->H.GRTYP[0]) {
      case 'X': // This is a $#@$@#% grid (orca)
      case 'O':
         return(EZGrid_LLGetValueO(Grid,NULL,Mode,Lat,Lon,K0,K1,Value,NULL,1.0));
         break;

      case 'Y': // This is a point cloud
         return(EZGrid_LLGetValueY(Grid,NULL,Mode,Lat,Lon,K0,K1,Value,NULL,1.0));
         break;

      case 'M': // This is a triangle mesh
         return(EZGrid_LLGetValueM(Grid,NULL,Mode,Lat,Lon,K0,K1,Value,NULL,1.0));
         break;

      case 'Z':
         // Handle the case of ZW grids
         if( Grid->H.GRTYP[1] == 'W' ) {
            double i,j;

            if( !Grid->GRef->UnProject(Grid->GRef,&i,&j,Lat,Lon,0,1) ) {
               return(FALSE);
            }
            return(EZGrid_IJGetValue(Grid,Mode,i,j,K0,K1,Value));
         }
         // ELSE, we fallthrough (no break)

      default: // This is a regular RPN grid

         // RPN_IntLock();
         c_gdxyfll(Grid->GID,&i,&j,&Lat,&Lon,1);
         // RPN_IntUnlock();
         return(EZGrid_IJGetValue(Grid,Mode,i-1.0f,j-1.0f,K0,K1,Value));         
   }
   
   return(FALSE);
}

int EZGrid_LLGetValueO(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,float Lat,float Lon,int K0,int K1,float* restrict UU,float* restrict VV,float Conv) {

   TGridTile   *tu,*tv;
   double       i,j,d,th,len;
   int          k=0,ik=0;
   unsigned int idx;
   
   if (!GridU || (GridU->H.GRTYP[0]!='O' && GridU->H.GRTYP[0]!='X')) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }
   
   if (!GridU->GRef->UnProject(GridU->GRef,&i,&j,Lat,Lon,FALSE,TRUE)){
      return(FALSE);
   }

   tu=tv=NULL;
   k=K0;
   if (GridV) 
      d=GeoRef_GeoDir(GridU->GRef,i,j);
   
   do {   
                   tu=&GridU->Tiles[0]; if (!EZGrid_IsLoaded(tu,k)) EZGrid_TileGetData(GridU,tu,k);
      if (GridV) { tv=&GridV->Tiles[0]; if (!EZGrid_IsLoaded(tv,k)) EZGrid_TileGetData(GridV,tv,k); }
            
      if (Mode==EZ_NEAREST) {
         idx=lrintf(j)*tu->NI+lrintf(i);
         if (tu->Mask && tu->Mask[k] && !tu->Mask[k][idx]) {
            ik++;
            continue;
         }
                 UU[ik]=tu->Data[k][idx];
         if (tv) VV[ik]=tv->Data[k][idx];
      } else {
                 UU[ik]=Vertex_ValS(tu->Data[k],tu->Mask[k],tu->NI,tu->NJ,i,j,FALSE);
         if (tv) VV[ik]=Vertex_ValS(tv->Data[k],tu->Mask[k],tv->NI,tv->NJ,i,j,FALSE);    
      }
    
      if (Conv!=1.0) {
                 UU[ik]*=Conv;
         if (tv) VV[ik]*=Conv;         
      }
      
      // Re-orient components geographically
      if (GridV) {
         th=atan2(UU[ik],VV[ik])-d;
         len=hypot(UU[ik],VV[ik]);
         UU[ik]=len*sin(th);
         VV[ik]=len*cos(th);
      }
      ik++;
   } while ((K0<=K1?k++:k--)!=K1);
                
   return(TRUE);  
}

int EZGrid_LLGetValueY(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,float Lat,float Lon,int K0,int K1,float* restrict UU,float* restrict VV,float Conv) {

   TGridTile *tu,*tv;
   double     r,wt,efact,dists[EZGRID_YLINEARCOUNT],w[EZGRID_YLINEARCOUNT];
   int        k,ik=0,n,nb=-1,idxs[EZGRID_YLINEARCOUNT];
   
   if (!GridU || GridU->H.GRTYP[0]!='Y') {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }
   
   CLAMPLON(Lon);
   // Find nearest(s) points: 1 point if Mode==EZ_NEAREST or EZGRID_YLINEARCOUNT points if  Mode!=EZ_NEAREST
   if (!(nb=GeoRef_Nearest(GridU->GRef,Lon,Lat,idxs,dists,Mode==EZ_NEAREST?1:EZGRID_YLINEARCOUNT))) {
      return(FALSE);
   }
   
   if (nb>1) {
      // Get search radius from farthest point
      r=dists[nb-1];
      
      if (EZGRID_YINTERP==EZ_BARNES) {
         // 14.2 factor determined with trial-and-error tests.
         // This factor narrows the gaussian distribution shape(more weight for the nearest data point)
         efact=M_PI*14.2/(r*r);
      } else {
         // Ensure that the weight of the farthest data point weight is not 0.0 for EZ_CRESSMAN interp. type.
         r=1.001*r;    
      }
      
      // Calculate modulated weight
      wt=0;
      for(n=0;n<nb;n++) {                  
         if (EZGRID_YINTERP==EZ_BARNES) {
            w[n]=exp(-efact*(dists[n]*dists[n]));
         } else {
            w[n]=(r-dists[n])/(r+dists[n]);
         }
         wt+=w[n];
      }
   }
   
   tu=tv=NULL;
   k=K0;
   do {
                   tu=&GridU->Tiles[0]; if (!EZGrid_IsLoaded(tu,k)) EZGrid_TileGetData(GridU,tu,k);
      if (GridV) { tv=&GridV->Tiles[0]; if (!EZGrid_IsLoaded(tv,k)) EZGrid_TileGetData(GridV,tv,k); }
            
      if (nb==1) {
         // For a single nearest, return value
                 UU[ik]=tu->Data[k][idxs[0]];
         if (tv) VV[ik]=tv->Data[k][idxs[0]];
      } else {
         // Otherwise, interpolate
                 UU[ik]=0.0;
         if (tv) VV[ik]=0.0;
         
         
         // Sum values modulated by weight
         for(n=0;n<nb;n++) {                  
                    UU[ik]+=tu->Data[k][idxs[n]]*w[n];
            if (tv) VV[ik]+=tv->Data[k][idxs[n]]*w[n];
         }
                 UU[ik]/=wt;
         if (tv) VV[ik]/=wt;
      }
      
      if (Conv!=1.0) {
                 UU[ik]*=Conv;
         if (tv) VV[ik]*=Conv;         
      }
      ik++;
   } while ((K0<=K1?k++:k--)!=K1);
                
   return(TRUE);  
}

int EZGrid_LLGetValueM(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,float Lat,float Lon,int K0,int K1,float* restrict UU,float* restrict VV,float Conv) {

   TGridTile   *tu,*tv;
   int          k=0,ik=0;
   TQTree      *node;
   TGeoRef     *gref;
   Vect3d       bary;
   int          n,idxs[3];
   intptr_t    idx;
   
   if (!GridU || GridU->H.GRTYP[0]!='M') {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }
   
   CLAMPLON(Lon);
   gref=GridU->GRef;
  
   // Check with previously cached triangle (30% faster overall)
   if (MIdx!=-1) {
      idxs[0]=gref->Idx[MIdx];
      idxs[1]=gref->Idx[MIdx+1];
      idxs[2]=gref->Idx[MIdx+2];
      
      // If the barycentric coordinates are within this triangle, get its interpolated value
      k=Bary_Get(bary,gref->Wght?gref->Wght[MIdx/3]:0.0,Lon,Lat,gref->AX[idxs[0]],gref->AY[idxs[0]],gref->AX[idxs[1]],gref->AY[idxs[1]],gref->AX[idxs[2]],gref->AY[idxs[2]]);
   }

   // Otherwise, look for the enclosing triangle
   if (!k) {
      if ((node=QTree_Find(gref->QTree,Lon,Lat)) && node->NbData) {
         // Loop on this nodes data payload
         for(n=0;n<node->NbData;n++) {
            idx=(intptr_t)node->Data[n].Ptr-1; // Remove false pointer increment
            idxs[0]=gref->Idx[idx];
            idxs[1]=gref->Idx[idx+1];
            idxs[2]=gref->Idx[idx+2];
            
            // if the Barycentric coordinates are within this triangle, get its interpolated value
            if ((k=Bary_Get(bary,gref->Wght?gref->Wght[idx/3]:0.0,Lon,Lat,gref->AX[idxs[0]],gref->AY[idxs[0]],gref->AX[idxs[1]],gref->AY[idxs[1]],gref->AX[idxs[2]],gref->AY[idxs[2]]))) {
               MIdx=idx;
               break;
            }
         }
      }
   }
   
   // If we found one, get the interpolated values
   if (k) {
      tu=tv=NULL;
      k=K0;
      do {
                      tu=&GridU->Tiles[0]; if (!EZGrid_IsLoaded(tu,k)) EZGrid_TileGetData(GridU,tu,k);
         if (GridV) { tv=&GridV->Tiles[0]; if (!EZGrid_IsLoaded(tv,k)) EZGrid_TileGetData(GridV,tv,k); }
               
         if (Mode==EZ_NEAREST) {
                    n=(bary[0]>bary[1]?(bary[0]>bary[2]?0:2):(bary[1]>bary[2]?1:2));
                    UU[ik]=tu->Data[k][n];
            if (tv) VV[ik]=tv->Data[k][n];
         } else {
                    UU[ik]=Bary_Interp(bary,tu->Data[k][idxs[0]],tu->Data[k][idxs[1]],tu->Data[k][idxs[2]]);    
            if (tv) VV[ik]=Bary_Interp(bary,tv->Data[k][idxs[0]],tv->Data[k][idxs[1]],tv->Data[k][idxs[2]]);    
         }
                  
         if (Conv!=1.0) {
                    UU[ik]*=Conv;
            if (tv) VV[ik]*=Conv;         
         }
         ik++;
      } while ((K0<=K1?k++:k--)!=K1);
   } else {
      // Out of meshe
      return(FALSE);      
   }
                
   return(TRUE);  
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
 *   <Mode>       : Interpolarion mode (EZ_NEAREST,EZ_LINEAR)
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
int32_t f77name(ezgrid_llgetuvvalue)(int32_t *gdidu,int32_t *gdidv,int32_t *mode,float *lat,float *lon,int32_t *k0,int32_t *k1,float *uu,float *vv,float *conv) {
   return(EZGrid_LLGetUVValue(GridCache[*gdidu],GridCache[*gdidv],*mode,*lat,*lon,*k0-1,*k1-1,uu,vv,*conv));
}
int EZGrid_LLGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,float Lat,float Lon,int K0,int K1,float* restrict UU,float* restrict VV,float Conv) {

   float i,j;

   if (!GridU || !GridV) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   if (K0<0 || K0>=GridU->H.NK || K1<0 || K1>=GridU->H.NK) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): K(%i,%i)\n",__func__,GridU->H.NOMVAR,K0,K1);
      return(FALSE);
   }
   
   switch(GridU->H.GRTYP[0]) {
      case 'X': // This is a $#@$@#% grid (orca)
      case 'O':
         return(EZGrid_LLGetValueO(GridU,GridV,Mode,Lat,Lon,K0,K1,UU,VV,Conv));
         break;
         
      case 'Y': // This is a point cloud
         return(EZGrid_LLGetValueY(GridU,GridV,Mode,Lat,Lon,K0,K1,UU,VV,Conv));
         break;
         
      case 'M': // This is a triangle mesh
         return(EZGrid_LLGetValueM(GridU,GridV,Mode,Lat,Lon,K0,K1,UU,VV,Conv));
         break;

      case 'Z':
         // Handle the case of ZW grids
         if( GridU->H.GRTYP[1] == 'W' ) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unimplemented for ZW grids\n",__func__);
            return(FALSE);
         }
         // ELSE, we fallthrough (no break)
                  
      default: // This is a regular RPN grid
         
         // RPN_IntLock();
         c_gdxyfll(GridU->GID,&i,&j,&Lat,&Lon,1);
         // RPN_IntUnlock();

         return(EZGrid_IJGetUVValue(GridU,GridV,Mode,i-1.0f,j-1.0f,K0,K1,UU,VV,Conv));
   }
   
   return(FALSE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_IJGetValue>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeur a une coordonnee IJ en point de grille pour un range de niveaux
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Mode>       : Interpolarion mode (EZ_NEAREST,EZ_LINEAR)
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

int32_t f77name(ezgrid_ijgetvalue)(int32_t *gdid,int32_t *mode,float *i,float *j,int32_t *k0,int32_t *k1,float *val) {
   return(EZGrid_IJGetValue(GridCache[*gdid],*mode,*i-1,*j-1,*k0-1,*k1-1,val));
}

int EZGrid_IJGetValue(TGrid* restrict const Grid,TGridInterpMode Mode,float I,float J,int K0,int K1,float* restrict Value) {

   TGridTile *t,*tw=NULL;
   int        i,j,k,ik=0,idx,idxj,idxw,idxwj,wrap=0;
   float      dx,dy,dxy,d[4];

   if (!Grid || Grid->GID<0) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   // Check inclusion in master grid limits
   if (I<0 || J<0 || K0<0 || K1<0 || J>=Grid->H.NJ-0.5 || K0>=Grid->H.NK || K1>=Grid->H.NK) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): I(%f) J(%f) K(%i,%i)\n",__func__,Grid->H.NOMVAR,I,J,K0,K1);
      return(FALSE);
   }

   if (Mode==EZ_NEAREST) {
      I=ROUND(I);
      J=ROUND(J);
      wrap=0;
   }
   
   // Check for overshoot in X (For linear its NI-1 but nearest it's NI)
   if (I>=Grid->H.NI-Mode) {
      if (Grid->Wrap) {
         wrap=1;
      } else {
         Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): I(%f) J(%f) K(%i,%i)\n",__func__,Grid->H.NOMVAR,I,J,K0,K1);
         return(FALSE);
      }
   }      
   
   if (!(t=EZGrid_TileGet(Grid,I,J))) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Tile not found (%s) I(%.f) J(%f)\n",__func__,Grid->H.NOMVAR,I,J);
      return(FALSE);
   }

   // Calculate in-tile indexes and limits
   i=I;
   j=J;
   dx=I-i;
   dy=J-j;
   dxy=dx*dy;
   i=i-t->HI;
   j=j-t->HJ;
   //EZ   I+=1.0f;J+=1.0f;

   idx =j*t->HNI+i;
   idxj=j<t->HNJ-1?idx+t->HNI:idx;

   // Get the wrapping tile
   if (wrap) {
      if (!(tw=EZGrid_TileGet(Grid,0,J))) {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: Wrapped tile not found (%s) I(%.f) J(%f)\n",__func__,Grid->H.NOMVAR,I,J);
         return(FALSE);
      }
      // If we wrap, index of first point within wrapped tile
      idxw=j*tw->HNI;
      idxwj=idxw+(idx==idxj?0:tw->HNI);
   }

   k=K0;
   do {
      if (!EZGrid_IsLoaded(t,k))
         EZGrid_TileGetData(Grid,t,k);
 
      d[0]=t->Data[k][idx];

      if (Mode==EZ_NEAREST) {
         Value[ik]=d[0];
      } else {
         d[2]=t->Data[k][idxj];

         if (tw) {
            if (!EZGrid_IsLoaded(tw,k))
               EZGrid_TileGetData(Grid,tw,k);

            d[1]=tw->Data[k][idxw];
            d[3]=tw->Data[k][idxwj];
         } else {
            d[1]=t->Data[k][idx+1];
            d[3]=t->Data[k][idxj+1];
         }

         Value[ik]=d[0] + (d[1]-d[0])*dx + (d[2]-d[0])*dy + (d[3]-d[1]-d[2]+d[0])*dxy;
      }

      ik++;
//         RPN_IntLock();
//         c_gdxysval(t->GID,&Value[ik++],t->Data[k],&I,&J,1);
//         RPN_IntUnlock();
   } while ((K0<=K1?k++:k--)!=K1);

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
 *   <Mode>       : Interpolarion mode (EZ_NEAREST,EZ_LINEAR)
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
int32_t f77name(ezgrid_ijgetuvvalue)(int32_t *gdidu,int32_t *gdidv,int32_t *mode,float *i,float *j,int32_t *k0,int32_t *k1,float *uu,float *vv,float *conv) {
   return(EZGrid_IJGetUVValue(GridCache[*gdidu],GridCache[*gdidv],*mode,*i-1,*j-1,*k0-1,*k1-1,uu,vv,*conv));
}
int EZGrid_IJGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,float I,float J,int K0,int K1,float* restrict UU,float* restrict VV,float Conv) {

   TGridTile *tu,*tv;
   double     d,v;
   int        ik=0,k;

   if (!GridU || !GridV || GridU->GID<0 || GridV->GID<0) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   // Check inclusion in master grid limits
   if (I<0 || J<0 || K0<0 || K1<0 || I>GridU->H.NI-0.5 || J>GridU->H.NJ-0.5 || K0>=GridU->H.NK || K1>=GridU->H.NK) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s,%s): I(%f) J(%f) K(%i,%i)\n",__func__,GridU->H.NOMVAR,GridV->H.NOMVAR,I,J,K0,K1);
      return(FALSE);
   }

   if (Mode==EZ_NEAREST) {
      I=ROUND(I);
      J=ROUND(J);
   }

   if (!(tu=EZGrid_TileGet(GridU,I,J))) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Tile not found (%s) I(%f) J(%f)\n",__func__,GridU->H.NOMVAR,I,J);
      return(FALSE);
   }
   if (!(tv=EZGrid_TileGet(GridV,I,J))) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Tile not found (%s) I(%f) J(%f)\n",__func__,GridV->H.NOMVAR,I,J);
      return(FALSE);
   }

   // Have to readjust coordinate within tile for ezscint
   I=I-tu->HI+1.0;
   J=J-tu->HJ+1.0;

   k=K0;
   do {
      if (!EZGrid_IsLoaded(tu,k))
         EZGrid_TileGetData(GridU,tu,k);
      if (!EZGrid_IsLoaded(tv,k))
         EZGrid_TileGetData(GridV,tv,k);

//      RPN_IntLock();
      c_gdxywdval(tu->GID,&UU[ik],&VV[ik],tu->Data[k],tv->Data[k],&I,&J,1);
//      RPN_IntUnlock();

      d=DEG2RAD(VV[ik]);
      v=UU[ik]*Conv;

      UU[ik]=-v*sin(d);
      VV[ik]=-v*cos(d);
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
int32_t f77name(ezgrid_getvalue)(int32_t *gdid,int32_t *i,int32_t *j,int32_t *k0,int32_t *k1,float *val) {
   return(EZGrid_GetValue(GridCache[*gdid],*i-1,*j-1,*k0-1,*k1-1,val));
}
int EZGrid_GetValue(const TGrid* restrict const Grid,int I,int J,int K0,int K1,float* restrict Value) {

   TGridTile *t;
   int        k,ik=0;

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   // Check inclusion in master grid limits
   if (I<0 || J<0 || K0<0 || K1<0 || I>=Grid->H.NI-0.5 || J>=Grid->H.NJ-0.5 || K0>=Grid->H.NK || K1>=Grid->H.NK) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s) I(%i) J(%i) K(%i,%i)\n",__func__,Grid->H.NOMVAR,I,J,K0,K1);
      return(FALSE);
   }

   if (!(t=EZGrid_TileGet(Grid,I,J))) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Tile not found (%s) I(%i) J(%i)\n",__func__,Grid->H.NOMVAR,I,J);
      return(FALSE);
   }

   k=K0;

   do {
      if (!EZGrid_IsLoaded(t,k))
         EZGrid_TileGetData(Grid,t,k);

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
int32_t f77name(ezgrid_getvalues)(int32_t *gdid,int32_t *nb,float *i,float *j,float *k,float *val) {
   return(EZGrid_GetValues(GridCache[*gdid],-(*nb),i,j,k,val));
}
int EZGrid_GetValues(const TGrid* restrict const Grid,int Nb,float* restrict const I,float* restrict const J,float* restrict const K,float* restrict Value) {

   TGridTile *t;
   int        n,i,j,k;

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
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

      // Check inclusion in master grid limits
      if (i<0 || j<0 || k<0 || i>=Grid->H.NI || j>=Grid->H.NJ || k>=Grid->H.NK) {
         Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): I(%i) J(%i) K(%i)\n",__func__,Grid->H.NOMVAR,i,j,k);
         return(FALSE);
      }

      if (!(t=EZGrid_TileGet(Grid,i,j))) {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: Tile not found (%s) I(%i) J(%i)\n",__func__,Grid->H.NOMVAR,i,j);
         return(FALSE);
      }
      if (!EZGrid_IsLoaded(t,k))
         EZGrid_TileGetData(Grid,t,k);

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
int f77name(ezgrid_getarray)(int32_t *gdid,int32_t *k,float *vals) {
   return(EZGrid_GetArray(GridCache[*gdid],*k-1,vals));
}
int EZGrid_GetArray(TGrid* restrict const Grid,int K,float* restrict Value) {

   float *data;

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   // Check inclusion in master grid limits
   if (K<0 || K>=Grid->H.NK) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): K(%i)\n",__func__,Grid->H.NOMVAR,K);
      return(FALSE);
   }
   data=EZGrid_TileBurnAll(Grid,K,NULL);
   memcpy(Value,data,Grid->H.NIJ*sizeof(float));

   return(TRUE);
}

float* EZGrid_GetArrayPtr(TGrid* restrict const Grid,int K) {

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   // Check inclusion in master grid limits
   if (K<0 || K>=Grid->H.NK) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): K(%i)\n",__func__,Grid->H.NOMVAR,K);
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
int32_t f77name(ezgrid_getrange)(int32_t *gdid,int32_t *i0,int32_t *j0,int32_t *k0,int32_t *i1,int32_t *j1,int32_t *k1,float *val) {
   return(EZGrid_GetRange(GridCache[*gdid],*i0-1,*j0-1,*k0-1,*i1-1,*j1-1,*k1-1,val));
}
int EZGrid_GetRange(const TGrid* restrict const Grid,int I0,int J0,int K0,int I1,int J1,int K1,float* restrict Value) {

   TGridTile *t;
   int        ik=0;
   int        i,j,k;

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   // Check inclusion in master grid limits
   if (I0<0 || J0<0 || K0<0 || I0>Grid->H.NI-1 || J0>Grid->H.NJ-1 || K0>=Grid->H.NK ||
       I1<0 || J1<0 || K1<0 || I1>Grid->H.NI-1 || J1>Grid->H.NJ-1 || K1>=Grid->H.NK) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): I(%i,%i) J(%i,%i) K(%i,%i)\n",__func__,Grid->H.NOMVAR,I0,I1,J0,J1,K0,K1);
      return(FALSE);
   }

   // Loop on coverage
   for(k=K0;k<=K1;k++) {
      for(j=J0;j<=J1;j++) {
         for(i=I0;i<=I1;i++) {
            if (!(t=EZGrid_TileGet(Grid,i,j))) {
               Lib_Log(APP_LIBEER,APP_ERROR,"%s: Tile not found (%s) I(%i) J(%i)\n",__func__,Grid->H.NOMVAR,i,j);
               return(FALSE);
            }
            if (!EZGrid_IsLoaded(t,k))
               EZGrid_TileGetData(Grid,t,k);

            Value[ik++]=EZGrid_TileValue(t,i,j,k);
         }
      }
   }
   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetDims>
 * Creation : Avril 2010 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les valeurs de distance en X et Y ainsi que l'aire
 *            pour chaque cellule de la grille
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Invert>     : Invert (1/area)
 *   <DX>         : Valeurs de distance en X
 *   <DY>         : Valeurs de distance en Y
 *   <DA>         : Valeurs de l'aire
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *    - Si un des tableau est NULL, il ne sera pas remplie
 *----------------------------------------------------------------------------
*/
int f77name(ezgrid_getdims)(int32_t *gdid,int32_t *k,float *dx,float *dy,float *da) {
   return(EZGrid_GetDims(GridCache[*gdid],*k-1,dx,dy,da));
}

int EZGrid_GetDims(TGrid* restrict const Grid,int Invert,float* DX,float* DY,float* DA) {

   return(GeoRef_CellDims(Grid->GRef,Invert,DX,DY,DA));

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
int EZGrid_GetLL(TGrid* restrict const Grid,float* Lat,float* Lon,float* I,float* J,int Nb) {

   int i,ok=0;
   double la,lo;
   float fi,fj;

   if (Grid && Grid->GID>=0) {
   //   RPN_IntLock();
      for(i=0;i<Nb;i++) {
         fi=I[i]+1.0;
         fj=J[i]+1.0;
         if ((ok=c_gdllfxy(Grid->GID,&Lat[i],&Lon[i],&fi,&fj,1))<0) {
            break;
         }
      }
   //   RPN_IntUnlock();
   } else {
      for(i=0;i<Nb;i++) {
         Grid->GRef->Project(Grid->GRef,I[i],J[i],&la,&lo,FALSE,TRUE);   
         Lon[i]=lo;
         Lat[i]=la;
      }    
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
int EZGrid_GetIJ(TGrid* restrict const Grid,float* Lat,float* Lon,float* I,float* J,int Nb) {

   int i,ok=0;
   double x,y;
   
   if (Grid && Grid->GID>=0) {
  //   RPN_IntLock();
      if (c_gdxyfll(Grid->GID,I,J,Lat,Lon,Nb)!=0) {
         return(FALSE);
      }
   //   RPN_IntUnlock();

      for(i=0;i<Nb;i++) {
         I[i]-=1.0;
         J[i]-=1.0;
      }
   } else {
      for(i=0;i<Nb;i++) {
         if (!Grid->GRef->UnProject(Grid->GRef,&x,&y,Lat[i],Lon[i],FALSE,TRUE)) {
            return(FALSE);
         }
         I[i]=x;
         J[i]=y;
      }    
   }
   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetBary>
 * Creation : Octobre 2015 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir les coordonnees barycentrique et index associe a partir de latlon
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Lat>        : Latitudes
 *   <Lon>        : Longitudes
 *   <Bary>       : Coordonne barycentrique
 *   <Index>      : Index des vertices associees aux coordonnees (Optionnel)
 *
 * Retour:
 *   <int>       : Code d'erreur (0=erreur, 1=ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_GetBary(TGrid* restrict const Grid,float Lat,float Lon,Vect3d Bary,Vect3i Index) {

   TQTree       *node;
   unsigned int *idx,n;
   intptr_t     t;
   
   // Is this a triangle mesh
   if (Grid && Grid->H.GRTYP[0]=='M') {
      idx=Grid->GRef->Idx;
      CLAMPLON(Lon);
      
      // Find enclosing triangle
      if ((node=QTree_Find(Grid->GRef->QTree,Lon,Lat)) && node->NbData) {
         
         // Loop on this nodes data payload
         for(n=0;n<node->NbData;n++) {
            t=(intptr_t)node->Data[n].Ptr-1; // Remove false pointer increment
            
            // if the Barycentric coordinates are within this triangle, get its interpolated value
            if (Bary_Get(Bary,Grid->GRef->Wght?Grid->GRef->Wght[t/3]:0.0,Lon,Lat,Grid->GRef->AX[idx[t]],Grid->GRef->AY[idx[t]],Grid->GRef->AX[idx[t+1]],Grid->GRef->AY[idx[t+1]],Grid->GRef->AX[idx[t+2]],Grid->GRef->AY[idx[t+2]])) {
               if (Index) {
                  Index[0]=idx[t];                    
                  Index[1]=idx[t+1];                    
                  Index[2]=idx[t+2];                    
               }
               return(TRUE);
            }
         }
      }
   }
   
   // We must be out of the tin
   return(FALSE);
}

#endif
