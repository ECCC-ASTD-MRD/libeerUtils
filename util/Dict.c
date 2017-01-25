/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Interrogation du dictionnaire des variables
 * Fichier      : Dict.c
 * Creation     : Mai 2014 - J.P. Gauthier
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
#include "RPN.h"
#include "Dict.h"

#define APP_NAME    "Dict"
#define APP_DESC    "CMC/RPN dictionary variable information."

char *THINT[]       = { "ATTENTION: Si le TYPVAR d'une variable est '!', utilisez -k avec l'ETIKET pour obtenir la bonne signification","WARNING: If a variable's TYPVAR is '!, use -k flag with the ETIKET to get the right definition" };

int Dict_CheckRPN(char **RPNFile);
int Dict_CheckCFG(char *CFGFile);

int main(int argc, char *argv[]) {

   TDict_Encoding coding=DICT_UTF8;
   int            ok=1,lng=0,xml=0,desc=DICT_SHORT,search=DICT_EXACT,st=DICT_ALL,ip1,ip2,ip3,d=0,code=EXIT_SUCCESS;
   char          *var,*type,*lang,*encoding,*origin,*etiket,*state,*dicfile[APP_LISTMAX],*rpnfile[APP_LISTMAX],*cfgfile,dicdef[APP_BUFMAX],*env;
   
   TApp_Arg appargs[]=
      { { APP_CHAR|APP_FLAG, &var,      1,             "n", "nomvar"      , "Search variable name ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_CHAR|APP_FLAG, &type,     1,             "t", "typvar"      , "Search variable type ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_INT32,         &ip1,      1,             "1",  "ip1"        , "Search IP1 ("APP_COLOR_GREEN"-1"APP_COLOR_RESET")" },
        { APP_INT32,         &ip3,      1,             "3",  "ip3"        , "Search IP3 ("APP_COLOR_GREEN"-1"APP_COLOR_RESET")" },
        { APP_CHAR,          &origin,   1,             "o", "origin"      , "Search originator ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_CHAR,          &etiket,   1,             "k", "etiket"      , "ETIKET modifier ("APP_COLOR_GREEN"\"\""APP_COLOR_RESET")" },
        { APP_CHAR,          &state,    1,             "s", "state"       , "Search state ("APP_COLOR_GREEN"all"APP_COLOR_RESET",obsolete,current,future,incomplete)" },
        { APP_FLAG,          &lng,      1,             "l", "long"        , "use long description" },
        { APP_FLAG,          &xml,      1,             "x", "xml"         , "use xml description" },
        { APP_FLAG,          &search,   1,             "g", "glob"        , "use glob search pattern" },
        { APP_CHAR,          &encoding, 1,             "e", "encoding"    , "encoding type (iso8859-1,"APP_COLOR_GREEN"utf8"APP_COLOR_RESET",ascii)" },
        { APP_CHAR,          dicfile,   APP_LISTMAX-1, "d", "dictionnary" , "dictionnary file(s) ("APP_COLOR_GREEN"$AFSISIO/datafiles/constants/ops.variable_dictionary.xml"APP_COLOR_RESET")" },
        { APP_CHAR,          rpnfile,   APP_LISTMAX-1, "f", "fstd"        , "Check RPN standard file(s) for unknow variables" },
        { APP_CHAR,          &cfgfile,  1,             "c", "cfg"         , "Check GEM configuration file for unknow variables" },
        { 0 } };
        
   memset(rpnfile,0x0,APP_LISTMAX*sizeof(rpnfile[0]));
   memset(dicfile,0x0,APP_LISTMAX*sizeof(dicfile[0]));
   var=type=lang=encoding=cfgfile=origin=etiket=state=NULL;
   ip1=ip2=ip3=-1;
   
   App_Init(APP_MASTER,APP_NAME,VERSION,APP_DESC,__TIMESTAMP__);

   if (!App_ParseArgs(appargs,argc,argv,APP_ARGSLANG)) {
      exit(EXIT_FAILURE);      
   }

   desc=xml?DICT_XML:lng?DICT_LONG:DICT_SHORT;
   
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
   Dict_SetSearch(search,st,origin,ip1,ip2,ip3,NULL);

   // Apply encoding type
   Dict_SetModifier(etiket);   

   if (encoding) {
      if (!strcmp(encoding,"iso8859-1")) {
         coding=DICT_ISO8859_1;
      } else if (!strcmp(encoding,"utf8")) {
         coding=DICT_UTF8;
      } else if (!strcmp(encoding,"ascii")) {
         coding=DICT_ASCII;
      } else {
         App_Log(ERROR,"Invalid encoding, must me iso8859-1, utf8 or ascii\n");
         exit(EXIT_FAILURE);
      }
   }
   
   // Launch the app   
   if (rpnfile[0] || cfgfile) {
      App_Start();
   }
   
   // Check for default dicfile
   if (!dicfile[0]) {
      // Check for AFSISIO 
      if (!(env=getenv("AFSISIO"))) {
         App_Log(ERROR,"Environment variable AFSISIO not defined, source the CMOI base domain.\n");
         exit(EXIT_FAILURE);   
      }

      snprintf(dicdef,APP_BUFMAX, "%s%s",env,"/datafiles/constants/ops.variable_dictionary.xml");
      if (!(ok=Dict_Parse(dicdef,coding))) {
         exit(EXIT_FAILURE);            
      }
      fprintf(stderr,"%s\n",Dict_Version());
   }
   
   // Check for optional dicfile(s)
   while(dicfile[d]) {   
      if (!(ok=Dict_Parse(dicfile[d],coding))) {
          exit(EXIT_FAILURE);            
      }
      fprintf(stderr,"%s\n",Dict_Version());
      d++;
   }        
          
   fprintf(stderr,"\n");
   
   if (rpnfile[0]) {
      ok=Dict_CheckRPN(rpnfile);
   } else if (cfgfile) {
      ok=Dict_CheckCFG(cfgfile);
   } else {
      if (var)  Dict_PrintVars(var,desc,App->Language); 
      if (type) Dict_PrintTypes(type,desc,App->Language);
      
      if (!etiket && search!=DICT_GLOB) fprintf(stderr,"\n%s* %s%s\n",APP_COLOR_MAGENTA,THINT[App->Language],APP_COLOR_RESET); 
   }

   if (rpnfile[0] || cfgfile) {
      code=App_End(ok?-1:EXIT_FAILURE);
   }
   App_Free();

   exit(code);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_CheckCFG>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Check an RPN file for unknow variables
 *
 * Parametres  :
 *  <CFGFile>  : CFG FST file
 *
 * Retour:
 *  <Err>      : Error code (0=Bad,1=Ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int Dict_CheckCFG(char *CFGFile){

   FILE     *fp;
   TDictVar *var;
   TList    *unknown,*known;
   char      buf[APP_BUFMAX],*idx,*values,*value,*valuesave;
   int       nb_unknown=0,nb_known=0;
   
   if (!(fp=fopen(CFGFile,"r"))) {
      App_Log(ERROR,"Unable to open config file: %s\n",CFGFile);
      return(0);
   }
   
   unknown=known=NULL;
   
   // Parse GEM cfg file
   while(fgets(buf,APP_BUFMAX,fp)) {
      
      // Process directives
#ifdef _AIX_
      if ((idx=strstr(buf,"sortie")) || (idx=strstr(buf,"SORTIE"))) {
#else
      if ((idx=strcasestr(buf,"sortie"))) {           
#endif
         // Locate var list within []
         values=strstr(idx,"(")+1;
         strrep(buf,'[',' ');
         strrep(buf,']','\0');
         
         // Parse all var separated by ,
         valuesave=NULL;
         while((value=strtok_r(values,",",&valuesave))) {
            strtrim(value,' ');
            
            App_Log(DEBUG,"Found variable: %s\n",value);
            
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
      while((known=TList_Find(known,NULL,""))) {
         var=(TDictVar*)(known->Data);        
         Dict_PrintVar(var,DICT_SHORT,App->Language);
         known=known->Next;
      }
      printf("\n");
   }
   
   if (unknown) {
      printf("Unknown variables (%i)\n-------------------------------------------------------------------------------------\n",nb_unknown);
      while((unknown=TList_Find(unknown,NULL,""))) {
         var=(TDictVar*)(unknown->Data);       
         printf("%-4s\n",var->Name);        
         unknown=unknown->Next;
      }
      printf("\n");   
   }
   
   if (nb_unknown) {
      App_Log(ERROR,"Found %i unknown variables\n",nb_unknown);
   } else {
      App_Log(INFO,"No unknown variables found\n");      
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
 *  <RPNFile>  : RPN FST file(s)
 *
 * Retour:
 *  <Err>      : Error code (0=Bad,1=Ok)
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int Dict_CheckRPN(char **RPNFile){

   TRPNHeader head;
   TDictVar  *var;
   TList     *unknown,*known;
   int        fid,ni,nj,nk,n,nb_unknown=0,nb_known=0,nidx,idxs[11];
   double     nhour;

   unknown=known=NULL;
   n=0;
   
   while(RPNFile[n]) {
       App_Log(DEBUG,"Looking into RPN file: %s\n",RPNFile[n]);
      
      if  ((fid=cs_fstouv(RPNFile[n],"STD+RND+R/O"))<0) {
         App_Log(ERROR,"Unable to open RPN file: %s\n",RPNFile[n]);
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
      while((known=TList_Find(known,NULL,""))) {
         var=(TDictVar*)(known->Data);        
         Dict_PrintVar(var,DICT_SHORT,App->Language);
         known=known->Next;
      }
      printf("\n");
   }
   
   if (unknown) {
      printf("Unknown variables (%i) with 10 first IP1s\n-------------------------------------------------------------------------------------\n",nb_unknown);
      while((unknown=TList_Find(unknown,NULL,""))) {
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
      App_Log(ERROR,"Found %i unknown variables\n",nb_unknown);
   } else {
      App_Log(INFO,"No unknown variables found\n");      
   }
      
   return(nb_unknown==0);
}