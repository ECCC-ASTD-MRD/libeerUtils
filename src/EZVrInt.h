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
 *   Nom         : -
 *   Date        : -
 *   Description : -
 *
 *==============================================================================
 */

#ifndef _EZVrInt_h
#define _EZVrInt_h

#include "eerUtils.h"

/* Interpolation */
#define VINEAREST_NEIGHBOUR 0x001
#define VILINEAR            0x002
#define VICUBIC_WITH_DERIV  0x004
#define VICUBIC_LAGRANGE    0x008

/* Extrapolation */
#define VICLAMPED    0x010
#define VILAPSERATE  0x020

/* Other options */
#define VIVERBOSE    0x040

/* check for float exception (will do nothing on SX6) */
#define VICHECKFLOAT 0x080

#define VIGRIDLENGTH 256

typedef struct viInterp {
   TZRef          ZRefs[VIGRIDLENGTH];
   TZRef         *ZRefSrc,*ZRefDest;
   float         *gCubeSrc_p,*gCubeDest_p;
   int           *gInterpIndex_p;
   unsigned char gViOption;
   int           gNi, gNj;
   int           index,last,same;
} viInterp;

viInterp* c_videfine (void);
int c_viundefine(viInterp *interp);
int c_viqkdef(viInterp *interp,TZRef *ZRef);
int c_videfset(viInterp *interp,const int ni,const int nj,int idGrdDest,int idGrdSrc);
int c_visetopt (viInterp *interp,const char *option, const char *value);
int c_visetopti (viInterp *interp,const unsigned char option);
int c_visint(viInterp *interp,float *stateOut,float *stateIn,float *derivOut,float *derivIn,float extrapGuideDown,float extrapGuideUp);

/*Interface Fortran*/
wordint f77name (videfine)   ();
wordint f77name (viundefine) ();
wordint f77name (viqkdef)    (wordint *, wordint *, ftnfloat *, ftnfloat *, ftnfloat *, ftnfloat *, ftnfloat *, ftnfloat *, ftnfloat *);
wordint f77name (viqkdefset) (wordint *, wordint *, wordint *, wordint *);
wordint f77name (visetopt)   (wordint *, wordint *);
wordint f77name (visint)     (ftnfloat *, ftnfloat *, ftnfloat *, ftnfloat *, ftnfloat *, ftnfloat *);

#endif
