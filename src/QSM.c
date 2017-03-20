#include "QSM.h"
#include <stdlib.h>

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

void QSM_Free(TQSM *QSM) {
    if( QSM ) {
        free(QSM->Freq);
        free(QSM->CDF);
        free(QSM);
    }
}

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

void QSM_Add(TQSM *restrict QSM,TQSMSym Sym) {
    QSM->Freq[Sym]++;
    QSM->NextUp--;
    if( !QSM->NextUp )
        QSM_Update(QSM);
}

void QSM_GetFreq(TQSM *restrict QSM,TQSMSym Sym,TQSMFreq *restrict Freq,TQSMFreq *restrict LTFreq,TQSMFreq *restrict MaxFreq) {
    *LTFreq = QSM->CDF[Sym];
    *Freq = QSM->CDF[Sym+1]-QSM->CDF[Sym];
    *MaxFreq = QSM->CDF[QSM->Size];
}

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

