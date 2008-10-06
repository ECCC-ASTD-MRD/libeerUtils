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
 * Modification:
 *
 *   Nom         : - J.P. Gauthier
 *   Date        : - 14 mai 2003
 *   Description : - version 1.0rc1 : look more like ezcint
 *
 *
 *   Nom         : - J.P. Gauthier
 *   Date        : - 1 Septembre 2004
 *   Description : - version 1.0rc2 : Protection of the previous gird from overwriting
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

/*
 * 1D Interpolation functions
 */
extern void f77name (interp1d_findpos) ();
extern void f77name (interp1d_nearestneighbour) ();
extern void f77name (interp1d_linear) ();
extern void f77name (interp1d_cubicwithderivs) ();
extern void f77name (interp1d_cubiclagrange) ();
extern void f77name (extrap1d_lapserate) ();

/*
 * Convertion function
 */
extern int f77name (hybrid_to_pres) ();

/*
 * Prototypes
 */
int  vigetLnP (struct sVerticalGrid *, float *, int, int, float *, float *);
void vigetMASL (struct sVerticalGrid *, float *, int, int);
void vicleanGrid (viInterp *);

/*
 * Fortran Interface
 */
wordint f77name (videfine) () {
   c_videfine ();
   return(1);
}
wordint f77name (viundefine) () {
   return(c_viundefine(FInterp));
}

wordint f77name (viqkdef) (wordint *numLevel, wordint *gridType, ftnfloat *levelList,
                           ftnfloat *top, ftnfloat *pRef, ftnfloat *rCoef,
                           ftnfloat *gz) {
   return c_viqkdef (FInterp,*numLevel, *gridType, levelList, *top, *pRef, *rCoef, gz);
}


wordint f77name (videfset) (wordint *ni, wordint *nj, wordint *idGrdDest,
                           wordint *idGrdSrc, ftnfloat *surf, ftnfloat *top) {
   return c_videfset (FInterp,*ni, *nj, *idGrdDest, *idGrdSrc, surf, top);
}

wordint f77name (visetopt) (wordint *option, wordint *value) {
   return c_visetopt(FInterp,(const char *) option, (const char *)value);
}

wordint f77name (visint) (ftnfloat *stateOut, ftnfloat *stateIn, ftnfloat *derivOut,
                          ftnfloat *derivIn, ftnfloat *extrapGuideDown, ftnfloat *extrapGuideUp) {
   return c_visint (FInterp,stateOut, stateIn, derivOut, derivIn, *extrapGuideDown, *extrapGuideUp);
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
 *  <gz>          : Geopotentiel
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

   memset(interp->gGridArray,0x0,GRIDLENGTH*sizeof(VerticalGrid));
   interp->gGrdSrc_p=NULL;
   interp->gGrdDest_p=NULL;
   interp->gGridSurf_p=NULL;
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
      if (interp->gGrdSrc_p)      free(interp->gGrdSrc_p);
      if (interp->gGrdDest_p)     free(interp->gGrdDest_p);
      if (interp->gGridSurf_p)    free(interp->gGridSurf_p);
      if (interp->gCubeSrc_p)     free(interp->gCubeSrc_p);
      if (interp->gCubeDest_p)    free(interp->gCubeDest_p);
      if (interp->gInterpIndex_p) free(interp->gInterpIndex_p);

      free(interp);
   }

   return(1);
}

