/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : EZVrInt
 * Creation  : version 1.0 mai 2003
 * Auteur    : Stéphane Gaudreault
 *
 * Description:
 *    Ezvrint is based on EzInterpv, a front end to the
 *    Interp1D package created by Jeffrey Blezius. This interface has been
 *    designed to resemble to the ezscint package. Most of the functionality
 *    of the Interp1D package is available through the ezvrint interface. The
 *    advantage of using ezvrint is that multiple interpolations are automated,
 *    using an interface with a familiar feel.
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

#if defined(__GNUC__) || defined(SGI)
   #include <fenv.h>
#elif defined(_AIX)
   #include <float.h>
   #include <fpxcp.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "EZVrInt.h"

static viInterp *FInterp;

/* 1D Interpolation functions */
extern void f77name (interp1d_findpos) ();
extern void f77name (interp1d_nearestneighbour) ();
extern void f77name (interp1d_linear) ();
extern void f77name (interp1d_cubicwithderivs) ();
extern void f77name (interp1d_cubiclagrange) ();
extern void f77name (extrap1d_lapserate) ();

/* Prototypes */
void vicleanGrid (viInterp *Interp);

/* Fortran Interface */
wordint f77name(videfine)(void) {
   c_videfine ();
   return(1);
}
wordint f77name(viundefine)(void) {
   return(c_viundefine(FInterp));
}

wordint f77name(viqkdef)(wordint *numLevel,wordint *gridType,ftnfloat *levelList,ftnfloat *top,ftnfloat *pRef,ftnfloat *rCoef,ftnfloat *zcoord,ftnfloat *a,ftnfloat *b) {
   TZRef zref;

   zref.Type=*gridType;
   zref.LevelNb=*numLevel;
   zref.Levels=levelList;
   zref.PTop=*top;
   zref.PRef=*pRef;
   zref.RCoef[0]=*rCoef;
   zref.P0=zcoord;
   zref.A=a;
   zref.B=b;

   return c_viqkdef(FInterp,&zref);
}

wordint f77name(videfset)(wordint *ni,wordint *nj,wordint *idGrdDest,wordint *idGrdSrc) {
   return c_videfset(FInterp,*ni,*nj,*idGrdDest,*idGrdSrc);
}

wordint f77name(visetopt)(wordint *option,wordint *value) {
   return c_visetopt(FInterp,(const char*)option,(const char*)value);
}

wordint f77name (visint)(ftnfloat *stateOut,ftnfloat *stateIn,ftnfloat *derivOut,ftnfloat *derivIn,ftnfloat *extrapGuideDown,ftnfloat *extrapGuideUp) {
   return c_visint(FInterp,stateOut,stateIn,derivOut,derivIn,*extrapGuideDown,*extrapGuideUp);
}

/*----------------------------------------------------------------------------
 * Nom      : <c_viqkdef>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      : This is the initializer for the internal data structures.  It defines
 *            one vertical grid. That grid may be used as the input grid or
 *            the output grid.
 *
 * Parametres     :
 *  <numLevel>    : number of vertical levels
 *  <gridType>    : the type of grid -- see values in #define
 *  <levelList>   : the levels, in units determined by gridType
 *  <top>         : In the case of hybrid : average pressure (mb) at model top
 *                  In Gal-Chen : height of the model top (meter)
 *                  (NOT REQUIRED FOR OTHER TYPE)
 *  <pRef>        : reference pressure (mb)
 *  <rCoef>       : known as 'expansion co-efficient'
 *  <zcoord>      : Geopotential for meters coord or surface pressure for pressure coords
 *
 * Retour         : id of the grid.
 *
 * Remarques :
 *
 * Modifications :
 *
 *    Nom         : Jean-Philippe Gauthier
 *    Date        : Septembre 2004
 *    Description : Proteger la grille prcedente d'etre remplacer par une autre en cas de
 *                  depassement des limites du nombres de grilles
 *                  La variable "last" garde la grille precedente
 *----------------------------------------------------------------------------
*/
viInterp* c_videfine (void) {

   viInterp *interp;

   interp=(viInterp*)malloc(sizeof(viInterp));

   memset(interp->ZRefs,0x0,VIGRIDLENGTH*sizeof(TZRef));
   interp->ZRefSrc=NULL;
   interp->ZRefDest=NULL;
   interp->gCubeSrc_p=NULL;
   interp->gCubeDest_p=NULL;
   interp->gInterpIndex_p=NULL;
   interp->gViOption=0x0000;
   interp->gNi=0;
   interp->gNj=0;
   interp->index=-1;
   interp->last=-1;

   /*Fortran default to global*/
   FInterp=interp;

   return(interp);
}

