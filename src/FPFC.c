/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de compression de nombre flotants
 * Fichier   : FPCompress.ch
 * Creation  : Octobre 2017
 * Auteur    : Eric Legault-Ouellet
 *
 * Description: Floating Point Fast Compress : Compress and inflate floating point
 *              numbers using a faster technique
 *
 * Note:
        This is an implementation of the paper
 *      "Fast lossless compression of scientific floating-point data" written
 *      by P. Ratanaworabhan J. Ke and M. Burtscher, published in
 *      Data Compression Conference 2006 pp. 133-142.
 *
 * License:
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation,
 *    version 2.1 of the License.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the
 *    Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *    Boston, MA 02111-1307, USA.
 *
 *==============================================================================
 */
#include "FPFC.h"

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#define bitsizeof(x)    ((int)sizeof(x)*CHAR_BIT)

const TBufByte LOW_BYTE = 0x0f;
const TBufByte HIGH_BYTE = 0xf0;

/*----------------------------------------------------------------------------
 * Nom      : <LZCNT>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Leading Zero Count : Retourne le nombre de bits significatifs à
 *            0 avant le premier bit à 1.
 *            Ex: 00001000 retournerait 4
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
inline static int LZCNT(uint32_t X) {
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
inline static int LZCNTl(uint64_t X) {
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
 * Nom      : <FPC_BufWriteByteMem>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Flush l'octet du buffer en mémoire
 *
 * Parametres :
 *  <Buf>   : Le buffer
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPFC_BufWriteByteMem(TFPFCBuf *restrict Buf) {
    if( Buf->NB < Buf->Size )
        Buf->Buf[Buf->NB++] = Buf->Byte;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufReadByteMem>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Lit un octet de la mémoire dans l'octet du buffer
 *
 * Parametres :
 *  <Buf>   : Le buffer
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPFC_BufReadByteMem(TFPFCBuf *restrict Buf) {
    if( Buf->NB < Buf->Size )
        Buf->Byte = Buf->Buf[Buf->NB++];
}


/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufWriteByteMem>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Flush le byte du buffer dans le stream
 *
 * Parametres :
 *  <Buf>   : Le buffer
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPFC_BufWriteByteIO(TFPFCBuf *restrict Buf) {
    putc(Buf->Byte,Buf->FD);
    ++Buf->NB;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufReadByteMem>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Lit un octet du stream dans l'octet du buffer
 *
 * Parametres :
 *  <Buf>   : Le buffer
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPFC_BufReadByteIO(TFPFCBuf *restrict Buf) {
    Buf->Byte = (TBufByte)getc(Buf->FD);
    ++Buf->NB;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufNewMem>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Initialise un buffer à partir d'un espace mémoire
 *
 * Parametres :
 *  <Data>  : L'espace mémoire où écrire/lire les données
 *  <N>     : La taille de l'espace mémoire (pour éviter les segfaults)
 *
 * Retour   : Le buffer initialisé
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TFPFCBuf* FPFC_BufNewMem(void* Data,unsigned long N) {
    TFPFCBuf *buf;
    if( (buf=malloc(sizeof(*buf))) ) {
        buf->Buf = Data;
        buf->FD = NULL;
        buf->NB = 0;
        buf->Size = N;

        buf->WriteByte = FPFC_BufWriteByteMem;
        buf->ReadByte = FPFC_BufReadByteMem;

        buf->Half = 0;
    }
    return buf;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufNewIO>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Initialise un buffer à partir d'un espace mémoire
 *
 * Parametres :
 *  <FD>    : Le file descriptor du fichier où lire/écrire les données
 *
 * Retour   : Le buffer initialisé
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TFPFCBuf* FPFC_BufNewIO(FILE* FD) {
    TFPFCBuf *buf;
    if( (buf=malloc(sizeof(*buf))) ) {
        buf->Buf = NULL;
        buf->FD = FD;
        buf->NB = 0;
        buf->Size = 0;

        buf->WriteByte = FPFC_BufWriteByteIO;
        buf->ReadByte = FPFC_BufReadByteIO;

        buf->Half = 0;
    }
    return buf;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufFlush>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Flush le half-byte restant si nécessaire
 *
 * Parametres :
 *  <Buf>   : Le buffer
 *
 * Retour   :
 *
 * Remarques : Seulement utile en écriture
 *
 *----------------------------------------------------------------------------
 */