int c_viqkdef (viInterp *interp,const int numLevel, const int gridType, float *levelList,
               float top, float pRef, float rCoef, float *gz) {
   int        i;

   if (!levelList || numLevel <= 0)
      return -1;

   /*On verifie si cette grille existe deja */
   for (i = 0; i < GRIDLENGTH; i++) {
      if (interp->gGridArray[i].numLevels == numLevel && interp->gGridArray[i].gridType == gridType) {
         if (memcmp(interp->gGridArray[i].level_p, levelList, numLevel * sizeof (float)) == 0) {
            interp->gGridArray[i].gz_p     = gz;
            interp->gGridArray[i].top      = top;
            interp->gGridArray[i].pRef     = pRef;
            interp->gGridArray[i].rCoef    = rCoef;
            if (interp->gViOption & VERBOSE) printf ("c_viqkdef: Grid already defined (%i)\n", i);
            interp->last=i;
            return i;
         }
      }
   }

   /* Select a new grid form the maximum 10 but do not use the previous one*/
   /* in case we are doing single point interpolation */

   interp->index=(interp->index+1)%GRIDLENGTH;
   if (interp->index==interp->last) {
      interp->index=(interp->index+1)%GRIDLENGTH;
   }
   interp->last=interp->index;

   if (interp->gViOption & VERBOSE) printf ("c_viqkdef: New grid (%i)\n", interp->index);
   if (interp->gGridArray[interp->index].level_p != NULL) {
      free (interp->gGridArray[interp->index].level_p);
      interp->gGridArray[interp->index].level_p = NULL;
   }

   interp->gGridArray[interp->index].level_p = (float *) malloc (numLevel * sizeof (float));
   if (!interp->gGridArray[interp->index].level_p) {
      fprintf (stderr, "c_viqkdef: malloc failed for gGridArray[%d].level_p\n", interp->index);
      return -1;
   }
   memcpy (interp->gGridArray[interp->index].level_p, levelList, numLevel * sizeof (float));

   interp->gGridArray[interp->index].numLevels = numLevel;
   interp->gGridArray[interp->index].gridType  = gridType;
   interp->gGridArray[interp->index].gz_p      = gz;
   interp->gGridArray[interp->index].top       = top;
   interp->gGridArray[interp->index].pRef      = pRef;
   interp->gGridArray[interp->index].rCoef     = rCoef;

   if (gridType == LVL_GALCHEN) {
      if (top == 0) {
         fprintf (stderr, "c_viqkdef: Height at the top of the atmosphere is needed for Gal-Chen interpolation\n");
         return -1;
      }
   } else if (gridType != LVL_HYBRID) {
      interp->gGridArray[interp->index].top = interp->gGridArray[interp->index].pRef = interp->gGridArray[interp->index].rCoef = 0.0f;
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
 *  <surf>        : surface pressure at each horiz location
 *  <top>         : ceiling pressure for each horiz point (required only for eta units)
 *
 * Retour         :  -1 : Error
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

int c_videfset (viInterp *interp,const int ni, const int nj, int idGrdDest, int idGrdSrc,
                float *surf, float *top) {

   int numInterpSets, src_ijDim, dst_ijDim;
#if defined(_AIX)
   fpflag_t flag;
#endif

   if (interp->gGrdSrc_p == &interp->gGridArray[idGrdSrc] && interp->gGrdDest_p == &interp->gGridArray[idGrdDest]) {
      if (interp->gViOption & VERBOSE) printf ("c_videfset: Grids are the same\n");
      if (surf) {
         if (memcmp (interp->gGridSurf_p,surf,ni*nj*sizeof (float))==0) {
            if (interp->gViOption & VERBOSE) printf ("c_videfset: Surface pressure are the same hence, same gridset\n");
            return 0;
         }
      } else {
         if (interp->gViOption & VERBOSE)  printf ("c_videfset: Same gridset\n");
         return 0;
      }
   }

   if (interp->gViOption & VERBOSE) printf ("c_videfset: New gridset\n");

   if (idGrdDest < 0 || idGrdDest > GRIDLENGTH || idGrdSrc < 0 || idGrdSrc > GRIDLENGTH) {
      fprintf (stderr, "c_videfset: Invalid grid ID passed to c_videfset\n");
      return -1;
   }

   interp->gGrdSrc_p = &interp->gGridArray[idGrdSrc];
   interp->gGrdDest_p = &interp->gGridArray[idGrdDest];

   if (surf) {
      if (interp->gGridSurf_p) free(interp->gGridSurf_p);
      interp->gGridSurf_p= (float*)malloc(ni*nj*sizeof(float));
      memcpy(interp->gGridSurf_p,surf,ni*nj*sizeof(float));
   }

   if (interp->gGrdSrc_p->numLevels==interp->gGrdDest_p->numLevels && memcmp (interp->gGrdSrc_p->level_p, interp->gGrdDest_p->level_p, interp->gGrdSrc_p->numLevels * sizeof (float)) == 0) {
      if (interp->gViOption & VERBOSE) printf ("c_videfset: Levels are the same\n");
      return 0;
   }

   if (interp->gViOption & CHECKFLOAT) {
      /* clear exception flags (plateforme specific) */
#if defined(__GNUC__) || defined(IRIX)
      feclearexcept(FE_ALL_EXCEPT);
#elif defined(_AIX)
      fp_clr_flag(FP_INVALID | FP_OVERFLOW | FP_UNDERFLOW | FP_DIV_BY_ZERO | FP_INEXACT);
#endif
   }

   vicleanGrid (interp);
   interp->gNi = ni;
   interp->gNj = nj;

   /*
    * Create cubes of vertical levels ...
    */
   interp->gCubeSrc_p = (float *) malloc (ni * nj * interp->gGrdSrc_p->numLevels * sizeof (float));
   if (!interp->gCubeSrc_p) {
      fprintf (stderr, "c_videfset: malloc failed for gCubeSrc_p\n");
      return -1;
   }
   interp->gCubeDest_p = (float *) malloc (ni * nj * interp->gGrdDest_p->numLevels * sizeof (float));
   if (!interp->gCubeSrc_p) {
      fprintf (stderr, "c_videfset: malloc failed for gCubeDest_p\n");
      return -1;
   }

   /*
    * ... and convert them to ...
    */

   if (!interp->gGrdSrc_p || !interp->gGrdDest_p) {
      fprintf (stderr, "Grid does not exists");
      return -1;
   }

   if ((interp->gGrdDest_p->gridType == LVL_GALCHEN) || (interp->gGrdDest_p->gridType == LVL_MASL) ||
       (interp->gGrdDest_p->gridType == LVL_MAGL) || (interp->gGrdDest_p->gridType == LVL_UNDEF) ||
       (interp->gGrdSrc_p->gridType == LVL_GALCHEN) || (interp->gGrdSrc_p->gridType == LVL_MASL) ||
       (interp->gGrdSrc_p->gridType == LVL_MAGL) || (interp->gGrdSrc_p->gridType == LVL_UNDEF)) {
      /*
       * ... height above sea
       */
      vigetMASL (interp->gGrdSrc_p, interp->gCubeSrc_p, ni, nj);
      vigetMASL (interp->gGrdDest_p, interp->gCubeDest_p, ni, nj);
   } else {
      /*
       * ... or ln(P)
       */
      if (vigetLnP (interp->gGrdSrc_p, interp->gCubeSrc_p, ni, nj, surf, top) < 0) {
         /*
          * if the grid type is not implemented
          */
         return -1;
      }
      if (vigetLnP (interp->gGrdDest_p, interp->gCubeDest_p, ni, nj, surf, top) < 0) {
         /*
          * if the grid type is not implemented
          */
         return -1;
      }
   }

   if (interp->gViOption & CHECKFLOAT) {
   /* Check for exception in the conversion (plateforme specific) */
#if defined(__GNUC__)  || defined(SGI)
      if (fetestexcept(FE_DIVBYZERO)) {
         fprintf (stderr, "WARNING : c_videfset: Grid conversion gives an infinity (DIVBYZERO)\n");
      }
      if (fetestexcept(FE_OVERFLOW)) {
         fprintf (stderr, "WARNING : c_videfset: Grid conversion gives an overflow\n");
      }
      if (fetestexcept(FE_UNDERFLOW)) {
         fprintf (stderr, "WARNING : c_videfset: Grid conversion gives an underflow\n");
      }
      if (fetestexcept(FE_INVALID)) {
         fprintf (stderr, "WARNING : c_videfset: Grid conversion gives a NaN (not a number)\n");
      }
#elif defined(_AIX)
      flag = fp_read_flag();
      if (flag & FP_INVALID) {
         fprintf (stderr, "WARNING : c_videfset: Grid conversion gives a NaN (not a number)\n");
      }
      if (flag & FP_OVERFLOW) {
         fprintf (stderr, "WARNING : c_videfset: Grid conversion gives an overflow\n");
      }
      if (flag & FP_UNDERFLOW) {
         fprintf (stderr, "WARNING : c_videfset: Grid conversion gives an underflow\n");
      }
      if (flag & FP_DIV_BY_ZERO) {
         fprintf (stderr, "WARNING : c_videfset: Grid conversion gives an infinity (DIV_BY_ZERO)\n");
      }
#else
      fprintf(stderr,"Exception check is not availlable on this architecture");
#endif
   }

   interp->gInterpIndex_p = (int *) malloc(ni * nj * interp->gGrdDest_p->numLevels * sizeof (int));
   if (!interp->gInterpIndex_p) {
      fprintf (stderr, "c_videfset: malloc failed for gInterpIndex_p\n");
      return -1;
   }

   numInterpSets = src_ijDim = dst_ijDim = ni * nj;

   /*
    * Find array indices between which interpolation will occur.  The
    * output of this routine is input for each of the interpolation
    * routines. The output is in gInterpIndex_p.
    */
   (void) f77name (interp1d_findpos) (&numInterpSets, &interp->gGrdSrc_p->numLevels,
                             &interp->gGrdDest_p->numLevels, &src_ijDim, &dst_ijDim,
                             interp->gCubeSrc_p, interp->gInterpIndex_p, interp->gCubeDest_p);
   return 0;
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
         interp->gViOption |= NEAREST_NEIGHBOUR;
      } else if (strncmp (value, "LINEAR", 6) == 0) {
         interp->gViOption |= LINEAR;
      } else if (strncmp (value, "CUBICWITHDERIVS", 15) == 0) {
         interp->gViOption |= CUBIC_WITH_DERIV;
      } else if (strncmp (value, "CUBIC", 5) == 0) {
         interp->gViOption |= CUBIC_LAGRANGE;
      }
   } else if (strncmp (option, "EXTRAP_DEGREE" ,13) == 0) {
      if (strncmp (value, "CLAMPED", 7) == 0) {
         interp->gViOption |= CLAMPED;
      } else if (strncmp (value, "LAPSERATE", 9) == 0) {
         interp->gViOption |= LAPSERATE;
      }
   } else if (strncmp (option, "VERBOSE", 7) == 0) {
      if (strncmp (value, "YES", 3)  == 0) {
         interp->gViOption |= VERBOSE;
      } else {
         interp->gViOption &= ~VERBOSE;
      }
   } else if (strncmp (option, "CHECKFLOAT", 10) == 0) {
      interp->gViOption |= CHECKFLOAT;
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
   return (interp->gViOption |= option);
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

int c_visint (viInterp *interp,float *stateOut, float *stateIn, float *derivOut, float *derivIn,
              float extrapGuideDown, float extrapGuideUp) {

   int surf = interp->gNi * interp->gNj;
   int extrapEnable = 0;

   /* Assume that everything is set correctly */
   if (!interp->gCubeSrc_p || !interp->gCubeDest_p || !interp->gGrdDest_p || !interp->gInterpIndex_p || (interp->gViOption == 0x000)) {
      return -1;
   }

   if (memcmp (interp->gGrdSrc_p->level_p, interp->gGrdDest_p->level_p, interp->gGrdSrc_p->numLevels * sizeof (float)) == 0) {
      memcpy (stateOut, stateIn, surf * interp->gGrdSrc_p->numLevels);
      printf ("c_videfset: Grids are the same\n");
      return 0;
   }

   /*
    * Interpolation
    * Setting extrapEnableDown andextrapEnableUp to .false.
    * yields 'clamped'
    */
   if (interp->gViOption & NEAREST_NEIGHBOUR) {
      (void) f77name (interp1d_nearestneighbour) (&surf, &interp->gGrdSrc_p->numLevels,
                                         &interp->gGrdDest_p->numLevels, &surf,
                                         &surf, interp->gCubeSrc_p, stateIn,
                                         derivIn, interp->gInterpIndex_p,
                                         interp->gCubeDest_p, stateOut, derivOut,
                                         &extrapEnable,&extrapEnable,&extrapGuideDown, &extrapGuideUp);

   } else if (interp->gViOption & LINEAR) {
      (void) f77name (interp1d_linear) (&surf, &interp->gGrdSrc_p->numLevels,
                               &interp->gGrdDest_p->numLevels, &surf, &surf,
                               interp->gCubeSrc_p, stateIn, derivIn, interp->gInterpIndex_p,
                               interp->gCubeDest_p, stateOut, derivOut,
                               &extrapEnable, &extrapEnable, &extrapGuideDown, &extrapGuideUp);

   } else if (interp->gViOption & CUBIC_WITH_DERIV) {
      if ((derivIn == NULL) && (derivOut == NULL)) {
         fprintf (stderr,
                  "Error : Cubic interpolation with derivatives requested\n");
         return -1;
      }

      (void) f77name (interp1d_cubicwithderivs) (&surf, &interp->gGrdSrc_p->numLevels,
                                        &interp->gGrdDest_p->numLevels, &surf, &surf,
                                        interp->gCubeSrc_p, stateIn, derivIn,
                                        interp->gInterpIndex_p, interp->gCubeDest_p, stateOut, derivOut,
                                        &extrapEnable, &extrapEnable, &extrapGuideDown, &extrapGuideUp);

   } else if (interp->gViOption & CUBIC_LAGRANGE) {
      (void) f77name (interp1d_cubiclagrange) (&surf, &interp->gGrdSrc_p->numLevels,
                                      &interp->gGrdDest_p->numLevels, &surf, &surf,
                                      interp->gCubeSrc_p, stateIn, derivIn,
                                      interp->gInterpIndex_p, interp->gCubeDest_p, stateOut, derivOut,
                                      &extrapEnable, &extrapEnable, &extrapGuideDown, &extrapGuideUp);
   } else {
      fprintf (stderr, "Error : Unknown interpolation algorithm\n");
      return -1;
   }

   if (interp->gViOption & VERBOSE) printf ("c_visint: Interpolation completed\n");

   /* Extrapolation */
   if (interp->gViOption & CLAMPED) {
      /*
       * Do nothing. It has already been done during interpolation.
       * This option exist for compatibility with EzInterpv
       */
      return 0;

   } else if (interp->gViOption & LAPSERATE) {
      extrapEnable = 1;

      (void) f77name (extrap1d_lapserate) (&surf, &interp->gGrdSrc_p->numLevels,
                                  &interp->gGrdDest_p->numLevels, &surf, &surf,
                                  interp->gCubeSrc_p, stateIn, derivIn,
                                  interp->gInterpIndex_p, interp->gCubeDest_p, stateOut, derivOut,
                                  &extrapEnable, &extrapEnable, &extrapGuideDown, &extrapGuideUp);

      if (interp->gViOption & VERBOSE) printf ("c_visint: Lapserate extrapolation completed\n");
   }

   return 0;
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

void vicleanGrid (viInterp *interp) {

   if (interp->gCubeSrc_p) {
      free (interp->gCubeSrc_p);
      interp->gCubeSrc_p = NULL;
   }

   if (interp->gCubeDest_p) {
      free (interp->gCubeDest_p);
      interp->gCubeDest_p = NULL;
   }

   if (interp->gInterpIndex_p) {
      free (interp->gInterpIndex_p);
      interp->gInterpIndex_p = NULL;
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <vigetMASL>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      : Converts each pressure in the cube of vertical levels to meter
 *            above sea.
 *
 * Parametres     :
 *  <grd_p>       : pointer to the grid structure
 *  <height>      : meter above sea
 *  <ni>          : horizontal dimensions
 *  <nj>          : horizontal dimensions
 *
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

void vigetMASL (VerticalGrid * grd_p, float *height, int ni, int nj) {
   int i, j, k, indiceK, indiceJK, surf, indiceIJK;
   float heightTopo = 0.0f;

   i = j = k = indiceK = indiceJK = indiceIJK = 0;
   surf = ni * nj;

   /*
    * Create the 'cube' of vertical levels
    */
   switch (grd_p->gridType) {
      case LVL_PRES:
      case LVL_HYBRID:
      case LVL_SIGMA:
      case LVL_ETA:
      case LVL_UNDEF:
      case LVL_THETA:
         for (i = 0; i < surf * grd_p->numLevels; i++) {
            height[i] = grd_p->gz_p[i] * 10;
         }
         break;

      case LVL_MASL:
         for (k = 0; k < grd_p->numLevels; k++) {
            indiceK = k * surf;
            for (i = 0; i < surf; i++) {
               height[indiceK + i] = grd_p->level_p[k];
            }
         }
         break;

      case LVL_MAGL:
         /*
          * Add the topography to the gz to get the heigth above the sea
          */
         for (k = 0; k < grd_p->numLevels; k++) {
            indiceK = k * surf;
            for (j = 0; j < nj; j++) {
               indiceJK = j * ni;
               for (i = 0; i < ni; i++) {
                  indiceIJK = indiceK + indiceJK + i;
                  /* optimiser la mult par 10 */
                  height[indiceIJK] = (grd_p->gz_p[indiceJK + i] * 10) + grd_p->level_p[k];
               }
            }
         }
         break;

      case LVL_GALCHEN:
         /*
          * height = GALCHEN * (1 - h0/H) + h0
          * Where
          *  - GALCHEN is the level in gal-chen meters
          *  - h0 is the topography
          *  - H is the height of the top of the atmosphere
          */
         for (k = 0; k < grd_p->numLevels; k++) {
            indiceK = k * surf;
            for (j = 0; j < nj; j++) {
               indiceJK = j * ni;
               for (i = 0; i < ni; i++) {
                  indiceIJK = indiceK + indiceJK + i;
                  heightTopo = grd_p->gz_p[indiceJK + i] * 10;
                  height[indiceIJK] = grd_p->level_p[k] * (1 - heightTopo / grd_p->top) + heightTopo;
               }
            }
         }
         break;

      default:
         fprintf (stderr, "vigetMASL: invalid type entry");
         return;
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <vigetLnP>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      : Converts each pressure in the cube of vertical levels to ln(P).
 *            The exception is the 'generic' grid type. In this case, the
 *            'pressure' is copied directly as 'ln(P)', and no conversion
 *            is performed.
 *
 * Parametres     :
 *  <grd_p>       : pointer to the grid structure
 *  <lnP>         : ln pressure, referenced to 1 mb
 *  <ni>          : horizontal dimensions
 *  <nj>          : horizontal dimensions
 *  <pSurf>       : surface pressure for each horiz point, in units of mb
 *  <top>        : ceiling pressure for each horiz point (required only
 *                  for eta units, in units of mb)
 *
 * Retour         :  -1 : Error
 *
 * Remarques : The conversion from P to ln P cannot be left up to the user
 *             and must be performed here because there is one case
 *             (generic) where the conversion has no sense. This routine
 *             automatically does no conversion in this case.
 *
 *             In the case of LVL_HYBRID, the first vertical level
 *             of the grid must be the ceiling. This constraint is imposed
 *             by the function, hybrid_to_pres, which is used to make the
 *             conversion to pressure.
 *
 * Modifications :
 *
 *    Nom         :
 *    Date        :
 *    Description :
 *----------------------------------------------------------------------------
*/

int vigetLnP (VerticalGrid * grd_p, float *lnP, int ni, int nj, float *pSurf,
              float *top) {

   int i, j, k, indiceK, indiceJK, surf;
   float *hybridModel;

   i = j = k = indiceK = indiceJK = 0;
   surf = ni * nj;

   if (!pSurf && grd_p->gridType != LVL_PRES) {
      fprintf (stderr, "c_videfset: Surface pressure is required\n");
      return -1;
   }

   /*
    * Create the 'cube' of vertical levels
    */
   switch (grd_p->gridType) {
      case LVL_PRES:
         for (k = 0; k < grd_p->numLevels; k++) {
            indiceK = k * surf;
            for (j = 0; j < nj; j++) {
               indiceJK = j * ni;
               for (i = 0; i < ni; i++) {
                  lnP[indiceK + indiceJK + i] = (float) log(grd_p->level_p[k]);
               }
            }
         }
         break;

      case LVL_ETA:
         if (!top) {
            fprintf (stderr, "c_videfset: Ceiling pressure is required for grid type ETA\n");
            return -1;
         }

         for (k = 0; k < grd_p->numLevels; k++) {
            indiceK = k * surf;
            for (j = 0; j < nj; j++) {
               indiceJK = j * ni;
               for (i = 0; i < ni; i++) {
                  lnP[indiceK + indiceJK + i] = (float) log(top[indiceJK + i] + (pSurf[indiceJK + i] - top[indiceJK + i])
                         * grd_p->level_p[k]);
               }
            }
         }
         break;
      case LVL_SIGMA:
         for (k = 0; k < grd_p->numLevels; k++) {
            indiceK = k * surf;
            for (j = 0; j < nj; j++) {
               indiceJK = j * ni;
               for (i = 0; i < ni; i++) {
                  lnP[indiceK + indiceJK + i] = (float) log(pSurf[indiceJK + i] * grd_p->level_p[k]);
               }
            }
         }
         break;

      case LVL_HYBRID:
         /* variable bidon : le £$@¬¤¢ de fortran en a besoin */
         i = 1;
         hybridModel = (float *) malloc (grd_p->numLevels * sizeof (float));
         f77name (hybrid_to_pres) (lnP, hybridModel, &grd_p->top, pSurf, &surf,
                                   &i, &grd_p->rCoef, &grd_p->pRef, grd_p->level_p,
                                   &grd_p->numLevels);
         free (hybridModel);
         for (k = 0; k < grd_p->numLevels * surf; k++) {
            lnP[k] = (float) log (lnP[k]);
         }
         break;

      default:
         fprintf (stderr, "vigetLnP: invalid type entry");
         return -1;
   }

   return 0;
}
