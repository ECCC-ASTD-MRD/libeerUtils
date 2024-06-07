#ifndef __OMP_UTILS_H
#define __OMP_UTILS_H

#ifdef _OPENMP
#include "omp.h"

// GCC errors out if a const variable is put into a shared() section of OMP, but intel needs it if it is missing...
#if defined __INTEL_COMPILER || __GNUC__ >= 9
#define OMP_CSTSHR(...) shared(__VA_ARGS__)
#elif defined __GNUC__
#define OMP_CSTSHR(...)
#else // not __GNUC__ nor __INTEL_COMPILER
#define OMP_CSTSHR(...) shared(__VA_ARGS__)
#endif

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

// Check for OpenMP 3.1 or greater
#if _OPENMP >= 201107
#define OMP_ATOMIC_WRITE(Code) _Pragma("omp atomic write") Code
#define OMP_ATOMIC_CAPTURE(Code) _Pragma("omp atomic capture") Code
#else
#define OMP_ATOMIC_WRITE(Code) _Pragma("omp critical") { Code; }
#define OMP_ATOMIC_CAPTURE(Code) _Pragma("omp critical") { Code; }
#endif

#define OMP_PARALLEL_FOR_GUIDED(Min,Max,Fac,Id,BlckCntr) \
    const size_t _OMP_GMIN=(Min); \
    const size_t _OMP_GMAX=(Max); \
    const size_t _OMP_GN=_OMP_GMAX-_OMP_GMIN; \
    const size_t _OMP_NUMT=omp_get_num_threads(); \
    const size_t _OMP_TNUM=omp_get_thread_num(); \
    const size_t _OMP_NBLOCKS=_OMP_NUMT>1?(Fac):1; \
    const size_t _OMP_PN=_OMP_GN/_OMP_NBLOCKS; \
    const size_t _OMP_R=_OMP_GN%_OMP_NBLOCKS; \
    size_t _OMP_BLOCKN=_OMP_TNUM; \
    if( _OMP_TNUM==0 ) { OMP_ATOMIC_WRITE(BlckCntr=_OMP_NUMT); } \
    Id: { \
        const size_t OMP_N=_OMP_PN+(_OMP_R>_OMP_BLOCKN); \
        const size_t OMP_START=_OMP_GMIN+_OMP_BLOCKN*_OMP_PN+(_OMP_BLOCKN<=_OMP_R?_OMP_BLOCKN:_OMP_R); \
        const size_t OMP_END=OMP_START+OMP_N;

#define OMP_PARALLEL_FOR_GUIDED_NEXT(Id,BlckCntr) } \
    OMP_ATOMIC_CAPTURE(_OMP_BLOCKN=BlckCntr++); \
    if( _OMP_BLOCKN<_OMP_NBLOCKS ) goto Id

#else //_OPENMP

#define OMP_PARALLEL_FOR_DECL(Min,Max) \
    const size_t OMP_START=(Min); \
    const size_t OMP_END=(Max); \
    const size_t OMP_N=OMP_END-OMP_START;

#define OMP_PARALLEL_FOR_GUIDED(Min,Max,Fac,Id,BlckCntr) OMP_PARALLEL_FOR_DECL(Min,Max)
#define OMP_PARALLEL_FOR_GUIDED_NEXT(Id,BlckCntr)
#define OMP_ATOMIC_WRITE(Code)   Code
#define OMP_ATOMIC_CAPTURE(Code) Code

#endif //_OPENMP

#ifdef _OPENMP
#define NANO_TIMER_PRINT printf("[Thread %d] [line %d] Elapsed time : %.2f msecs (%f secs) with a resolution of %u nsecs\n",omp_get_thread_num(),_ln,_diff,_diff/1000.0,(unsigned int)_t2.tv_nsec)
#else //_OPENMP
#define NANO_TIMER_PRINT printf("[line %d] Elapsed time : %.2f msecs (%f secs) with a resolution of %u nsecs\n",_ln,_diff,_diff/1000.0,(unsigned int)_t2.tv_nsec)
#endif //_OPENMP

#define NANO_TIMER_START \
    struct timespec _t1,_t2; \
    double _diff; \
    const int _ln=__LINE__; \
    clock_gettime(CLOCK_MONOTONIC,&_t1)

#define NANO_TIMER_STOP \
    clock_gettime(CLOCK_MONOTONIC,&_t2); \
    _diff = (double)((_t2.tv_sec-_t1.tv_sec)*1000000000+_t2.tv_nsec-_t1.tv_nsec)/1000000.0; \
    clock_getres(CLOCK_MONOTONIC,&_t2);

#define NANO_TIMER(Code) { NANO_TIMER_START; { Code; }; NANO_TIMER_STOP; NANO_TIMER_PRINT; }

#endif //__OMP_UTILS_H

