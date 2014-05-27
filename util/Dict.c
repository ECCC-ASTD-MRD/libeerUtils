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

int main(int argc, char *argv[]) {

   TApp      *app;
   int       ok=1,desc=DICT_SHORT,search=DICT_EXACT,st=DICT_ALL,ip1,ip2,ip3;
   char      *var,*type,*lang,*encoding,*origin,*state,*dicfile,*rpnfile,dicdef[4096];
   
   TApp_Arg appargs[]=
      { { APP_CHAR,  (void**)&var,      "n", "nomvar"      , "Search variable name ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_CHAR,  (void**)&type,     "t", "typvar"      , "Search variable type ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_INT32, (void**)&ip1,      "1",  "ip1"        , "Search IP1 ("APP_COLOR_GREEN"-1"APP_COLOR_RESET")" },
        { APP_INT32, (void**)&ip3,      "3",  "ip3"        , "Search IP3 ("APP_COLOR_GREEN"-1"APP_COLOR_RESET")" },
        { APP_CHAR,  (void**)&origin,   "o", "origin"      , "Search originator ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_CHAR,  (void**)&state,    "s", "state"       , "Search state ("APP_COLOR_GREEN"all"APP_COLOR_RESET",obsolete,current,future)" },
        { APP_FLAG,  (void**)&desc,     "l", "long"        , "use long description" },
        { APP_FLAG,  (void**)&search,   "g", "glob"        , "use glob search pattern" },
        { APP_CHAR,  (void**)&lang,     "a", "language"    , "language ("APP_COLOR_GREEN"english"APP_COLOR_RESET",francais)" },
        { APP_CHAR,  (void**)&encoding, "e", "encoding"    , "encoding type ("APP_COLOR_GREEN"iso8859-1"APP_COLOR_RESET",utf8,ascii)" },
        { APP_CHAR,  (void**)&dicfile,  "d", "dictionnary" , "dictionnary file ("APP_COLOR_GREEN"$AFSISIO/..."APP_COLOR_RESET")" },
        { APP_CHAR,  (void**)&rpnfile,  "f", "fstd"        , "Check an RPN standard file for unknow variables" },
        { 0 } };
        
   var=type=lang=encoding=dicfile=origin=rpnfile=state=NULL;
   ip1=ip2=ip3=-1;
   
   app=App_New(APP_NAME,APP_VERSION,APP_DESC,__TIMESTAMP__);

   if (!App_ParseArgs(app,appargs,argc,argv,APP_NOARGSLOG)) {
      exit(EXIT_FAILURE);      
   }

   // Check for default dicfile
   if (!dicfile) {
      sprintf(dicdef, "%s%s",getenv("AFSISIO"),"/datafiles/constants/stdf.variable_dictionary.xml");
      dicfile=dicdef;
   }
   
   // Check the language in the environment
   if (!lang && !(lang=getenv("CMCLNG"))) {
      lang=strdup("english");
   }
  
   if (!var && !type) {
      var=strdup("");
      type=strdup("");
      search=DICT_GLOB;
   }

   if (state) {
      if (!strcasecmp(state,"obsolete")) st=DICT_OBSOLETE;
      if (!strcasecmp(state,"current"))  st=DICT_CURRENT;
      if (!strcasecmp(state,"future"))   st=DICT_FUTURE;
   }

   // Apply search method
   Dict_SetSearch(search,st,origin,ip1,ip2,ip3);

   /*
   if (encoding) {
      if (!strcmp(encoding,"iso8859-1")) {
         Dict_SetEncoding(XML_CHAR_ENCODING_8859_1);
      } else if (!strcmp(encoding,"utf8")) {
         Dict_SetEncoding(XML_CHAR_ENCODING_UTF8);
      } else {
         Dict_SetEncoding(XML_CHAR_ENCODING_ASCII);   
      }
   }
*/
   // Launch the app
//   App_Start(app);
   
   if (!(ok=Dict_Parse(dicfile))) {
     App_Log(app,ERROR,"Invalid file\n");
   } else {
      if (rpnfile) {
         ok=Dict_CheckRPN(app,rpnfile);
      } else {
         if (var)  Dict_PrintVars(var,desc,lang); 
         if (type) Dict_PrintTypes(type,desc,lang);
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

int Dict_CheckRPN(TApp *App,char *RPNFile){

   TRPNHeader head;
   TDictVar  *var,*metvar;
   TList     *vars,*list;
   int        fid,ni,nj,nk,n,nb=0,nidx,idxs[11];
   double     nhour;
   
   if  ((fid=cs_fstouv(RPNFile,"STD+RND+R/O"))<0) {
      App_Log(App,ERROR,"Unable to open RPN file: %s\n",RPNFile);
      return(0);
   }
   
   vars=NULL;
   
   head.KEY=c_fstinf(fid,&ni,&nj,&nk,-1,"",-1,-1,-1,"","");

   /* Boucel sur tout les enregistrements */
   while (head.KEY>=0) {

      strcpy(head.NOMVAR,"    ");
      strcpy(head.TYPVAR,"  ");
      strcpy(head.ETIKET,"            ");
      c_fstprm(head.KEY,&head.DATEO,&head.DEET,&head.NPAS,&ni,&nj,&nk,&head.NBITS,&head.DATYP,&head.IP1,&head.IP2,&head.IP3,head.TYPVAR,head.NOMVAR,
               head.ETIKET,head.GRTYP,&head.IG1,&head.IG2,&head.IG3,&head.IG4,&head.SWA,&head.LNG,&head.DLTF,&head.UBC,&head.EX1,&head.EX2,&head.EX3);

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

      if (!(var=Dict_GetVar(head.NOMVAR))) {
//         printf("%-4s \n",head.NOMVAR);
         
         if (!TList_Find(vars,Dict_CheckVar,head.NOMVAR)) {
            nb++;
            metvar=(TDictVar*)calloc(1,sizeof(TDictVar));
            strncpy(metvar->Name,head.NOMVAR,4);
            vars=TList_AddSorted(vars,Dict_SortVar,metvar);
         }
      }
      head.KEY=c_fstsui(fid,&ni,&nj,&nk);
   }
   
   if (vars) {
      printf("Var   IP1s (10 first)\n----  ---------------------------------------------------------------------\n",nb);
      list=vars;
      while(list=TList_Find(list,Dict_CheckVar,"")) {
         var=(TDictVar*)(list->Data);
         
         /*Recuperer les indexes de tout les niveaux*/
         cs_fstinl(fid,&ni,&nj,&nk,head.DATEV,"",-1,-1,-1,"",var->Name,idxs,&nidx,11);

         
         printf("%-4s  ",var->Name);
         for(n=0;n<(nidx>10?10:nidx);n++) {
            c_fstprm(idxs[n],&head.DATEO,&head.DEET,&head.NPAS,&ni,&nj,&nk,&head.NBITS,&head.DATYP,&head.IP1,&head.IP2,&head.IP3,head.TYPVAR,head.NOMVAR,
                    head.ETIKET,head.GRTYP,&head.IG1,&head.IG2,&head.IG3,&head.IG4,&head.SWA,&head.LNG,&head.DLTF,&head.UBC,&head.EX1,&head.EX2,&head.EX3);
            printf ("%8i ",head.IP1);
         }
         
         if (nidx>9) printf ("...");
         printf("\n");
      
         list=list->Next;
      }
   }
   printf("\nFound %i unknown variables\n",nb);
   
   cs_fstfrm(fid);
   
   return(1);
}