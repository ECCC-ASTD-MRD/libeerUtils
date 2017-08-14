/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de compression de nombre flotants
 * Fichier   : FPCompress.ch
 * Creation  : Mars 2017
 * Auteur    : Eric Legault-Ouellet
 *
 * Description: Compress and inflate floating point numbers
 *
 * Note:
        This is an implementation of the paper
 *      "Fast and Efficient Compression of Floating-Point Data" written
 *      by Peter Lindstrom and Martin Isenburg, published in
 *      IEEE Transactions on Visualization and Computer Graphics 12(5):1245-50,
 *      September 2006
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

#include "FPCompressD.h"
#include "FPCompressF.h"
#include <limits.h>
#include <stdlib.h>
#include "App.h"
#include "QSM.h"

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#define FPC_BUF_SIZE    134217728 // 128 MB

#define bitsizeof(x)    (sizeof(x)*CHAR_BIT)
#define TOPBYTE(x)      ((x)>>(bitsizeof(x)-8))
#define FPCTOPBYTE(x)   ((x)>>(bitsizeof(TFPCType)-8))

typedef struct TFPCCtx {
    TQSM            *QSM;
    unsigned char   *CData;
    size_t          Cnt;
    size_t          BufCnt;
    TFPCType        Range;
    TFPCType        Low;
    TFPCType        Code;
    int             FD;
} TFPCCtx;

