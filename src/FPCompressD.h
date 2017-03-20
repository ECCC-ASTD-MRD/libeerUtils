#ifndef _FPCOMPRESSD_H
#define _FPCOMPRESSD_H

#include <stdio.h>
#include <inttypes.h>

int FPC_InflateD(FILE* FD,double *restrict Data,int NI,int NJ,int NK,size_t CSize);
int FPC_CompressD(FILE* FD,double *restrict Data,int NI,int NJ,int NK);

#endif //_FPCOMPRESSD_H
