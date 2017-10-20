/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de compression de nombre flottants
 * Fichier   : FPFC.c
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
#include "App.h"

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#define bitsizeof(x)    ((int)sizeof(x)*CHAR_BIT)

const TBufByte LOW_BYTE = 0x0f;
const TBufByte HIGH_BYTE = 0xf0;


typedef struct TFPFCBuf TFPFCBuf;
typedef struct TFPFCBuf {
#ifdef FPFC_USE_MEM_IO
    TBufByte    *Buf;   // Buffer where to read/write the half-bytes
    size_t      Size;   // Total size of the buffer (in half-bytes)
#else //FPFC_USE_MEM_IO
    FILE        *FD;    // File descriptor
#endif //FPFC_USE_MEM_IO
    size_t      NB;     // Number of bytes written to the buffer
    TBufByte    Byte;   // Half byte temp storage
    TBufByte    Half;   // Flag indicating if there is a half byte in the storage
} TFPFCBuf;

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
 * Nom      : <FPC_BufWriteByte>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Flush l'octet au IO
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
static void FPFC_BufWriteByte(TFPFCBuf *restrict Buf) {
#ifdef FPFC_USE_MEM_IO
    if( Buf->NB < Buf->Size )
        Buf->Buf[Buf->NB] = Buf->Byte;
    ++Buf->NB;
#else
    putc(Buf->Byte,Buf->FD);
    ++Buf->NB;
#endif
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_BufReadByte>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Lit un octet du IO
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
static void FPFC_BufReadByte(TFPFCBuf *restrict Buf) {
#ifdef FPFC_USE_MEM_IO
    if( Buf->NB < Buf->Size )
        Buf->Byte = Buf->Buf[Buf->NB];
    ++Buf->NB;
#else
    Buf->Byte = (TBufByte)getc(Buf->FD);
    ++Buf->NB;
#endif
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
        FPFC_BufWriteByte(Buf);
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
        FPFC_BufWriteByte(Buf);
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
        FPFC_BufWriteByte(Buf);
        Buf->Half = 0;
        HBytes >>= 4;
        --NHB;
    }

    // Write complete bytes
    while( NHB >= 2 ) {
        Buf->Byte = (TBufByte)HBytes;
        FPFC_BufWriteByte(Buf);
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
        FPFC_BufReadByte(Buf);
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
        FPFC_BufReadByte(Buf);
        bytes |= ((uint64_t)Buf->Byte)<<bs;
        NHB -= 2;
        bs += 8;
    }

    // Read the last half byte (high bits)
    if( NHB ) {
        FPFC_BufReadByte(Buf);
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
 *  <Data>      : Les données à compresser
 *  <N>         : Le nombre de double à compresser
 *  <CData>     : [?] Le buffer où écrire les données compressées
 *  <CBufSize>  : [?] La taille du buffer de données compressées
 *  <FD>        : [?] Le handle du fichier où écrire les données compressées
 *  <CSize>     : [OUT] La taille (Bytes) des données compressées
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int FPFC_Compressl(double *restrict Data,size_t N,FPFC_IO_PARAM,size_t *CSize) {
    int64_t         *htbl=NULL,*dpred;
    uint32_t        hash,delta[3]={0u},d2;
    int64_t         val,pred,prev;
    const uint64_t  dmsk=(1ul<<((int)sizeof(int64_t)*8-14))-1ul;
    const uint32_t  hmsk=((1u<<20)-1u);
    unsigned int    d,lzc;
    unsigned long   nb=N*sizeof(*Data);

#ifdef FPFC_USE_MEM_IO
    TFPFCBuf buf = (TFPFCBuf){CData,CBufSize,0,0,0};
#else //FPFC_USE_MEM_IO
    TFPFCBuf buf = (TFPFCBuf){FD,0,0,0};
#endif //FPFC_USE_MEM_IO

    // Allocate the hash table's space (2*2^20)
    htbl = calloc((1l<<21),sizeof(*htbl));

    // Loop on the data to compress
    for(d=0,d2=0,hash=0,prev=0; N; --N) {
        // Encode the value as an int
        val = *(int64_t*)Data++;

        // Locate the previous differences that will help with the prediction
        dpred = &htbl[hash*2];

        // Update the deltas and the hash
        // We only keep the 14 MSB (1(sign)+11(exp)+2(MSB mantissa)) of the delta to calculate the hash
        // Note that hash(d0,d1,d2) = lsb0..19(d0 XOR d1<<5 XOR d2<<10)
        // So to get the next hash, we only need to re-XOR d2<<10, shift by 5 and XOR d0
        hash ^= d2<<10;
        d2 = delta[d];
        delta[d] = (uint32_t)((uint64_t)(val-prev)>>50);
        hash = (hash<<5 ^ delta[d])&hmsk;
        d = (d+1)&1;

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
        FPFC_BufWriteHalfByte(&buf,(TBufByte)lzc);
        FPFC_BufWriteHalfBytes(&buf,pred,(unsigned int)sizeof(pred)*2u-lzc);

        prev = val;
    }

    free(htbl);
    FPFC_BufFlush(&buf);

    App_Log(DEBUG,"Compression ratio : %.4f\n",(double)buf.NB/nb);
    if( buf.NB >= nb ) {
        App_Log(WARNING,"Compressed data is larger than original (compressed=%ld ori=%ld)\n",buf.NB,nb);
        return APP_ERR;
    }

    *CSize = buf.NB;
    return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_Inflatel>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Décompresse des double
 *
 * Parametres :
 *  <Data>      : [OUT] Les données décompressés
 *  <N>         : Le nombre de double à décompresser
 *  <CData>     : [?] Le buffer où lire les données compressées
 *  <CBufSize>  : [?] La taille du buffer de données compressées
 *  <FD>        : [?] Le handle du fichier où lire les données compressées
 *
 * Retour   : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int FPFC_Inflatel(double *restrict Data,size_t N,FPFC_IO_PARAM) {
    int64_t         *htbl=NULL,*dpred;
    uint32_t        hash,delta[3]={0u},d2;
    int64_t         val,pred,prev;
    const uint64_t  dmsk=(1ul<<((int)sizeof(int64_t)*8-14))-1ul;
    const uint32_t  hmsk=((1u<<20)-1u);
    unsigned int    d,lzc;
    uint64_t        n=0;

#ifdef FPFC_USE_MEM_IO
    TFPFCBuf buf = (TFPFCBuf){CData,CBufSize,0,0,0};
#else //FPFC_USE_MEM_IO
    TFPFCBuf buf = (TFPFCBuf){FD,0,0,0};
#endif //FPFC_USE_MEM_IO

    // Allocate the hash table's space (2*2^20)
    htbl = calloc((1l<<21),sizeof(*htbl));

    // Loop on the data to inflate
    for(d=0,d2=0,hash=0,prev=0; N; --N) {
        // Read the LZC and the remaining bytes
        lzc = FPFC_BufReadHalfByte(&buf);
        val = FPFC_BufReadHalfBytes(&buf,(unsigned int)sizeof(pred)*2-lzc);

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
        hash ^= d2<<10;
        d2 = delta[d];
        delta[d] = (uint32_t)((uint64_t)(val-prev)>>50);
        hash = (hash<<5 ^ delta[d])&hmsk;
        d = (d+1)&1;

        prev = val;
        Data[n++] = *(double*)&val;
    }

    free(htbl);

#ifdef FPFC_USE_MEM_IO
    return buf.NB<=buf.Size ? APP_OK : APP_ERR;
#else //FPFC_USE_MEM_IO
    return APP_OK;
#endif //FPFC_USE_MEM_IO
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_Compress>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Compresse des float
 *
 * Parametres :
 *  <Data>      : Les données à compresser
 *  <N>         : Le nombre de float à compresser
 *  <CData>     : [?] Le buffer où écrire les données compressées
 *  <CBufSize>  : [?] La taille du buffer de données compressées
 *  <FD>        : [?] Le handle du fichier où écrire les données compressées
 *  <CSize>     : [OUT] La taille (Bytes) des données compressées
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int FPFC_Compress(float *restrict Data,size_t N,FPFC_IO_PARAM,size_t *CSize) {
    int32_t         *htbl=NULL,*dpred;
    uint32_t        hash,delta[2]={0},d2;
    int32_t         val,pred,prev;
    const uint32_t  dmsk=(1u<<((int)sizeof(int32_t)*8-14))-1u;
    const uint32_t  hmsk=((1u<<20)-1u);
    unsigned int    d,lzc;
    unsigned long   nb=N*sizeof(*Data);

#ifdef FPFC_USE_MEM_IO
    TFPFCBuf buf = (TFPFCBuf){CData,CBufSize,0,0,0};
#else //FPFC_USE_MEM_IO
    TFPFCBuf buf = (TFPFCBuf){FD,0,0,0};
#endif //FPFC_USE_MEM_IO

    // Allocate the hash table's space (2*2^20)
    htbl = calloc((1l<<21),sizeof(*htbl));

    // Loop on the data to compress
    for(d=0,d2=0,hash=0,prev=0; N; --N) {
        // Encode the value as an int
        val = *(int32_t*)Data++;

        // Locate the previous differences that will help with the prediction
        dpred = &htbl[hash*2];

        // Update the deltas and the hash
        // We only keep the 11 MSB of the delta (1(sign)+8(exp)+2(MSB mantissa)) to calculate the hash
        // Note that hash(d0,d1,d2) = lsb0..19(d0 XOR d1<<5 XOR d2<<10)
        // So to get the next hash, we only need to re-XOR d2<<10, shift by 5 and XOR d0
        hash ^= d2<<10;
        d2 = delta[d];
        delta[d] = (uint32_t)(val-prev)>>21;
        hash = (hash<<5 ^ delta[d])&hmsk;
        d = (d+1)&1;

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

        // Get the leading zeros count we'll encode and Write the remaining 32-4*LZC bits
        // We encode the LZC using 4 bits and work at half-byte granularity.
        // Ergo, for floats, we can encode the full 32 bits, but still have to round down to the lowest multiple of 4
        if( pred ) {
            lzc = LZCNT(pred)>>2;
            FPFC_BufWriteHalfByte(&buf,(TBufByte)lzc);
            FPFC_BufWriteHalfBytes(&buf,pred,(unsigned int)sizeof(pred)*2u-lzc);
        } else {
            FPFC_BufWriteHalfByte(&buf,(TBufByte)8);
        }

        prev = val;
    }

    free(htbl);
    FPFC_BufFlush(&buf);

    App_Log(DEBUG,"Compression ratio : %.4f\n",(double)buf.NB/nb);
    if( buf.NB >= nb ) {
        App_Log(WARNING,"Compressed data is larger than original (compressed=%ld ori=%ld)\n",buf.NB,nb);
        return APP_ERR;
    }

    *CSize = buf.NB;
    return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPFC_Inflate>
 * Creation : Octobre 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Décompresse des float
 *
 * Parametres :
 *  <Data>      : [OUT] Les données décompressés
 *  <N>         : Le nombre de float à décompresser
 *  <CData>     : [?] Le buffer où lire les données compressées
 *  <CBufSize>  : [?] La taille du buffer de données compressées
 *  <FD>        : [?] Le handle du fichier où lire les données compressées
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int FPFC_Inflate(float *restrict Data,size_t N,FPFC_IO_PARAM) {
    int32_t         *htbl=NULL,*dpred;
    uint32_t        hash,delta[3]={0u},d2;
    int32_t         val,pred,prev;
    const uint32_t  dmsk=(1u<<((int)sizeof(int32_t)*8-14))-1u;
    const uint32_t  hmsk=((1u<<20)-1u);
    unsigned int    d,lzc;
    uint64_t        n=0;

#ifdef FPFC_USE_MEM_IO
    TFPFCBuf buf = (TFPFCBuf){CData,CBufSize,0,0,0};
#else //FPFC_USE_MEM_IO
    TFPFCBuf buf = (TFPFCBuf){FD,0,0,0};
#endif //FPFC_USE_MEM_IO

    // Allocate the hash table's space (2*2^20)
    htbl = calloc((1l<<21),sizeof(*htbl));

    // Loop on the data to inflate
    for(d=0,d2=0,hash=0,prev=0; N; --N) {
        // Read the LZC and the remaining bytes
        lzc = FPFC_BufReadHalfByte(&buf);
        val = lzc!=8u ? FPFC_BufReadHalfBytes(&buf,(unsigned int)sizeof(pred)*2-lzc) : 0;

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
        hash ^= d2<<10;
        d2 = delta[d];
        delta[d] = (uint32_t)(val-prev)>>21;
        hash = (hash<<5 ^ delta[d])&hmsk;
        d = (d+1)&1;

        prev = val;
        Data[n++] = *(float*)&val;
    }

    free(htbl);

#ifdef FPFC_USE_MEM_IO
    return buf.NB<=buf.Size ? APP_OK : APP_ERR;
#else //FPFC_USE_MEM_IO
    return APP_OK;
#endif //FPFC_USE_MEM_IO
}
