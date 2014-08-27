/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
  * Dorval, Quebec
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
TApp *App_New(char *Name,char *Version,char *Desc,char* Stamp) {

   TApp *app;

   app=(TApp*)malloc(sizeof(TApp));
   app->Name=Name?strdup(Name):NULL;
   app->Version=Version?strdup(Version):NULL;
   app->Desc=Desc?strdup(Desc):NULL;
   app->TimeStamp=Stamp?strdup(Stamp):NULL;
   app->LogFile=strdup("stdout");
   app->LogStream=(FILE*)NULL;
   app->LogWarning=0;
   app->LogError=0;
   app->LogColor=FALSE;
   app->Tag=NULL;
   app->LogLevel=INFO;
   app->State=STOP;
   app->Percent=0.0;
   app->NbThread=1;
   app->NbMPI=1;
   app->RankMPI=0;
   app->CountsMPI=NULL;
   app->DisplsMPI=NULL;
   app->OMPSeed=NULL;
   app->Seed=time(NULL);

   // Check the language in the environment
   if (!(app->Language=getenv("CMCLNG"))) {
      app->Language=strdup("english");
   }

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
   free(App->Desc);
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
   int print=0,t,mpi;

#ifdef HAVE_RMN
   // RMN Lib settings
   c_fstopc("MSGLVL","WARNIN",0);
   c_fstopc("TOLRNC","SYSTEM",0);
   c_ezsetopt("INTERP_DEGREE","LINEAR");
   c_ezsetopt("VERBOSE","NO");
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
      App_Log(App,MUST,"Application    : %s %s (%s)\n",App->Name,App->Version,App->TimeStamp);
      App_Log(App,MUST,"Lib eerUtils   : %s (%s)\n",VERSION,__TIMESTAMP__);

#ifdef HAVE_RMN
      // Extract RMNLIB version
      f77name(rmnlib_version)(rmn,&print,127);
      rmn[126]='\0';
      App_Log(App,MUST,"Lib RMN        : %s\n",&rmn[22]);
#endif
      App_Log(App,MUST,"\nStart time     : (UTC) %s",ctime(&App->Time.tv_sec));

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
   int            nb;

#ifdef _MPI
   if (App->NbMPI>1) {
      MPI_Reduce(&App->LogWarning,&nb,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
      App->LogWarning=nb;
      MPI_Reduce(&App->LogError,&nb,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
      App->LogError=nb;
   }
#endif

   if (!App->RankMPI) {

      gettimeofday(&end,NULL);

      App_Log(App,MUST,"\n-------------------------------------------------------------------------------\n");
      App_Log(App,MUST,"Finish time    : (UTC) %s",ctime(&end.tv_sec));
      App_Log(App,MUST,"Execution time : %li seconds\n",end.tv_sec-App->Time.tv_sec);

      // Select status code based on error number
      if (Status<0) {
         Status=App->LogError?1:0;
      }
      
      if (Status!=0) {
         App_Log(App,MUST,"Status         : Error %i (%i Errors)\n",Status,App->LogError);
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
   static char *colors[] = { APP_COLOR_RED, APP_COLOR_BLUE, "", APP_COLOR_CYAN, APP_COLOR_CYAN };
   char        *color;
   va_list      args;

   color=App->LogColor?colors[Level]:colors[INFO];
      
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
   if (Level==ERROR)   App->LogError++;

   if (Level<=App->LogLevel) {
      if (Level>=0) {
         fprintf(App->LogStream,"%s(%s) ",color,levels[Level]);
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

      if (App->LogColor)
         fprintf(App->LogStream,APP_COLOR_RESET);
      
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
 * Nom      : <App_PrintArgs>
 * Creation : Mars 2014 - J.P. Gauthier
 *
 * But      : Print arguments information.
 *
 * Parametres  :
 *  <App>     : Parametres de l'application
 *  <AArgs>   : Arguments definition
 *  <Token>   : Invalid token if any, NULL otherwise
 *  <Flags>    : configuration flags
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/

void App_PrintArgs(TApp *App,TApp_Arg *AArgs,char *Token,int Flags) {
   
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
   printf("\n");
   if (!(Flags&APP_NOARGSLOG)) {
      printf("\n\t-%s, --%-15s %s","l", "log",     "Log file ("APP_COLOR_GREEN"stdout"APP_COLOR_RESET",stderr,file)");
   }
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
 *  <App>      : Parametres de l'application
 *  <AArg>     : Argument definition
 *  <Value>    : Value to extract
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
inline int App_GetArgs(TApp *App,TApp_Arg *AArg,char *Value) {
   
   char *endptr=NULL;
   errno=0;

   if (Value) {   
      switch(AArg->Type&(~APP_FLAG)) {
         case APP_LIST :  (*AArg->Var++)=Value;                                   break;
         case APP_CHAR :  (*AArg->Var)=Value;                                     break;
         case APP_UINT32: (*(unsigned int*)AArg->Var)=strtol(Value,&endptr,10);   break;
         case APP_INT32:  (*(int*)AArg->Var)=strtol(Value,&endptr,10);            break;
         case APP_UINT64: (*(unsigned long*)AArg->Var)=strtol(Value,&endptr,10);  break;
         case APP_INT64:  (*(long*)AArg->Var)=strtol(Value,&endptr,10);           break;
         case APP_FLOAT32:(*(float*)AArg->Var)=strtof(Value,&endptr);             break;
         case APP_FLOAT64:(*(double*)AArg->Var)=strtod(Value,&endptr);            break;
      }
   } else {
      if (AArg->Type&APP_FLAG) (*AArg->Var)=(void*)0x01;
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
 *  <App>      : Parametres de l'application
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
int App_ParseArgs(TApp *App,TApp_Arg *AArgs,int argc,char *argv[],int Flags) {

   int       i=-1,ok=TRUE;
   char     *tok,*ptok=NULL,*env=NULL,*str;
   TApp_Arg *aarg=NULL;
   
   str=env=getenv("APP_PARAMS");

   if (argc==1 && !env && Flags&APP_NOARGSFAIL) {
      App_PrintArgs(App,AArgs,NULL,Flags) ;
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
         if (!(Flags&APP_NOARGSLOG) && (strcasecmp(tok,"-l")==0 || strcasecmp(tok,"--log")==0)) {               // Log file
            App->LogFile=env?strtok(str," "):argv[++i];
         } else if (strcasecmp(tok,"-v")==0 || strcasecmp(tok,"--verbose")==0) {    // Verbose degree
            App_LogLevel(App,env?strtok(str," "):argv[++i]);
         } else if (strcasecmp(tok,"--verbosecolor")==0) {                          // Use color in log messages
            App->LogColor=TRUE;
         } else if (strcasecmp(tok,"-h")==0 || strcasecmp(tok,"--help")==0) {       // Help
            App_PrintArgs(App,AArgs,NULL,Flags) ;
            exit(EXIT_SUCCESS);
         } else {
            // Process specific argument
            aarg=AArgs;
            while(aarg->Short) {        
               if ((aarg->Short && tok[1]==aarg->Short[0] && tok[2]=='\0') || (aarg->Long && strcasecmp(&tok[2],aarg->Long)==0)) {
                  ok=(aarg->Type==APP_FLAG?(*(int*)aarg->Var)=TRUE:App_GetArgs(App,aarg,env?strtok(str," "):argv[++i]));
                  break;
               }
               aarg++;
            }
         }
         
         // Argument not found
         if (aarg && (!ok || !aarg->Short)) {
            App_PrintArgs(App,AArgs,tok,Flags) ;
            ok=FALSE;
            break;
         }
         
         ++i;
         ptok=tok;
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

   if (!(buf=(char*)alloca(APP_BUFMAX))) {
      App_Log(App,ERROR,"Unable to allocate input parsing buffer\n");
      return(0);
   }

   while(fgets(buf,APP_BUFMAX,fp)) {

      //Check for comments
      strtrim(buf,'\n');
      strrep(buf,'\t',' ');
      if (idx=index(buf,'#')) *idx='\0';

      //Parse the token
      parse=NULL;
      if (idx=index(buf,'=')) {
         tokensave=NULL;
         parse=strtok_r(buf,"=",&tokensave);
      }

      if (parse) {
         // If we find a token, remove spaces and get the associated value
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
               return(0);
            }
            seq++;
            values=NULL;
         }
      }
   }

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
 * Nom      : <App_ParseDate>
 * Creation : Fevrier 2013 - J.P. Gauthier
 *
 * But      : Parse a date value (YYYMMDDhhmm.
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
int App_ParseDate(TApp *App,char *Param,char *Value,time_t *Var) {
   
   long long t;
   char     *ptr;

   if (strlen(Value)!=12 || (t=strtoll(Value,&ptr,10))<=0) {
       App_Log(App,ERROR,"Invalid value for %s, must be YYYYMMDDHHMM: %s\n",Param,Value);
      return(0);
   }
   *Var=System_DateTime2Seconds(t/10000,(t-(t/10000*10000))*100,1);

   return(1);
}   

int App_ParseDateSplit(TApp *App,char *Param,char *Value,int *Year,int *Month,int *Day,int *Hour,int *Min) {
   
   long long t;
   char     *ptr;

   if (strlen(Value)!=12 || (t=strtoll(Value,&ptr,10))<=0) {
      App_Log(App,ERROR,"Invalid value for %s, must be YYYYMMDDHHMM: %s\n",Param,Value);
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
 *  <App>     : Parametres de l'application
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
int App_ParseCoords(TApp *App,char *Param,char *Value,double *Lat,double *Lon,int Index) {
   
   double coord;
   char *ptr;
   
   coord=strtod(Value,&ptr);

   switch(Index) {
      case 0: 
         if (coord<-90.0 || coord>90.0) {
            App_Log(App,ERROR,"Invalid latitude coordinate: %s\n",Value);
            return(0);
         }
         *Lat=coord;
         break;
      case 1:
         if (coord<0.0) coord+=360.0;
         if (coord<0.0 || coord>360.0) {
            App_Log(App,ERROR,"Invalid longitude coordinate: %s\n",Value);
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
