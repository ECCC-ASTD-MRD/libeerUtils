#ifndef _FPFC_H
#define _FPFC_H

#include <stdio.h>

typedef unsigned char TBufByte;

typedef struct TFPFCBuf TFPFCBuf;
typedef struct TFPFCBuf {
    TBufByte        *Buf;   // Buffer where to read/write the half-bytes
    FILE            *FD;    // File descriptor
    unsigned long   NB;     // Number of bytes written to the buffer
    unsigned long   Size;   // Total size of the buffer (in half-bytes)

    // Buffer functions
    void    (*WriteByte)    (TFPFCBuf*);
    void    (*ReadByte)     (TFPFCBuf*);

    TBufByte    Byte;   // Half byte temp storage
    TBufByte    Half;   // Flag indicating if there is a half byte in the storage
} TFPFCBuf;

#define FPFC_BufFree(Buf) if(Buf){free(Buf);Buf=NULL;}
TFPFCBuf* FPFC_BufNewMem(void* Data,unsigned long N);
TFPFCBuf* FPFC_BufNewIO(FILE* FD);

// Double functions
void FPFC_Compressl(double *restrict Data,unsigned long N,TFPFCBuf *restrict Buf);
void FPFC_Inflatel(double *restrict Data,unsigned long N,TFPFCBuf *restrict Buf);

// Float functions
void FPFC_Compress(float *restrict Data,unsigned long N,TFPFCBuf *restrict Buf);
void FPFC_Inflate(float *restrict Data,unsigned long N,TFPFCBuf *restrict Buf);

#endif // _FPFC_H