/*----------------------------------------------------------------------------
 * Nom      : <BSR>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Bit Scan Reverse : retourne la position du bit le plus
 *            significatif à 1.
 *            Ex: 00001000 retournerait 3
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
inline static int BSR(unsigned int X) {
    int k;
#if __x86_64__ || defined __i386__
    __asm__("bsr %1,%0":"=r"(k):"rm"(X));
#elif defined __INTEL_COMPILER
    k = _bit_scan_reverse(x);
#elif defined __GNUC__
    k = (int)bitsizeof(x)-1-__builtin_clz(x);
#else
    k=0;
    while( X>>=1 ) ++k;
#endif
    return k;
}
inline static int BSRl(unsigned long X) {
#if __x86_64__ || defined __i386__
    long k;
    __asm__("bsr %1,%0":"=r"(k):"rm"(X));
    return (int)k;
#elif defined __INTEL_COMPILER
    return (int)bitsizeof(x)-1-(int)_lzcnt_u64(x);
#elif defined __GNUC__
    return (int)bitsizeof(x)-1-__builtin_clzl(x);
#else
    int k=0;
    while( X>>=1 ) ++k;
    return k;
#endif
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_New>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Retourne une structure TFPCCtx initialisée
 *
 * Parametres :
 *  <CData> : Buffer that contains or will receive the data
 *  <FD>    : File descriptor where to write directly (-1 to use CData)
 *
 * Retour   : Une structure TFPCCtx initialisée ou NULL en cas d'erreur
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static TFPCCtx* FPC_New(char *restrict CData,int FD) {
    TFPCCtx *ctx = malloc(sizeof(*ctx));

    if( ctx ) {
        if( !(ctx->QSM=QSM_New(bitsizeof(TFPCType)*2+1,1u<<16,8192)) ) {
            free(ctx);
            return NULL;
        }
        
        ctx->CData = CData;
        ctx->Cnt = 0;
        ctx->BufCnt = 0;
        ctx->Range = (TFPCType)-1;
        ctx->Low = 0;
        ctx->FD = FD;
    }

    return ctx;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_Free>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Libère une structure TFPCCtx
 *
 * Parametres :
 *  <Ctx>   : Structure dont il faut libérer la mémoire
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPC_Free(TFPCCtx *Ctx) {
    if( Ctx ) {
        QSM_Free(Ctx->QSM);
        free(Ctx);
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_PutByte>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Écrit un byte de donnée dans un fichier
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPC_PutByte(TFPCCtx *restrict Ctx) {
    Ctx->CData[Ctx->BufCnt++] = (char)TOPBYTE(Ctx->Low);
    Ctx->Low <<= 8;
    ++Ctx->Cnt;

    // Flush the buffer
    if( Ctx->FD!=-1 && Ctx->BufCnt>=FPC_BUF_SIZE ) {
        if( write(Ctx->FD,Ctx->CData,Ctx->BufCnt) != Ctx->BufCnt ) {
            App_Log(ERROR,"Problem while writing the data : field will be corrupted\n");
        }
        Ctx->BufCnt = 0;
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_FlushBytes>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Écrit les derniers bytes restant pour compléter la compression
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPC_FlushBytes(TFPCCtx *restrict Ctx) {
    size_t i;

    // Write the last 4 bytes
    for(i=0; i<sizeof(Ctx->Low); ++i) {
        FPC_PutByte(Ctx);
    }

    // Flush the buffer
    if( Ctx->FD!=-1 && Ctx->BufCnt ) {
        if( write(Ctx->FD,Ctx->CData,Ctx->BufCnt) != Ctx->BufCnt ) {
            App_Log(ERROR,"Problem while writing the data : field will be corrupted\n");
        }
        Ctx->BufCnt = 0;
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_AdjustRange>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Ajuste (normalize) le range en écrivant les bytes qui sont
 *            immuables
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPC_AdjustRange(TFPCCtx *restrict Ctx) {
    // If the top byte is the same when you add the range, this means that it won't change anymore and may be ouputed
    while( !FPCTOPBYTE(Ctx->Low^(Ctx->Low+Ctx->Range)) ) {
        FPC_PutByte(Ctx);
        // Add one more "digit" (see range encoder paper by G.N.N. Martin)
        Ctx->Range <<= 8;
    }

    // Top byte is not fixed but range is too small. Fudge range to avoid carry and ouput top 2 bytes.
    // See Lindstrom et al "Fast and efficient compression of floating-point data"
    if( !(Ctx->Range>>16) ) {
        FPC_PutByte(Ctx);
        FPC_PutByte(Ctx);
        Ctx->Range = -Ctx->Low;
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_EncodeK>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : encode et ajoute au range le K, soit le nombre de bits significatifs
 *            nuls avant le premier '1'
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *  <K>     : La valeur à encoder
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPC_EncodeK(TFPCCtx *restrict Ctx,TFPCType K) {
    TQSMFreq freq,ltcfreq;

    // Get the frequence, cumulative number of smaller items and total number of items in the model
    QSM_GetFreq(Ctx->QSM,K,&freq,&ltcfreq);
    QSM_Add(Ctx->QSM,K);
    
    // R(i) = ⌊R(i-1)(Fai+fai)⌋ - ⌊R(i-1)Fai⌋
    // où Fai = ltcfreq/totfreq et fai = freq/totfreq
    // La division entière (int) par totfreq se chargeant de floor()ing, le floor peut disparaître
    // R(i) = int(R(i-1)/totfreq) * (ltcfreq+freq-ltcfreq)
    // R(i) = int(R(i-1)/totfreq) * freq
    // pour R[low,high), nous avons donc :
    //  low = low + int(R(i-1)/totfreq) * ltcfreq
    //  high= low + R(i)
    Ctx->Range >>= 16; // totfreq is maintained at 1<<16 so this is akin to dividing by totfreq
    Ctx->Low += Ctx->Range * ltcfreq;
    Ctx->Range *= freq;

    FPC_AdjustRange(Ctx);
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_EncodeShift>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Encode verbatim les bits donnés
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *  <Bits>  : Les bits à encoder
 *  <K>     : Le nombre de bits à encoder
 *
 * Retour   :
 *
 * Remarques : Voir "FPC_EncodeBits"
 *
 *----------------------------------------------------------------------------
 */
