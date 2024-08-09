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
static TGridDef       *GridCache[EZGRID_CACHEMAX];              // Grid cache list
static __thread int    MIdx=-1;                                 // Cached previously used mesh index

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Wrap>
 * Creation : Mai 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Tester si une grille est globale (wrap-around)
 *
 * Parametres :
 *   <GDef>   : Définition de grille
 *
 * Retour:
 *  <Bool>   : (0:no wrap, 1: wrap)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
static int EZGrid_Wrap(TGridDef* restrict const GDef) {

   float i,j,lat,lon;

   GDef->Wrap=0;
   GDef->Pole[0]=GDef->Pole[1]=0.0f;

   if (GDef->GRTYP[0]!='M' && GDef->GRTYP[0]!='W' && GDef->GRTYP[1]!='W' && GDef->GRTYP[0]!='Y') {

   //   RPN_IntLock();
      // Check for south pole coverage
      i=1.0;j=1.0;
      c_gdllfxy(GDef->GID,&lat,&lon,&i,&j,1);
      if (lat==-90.0) {
         i=1.0;j=1.5;
         c_gdllfxy(GDef->GID,&GDef->Pole[0],&lon,&i,&j,1);
      }

      // Check for north pole coverage
      i=1.0;j=GDef->NJ;
      c_gdllfxy(GDef->GID,&lat,&lon,&i,&j,1);
      if (lat==90.0) {
         i=1.0;j=GDef->NJ-0.5;
         c_gdllfxy(GDef->GID,&GDef->Pole[1],&lon,&i,&j,1);
      }

      i=GDef->NI+1.0f;
      j=GDef->NJ/2.0f;
      c_gdllfxy(GDef->GID,&lat,&lon,&i,&j,1);
      c_gdxyfll(GDef->GID,&i,&j,&lat,&lon,1);

   //   RPN_IntUnlock();

      if (GDef->GRTYP[0]=='A' || GDef->GRTYP[0]=='B' || GDef->GRTYP[0]=='G') {
         GDef->Wrap=1;
      } else {
         // If the grid wraps
         if (i<GDef->NI) {
            // check if the last gridpoint is a repeat of the first
            if (lrintf(i)==1) {
               // No repeat
               GDef->Wrap=1;
            } else {
               // First gridpoint is repeated at end
               GDef->Wrap=2;
            }
         }
      }
   }

   return(GDef->Wrap);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetData>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Recuperer les donnees d'une tuile
 *
 * Parametres :
 *   <Grid>      : Grille
 *   <K>         : Niveau à lire
 *
 * Retour: APP_OK si OK, APP_ERR sinon
 *
 * Remarques :
 *      - Cette fonction implemente le read on demand et interpolate on demand
 *      - Les donnees sont lues dans le fichier standard ou interpollee
 *        entre deux champs si necessaire
 *      - On utilise des variables (data et datak) temporaire pour les allocations
 *        afin de limiter le nombre de mutex lock
 *----------------------------------------------------------------------------
*/
int EZGrid_GetData(TGrid* restrict Grid,int K) {
   TRPNHeader  h={0};
   TGridDef    *gdef=Grid->GDef;
   void        *buf=NULL;
   int         key,code=APP_ERR,locked=0;
   int         i,ni,nj,nk,t=0;
   int         *tmpi,flag=0,ip1=0,type;
   int         idx,idxt,n,ntiles;
   char        format;

   if (K<0 || K>=gdef->ZRef->LevelNb) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Invalid level (%s): K(%d) NK(%d)\n",__func__,Grid->H.NOMVAR,K,gdef->ZRef->LevelNb);
      return APP_ERR;
   }

   pthread_mutex_lock(&Grid->Mutex);

   if( !EZGrid_IsLoaded(Grid,K) ) {
      // Allocate levels data if not already done
      if( !Grid->Data ) {
         if( !(Grid->Data=calloc(gdef->ZRef->LevelNb,sizeof(*Grid->Data))) ) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for grid data levels (%s)\n",__func__,Grid->H.NOMVAR);
            goto end;
         }
      }

      // Allocate K level data if not already done
      if( !Grid->Data[K] ) {
         if( !(Grid->Data[K]=calloc(gdef->NIJ,sizeof(*Grid->Data[K]))) ) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for grid data level %d (%s)\n",__func__,K,Grid->H.NOMVAR);
            goto end;
         }
      }

      if( Grid->H.FID>=0 ) {
         ip1 = gdef->ZRef->LevelNb>1 ? ZRef_Level2IP(gdef->ZRef->Levels[K],gdef->ZRef->Type,gdef->ZRef->Style) : -1;

         RPN_FieldLock();
         locked=1;

         // Find the field
         if( (key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,Grid->H.DATEV,Grid->H.ETIKET,ip1,Grid->H.IP2,-1,Grid->H.TYPVAR,Grid->H.NOMVAR)) < 0 ) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not find field (%s) at level %f (%i)\n",__func__,Grid->H.NOMVAR,gdef->ZRef->Levels[K],ip1);
            goto end;
         }

         if( !gdef->NbTiles ) {
            // Not a tiled field, just read it
            if( RPN_ReadData(Grid->Data[K],TD_Float32,key) != APP_OK ) {
               Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not read field (%s) at level %f (%i)\n",__func__,Grid->H.NOMVAR,gdef->ZRef->Levels[K],ip1);
               goto end;
            }
         } else { // We have a tiled field
            // Allocate temp buffer that will hold the tile data
            APP_MEM_ASRT_END( buf,malloc(gdef->NIJ*sizeof(*Grid->Data[K])) );

            // Loop on the remaining fields
            ntiles=0;
            while( key >= 0 ) {
               // We need IG3 and IG4 for the grid position (note that the grid position starts at 1)
               cs_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
                     h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);

               // Read the field
               if( RPN_ReadData(buf,TD_Float32,key) != APP_OK ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not read field (%s) tile (%d) at level %f (%i)\n",__func__,Grid->H.NOMVAR,gdef->ZRef->Levels[K],ip1);
                  goto end;
               }

               // Put the data in the master grid
               idx   = (h.IG4-1)*gdef->NI + (h.IG3-1);
               idxt  = (h.IG4==1?gdef->Halo:0)*h.NI + (h.IG3==1?gdef->Halo:0);
               ni    = h.NI - (h.IG3==1?0:gdef->Halo) - (h.IP3%gdef->NTI?gdef->Halo:0);
               nj    = h.NJ - (h.IG4==1?0:gdef->Halo) - (h.IP3>(gdef->NTJ-1)*gdef->NTI?0:gdef->Halo);
               for(; nj; --nj,idx+=gdef->NI,idxt+=h.NI) {
                  memcpy(&Grid->Data[K][idx],(__typeof(Grid->Data[K]))buf+idxt,ni*sizeof(*Grid->Data[K]));
               }

               // Get the next tile
               ++ntiles;
               key = c_fstsui(Grid->H.FID,&ni,&nj,&nk);
            }

            if( ntiles != gdef->NbTiles ) {
               Lib_Log(APP_LIBEER,APP_ERROR,"%s: The number of tiles read (%d) is different then the number of expected tiles by the master grid (%d) for field (%s)\n",__func__,ntiles,gdef->NbTiles,Grid->H.NOMVAR);
               goto end;
            }
         }

         // Apply Factor if needed
         if( Grid->Factor!=1.0f ) {
            for(idx=0; idx<gdef->NIJ; ++idx)
               Grid->Data[K][idx] *= Grid->Factor;
         }

         // Check for mask (TYPVAR==@@)
         if( Grid->H.TYPVAR[1]=='@' ) {
            // Find the field
            if( (key=c_fstinf(Grid->H.FID,&ni,&nj,&nk,Grid->H.DATEV,Grid->H.ETIKET,ip1,Grid->H.IP2,-1,"@@",Grid->H.NOMVAR)) >= 0 ) {
               // Allocate mask data levels if not already done
               if( !Grid->Mask ) {
                  if( !(Grid->Mask=calloc(gdef->ZRef->LevelNb,sizeof(*Grid->Mask))) ) {
                     Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for mask levels (%s)\n",__func__,Grid->H.NOMVAR);
                     goto end;
                  }
               }

               // Allocate mask data for level K if not already done
               if( !Grid->Mask[K] ) {
                  if( !(Grid->Mask[K]=malloc(ni*nj*sizeof(*Grid->Mask[K]))) ) {
                     Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for mask level %d (%s)\n",__func__,K,Grid->H.NOMVAR);
                     goto end;
                  }
               }

               if( !gdef->NbTiles ) {
                  // Not a tiled mask, just read it
                  if( RPN_ReadData(Grid->Mask[K],TD_Byte,key) != APP_OK ) {
                     Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to read mask (%s)\n",__func__,Grid->H.NOMVAR);
                     goto end;
                  }
               } else { // We have a tiled mask
                  // Allocate temp buffer that will hold the tile data
                  if( !buf )
                     APP_MEM_ASRT_END( buf,malloc(gdef->NIJ*sizeof(*Grid->Mask[K])) );

                  // Loop on the remaining fields
                  ntiles=0;
                  while( key >= 0 ) {
                     // We need IG3 and IG4 for the grid position (note that the grid position starts at 1)
                     cs_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
                           h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);

                     // Read the field
                     if( RPN_ReadData(buf,TD_Byte,key) != APP_OK ) {
                        Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not read mask (%s) tile (%d) at level %f (%i)\n",__func__,Grid->H.NOMVAR,gdef->ZRef->Levels[K],ip1);
                        goto end;
                     }

                     // Put the data in the master grid
                     idx   = (h.IG4-1)*gdef->NI + (h.IG3-1);
                     idxt  = (h.IG4==1?gdef->Halo:0)*h.NI + (h.IG3==1?gdef->Halo:0);
                     ni    = h.NI - (h.IG3==1?0:gdef->Halo) - (h.IP3%gdef->NTI?gdef->Halo:0);
                     nj    = h.NJ - (h.IG4==1?0:gdef->Halo) - (h.IP3>(gdef->NTJ-1)*gdef->NTI?0:gdef->Halo);
                     for(; nj; --nj,idx+=gdef->NI,idxt+=h.NI) {
                        memcpy(&Grid->Mask[K][idx],(__typeof(Grid->Mask[K]))buf+idxt,ni*sizeof(*Grid->Mask[K]));
                     }

                     // Get the next tile
                     ++ntiles;
                     key = c_fstsui(Grid->H.FID,&ni,&nj,&nk);
                  }

                  if( ntiles != gdef->NbTiles ) {
                     Lib_Log(APP_LIBEER,APP_ERROR,"%s: The number of tiles read (%d) is different then the number of expected tiles by the master grid (%d) for field (%s)\n",__func__,ntiles,gdef->NbTiles,Grid->H.NOMVAR);
                     goto end;
                  }
               }
            } else {
               Lib_Log(APP_LIBEER,APP_WARNING,"%s: Could not find field mask (%s) at level %f (%i)\n",__func__,Grid->H.NOMVAR,gdef->ZRef->Levels[K],ip1);
            }
         }
      } else if( Grid->T0 && Grid->T1 ) {  // Check for time interpolation needs
         // Make sure the data from the needed tile is loaded
         if( EZGrid_GetData(Grid->T0,K) != APP_OK ) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to get data for grid T0 (%s)\n",__func__,Grid->H.NOMVAR);
            goto end;
         }
         if( EZGrid_GetData(Grid->T1,K) != APP_OK ) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to get data for grid T1 (%s)\n",__func__,Grid->H.NOMVAR);
            goto end;
         }

         // Interpolate between by applying factors
         Grid->Mask = Grid->T0->Mask;
         for(i=0; i<gdef->NIJ; ++i) {
            // Note: it is not clear whether we should keep factor multiplication here.
            // On the one hand, it is needed if the factor is only set on the field that has T0/T1 set,
            // on the other, it shouldn't be here if the fields T0 and T1 both have the factor set as well
            Grid->Data[K][i] = (Grid->T0->Data[K][i]*Grid->FT0 + Grid->T1->Data[K][i]*Grid->FT1) * Grid->Factor;
         }
      } else {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid field; can't read nor interpolate (%s) at level %d\n",__func__,Grid->H.NOMVAR,K);
         goto end;
      }
   }

   code = APP_OK;