int c_viundefine(viInterp *interp) {

   if (interp) {
      if (interp->ZRefSrc)        free(interp->ZRefSrc);
      if (interp->ZRefDest)       free(interp->ZRefDest);
      if (interp->gCubeSrc_p)     free(interp->gCubeSrc_p);
      if (interp->gCubeDest_p)    free(interp->gCubeDest_p);
      if (interp->gInterpIndex_p) free(interp->gInterpIndex_p);

      free(interp);
   }

   return(1);
}

int c_viqkdef(viInterp *interp,TZRef *ZRef) {
   int i;

   if (!ZRef->Levels || ZRef->LevelNb<=0)
      return(0);

   /*On verifie si cette grille existe deja */
   for (i=0;i<VIGRIDLENGTH;i++) {
      if (interp->ZRefs[i].LevelNb==ZRef->LevelNb && interp->ZRefs[i].Type==ZRef->Type && interp->ZRefs[i].PTop==ZRef->PTop &&
           interp->ZRefs[i].PRef==ZRef->PRef && interp->ZRefs[i].RCoef[0]==ZRef->RCoef[0] && interp->ZRefs[i].RCoef[1]==ZRef->RCoef[1] &&
          interp->ZRefs[i].P0==ZRef->P0 && interp->ZRefs[i].A==ZRef->A && interp->ZRefs[i].B==ZRef->B) {
          if (memcmp(interp->ZRefs[i].Levels,ZRef->Levels,ZRef->LevelNb*sizeof(float))==0) {
             if (interp->gViOption & VIVERBOSE) printf ("(INFO) c_viqkdef: Grid already defined (%i)\n", i);
             interp->last=i;
             return(i);
         }
      }
   }

   /* Select a new grid form the maximum but do not use the previous one*/
   /* in case we are doing single point interpolation */
   interp->index=(interp->index+1)%VIGRIDLENGTH;
   if (interp->index==interp->last) {
      interp->index=(interp->index+1)%VIGRIDLENGTH;
   }
   interp->last=interp->index;

   if (interp->gViOption & VIVERBOSE) printf ("(INFO) c_viqkdef: New grid (%i)\n", interp->index);
   if (interp->ZRefs[interp->index].Levels) {
      free (interp->ZRefs[interp->index].Levels);
      interp->ZRefs[interp->index].Levels = NULL;
   }

   memcpy(&interp->ZRefs[interp->index],ZRef,sizeof(TZRef));

   interp->ZRefs[interp->index].Levels = (float*)malloc(ZRef->LevelNb*sizeof(float));
   if (!interp->ZRefs[interp->index].Levels) {
      fprintf (stderr, "(ERROR) c_viqkdef: malloc failed for ZRefs[%d].Levels\n", interp->index);
      return -1;
   }

   memcpy(interp->ZRefs[interp->index].Levels,ZRef->Levels,ZRef->LevelNb*sizeof (float));

   if (ZRef->Type == LVL_GALCHEN) {
      if (ZRef->PTop == 0) {
         fprintf(stderr, "(ERROR) c_viqkdef: Height at the top of the atmosphere is needed for Gal-Chen interpolation\n");
         return -1;
      }
   } else if (ZRef->Type == LVL_ETA) {
      if (ZRef->PTop == 0) {
         fprintf(stderr, "(ERROR) c_viqkdef: Pressure at the top of the atmosphere is needed for Eta interpolation\n");
         return -1;
      }
   } else if (ZRef->Type != LVL_HYBRID) {
      interp->ZRefs[interp->index].PTop = interp->ZRefs[interp->index].PRef = interp->ZRefs[interp->index].RCoef[0] = interp->ZRefs[interp->index].RCoef[1]= 0.0f;
   }

   /*Make sure the gridset is reinitialised if we just changed the grid it used on last set*/
   if (interp->ZRefSrc==&interp->ZRefs[interp->index]) {
      interp->ZRefSrc=NULL;
   }
   if (interp->ZRefDest==&interp->ZRefs[interp->index]) {
      interp->ZRefDest=NULL;
   }

   return(interp->index);
}

