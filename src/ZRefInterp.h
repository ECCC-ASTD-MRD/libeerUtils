/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : EZVrInt
 * Creation     : version 1.0 mai 2003
 * Auteur       : Stéphane Gaudreault
 * Revision     : $Id$
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

#ifndef _ZRefInterp_h
#define _ZRefInterp_h

#include "ZRef.h"

/* Interpolation */
#define ZRNEAREST_NEIGHBOUR 0x001
#define ZRLINEAR            0x002
#define ZRCUBIC_WITH_DERIV  0x004
#define ZRCUBIC_LAGRANGE    0x008

/* Extrapolation */
#define ZRCLAMPED    0x010
#define ZRLAPSERATE  0x020

/* Other options */
#define ZRVERBOSE    0x040

/* check for float exception (will do nothing on SX6) */
#define ZRCHECKFLOAT 0x080

typedef struct TZRefInterp {
   TZRef         *ZRefSrc,*ZRefDest;       // Source and destination vertical references
   int           *Indexes;                 // Interpolation array indices
   int           NIJ;                      // 2D dimensions
   int           Same;                     // Flag indicating source and destination are the same
} TZRefInterp;


int          ZRefInterp_Free(TZRefInterp *Interp);
void         ZRefInterp_Clear(TZRefInterp *Interp);
TZRefInterp *ZRefInterp_Define(TZRef *ZRefDest,TZRef *ZRefSrc,const int NI,const int NJ);
int          ZRefInterp(TZRefInterp *Interp,float *stateOut,float *stateIn,float *derivOut,float *derivIn,float extrapGuideDown,float extrapGuideUp);
int          ZRefInterp_SetOption(const char *Option,const char *Value);
int          ZRefInterp_SetOptioni(const unsigned char option);

#endif
