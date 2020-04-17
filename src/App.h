/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de fonctions utiles
 * Fichier   : App.h
 * Creation  : Septembre 2008 - J.P. Gauthier
 *
 * Description: Fonctions génériques à tout les modèles.
 *
 * Remarques :
 *
 * License      :
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
#ifndef _App_h
#define _App_h

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <malloc.h>
#include <alloca.h>
#include <errno.h>
#include <limits.h>
#include <float.h>

#include "timestamp.h"

#ifdef _OPENMP
#   include <omp.h>
#endif

#ifdef _MPI
#   include <mpi.h>
#endif

#define APP_ERRORSIZE 2048

#define APP_COLOR_BLINK   "\x1b[5m"
#define APP_COLOR_BLACK   "\x1b[0;30m"
#define APP_COLOR_RED     "\x1b[0;31m"
#define APP_COLOR_GREEN   "\x1b[0;32m"
#define APP_COLOR_YELLOW  "\x1b[0;33m"
#define APP_COLOR_BLUE    "\x1b[0;34m"
#define APP_COLOR_MAGENTA "\x1b[0;35m"
#define APP_COLOR_CYAN    "\x1b[0;36m"
#define APP_COLOR_GRAY    "\x1b[0;37m"
#define APP_COLOR_RESET   "\x1b[0m"

#define APP_MASTER    0
#define APP_THREAD    1

#define APP_BUFMAX    32768               // Maximum input buffer length
#define APP_LISTMAX   4096                // MAximum number of items in a flag list
#define APP_SEED      1049731793          // Initial FIXED seed

#define APP_NOARGSFLAG 0x00               // No flag specified
#define APP_NOARGSFAIL 0x01               // Fail if no arguments are specified
#define APP_ARGSLOG    0x02               // Use log flag
#define APP_ARGSLANG   0x04               // Multilingual app
#define APP_ARGSSEED   0x08               // Use seed flag
#define APP_ARGSTHREAD 0x10               // Use thread flag
#define APP_ARGSTMPDIR 0x20               // Use tmp dir

#ifdef __xlC__
#   define APP_ONCE    ((1)<<3)
#else
#   define APP_ONCE    ((__COUNTER__+1)<<3)
#endif
#define APP_MAXONCE 1024

typedef enum { MUST=-1,ERROR=0,WARNING=1,INFO=2,DEBUG=3,EXTRA=4 } TApp_LogLevel;
typedef enum { STOP,RUN,DONE } TApp_State;
typedef enum { APP_NIL=0x0,APP_FLAG=0x01,APP_CHAR=0x02,APP_UINT32=0x04,APP_INT32=0x06,APP_UINT64=0x08,APP_INT64=0x0A,APP_FLOAT32=0x0C,APP_FLOAT64=0x0E } TApp_Type;
typedef enum { APP_FR=0x0,APP_EN=0x01 } TApp_Lang;
typedef enum { APP_OK=1,APP_ERR=0 } TApp_RetCode;
typedef enum { APP_AFFINITY_NONE=0,APP_AFFINITY_COMPACT=1,APP_AFFINITY_SCATTER=2,APP_AFFINITY_SOCKET=3 } TApp_Affinity;

#define APP_ASRT_OK(x) if( (x)!=APP_OK ) return(APP_ERR)
#define APP_ASRT_OK_M(Fct, ...) \
   if( (Fct)!=APP_OK ) { \
      App_Log(ERROR, __VA_ARGS__); \
      return(APP_ERR); \
   }

// Check FST function and return the specified value if an error was encountered
#define APP_FST_ASRT_H(Fct, ...) \
   if( (Fct) < 0 ) { \
      App_Log(ERROR, __VA_ARGS__); \
      return(APP_ERR); \
   }
#define APP_FST_ASRT(Fct, ...) \
   if( (Fct) != 0 ) { \
      App_Log(ERROR, __VA_ARGS__); \
      return(APP_ERR); \
   }