end:
   if( locked ) {
      RPN_FieldUnlock();
   }

   pthread_mutex_unlock(&Grid->Mutex);

   if( buf ) {
      free(buf);
   }

   return code;
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_CacheFind>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir la grille "template" correspondant a la grille specifiee
 *
 * Parametres :
 *   <H>         : Header décrivant la définition de grille à rechercher dans la cache
 *
 * Retour:
 *  <TGridDef*>  : Définition de grille (ou NULL si non trouvée)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
static TGridDef* EZGrid_CacheFind(const TRPNHeader *H) {
   int   n,k;
   int   type;
   float level;

   if( H ) {
      pthread_mutex_lock(&CacheMutex);
      for(n=0; n<EZGRID_CACHEMAX && GridCache[n]; ++n) {
         // Check for same dimensions
         if( GridCache[n]->NI!=H->NI || GridCache[n]->NJ!=H->NJ || GridCache[n]->ZRef->LevelNb!=H->NK ) {
            continue;
         }
         // Check for same level type and definitions
         level = ZRef_IP2Level(H->IP1,&type);
         type = type==LVL_SIGMA ? LVL_ETA : type;
         if( type!=GridCache[n]->ZRef->Type ) {
            continue;
         }
         // Make sure we can find the level in our level list
         for(k=0; k<GridCache[n]->ZRef->LevelNb; ++k) if( GridCache[n]->ZRef->Levels[k]==level ) break;
         if( k == GridCache[n]->ZRef->LevelNb )
            continue;

         // Make sure we have the same type of grid
         if( GridCache[n]->GRTYP[0]!=H->GRTYP[0] && (H->GRTYP[0]!='#' || GridCache[n]->GRTYP[0]!='Z') ) {
            continue;
         }

         // Check for same grid descriptors
         switch( H->GRTYP[0] ) {
            case '#':   if( GridCache[n]->IG1!=H->IG1 || GridCache[n]->IG2!=H->IG2 ) continue;
            case 'Z':
            case 'M':
            case 'Y':
            case 'O':
            case 'X':
            case 'W':   if( GridCache[n]->IG1!=H->IG1 || GridCache[n]->IG2!=H->IG2 || GridCache[n]->IG3!=H->IG3 ) continue;
            default:    if( GridCache[n]->IG1!=H->IG1 || GridCache[n]->IG2!=H->IG2 || GridCache[n]->IG3!=H->IG3 || GridCache[n]->IG4!=H->IG4 ) continue;
         }

         // At this point, we consider this to be the right grid definition
         pthread_mutex_unlock(&CacheMutex);
         return GridCache[n];
      }
      pthread_mutex_unlock(&CacheMutex);
   }

   return NULL;
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_CacheAdd>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Ajoute la grille specifiee au tableau de grilles en "cache"
 *
 * Parametres :
 *   <GDef>      : Définition de grille à ajouter à la cache
 *
 * Retour:
 *  <int>        : Index de l'emplacement de l'ajout (ou -1 si erreur)
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
static int EZGrid_CacheAdd(TGridDef* restrict const GDef) {
   int n,i=-1;

   if( GDef ) {
      pthread_mutex_lock(&CacheMutex);
      for(n=0; n<EZGRID_CACHEMAX; ++n) {
         if( !GridCache[n] ) {
            GridCache[n] = GDef;
            i = n;
            break;
         }
      }
      pthread_mutex_unlock(&CacheMutex);
   }

   return i;
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
 *   <NBits>     : Packing (0 means use header's NBITS)
 *   <Overwrite> : Reecrie sur les champs existant
 *
 * Retour:
 *  <int>        : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_Write(int FId,TGrid* restrict const Grid,int NBits,int Overwrite) {
   int k,key,ip1;

   if (!Grid)
      return APP_ERR;

   ip1=Grid->H.IP1;

   for(k=0;k<Grid->GDef->ZRef->LevelNb;k++) {
      if (Grid->GDef->ZRef->LevelNb>1) {
         ip1=ZRef_Level2IP(Grid->GDef->ZRef->Levels[k],Grid->GDef->ZRef->Type,DEFAULT);
      }

      key=cs_fstecr(Grid->Data[k],NBits?-NBits:-Grid->H.NBITS,FId,Grid->H.DATEO,Grid->H.DEET,Grid->H.NPAS,Grid->GDef->NI,Grid->GDef->NJ,1,ip1,Grid->H.IP2,Grid->H.IP3,
            (char*)Grid->H.TYPVAR,(char*)Grid->H.NOMVAR,(char*)Grid->H.ETIKET,Grid->H.GRTYP,Grid->GDef->IG1,Grid->GDef->IG2,Grid->GDef->IG3,Grid->GDef->IG4,Grid->H.DATYP,Overwrite);

      if( key < 0 ) {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to write field (%s) at level (%d)\n",__func__,Grid->H.NOMVAR,k);
         return APP_ERR;
      }
   }

   return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Get>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Lire la definition de grille dans le fichier
 *
 * Parametres :
 *   <GDef>      : Définition de grille
 *   <H>         : Header du champ dont ont veut la définition de grille
 *   <Incr>      : Ordre de tri des niveaux (IP1) (-1=decroissant, 0=1 seul niveau, 1=croissant)
 *
 * Retour:
 *  <int>       : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *
 *    - On construit la structure de donnee de la grille sans lire les donnees elle-meme
 *    - On lit toutes les descriptions de tuiles et on en deduit le nombre en I et J et les halos
 *    - On lit tous les niveau disponibles
 *----------------------------------------------------------------------------
*/
static int EZGrid_Get(TGridDef* restrict const GDef,const TRPNHeader *restrict H,int Incr) {
   int n,ni,nj,nk,key;

   // Get vertical reference
   if( !(GDef->ZRef=ZRef_New()) ) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not allocate memory for ZRef\n",__func__);
      return APP_ERR;
   }
   // Get levels
   ZRef_GetLevels(GDef->ZRef,H,Incr);
   // Decode vertical coordinate parameters
   ZRef_DecodeRPN(GDef->ZRef,H->FID);
   // Force sigma to eta
   GDef->ZRef->Type = GDef->ZRef->Type==LVL_SIGMA ? LVL_ETA : GDef->ZRef->Type;

   // Initialize the rest of the structure
   GDef->GRef     = NULL;
   GDef->NI       = H->NI;
   GDef->NJ       = H->NJ;
   GDef->NIJ      = GDef->NI*GDef->NJ;
   GDef->IG1      = H->IG1;
   GDef->IG2      = H->IG2;
   GDef->IG3      = H->IG3;
   GDef->IG4      = H->IG4;
   GDef->GID      = -1;
   GDef->Wrap     = 0;
   GDef->Pole[0]  = 0.0f;
   GDef->Pole[1]  = 0.0f;
   GDef->NbTiles  = 0;
   GDef->NTI      = 0;
   GDef->NTJ      = 0;
   GDef->Halo     = 0;
   GDef->GRTYP[0] = H->GRTYP[0];
   GDef->GRTYP[1] = '\0';


   // Check if we have a tiled field
   if( H->GRTYP[0]=='#' ) {
      TRPNHeader h;
      memset(&h,0,sizeof(TRPNHeader));

      // Check for master grid descriptor and adjust the header for the master grid dimensions
      if(      cs_fstinf(H->FID,&GDef->NI,&h.NJ,&h.NK,-1,"",H->IG1,H->IG2,-1,"",">>") < 0
            || cs_fstinf(H->FID,&h.NI,&GDef->NJ,&h.NK,-1,"",H->IG1,H->IG2,-1,"","^^") < 0 ) {
         Lib_Log(APP_LIBEER,APP_WARNING,"%s: Could not find master grid descriptor (>>,^^)\n",__func__);
         return APP_ERR;
      }

      // Change the grid definition to the untiled version of the grid
      GDef->NIJ      = GDef->NI*GDef->NJ;
      GDef->GRTYP[0] = 'Z';
      GDef->IG3      = 0;
      GDef->IG4      = 0;

      // Needed for halo calculations
      ni = 0;

      // Loop on all the tiles
      key=cs_fstinf(H->FID,&h.NI,&h.NJ,&h.NK,H->DATEV,(char*)H->ETIKET,H->IP1,H->IP2,-1,(char*)H->TYPVAR,(char*)H->NOMVAR);
      while( key >= 0 ) {
         ++GDef->NbTiles;

         // We need IG3 and IG4 for the grid position
         cs_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
               h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);

         // Add to tile count if first row or first column
         if( h.IG3==1 ) { ++GDef->NTJ; }
         if( h.IG4==1 ) { ++GDef->NTI; ni+=h.NI; }

         // Get the next tile
         key = cs_fstsui(H->FID,&h.NI,&h.NJ,&h.NK);
      }

      // Is there a halo around the tiles
      if( ni>H->NI ) {
         // Calculate halo width
         // Note: there is a halo on either side of each tile but not in the sides of the grid
         GDef->Halo = (ni-H->NI)/(GDef->NTI*2-2);
      }
   }

   // Create master grid
   switch( GDef->GRTYP[0] ) {
      case 'M':
         GDef->GRef = GeoRef_RPNSetup(GDef->NI,GDef->NJ,GDef->GRTYP,GDef->IG1,GDef->IG2,GDef->IG3,GDef->IG4,H->FID);

         cs_fstinf(H->FID,&ni,&nj,&nk,-1,"",GDef->IG1,GDef->IG2,GDef->IG3,"","##");
         GDef->GRef->NIdx = ni*nj*nk;
         GDef->GRef->Idx = malloc(GDef->GRef->NIdx*sizeof(unsigned int));
         GDef->GRef->AY = malloc(GDef->GRef->NX*sizeof(float));
         GDef->GRef->AX = malloc(GDef->GRef->NX*sizeof(float));

         RPN_sRead(GDef->GRef->AY,TD_Float32,H->FID,&ni,&nj,&nk,-1,"",GDef->IG1,GDef->IG2,GDef->IG3,"","^^");
         RPN_sRead(GDef->GRef->AX,TD_Float32,H->FID,&ni,&nj,&nk,-1,"",GDef->IG1,GDef->IG2,GDef->IG3,"",">>");
         RPN_sRead(GDef->GRef->Idx,TD_UInt32,H->FID,&ni,&nj,&nk,-1,"",GDef->IG1,GDef->IG2,GDef->IG3,"","##");

         GeoRef_BuildIndex(GDef->GRef);
         break;

      case 'X':
      case 'Y':
      case 'O':
         GDef->GRef = GeoRef_RPNSetup(GDef->NI,GDef->NJ,GDef->GRTYP,GDef->IG1,GDef->IG2,GDef->IG3,GDef->IG4,H->FID);

         GDef->GRef->AY = malloc(GDef->NIJ*sizeof(float));
         GDef->GRef->AX = malloc(GDef->NIJ*sizeof(float));

         RPN_sRead(GDef->GRef->AY,TD_Float32,H->FID,&ni,&nj,&nk,-1,"",GDef->IG1,GDef->IG2,GDef->IG3,"","^^");
         RPN_sRead(GDef->GRef->AX,TD_Float32,H->FID,&ni,&nj,&nk,-1,"",GDef->IG1,GDef->IG2,GDef->IG3,"",">>");

         GeoRef_BuildIndex(GDef->GRef);
         break;

      case 'Z':
         {
            int tmpi,desc,key,ig1,ig2,ig3,ni,nj,nk;
            char grtyp[2]={'\0'},tmpc[13]={'\0'};

            // Get the X descriptor
            if( (desc=cs_fstinf(H->FID,&ni,&nj,&tmpi,-1,"",GDef->IG1,GDef->IG2,GDef->IG3,"",">>")) <=0 ) {
               Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not find grid descriptor (>>) of Z grid\n",__func__);
               return APP_ERR;
            }
            if( cs_fstprm(desc,&ni,&nj,&nk,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,
                  tmpc,tmpc,tmpc,grtyp,&ig1,&ig2,&ig3,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi,&tmpi) ) {
               Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not stat grid descriptor (>>) of Z grid\n",__func__);
               return APP_ERR;
            }

            // If we have a W grtyp, then we are dealing with a WKT projection
            if( grtyp[0] == 'W' ) {
               double transform[6];
               char *str=NULL;

               GDef->GRTYP[1] = 'W';

               // Read the transformation matrix
               if( (key=cs_fstinf(H->FID,&ni,&nj,&nk,-1,"",ig1,ig2,ig3,"","MTRX")) <=0 ) {
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
               if( (key=cs_fstinf(H->FID,&ni,&nj,&nk,-1,"",ig1,ig2,ig3,"","PROJ")) <=0 ) {
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
               GDef->GRef = GeoRef_WKTSetup(GDef->NI,GDef->NJ,GDef->GRTYP,GDef->IG1,GDef->IG2,GDef->IG3,GDef->IG4,str,transform,NULL,NULL);

               // Memory for the descriptors
               if( !(GDef->GRef->AX=malloc(GDef->NIJ*sizeof(*GDef->GRef->AX))) ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not allocate memory for descriptor (>>) of Z grid\n",__func__);
                  goto werr;
               }
               if( !(GDef->GRef->AY=malloc(GDef->NIJ*sizeof(*GDef->GRef->AY))) ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not allocate memory for descriptor (^^) of Z grid\n",__func__);
                  goto werr;
               }

               // Read the descriptors
               if( RPN_sReadData(GDef->GRef->AX,TD_Float32,desc) != APP_OK ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not read grid descriptor (>>) of Z grid\n",__func__);
                  goto werr;
               }
               if( RPN_sRead(GDef->GRef->AY,TD_Float32,H->FID,&ni,&nj,&nk,-1,"",GDef->IG1,GDef->IG2,GDef->IG3,"","^^") != APP_OK ) {
                  Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not find grid descriptor (^^) of Z grid\n",__func__);
                  goto werr;
               }

               // No need to do the default steps
               free(str);
               break;
werr:
               free(str);
               GeoRef_Free(GDef->GRef); GDef->GRef=NULL;
               return APP_ERR;
            }
            // *** NO BREAK : Fall through to default condition if our Z grid is not on a W
         }

      default:
         GDef->GRef = GeoRef_RPNSetup(GDef->NI,GDef->NJ,GDef->GRTYP,GDef->IG1,GDef->IG2,GDef->IG3,GDef->IG4,H->FID);

         GDef->GID = GDef->GRef->Ids[0];
         GDef->Wrap = EZGrid_Wrap(GDef);
   }
   GeoRef_Qualify(GDef->GRef);

   return APP_OK;
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

   if( (new=(TGrid*)malloc(sizeof(TGrid))) ) {
      pthread_mutex_init(&new->Mutex,NULL);
      new->GDef=NULL;
      new->Data=NULL;
      new->Mask=NULL;
      new->T0=new->T1=NULL;
      new->FT0=new->FT1=0.0f;
      new->Factor=1.0f;

      memset(&new->H,0,sizeof(TRPNHeader));
      new->H.FID=-1;
   }
   return(new);
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_CopyGrid>
 * Creation : Fevrier 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Copier une grille
 *
 * Parametres :
 *  <Master>  : Copy from master (NULL=new empty)
 *  <Level>   : (Level to copy or -1 for all)
 *  <Alloc>   : Force allocation of the memory
 *
 * Retour:
 *  <TGrid>   : Nouvelle Grille (NULL=Error)
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
TGrid *EZGrid_CopyGrid(const TGrid *restrict Master,int Level,int Alloc) {

   TGrid *new=NULL;
   int    n;

   if( Master && (Level<0 || Level<Master->GDef->ZRef->LevelNb) && (new=malloc(sizeof(*new))) ) {
      pthread_mutex_init(&new->Mutex,NULL);
      new->Data=NULL;
      new->Mask=NULL;
      new->T0=new->T1=NULL;
      new->FT0=new->FT1=0.0f;
      new->Factor=1.0f;

      new->H=Master->H;
      new->H.FID=-1;

      if( Level>=0 ) {
         new->H.IP1 = ZRef_Level2IP(Master->GDef->ZRef->Levels[Level],Master->GDef->ZRef->Type,Master->GDef->ZRef->Style);
         new->H.NK = 1;

         // Check if there is already a grid definition with that single level
         if( !(new->GDef=EZGrid_CacheFind(&new->H)) ) {
            if( !(new->GDef=malloc(sizeof(*new->GDef))) ) {
               Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for grid definition\n",__func__);
               EZGrid_Free(new);
               return NULL;
            }
            *new->GDef = *Master->GDef;

            GeoRef_Incr(new->GDef->GRef);

            // The only thing that changes is the ZRef (down to one level)
            new->GDef->ZRef = ZRef_Define(Master->GDef->ZRef->Type,1,Master->GDef->ZRef->Levels+Level);
            new->GDef->ZRef->Style = Master->GDef->ZRef->Style;

            // Add to cache
            EZGrid_CacheAdd(new->GDef);
         }
      } else {
         new->GDef=Master->GDef;
      }

      // Allocate the memory if asked
      if( Alloc ) {
         int k;

         if( !(new->Data=calloc(new->GDef->ZRef->LevelNb,sizeof(*new->Data))) ) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for grid data levels\n",__func__);
            EZGrid_Free(new);
            return NULL;
         }

         for(k=0; k<new->GDef->ZRef->LevelNb; ++k) {
            if( !(new->Data[k]=calloc(new->GDef->NIJ,sizeof(*new->Data[k]))) ) {
               Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for grid data level %d\n",__func__,k);
               EZGrid_Free(new);
               return NULL;
            }
         }

         EZGrid_Clear(new);
      }
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
void EZGrid_Free(TGrid* restrict const Grid) {
   int k;

   if (Grid) {
      // Free data
      if (Grid->Data) {
         for(k=0; k<Grid->GDef->ZRef->LevelNb; ++k) {
            if( Grid->Data[k] ) {
               free(Grid->Data[k]);
            }
         }

         free(Grid->Data);
         Grid->Data=NULL;
      }

      // Free mask
      if( Grid->Mask && !Grid->T0 ) {
         for(k=0; k<Grid->GDef->ZRef->LevelNb; ++k) {
            if( Grid->Mask[k] ) {
               free(Grid->Mask[k]);
            }
         }

         free(Grid->Mask);
         Grid->Mask=NULL;
      }

      // Make sure we don't have dangling pointers
      // This will make sure any program pointing to a freed grid will crash right away
      Grid->GDef = NULL;
      Grid->T0 = NULL;
      Grid->T1 = NULL;

      pthread_mutex_destroy(&Grid->Mutex);

      free(Grid);
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_Clear>
 * Creation : Avril 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Marquer la memoire comme non-initialisee en assignant NAN
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

   // Mark data as not loaded
   if( Grid->Data ) {
      for(k=0; k<Grid->GDef->ZRef->LevelNb; ++k) {
         if( Grid->Data[k] ) {
            Grid->Data[k][0] = f;
         }
      }
   }

   // If grid is interpolated, mask is a reference
   if (Grid->T0) Grid->Mask=NULL;

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
TGrid *EZGrid_ReadIdx(int FId,int Key,int Incr) {

   TRPNHeader h;
   TGrid     *mst,*new;
   int        n,ip3;
   double     nh;
   int        idlst[RPNMAX];

   new=EZGrid_New();
   new->H.FID=FId;

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
   if( !(new->GDef=EZGrid_CacheFind(&new->H)) ) {
      if( !(new->GDef=malloc(sizeof(*new->GDef))) ) {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to allocate memory for grid definition\n",__func__);
         EZGrid_Free(new);
         return NULL;
      }
      if( EZGrid_Get(new->GDef,&new->H,Incr) != APP_OK  ) {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not obtain grid definition (%s)\n",__func__,new->H.NOMVAR);
         EZGrid_Free(new);
         return NULL;
      }

      EZGrid_CacheAdd(new->GDef);
   }

   return new;
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_LoadAll>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Forcer la lecture des donnees 3D de la grille
 *
 * Parametres :
 *   <Grid>      : Grille
 *
 * Retour:
 *  <int>        : APPK_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_LoadAll(TGrid* restrict const Grid) {
   int k=0;

   if (Grid) {
      for(k=0; k<Grid->GDef->ZRef->LevelNb; ++k) {
         APP_ASRT_OK( EZGrid_GetData(Grid,k) );
      }
   }

   return APP_OK;
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
TGrid *EZGrid_InterpTime(TGrid* restrict Grid,TGrid* restrict Grid0,TGrid* restrict Grid1,int Date) {
   double delay,dt;

   /*Figure out the delai between the 2 fields*/
   f77name(difdatr)((int*)&Grid0->H.DATEV,(int*)&Grid1->H.DATEV,&delay);
   f77name(difdatr)((int*)&Grid0->H.DATEV,&Date,&dt);

   dt/=delay;

   return(EZGrid_InterpFactor(Grid,Grid0,Grid1,1.0-dt,dt));
}

void EZGrid_Factor(TGrid* restrict Grid,const float Factor) {
   Grid->Factor=Factor;
}

TGrid *EZGrid_InterpFactor(TGrid* restrict Grid,TGrid* restrict Grid0,TGrid* restrict Grid1,float Factor0,float Factor1) {
   TGrid *new;
   int    i;

   /*Allocate new tile*/
   if (Grid) {
      new=Grid;
   } else {
      new=EZGrid_New();

      new->GDef      = Grid0->GDef;
      new->H         = Grid0->H;
      new->H.FID     = -1;
      new->Factor    = Grid0->Factor;
   }

   new->T0  = Grid0;
   new->T1  = Grid1;
   new->FT0 = Factor0;
   new->FT1 = Factor1;

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
 * Retour: APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_Interp(TGrid* restrict const To, TGrid* restrict const From) {

   int    ok;
   float *from,*to;

   if (!From) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid input grid (%s)\n",__func__,From->H.NOMVAR);
      return APP_ERR;
   }

   if (!To) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid output grid (%s)\n",__func__,To->H.NOMVAR);
      return APP_ERR;
   }

   if( EZGrid_GetData(From,0) != APP_OK ) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Problems with input grid\n",__func__);
      return APP_ERR;
   }
   if( EZGrid_GetData(To,0) != APP_OK ) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Problems with output grid\n",__func__);
      return APP_ERR;
   }

   RPN_IntLock();
   ok=c_ezdefset(To->GDef->GID,From->GDef->GID);
   ok=c_ezsint(To->Data[0],From->Data[0]);
   RPN_IntUnlock();
   if (ok<0)  {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unable to do interpolation (c_ezscint (%i))\n",__func__,ok);
      return APP_ERR;
   }

   return APP_OK;
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
int EZGrid_GetLevelNb(const TGrid* restrict const Grid) {
   return(Grid->GDef->ZRef->LevelNb);
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
int EZGrid_GetLevelType(const TGrid* restrict const Grid) {
   return(Grid->GDef->ZRef->Type);
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
int EZGrid_GetLevels(const TGrid* restrict const Grid,float* restrict Levels,int* restrict Type) {

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   memcpy(Levels,Grid->GDef->ZRef->Levels,Grid->GDef->ZRef->LevelNb*sizeof(float));
   *Type=Grid->GDef->ZRef->Type;

   return(Grid->GDef->ZRef->LevelNb);
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
float EZGrid_GetLevel(const TGrid* restrict const Grid,float Pressure,float P0) {

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(-1.0);
   }

   return(ZRef_Pressure2Level(Grid->GDef->ZRef,P0,Pressure));
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
float EZGrid_GetPressure(const TGrid* restrict const Grid,float Level,float P0,float P0LS) {

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(-1.0);

   }

   return(ZRef_Level2Pressure(Grid->GDef->ZRef,P0,P0LS,Level));
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
 *   <int>       : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *
 *   - On effectue une interpolation lineaire
 *   - Cette fonction permet de recuperer un profile
 *   - Gere les grille Y (Cloud point) et M (Triangle meshes)
 *----------------------------------------------------------------------------
*/
int EZGrid_LLGetValue(TGrid* restrict const Grid,TGridInterpMode Mode,double Lat,double Lon,int K0,int K1,float* restrict Value) {
   float i,j;
   float latf,lonf;

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return APP_ERR;
   }

   if (K0<0 || K0>=Grid->GDef->ZRef->LevelNb || K1<0 || K1>=Grid->GDef->ZRef->LevelNb) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): K(%i,%i)\n",__func__,Grid->H.NOMVAR,K0,K1);
      return APP_ERR;
   }

   switch(Grid->GDef->GRTYP[0]) {
      case 'X': // This is a $#@$@#% grid (orca)
      case 'O':
         return EZGrid_LLGetValueO(Grid,NULL,Mode,Lat,Lon,K0,K1,Value,NULL,1.0);

      case 'Y': // This is a point cloud
         return EZGrid_LLGetValueY(Grid,NULL,Mode,Lat,Lon,K0,K1,Value,NULL,1.0);

      case 'M': // This is a triangle mesh
         return EZGrid_LLGetValueM(Grid,NULL,Mode,Lat,Lon,K0,K1,Value,NULL,1.0);

      case 'Z':
         // Handle the case of ZW grids
         if( Grid->GDef->GRTYP[1] == 'W' ) {
            double i,j;

            if( !Grid->GDef->GRef->UnProject(Grid->GDef->GRef,&i,&j,Lat,Lon,0,1) ) {
               return APP_ERR;
            }
            return EZGrid_IJGetValue(Grid,Mode,i,j,K0,K1,Value);
         }
         // ELSE, we fallthrough (no break)

      default: // This is a regular RPN grid
         latf = (float)Lat;
         // EZSCINT has problems with negative longitudes
         lonf = Lon<0.0 ? (float)Lon+360.0f : (float)Lon;
         // RPN_IntLock();
         c_gdxyfll(Grid->GDef->GID,&i,&j,&latf,&lonf,1);
         // RPN_IntUnlock();
         return(EZGrid_IJGetValue(Grid,Mode,i-1.0f,j-1.0f,K0,K1,Value));
   }

   return APP_ERR;
}

int EZGrid_LLGetValueO(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,double Lat,double Lon,int K0,int K1,float* restrict UU,float* restrict VV,float Conv) {
   double       i,j,d,th,len;
   int          k=0,ik=0;
   unsigned int idx;

   if (!GridU || (GridU->GDef->GRTYP[0]!='O' && GridU->GDef->GRTYP[0]!='X')) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return APP_ERR;
   }

   if (!GridU->GDef->GRef->UnProject(GridU->GDef->GRef,&i,&j,Lat,Lon,FALSE,TRUE)){
      return APP_ERR;
   }

   k=K0;
   if (GridV)
      d=GeoRef_GeoDir(GridU->GDef->GRef,i,j);

   do {
      // Make sure the level is loaded
      if(            !EZGrid_IsLoaded(GridU,k) ) APP_ASRT_OK( EZGrid_GetData(GridU,k) );
      if( GridV &&   !EZGrid_IsLoaded(GridV,k) ) APP_ASRT_OK( EZGrid_GetData(GridV,k) );

      if( Mode == EZ_NEAREST ) {
         // Get the nearest index
         idx=lrint(j)*GridU->GDef->NI+lrint(i);

         // Check if in the mask
         if( GridU->Mask && GridU->Mask[k] && !GridU->Mask[k][idx] ) {
            ik++;
            continue;
         }
                     UU[ik] = GridU->Data[k][idx];
         if (GridV)  VV[ik] = GridV->Data[k][idx];
      } else {
                     UU[ik] = Vertex_ValS(GridU->Data[k],GridU->Mask[k],GridU->GDef->NI,GridU->GDef->NJ,i,j,FALSE);
         if (GridV)  VV[ik] = Vertex_ValS(GridV->Data[k],GridV->Mask[k],GridV->GDef->NI,GridV->GDef->NJ,i,j,FALSE);
      }

      if (Conv!=1.0) {
                     UU[ik] *= Conv;
         if (GridV)  VV[ik] *= Conv;
      }

      // Re-orient components geographically
      if (GridV) {
         th = atan2(UU[ik],VV[ik])-d;
         len = hypot(UU[ik],VV[ik]);
         UU[ik] = len*sin(th);
         VV[ik] = len*cos(th);
      }
      ik++;
   } while ((K0<=K1?k++:k--)!=K1);

   return APP_OK;
}

int EZGrid_LLGetValueY(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,double Lat,double Lon,int K0,int K1,float* restrict UU,float* restrict VV,float Conv) {
   double     r,wt,efact,dists[EZGRID_YLINEARCOUNT],w[EZGRID_YLINEARCOUNT];
   int        k,ik=0,n,nb=-1,idxs[EZGRID_YLINEARCOUNT];

   if (!GridU || GridU->GDef->GRTYP[0]!='Y') {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return APP_ERR;
   }

   CLAMPLON(Lon);
   // Find nearest(s) points: 1 point if Mode==EZ_NEAREST or EZGRID_YLINEARCOUNT points if  Mode!=EZ_NEAREST
   if (!(nb=GeoRef_Nearest(GridU->GDef->GRef,Lon,Lat,idxs,dists,Mode==EZ_NEAREST?1:EZGRID_YLINEARCOUNT))) {
      return APP_ERR;
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
      wt = 1.0/wt;
   }

   k=K0;
   do {
      if(            !EZGrid_IsLoaded(GridU,k) ) APP_ASRT_OK( EZGrid_GetData(GridU,k) );
      if( GridV &&   !EZGrid_IsLoaded(GridV,k) ) APP_ASRT_OK( EZGrid_GetData(GridV,k) );

      if (nb==1) {
         // For a single nearest, return value
                     UU[ik] = GridU->Data[k][idxs[0]];
         if (GridV)  VV[ik] = GridV->Data[k][idxs[0]];
      } else {
         // Otherwise, interpolate
                     UU[ik]=0.0;
         if (GridV)  VV[ik]=0.0;

         // Sum values modulated by weight
         for(n=0;n<nb;n++) {
                        UU[ik] += GridU->Data[k][idxs[n]]*w[n];
            if (GridV)  VV[ik] += GridV->Data[k][idxs[n]]*w[n];
         }
                     UU[ik] *= wt;
         if (GridV)  VV[ik] *= wt;
      }

      if (Conv!=1.0) {
                     UU[ik] *= Conv;
         if (GridV)  VV[ik] *= Conv;
      }
      ik++;
   } while ((K0<=K1?k++:k--)!=K1);

   return APP_OK;
}

int EZGrid_LLGetValueM(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,double Lat,double Lon,int K0,int K1,float* restrict UU,float* restrict VV,float Conv) {
   int          k=0,ik=0;
   TQTree      *node;
   TGeoRef     *gref;
   Vect3d       bary;
   int          n,idxs[3];
   intptr_t    idx;

   if (!GridU || GridU->GDef->GRTYP[0]!='M') {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return APP_ERR;
   }

   CLAMPLON(Lon);
   gref=GridU->GDef->GRef;

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
      k=K0;
      do {
         if(            !EZGrid_IsLoaded(GridU,k) ) APP_ASRT_OK( EZGrid_GetData(GridU,k) );
         if( GridV &&   !EZGrid_IsLoaded(GridV,k) ) APP_ASRT_OK( EZGrid_GetData(GridV,k) );

         if (Mode==EZ_NEAREST) {
            n=(bary[0]>bary[1]?(bary[0]>bary[2]?0:2):(bary[1]>bary[2]?1:2));

                        UU[ik] = GridU->Data[k][n];
            if (GridV)  VV[ik] = GridV->Data[k][n];
         } else {
                        UU[ik] = Bary_Interp(bary,GridU->Data[k][idxs[0]],GridU->Data[k][idxs[1]],GridU->Data[k][idxs[2]]);
            if (GridV)  VV[ik] = Bary_Interp(bary,GridV->Data[k][idxs[0]],GridV->Data[k][idxs[1]],GridV->Data[k][idxs[2]]);
         }

         if (Conv!=1.0) {
                        UU[ik] *= Conv;
            if (GridV)  VV[ik] *= Conv;
         }
         ik++;
      } while ((K0<=K1?k++:k--)!=K1);
   } else {
      // Out of meshe
      return APP_ERR;
   }

   return APP_OK;
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
 *   <int>       : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *
 *   - On effectue une interpolation lineaire
 *   - Cette fonction permet de recuperer un profile
 *----------------------------------------------------------------------------
*/
int EZGrid_LLGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,double Lat,double Lon,int K0,int K1,float* restrict UU,float* restrict VV,float Conv) {
   float i,j;
   float latf,lonf;

   if (!GridU || !GridV) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return APP_ERR;
   }

   if (K0<0 || K0>=GridU->GDef->ZRef->LevelNb || K1<0 || K1>=GridU->GDef->ZRef->LevelNb) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): K(%i,%i)\n",__func__,GridU->H.NOMVAR,K0,K1);
      return APP_ERR;
   }

   switch(GridU->GDef->GRTYP[0]) {
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
         if( GridU->GDef->GRTYP[1] == 'W' ) {
            Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unimplemented for ZW grids\n",__func__);
            return APP_ERR;
         }
         // ELSE, we fallthrough (no break)

      default: // This is a regular RPN grid
         latf = (float)Lat;
         // EZSCINT has problems with negative longitudes
         lonf = Lon<0.0 ? (float)Lon+360.0f : (float)Lon;

         // RPN_IntLock();
         c_gdxyfll(GridU->GDef->GID,&i,&j,&latf,&lonf,1);
         // RPN_IntUnlock();

         return EZGrid_IJGetUVValue(GridU,GridV,Mode,i-1.0f,j-1.0f,K0,K1,UU,VV,Conv);
   }

   return APP_ERR;
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
 *   <int>       : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *
 *   - On effectue une interpolation lineaire
 *   - Cette fonction permet de recuperer un profile
 *----------------------------------------------------------------------------
*/

int EZGrid_IJGetValue(TGrid* restrict const Grid,TGridInterpMode Mode,float I,float J,int K0,int K1,float* restrict Value) {
   int        i,j,k,ik,idx,idxs[4],maxiw;
   float      dx,dy,dxy,d[4];

   if( !Grid ) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return APP_ERR;
   }

   i = (int)I;

   // Maximum floored i value that we can have (including when we wrap)
   maxiw = Grid->GDef->NI-1 - (Grid->GDef->Wrap==2);

   // Check inclusion in master grid limits
   if( I<0.0f || J<0.0f || J>Grid->GDef->NJ-1 || I>Grid->GDef->NI-1+(Grid->GDef->Wrap==1) || i>maxiw || K0<0 || K1<0 || K0>=Grid->GDef->ZRef->LevelNb || K1>=Grid->GDef->ZRef->LevelNb ) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s): I(%f) J(%f) K(%i,%i)\n",__func__,Grid->H.NOMVAR,I,J,K0,K1);
      return APP_ERR;
   }

   // Calculate indexes and interpolation factors
   if( Mode == EZ_NEAREST ) {
      idx = lrintf(J)*Grid->GDef->NI + lrintf(I);
   } else {
      j = (int)J;

      idxs[0] = j*Grid->GDef->NI + i;

      if( j < Grid->GDef->NJ-1 ) {
         idxs[2] = idxs[0] + Grid->GDef->NI;
         dy = J-j;
      } else {
         // Adjustment for when the J coordinate falls exactly on the last gridpoint or just past it but still inside the cell
         idxs[2] = idxs[0];
         dy = 0.0f;
      }

      if( i < maxiw ) {
         idxs[1] = idxs[0] + 1;
         idxs[3] = idxs[2] + 1;
         dx = I-i;
      } else if( Grid->GDef->Wrap ) {
         // We are wrapping, the index is at the start of the current row
         idxs[1] = idxs[0] - i;
         idxs[3] = idxs[2] - i;
         dx = I-i;
      } else {
         // Adjustment for when the I coordinate falls exactly on the last gridpoint or just past it but still inside the cell
         idxs[1] = idxs[0];
         idxs[3] = idxs[2];
         dx = 0.0f;
      }

      dxy = dx*dy;
   }

   k=K0;
   ik=0;
   do {
      if( !EZGrid_IsLoaded(Grid,k) )
         APP_ASRT_OK( EZGrid_GetData(Grid,k) );

      if (Mode==EZ_NEAREST) {
         Value[ik] = Grid->Data[k][idx];
      } else {
         d[0]=Grid->Data[k][idxs[0]];
         d[1]=Grid->Data[k][idxs[1]];
         d[2]=Grid->Data[k][idxs[2]];
         d[3]=Grid->Data[k][idxs[3]];

         Value[ik]=d[0] + (d[1]-d[0])*dx + (d[2]-d[0])*dy + (d[3]-d[1]-d[2]+d[0])*dxy;
      }

      ik++;
   } while ((K0<=K1?k++:k--)!=K1);

   return APP_OK;
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
 *   <int>       : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *
 *   - On effectue une interpolation lineaire
 *   - Cette fonction permet de recuperer un profile
 *----------------------------------------------------------------------------
*/
int EZGrid_IJGetUVValue(TGrid* restrict const GridU,TGrid* restrict const GridV,TGridInterpMode Mode,float I,float J,int K0,int K1,float* restrict UU,float* restrict VV,float Conv) {
   double     d,v;
   int        ik,k;

   if (!GridU || !GridV || GridU->GDef->GID<0 || GridU->GDef->GID!=GridV->GDef->GID ) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return APP_ERR;
   }

   // Check inclusion in master grid limits
   if (I<0.0f || J<0.0f || K0<0 || K1<0 || I>GridU->GDef->NI-1 || J>GridU->GDef->NJ-1 || K0>=GridU->GDef->ZRef->LevelNb || K1>=GridU->GDef->ZRef->LevelNb) {
      Lib_Log(APP_LIBEER,APP_DEBUG,"%s: Coordinates out of range (%s,%s): I(%f) J(%f) K(%i,%i)\n",__func__,GridU->H.NOMVAR,GridV->H.NOMVAR,I,J,K0,K1);
      return APP_ERR;
   }

   if( Mode == EZ_NEAREST ) {
      I = roundf(I);
      J = roundf(J);
   }

   // Have to readjust coordinate for ezscint (1..N instead of 0..N-1)
   I += 1.0;
   J += 1.0;

   k=K0;
   ik=0;
   do {
      if( !EZGrid_IsLoaded(GridU,k) ) APP_ASRT_OK( EZGrid_GetData(GridU,k) );
      if( !EZGrid_IsLoaded(GridV,k) ) APP_ASRT_OK( EZGrid_GetData(GridV,k) );

//      RPN_IntLock();
      c_gdxywdval(GridU->GDef->GID,&UU[ik],&VV[ik],GridU->Data[k],GridV->Data[k],&I,&J,1);
//      RPN_IntUnlock();

      d = DEG2RAD(VV[ik]);
      v = UU[ik]*Conv;

      UU[ik] = -v*sin(d);
      VV[ik] = -v*cos(d);

      ik++;
   } while ((K0<=K1?k++:k--)!=K1);

   return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <EZGrid_GetArrayPtr>
 * Creation : Janvier 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Obtenir un pointeur sur les valeurs
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <K>          : Niveau
 *
 * Retour:  Un pointeur sur les valeurs
 *
 * Remarques :
 *
 *   - On effectue aucune interpolation
 *----------------------------------------------------------------------------
*/

float* EZGrid_GetArrayPtr(TGrid* restrict const Grid,int K) {

   if (!Grid) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return(FALSE);
   }

   if( EZGrid_GetData(Grid,K) != APP_OK ) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Could not get data at level %d (%s)\n",__func__,K,Grid->H.NOMVAR);
      return NULL;
   }

   return Grid->Data[K];
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
int EZGrid_GetDims(const TGrid* restrict const Grid,int Invert,float* DX,float* DY,float* DA) {

   return(GeoRef_CellDims(Grid->GDef->GRef,Invert,DX,DY,DA));

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
 *   <int>       : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *    - Ceci n'est qu'un wrapper sur c_gdllfxy pour le rendre threadsafe
 *----------------------------------------------------------------------------
*/
int EZGrid_GetLL(TGrid* restrict const Grid,double* Lat,double* Lon,float* I,float* J,int Nb) {
   int i;
   float latf,lonf;
   float fi,fj;

   if( !Grid ) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return APP_ERR;
   }

   if( Grid->GDef->GID >= 0 ) {
   //   RPN_IntLock();
      for(i=0; i<Nb; ++i) {
         fi = I[i]+1.0;
         fj = J[i]+1.0;
         if( c_gdllfxy(Grid->GDef->GID,&latf,&lonf,&fi,&fj,1)<0 ) {
            return APP_ERR;
         }
         Lat[i] = latf;
         Lon[i] = lonf;
      }
   //   RPN_IntUnlock();
   } else {
      for(i=0; i<Nb; ++i) {
         APP_ASRT_OK( Grid->GDef->GRef->Project(Grid->GDef->GRef,I[i],J[i],&Lat[i],&Lon[i],FALSE,TRUE) );
      }
   }

   return APP_OK;
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
 *   <int>       : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *    - Ceci n'est qu'un wrapper sur c_gdxyfll pour le rendre threadsafe
 *----------------------------------------------------------------------------
*/
int EZGrid_GetIJ(TGrid* restrict const Grid,double* Lat,double* Lon,float* I,float* J,int Nb) {
   int i;
   double x,y;
   float latf,lonf;

   if( !Grid ) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return APP_ERR;
   }

   if( Grid->GDef->GID >= 0 ) {
  //   RPN_IntLock();
      for(i=0; i<Nb; ++i) {
         latf = (float)Lat[i];
         // EZSCINT has problems with negative longitudes
         lonf = Lon[i]<0.0 ? (float)Lon[i]+360.0f : (float)Lon[i];
         if( c_gdxyfll(Grid->GDef->GID,&I[i],&J[i],&latf,&lonf,1)!=0 ) {
            return APP_ERR;
         }
         I[i] -= 1.0f;
         J[i] -= 1.0f;
      }
   //   RPN_IntUnlock();
   } else {
      for(i=0; i<Nb; ++i) {
         APP_ASRT_OK( Grid->GDef->GRef->UnProject(Grid->GDef->GRef,&x,&y,Lat[i],Lon[i],FALSE,TRUE) );
         I[i] = (float)x;
         J[i] = (float)y;
      }
   }

   return APP_OK;
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
 *   <int>       : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int EZGrid_GetBary(TGrid* restrict const Grid,double Lat,double Lon,Vect3d Bary,Vect3i Index) {
   TGeoRef      *gref;
   TQTree       *node;
   unsigned int *idx,n;
   intptr_t     t;

   if( !Grid || Grid->GDef->GRTYP[0]=='M' ) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid grid\n",__func__);
      return APP_ERR;
   }

   // Is this a triangle mesh
   gref = Grid->GDef->GRef;
   idx = gref->Idx;
   CLAMPLON(Lon);

   // Find enclosing triangle
   if ((node=QTree_Find(gref->QTree,Lon,Lat)) && node->NbData) {

      // Loop on this nodes data payload
      for(n=0;n<node->NbData;n++) {
         t=(intptr_t)node->Data[n].Ptr-1; // Remove false pointer increment

         // if the Barycentric coordinates are within this triangle, get its interpolated value
         if (Bary_Get(Bary,gref->Wght?gref->Wght[t/3]:0.0,Lon,Lat,gref->AX[idx[t]],gref->AY[idx[t]],gref->AX[idx[t+1]],gref->AY[idx[t+1]],gref->AX[idx[t+2]],gref->AY[idx[t+2]])) {
            if (Index) {
               Index[0]=idx[t];
               Index[1]=idx[t+1];
               Index[2]=idx[t+2];
            }
            return APP_OK;
         }
      }
   }

   // We must be out of the tin
   return APP_ERR;
}

#endif