static void FPFC_BufFlush(TFPFCBuf *restrict Buf) {
    if( Buf->Half ) {
        Buf->WriteByte(Buf);
        Buf->Half=0;
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufWriteHalfByte>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Écrit un half-byte
 *
 * Parametres :
 *  <Buf>   : Le buffer
 *  <HByte> : Half-byte à écrire
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPFC_BufWriteHalfByte(TFPFCBuf *restrict Buf,TBufByte HByte) {
    if( Buf->Half ) {
        Buf->Byte |= HByte&LOW_BYTE;
        Buf->WriteByte(Buf);
        Buf->Half = 0;
    } else {
        Buf->Byte = HByte<<4;
        Buf->Half = 1;
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufWriteHalfBytes>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Écrit des half-bytes
 *
 * Parametres :
 *  <Buf>   : Le buffer
 *  <HBytes>: Half-bytes à écrire
 *  <NHB>   : Number of Half-Bytes
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPFC_BufWriteHalfBytes(TFPFCBuf *restrict Buf,uint64_t HBytes,unsigned int NHB) {
    // Complete the half byte in the buffer
    if( Buf->Half ) {
        Buf->Byte |= (TBufByte)HBytes&LOW_BYTE;
        Buf->WriteByte(Buf);
        Buf->Half = 0;
        HBytes >>= 4;
        --NHB;
    }

    // Write complete bytes
    while( NHB >= 2 ) {
        Buf->Byte = (TBufByte)HBytes;
        Buf->WriteByte(Buf);
        HBytes >>= 8;
        NHB -= 2;
    }

    // Write the last half byte
    if( NHB ) {
        Buf->Byte = (TBufByte)(HBytes<<4);
        Buf->Half = 1;
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufReadHalfByte>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Lit un half-byte
 *
 * Parametres :
 *  <Buf>   : Le buffer
 *
 * Retour   : Le half-byte lu
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static TBufByte FPFC_BufReadHalfByte(TFPFCBuf *restrict Buf) {
    if( Buf->Half ) {
        Buf->Half = 0;
        return Buf->Byte&LOW_BYTE;
    } else {
        Buf->ReadByte(Buf);
        Buf->Half = 1;
        return Buf->Byte>>4;
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufReadHalfBytes>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Lit des half-bytes
 *
 * Parametres :
 *  <Buf>   : Le buffer
 *  <NHB>   : Number of Half-Bytes
 *
 * Retour   : Les half-bytes lus
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static uint64_t FPFC_BufReadHalfBytes(TFPFCBuf *restrict Buf,unsigned int NHB) {
    uint64_t bytes=0ul;
    int bs=0;

    // Read the half byte in the buffer (low bits)
    if( Buf->Half ) {
        bytes = Buf->Byte&LOW_BYTE;
        Buf->Half = 0;
        --NHB;
        bs = 4;
    }

    // Read complete bytes
    while( NHB >= 2 ) {
        Buf->ReadByte(Buf);
        bytes |= ((uint64_t)Buf->Byte)<<bs;
        NHB -= 2;
        bs += 8;
    }

    // Read the last half byte (high bits)
    if( NHB ) {
        Buf->ReadByte(Buf);
        Buf->Half = 1;
        bytes |= ((uint64_t)(Buf->Byte>>4))<<bs;
    }

    return bytes;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_Compressl>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Compresse des double
 *
 * Parametres :
 *  <Data>  : Les données à compresser
 *  <N>     : Le nombre de double à compresser
 *  <Buf>   : Le buffer où écrire les données compressées
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
void FPFC_Compressl(double *restrict Data,unsigned long N,TFPFCBuf *restrict Buf) {
    int64_t         *htbl=NULL,*dpred;
    uint32_t        hash,delta[3]={0u};
    int64_t         val,pred,prev;
    const uint64_t  dmsk=(1ul<<((int)sizeof(int64_t)*8-14))-1ul;
    const uint32_t  hmsk=((1u<<20)-1u);
    unsigned int    d0,d2,lzc;

    // Allocate the hash table's space (2*2^20)
    htbl = calloc((1l<<21),sizeof(*htbl));

    // Calculate the hash
    for(d0=0,hash=0,prev=0; N; --N) {
        // Encode the value as an int
        val = *(int64_t*)Data++;

        // Locate the previous differences that will help with the prediction
        dpred = &htbl[hash*2];

        // Update the deltas and the hash
        // We only keep the 14 MSB (1(sign)+11(exp)+2(MSB mantissa)) of the delta to calculate the hash
        // Note that hash(d0,d1,d2) = lsb0..19(d0 XOR d1<<5 XOR d2<<10)
        // So to get the next hash, we only need to re-XOR d2<<10, shift by 5 and XOR d0
        d2 = (d0+2u)%3;
        hash ^= delta[d0]<<10;
        delta[d0] = (uint32_t)((uint64_t)(val-prev)>>50);
        hash = (hash<<5 ^ delta[d0])&hmsk;
        d0 = d2;

        // Use the previous differences to make the prediction
        pred = prev + dpred[0];
        // Use only the last diff if the two last diffs are too far apart (their first 14 bits are different)
        if( (dpred[0]&dmsk) == (dpred[1]&dmsk) ) {
            pred += dpred[0]-dpred[1];
        }

        // Update the hash table
        dpred[1] = dpred[0];
        dpred[0] = val-prev;

        // XOR the prediction and the value. The better the prediction, the more significant bits will be set to zero
        pred ^= val;

        // Get the leading zeros count we'll encode
        // We encode the LZC using 4 bits and work at half-byte granularity.
        // Ergo, we can't encode more than 60 bits and have to round down to the lowest multiple of 4
        lzc = pred ? LZCNTl(pred)>>2 : 0xf;

        // Write the LZC and the remaining 64-4*LZC bits
        FPFC_BufWriteHalfByte(Buf,(TBufByte)lzc);
        FPFC_BufWriteHalfBytes(Buf,pred,(unsigned int)sizeof(pred)*2u-lzc);

        prev = val;
    }

    FPFC_BufFlush(Buf);

    free(htbl);
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_Inflatel>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Décompresse des double
 *
 * Parametres :
 *  <Data>  : [OUT] Les données décompressés
 *  <N>     : Le nombre de double à décompresser
 *  <Buf>   : Le buffer où lire les données compressées
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
void FPFC_Inflatel(double *restrict Data,unsigned long N,TFPFCBuf *restrict Buf) {
    int64_t         *htbl=NULL,*dpred;
    uint32_t        hash,delta[3]={0u};
    int64_t         val,pred,prev;
    const uint64_t  dmsk=(1ul<<((int)sizeof(int64_t)*8-14))-1ul;
    const uint32_t  hmsk=((1u<<20)-1u);
    unsigned int    d0,d2,lzc;
    uint64_t        n=0;

    // Allocate the hash table's space (2*2^20)
    htbl = calloc((1l<<21),sizeof(*htbl));

    // Calculate the hash
    for(d0=0,hash=0,prev=0; N; --N) {
        // Read the LZC and the remaining bytes
        lzc = FPFC_BufReadHalfByte(Buf);
        val = FPFC_BufReadHalfBytes(Buf,(unsigned int)sizeof(pred)*2-lzc);

        // Locate the previous differences that will help with the prediction
        dpred = &htbl[hash*2];

        // Get what would have been the prediction
        pred = prev + dpred[0];
        // Use only the last diff if the two last diffs are too far apart (their first 14 bits are different)
        if( (dpred[0]&dmsk) == (dpred[1]&dmsk) ) {
            pred += dpred[0]-dpred[1];
        }

        // Get the real value back by re-xoring the prediction
        val ^= pred;

        // Update the hash table
        dpred[1] = dpred[0];
        dpred[0] = val-prev;

        // Update the deltas and the hash
        // We only keep the 14 MSB (1(sign)+11(exp)+2(MSB mantissa)) of the delta to calculate the hash
        // Note that hash(d0,d1,d2) = lsb0..19(d0 XOR d1<<5 XOR d2<<10)
        // So to get the next hash, we only need to re-XOR d2<<10, shift by 5 and XOR d0
        d2 = (d0+2u)%3;
        hash ^= delta[d0]<<10;
        delta[d0] = (uint32_t)((uint64_t)(val-prev)>>50);
        hash = (hash<<5 ^ delta[d0])&hmsk;
        d0 = d2;

        prev = val;
        Data[n++] = *(double*)&val;
    }

    free(htbl);
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_Compress>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Compresse des float
 *
 * Parametres :
 *  <Data>  : Les données à compresser
 *  <N>     : Le nombre de float à compresser
 *  <Buf>   : Le buffer où écrire les données compressées
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
void FPFC_Compress(float *restrict Data,unsigned long N,TFPFCBuf *restrict Buf) {
    int32_t         *htbl=NULL,*dpred;
    uint32_t        hash,delta[3]={0u};
    int32_t         val,pred,prev;
    const uint32_t  dmsk=(1u<<((int)sizeof(int32_t)*8-14))-1u;
    const uint32_t  hmsk=((1u<<20)-1u);
    unsigned int    d0,d2,lzc;

    // Allocate the hash table's space (2*2^20)
    htbl = calloc((1l<<21),sizeof(*htbl));

    // Calculate the hash
    for(d0=0,hash=0,prev=0; N; --N) {
        // Encode the value as an int
        val = *(int32_t*)Data++;

        // Locate the previous differences that will help with the prediction
        dpred = &htbl[hash*2];

        // Update the deltas and the hash
        // We only keep the 11 MSB of the delta (1(sign)+8(exp)+2(MSB mantissa)) to calculate the hash
        // Note that hash(d0,d1,d2) = lsb0..19(d0 XOR d1<<5 XOR d2<<10)
        // So to get the next hash, we only need to re-XOR d2<<10, shift by 5 and XOR d0
        d2 = (d0+2u)%3;
        hash ^= delta[d0]<<10;
        delta[d0] = (uint32_t)(val-prev)>>21;
        hash = (hash<<5 ^ delta[d0])&hmsk;
        d0 = d2;

        // Use the previous differences to make the prediction
        pred = prev + dpred[0];
        // Use only the last diff if the two last diffs are too far apart (their first 14 bits are different)
        if( (dpred[0]&dmsk) == (dpred[1]&dmsk) ) {
            pred += dpred[0]-dpred[1];
        }

        // Update the hash table
        dpred[1] = dpred[0];
        dpred[0] = val-prev;

        // XOR the prediction and the value. The better the prediction, the more significant bits will be set to zero
        pred ^= val;

        // Get the leading zeros count we'll encode
        // We encode the LZC using 4 bits and work at half-byte granularity.
        // Ergo, we can't encode more than 28 bits and have to round down to the lowest multiple of 4
        lzc = pred ? LZCNT(pred)>>2 : 0x7;

        // Write the LZC and the remaining 64-4*LZC bits
        FPFC_BufWriteHalfByte(Buf,(TBufByte)lzc);
        FPFC_BufWriteHalfBytes(Buf,pred,(unsigned int)sizeof(pred)*2u-lzc);

        prev = val;
    }

    FPFC_BufFlush(Buf);

    free(htbl);
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_Inflate>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Décompresse des float
 *
 * Parametres :
 *  <Data>  : [OUT] Les données décompressés
 *  <N>     : Le nombre de float à décompresser
 *  <Buf>   : Le buffer où lire les données compressées
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
void FPFC_Inflate(float *restrict Data,unsigned long N,TFPFCBuf *restrict Buf) {
    int32_t         *htbl=NULL,*dpred;
    uint32_t        hash,delta[3]={0u};
    int32_t         val,pred,prev;
    const uint32_t  dmsk=(1u<<((int)sizeof(int32_t)*8-14))-1u;
    const uint32_t  hmsk=((1u<<20)-1u);
    unsigned int    d0,d2,lzc;
    uint64_t        n=0;

    // Allocate the hash table's space (2*2^20)
    htbl = calloc((1l<<21),sizeof(*htbl));

    // Calculate the hash
    for(d0=0,hash=0,prev=0; N; --N) {
        // Read the LZC and the remaining bytes
        lzc = FPFC_BufReadHalfByte(Buf);
        val = FPFC_BufReadHalfBytes(Buf,(unsigned int)sizeof(pred)*2-lzc);

        // Locate the previous differences that will help with the prediction
        dpred = &htbl[hash*2];

        // Get what would have been the prediction
        pred = prev + dpred[0];
        // Use only the last diff if the two last diffs are too far apart (their first 14 bits are different)
        if( (dpred[0]&dmsk) == (dpred[1]&dmsk) ) {
            pred += dpred[0]-dpred[1];
        }

        // Get the real value back by re-xoring the prediction
        val ^= pred;

        // Update the hash table
        dpred[1] = dpred[0];
        dpred[0] = val-prev;

        // Update the deltas and the hash
        // We only keep the 11 MSB of the delta (1(sign)+8(exp)+2(MSB mantissa)) to calculate the hash
        // Note that hash(d0,d1,d2) = lsb0..19(d0 XOR d1<<5 XOR d2<<10)
        // So to get the next hash, we only need to re-XOR d2<<10, shift by 5 and XOR d0
        d2 = (d0+2u)%3;
        hash ^= delta[d0]<<10;
        delta[d0] = (uint32_t)(val-prev)>>21;
        hash = (hash<<5 ^ delta[d0])&hmsk;
        d0 = d2;

        prev = val;
        Data[n++] = *(float*)&val;
    }

    free(htbl);
}
