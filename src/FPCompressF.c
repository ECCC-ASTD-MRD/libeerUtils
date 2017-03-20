#include "FPCompressF.h"

#define L(x) x
#define R(x) x##F
typedef uint32_t TFPCType;
typedef float TFPCReal;
#include "FPCompress.ch"
