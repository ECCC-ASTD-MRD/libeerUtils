/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : EZVrInt
 * Creation     : version 1.0 mai 2003
 * Auteur       : Stéphane Gaudreault
 *
 * Description:
 *    Ezvrint is based on EzInterpv, a front end to the
 *    interp1D package created by Jeffrey Blezius. This interface has been
 *    designed to resemble to the ezscint package. Most of the functionality
 *    of the interp1D package is available through the ezvrint interface. The
 *    advantage of using ezvrint is that multiple Interpolations are automated,
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

#include "App.h"
#include "RPN.h"
#include "eerUtils.h"
#include "ZRefInterp.h"

#if defined(__GNUC__) || defined(SGI)
   #include <fenv.h>
#elif defined(_AIX)
   #include <float.h>
   #include <fpxcp.h>
#endif

static TZRefInterp *FInterp;
static unsigned char ZRefInterp_Options=0x0000;

/* Fortran Interface */
/*
int32_t f77name(zrefinterp_free)(void) {
   return(ZRefInterp_Free(FInterp));
}

int32_t f77name(viqkdef)(int32_t *numLevel,int32_t *gridType,float *levelList,float *top,float *pRef,float *rCoef,float *zcoord,float *a,float *b) {
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

int32_t f77name(videfset)(int32_t *ni,int32_t *nj,int32_t *idGrdDest,int32_t *idGrdSrc) {
   return ZRefInterp_Define(FInterp,*ni,*nj,*idGrdDest,*idGrdSrc);
}

int32_t f77name(visetopt)(int32_t *option,int32_t *value) {
   return ZRefInterp_SetOption(FInterp,(const char*)option,(const char*)value);
}

int32_t f77name (visint)(float *stateOut,float *stateIn,float *derivOut,float *derivIn,float *extrapGuideDown,float *extrapGuideUp) {
   return c_visint(FInterp,stateOut,stateIn,derivOut,derivIn,*extrapGuideDown,*extrapGuideUp);
}
*/
/*----------------------------------------------------------------------------
 * Nom      : <ZRefInterp_Free>
 * Creation : Janvier 2013 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Free internal memory arrays
 *
 * Parametres :
 *  <Interp>  : Interpolator
 *
 * Retour     : 
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
int ZRefInterp_Free(TZRefInterp *Interp) {

   if (Interp) {
      if (Interp->Indexes) free(Interp->Indexes); Interp->Indexes=NULL;
      
      ZRef_Free(Interp->ZRefSrc);
      ZRef_Free(Interp->ZRefDest);

      free(Interp);
   }
   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRefInterp_Define>
 * Creation : Janvier 2013 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Define the set of grids to be used for the vertical interpolation.
 *
 * Parametres    :
 *  <ZRefDest>   : Vertical grid of output points
 *  <ZRefSrc>    : Vertical grid of input points
 *  <NI>         : dimension of the horizontal grid in the Interpolation
 *  <NJ>         : other dimension of the horizontal grid in the Interpolation
 *
 * Retour        :  ZRefInterp or NULL for Error
 *
 * Remarques : It is assumed that the vertical levels of the ZRefSrc are in
 *             either ascending or descending order.
 *
 *             The vertical levels of the grid ZRefDest may be in any order.
 *             The determination of the location of each target level is completely
 *             independent of all the others.
 *
 *----------------------------------------------------------------------------
*/
TZRefInterp *ZRefInterp_Define(TZRef *ZRefDest,TZRef *ZRefSrc,const int NI,const int NJ) {

   TZRefInterp *interp;
   int          magl=0;
#if defined(_AIX)
   fpflag_t flag;
#endif
   
   if (!ZRefDest || !ZRefSrc) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Invalid vertical reference\n",__func__);
      return(NULL);
   }

   interp=(TZRefInterp*)malloc(sizeof(TZRefInterp));
   if (!interp) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: malloc failed for Interp\n",__func__);
      return(NULL);
   }
   
   // Increment ZRef's reference count
   ZRef_Incr(ZRefSrc);
   ZRef_Incr(ZRefDest);

   interp->ZRefSrc=ZRefSrc;
   interp->ZRefDest=ZRefDest;
   interp->Indexes=NULL;
   interp->NIJ=NI*NJ;
    
   // Fortran default to global
   FInterp=interp;

   // Check if source and destination are the same
   if (ZRefSrc->Type==ZRefDest->Type && ZRefSrc->LevelNb==ZRefDest->LevelNb && memcmp(ZRefSrc->Levels,ZRefDest->Levels,ZRefSrc->LevelNb*sizeof(float))==0) {
      interp->Same=1;
      if (ZRefInterp_Options & ZRVERBOSE) Lib_Log(APP_LIBEER,APP_INFO,"%s: Same vertical reference\n",__func__);

      return(interp);
   }
   interp->Same=0;

   if (ZRefInterp_Options & ZRCHECKFLOAT) {
      // clear exception flags (plateforme specific) 
#if defined(__GNUC__) || defined(IRIX)
      feclearexcept(FE_ALL_EXCEPT);
#elif defined(_AIX)
      fp_clr_flag(FP_INVALID | FP_OVERFLOW | FP_UNDERFLOW | FP_DIV_BY_ZERO | FP_INEXACT);
#endif
   }

   // Figure out the type of vertical reference (magl or log(p))
   magl=((ZRefDest->Type == LVL_GALCHEN) || (ZRefDest->Type == LVL_MASL) || (ZRefDest->Type == LVL_MAGL) ||
       (ZRefSrc->Type == LVL_GALCHEN)  || (ZRefSrc->Type == LVL_MASL)  || (ZRefSrc->Type == LVL_MAGL));

   // Create cubes of vertical levels ...
   if (!ZRefSrc->PCube) {
      ZRefSrc->PCube=(float*)malloc(interp->NIJ*ZRefSrc->LevelNb*sizeof(float));
      if (!ZRefSrc->PCube) {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: malloc failed for ZRefSrc->PCube\n",__func__);
         ZRefInterp_Free(interp);
         return(NULL);
      }
      if (magl) {
         if (ZRefInterp_Options & ZRVERBOSE) Lib_Log(APP_LIBEER,APP_INFO,"%s: Calculating meter cube for ZRefSrc\n",__func__);
         if (!ZRef_KCube2Meter(ZRefSrc,ZRefSrc->P0,interp->NIJ,ZRefSrc->PCube)) {
            ZRefInterp_Free(interp);
            return(NULL);
         }
      } else {
         if (ZRefInterp_Options & ZRVERBOSE) Lib_Log(APP_LIBEER,APP_INFO,"%s: Calculating pressure cube for ZRefSrc\n",__func__);
         if (!ZRef_KCube2Pressure(ZRefSrc,ZRefSrc->P0,ZRefSrc->P0LS,interp->NIJ,TRUE,ZRefSrc->PCube)) {
            ZRefInterp_Free(interp);
            return(NULL);
         }
      }
   }
   
   if (!ZRefDest->PCube) {
      ZRefDest->PCube=(float*)malloc(interp->NIJ*ZRefDest->LevelNb*sizeof(float));
      if (!ZRefDest->PCube) {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: malloc failed for ZRefDest->PCube\n",__func__);
         ZRefInterp_Free(interp);
         return(NULL);
      }
      if (magl) {
         if (ZRefInterp_Options & ZRVERBOSE) Lib_Log(APP_LIBEER,APP_INFO,"%s: Calculating meter cube for ZRefDest\n",__func__);
         if (!ZRef_KCube2Meter(ZRefDest,ZRefDest->P0,interp->NIJ,ZRefDest->PCube)) {
            ZRefInterp_Free(interp);
            return(NULL);
         }
      } else {
         if (ZRefInterp_Options & ZRVERBOSE) Lib_Log(APP_LIBEER,APP_INFO,"%s: Calculating pressure cube for ZRefDest\n",__func__);
         if (!ZRef_KCube2Pressure(ZRefDest,ZRefDest->P0,ZRefDest->P0LS,interp->NIJ,TRUE,ZRefDest->PCube)) {
            ZRefInterp_Free(interp);
            return(NULL);
         }
      }
   }
   
   if (ZRefInterp_Options & ZRCHECKFLOAT) {
      // Check for exception in the conversion (plateforme specific)
#if defined(__GNUC__)  || defined(SGI)
      if (fetestexcept(FE_DIVBYZERO)) {
         Lib_Log(APP_LIBEER,APP_WARNING,"%s: Grid conversion gives an infinity (DIVBYZERO)\n",__func__);
      }
      if (fetestexcept(FE_OVERFLOW)) {
         Lib_Log(APP_LIBEER,APP_WARNING,"%s: Grid conversion gives an overflow\n",__func__);
      }
      if (fetestexcept(FE_UNDERFLOW)) {
         Lib_Log(APP_LIBEER,APP_WARNING,"%s: Grid conversion gives an underflow\n",__func__);
      }
      if (fetestexcept(FE_INVALID)) {
         Lib_Log(APP_LIBEER,APP_WARNING,"%s: Grid conversion gives a NaN (not a number)\n",__func__);
      }
#elif defined(_AIX)
      flag = fp_read_flag();
      if (flag & FP_INVALID) {
         Lib_Log(APP_LIBEER,APP_WARNING,"%s: Grid conversion gives a NaN (not a number)\n",__func__);
      }
      if (flag & FP_OVERFLOW) {
         Lib_Log(APP_LIBEER,APP_WARNING,"%s: Grid conversion gives an overflow\n",__func__);
      }
      if (flag & FP_UNDERFLOW) {
         Lib_Log(APP_LIBEER,APP_WARNING,"%s: Grid conversion gives an underflow\n",__func__);
      }
      if (flag & FP_DIV_BY_ZERO) {
         Lib_Log(APP_LIBEER,APP_WARNING,"%s: Grid conversion gives an infinity (DIV_BY_ZERO)\n",__func__);
      }
#else
      Lib_Log(APP_LIBEER,APP_WARNING,"%s: Exception check is not availlable on this architecture\n",__func__);
#endif
   }

   // Calculate interpolation indexes
   interp->Indexes=(int*)malloc(interp->NIJ*ZRefDest->LevelNb*sizeof(int));
   if (!interp->Indexes) {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: malloc failed for Indexes\n",__func__);
      ZRefInterp_Free(interp);
      return(NULL);
   }

   /* Find array indices between which Interpolation will occur.  The
    * output of this routine is input for each of the interpolation
    * routines. The output is in Indexes.
    */
