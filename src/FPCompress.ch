#include "FPCompressD.h"
#include "FPCompressF.h"
#include <limits.h>
#include <stdlib.h>
#include "App.h"
#include "QSM.h"

#define bitsizeof(x)    (sizeof(x)*CHAR_BIT)
#define TOPBYTE(x)      ((x)>>(bitsizeof(x)-8))
#define FPCTOPBYTE(x)   ((x)>>(bitsizeof(TFPCType)-8))

// Returns the number of leading 0-bits in x, starting from the most significant bit position
// If x is 0, the result is undefined
#if defined __INTEL_COMPILER
#define MSB(x)  (sizeof(x)==8?(int)_lzcnt_u64(x):_bit_scan_reverse(x))
#elif defined __GNUC__
#define MSB(x)  (sizeof(x)==8?((int)bitsizeof(x)-__builtin_clzl(x)-1):((int)bitsizeof(x)-__builtin_clz(x)-1))
#else // not GCC nor intel
#define MSB(x)  (sizeof(x)==8?CountLeadingZerosl(x):CountLeadingZeros(x))
// X must be different from 0
static int CountLeadingZeros(unsigned int X) {
    uint32_t k=0;
    while( X>>=1 ) ++k;
    return k;
}
// X must be different from 0
static int CountLeadingZerosl(unsigned long X) {

    uint32_t k=0;
    while( X>>=1 ) ++k;
    return k;
}
#endif

#define QSM_TARGET 128

typedef struct TFPCCtx {
    TQSM        *QSM;
    FILE*       FD;
    size_t      Cnt;
    TFPCType    Range;
    TFPCType    Low;
    TFPCType    Code;
} TFPCCtx;

static TFPCCtx* FPC_New(FILE* FD,TQSMSym Size,TQSMUpF UpFreq) {
    TFPCCtx *ctx = malloc(sizeof(*ctx));

    if( ctx ) {
        if( !(ctx->QSM=QSM_New(Size,UpFreq)) ) {
            free(ctx);
            return NULL;
        }
        
        ctx->FD = FD;
        ctx->Cnt = 0;
        ctx->Range = (TFPCType)-1;
        ctx->Low = 0;
    }

    return ctx;
}

static void FPC_Free(TFPCCtx *Ctx) {
    if( Ctx ) {
        QSM_Free(Ctx->QSM);
        free(Ctx);
    }
}

static void FPC_PutByte(TFPCCtx *restrict Ctx) {
    putc(TOPBYTE(Ctx->Low),Ctx->FD);
    Ctx->Low <<= 8;
    ++Ctx->Cnt;
}

static void FPC_FlushBytes(TFPCCtx *restrict Ctx) {
    size_t i;

    // Read in the first 4 bytes
    for(i=0; i<sizeof(Ctx->Low); ++i) {
        FPC_PutByte(Ctx);
    }
}

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

static void FPC_EncodeK(TFPCCtx *restrict Ctx,TFPCType K) {
    TQSMFreq freq,ltcfreq,totfreq;

    // Get the frequence, cumulative number of smaller items and total number of items in the model
    QSM_GetFreq(Ctx->QSM,K,&freq,&ltcfreq,&totfreq);
    QSM_Add(Ctx->QSM,K);
    
    // R(i) = ⌊R(i-1)(Fai+fai)⌋ - ⌊R(i-1)Fai⌋
    // où Fai = ltcfreq/totfreq et fai = freq/totfreq
    // La division entière (int) par totfreq se chargeant de floor()ing, le floor peut disparaître
    // R(i) = int(R(i-1)/totfreq) * (ltcfreq+freq-ltcfreq)
    // R(i) = int(R(i-1)/totfreq) * freq
    // pour R[low,high), nous avons donc :
    //  low = low + int(R(i-1)/totfreq) * ltcfreq
    //  high= low + R(i)
    //Ctx->Range /= totfreq;
    Ctx->Range >>= 16;
    Ctx->Low += Ctx->Range * ltcfreq;
    Ctx->Range *= freq;

    FPC_AdjustRange(Ctx);
}

static void FPC_EncodeShift(TFPCCtx *restrict Ctx,TFPCType Bits,TFPCType K) {
    Ctx->Range >>= K;
    Ctx->Low += Ctx->Range*Bits;
    FPC_AdjustRange(Ctx);
}

// Bits : remaining bits to encode as-is
// K : number of remaining bits
static void FPC_EncodeBits(TFPCCtx *restrict Ctx,TFPCType Bits,TFPCType K) {
    size_t i;
    for(i=1; i<sizeof(Bits)/2 && K>16; ++i) {
        FPC_EncodeShift(Ctx,Bits&0xffff, 16);
        Bits >>= 16;
        K -= 16;
    }
    FPC_EncodeShift(Ctx,Bits,K);
}

