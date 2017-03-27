/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de compression de nombre flotants
 * Fichier   : QSM.c
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
#include "QSM.h"
#include <stdlib.h>

/*----------------------------------------------------------------------------
 * Nom      : <QSM_New>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Retourne une structure TQSM initialisée
 *
 * Parametres :
 *  <Size>  : Maximum number of different symbols
 *  <UpFreq>: Target update frequency
 *
 * Retour   : Une structure TQSM initialisée ou NULL en cas d'erreur
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TQSM* QSM_New(TQSMSym Size,TQSMFreq TotFreq,TQSMUpF UpFreq) {
    TQSM *qsm = malloc(sizeof(*qsm));

    if( qsm ) {
        if( !(qsm->Freq=malloc(Size*sizeof(*qsm->Freq))) ) {
            free(qsm);
            return NULL;
        }
        if( !(qsm->CDF=malloc((Size+1)*sizeof(*qsm->CDF))) ) {
            free(qsm->Freq);
            free(qsm);
            return NULL;
        }
        qsm->Size = Size;

        // Since the total cumulative frequency is fixed, distribute that total frequency over
        // all the symbols (note that the integer division is taken into account here)
        TQSMFreq i,f=TotFreq/Size,sizei=Size-TotFreq%Size;
        for(i=0; i<sizei; ++i)  qsm->Freq[i] = f;
        for(++f; i<Size; ++i)   qsm->Freq[i] = f;

        qsm->NextIncr = 0;
        qsm->UpFreq = UpFreq;
        qsm->CurUpFreq = 8;

        QSM_Update(qsm);
    }

    return qsm;
}

/*----------------------------------------------------------------------------
 * Nom      : <QSM_Free>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Libère une structure TQSM
 *
 * Parametres :
 *  <QSM>   : Structure à libérer
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
void QSM_Free(TQSM *QSM) {
    if( QSM ) {
        free(QSM->Freq);
        free(QSM->CDF);
        free(QSM);
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <QSM_Update>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Met à jour la CDF (Cumulative Distribution Function)
 *
 * Parametres :
 *  <QSM>   : Modèle
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
void QSM_Update(TQSM *restrict QSM) {
    TQSMFreq *restrict cdf = QSM->CDF;
    TQSMFreq *restrict freq = QSM->Freq;
    TQSMFreq newfreq=0;
    TQSMSym i;

    // If we just need to increment the increment
    if( QSM->NextIncr ) {
        ++QSM->Incr;
        QSM->NextUp = QSM->NextIncr;
        QSM->NextIncr = 0;
        return;
    }

    // Update the update frequency if we are not yet at the target level
    if( QSM->CurUpFreq < QSM->UpFreq ) {
        QSM->CurUpFreq = QSM->CurUpFreq*2<QSM->UpFreq ? QSM->CurUpFreq*2 : QSM->UpFreq;
    }
    QSM->NextUp = QSM->CurUpFreq;

    // Build the cumulative distribution function table
    QSM->CDF[0] = 0;
    for(i=0; i<QSM->Size; ++i) {
        // Update the cumulative distribution function
        cdf[i+1] = cdf[i] + freq[i];

        // Divide the frequencies by 2 rounded up, so that we don't bust our maximum frequency
        // (rounded up with a minimum of 1, since we don't want any null frequency. After all,
        // if we ask for a symbol, it has to be present in the first place)
        freq[i] = (freq[i]>>1)|1;
        newfreq += freq[i];
    }

    // Get the number we need to add to get back to our fixed total frequency
    newfreq = cdf[QSM->Size]-newfreq;

    // Given the number of steps we have until the next update, calculate the increment to add to the frequency
    QSM->Incr = newfreq/QSM->NextUp;
    // Number of values to add until the next update, but with an incremented Incr (due to integer division)
    QSM->NextIncr = newfreq*QSM->NextUp;
    QSM->NextUp -= QSM->NextIncr;
}

/*----------------------------------------------------------------------------
 * Nom      : <QSM_Add>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Incrémente la fréquence d'un symbole
 *
 * Parametres :
 *  <QSM>   : Modèle
 *  <Sym>   : Symbole dont on doit incrémenter la fréquence
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
void QSM_Add(TQSM *restrict QSM,TQSMSym Sym) {
    QSM->Freq[Sym] += QSM->Incr;
    QSM->NextUp--;
    if( !QSM->NextUp )
        QSM_Update(QSM);
}

/*----------------------------------------------------------------------------
 * Nom      : <QSM_GetFreq>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Retourne la fréquence et fréquence cumulée d'un symbole
 *
 * Parametres :
 *  <QSM>   : Modèle
 *  <Sym>   : Symbole dont on veut les fréquences
 *  <Freq>  : [OUT] Fréquence du symbole
 *  <LTFreq>: [OUT] Lesser-Than cumulative frequency
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
void QSM_GetFreq(TQSM *restrict QSM,TQSMSym Sym,TQSMFreq *restrict Freq,TQSMFreq *restrict LTFreq) {
    *LTFreq = QSM->CDF[Sym];
    *Freq = QSM->CDF[Sym+1]-QSM->CDF[Sym];
}

/*----------------------------------------------------------------------------
 * Nom      : <QSM_GetSym>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Retourne le symbole correspondant à la fréquence cumulée donnée
 *
 * Parametres :
 *  <QSM>   : Modèle
 *  <LTFreq>: [IN/OUT] Lesser-Than cumulative frequency
 *  <Freq>  : [OUT] Fréquence du symbole
 *
 * Retour   : Le symbole correspondant à la fréquence cumulée donnée
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TQSMSym QSM_GetSym(TQSM *restrict QSM,TQSMFreq *restrict LTFreq,TQSMFreq *restrict Freq) {
    TQSMFreq *restrict cdf = QSM->CDF;
    TQSMSym from=0,to=QSM->Size,mid;

    // Binary search to find the cumulative frequency
    while( from+1 < to ) {
        mid = (from+to)/2;
        if( *LTFreq < cdf[mid] ) {
            to = mid;
        } else {
            from = mid;
        }
    }

    // Set the "real" cumulative frequency and frequency
    *LTFreq = cdf[from];
    *Freq = cdf[from+1] - *LTFreq;
    return from;
}

inline TQSMFreq QSM_GetTotFreq(TQSM *restrict QSM) {
    return QSM->CDF[QSM->Size];
}
