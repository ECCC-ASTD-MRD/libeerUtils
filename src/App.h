/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de fonctions utiles
 * Fichier   : App.h
 * Creation  : Septembre 2008 - J.P. Gauthier
 * Revision  : $Id: Model.h 715 2012-08-31 14:26:15Z afsr005 $
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

#ifdef _OPENMP
#   include <omp.h>
#endif

#ifdef _MPI
#   include <mpi.h>
#endif

#define APP_BUFMAX   32768      // Maximum input buffer length
#define APP_SEED     1049731793 // Initial FIXED seed

typedef enum { MUST=-1,ERROR=0,WARNING=1,INFO=2,DEBUG=3,EXTRA=4 } TLogLevel;
typedef enum { STOP,RUN,DONE } TAppState;

/*Application controller definition*/
typedef struct TApp {
    char*          Name;                 // Name
    char*          Version;              // Version
    char*          LogFile;              // Log file
    int            LogWarning;           // Number of warnings
    char*          Tag;                  // Identificateur
    FILE*          LogStream;            // Log file associated stream
    TLogLevel      LogLevel;             // Level of log
    TAppState      State;                // State of application
    double         Percent;              // Percentage of execution done (0=not started, 100=finished)
    struct timeval Time;                 // Timer for execution time

    int            Seed,*OMPSeed;        // Random number generator seed
    int           *CountsMPI,*DisplsMPI; // MPI couting and gathering arrays
    int            NbMPI,RankMPI;        // Number of MPI process
    int            NbThread;             // Number of OpenMP threads
} TApp;

typedef int (TApp_InputParseProc) (TApp *App,void *Def,char *Token,char *Value,int Index);

TApp *App_New(char* Name,char* Version);
void  App_Free(TApp *App);
void  App_Start(TApp *App);
void  App_End(TApp *App,int Status);
int   App_Done(TApp *App);
void  App_Log(TApp *App,TLogLevel Level,const char *Format,...);
int   App_InputParse(TApp *App,void *Def,char *File,TApp_InputParseProc *ParseProc);
void  App_SeedInit(TApp *App);

#endif
