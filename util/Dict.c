/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Interrogation du dictionnaire des variables
 * Fichier      : Dict.c
 * Creation     : Avril 2006 - J.P. Gauthier
 *
 * Description  : Basé fortement sur le code d'Yves Chartier afin de convertir
 *                Le binaire un fonctions de librairies.
 *
 * Remarques    :
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
 *=========================================================
 */
#include "App.h"
#include "eerUtils.h"
#include "EZTile.h"
#include "Dict.h"

#define APP_NAME    "Dict"
#define APP_DESC    "CMC/RPN dictionary variable information."

int Dict_CheckRPN(TApp *App,char **RPNFile);
int Dict_CheckCFG(TApp *App,char *CFGFile);

int main(int argc, char *argv[]) {

   TApp      *app;
   int       ok=1,desc=DICT_SHORT,search=DICT_EXACT,st=DICT_ALL,ip1,ip2,ip3;
   char      *var,*type,*lang,*encoding,*origin,*state,*dicfile,*rpnfile[4096],*cfgfile,dicdef[4096];
   
   TApp_Arg appargs[]=
      { { APP_CHAR|APP_FLAG, (void**)&var,      "n", "nomvar"      , "Search variable name ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_CHAR|APP_FLAG, (void**)&type,     "t", "typvar"      , "Search variable type ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_INT32,         (void**)&ip1,      "1",  "ip1"        , "Search IP1 ("APP_COLOR_GREEN"-1"APP_COLOR_RESET")" },
        { APP_INT32,         (void**)&ip3,      "3",  "ip3"        , "Search IP3 ("APP_COLOR_GREEN"-1"APP_COLOR_RESET")" },
        { APP_CHAR,          (void**)&origin,   "o", "origin"      , "Search originator ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_CHAR,          (void**)&state,    "s", "state"       , "Search state ("APP_COLOR_GREEN"all"APP_COLOR_RESET",obsolete,current,future,incomplete)" },
        { APP_FLAG,          (void**)&desc,     "l", "long"        , "use long description" },
        { APP_FLAG,          (void**)&search,   "g", "glob"        , "use glob search pattern" },
        { APP_CHAR,          (void**)&lang,     "a", "language"    , "language ("APP_COLOR_GREEN"$CMCLNG,english"APP_COLOR_RESET",francais)" },
        { APP_CHAR,          (void**)&encoding, "e", "encoding"    , "encoding type (iso8859-1,utf8,"APP_COLOR_GREEN"ascii"APP_COLOR_RESET")" },
        { APP_CHAR,          (void**)&dicfile,  "d", "dictionnary" , "dictionnary file ("APP_COLOR_GREEN"$AFSISIO/datafiles/constants/stdf.variable_dictionary.xml"APP_COLOR_RESET")" },
        { APP_LIST,          (void**)&rpnfile,  "f", "fstd"        , "Check RPN standard file(s) for unknow variables" },
        { APP_CHAR,          (void**)&cfgfile,  "c", "cfg"         , "Check GEM configuration file(s) for unknow variables" },
        { 0 } };
        
   memset(rpnfile,0x0,4096);
   var=type=lang=encoding=dicfile=cfgfile=origin=state=NULL;
   ip1=ip2=ip3=-1;
   
   app=App_New(APP_NAME,VERSION,APP_DESC,__TIMESTAMP__);

   if (!App_ParseArgs(app,appargs,argc,argv,APP_NOARGSLOG)) {
      exit(EXIT_FAILURE);      
   }

   if (rpnfile[4095]) {
      App_Log(app,ERROR,"Too many RPN files, (max=4095)");
      exit(EXIT_FAILURE);     
   }

   // Check for default dicfile
   if (!dicfile) {
      sprintf(dicdef, "%s%s",getenv("AFSISIO"),"/datafiles/constants/ops.variable_dictionary.xml");
      dicfile=dicdef;
   }
      
   // Check the language in the environment
   if (lang) {
      app->Language=lang;
   }

   if (!var && !type && !cfgfile && !rpnfile[0]) {
      var=strdup("");
      type=strdup("");
      search=DICT_GLOB;
   } else {
      if (var==(void*)APP_FLAG || (var && strncmp(var,"all",3)==0)) {
         var=strdup("");
         search=DICT_GLOB;
      }      
      if (type==(void*)APP_FLAG || (type && strncmp(type,"all",3)==0)) {
         type=strdup("");
         search=DICT_GLOB;
      }      
   }

   if (state) {
      if (!strcasecmp(state,"obsolete"))   st=DICT_OBSOLETE;
      if (!strcasecmp(state,"current"))    st=DICT_CURRENT;
      if (!strcasecmp(state,"future"))     st=DICT_FUTURE;
      if (!strcasecmp(state,"incomplete")) st=DICT_INCOMPLETE;
   }

   // Apply search method
   Dict_SetSearch(search,st,origin,ip1,ip2,ip3);

   // Apply encoding type
   Dict_SetEncoding(DICT_ASCII);   

   if (encoding) {
      if (!strcmp(encoding,"iso8859-1")) {
         Dict_SetEncoding(DICT_ISO8859_1);
      } else if (!strcmp(encoding,"utf8")) {
         Dict_SetEncoding(DICT_UTF8);
      } else if (!strcmp(encoding,"ascii")) {
      } else {
         App_Log(app,ERROR,"Invalid encoding, must me iso8859-1, utf8 or ascii\n");
         exit(EXIT_FAILURE);
      }
   }
   
   // Launch the app   
   if (rpnfile[0] || cfgfile) {
      App_Start(app);
   }
   
   if (!(ok=Dict_Parse(dicfile))) {
     App_Log(app,ERROR,"Invalid file\n");
   } else {
      fprintf(stderr,"%s\n\n",Dict_Version());
      
      if (rpnfile[0]) {
         ok=Dict_CheckRPN(app,rpnfile);
      } else if (cfgfile) {
         ok=Dict_CheckCFG(app,cfgfile);
      } else {
         if (var)  Dict_PrintVars(var,desc,app->Language); 
         if (type) Dict_PrintTypes(type,desc,app->Language);
      }
   }

   if (rpnfile[0] || cfgfile) {
      App_End(app,!ok);
   }
   App_Free(app);

   if (!ok) {
      exit(EXIT_FAILURE);
   } else {
      exit(EXIT_SUCCESS);
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_CheckCFG>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Check an RPN file for unknow variables
 *
 * Parametres  :
 *  <App>      : Application parameters
 *  <CFGFile>  : CFG FST file
 *
 * Retour:
 *  <Err>      : Error code (0=Bad,1=Ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int Dict_CheckCFG(TApp *App,char *CFGFile){

   FILE     *fp;
   TDictVar *var;
   TList    *unknown,*known;
   char      buf[APP_BUFMAX],*idx,*values,*value,*valuesave;
   int       d,nb_unknown=0,nb_known=0;
   
   if (!(fp=fopen(CFGFile,"r"))) {
      App_Log(App,ERROR,"Unable to open config file: %s\n",CFGFile);
      return(0);
   }
   
   unknown=known=NULL;
   
   // Parse GEM cfg file
   while(fgets(buf,APP_BUFMAX,fp)) {
      
      // Process directives
         if (idx=strcasestr(buf,"sortie")) {
            
         // Locate var list within []
         values=strcasestr(idx,"(")+1;
         strrep(buf,'[',' ');
         strrep(buf,']','\0');
         
         // Parse all var separated by ,
         valuesave=NULL;
         while(value=strtok_r(values,",",&valuesave)) {
            strtrim(value,' ');
            
            App_Log(App,DEBUG,"Found variable: %s\n",value);
            
            // Si la variable n'existe pas
            if (!(var=Dict_GetVar(value))) {
               
               // Si pas encore dans la liste des inconnus
               if (!TList_Find(unknown,Dict_CheckVar,value)) {
                  nb_unknown++;
                  var=(TDictVar*)calloc(1,sizeof(TDictVar));
                  strncpy(var->Name,value,8);
                  unknown=TList_AddSorted(unknown,Dict_SortVar,var);
               }
            } else {
               if (!TList_Find(known,Dict_CheckVar,var->Name)) {
                  nb_known++;
                  known=TList_AddSorted(known,Dict_SortVar,var);
               }
            }
            values=NULL;           
         }
      }
   }  
      
   if (known) {
      printf("Known variables (%i)\n-------------------------------------------------------------------------------------\n",nb_known);
      while(known=TList_Find(known,NULL,"")) {
         var=(TDictVar*)(known->Data);        
         Dict_PrintVar(var,DICT_SHORT,App->Language);
         known=known->Next;
      }
      printf("\n");
   }
   
   if (unknown) {
      printf("Unknown variables (%i)\n-------------------------------------------------------------------------------------\n",nb_unknown);
      while(unknown=TList_Find(unknown,NULL,"")) {
         var=(TDictVar*)(unknown->Data);       
         printf("%-4s\n",var->Name);        
         unknown=unknown->Next;
      }
      printf("\n");   
   }
   
   if (nb_unknown) {
      App_Log(App,ERROR,"Found %i unknown variables\n",nb_unknown);
   } else {
      App_Log(App,INFO,"No unknown variables found\n");      
   }
    
   return(nb_unknown==0);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_CheckRPN>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Check an RPN file for unknow variables
 *
 * Parametres  :
 *  <App>      : Application parameters
 *  <RPNFile>  : RPN FST file(s)
 *
 * Retour:
 *  <Err>      : Error code (0=Bad,1=Ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int Dict_CheckRPN(TApp *App,char **RPNFile){

   TRPNHeader head;
   TDictVar  *var;
   TList     *unknown,*known;
   int        fid,ni,nj,nk,n,nb_unknown=0,nb_known=0,nidx,idxs[11];
   double     nhour;

   unknown=known=NULL;
   n=0;
   
   while(RPNFile[n]) {
       App_Log(App,DEBUG,"Looking into RPN file: %s\n",RPNFile[n]);
      
      if  ((fid=cs_fstouv(RPNFile[n],"STD+RND+R/O"))<0) {
         App_Log(App,ERROR,"Unable to open RPN file: %s\n",RPNFile[n]);
         return(0);
      }
           
      head.KEY=c_fstinf(fid,&ni,&nj,&nk,-1,"",-1,-1,-1,"","");

      // Boucle sur tout les enregistrements
      while (head.KEY>=0) {

         strcpy(head.NOMVAR,"    ");
         strcpy(head.TYPVAR,"  ");
         strcpy(head.ETIKET,"            ");
         c_fstprm(head.KEY,&head.DATEO,&head.DEET,&head.NPAS,&ni,&nj,&nk,&head.NBITS,&head.DATYP,&head.IP1,&head.IP2,&head.IP3,head.TYPVAR,head.NOMVAR,
                  head.ETIKET,head.GRTYP,&head.IG1,&head.IG2,&head.IG3,&head.IG4,&head.SWA,&head.LNG,&head.DLTF,&head.UBC,&head.EX1,&head.EX2,&head.EX3);

         // Calculer la date de validite
         nhour=((double)head.NPAS*head.DEET)/3600.0;
         if (head.DATEO==0) {
            head.DATEV=0;
         } else {
            f77name(incdatr)(&head.DATEV,&head.DATEO,&nhour);
         }
         if (head.DATEV==101010101) head.DATEV=-1;

         strtrim(head.NOMVAR,' ');
         strtrim(head.TYPVAR,' ');
         strtrim(head.ETIKET,' ');

         // Si la variable n'existe pas
         if (!(var=Dict_GetVar(head.NOMVAR))) {
            // Si pas encore dans la liste des inconnus
            if (!TList_Find(unknown,Dict_CheckVar,head.NOMVAR)) {
               nb_unknown++;
               var=(TDictVar*)calloc(1,sizeof(TDictVar));
               
               // Keep the var and its date and file for later search
               strncpy(var->Name,head.NOMVAR,4);
               var->NCodes=n;
               var->Nature=head.DATEV;
               unknown=TList_AddSorted(unknown,Dict_SortVar,var);
            }
         } else {
            if (!TList_Find(known,Dict_CheckVar,var->Name)) {
               nb_known++;
               known=TList_AddSorted(known,Dict_SortVar,var);
            }
         }
         head.KEY=c_fstsui(fid,&ni,&nj,&nk);
      }
      cs_fstfrm(fid);
      n++;
   }
   
   if (known) {
      printf("Known variables (%i)\n-------------------------------------------------------------------------------------\n",nb_known);
      while(known=TList_Find(known,NULL,"")) {
         var=(TDictVar*)(known->Data);        
         Dict_PrintVar(var,DICT_SHORT,App->Language);
         known=known->Next;
      }
      printf("\n");
   }
   
   if (unknown) {
      printf("Unknown variables (%i) with 10 first IP1s\n-------------------------------------------------------------------------------------\n",nb_unknown);
      while(unknown=TList_Find(unknown,NULL,"")) {
         var=(TDictVar*)(unknown->Data);
         
         fid=cs_fstouv(RPNFile[var->NCodes],"STD+RND+R/O");
         
         // Recuperer les indexes de tout les niveaux
         cs_fstinl(fid,&ni,&nj,&nk,var->Nature,"",-1,-1,-1,"",var->Name,idxs,&nidx,11);

         // Afficiher la variable et les 10 premiers IP1
         printf("%-4s  ",var->Name);
         for(n=0;n<(nidx>10?10:nidx);n++) {
            c_fstprm(idxs[n],&head.DATEO,&head.DEET,&head.NPAS,&ni,&nj,&nk,&head.NBITS,&head.DATYP,&head.IP1,&head.IP2,&head.IP3,head.TYPVAR,head.NOMVAR,
                    head.ETIKET,head.GRTYP,&head.IG1,&head.IG2,&head.IG3,&head.IG4,&head.SWA,&head.LNG,&head.DLTF,&head.UBC,&head.EX1,&head.EX2,&head.EX3);
            printf ("%8i ",head.IP1);
         }
         
         if (nidx>9) printf ("...");
         printf("\n");
      
         unknown=unknown->Next;
         cs_fstfrm(fid);
      }
      printf("\n");
   }
   
   if (nb_unknown) {
      App_Log(App,ERROR,"Found %i unknown variables\n",nb_unknown);
   } else {
      App_Log(App,INFO,"No unknown variables found\n");      
   }
      
   return(nb_unknown==0);
}