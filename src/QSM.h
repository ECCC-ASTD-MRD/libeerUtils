#ifndef _QSM_H
#define _QSM_H

// Implementation of a Quasi-Static Model of Cumulative Distribution Function

#include <stdio.h>
#include <inttypes.h>

typedef uint32_t TQSMSym;   // In theory, because of the way range coding works, Sym needs to contain at most bitsizeof(*data)+1 symbols. A char would be sufficient even for a long double. (129 values)
typedef uint64_t TQSMFreq;  // This is the parameter that limits the total size of the field being encoded
typedef uint16_t TQSMUpF;   // Update freqency type. This should be fairly small.

typedef struct TQSM {
    TQSMFreq    *Freq;      // Frequencies
    TQSMFreq    *CDF;       // Cumulative Distribution Function table
    TQSMSym     Size;       // Size of the tables
    TQSMUpF     UpFreq;     // Update frequency target
    TQSMUpF     CurUpFreq;  // Current update frequency (will enventually reach the target)
    TQSMUpF     NextUp;     // Number of values to add before next update
} TQSM;


TQSM* QSM_New(TQSMSym Size,TQSMUpF UpFreq);
void QSM_Free(TQSM *QSM);
void QSM_Update(TQSM *restrict QSM);
void QSM_Add(TQSM *restrict QSM,TQSMSym Sym);
void QSM_GetFreq(TQSM *restrict QSM,TQSMSym Sym,TQSMFreq *restrict Freq,TQSMFreq *restrict LTFreq,TQSMFreq *restrict MaxFreq);
TQSMSym QSM_GetSym(TQSM *restrict QSM,TQSMFreq *restrict LTFreq,TQSMFreq *restrict Freq);

#endif //_QSM_H
