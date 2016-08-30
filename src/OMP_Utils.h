#ifndef __OMP_UTILS_H
#define __OMP_UTILS_H

#ifdef _OPENMP
#include "omp.h"

// Defines OMP_N (number of iterations for this thread), OMP_START (starting index for this thread) and OMP_END (OMP_START+OMP_N)
#define OMP_PARALLEL_FOR_DECL(Min,Max) \
    const size_t _OMP_GMIN=(Min); \
    const size_t _OMP_GMAX=(Max); \
    const size_t _OMP_GN=_OMP_GMAX-_OMP_GMIN; \
    const size_t _OMP_NUMT=omp_get_num_threads(); \
    const size_t _OMP_TNUM=omp_get_thread_num(); \
    const size_t _OMP_PN=_OMP_GN/_OMP_NUMT; \
    const size_t _OMP_R=_OMP_GN%_OMP_NUMT; \
    const size_t OMP_N=_OMP_PN+(_OMP_R>_OMP_TNUM); \
    const size_t OMP_START=_OMP_GMIN+_OMP_TNUM*_OMP_PN+(_OMP_TNUM<=_OMP_R?_OMP_TNUM:_OMP_R); \
    const size_t OMP_END=OMP_START+OMP_N;

//#define OMP_PARALLEL_FOR_INIT(Min,Max,OMP_DIRS) \
//_Pragma("omp parallel "OMP_DIRS) \
//{ \
//    OMP_PARALLEL_FOR_DECL(Min,Max);

#else //_OPENMP

#define OMP_PARALLEL_FOR_DECL(Min,Max) \
    const size_t OMP_START=(Min); \
    const size_t OMP_END=(Max); \
    const size_t OMP_N=OMP_END-OMP_START;

//#define OMP_PARALLEL_FOR_INIT(Min,Max,OMP_DIRS) { \
//    OMP_PARALLEL_FOR_DECL(Min,Max);

#endif //_OPENMP

//#define OMP_PARALLEL_FOR_END }

#define NANO_TIMER(Code) { \
    struct timespec _t1,_t2; \
    double _diff; \
    const int _ln=__LINE__; \
    clock_gettime(CLOCK_MONOTONIC,&_t1); \
    Code; \
    clock_gettime(CLOCK_MONOTONIC,&_t2); \
    _diff = (double)((_t2.tv_sec-_t1.tv_sec)*1000000000+_t2.tv_nsec-_t1.tv_nsec); \
    clock_getres(CLOCK_MONOTONIC,&_t2); \
    printf("[line %d] Elapsed time : %.2f msecs (%f secs) with a resolution of %u nsecs\n",_ln,_diff/1000000.0,_diff/1000000000.0,(unsigned int)_t2.tv_nsec); \
}

#endif //__OMP_UTILS_H

