#ifndef BITSTUFF_H
#define BITSTUFF_H

#if defined __INTEL_COMPILER
#include <immintrin.h>
#endif

#define bitsizeof(x)    ((int)sizeof(x)*CHAR_BIT)

/*----------------------------------------------------------------------------
 * Nom      : <lzcnt>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Leading Zero Count : Retourne le nombre de bits significatifs à
 *            0 avant le premier bit à 1.
 *            Ex: 00001100 retournerait 4
 *
 * Parametres :
 *  <X>     : La valeur dont on veut le LZC. La valeur doit être non-nulle!
 *
 * Retour   : Le nombre de 0 significatifs
 *
 * Remarques : If X is 0, the result is undefined
 *
 *----------------------------------------------------------------------------
 */
inline static int lzcnt(unsigned int X) {
#if defined __INTEL_COMPILER
    return (int)_lzcnt_u32(X);
#elif defined __GNUC__
    return (int)__builtin_clz(X);
#else
    int k=1;
    while( X>>=1 ) ++k;
    return bitsizeof(X)-k;
#endif
}
inline static int lzcntl(unsigned long X) {
#if defined __INTEL_COMPILER
    return (int)_lzcnt_u64(X);
#elif defined __GNUC__
    return (int)__builtin_clzl(X);
#else
    int k=1;
    while( X>>=1 ) ++k;
    return bitsizeof(X)-k;
#endif
}

/*----------------------------------------------------------------------------
 * Nom      : <tzcnt>
 * Creation : Juillet 2021 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Trailing Zero Count : Retourne le nombre de bits à 0 avant le
 *            premier bit le moins significatif (LSB) à 1.
 *            Ex: 00001100 retournerait 2
 *
 * Parametres :
 *  <X>     : La valeur dont on veut le TZC. La valeur doit être non-nulle!
 *
 * Retour   : Le nombre de 0 avant le LSB (least significant bit)
 *
 * Remarques : If X is 0, the result is undefined
 *
 *----------------------------------------------------------------------------
 */
inline static int tzcnt(unsigned int X) {
#if defined __INTEL_COMPILER
    return (int)_tzcnt_u32(X);
#elif defined __GNUC__
    return (int)__builtin_ctz(X);
#else
    int k=0;
    while( !(X&1u) ) {
       X>>=1;
       ++k;
    }
    return k;
#endif
}
inline static int tzcntl(unsigned long X) {
#if defined __INTEL_COMPILER
    return (int)_tzcnt_u64(X);
#elif defined __GNUC__
    return (int)__builtin_ctzl(X);
#else
    int k=0;
    while( !(X&1u) ) {
       X>>=1;
       ++k;
    }
    return k;
#endif
}


/*----------------------------------------------------------------------------
 * Nom      : <bsr>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Bit Scan Reverse : retourne la position du bit le plus
 *            significatif à 1.
 *            Ex: 00001110 retournerait 3
 *
 * Parametres :
 *  <X>     : La valeur dont on veut le MSB. La valeur doit être non-nulle!
 *
 * Retour   : la position du bit le plus significatif à 1
 *
 * Remarques : If X is 0, the result is undefined
 *
 *----------------------------------------------------------------------------
 */
inline static int bsr(unsigned int X) {
    int k;
#if __x86_64__ || defined __i386__
    __asm__("bsr %1,%0":"=r"(k):"rm"(X));
#elif defined __INTEL_COMPILER
    k = _bit_scan_reverse(x);
#elif defined __GNUC__
    k = (int)bitsizeof(x)-1-lzcnt(X);
#else
    k=0;
    while( X>>=1 ) ++k;
#endif
    return k;
}
inline static int bsrl(unsigned long X) {
#if __x86_64__ || defined __i386__
    long k;
    __asm__("bsr %1,%0":"=r"(k):"rm"(X));
    return (int)k;
#elif defined __INTEL_COMPILER || defined __GNUC__
    return (int)bitsizeof(x)-1-lzcntl(X);
#else
    int k=0;
    while( X>>=1 ) ++k;
    return k;
#endif
}

/*----------------------------------------------------------------------------
 * Nom      : <popcnt>
 * Creation : Juillet 2021 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Pop count : retourne le nombre de bit à 1
 *            Ex: 01001100 retournerait 3
 *
 * Parametres :
 *  <X>     : La valeur dont on veut le popcount.
 *
 * Retour   : Le nombre de bits à 1
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
inline static int popcnt(unsigned int X) {
#if defined __INTEL_COMPILER
    return (int)_mm_popcnt_u32(X);
#elif defined __GNUC__
    return (int)__builtin_popcount(X);
#else
    int k=0;
    while( X ) {
       k+=X&1u;
       X>>=1;
    }
    return k;
#endif
}

inline static int popcntl(unsigned long X) {
#if defined __INTEL_COMPILER
    return (int)_mm_popcnt_u64(X);
#elif defined __GNUC__
    return (int)__builtin_popcountl(X);
#else
    int k=0;
    while( X ) {
       k+=X&1ul;
       X>>=1;
    }
    return k;
#endif
}

#endif // BITSTUFF_H