static void FPC_EncodeShift(TFPCCtx *restrict Ctx,TFPCType Bits,TFPCType K) {
    Ctx->Range >>= K;
    Ctx->Low += Ctx->Range*Bits;
    FPC_AdjustRange(Ctx);
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_EncodeBits>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Encode verbatim les bits donnés
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *  <Bits>  : Les bits à encoder
 *  <K>     : Le nombre de bits à encoder
 *
 * Retour   :
 *
 * Remarques :
 *      Il est nécessaire d'encoder les bits graduellement, sans quoi un trop
 *      gros "shift" pourrait causer un rnage nul, ce qui invaliderait le
 *      range coding.
 *
 *----------------------------------------------------------------------------
 */
static void FPC_EncodeBits(TFPCCtx *restrict Ctx,TFPCType Bits,TFPCType K) {
    size_t i;
    for(i=1; i<sizeof(Bits)/2 && K>16; ++i) {
        FPC_EncodeShift(Ctx,Bits&0xffff, 16);
        Bits >>= 16;
        K -= 16;
    }
    FPC_EncodeShift(Ctx,Bits,K);
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_Compress>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Compresse les nombre à points flottants donnés
 *
 * Parametres :
 *  <CData> : Le buffer où écrire les données compressées (NULL si FD)
 *  <FD>    : Le fichier où écrire les données directement (-1 si CData)
 *  <Data>  : Les nombres flotants à écrire
 *  <NI>    : Dimension en I
 *  <NJ>    : Dimension en J
 *  <NK>    : Dimension en K
 *  <CSize> : [OUT] Taille compressée
 *
 * Retour   : APP_OK si ok, APP_ERR si error
 *
 * Remarques :
 *    The CData buffer must have at least NI*NJ*NK bytes
 *
 *----------------------------------------------------------------------------
 */
#define BUFDIFF(i,j,k) ((k)*d1*d2+(j)*d1+(i))
#define BUFIDX(i,j,k)  (bufi<BUFDIFF(i,j,k)?bufs-BUFDIFF(i,j,k)+bufi:bufi-BUFDIFF(i,j,k))
int R(FPC_Compress)(void *restrict CData,int FD,TFPCReal *restrict Data,int NI,int NJ,int NK,size_t *restrict CSize) {
    TFPCCtx         *ctx;
    size_t          bufs,bufi,i,n=NI*NJ*NK,d1=0,d2=0;
    TFPCType        *buf,udata,upred,diff,k,zero=bitsizeof(udata);
    const TFPCType  hbit=L(1u)<<(bitsizeof(hbit)-1);
    const int       ndims=(NI>1)+(NJ>1)+(NK>1),flag=(NI>1)<<2|(NJ>1)<<1|(NK>1);
    int             code=APP_OK;

    // Make sure dimensions are valid
    if( NI<1 || NJ<1 || NK<1 ) {
        App_Log(ERROR,"Dimensions must be greater or equal to 1\n");
        return APP_ERR;
    }

    // Make sure we have a chance to compress stuff
    //if( n < 16 ) {
    //    App_Log(ERROR,"Compression of a field of less than 16 values is a waste of time\n");
    //    return APP_ERR;
    //}

    // If a file was given, use a write buffer instead
    if( FD!=-1 && !(CData=malloc(FPC_BUF_SIZE)) ) {
        App_Log(ERROR,"Could not allocate memory for write buffer\n");
        return APP_ERR;
    }

    if( !(ctx=FPC_New(CData,FD)) ) {
        App_Log(ERROR,"Could not allocate memory for context\n");
        if( FD!=-1 )
            free(CData);
        return APP_ERR;
    }

    switch(flag) {
        case 1: // 001 : NK>1
            bufs = 1;
            d1 = NK;
            break;
        case 2: // 010 : NJ>1
            bufs = 1;
            d1 = NJ;
            break;
        case 3: // 011 : NJ>1 NK>1
            d1 = NJ;
            d2 = NK;
            bufs = d1+1;
            break;
        case 4: // 100 : NI>1
            bufs = 1;
            d1 = NI;
            break;
        case 5: // 101 : NI>1 NK>1
            d1 = NI;
            d2 = NK;
            bufs = d1+1;
            break;
        case 6: // 110 : NI>1 NJ>1
            d1 = NI;
            d2 = NJ;
            bufs = d1+1;
            break;
        case 7: // 111 : NI>1 NJ>1 NK>1
            d1 = NI;
            d2 = NJ;
            bufs = d1*d2+d1+1;
            break;
        default:
            // impossible
            break;
    }
    App_Log(DEBUG,"Compressing %zu values, flag=%d ndims=%u d1=%zu d2=%zu bufs=%zu\n",n,flag,ndims,d1,d2,bufs);

    // Allocate the buffer memory
    APP_MEM_ASRT( buf,calloc(bufs,sizeof(*buf)) );

    // We must not go past the result buffer (if not writing to a file)
    const size_t maxcnt = (FD==-1) ? n-sizeof(ctx->Low) : SIZE_MAX;
    for(i=0,bufi=0; i<n && ctx->Cnt<maxcnt; ++i) {
        // To prevent any rounding/order of operation problem with floats, we'll do integer arithmetic instead
        // We can't map to signed int because of the two's complement representation on many archs, so
        // we map the floats to unsigned integers by flipping either the most significant bit (for positive floats)
        // or all bits (for negative floats). See Lindstrom et al "Fast and efficient compression of floating-point data"
        udata = *(TFPCType*)&Data[i];
        udata = udata&hbit ? ~udata : udata|hbit;

        // Get the prediction using the Lorenzo predictor technique (Ibarria, Lindstrom et al.)
        // Note : since flag is constant, the compiler will most certainly optimize this
        switch(ndims) {
            case 1:
                upred = buf[BUFIDX(1,0,0)];
                break;
            case 2:
                upred = buf[BUFIDX(1,0,0)] - buf[BUFIDX(1,1,0)] + buf[BUFIDX(0,1,0)];
                break;
            case 3:
                upred = buf[BUFIDX(1,0,0)] - buf[BUFIDX(0,1,1)]
                    +   buf[BUFIDX(0,1,0)] - buf[BUFIDX(1,0,1)]
                    +   buf[BUFIDX(0,0,1)] - buf[BUFIDX(1,1,0)]
                    +   buf[BUFIDX(1,1,1)];
                break;
            default:
                // impossible
                break;
        }

        // Store the value in the buffer. Note that we can't do that before the prediction because we would need a bigger buffer (+1) then
        buf[bufi] = udata;
        bufi = bufi+1>=bufs ? 0 : bufi+1;

        // Encode the value
        if( upred < udata ) {
            // Underprediction
            diff = udata-upred;
            k = L(BSR)(diff);
            // Zero is set to bitsizeof(*data) and k<=bitsizeof(*data)-1 ; this means that 0<=(zero+-(k+1))<=bitsizeof(*data)*2
            // (0 to 64 (inclusive) if 32 bits)
            FPC_EncodeK(ctx,zero+(k+1));
            FPC_EncodeBits(ctx,diff-(L(1u)<<k),k);
        } else if( upred > udata ) {
            // Overprediction
            diff = upred-udata;
            k = L(BSR)(diff);
            // Zero is set to bitsizeof(*data) and k<=bitsizeof(*data)-1 ; this means that 0<=(zero+-(k+1))<=bitsizeof(*data)*2
            // (0 to 64 (inclusive) if 32 bits)
            FPC_EncodeK(ctx,zero-(k+1));
            FPC_EncodeBits(ctx,diff-(L(1u)<<k),k);
        } else {
            // Perfect prediction
            FPC_EncodeK(ctx,zero);
        }
    }

    if( ctx->Cnt < maxcnt ) {
       FPC_FlushBytes(ctx);
       n *= sizeof(*Data);
       if( ctx->Cnt <= n ) {
           *CSize = ctx->Cnt;
           App_Log(DEBUG,"Initial data size : %zu bytes. New data size : %zu bytes. Compression factor : %.2f\n",n,ctx->Cnt,(double)(n)/(double)ctx->Cnt);
       } else {
           App_Log(WARNING,"Compression was aborted because the size of the compressed data would have been over the inital size.\n");
           code = APP_ERR;
       }
    } else {
       App_Log(WARNING,"Compression was aborted because we were going to be over the resulting buffer size, which means the compression wasn't working very well on this dataset anyway.\n");
       code = APP_ERR;
    }

    if( FD != -1 ) {
        free(CData);
    }

    free(buf);
    FPC_Free(ctx);
    return code;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_GetByte>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Lit un byte de donnée d'un fichier
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPC_GetByte(TFPCCtx *restrict Ctx) {
    Ctx->Code = (Ctx->Code<<8)|Ctx->CData[Ctx->Cnt++];
    Ctx->Low <<= 8;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_InitBytes>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Initialise le décodage en lisant autant de bytes que peux contenir
 *            le range
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPC_InitBytes(TFPCCtx *restrict Ctx) {
    size_t i;

    // Read in the first 4 bytes
    for(i=0; i<sizeof(Ctx->Low); ++i) {
        FPC_GetByte(Ctx);
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_AdjustRange>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Ajuste (normalize) le range en lisant les bytes qui sont
 *            immuables (inverse de FPC_AdjustRange)
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *
 * Retour   :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void FPC_DAdjustRange(TFPCCtx *restrict Ctx) {
    // If the top byte is the same when you add the range, this means that it won't change anymore and may be ouputed
    while( !FPCTOPBYTE(Ctx->Low^(Ctx->Low+Ctx->Range)) ) {
        FPC_GetByte(Ctx);
        // Add one more "digit" (see range encoder paper by G.N.N. Martin)
        Ctx->Range <<= 8;
    }

    // Top byte is not fixed but range is too small. Fudge range to avoid carry and ouput top 2 bytes.
    // See Lindstrom et al "Fast and efficient compression of floating-point data"
    if( !(Ctx->Range>>16) ) {
        FPC_GetByte(Ctx);
        FPC_GetByte(Ctx);
        Ctx->Range = -Ctx->Low;
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_DecodeK>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Decode le 'K' (nombre de high bits à 0)
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *
 * Retour   : Le 'K' décodé
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static TFPCType FPC_DecodeK(TFPCCtx *restrict Ctx) {
    TQSMFreq freq,ltcfreq;
    TFPCType k;

    // Get the lesser-than cumulative frequency
    Ctx->Range >>= 16; // totfreq is maintained at 1<<16 so this is akin to dividing by totfreq
    ltcfreq = (Ctx->Code-Ctx->Low)/Ctx->Range;

    // Get K from the model
    k = QSM_GetSym(Ctx->QSM,&ltcfreq,&freq);
    QSM_Add(Ctx->QSM,k);

    // Adjust the range
    Ctx->Low += Ctx->Range*ltcfreq;
    Ctx->Range *= freq;
    FPC_DAdjustRange(Ctx);

    return k;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_DecodeShift>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Decode verbatim les bits
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *  <K>     : Le nombre de bits à décoder
 *
 * Retour   : Les bits décodés
 *
 * Remarques : Voir "FPC_EncodeBits"
 *
 *----------------------------------------------------------------------------
 */
static TFPCType FPC_DecodeShift(TFPCCtx *restrict Ctx,TFPCType K) {
    TFPCType bits;

    Ctx->Range >>= K;
    bits = (Ctx->Code-Ctx->Low)/Ctx->Range;
    Ctx->Low += Ctx->Range*bits;
    FPC_DAdjustRange(Ctx);

    return bits;
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_DecodeBits>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Decode verbatim les bits
 *
 * Parametres :
 *  <Ctx>   : Contexte
 *  <K>     : Le nombre de bits à décoder
 *
 * Retour   : Les bits décodés
 *
 * Remarques :
 *      Voir "FPC_EncodeBits"
 *
 *----------------------------------------------------------------------------
 */
static TFPCType FPC_DecodeBits(TFPCCtx *restrict Ctx,TFPCType K) {
    TFPCType bits=0,shift=0;
    size_t i;

    for(i=1; i<sizeof(bits)/2 && K>16; ++i) {
        bits += FPC_DecodeShift(Ctx,16)<<shift;
        shift += 16;
        K -= 16;
    }
    return bits+(FPC_DecodeShift(Ctx,K)<<shift);
}

/*----------------------------------------------------------------------------
 * Nom      : <FPC_Inflate>
 * Creation : Mars 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Décompresse les nombre à points flottants
 *
 * Parametres :
 *  <CData> : Le buffer contenant les données compressées (NULL si FD)
 *  <FD>    : Le fichier où écrire les données directement (-1 si CData)
 *  <Data>  : [OUT] Les nombres flotants décompressés
 *  <NI>    : Dimension en I
 *  <NJ>    : Dimension en J
 *  <NK>    : Dimension en K
 *
 * Retour   : APP_OK si ok, APP_ERR si error
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int R(FPC_Inflate)(void *restrict CData,int FD,TFPCReal *restrict Data,int NI,int NJ,int NK) {
    TFPCCtx         *ctx;
    size_t          bufs,bufi,i,n=NI*NJ*NK,d1=0,d2=0,mxtra=0;
    TFPCType        *buf,udata,upred,diff,k,zero=bitsizeof(udata);
    const TFPCType  hbit=L(1u)<<(bitsizeof(hbit)-1);
    const int       ndims=(NI>1)+(NJ>1)+(NK>1),flag=(NI>1)<<2|(NJ>1)<<1|(NK>1);

    // Make sure dimensions are valid
    if( NI<1 || NJ<1 || NK<1 ) {
        App_Log(ERROR,"Dimensions must be greater or equal to 1\n");
        return APP_ERR;
    }

    // If FD was given, map the file
    if( FD != -1 ) {
        off_t off,poff;

        // Get where we are in the file
        if( (off=lseek(FD,0,SEEK_CUR)) == -1 ) {
            App_Log(ERROR,"Could not tell position in the file : %s\n",strerror(errno));
            return APP_ERR;
        }
        // Get a page-floored offset
        poff = off & ~((size_t)sysconf(_SC_PAGESIZE)-1);
        // Get the size of the extra mapping (from the floored offset to the begining of the data we want)
        mxtra = off-poff;
        // Map that part of the file
        if( (CData=mmap(NULL,mxtra+n*sizeof(*Data),PROT_READ,MAP_SHARED,FD,poff)) == MAP_FAILED ) {
            App_Log(ERROR,"Could not map file to decompress data : %s\n",strerror(errno));
            return APP_ERR;
        }
    }

    if( !(ctx=FPC_New((char*)CData+mxtra,-1)) ) {
        App_Log(ERROR,"Could not allocate memory for context\n");
        return APP_ERR;
    }

    switch(flag) {
        case 1: // 001 : NK>1
            bufs = 1;
            d1 = NK;
            break;
        case 2: // 010 : NJ>1
            bufs = 1;
            d1 = NJ;
            break;
        case 3: // 011 : NJ>1 NK>1
            d1 = NJ;
            d2 = NK;
            bufs = d1+1;
            break;
        case 4: // 100 : NI>1
            bufs = 1;
            d1 = NI;
            break;
        case 5: // 101 : NI>1 NK>1
            d1 = NI;
            d2 = NK;
            bufs = d1+1;
            break;
        case 6: // 110 : NI>1 NJ>1
            d1 = NI;
            d2 = NJ;
            bufs = d1+1;
            break;
        case 7: // 111 : NI>1 NJ>1 NK>1
            d1 = NI;
            d2 = NJ;
            bufs = d1*d2+d1+1;
            break;
        default:
            // impossible
            break;
    }
    App_Log(DEBUG,"Inflating %zu values, flag=%d ndims=%u d1=%zu d2=%zu bufs=%zu\n",n,flag,ndims,d1,d2,bufs);

    // Allocate the buffer memory
    APP_MEM_ASRT( buf,calloc(bufs,sizeof(*buf)) );

    FPC_InitBytes(ctx);

    for(i=0,bufi=0; i<n; ++i) {
        // Get the prediction using the Lorenzo predictor technique (Ibarria, Lindstrom et al.)
        // Note : since ndims is constant, the compiler will most certainly optimize this
        switch(ndims) {
            case 1:
                upred = buf[BUFIDX(1,0,0)];
                break;
            case 2:
                upred = buf[BUFIDX(1,0,0)] - buf[BUFIDX(1,1,0)] + buf[BUFIDX(0,1,0)];
                break;
            case 3:
                upred = buf[BUFIDX(1,0,0)] - buf[BUFIDX(0,1,1)]
                    +   buf[BUFIDX(0,1,0)] - buf[BUFIDX(1,0,1)]
                    +   buf[BUFIDX(0,0,1)] - buf[BUFIDX(1,1,0)]
                    +   buf[BUFIDX(1,1,1)];
                break;
            default:
                // impossible
                break;
        }

        // Get the shifted k
        k = FPC_DecodeK(ctx);

        // A number under "zero" means an overprediction. a number over "zero" means an underprediction. zero means perfect prediction
        if( k > zero ) {
            // Underprediction
            k -= zero+1;
            diff = (L(1u)<<k) + FPC_DecodeBits(ctx,k);
            udata = diff+upred;
        } else if( k < zero ) {
            // Overprediction
            k = zero-1-k;
            diff = (L(1u)<<k) + FPC_DecodeBits(ctx,k);
            udata = upred-diff;
        } else {
            // Perfect prediction
            udata = upred;
        }

        // Store the value in the buffer
        buf[bufi] = udata;
        bufi = bufi+1>=bufs ? 0 : bufi+1;

        // To prevent any rounding/order of operation problem with floats, we do integer arithmetic instead
        // We can't map to signed int because of the two's complement representation on many archs, so
        // we map the floats to unsigned integers by flipping either the most significant bit (for positive floats)
        // or all bits (for negative floats). See Lindstrom et al "Fast and efficient compression of floating-point data"
        udata = udata&hbit ? udata&~hbit : ~udata;
        Data[i] = *(TFPCReal*)&udata;
    }
    App_Log(DEBUG,"Inflated size : %zu bytes. Read : %zu bytes\n",n*sizeof(*Data),ctx->Cnt);

    if( FD != -1 ) {
        munmap(CData,mxtra+n*sizeof(*Data));
    }

    free(buf);
    FPC_Free(ctx);
    return APP_OK;
}