// Memory helpers
#define APP_MEM_ASRT(Buf,Fct) \
   if( !(Buf=(Fct)) ) { \
      App_Log(ERROR,"(%s) Could not allocate memory for field %s at line %d.\n",__func__,#Buf,__LINE__); \
      return(APP_ERR); \
   }
#define APP_FREE(Ptr) if(Ptr) { free(Ptr); Ptr=NULL; }
// MPI helpers
#ifdef _MPI
#define APP_MPI_ASRT(Fct) { \
   int err = (Fct); \
   if( err!=MPI_SUCCESS ) { \
      App_Log(ERROR,"(%s) MPI call %s at line %d failed with code %d for MPI node %d\n",__func__,#Fct,__LINE__,err,App->RankMPI); \
      return(APP_ERR); \
   } \
}
#define APP_MPI_CHK(Fct) { \
   int err = (Fct); \
   if( err!=MPI_SUCCESS ) { \
      App_Log(ERROR,"(%s) MPI call %s at line %d failed with code %d for MPI node %d\n",__func__,#Fct,__LINE__,err,App->RankMPI); \
   } \
}
#endif //_MPI

// Argument definitions
typedef struct TApp_Arg {
   TApp_Type    Type;                    // Argument type
   void         *Var;                    // Where to put the argument value(s)
   int          Multi;                   // Multiplicity of the argument (Maximum number of values for a list)
   char         *Short,*Long,*Info;      // Argument flags and description
} TApp_Arg;

// Application controller definition
typedef struct TApp {
    char*          Name;                 // Name
    char*          Version;              // Version
    char*          Desc;                 // Description
    char*          TimeStamp;            // Compilation timestamp
    char*          LogFile;              // Log file
//    char*          TmpDir;               // Tmp directory
    char*          Tag;                  // Identificateur
    FILE*          LogStream;            // Log file associated stream
    int            LogWarning;           // Number of warnings
    int            LogError;             // Number of errors
    int            LogColor;             // Use coloring in the logs
    TApp_LogLevel  LogLevel;             // Level of log
    TApp_State     State;                // State of application
    TApp_Lang      Language;             // Language (default: $CMCLNG or APP_EN)
    double         Percent;              // Percentage of execution done (0=not started, 100=finished)
    struct timeval Time;                 // Timer for execution time
    int            Type;                 // App object type (APP_MASTER,APP_THREAD)

    int            Seed,*OMPSeed;        // Random number generator seed
    int           *TotalsMPI;            // MPI total number of items arrays
    int           *CountsMPI;            // MPI count gathering arrays
    int           *DisplsMPI;            // MPI displacement gathering arrays
    int            NbMPI,RankMPI;        // Number of MPI process
    int            NbThread;             // Number of OpenMP threads
    int            Signal;               // Trapped signal
    TApp_Affinity  Affinity;             // Thread placement affinity
    int            NbNodeMPI,NodeRankMPI;// Number of MPI process on the current node
#ifdef _MPI
    MPI_Comm       NodeComm,NodeHeadComm;// Communicator for the current node and the head nodes
#endif //_MPI
} TApp;

#ifndef APP_BUILD
extern __thread TApp *App;               // Per thread App pointer
#endif

typedef int (TApp_InputParseProc) (void *Def,char *Token,char *Value,int Index);

TApp *App_Init(int Type,char* Name,char* Version,char* Desc,char* Stamp);
void  App_Free(void);
void  App_Start(void);
int   App_End(int Status);
void  App_Log(TApp_LogLevel Level,const char *Format,...);
void  App_LogOpen(void);
void  App_LogClose(void);
int   App_LogLevel(char *Val);
void  App_Progress(float Percent,const char *Format,...);
int   App_ParseArgs(TApp_Arg *AArgs,int argc,char *argv[],int Flags);
int   App_ParseInput(void *Def,char *File,TApp_InputParseProc *ParseProc);
int   App_ParseBool(char *Param,char *Value,char *Var);
int   App_ParseDate(char *Param,char *Value,time_t *Var);
int   App_ParseDateSplit(char *Param,char *Value,int *Year,int *Month,int *Day,int *Hour,int *Min);
int   App_ParseCoords(char *Param,char *Value,double *Lat,double *Lon,int Index);
void  App_SeedInit(void);
char* App_ErrorGet(void);
int   App_ThreadPlace(void);
void  App_Trap(int Signal);
int   App_IsDone(void); 
int   App_IsMPI(void);
int   App_IsOMP(void);
int   App_IsSingleNode(void);
int   App_IsAloneNode(void);

#endif