#define BUFDIFF(i,j,k) ((k)*d1*d2+(j)*d1+(i))
#define BUFIDX(i,j,k)  (bufi<BUFDIFF(i,j,k)?bufs-BUFDIFF(i,j,k)+bufi:bufi-BUFDIFF(i,j,k))
int R(FPC_Compress)(FILE* FD,TFPCReal *restrict Data,int NI,int NJ,int NK,size_t *restrict CSize) {
    TFPCCtx         *ctx;
    size_t          bufs,bufi,i,n=NI*NJ*NK,d1=0,d2=0;
    TFPCType        *buf,udata,upred,diff,k,zero=bitsizeof(udata);
    const TFPCType  hbit=L(1u)<<(bitsizeof(hbit)-1);
    const int       ndims=(NI>1)+(NJ>1)+(NK>1),flag=(NI>1)<<2|(NJ>1)<<1|(NK>1);

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

    if( !(ctx=FPC_New(FD,bitsizeof(TFPCType)*2+1,QSM_TARGET)) ) {
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
    App_Log(DEBUG,"Compressing %zu values, flag=%d ndims=%u d1=%zu d2=%zu bufs=%zu\n",n,flag,ndims,d1,d2,bufs);

    // Allocate the buffer memory
    APP_MEM_ASRT( buf,calloc(bufs,sizeof(*buf)) );

    for(i=0,bufi=0; i<n; ++i) {
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
            k = MSB(diff);
            // Zero is set to bitsizeof(*data) and k<=bitsizeof(*data)-1 ; this means that 0<=(zero+-(k+1))<=bitsizeof(*data)*2
            // (0 to 64 (inclusive) if 32 bits)
            FPC_EncodeK(ctx,zero+(k+1));
            FPC_EncodeBits(ctx,diff-(1u<<k),k);
        } else if( upred > udata ) {
            // Overprediction
            diff = upred-udata;
            k = MSB(diff);
            // Zero is set to bitsizeof(*data) and k<=bitsizeof(*data)-1 ; this means that 0<=(zero+-(k+1))<=bitsizeof(*data)*2
            // (0 to 64 (inclusive) if 32 bits)
            FPC_EncodeK(ctx,zero-(k+1));
            FPC_EncodeBits(ctx,diff-(1u<<k),k);
        } else {
            // Perfect prediction
            FPC_EncodeK(ctx,zero);
        }
        //if( i>=5 ) break;
    }

    FPC_FlushBytes(ctx);
    n *= sizeof(*Data);
    App_Log(DEBUG,"Initial data size : %zu bytes. New data size : %zu bytes. Compression factor : %.2f\n",n,ctx->Cnt,(double)(n)/(double)ctx->Cnt);
    if( ctx->Cnt > n ) {
        App_Log(WARNING,"The compressed field is bigger (%lu bytes|%lu MB) than the initial field (%zu bytes|%zu MB) by a factor of %.2f\n",
                ctx->Cnt,(ctx->Cnt+1024*1024/2)/(1024*1024),n,(n+1024*1024/2)/(1024*1024),(double)ctx->Cnt/(double)(n));
    }
    *CSize = ctx->Cnt;

    free(buf);
    FPC_Free(ctx);
    return APP_OK;
}

static void FPC_GetByte(TFPCCtx *restrict Ctx) {
    Ctx->Code = (Ctx->Code<<8)|getc(Ctx->FD);
    Ctx->Low <<= 8;
    ++Ctx->Cnt;
}

static void FPC_InitBytes(TFPCCtx *restrict Ctx) {
    size_t i;

    // Read in the first 4 bytes
    for(i=0; i<sizeof(Ctx->Low); ++i) {
        FPC_GetByte(Ctx);
    }
}

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

static TFPCType FPC_DecodeK(TFPCCtx *restrict Ctx) {
    TQSMFreq freq,ltcfreq;
    TFPCType k;

    // Get the lesser-than cumulative frequency
    Ctx->Range >>= 16;
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

static TFPCType FPC_DecodeShift(TFPCCtx *restrict Ctx,TFPCType K) {
    TFPCType bits;

    Ctx->Range >>= K;
    bits = (Ctx->Code-Ctx->Low)/Ctx->Range;
    Ctx->Low += Ctx->Range*bits;
    FPC_DAdjustRange(Ctx);

    return bits;
}

// Bits : remaining bits to encode as-is
// K : number of remaining bits
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


int R(FPC_Inflate)(FILE* FD,TFPCReal *restrict Data,int NI,int NJ,int NK) {
    TFPCCtx         *ctx;
    size_t          bufs,bufi,i,n=NI*NJ*NK,d1=0,d2=0;
    TFPCType        *buf,udata,upred,diff,k,zero=bitsizeof(udata);
    const TFPCType  hbit=L(1u)<<(bitsizeof(hbit)-1);
    const int       ndims=(NI>1)+(NJ>1)+(NK>1),flag=(NI>1)<<2|(NJ>1)<<1|(NK>1);

    // Make sure dimensions are valid
    if( NI<1 || NJ<1 || NK<1 ) {
        App_Log(ERROR,"Dimensions must be greater or equal to 1\n");
        return APP_ERR;
    }

    if( !(ctx=FPC_New(FD,bitsizeof(TFPCType)*2+1,QSM_TARGET)) ) {
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
    App_Log(DEBUG,"Inflated size : %zu. Read : %zu bytes\n",n*sizeof(*Data),ctx->Cnt);

    free(buf);
    FPC_Free(ctx);
    return APP_OK;
}

