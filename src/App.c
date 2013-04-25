/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
//  * Dorval, Quebec
 *
 * Projet    : Librairie de fonctions utiles
 * Fichier   : App.c
 * Creation  : Septembre 2008 - J.P. Gauthier
 * Revision  : $Id: Model.c 715 2012-08-31 14:26:15Z afsr005 $
 *
 * Description: Fonctions génériques à toute les applications.
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
#include "App.h"
#include "eerUtils.h"

/*----------------------------------------------------------------------------
 * Nom      : <App_New>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Creer une nouvelle structure App
 *
 * Parametres :
 *
 * Retour     :
 *  <App>     : Parametres de l'application initialisee
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
TApp *App_New(char *Name,char *Version) {

   TApp *app;

   app=(TApp*)malloc(sizeof(TApp));
   app->Name=strdup(Name);
   app->Version=strdup(Version);
   app->LogFile=strdup("stdout");
   app->LogStream=(FILE*)NULL;
   app->LogWarning=0;
   app->Tag=NULL;
   app->LogLevel=2;
   app->State=STOP;
   app->Percent=0.0;
   app->NbThread=1;
   app->NbMPI=1;
   app->RankMPI=0;
   app->CountsMPI=NULL;
   app->DisplsMPI=NULL;
   app->OMPSeed=NULL;
   app->Seed=time(NULL);

   return(app);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_Free>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Liberer une structure App
 *
 * Parametres :
 *  <App>     : Parametres de l'application
 *
 * Retour     :
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
void App_Free(TApp *App) {

   free(App->Name);
   free(App->Version);
   free(App->LogFile);

   if (App->Tag) free(App->Tag);

   if (App->CountsMPI) free(App->CountsMPI);
   if (App->DisplsMPI) free(App->DisplsMPI);
   if (App->OMPSeed)   free(App->OMPSeed);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_IsDone>
 * Creation : Aout 2011 - J.P. Gauthier
 *
 * But      : Check if the application has finished.
 *
 * Parametres  :
 *  <App>     : Parametres de l'application
 *
 * Retour:
 *  <Done>     : Finished (0,1)
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int App_IsDone(TApp *App) {

   if (App->State==DONE) {
      return(1);
   }
   return(0);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_Start>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Initialiser l'execution de l'application et afficher l'entete
 *
 * Parametres :
 *  <App>     : Parametres de l'application
 *
 * Retour     :
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
void App_Start(TApp *App) {

   char rmn[128],*env=NULL;
   int print=1,t,mpi;

#ifdef HAVE_RMN
   // RMN Lib settings
   c_fstopc("MSGLVL","WARNIN",0);
   c_fstopc("TOLRNC","SYSTEM",0);
   c_ezsetopt("INTERP_DEGREE","LINEAR");
#endif
   
   App->State=RUN;

   gettimeofday(&App->Time,NULL);

   // Initialize MPI.
#ifdef _MPI
   MPI_Initialized(&mpi);

   if (mpi) {
      MPI_Comm_size(MPI_COMM_WORLD,&App->NbMPI);
      MPI_Comm_rank(MPI_COMM_WORLD,&App->RankMPI);

      App->CountsMPI=(int*)malloc(App->NbMPI*sizeof(int));
      App->DisplsMPI=(int*)malloc(App->NbMPI*sizeof(int));
   }
#endif

   // Initialize OpenMP
#ifdef _OPENMP
   if (App->NbThread>1) {
      // If a number of thread was specified on the command line
      omp_set_num_threads(App->NbThread);
   } else {
      // Otherwise try to get it from the environement
      if ((env=getenv("OMP_NUM_THREADS"))) {
         App->NbThread=atoi(env);
      } else {
         omp_set_num_threads(0);
      }
   }
#else
   App->NbThread=1;
#endif

   // Modify seed value for current processor/thread for parallelization.
   App->OMPSeed=(int*)calloc(App->NbThread,sizeof(int));

   if (!App->RankMPI) {

      App_Log(App,MUST,"-------------------------------------------------------------------------------\n");
      App_Log(App,MUST,"Model          : %s (%s)\n",App->Name,App->Version);
      App_Log(App,MUST,"Lib Utils      : %s\n",VERSION);

#ifdef HAVE_RMN
      // Extract RMNLIB version
      f77name(rmnlib_version)(rmn,&print,127);
      rmn[89]='\0';
      App_Log(App,MUST,"Lib RMN        : %s\n",&rmn[13]);
#endif
      App_Log(App,MUST,"Start time     : (UTC) %s",ctime(&App->Time.tv_sec));

      if (App->NbThread>1) {
         App_Log(App,MUST,"OpenMP threads : %i\n",App->NbThread);

      }
      if (App->NbMPI>1) {
         App_Log(App,MUST,"MPI processes  : %i\n",App->NbMPI);

      }
      App_Log(App,MUST,"-------------------------------------------------------------------------------\n\n");
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <App_End>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Finaliser l'execution du modele et afficher le footer
 *
 * Parametres :
 *  <App>     : Parametres de l'application
 *
 * Retour     :
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
void App_End(TApp *App,int Status) {

   struct timeval end;
   int            wrng;

#ifdef _MPI
   if (App->NbMPI>1) {
      MPI_Reduce(&App->LogWarning,&wrng,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
      App->LogWarning=wrng;
   }
#endif

   if (!App->RankMPI) {

      gettimeofday(&end,NULL);

      App_Log(App,MUST,"\n-------------------------------------------------------------------------------\n");
      App_Log(App,MUST,"Finish time    : (UTC) %s",ctime(&end.tv_sec));
      App_Log(App,MUST,"Execution time : %li seconds\n",end.tv_sec-App->Time.tv_sec);

      if (Status<1) {
         App_Log(App,MUST,"Status         : Error (%i)\n",Status);
      } else {
         App_Log(App,MUST,"Status         : Ok (%i Warnings)\n",App->LogWarning);
      }

      App_Log(App,MUST,"-------------------------------------------------------------------------------\n");

      if (App->LogStream!=stdout && App->LogStream!=stderr) {
         fclose(App->LogStream);
      }

      App->State=DONE;
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <App_Log>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Imprimer un message de manière standard
 *
 * Parametres :
 *  <App>     : Parametres de l'application
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
void App_Log(TApp *App,TApp_LogLevel Level,const char *Format,...) {

   static char *levels[] = { "ERROR","WARNING","INFO","DEBUG","EXTRA" };
   va_list args;

   if (!App->LogStream) {
      if (strcmp(App->LogFile,"stdout")==0) {
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

   if (Level==WARNING) App->LogWarning++;

   if (Level<=App->LogLevel) {
      if (Level>=0) {
         fprintf(App->LogStream,"(%s) ",levels[Level]);
      }
      va_start(args,Format);
      if (Level==ERROR) {
         va_start(args,Format);
         vfprintf(stderr,Format,args);
         va_end(args);
      }
      va_start(args,Format);
      vfprintf(App->LogStream,Format,args);
      va_end(args);

      fflush(App->LogStream);
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <App_Log>
 * Creation : Septembre 2008 - J.P. Gauthier
 *
 * But      : Imprimer un message de manière standard
 *
 * Parametres :
 *  <App>     : Parametres de l'application
 *  <Val>     : Niveau de log a traiter
 *
 * Retour:
 *
 * Remarques  :
 *----------------------------------------------------------------------------
*/
int App_LogLevel(TApp *App,char *Val) {

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
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_InputParse>
 * Creation : Avril 2010 - J.P. Gauthier
 *
 * But      : Parse an imput file.
 *
 * Parametres  :
 *  <App>     : Parametres de l'application
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
int App_ParseInput(TApp *App,void *Def,char *File,TApp_InputParseProc *ParseProc) {

   FILE *fp;
   int   n=0,seq;
   char  token[256],*parse,*values,*value,*valuesave,*idx,*buf,*tokensave;

   if (!(fp=fopen(File,"r"))) {
      App_Log(App,ERROR,"Unable to open input file: %s\n",File);
      return(0);
   }

   if (!(buf=(char*)malloc(APP_BUFMAX))) {
      App_Log(App,ERROR,"Unable to allocate input parsing buffer\n");
      return(0);
   }

   while(fgets(buf,APP_BUFMAX,fp)) {

      //Check for comments
      strtrim(buf,'\n');
      if (idx=index(buf,'#')) *idx='\0';

      //Parse the token
      parse=NULL;
      if (idx=index(buf,'=')) {
         tokensave=NULL;
         parse=strtok_r(buf,"=",&tokensave);
      }

      if (parse) {
         // If we find a token, remove spaces and get the associated value
         strrep(parse,'\t',' ');
         strtrim(parse,' ');
         strncpy(token,parse,256);
         values=strtok_r(NULL,"=",&tokensave);
         n++;
         seq=0;
      } else {
        // Otherwise, keep the last token and get a new value for it
         values=buf;
         strtrim(values,' ');
      }

      // Loop on possible space separated values
      if (values && strlen(values)>1) {
         valuesave=NULL;
         while(value=strtok_r(values," ",&valuesave)) {
            if (seq) {
               App_Log(App,DEBUG,"Input parameters: %s(%i) = %s\n",token,seq,value);
            } else {
               App_Log(App,DEBUG,"Input parameters: %s = %s\n",token,value);
            }

            // Call mode specific imput parser
            if (!ParseProc(App,Def,token,value,seq)) {
               fclose(fp);
               free(buf);
               return(0);
            }
            seq++;
            values=NULL;
         }
      }
   }

   free(buf);
   return(n);
}

/*----------------------------------------------------------------------------
 * Nom      : <App_ParseBool>
 * Creation : Fevrier 2013 - J.P. Gauthier
 *
 * But      : Parse a boolean value.
 *
 * Parametres :
 *  <App>     : Parametres de l'application
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
int App_ParseBool(TApp *App,char *Param,char *Value,char *Var) {
   
  if (strcasecmp(Value,"true")==0 || strcmp(Value,"1")==0) {
      *Var=1;
   } else if (strcasecmp(Value,"false")==0 || strcmp(Value,"0")==0) {
      *Var=0;
   } else {
      App_Log(App,ERROR,"Invalid value for %s, must be TRUE(1) or FALSE(0): %s\n",Param,Value);
      return(0);
   }
   return(1);
}   


/*----------------------------------------------------------------------------
 * Nom      : <Model_SeedInit>
 * Creation : Aout 2011 - J.P. Gauthier
 *
 * But      : Initialise seeds for MPI/OpenMP.
 *
 * Parametres  :
 *  <App>     : Parametres de l'application
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
void App_SeedInit(TApp *App) {

   int t;

   // Modify seed value for current processor/thread for parallelization.
   for(t=1;t<App->NbThread;t++)
      App->OMPSeed[t]=App->Seed+1000000*(App->RankMPI*App->NbThread+t);

   App->OMPSeed[0]=App->Seed+=1000000*App->RankMPI;
}
