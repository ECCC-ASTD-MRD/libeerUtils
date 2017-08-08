#ifndef _SM_H
#define _SM_H

#include <stddef.h>

#define SM_FREE(Addr) if(Addr) { SM_Free(Addr); Addr=NULL; }

size_t SM_RoundPageSize(size_t Size);
void* SM_Alloc(size_t Size);
void* SM_Calloc(size_t Num,size_t Size);
int SM_Sync(void *Addr,int NodeHeadRank);
void SM_Free(void *Addr);

#endif // _SM_H
