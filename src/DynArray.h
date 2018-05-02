#ifndef _DYNARRAY_H
#define _DYNARRAY_H

#include <string.h>

typedef struct DynArray {
   void     *Arr;    // Data of the array
   size_t   Size;    // Size of the array (bytes)
   size_t   N;       // Number of bytes used in the array
} DynArray;

int DynArray_Init(DynArray *restrict DA,size_t Size);
void DynArray_Free(DynArray *restrict DA);
int DynArray_Push(DynArray *restrict DA,const void *restrict Vals,size_t Size);
int DynArray_Pushd(DynArray *restrict DA,double Val);
int DynArray_Pushf(DynArray *restrict DA,float Val);
int DynArray_Pushi(DynArray *restrict DA,int Val);

#endif // _DYNARRAY_H
