/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de fonctions utiles
 * Fichier   : App->c
 * Creation  : Septembre 2008 - J.P. Gauthier
 *
 * Description: Fonctions gé©nériques à toute les applications.
 *
 * Remarques :
 *    This package can uses the following environment variables if defined
 *       APP_PARAMS       : List of parameters for the appliaction (instead of givins on command line) 
 *       APP_VERBOSE      : Define verbose level (ERROR,WARNING,INFO,DEBUG,EXTRA or 0-4)
 *       APP_VERBOSECOLOR : Use color in log messages
 * 
 *       CMCLNG           : Language tu use (francais,english)
 *       OMP_NUM_THREADS  : Number of openMP threads
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
#define APP_BUILD

#include <sched.h>
#include <unistd.h>
#include <sys/signal.h>
#ifndef _AIX
#include <sys/syscall.h>
#endif

#include "App.h"
#include "eerUtils.h"
#include "RPN.h"

static TApp AppInstance;                         // Static App instance
__thread TApp *App=&AppInstance;                 // Per thread App pointer
static __thread char APP_ERROR[APP_ERRORSIZE];   // Last error is accessible through this

char* App_ErrorGet(void) {
   return(APP_ERROR);
}

unsigned int App_OnceTable[APP_MAXONCE];                // Log once table

int App_IsDone(void)       { return(App->State==DONE); }
int App_IsMPI(void)        { return(App->NbMPI>1); }
int App_IsOMP(void)        { return(App->NbThread>1); }
int App_IsSingleNode(void) { return(App->NbNodeMPI==App->NbMPI); }
int App_IsAloneNode(void)  { return(App->NbNodeMPI==1); }

