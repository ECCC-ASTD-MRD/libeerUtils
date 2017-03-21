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
TQSM* QSM_New(TQSMSym Size,TQSMUpF UpFreq) {
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

        // Make everything have a default count of 1. That wasy, every symbol starts as likely
        // and we don't have queries with 0 probability before the first couple of updates
        TQSMFreq f;
        for(f=0; f<Size; ++f) qsm->Freq[f]=1;
        for(f=0; f<=Size; ++f) qsm->CDF[f]=f;

        qsm->UpFreq = UpFreq;
        qsm->CurUpFreq = 8;
        qsm->NextUp = qsm->CurUpFreq;
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
    TQSMSym i;

    // Update the update frequency if we are not yet at the target level
    if( QSM->CurUpFreq < QSM->UpFreq ) {
        QSM->CurUpFreq = QSM->CurUpFreq*2<QSM->UpFreq ? QSM->CurUpFreq*2 : QSM->UpFreq;
    }
    QSM->NextUp = QSM->CurUpFreq;

    // Build the cumulative distribution function table
    QSM->CDF[0] = 0;
    for(i=0; i<QSM->Size; ++i) {
        QSM->CDF[i+1] = QSM->CDF[i] + QSM->Freq[i];
    }
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
    QSM->Freq[Sym]++;
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
 *  <MaxFreq: [OUT] Total cumulative frequency
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
void QSM_GetFreq(TQSM *restrict QSM,TQSMSym Sym,TQSMFreq *restrict Freq,TQSMFreq *restrict LTFreq,TQSMFreq *restrict MaxFreq) {
    *LTFreq = QSM->CDF[Sym];
    *Freq = QSM->CDF[Sym+1]-QSM->CDF[Sym];
    *MaxFreq = QSM->CDF[QSM->Size];
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