#ifdef HAVE_RMN
   (void)f77name(interp1d_findpos)(&interp->NIJ,&ZRefSrc->LevelNb,&ZRefDest->LevelNb,&interp->NIJ,&interp->NIJ,ZRefSrc->PCube,interp->Indexes,ZRefDest->PCube);
#else
   Lib_Log(APP_LIBEER,APP_ERROR,"%s: Need RMNLIB\n",__func__);
#endif
   
   return(interp);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRefInterp_SetOption>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      :  set options in for the Interpolation / extrapolation algorithms.
 * The option and the value to which it is to be set are passed as strings.
 * These string are similar to those used by ezcint
 *
 *
 * Parametres     :
 *  <Option>      : string representing the option to be set:
 *                  "INTERP_DEGREE", "EXTRAP_DEGREE" or "VERBOSE"
 *  <Value>       : string representing the value to which the option is to
 *                  be set.
 *                  Possible values for the option, "INTERP_DEGREE", are:
 *                  "NEAREST", "LINEAR", "CUBICWITHDERIVS" or "CUBIC"
 *                  Possible values for the option, "EXTRAP_DEGREE", are:
 *                  "CLAMPED" or "LAPSERATE"
 *                  Possible values for the option, "VERBOSE", are:
 *                  YES or NO
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int ZRefInterp_SetOption (const char *Option, const char *Value) {

   if (strncmp (Option, "INTERP_DEGREE", 13) == 0) {
      if (strncmp (Value, "NEAREST", 7) == 0) {
         ZRefInterp_Options |= ZRNEAREST_NEIGHBOUR;
      } else if (strncmp (Value, "LINEAR", 6) == 0) {
         ZRefInterp_Options |= ZRLINEAR;
      } else if (strncmp (Value, "CUBICWITHDERIVS", 15) == 0) {
         ZRefInterp_Options |= ZRCUBIC_WITH_DERIV;
      } else if (strncmp (Value, "CUBIC", 5) == 0) {
         ZRefInterp_Options |= ZRCUBIC_LAGRANGE;
      }
   } else if (strncmp (Option, "EXTRAP_DEGREE" ,13) == 0) {
      if (strncmp (Value, "CLAMPED", 7) == 0) {
         ZRefInterp_Options |= ZRCLAMPED;
      } else if (strncmp (Value, "LAPSERATE", 9) == 0) {
         ZRefInterp_Options |= ZRLAPSERATE;
      }
   } else if (strncmp (Option, "VERBOSE", 7) == 0) {
      if (strncmp (Value, "YES", 3)  == 0) {
         ZRefInterp_Options |= ZRVERBOSE;
      } else {
         ZRefInterp_Options &= ~ZRVERBOSE;
      }
   } else if (strncmp (Option, "CHECKFLOAT", 10) == 0) {
      ZRefInterp_Options |= ZRCHECKFLOAT;
   }

   return (ZRefInterp_Options);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRefInterp_SetOptioni>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      :  set options in for the Interpolation / extrapolation algorithms.
 * The option and the value to which it is to be set are passed as 8 bits
 * unsigned integer.
 *
 * Parametres     :
 *  <Option>      : 8 bit unsigned integer representing the value to
 *                  which the option is to be set.
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int ZRefInterp_SetOptioni (const unsigned char Option) {
   return(ZRefInterp_Options|=Option);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRefInterp>
 * Creation : mai 2003 - S. Gaudreault - CMC/CMOE
 *
 * But      : Perform the selected Interpolation
 *
 * Parametres     :
 *  <stateOut>          : output array of the state
 *  <stateIn>           : input array of the state
 *  <derivOut>          : output array of the derivative
 *  <derivIn>           : input array of the derivative
 *  <extrapGuideDown>   : value used for extrapolating below the values of
 *                        the source grid (set in ZRefInterp_Define). The meaning
 *                        of these values depends on the selected
 *                        extrapolation method.
 *  <extrapGuideUp>     : value used for extrapolating above the values of
 *                        the source grid (set in ZRefInterp_Define). The meaning
 *                        of these values depends on the selected
 *                        extrapolation method.
 *
 * Retour         :  -1 : Error
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int ZRefInterp(TZRefInterp *Interp,float *stateOut,float *stateIn,float *derivOut,float *derivIn,float extrapGuideDown,float extrapGuideUp) {

   int surf = Interp->NIJ;
   int extrapEnable = 0;

   if (Interp->Same) {
      memcpy(stateOut,stateIn,Interp->NIJ*Interp->ZRefDest->LevelNb*sizeof(float));
      if (ZRefInterp_Options & ZRVERBOSE) Lib_Log(APP_LIBEER,APP_INFO,"%s: Same vertical reference, copying data (%ix%i)\n",__func__,Interp->NIJ,Interp->ZRefDest->LevelNb);
      return(1);
   }

   /* Assume that everything is set correctly */
   if (!Interp->ZRefSrc || !Interp->ZRefDest || !Interp->ZRefSrc->PCube || !Interp->ZRefDest->PCube || !Interp->Indexes || (ZRefInterp_Options == 0x000)) {
      return(0);
   }

#ifdef HAVE_RMN
   /*
    * Interpolation
    * Setting extrapEnableDown andextrapEnableUp to .false.
    * yields 'clamped'
    */
   if (ZRefInterp_Options & ZRNEAREST_NEIGHBOUR) {
      (void)f77name(interp1d_nearestneighbour)(&surf,&Interp->ZRefSrc->LevelNb,&Interp->ZRefDest->LevelNb,&surf,&surf,
         Interp->ZRefSrc->PCube,stateIn,derivIn,Interp->Indexes,Interp->ZRefDest->PCube,stateOut,derivOut,
         &extrapEnable,&extrapEnable,&extrapGuideDown,&extrapGuideUp);

   } else if (ZRefInterp_Options & ZRLINEAR) {
      (void)f77name(interp1d_linear)(&surf,&Interp->ZRefSrc->LevelNb,&Interp->ZRefDest->LevelNb,&surf,&surf,
         Interp->ZRefSrc->PCube,stateIn,derivIn,Interp->Indexes,Interp->ZRefDest->PCube,stateOut,derivOut,
         &extrapEnable,&extrapEnable,&extrapGuideDown,&extrapGuideUp);

   } else if (ZRefInterp_Options & ZRCUBIC_WITH_DERIV) {
      if ((derivIn == NULL) && (derivOut == NULL)) {
         Lib_Log(APP_LIBEER,APP_ERROR,"%s: Cubic Interpolation with derivatives requested\n",__func__);
         return(0);
      }

      (void)f77name(interp1d_cubicwithderivs)(&surf,&Interp->ZRefSrc->LevelNb,&Interp->ZRefDest->LevelNb,&surf,&surf,
         Interp->ZRefSrc->PCube,stateIn,derivIn,Interp->Indexes,Interp->ZRefDest->PCube,stateOut,derivOut,
         &extrapEnable,&extrapEnable,&extrapGuideDown,&extrapGuideUp);

   } else if (ZRefInterp_Options & ZRCUBIC_LAGRANGE) {
      (void)f77name(interp1d_cubiclagrange)(&surf,&Interp->ZRefSrc->LevelNb,&Interp->ZRefDest->LevelNb,&surf,&surf,
         Interp->ZRefSrc->PCube,stateIn,derivIn,Interp->Indexes,Interp->ZRefDest->PCube,stateOut,derivOut,
         &extrapEnable,&extrapEnable,&extrapGuideDown,&extrapGuideUp);
   } else {
      Lib_Log(APP_LIBEER,APP_ERROR,"%s: Unknown Interpolation algorithm\n",__func__);
      return(0);
   }

   if (ZRefInterp_Options & ZRVERBOSE) Lib_Log(APP_LIBEER,APP_INFO,"%s: Interpolation completed\n",__func__);

   /* Extrapolation */
   if (ZRefInterp_Options & ZRCLAMPED) {
      /*
       * Do nothing. It has already been done during Interpolation.
       * This option exist for compatibility with EzInterpv
       */
      return 0;

   } else if (ZRefInterp_Options & ZRLAPSERATE) {
      extrapEnable = 1;

      (void)f77name(extrap1d_lapserate)(&surf,&Interp->ZRefSrc->LevelNb,&Interp->ZRefDest->LevelNb,&surf,&surf,
         Interp->ZRefSrc->PCube,stateIn,derivIn,Interp->Indexes,Interp->ZRefDest->PCube,stateOut,derivOut,
         &extrapEnable,&extrapEnable,&extrapGuideDown,&extrapGuideUp);

      if (ZRefInterp_Options & ZRVERBOSE) Lib_Log(APP_LIBEER,APP_INFO,"%s: Lapserate extrapolation completed\n",__func__);
   }
#else
   Lib_Log(APP_LIBEER,APP_ERROR,"%s: Need RMNLIB\n",__func__);
#endif
      
   return(1);
}
