/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de fonctions utiles
 * Fichier   : App.h
 * Creation  : Septembre 2008 - J.P. Gauthier
 * Revision  : $Id$
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

#ifdef _OPENMP
#   include <omp.h>
#endif

#ifdef _MPI
#   include <mpi.h>
#endif

#define APP_ERRORSIZE 2048

#define APP_COLOR_BLINK   "\x1b[5m"
#define APP_COLOR_BLACK   "\x1b[30m"
#define APP_COLOR_RED     "\x1b[31m"
#define APP_COLOR_GREEN   "\x1b[32m"
#define APP_COLOR_YELLOW  "\x1b[33m"
#define APP_COLOR_BLUE    "\x1b[34m"
#define APP_COLOR_MAGENTA "\x1b[35m"
#define APP_COLOR_CYAN    "\x1b[36m"
#define APP_COLOR_GRAY    "\x1b[37m"
#define APP_COLOR_RESET   "\x1b[0m"

#define APP_BUFMAX    32768               // Maximum input buffer length
#define APP_SEED      1049731793          // Initial FIXED seed

#define APP_NOARGSFLAG 0x00               // No flag specified
#define APP_NOARGSFAIL 0x01               // Fail if no arguments are specified
#define APP_ARGSLOG    0x02               // Use log flag
#define APP_ARGSLANG   0x04               // Multilingual app

typedef enum { MUST=-1,ERROR=0,WARNING=1,INFO=2,DEBUG=3,EXTRA=4 } TApp_LogLevel;
typedef enum { STOP,RUN,DONE } TApp_State;
typedef enum { APP_NIL=0x0,APP_FLAG=0x01,APP_LIST=0x02,APP_CHAR=0x04,APP_UINT32=0x06,APP_INT32=0x08,APP_UINT64=0x0A,APP_INT64=0x0C,APP_FLOAT32=0x0E,APP_FLOAT64=0x10 } TApp_Type;
typedef enum { APP_FR=0x0,APP_EN=0x01 } TApp_Lang;

// Argument definitions
typedef struct TApp_Arg {
   TApp_Type    Type;
   void         **Var;
   char         *Short,*Long,*Info;
} TApp_Arg;

/*Application controller definition*/
typedef struct TApp {
    char*          Name;                 // Name
    char*          Version;              // Version
    char*          Desc;                 // Description
    char*          TimeStamp;            // Compilation timestamp
    char*          LogFile;              // Log file
    int            LogWarning;           // Number of warnings
    int            LogError;             // Number of errors
    int            LogColor;             // Use coloring in the logs
    char*          Tag;                  // Identificateur
    FILE*          LogStream;            // Log file associated stream
    TApp_LogLevel  LogLevel;             // Level of log
    TApp_State     State;                // State of application
    TApp_Lang      Language;             // Language (default: $CMCLNG or APP_EN)
    double         Percent;              // Percentage of execution done (0=not started, 100=finished)
    struct timeval Time;                 // Timer for execution time

    int            Seed,*OMPSeed;        // Random number generator seed
    int           *CountsMPI,*DisplsMPI; // MPI counting and gathering arrays
    int            NbMPI,RankMPI;        // Number of MPI process
    int            NbThread;             // Number of OpenMP threads
} TApp;

typedef int (TApp_InputParseProc) (TApp *App,void *Def,char *Token,char *Value,int Index);

TApp *App_New(char* Name,char* Version,char* Desc,char* Stamp);
void  App_Free(TApp *App);
void  App_Start(TApp *App);
void  App_End(TApp *App,int Status);
int   App_IsDone(TApp *App);
void  App_Log(TApp *App,TApp_LogLevel Level,const char *Format,...);
void  App_LogOpen(TApp *App);
void  App_LogClose(TApp *App);
int   App_LogLevel(TApp *App,char *Val);
void  App_Progress(TApp *App,float Percent,const char *Format,...);
int   App_ParseArgs(TApp *App,TApp_Arg *AArgs,int argc,char *argv[],int Flags);
int   App_ParseInput(TApp *App,void *Def,char *File,TApp_InputParseProc *ParseProc);
int   App_ParseBool(TApp *App,char *Param,char *Value,char *Var);
int   App_ParseDate(TApp *App,char *Param,char *Value,time_t *Var);
int   App_ParseDateSplit(TApp *App,char *Param,char *Value,int *Year,int *Month,int *Day,int *Hour,int *Min);
int   App_ParseCoords(TApp *App,char *Param,char *Value,double *Lat,double *Lon,int Index);
void  App_SeedInit(TApp *App);

void  App_ErrorSet(const char *Format,...);
char* App_ErrorGet(void);

#endif