/*----------------------------------------------------------------------------
 * Nom      : <c_videfset>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      : Define the set of grids to be used for the vertical interpolation.
 *
 * Parametres     :
 *  <ni>          : dimension of the horizontal grid in the interpolation
 *  <nj>          : other dimension of the horizontal grid in the interpolation
 *  <idGrdDest>   : id of the vertical grid of output points: obtained from c_viqkdef
 *  <idGrdSrc>    : id of the vertical grid of input points: obtained from c_viqkdef
 *
 * Retour         :  0 : Error
 *
 * Remarques : It is assumed that the vertical levels of the grid 'idGrdSrc' are in
 *             either ascending or descending order.
 *
 *             The vertical levels of the grid 'idGrdDest' may be in any order.
 *             The determination of the location of each target level is completely
 *             independent of all the others.
 *
 * Modifications :
 *
 *    Nom         :
 *    Date        :
 *    Description :
 *----------------------------------------------------------------------------
*/
int c_videfset(viInterp *interp,const int ni,const int nj,int idGrdDest,int idGrdSrc) {

   int numInterpSets, src_ijDim, dst_ijDim;
#if defined(_AIX)
   fpflag_t flag;
#endif
   if (idGrdDest < 0 || idGrdDest > VIGRIDLENGTH || idGrdSrc < 0 || idGrdSrc > VIGRIDLENGTH) {
      fprintf (stderr,"(ERROR) c_videfset: Invalid grid ID passed to c_videfset\n");
      return(0);
   }

   if (interp->ZRefSrc==&interp->ZRefs[idGrdSrc] && interp->ZRefDest==&interp->ZRefs[idGrdDest]) {
      if (interp->gViOption & VIVERBOSE) printf ("(INFO) c_videfset: Same gridset\n");
      return(1);
   }

   if (interp->gViOption & VIVERBOSE) printf ("(INFO) c_videfset: New gridset\n");

   interp->ZRefSrc = &interp->ZRefs[idGrdSrc];
   interp->ZRefDest = &interp->ZRefs[idGrdDest];

   if (!interp->ZRefSrc || !interp->ZRefDest) {
      fprintf(stderr,"(ERROR) c_videfset: Grid does not exists");
      return(0);
   }

   if (interp->ZRefSrc->LevelNb==interp->ZRefDest->LevelNb &&
      memcmp(interp->ZRefSrc->Levels,interp->ZRefDest->Levels,interp->ZRefSrc->LevelNb*sizeof(float))==0) {
      interp->same=1;
      if (interp->gViOption & VIVERBOSE) printf ("(INFO) c_videfset: Levels are the same\n");
      return(1);
   }
   interp->same=0;

   if (interp->gViOption & VICHECKFLOAT) {
      /* clear exception flags (plateforme specific) */
#if defined(__GNUC__) || defined(IRIX)
      feclearexcept(FE_ALL_EXCEPT);
#elif defined(_AIX)
      fp_clr_flag(FP_INVALID | FP_OVERFLOW | FP_UNDERFLOW | FP_DIV_BY_ZERO | FP_INEXACT);
#endif
   }

   vicleanGrid(interp);
   interp->gNi = ni;
   interp->gNj = nj;

   /* Create cubes of vertical levels ... */
   interp->gCubeSrc_p=(float*)malloc(ni*nj*interp->ZRefSrc->LevelNb*sizeof(float));
   if (!interp->gCubeSrc_p) {
      fprintf(stderr,"(ERROR) c_videfset: malloc failed for gCubeSrc_p\n");
      return(0);
   }
   interp->gCubeDest_p=(float*)malloc(ni*nj*interp->ZRefDest->LevelNb*sizeof (float));
   if (!interp->gCubeSrc_p) {
      fprintf(stderr,"(ERROR) c_videfset: malloc failed for gCubeDest_p\n");
      return(0);
   }

   /* and convert them to ... */

   if ((interp->ZRefDest->Type == LVL_GALCHEN) || (interp->ZRefDest->Type == LVL_MASL) || (interp->ZRefDest->Type == LVL_MAGL) ||
       (interp->ZRefSrc->Type == LVL_GALCHEN) || (interp->ZRefSrc->Type == LVL_MASL) || (interp->ZRefSrc->Type == LVL_MAGL)) {

      /* ... height above sea  */
      if (!ZRef_KCube2Meter(interp->ZRefSrc,interp->ZRefSrc->P0,ni*nj,interp->gCubeSrc_p)) {
         return(0);
      }
      if (!ZRef_KCube2Meter(interp->ZRefDest,interp->ZRefDest->P0,ni*nj,interp->gCubeDest_p)) {
         return(0);
      }
   } else {
      /* ... or ln(P) */
      if (!ZRef_KCube2Pressure(interp->ZRefSrc,interp->ZRefSrc->P0,ni*nj,TRUE,interp->gCubeSrc_p)) {
         return(0);
      }
      if (!ZRef_KCube2Pressure(interp->ZRefDest,interp->ZRefDest->P0,ni*nj,TRUE,interp->gCubeDest_p)) {
         return(0);
      }
   }

   if (interp->gViOption & VICHECKFLOAT) {
   /* Check for exception in the conversion (plateforme specific) */
#if defined(__GNUC__)  || defined(SGI)
      if (fetestexcept(FE_DIVBYZERO)) {
         fprintf (stderr, "(WARNING) c_videfset: Grid conversion gives an infinity (DIVBYZERO)\n");
      }
      if (fetestexcept(FE_OVERFLOW)) {
         fprintf (stderr, "(WARNING) c_videfset: Grid conversion gives an overflow\n");
      }
      if (fetestexcept(FE_UNDERFLOW)) {
         fprintf (stderr, "(WARNING) c_videfset: Grid conversion gives an underflow\n");
      }
      if (fetestexcept(FE_INVALID)) {
         fprintf (stderr, "(WARNING) c_videfset: Grid conversion gives a NaN (not a number)\n");
      }
#elif defined(_AIX)
      flag = fp_read_flag();
      if (flag & FP_INVALID) {
         fprintf (stderr, "(WARNING) c_videfset: Grid conversion gives a NaN (not a number)\n");
      }
      if (flag & FP_OVERFLOW) {
         fprintf (stderr, "(WARNING) c_videfset: Grid conversion gives an overflow\n");
      }
      if (flag & FP_UNDERFLOW) {
         fprintf (stderr, "(WARNING) c_videfset: Grid conversion gives an underflow\n");
      }
      if (flag & FP_DIV_BY_ZERO) {
         fprintf (stderr, "(WARNING) c_videfset: Grid conversion gives an infinity (DIV_BY_ZERO)\n");
      }
#else
      fprintf(stderr,"(WARNING) Exception check is not availlable on this architecture");
#endif
   }

   interp->gInterpIndex_p = (int *) malloc(ni * nj * interp->ZRefDest->LevelNb*sizeof (int));
   if (!interp->gInterpIndex_p) {
      fprintf (stderr, "(ERROR) c_videfset: malloc failed for gInterpIndex_p\n");
      return(0);
   }

   numInterpSets = src_ijDim = dst_ijDim = ni * nj;

   /*
    * Find array indices between which interpolation will occur.  The
    * output of this routine is input for each of the interpolation
    * routines. The output is in gInterpIndex_p.
    */
   (void)f77name(interp1d_findpos)(&numInterpSets,&interp->ZRefSrc->LevelNb,
                             &interp->ZRefDest->LevelNb,&src_ijDim,&dst_ijDim,
                             interp->gCubeSrc_p,interp->gInterpIndex_p,interp->gCubeDest_p);
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <c_visetopt>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      :  set options in for the interpolation / extrapolation algorithms.
 * The option and the value to which it is to be set are passed as strings.
 * These string are similar to those used by ezcint
 *
 *
 * Parametres     :
 *  <option>      : string representing the option to be set:
 *                  "INTERP_DEGREE", "EXTRAP_DEGREE" or "VERBOSE"
 *  <value>       : string representing the value to which the option is to
 *                  be set.
 *                  Possible values for the option, "INTERP_DEGREE", are:
 *                  "NEAREST", "LINEAR", "CUBICWITHDERIVS" or "CUBIC"
 *                  Possible values for the option, "EXTRAP_DEGREE", are:
 *                  "CLAMPED" or "LAPSERATE"
 *                  Possible values for the option, "VERBOSE", are:
 *                  YES or NO
 *
 * Remarques :
 *
 * Modifications :
 *
 *    Nom         :
 *    Date        :
 *    Description :
 *----------------------------------------------------------------------------
*/
int c_visetopt (viInterp *interp,const char *option, const char *value) {

   if (strncmp (option, "INTERP_DEGREE", 13) == 0) {
      if (strncmp (value, "NEAREST", 7) == 0) {
         interp->gViOption |= VINEAREST_NEIGHBOUR;
      } else if (strncmp (value, "LINEAR", 6) == 0) {
         interp->gViOption |= VILINEAR;
      } else if (strncmp (value, "CUBICWITHDERIVS", 15) == 0) {
         interp->gViOption |= VICUBIC_WITH_DERIV;
      } else if (strncmp (value, "CUBIC", 5) == 0) {
         interp->gViOption |= VICUBIC_LAGRANGE;
      }
   } else if (strncmp (option, "EXTRAP_DEGREE" ,13) == 0) {
      if (strncmp (value, "CLAMPED", 7) == 0) {
         interp->gViOption |= VICLAMPED;
      } else if (strncmp (value, "LAPSERATE", 9) == 0) {
         interp->gViOption |= VILAPSERATE;
      }
   } else if (strncmp (option, "VERBOSE", 7) == 0) {
      if (strncmp (value, "YES", 3)  == 0) {
         interp->gViOption |= VIVERBOSE;
      } else {
         interp->gViOption &= ~VIVERBOSE;
      }
   } else if (strncmp (option, "CHECKFLOAT", 10) == 0) {
      interp->gViOption |= VICHECKFLOAT;
   }

   return(interp->gViOption);
}

/*----------------------------------------------------------------------------
 * Nom      : <c_visetopti>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      :  set options in for the interpolation / extrapolation algorithms.
 * The option and the value to which it is to be set are passed as 8 bits
 * unsigned integer.
 *
 * Parametres     :
 *  <option>      : 8 bit unsigned integer representing the value to
 *                  which the option is to be set.
 *
 * Remarques :
 *
 * Modifications :
 *
 *    Nom         :
 *    Date        :
 *    Description :
 *----------------------------------------------------------------------------
*/
int c_visetopti (viInterp *interp,const unsigned char option) {
   return(interp->gViOption|=option);
}

/*----------------------------------------------------------------------------
 * Nom      : <c_visint>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      : Perform the selected interpolation
 *
 * Parametres     :
 *  <stateOut>          : output array of the state
 *  <stateIn>           : input array of the state
 *  <derivOut>          : output array of the derivative
 *  <derivIn>           : input array of the derivative
 *  <extrapGuideDown>   : value used for extrapolating below the values of
 *                        the source grid (set in c_videfset). The meaning
 *                        of these values depends on the selected
 *                        extrapolation method.
 *  <extrapGuideUp>     : value used for extrapolating above the values of
 *                        the source grid (set in c_videfset). The meaning
 *                        of these values depends on the selected
 *                        extrapolation method.
 *
 * Retour         :  -1 : Error
 *
 * Remarques :
 *
 * Modifications :
 *
 *    Nom         :
 *    Date        :
 *    Description :
 *----------------------------------------------------------------------------
*/
int c_visint(viInterp *interp,float *stateOut,float *stateIn,float *derivOut,float *derivIn,float extrapGuideDown,float extrapGuideUp) {

   int surf = interp->gNi * interp->gNj;
   int extrapEnable = 0;

   /* Assume that everything is set correctly */
   if (!interp->gCubeSrc_p || !interp->gCubeDest_p || !interp->ZRefDest || !interp->gInterpIndex_p || (interp->gViOption == 0x000)) {
      return(0);
   }

   if (interp->same) {
      memcpy(stateOut,stateIn,surf*interp->ZRefSrc->LevelNb);
      printf ("(INFO) c_visint: Grids are the same\n");
      return(1);
   }

   /*
    * Interpolation
    * Setting extrapEnableDown andextrapEnableUp to .false.
    * yields 'clamped'
    */
   if (interp->gViOption & VINEAREST_NEIGHBOUR) {
      (void)f77name(interp1d_nearestneighbour)(&surf,&interp->ZRefSrc->LevelNb,&interp->ZRefDest->LevelNb,&surf,
                                         &surf,interp->gCubeSrc_p,stateIn,derivIn,interp->gInterpIndex_p,interp->gCubeDest_p,stateOut,derivOut,
                                         &extrapEnable,&extrapEnable,&extrapGuideDown,&extrapGuideUp);

   } else if (interp->gViOption & VILINEAR) {
      (void)f77name(interp1d_linear)(&surf,&interp->ZRefSrc->LevelNb,&interp->ZRefDest->LevelNb,&surf,&surf,
                               interp->gCubeSrc_p,stateIn,derivIn,interp->gInterpIndex_p,interp->gCubeDest_p,stateOut,derivOut,
                               &extrapEnable,&extrapEnable,&extrapGuideDown,&extrapGuideUp);

   } else if (interp->gViOption & VICUBIC_WITH_DERIV) {
      if ((derivIn == NULL) && (derivOut == NULL)) {
         fprintf (stderr,"(ERROR) c_visint: Cubic interpolation with derivatives requested\n");
         return(0);
      }

      (void)f77name(interp1d_cubicwithderivs)(&surf,&interp->ZRefSrc->LevelNb,&interp->ZRefDest->LevelNb,&surf,&surf,
                                        interp->gCubeSrc_p,stateIn,derivIn,interp->gInterpIndex_p,interp->gCubeDest_p,stateOut,derivOut,
                                        &extrapEnable,&extrapEnable,&extrapGuideDown,&extrapGuideUp);

   } else if (interp->gViOption & VICUBIC_LAGRANGE) {
      (void)f77name(interp1d_cubiclagrange)(&surf,&interp->ZRefSrc->LevelNb,&interp->ZRefDest->LevelNb,&surf,&surf,
                                      interp->gCubeSrc_p,stateIn,derivIn,interp->gInterpIndex_p,interp->gCubeDest_p,stateOut,derivOut,
                                      &extrapEnable,&extrapEnable,&extrapGuideDown,&extrapGuideUp);
   } else {
      fprintf (stderr, "(ERROR) c_visint: Unknown interpolation algorithm\n");
      return(0);
   }

   if (interp->gViOption & VIVERBOSE) printf ("(INFO) c_visint: Interpolation completed\n");

   /* Extrapolation */
   if (interp->gViOption & VICLAMPED) {
      /*
       * Do nothing. It has already been done during interpolation.
       * This option exist for compatibility with EzInterpv
       */
      return 0;

   } else if (interp->gViOption & VILAPSERATE) {
      extrapEnable = 1;

      (void)f77name(extrap1d_lapserate)(&surf,&interp->ZRefSrc->LevelNb,&interp->ZRefDest->LevelNb,&surf,&surf,
                                  interp->gCubeSrc_p,stateIn,derivIn,interp->gInterpIndex_p,interp->gCubeDest_p,stateOut,derivOut,
                                  &extrapEnable,&extrapEnable,&extrapGuideDown,&extrapGuideUp);

      if (interp->gViOption & VIVERBOSE) printf ("(INFO) c_visint: Lapserate extrapolation completed\n");
   }

   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <vicleanGrid>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      : Libere la mémoire associées aux grilles
 *
 * Parametres     :
 *  <void>
 *
 * Retour         :  void
 *
 * Remarques :
 *
 * Modifications :
 *
 *    Nom         :
 *    Date        :
 *    Description :
 *----------------------------------------------------------------------------
*/
void vicleanGrid(viInterp *Interp) {

   if (Interp->gCubeSrc_p) {
      free(Interp->gCubeSrc_p);
      Interp->gCubeSrc_p=NULL;
   }

   if (Interp->gCubeDest_p) {
      free(Interp->gCubeDest_p);
      Interp->gCubeDest_p=NULL;
   }

   if (Interp->gInterpIndex_p) {
      free(Interp->gInterpIndex_p);
      Interp->gInterpIndex_p=NULL;
   }
}
