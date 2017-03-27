#ifndef _FPCOMPRESS_H
#define _FPCOMPRESS_H

#include <stdio.h>
#include <inttypes.h>

int FPC_Inflate(FILE* FD,float *restrict Data,int NI,int NJ,int NK,size_t CSize);
int FPC_Compress(FILE* FD,float *restrict Data,int NI,int NJ,int NK);

#endif //_FPCOMPRESS_H