/*----------------------------------------------------------------------------
 * Nom      : <App_Init>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Initialiser la structure App
 *
 * Parametres :
 *    <Type>    : App type (APP_MASTER=single independent process, APP_THREAD=threaded co-process)
 *    <Name>    : Application name
 *    <Version> : Version
 *    <Desc>    : Description
 *    <Stamp>   : TimeStamp
 * 
 * Retour     :
 *  <App>     : Parametres de l'application initialisee
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
TApp *App_Init(int Type,char *Name,char *Version,char *Desc,char* Stamp) {

   char *c;
   
   // In coprocess threaded mode, we need a different App object than the master thread
   App=(Type==APP_THREAD)?(TApp*)malloc(sizeof(TApp)):&AppInstance;
 
   App->Type=Type;
   App->Name=Name?strdup(Name):NULL;
   App->Version=Version?strdup(Version):NULL;
   App->Desc=Desc?strdup(Desc):NULL;
   App->TimeStamp=Stamp?strdup(Stamp):NULL;
   App->LogFile=strdup("stdout");
   App->LogStream=(FILE*)NULL;
   App->LogWarning=0;
   App->LogError=0;
   App->LogColor=FALSE;
   App->Tag=NULL;
   App->LogLevel=INFO;
   App->State=STOP;
   App->Percent=0.0;
   App->Affinity=APP_AFFINITY_NONE;
   App->NbThread=0;
   App->NbMPI=1;
   App->RankMPI=0;
   App->NbNodeMPI=1;
   App->NodeRankMPI=0;
   App->CountsMPI=NULL;
   App->DisplsMPI=NULL;
   App->OMPSeed=NULL;
   App->Seed=time(NULL);
   App->Signal=0;

#ifdef _MPI
   App->NodeComm=MPI_COMM_NULL;
   App->NodeHeadComm=MPI_COMM_NULL;
#endif

   // Check the log parameters in the environment 
   if ((c=getenv("APP_VERBOSE"))) {
      App_LogLevel(c);
   }
   if ((c=getenv("APP_VERBOSECOLOR"))) {
      App->LogColor=TRUE;
   }
   
   // Check the language in the environment 
   if ((c=getenv("CMCLNG"))) {
      App->Language=(c[0]=='f' || c[0]=='F')?APP_FR:APP_EN;
   } else {
      App->Language=APP_EN;
   }
 
   return(App);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_Free>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Liberer une structure App
 *
 * Parametres :
 *
 * Retour     :
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
void App_Free(void) {

   free(App->Name);
   free(App->Version);
   free(App->Desc);
   free(App->LogFile);
   free(App->TimeStamp);

   if (App->Tag) free(App->Tag);

   if (App->CountsMPI) free(App->CountsMPI);
   if (App->DisplsMPI) free(App->DisplsMPI);
   if (App->OMPSeed)   free(App->OMPSeed);
   
   if (App->Type==APP_THREAD) free(App); App=NULL;
}

/*----------------------------------------------------------------------------
 * Nom      : <App_NodeGroup>
 * Creation : Janvier 2017 - J.P. Gauthier
 *
 * But      : Initialiser les communicateurs intra-node et inter-nodes
 *
 * Parametres :
 *
 * Retour     :
 *
 * Remarques  :
 *    - On fait ca ici car quand on combine MPI et OpenMP, les threads se supperpose sur
 *      un meme CPU pour plusieurs job MPI sur un meme "socket@
 *----------------------------------------------------------------------------
*/
int App_NodeGroup() {
   if( App_IsMPI() ) {
#ifdef _MPI
      int i,color,mult;
      char *n,*names,*cptr;

      // Get the physical node unique name of mpi procs
      APP_MEM_ASRT(names,calloc(MPI_MAX_PROCESSOR_NAME*App->NbMPI,sizeof(*names)));

      n=names+App->RankMPI*MPI_MAX_PROCESSOR_NAME;
      APP_MPI_ASRT( MPI_Get_processor_name(n,&i) );
      APP_MPI_ASRT( MPI_Allgather(MPI_IN_PLACE,0,MPI_DATATYPE_NULL,names,MPI_MAX_PROCESSOR_NAME,MPI_CHAR,MPI_COMM_WORLD) );

      // Go through the names and check how many different nodes we have before our node
      for(i=0,color=-1,cptr=names; i<=App->RankMPI; ++i,cptr+=MPI_MAX_PROCESSOR_NAME) {
         ++color;
         if( !strncmp(n,cptr,MPI_MAX_PROCESSOR_NAME) ) {
            break;
         }
      }

      // Check if we have more than one group
      mult = color;
      for(cptr=names; !mult&&i<App->NbMPI; ++i,cptr+=MPI_MAX_PROCESSOR_NAME) {
         if( strncmp(n,cptr,MPI_MAX_PROCESSOR_NAME) ) {
            mult = 1;
         }
      }

      // If we have more than one node
      if( mult ) {
         // Split the MPI procs into node groups
         APP_MPI_ASRT( MPI_Comm_split(MPI_COMM_WORLD,color,App->RankMPI,&App->NodeComm) );

         // Get the number and rank of each nodes in this new group
         APP_MPI_ASRT( MPI_Comm_rank(App->NodeComm,&App->NodeRankMPI) );
         APP_MPI_ASRT( MPI_Comm_size(App->NodeComm,&App->NbNodeMPI) );

         // Create a communicator for the head process of each node
         APP_MPI_ASRT( MPI_Comm_split(MPI_COMM_WORLD,App->NodeRankMPI?MPI_UNDEFINED:0,App->RankMPI,&App->NodeHeadComm) );
      } else {
         App->NbNodeMPI = App->NbMPI;
         App->NodeRankMPI = App->RankMPI;
         App->NodeComm = MPI_COMM_WORLD;
         App->NodeHeadComm = MPI_COMM_NULL;
      }
#endif //_MPI
   } else {
      App->NbNodeMPI = App->NbMPI;
      App->NodeRankMPI = App->RankMPI;
#ifdef _MPI
      App->NodeComm = MPI_COMM_NULL;
      App->NodeHeadComm = MPI_COMM_NULL;
#endif //_MPI
   }

   return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <App_ThreadPlace>
 * Creation : Janvier 2017 - J.P. Gauthier
 *
 * But      : Initialiser l'emplacement des threads
 *
 * Parametres :
 *
 * Retour     :
 *
 * Remarques  :
 *    - On fait ca ici car quand on combine MPI et OpenMP, les threads se supperpose sur
 *      un meme CPU pour plusieurs job MPI sur un meme "socket@
 *----------------------------------------------------------------------------
*/
int App_ThreadPlace(void) {
   
#ifndef _AIX
   // No thread affinity request
   if (!App->Affinity)
      return(TRUE);
   
#ifdef _OPENMP
   if (App->NbThread>1) {
      
      int nbcpu=sysconf(_SC_NPROCESSORS_ONLN);   // Get number of available  cores
      int incmpi=nbcpu/App->NbMPI;               // Number of cores per MPI job
      
      #pragma omp parallel
      {
         cpu_set_t    set;
         unsigned int nid = omp_get_thread_num();
         pid_t        tid = (pid_t) syscall(SYS_gettid);
     
         CPU_ZERO(&set);

         switch(App->Affinity) {
            case APP_AFFINITY_SCATTER:   // Scatter threads evenly across all processors
                    CPU_SET((App->NodeRankMPI*incmpi)+omp_get_thread_num()*incmpi/App->NbThread,&set);
                    break;
               
            case APP_AFFINITY_COMPACT:   // Place threads closely packed
                    CPU_SET((App->NodeRankMPI*App->NbThread)+omp_get_thread_num(),&set);
                    break;
                 
            case APP_AFFINITY_SOCKET:    // Pack threads over scattered MPI (hope it fits with sockets) 
                    CPU_SET((App->NodeRankMPI*incmpi)+omp_get_thread_num(),&set);
                    break;
                          
         }
         sched_setaffinity(tid,sizeof(set),&set);
      }
   }
#endif
#endif
   return(TRUE);
}
  
/*----------------------------------------------------------------------------
 * Nom      : <App_Start>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Initialiser l'execution de l'application et afficher l'entete
 *
 * Parametres :
 *
 * Retour     :
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
void App_Start(void) {

   char rmn[128],*env=NULL;
   int print=0,t,th,mpi;

   // Trap signals (preemption)
   App_Trap(SIGUSR2);
   App_Trap(SIGTERM);

#ifdef HAVE_RMN
   // RMN Lib settings
   c_fstopc("MSGLVL","WARNIN",0);
   c_fstopc("TOLRNC","SYSTEM",0);
   c_ezsetopt("INTERP_DEGREE","LINEAR");
   c_ezsetopt("VERBOSE","NO");
#endif
   
   App->State      = RUN;
   App->LogWarning = 0;
   App->LogError   = 0;
   App->Percent    = 0.0;
   App->Signal     = 0;

   gettimeofday(&App->Time,NULL);

   // Initialize MPI.
#ifdef _MPI
   MPI_Initialized(&mpi);

   if (mpi) {
      MPI_Comm_size(MPI_COMM_WORLD,&App->NbMPI);
      MPI_Comm_rank(MPI_COMM_WORLD,&App->RankMPI);

      App->TotalsMPI=(int*)malloc((App->NbMPI+1)*sizeof(int));
      App->CountsMPI=(int*)malloc((App->NbMPI+1)*sizeof(int));
      App->DisplsMPI=(int*)malloc((App->NbMPI+1)*sizeof(int));

      App_NodeGroup();
   }
#endif

   // Initialize OpenMP
#ifdef _OPENMP
   if (App->NbThread) {
      // If a number of thread was specified on the command line
      omp_set_num_threads(App->NbThread);
   } else {
      // Otherwise try to get it from the environement
      if ((env=getenv("OMP_NUM_THREADS"))) {
         App->NbThread=atoi(env);
      } else {
         App->NbThread=1;
         omp_set_num_threads(0);
      }
   }
   
   // We need to initialize the per thread app pointer
   #pragma omp parallel
   {
      App=&AppInstance;
      
      // Need to define default for ezscint for each thread
      c_ezsetopt("INTERP_DEGREE","LINEAR");
      c_ezsetopt("VERBOSE","NO");
   }
   App_ThreadPlace();
#else
   App->NbThread=1;
#endif

   // Modify seed value for current processor/thread for parallelization.
   App->OMPSeed=(int*)calloc(App->NbThread,sizeof(int));

   if (!App->RankMPI) {

      App_Log(MUST,"-------------------------------------------------------------------------------------\n");
      App_Log(MUST,"Application    : %s %s (%s)\n",App->Name,App->Version,App->TimeStamp);
      App_Log(MUST,"Lib eerUtils   : %s (%s)\n",VERSION,__TIMESTAMP__);

#ifdef HAVE_RMN
      // Extract RMNLIB version
      f77name(rmnlib_version)(rmn,&print,127);
      rmn[126]='\0';
      App_Log(MUST,"Lib RMN        : %s\n",&rmn[22]);
#endif

      App_Log(MUST,"\nStart time     : (UTC) %s",ctime(&App->Time.tv_sec));

#ifdef _OPENMP
      if (App->NbThread>1) {
         // OpenMP specification version
         if       (_OPENMP >= 201811)  App_Log(MUST,"OpenMP threads : %i (Standard: %d -- OpenMP %s5.0)\n",App->NbThread,_OPENMP,_OPENMP>201811?">":"");
         else if  (_OPENMP >= 201511)  App_Log(MUST,"OpenMP threads : %i (Standard: %d -- OpenMP %s4.5)\n",App->NbThread,_OPENMP,_OPENMP>201511?">":"");
         else if  (_OPENMP >= 201307)  App_Log(MUST,"OpenMP threads : %i (Standard: %d -- OpenMP %s4.0)\n",App->NbThread,_OPENMP,_OPENMP>201307?">":"");
         else if  (_OPENMP >= 201107)  App_Log(MUST,"OpenMP threads : %i (Standard: %d -- OpenMP %s3.1)\n",App->NbThread,_OPENMP,_OPENMP>201107?">":"");
         else if  (_OPENMP >= 200805)  App_Log(MUST,"OpenMP threads : %i (Standard: %d -- OpenMP %s3.0)\n",App->NbThread,_OPENMP,_OPENMP>200805?">":"");
         else if  (_OPENMP >= 200505)  App_Log(MUST,"OpenMP threads : %i (Standard: %d -- OpenMP %s2.5)\n",App->NbThread,_OPENMP,_OPENMP>200505?">":"");
         else if  (_OPENMP >= 200203)  App_Log(MUST,"OpenMP threads : %i (Standard: %d -- OpenMP %s2.0)\n",App->NbThread,_OPENMP,_OPENMP>200203?">":"");
         else if  (_OPENMP >= 199810)  App_Log(MUST,"OpenMP threads : %i (Standard: %d -- OpenMP %s1.0)\n",App->NbThread,_OPENMP,_OPENMP>199810?">":"");
         else                          App_Log(MUST,"OpenMP threads : %i (Standard: %d)\n",App->NbThread,_OPENMP);
      }
#endif //_OPENMP

      if (App->NbMPI>1) {
#if defined MPI_VERSION && defined MPI_SUBVERSION
         // MPI specification version
         App_Log(MUST,"MPI processes  : %i (Standard: %d.%d)\n",App->NbMPI,MPI_VERSION,MPI_SUBVERSION);
#else
         App_Log(MUST,"MPI processes  : %i\n",App->NbMPI);
#endif
      }
      App_Log(MUST,"-------------------------------------------------------------------------------------\n\n");
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <App_End>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Finaliser l'execution du modele et afficher le footer
 *
 * Parametres  :
 *   <Status>  : User status to use
 * 
 * Retour      :
 *    <status> : Process exit status to be used
 * 
 * Remarques   :
 *----------------------------------------------------------------------------
*/
int App_End(int Status) {

   struct timeval end,dif;

#ifdef _MPI
   // The Status=INT_MIN means something went wrong and we want to crash gracefully and NOT get stuck 
   // on a MPI deadlock where we wait for a reduce and the other nodes are stuck on a BCast, for example
   if (App->NbMPI>1 && Status!=INT_MIN) {
      if( !App->RankMPI ) {
         MPI_Reduce(MPI_IN_PLACE,&App->LogWarning,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
         MPI_Reduce(MPI_IN_PLACE,&App->LogError,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
      } else {
         MPI_Reduce(&App->LogWarning,NULL,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
         MPI_Reduce(&App->LogError,NULL,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
      }
   }
#endif

   // Select status code based on error number
   if (Status<0) {
      Status=App->LogError?EXIT_FAILURE:EXIT_SUCCESS;
   }
   
   if (!App->RankMPI) {

      gettimeofday(&end,NULL);
      timersub(&end,&App->Time,&dif);

      App_Log(MUST,"\n-------------------------------------------------------------------------------------\n");
      if (App->Signal) {
         App_Log(MUST,"Trapped signal : %i\n",App->Signal);         
      }
      App_Log(MUST,"Finish time    : (UTC) %s",ctime(&end.tv_sec));
      App_Log(MUST,"Execution time : %.4f seconds\n",(float)dif.tv_sec+dif.tv_usec/1000000.0);

      
      if (Status!=EXIT_SUCCESS) {
         App_Log(MUST,"Status         : Error %i (%i Errors)\n",Status,App->LogError);
      } else {
         App_Log(MUST,"Status         : Ok (%i Warnings)\n",App->LogWarning);
      }

      App_Log(MUST,"-------------------------------------------------------------------------------------\n");

      App_LogClose();

      App->State=DONE;
   }
   
   return(App->Signal?128+App->Signal:Status);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_Trap>
 * Creation : Aout 2016 - J.P. Gauthier
 *
 * But      : Trap les signaux afin de terminer gracieusement *
 * Parametres :
 *   <Signal> : Signal to be trapped
 *
 * Retour:
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
void App_TrapProcess(int Signal) {

   App_Log(DEBUG,"Trapped signal %i\n",Signal);
   App->Signal=Signal;
   
   switch(Signal) {
      case SIGUSR2:
      case SIGTERM: App->State=DONE;
   }
}

void App_Trap(int Signal) {
   
   struct sigaction new,old;
   
   new.sa_sigaction=NULL;
   new.sa_handler=App_TrapProcess;
   new.sa_flags=0x0;
   sigemptyset(&new.sa_mask);   
   
   sigaction(Signal,&new,&old);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_LogOpen>
 * Creation : Septembre 2014 - J.P. Gauthier
 *
 * But      : Ouvrir le fichier log
 *
 * Parametres :
 *
 * Retour:
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
void App_LogOpen(void) {
   
   if (!App->LogStream) {
      if (!App->LogFile || strcmp(App->LogFile,"stdout")==0) {
         App->LogStream=stdout;
      } else if (strcmp(App->LogFile,"stderr")==0) {
         App->LogStream=stderr;
      } else {
         if (!App->RankMPI) {
            App->LogStream=fopen(App->LogFile,"w");
         } else {
            App->LogStream=fopen(App->LogFile,"a+");
         }
      }
      if (!App->LogStream) {
         App->LogStream=stdout;
         fprintf(stderr,"(WARNING) Unable to open log stream (%s), will use stdout instead\n",App->LogFile);
      }
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <App_LogClose>
 * Creation : Septembre 2014 - J.P. Gauthier
 *
 * But      : Fermer le fichier log
 *
 * Parametres :
 *
 * Retour:
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
void App_LogClose(void) {

   if (App->LogStream!=stdout && App->LogStream!=stderr) {
      fclose(App->LogStream);
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <App_Log>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Imprimer un message de maniÃ¨re standard
 *
 * Parametres :
 *  <Level>   : Niveau d'importance du message (MUST,ERROR,WARNING,INFO,DEBUG,EXTRA)
 *  <Format>  : Format d'affichage du message
 *  <...>     : Liste des variables du message
 *
 * Retour:
 *
 * Remarques  :
 *   - Cette fonctions s'utilise comme printf sauf qu'il y a un argument de plus,
 *     le niveau d'importance du message.
 *   - le niveau ERROR s'affichera sur de stderr alors que tout les autres seront
 *     sur stdout ou le fichier log
 *----------------------------------------------------------------------------
*/
void App_Log(TApp_LogLevel Level,const char *Format,...) {

   static char *levels[] = { "ERROR","WARNING","INFO","DEBUG","EXTRA" };
   static char *colors[] = { APP_COLOR_RED, APP_COLOR_BLUE, "", APP_COLOR_CYAN, APP_COLOR_CYAN };
   char        *color;
   va_list      args;

   if (!App->LogStream)
      App_LogOpen();

   // Check for once log flag
   if (Level>EXTRA) {
      // If we logged it at least once
      if (Level>>3<APP_MAXONCE && App_OnceTable[Level>>3]++)
         return;
      
      // Real log level
      Level&=0x7;
   }
   
   if (Level==WARNING) App->LogWarning++;
   if (Level==ERROR)   App->LogError++;

   if (Level<=App->LogLevel) {
      color=App->LogColor?colors[Level]:colors[INFO];
      
      if (Level>=0) {
#ifdef _MPI
         if (App_IsMPI())
            fprintf(App->LogStream,"%s#%02d (%s) ",color,App->RankMPI,levels[Level]);
         else 
#endif     
         fprintf(App->LogStream,"%s(%s) ",color,levels[Level]);
      }
      
      va_start(args,Format);
      vfprintf(App->LogStream,Format,args);
      
      va_end(args);

      if (App->LogColor)
         fprintf(App->LogStream,APP_COLOR_RESET);
      
      if (Level==ERROR) {
         // On errors, save for extenal to use (ex: Tcl)
         va_start(args,Format);
         vsnprintf(APP_ERROR,APP_ERRORSIZE,Format,args);
         va_end(args);
     
         // Force flush on error to garantee we'll see it
         fflush(App->LogStream);
      }
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <App_Progress>
 * Creation : Septembre 2014 - J.P. Gauthier
 *
 * But      : Imprimer un message d'indication d'avancement
 *
 * Parametres :
 *  <Percent> : Pourcentage d'avancement
 *  <Format>  : Format d'affichage du message
 *  <...>     : Liste des variables du message
 *
 * Retour:
 *
 * Remarques  :
 *   - Cette fonctions s'utilise comme printf sauf qu'il y a un argument de plus,
 *     le pourcentage d'avancement
 *----------------------------------------------------------------------------
*/
void App_Progress(float Percent,const char *Format,...) {

   va_list      args;

   App->Percent=Percent;

   if (!App->LogStream)
      App_LogOpen();

   fprintf(App->LogStream,"%s(PROGRESS) [%6.2f %%] ",(App->LogColor?APP_COLOR_MAGENTA:""),App->Percent);
   va_start(args,Format);
   vfprintf(App->LogStream,Format,args);
   va_end(args);

   if (App->LogColor)
      fprintf(App->LogStream,APP_COLOR_RESET);
      
   fflush(App->LogStream);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_LogLevel>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Definir le niveau de log courant
 *
 * Parametres :
 *  <Val>     : Niveau de log a traiter
 *
 * Retour:
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
int App_LogLevel(char *Val) {

   if (Val) {
      if (strcasecmp(Val,"ERROR")==0) {
         App->LogLevel=0;
      } else if (strcasecmp(Val,"WARNING")==0) {
         App->LogLevel=1;
      } else if (strcasecmp(Val,"INFO")==0) {
         App->LogLevel=2;
      } else if (strcasecmp(Val,"DEBUG")==0) {
         App->LogLevel=3;
      } else if (strcasecmp(Val,"EXTRA")==0) {
         App->LogLevel=4;
      } else {
         App->LogLevel=(TApp_LogLevel)atoi(Val);
      }
   }
   return(App->LogLevel);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_PrintArgs>
 * Creation : Mars 2014 - J.P. Gauthier
 *
 * But      : Print arguments information.
 *
 * Parametres  :
 *  <AArgs>   : Arguments definition
 *  <Token>   : Invalid token if any, NULL otherwise
 *  <Flags>    : configuration flags
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void App_PrintArgs(TApp_Arg *AArgs,char *Token,int Flags) {
   
   TApp_Arg *aarg=NULL;

   printf("%s (%s):\n\t%s\n\n",App->Name,App->Version,App->Desc);

   if (Token)
      printf("Bad option: %s\n\n",Token);
   
   printf("Usage:");

   // Process app specific argument
   aarg=AArgs;
   while(aarg && aarg->Short) {
      if (aarg->Short[0]=='\0') {
         printf("\n\t    --%-15s %s",aarg->Long,aarg->Info);
      } else {
         printf("\n\t-%s, --%-15s %s",aarg->Short,aarg->Long,aarg->Info);
      }
      aarg++;
   }

   // Process default argument
   if (Flags&APP_ARGSSEED)   printf("\n\t-%s, --%-15s %s","s", "seed",     "Seed (FIXED,"APP_COLOR_GREEN"VARIABLE"APP_COLOR_RESET" or seed)");
   if (Flags&APP_ARGSTHREAD) printf("\n\t-%s, --%-15s %s","t", "threads",     "Number of threads ("APP_COLOR_GREEN"0"APP_COLOR_RESET")");
   if (Flags&APP_ARGSTHREAD) printf("\n\t    --%-15s %s", "affinity",     "Thread affinity ("APP_COLOR_GREEN"NONE"APP_COLOR_RESET",COMPACT,SCATTER,SOCKET)");
   
   printf("\n");
   if (Flags&APP_ARGSLOG)    printf("\n\t-%s, --%-15s %s","l", "log",     "Log file ("APP_COLOR_GREEN"stdout"APP_COLOR_RESET",stderr,file)");
   if (Flags&APP_ARGSLANG)   printf("\n\t-%s, --%-15s %s","a", "language","Language ("APP_COLOR_GREEN"$CMCLNG"APP_COLOR_RESET",english,francais)");
   
   printf("\n\t-%s, --%-15s %s","v", "verbose",      "Verbose level (ERROR,WARNING,"APP_COLOR_GREEN"INFO"APP_COLOR_RESET",DEBUG,EXTRA or 0-4)");
   printf("\n\t    --%-15s %s",      "verbosecolor", "Use color for log messages");
   printf("\n\t-%s, --%-15s %s","h", "help",         "Help info");   
   printf("\n");
}

/*----------------------------------------------------------------------------
 * Nom      : <App_GetArgs>
 * Creation : Mars 2014 - J.P. Gauthier
 *
 * But      : Extract argument value
 *
 * Parametres  :
 *  <AArg>     : Argument definition
 *  <Value>    : Value to extract
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
#define LST_ASSIGN(type,lst,val) *(type)lst=val; lst=(type)lst+1
inline int App_GetArgs(TApp_Arg *AArg,char *Value) {
   
   char *endptr=NULL;
   errno=0;

   if (Value) {
      if ((--AArg->Multi)<0) {
         printf("Too many values for parametre -%s, --%s\n",AArg->Short,AArg->Long);
         exit(EXIT_FAILURE);
      }
      
      switch(AArg->Type&(~APP_FLAG)) {
         case APP_CHAR :  LST_ASSIGN(char**,        AArg->Var,Value);                   break;
         case APP_UINT32: LST_ASSIGN(unsigned int*, AArg->Var,strtol(Value,&endptr,10));break;
         case APP_INT32:  LST_ASSIGN(int*,          AArg->Var,strtol(Value,&endptr,10));break;
         case APP_UINT64: LST_ASSIGN(unsigned long*,AArg->Var,strtol(Value,&endptr,10));break;
         case APP_INT64:  LST_ASSIGN(long*,         AArg->Var,strtol(Value,&endptr,10));break;
         case APP_FLOAT32:LST_ASSIGN(float*,        AArg->Var,strtof(Value,&endptr));   break;
         case APP_FLOAT64:LST_ASSIGN(double*,       AArg->Var,strtod(Value,&endptr));   break;
      }
   } else {
      if (AArg->Type&APP_FLAG) *(int*)AArg->Var=0x01;
   }
   if (!(AArg->Type&APP_FLAG) && (!Value || (endptr && endptr==Value))) {
      printf("Invalid value for parametre -%s, --%s: %s\n",AArg->Short,AArg->Long,Value);
      exit(EXIT_FAILURE);
   }
   return(!errno);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_ParseArgs>
 * Creation : Mars 2014 - J.P. Gauthier
 *
 * But      : Parse default arguments.
 *
 * Parametres  :
 *  <AArgs>    : Arguments definition
 *  <argc>     : Number of argument
 *  <argv>     : Arguments
 *  <Flags>    : configuration flags
 *
 * Retour:
 *  <ok>       : 1 or 0 if failed
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int App_ParseArgs(TApp_Arg *AArgs,int argc,char *argv[],int Flags) {

   int       i=-1,ok=TRUE,ner=TRUE;
   char     *tok,*ptok=NULL,*env=NULL,*endptr=NULL,*str,*tmp;
   TApp_Arg *aarg=NULL;
   
   str=env=getenv("APP_PARAMS");

   if (argc==1 && !env && Flags&APP_NOARGSFAIL) {
      App_PrintArgs(AArgs,NULL,Flags) ;
      ok=FALSE;
   } else {

      // Parse parameters either on command line or through environment variable
      i=1;
      while((i<argc && (tok=argv[i])) || (env && (tok=strtok(str," ")))) {
         str=NULL;

         // Check if token is a flag or a value (for multi-value parameters)
         if (tok[0]!='-' && ptok) {
            tok=ptok;
            --i;
         }

         // Process default argument
         if ((Flags&APP_ARGSLANG) && (!strcasecmp(tok,"-a") || !strcasecmp(tok,"--language"))) {  // language (en,fr)
            i++;
            if ((ner=ok=(i<argc && argv[i][0]!='-'))) {
               tmp=env?strtok(str," "):argv[i];
               if (tmp[0]=='f' || tmp[0]=='F') {
                  App->Language=APP_FR;
               } else if  (tmp[0]=='e' || tmp[0]=='E') {
                  App->Language=APP_EN;
               } else {
                  printf("Invalid value for language, must be francais or english\n");
                  exit(EXIT_FAILURE);               
               }
            }
         } else if ((Flags&APP_ARGSLOG) && (!strcasecmp(tok,"-l") || !strcasecmp(tok,"--log"))) { // Log file
            i++;
            if ((ner=ok=(i<argc && argv[i][0]!='-'))) {
               free(App->LogFile);
               App->LogFile=strdup(env?strtok(str," "):argv[i]);
            }
         } else if ((Flags&APP_ARGSTHREAD) && (!strcasecmp(tok,"-t") || !strcasecmp(tok,"--threads"))) { // Threads
            i++;
            if ((ner=ok=(i<argc && argv[i][0]!='-'))) {
               tmp=env?strtok(str," "):argv[i];
               App->NbThread=strtol(tmp,&endptr,10);
            }
         } else if ((Flags&APP_ARGSTHREAD) && !strcasecmp(tok,"--affinity")) { // Threads
            i++;
            if ((ner=ok=(i<argc && argv[i][0]!='-'))) {
               tmp=env?strtok(str," "):argv[i];
               if (!strcasecmp(tmp,"NONE")) {
                  App->Affinity=APP_AFFINITY_NONE;
               } else if (!strcasecmp(tmp,"COMPACT")) {
                  App->Affinity=APP_AFFINITY_COMPACT;
               } else if (!strcasecmp(tmp,"SCATTER")) {
                  App->Affinity=APP_AFFINITY_SCATTER;
               } else if (!strcasecmp(tmp,"SOCKET")) {
                  App->Affinity=APP_AFFINITY_SOCKET;
               } else {
                  printf("Invalid value for thread affinity, NONE, COMPACT, SCATTER or SOCKET\n");
                  exit(EXIT_FAILURE);               
               }
            }
//         } else if ((Flags&APP_ARGSTMPDIR) && (!strcasecmp(tok,"--tmpdir"))) { // Use tmpdir if available
//            i++;
//            if ((ner=ok=(i<argc && argv[i][0]!='-'))) {
//               App->r=env?strtok(str," "):argv[i];
//            }
         } else if ((Flags&APP_ARGSSEED) && (!strcasecmp(tok,"-s") || !strcasecmp(tok,"--seed"))) { // Seed
            i++;
            if ((ner=ok=(i<argc && argv[i][0]!='-'))) {
               tmp=env?strtok(str," "):argv[i];
               if (strcasecmp(tmp,"VARIABLE")==0 || strcmp(tmp,"1")==0) {
                  // Seed is variable, according to number of elapsed seconds since January 1 1970, 00:00:00 UTC.
               } else if (strcasecmp(tmp,"FIXED")==0 || strcmp(tmp,"0")==0) {
                  // Seed is fixed
                  App->Seed = APP_SEED;
               } else {
                  // Seed is user defined
                  App->Seed=strtol(tmp,&endptr,10);
               }
            }
         } else if (!strcasecmp(tok,"-v") || !strcasecmp(tok,"--verbose")) {                      // Verbose degree
            i++;
            if ((ner=ok=(i<argc && argv[i][0]!='-'))) {
               App_LogLevel(env?strtok(str," "):argv[i]);
            }
         } else if (!strcasecmp(tok,"--verbosecolor")) {                                          // Use color in log messages
            App->LogColor=TRUE;
         } else if (!strcasecmp(tok,"-h") || !strcasecmp(tok,"--help")) {                         // Help
            App_PrintArgs(AArgs,NULL,Flags) ;
            exit(EXIT_SUCCESS);
         } else {
            // Process specific argument
            aarg=AArgs;
            while(aarg->Short) {        
               if ((aarg->Short && tok[1]==aarg->Short[0] && tok[2]=='\0') || (aarg->Long && strcasecmp(&tok[2],aarg->Long)==0)) {
                  ok=(aarg->Type==APP_FLAG?(*(int*)aarg->Var)=TRUE:App_GetArgs(aarg,env?strtok(str," "):((i+1<argc && argv[i+1][0]!='-')?argv[++i]:NULL)));
                  ptok=aarg->Type==APP_FLAG?NULL:tok;
                  break;
               }
               aarg++;
            }
         }
         
         if (!ner) {
             printf("Missing argument for %s\n",argv[--i]);
             exit(EXIT_FAILURE);               
         }
         
         // Argument not found
         if (aarg && (!ok || !aarg->Short)) {
            App_PrintArgs(AArgs,tok,Flags) ;
            ok=FALSE;
            break;
         }
         
         ++i;
      }
   }
   
   return(ok);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_ParseInput>
 * Creation : Avril 2010 - J.P. Gauthier
 *
 * But      : Parse an imput file.
 *
 * Parametres  :
 *  <Def>      : Model definitions
 *  <File>     : Input file to parse
 *  <ParseProc>: Model specific token parsing proc
 *
 * Retour:
 *  <nbtoken>  : Number of token parsed or 0 if failed
 *
 * Remarques :
 *   - This proc will parse an input file with the format TOKEN=VALUE
 *   - It will skip any comment and blank lines
 *   - It also allows for multiline definitions
 *----------------------------------------------------------------------------
*/
int App_ParseInput(void *Def,char *File,TApp_InputParseProc *ParseProc) {

   FILE *fp;
   int   n=0,seq=0;
   char  token[256],*parse,*values,*value,*valuesave,*idx,*buf,*tokensave;

   if (!(fp=fopen(File,"r"))) {
      App_Log(ERROR,"Unable to open input file: %s\n",File);
      return(0);
   }

   if (!(buf=(char*)alloca(APP_BUFMAX))) {
      App_Log(ERROR,"Unable to allocate input parsing buffer\n");
      return(0);
   }

   while(fgets(buf,APP_BUFMAX,fp)) {

      //Check for comments
      strtrim(buf,'\n');
      strrep(buf,'\t',' ');
      if ((idx=index(buf,'#'))) *idx='\0';

      //Parse the token
      parse=NULL;
      if ((idx=index(buf,'='))) {
         tokensave=NULL;
         parse=strtok_r(buf,"=",&tokensave);
      }

      if (parse) {
         // If we find a token, remove spaces and get the associated value
         strtrim(parse,' ');
         strncpy(token,parse,256);
         values=strtok_r(NULL,"=",&tokensave);
         seq=0;
         n++;
      } else {
        // Otherwise, keep the last token and get a new value for it
         values=buf;
         strtrim(values,' ');
      }

      // Loop on possible space separated values
      if (values && strlen(values)>1) {
         valuesave=NULL;
         while((value=strtok_r(values," ",&valuesave))) {
            if (seq) {
               App_Log(DEBUG,"Input parameters: %s(%i) = %s\n",token,seq,value);
            } else {
               App_Log(DEBUG,"Input parameters: %s = %s\n",token,value);
            }

            // Call mode specific imput parser
            if (!ParseProc(Def,token,value,seq)) {
               fclose(fp);
               return(0);
            }
            seq++;
            values=NULL;
         }
      }
   }

   fclose(fp);
   return(n);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_ParseBool>
 * Creation : Fevrier 2013 - J.P. Gauthier
 *
 * But      : Parse a boolean value.
 *
 * Parametres :
 *  <Param>   : Nom du parametre
 *  <Value>   : Value to parse
 *  <Var>     : Variable to put result into
 *
 * Retour:
 *  <ok>     : 1 = ok or 0 = failed
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int App_ParseBool(char *Param,char *Value,char *Var) {
   
  if (strcasecmp(Value,"true")==0 || strcmp(Value,"1")==0) {
      *Var=1;
   } else if (strcasecmp(Value,"false")==0 || strcmp(Value,"0")==0) {
      *Var=0;
   } else {
      App_Log(ERROR,"Invalid value for %s, must be TRUE(1) or FALSE(0): %s\n",Param,Value);
      return(0);
   }
   return(1);
}   

/*----------------------------------------------------------------------------
 * Nom      : <App_ParseDate>
 * Creation : Fevrier 2013 - J.P. Gauthier
 *
 * But      : Parse a date value (YYYMMDDhhmm.
 *
 * Parametres :
 *  <Param>   : Nom du parametre
 *  <Value>   : Value to parse
 *  <Var>     : Variable to put result into
 *
 * Retour:
 *  <ok>     : 1 = ok or 0 = failed
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int App_ParseDate(char *Param,char *Value,time_t *Var) {
   
   long long t;
   char     *ptr;

   if( (t=strtoll(Value,&ptr,10))<=0 ) {
      App_Log(ERROR,"Invalid value for %s, must be YYYYMMDDHHMM or YYYYMMDDHHMMSS: %s\n",Param,Value);
      return(0);
   }
   switch( strlen(Value) ) {
      case 12:
         *Var=System_DateTime2Seconds(t/10000,(t-(t/10000*10000))*100,1);
         break;
      case 14:
         *Var=System_DateTime2Seconds(t/1000000,(t-(t/1000000*1000000)),1);
         break;
      default:
         App_Log(ERROR,"Invalid value for %s, must be YYYYMMDDHHMM or YYYYMMDDHHMMSS: %s\n",Param,Value);
         return(0);
   }

   return(1);
}   

int App_ParseDateSplit(char *Param,char *Value,int *Year,int *Month,int *Day,int *Hour,int *Min) {
   
   long long t;
   char     *ptr;

   if (strlen(Value)!=12 || (t=strtoll(Value,&ptr,10))<=0) {
      App_Log(ERROR,"Invalid value for %s, must be YYYYMMDDHHMM: %s\n",Param,Value);
      return(0);
   }
   *Year=t/100000000;  t-=(long long)(*Year)*100000000;
   *Month=t/1000000;   t-=(*Month)*1000000;
   *Day=t/10000;       t-=(*Day)*10000;
   *Hour=t/100;        t-=(*Hour)*100;
   *Min=t;
   
   return(1);
}   

/*----------------------------------------------------------------------------
 * Nom      : <App_ParseCoord>
 * Creation : Aout 2013 - J.P. Gauthier
 *
 * But      : Parse a coordinate  value.
 *
 * Parametres :
 *  <Param>   : Nom du parametre
 *  <Value>   : Value to parse
 *  <Var>     : Variable to put result into
 *  <Index>   : Coordinate index (0:Lat, 1:Lon, 2:Height, 3:Speed)
 *
 * Retour:
 *  <ok>     : 1 = ok or 0 = failed
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int App_ParseCoords(char *Param,char *Value,double *Lat,double *Lon,int Index) {
   
   double coord;
   char *ptr;
   
   coord=strtod(Value,&ptr);

   switch(Index) {
      case 0: 
         if (coord<-90.0 || coord>90.0) {
            App_Log(ERROR,"Invalid latitude coordinate: %s\n",Value);
            return(0);
         }
         *Lat=coord;
         break;
      case 1:
         if (coord<0.0) coord+=360.0;
         if (coord<0.0 || coord>360.0) {
            App_Log(ERROR,"Invalid longitude coordinate: %s\n",Value);
            return(0);
         }
         *Lon=coord;
         break;
   }

   return(1);
}
      
/*----------------------------------------------------------------------------
 * Nom      : <App_SeedInit>
 * Creation : Aout 2011 - J.P. Gauthier
 *
 * But      : Initialise seeds for MPI/OpenMP.
 *
 * Parametres  :
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
void App_SeedInit() {

   int t;

   // Modify seed value for current processor/thread for parallelization.
   for(t=1;t<App->NbThread;t++)
      App->OMPSeed[t]=App->Seed+1000000*(App->RankMPI*App->NbThread+t);

   App->OMPSeed[0]=App->Seed+=1000000*App->RankMPI;
}

