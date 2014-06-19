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

#define APP_VERSION "2.0"
#define APP_NAME    "Dict"
#define APP_DESC    "CMC/RPN dictionary variable information."

int Dict_CheckRPN(TApp *App,char *RPNFile);
int Dict_CheckCFG(TApp *App,char *CFGFile);

int main(int argc, char *argv[]) {

   TApp      *app;
   int       ok=1,desc=DICT_SHORT,search=DICT_EXACT,st=DICT_ALL,ip1,ip2,ip3;
   char      *var,*type,*lang,*encoding,*origin,*state,*dicfile,*rpnfile,*cfgfile,dicdef[4096];
   
   TApp_Arg appargs[]=
      { { APP_CHAR,  (void**)&var,      "n", "nomvar"      , "Search variable name ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_CHAR,  (void**)&type,     "t", "typvar"      , "Search variable type ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_INT32, (void**)&ip1,      "1",  "ip1"        , "Search IP1 ("APP_COLOR_GREEN"-1"APP_COLOR_RESET")" },
        { APP_INT32, (void**)&ip3,      "3",  "ip3"        , "Search IP3 ("APP_COLOR_GREEN"-1"APP_COLOR_RESET")" },
        { APP_CHAR,  (void**)&origin,   "o", "origin"      , "Search originator ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_CHAR,  (void**)&state,    "s", "state"       , "Search state ("APP_COLOR_GREEN"all"APP_COLOR_RESET",obsolete,current,future,incomplete)" },
        { APP_FLAG,  (void**)&desc,     "l", "long"        , "use long description" },
        { APP_FLAG,  (void**)&search,   "g", "glob"        , "use glob search pattern" },
        { APP_CHAR,  (void**)&lang,     "a", "language"    , "language ("APP_COLOR_GREEN"$CMCLNG,english"APP_COLOR_RESET",francais)" },
        { APP_CHAR,  (void**)&encoding, "e", "encoding"    , "encoding type (iso8859-1,utf8,"APP_COLOR_GREEN"ascii"APP_COLOR_RESET")" },
        { APP_CHAR,  (void**)&dicfile,  "d", "dictionnary" , "dictionnary file ("APP_COLOR_GREEN"$AFSISIO/datafiles/constants/stdf.variable_dictionary.xml"APP_COLOR_RESET")" },
        { APP_CHAR,  (void**)&rpnfile,  "f", "fstd"        , "Check an RPN standard file for unknow variables" },
        { APP_CHAR,  (void**)&cfgfile,  "c", "cfg"         , "Check a GEM configuration file for unknow variables" },
        { 0 } };
        
   var=type=lang=encoding=dicfile=origin=rpnfile=state=NULL;
   ip1=ip2=ip3=-1;
   
   app=App_New(APP_NAME,APP_VERSION,APP_DESC,__TIMESTAMP__);

   if (!App_ParseArgs(app,appargs,argc,argv,APP_NOARGSLOG)) {
      exit(EXIT_FAILURE);      
   }

   // Check for default dicfile
   if (!dicfile) {
//      sprintf(dicdef, "%s%s",getenv("AFSISIO"),"/datafiles/constants/stdf.variable_dictionary.xml");
      sprintf(dicdef, "/home/afsr/005/Projects/libeerUtils/data/stdf.variable_dictionary.xml");
      dicfile=dicdef;
   }
   
   // Check the language in the environment
   if (lang) {
      app->Language=lang;
   }
  
   if (!var && !type && !rpnfile && !cfgfile) {
      var=strdup("");
      type=strdup("");
      search=DICT_GLOB;
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
//   App_Start(app);
   
   if (!(ok=Dict_Parse(dicfile))) {
     App_Log(app,ERROR,"Invalid file\n");
   } else {
      fprintf(stderr,"%s\n\n",Dict_Version());
      
      if (rpnfile) {
         ok=Dict_CheckRPN(app,rpnfile);
      } else if (cfgfile) {
         ok=Dict_CheckCFG(app,cfgfile);
      } else {
         if (var)  Dict_PrintVars(var,desc,app->Language); 
         if (type) Dict_PrintTypes(type,desc,app->Language);
      }
   }

//   App_End(app,ok==1);
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
   char     *directives[]={ "sortie(","sortie_p(",NULL };
   int       d,nb_unknown=0,nb_known=0;
   
   if (!(fp=fopen(CFGFile,"r"))) {
      App_Log(App,ERROR,"Unable to open config file: %s\n",CFGFile);
      return(0);
   }
   
   unknown=known=NULL;
   
   // Parse GEM cfg file
   while(fgets(buf,APP_BUFMAX,fp)) {
      
      // Process directives
      d=0;
      while(directives[d]) {
         if (idx=strcasestr(buf,directives[d])) {
            
            // Locate var list within []
            values=idx+strlen(directives[d]);
            strrep(buf,'[',' ');
            strrep(buf,']','\0');
            
            // Parse al var separated by ,
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
         d++;
      }
   }  
      
   if (known) {
      printf("Known variables (%i)\n-------------------------------------------------------------------------------------\n",nb_known);
      while(known=TList_Find(known,NULL,"")) {
         var=(TDictVar*)(known->Data);        
         Dict_PrintVar(var,DICT_SHORT,App->Language);
         known=known->Next;
      }
   }
   
   if (unknown) {
      printf("\nUnknown variables (%i)\n-------------------------------------------------------------------------------------\n",nb_unknown);
      while(unknown=TList_Find(unknown,NULL,"")) {
         var=(TDictVar*)(unknown->Data);       
         printf("%-4s\n",var->Name);        
         unknown=unknown->Next;
      }
   }
   printf("\n");
   
   App_Log(App,WARNING,"Found %i unknown variables\n",nb_unknown);
    
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_CheckRPN>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Check an RPN file for unknow variables
 *
 * Parametres  :
 *  <App>      : Application parameters
 *  <RPNFile>  : RPN FST file
 *
 * Retour:
 *  <Err>      : Error code (0=Bad,1=Ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int Dict_CheckRPN(TApp *App,char *RPNFile){

   TRPNHeader head;
   TDictVar  *var;
   TList     *unknown,*known;
   int        fid,ni,nj,nk,n,nb_unknown=0,nb_known=0,nidx,idxs[11];
   double     nhour;
   
   if  ((fid=cs_fstouv(RPNFile,"STD+RND+R/O"))<0) {
      App_Log(App,ERROR,"Unable to open RPN file: %s\n",RPNFile);
      return(0);
   }
   
   unknown=known=NULL;
   
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
            strncpy(var->Name,head.NOMVAR,4);
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
   
   if (known) {
      printf("Known variables (%i)\n-------------------------------------------------------------------------------------\n",nb_known);
      while(known=TList_Find(known,NULL,"")) {
         var=(TDictVar*)(known->Data);        
         Dict_PrintVar(var,DICT_SHORT,App->Language);
         known=known->Next;
      }
   }
   
   if (unknown) {
      printf("\nUnknown variables (%i) with 10 first IP1s\n-------------------------------------------------------------------------------------\n",nb_unknown);
      while(unknown=TList_Find(unknown,NULL,"")) {
         var=(TDictVar*)(unknown->Data);
         
         // Recuperer les indexes de tout les niveaux
         cs_fstinl(fid,&ni,&nj,&nk,head.DATEV,"",-1,-1,-1,"",var->Name,idxs,&nidx,11);

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
      }
   }
   printf("\n");
   
   App_Log(App,WARNING,"Found %i unknown variables\n",nb_unknown);
   
   cs_fstfrm(fid);
   
   return(1);
}