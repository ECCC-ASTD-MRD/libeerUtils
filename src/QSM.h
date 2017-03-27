/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de compression de nombre flotants
 * Fichier   : QSM.h
 * Creation  : Mars 2017
 * Auteur    : Eric Legault-Ouellet
 *
 * Description: Quasi-static Model implementation of a cumulative distribution
 *              function
 *
 * Note:
 *      Based on the model implementation of M. Schindler,
 *      Range Encoder version 1. 3, 2000.
 *      URL http://www.compressconsult.com/rangecoder/.
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
#ifndef _QSM_H
#define _QSM_H

// Implementation of a Quasi-Static Model of Cumulative Distribution Function

#include <stdio.h>
#include <inttypes.h>

typedef uint32_t TQSMSym;   // In theory, because of the way range coding works, Sym needs to contain at most bitsizeof(*data)+1 symbols. A char would be sufficient even for a long double. (129 values)
typedef uint32_t TQSMFreq;  // This is the parameter that limits the total size of the field being encoded
typedef uint32_t TQSMUpF;   // Update freqency type. This should be fairly small.

typedef struct TQSM {
    TQSMFreq    *Freq;      // Frequencies
    TQSMFreq    *CDF;       // Cumulative Distribution Function table
    TQSMSym     Size;       // Size of the tables
    TQSMFreq    Incr;       // Increment to add to the frequency (since our total frequency is stable, the frequencies must be proportional to the number of items in the model)
    TQSMUpF     NextIncr;   // Number of values to add until the next update, but with an incremented Incr (due to integer division)
    TQSMUpF     UpFreq;     // Update frequency target
    TQSMUpF     CurUpFreq;  // Current update frequency (will enventually reach the target)
    TQSMUpF     NextUp;     // Number of values to add before next update
} TQSM;


TQSM* QSM_New(TQSMSym Size,TQSMFreq TotFreq,TQSMUpF UpFreq);
void QSM_Free(TQSM *QSM);
void QSM_Update(TQSM *restrict QSM);
void QSM_Add(TQSM *restrict QSM,TQSMSym Sym);
void QSM_GetFreq(TQSM *restrict QSM,TQSMSym Sym,TQSMFreq *restrict Freq,TQSMFreq *restrict LTFreq);
TQSMSym QSM_GetSym(TQSM *restrict QSM,TQSMFreq *restrict LTFreq,TQSMFreq *restrict Freq);

#endif //_QSM_H
