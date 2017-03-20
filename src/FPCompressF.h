#ifndef _FPCOMPRESSF_H
#define _FPCOMPRESSF_H

#include <stdio.h>
#include <inttypes.h>

int FPC_InflateF(FILE* FD,float *restrict Data,int NI,int NJ,int NK,size_t CSize);
int FPC_CompressF(FILE* FD,float *restrict Data,int NI,int NJ,int NK);

#endif //_FPCOMPRESSF_H
