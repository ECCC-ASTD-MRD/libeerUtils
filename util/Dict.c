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
#include <sys/stat.h>

#include "App.h"
#include "eerUtils.h"
#include "RPN.h"
#include "Dict.h"
#include "ZRef.h"

#define APP_NAME    "Dict"
#define APP_DESC    "CMC/RPN dictionary variable information."

char *THINT[]       = { "ATTENTION: Si le TYPVAR d'une variable est '!', utilisez -k avec l'ETIKET pour obtenir la bonne signification","WARNING: If a variable's TYPVAR is '!, use -k flag with the ETIKET to get the right definition" };

int Dict_CheckRPN(char **RPNFile,int CheckOPS);
int Dict_CheckCFG(char *CFGFile);

int main(int argc, char *argv[]) {

   TDict_Encoding coding=DICT_UTF8;
   int            ok=1,lng=0,xml=0,ops=0,desc=DICT_SHORT,search=DICT_EXACT,st=DICT_ALL,ip1,ip2,ip3,d=0,code=EXIT_SUCCESS;
   char          *var,*type,*lang,*encoding,*origin,*etiket,*state,*dicfile[APP_LISTMAX],*rpnfile[APP_LISTMAX],*cfgfile,dicdef[APP_BUFMAX],*env;

   TApp_Arg appargs[]=
      { { APP_CHAR|APP_FLAG, &var,      1,             "n", "nomvar"      , "Search variable name ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_CHAR|APP_FLAG, &type,     1,             "t", "typvar"      , "Search variable type ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_INT32,         &ip1,      1,             "1", "ip1"         , "Search IP1 ("APP_COLOR_GREEN"-1"APP_COLOR_RESET")" },
        { APP_INT32,         &ip3,      1,             "3", "ip3"         , "Search IP3 ("APP_COLOR_GREEN"-1"APP_COLOR_RESET")" },
        { APP_CHAR,          &origin,   1,             "o", "origin"      , "Search originator ("APP_COLOR_GREEN"all"APP_COLOR_RESET")" },
        { APP_CHAR,          &etiket,   1,             "k", "etiket"      , "ETIKET modifier ("APP_COLOR_GREEN"\"\""APP_COLOR_RESET")" },
        { APP_CHAR,          &state,    1,             "s", "state"       , "Search state ("APP_COLOR_GREEN"all"APP_COLOR_RESET",obsolete,current,future,incomplete,deprecated)" },
        { APP_FLAG,          &lng,      1,             "l", "long"        , "Output long description" },
        { APP_FLAG,          &xml,      1,             "x", "xml"         , "Output in XML format" },
        { APP_FLAG,          &search,   1,             "g", "glob"        , "Use glob search pattern" },
        { APP_CHAR,          &encoding, 1,             "e", "encoding"    , "Encoding type (iso8859-1,"APP_COLOR_GREEN"utf8"APP_COLOR_RESET",ascii)" },
        { APP_CHAR,          dicfile,   APP_LISTMAX-1, "d", "dictionary"  , "Input dictionary file(s) ("APP_COLOR_GREEN"$CMCCONST/opdict/ops.variable_dictionary.xml"APP_COLOR_RESET")" },
        { APP_CHAR,          rpnfile,   APP_LISTMAX-1, "f", "fstd"        , "Check RPN standard file(s) for unknown variables" },
        { APP_CHAR,          &cfgfile,  1,             "c", "cfg"         , "Check GEM configuration file for unknown variables" },
        { APP_FLAG,          &ops    ,  1,             "" , "ops"         , "Force check of operational standards when used with RPN file check" },
        { 0 } };

   memset(rpnfile,0x0,APP_LISTMAX*sizeof(rpnfile[0]));
   memset(dicfile,0x0,APP_LISTMAX*sizeof(dicfile[0]));
   var=type=lang=encoding=cfgfile=origin=etiket=state=NULL;
   ip1=ip2=ip3=-1;

   App_Init(APP_MASTER,APP_NAME,VERSION,APP_DESC,BUILD_TIMESTAMP);

   if (!App_ParseArgs(appargs,argc,argv,APP_ARGSLANG)) {
      exit(EXIT_FAILURE);
   }

   desc=xml?DICT_XML:lng?DICT_LONG:DICT_SHORT;

   if (!var && !type && !cfgfile && !rpnfile[0] && ip1<0 && ip3<0 && !state && !origin) {
      var=strdup("");
      type=strdup("");
      search=DICT_GLOB;
   } else {
      if (var==(void*)APP_FLAG || (var && strncmp(var,"all",3)==0) || !var&&(ip1>=0||ip3>=0||state||origin)) {
         var=strdup("");
         search=DICT_GLOB;
      }
      if (type==(void*)APP_FLAG || (type && strncmp(type,"all",3)==0)) {
         type=strdup("");
         search=DICT_GLOB;
      }
   }

   if (state) {
      if (!strncasecmp(state,"obsolete",3))          { st=DICT_OBSOLETE;
      } else if (!strncasecmp(state,"current",3))    { st=DICT_CURRENT;
      } else if (!strncasecmp(state,"future",3))     { st=DICT_FUTURE;
      } else if (!strncasecmp(state,"incomplete",3)) { st=DICT_INCOMPLETE;
      } else if (!strncasecmp(state,"deprecated",3)) { st=DICT_DEPRECATED;
      } else {
         App_Log(ERROR,"Invalid state (%s), must be current, future, incomplete, obsolete or deprecated\n",state);
         exit(EXIT_FAILURE);
      }
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
         App_Log(ERROR,"Invalid encoding (%s), must me iso8859-1, utf8 or ascii\n",encoding);
         exit(EXIT_FAILURE);
      }
   }

   // Launch the app
   if (rpnfile[0] || cfgfile) {
      App_Start();
   }

   // Check for default dicfile
   if (!dicfile[0]) {
      // Check for CMCCONST
      if (!(env=getenv("CMCCONST"))) {
         App_Log(ERROR,"Environment variable CMCCONST not defined, source the CMOI base domain.\n");
         exit(EXIT_FAILURE);
      }

      snprintf(dicdef,APP_BUFMAX, "%s%s",env,"/opdict/ops.variable_dictionary.xml");
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
      ok=Dict_CheckRPN(rpnfile,ops);
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
   int       nb_unknown=0,nb_known=0,cont=0;

   if (!(fp=fopen(CFGFile,"r"))) {
      App_Log(ERROR,"Unable to open config file: %s\n",CFGFile);
      return(0);
   }

   unknown=known=NULL;

   // Parse GEM cfg file
   while(fgets(buf,APP_BUFMAX,fp)) {

      // Process directives
      if (cont || (idx=strcasestr(buf,"sortie"))) {
         // Locate var list within []
         values=cont?buf:strchr(idx,'[')+1;
         if( (value=strchr(values,']')) ) {
             *value = '\0';
             cont = 0;
         } else {
             cont = 1;
         }

         // Parse all var separated by ,
         valuesave=NULL;
         while((value=strtok_r(values,", \n",&valuesave))) {

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

typedef struct {
   int IP1,IP2,IP3;
   int Type,NRef;
   char GRTYP;
} TRPNRef;

#define DICT_MAXREF 256
#define DICT_MAXFLD 256000
#define DICT_MAXSIZE 8*1073741824

int Dict_CheckRPN(char **RPNFile,int CheckOPS) {

   TRPNHeader headtable[DICT_MAXFLD],*head;
   TDictVar  *var;
   TList     *unknown,*known;
   int        fid,key,ni,nj,nk,n,nb_unknown=0,nb_known=0,nidx,idxs[DICT_MAXREF],dateo=0;
   double     nhour;
   char       nvar[5],*etiket;

   TRPNRef    href[DICT_MAXREF];
   int        zref[DICT_MAXREF];
   int        hrefnb,zrefnb,headnb,h,z,nh,ztyp,zver;
   int        p0,pt,hy,tic,tac,toc,tuc,nhtic,nhtac,nhtoc,nhtuc,err=0;

   struct stat st;

   unknown=known=NULL;
   n=0;

   while(RPNFile[n]) {
      App_Log(INFO,"Checking RPN file: %s\n",RPNFile[n]);

      // Check file size (max 8Gb)
      stat(RPNFile[n],&st);
      if (st.st_size>((off_t)DICT_MAXSIZE)) {
         App_Log(ERROR,"File is greater the 8Gb limit: %s\n",RPNFile[n]);
      }

      if  ((fid=cs_fstouv(RPNFile[n],"STD+RND+R/O"))<0) {
         App_Log(ERROR,"Unable to open RPN file: %s\n",RPNFile[n]);
         return(0);
      }

      // Reset counter for this file
      p0=pt=hy=0,tic=tac=toc=tuc=zver=0,ztyp=-1;
      hrefnb=zrefnb=headnb=0;

      head=&headtable[headnb];
      head->KEY=c_fstinf(fid,&ni,&nj,&nk,-1,"",-1,-1,-1,"","");

      // Boucle sur tout les enregistrements
      while (head->KEY>=0) {

         strcpy(head->NOMVAR,"    ");
         strcpy(head->TYPVAR,"  ");
         strcpy(head->ETIKET,"            ");
         c_fstprm(head->KEY,&head->DATEO,&head->DEET,&head->NPAS,&ni,&nj,&nk,&head->NBITS,&head->DATYP,&head->IP1,&head->IP2,&head->IP3,head->TYPVAR,head->NOMVAR,
                  head->ETIKET,head->GRTYP,&head->IG1,&head->IG2,&head->IG3,&head->IG4,&head->SWA,&head->LNG,&head->DLTF,&head->UBC,&head->EX1,&head->EX2,&head->EX3);

         // Calculer la date de validite
         nhour=((double)head->NPAS*head->DEET)/3600.0;
         if (head->DATEO==0) {
            head->DATEV=0;
         } else {
            f77name(incdatr)(&head->DATEV,&head->DATEO,&nhour);
         }
         if (head->DATEV==101010101) head->DATEV=-1;

         strcpy(nvar,head->NOMVAR);
         strtrim(nvar,' ');

         // Si la variable n'existe pas
         if (!(var=Dict_GetVar(nvar))) {
            // Si pas encore dans la liste des inconnus
            if (!TList_Find(unknown,Dict_CheckVar,nvar)) {
               nb_unknown++;
               var=(TDictVar*)calloc(1,sizeof(TDictVar));

               // Keep the var and its date and file for later search
               strncpy(var->Name,nvar,4);
               var->NCodes=n;
               var->Nature=head->DATEV;
               unknown=TList_AddSorted(unknown,Dict_SortVar,var);
            }
         } else {
            if (!TList_Find(known,Dict_CheckVar,var->Name)) {
               nb_known++;
               known=TList_AddSorted(known,Dict_SortVar,var);
            }
         }

         if (CheckOPS) {

            if (!RPN_IsDesc(head->NOMVAR)) {

               // Keep regular field ETIKET and DATEO for descriptor field checks
               dateo=!dateo?head->DATEO:dateo;
               etiket=head->ETIKET;

               // Check same DATEO
               if (dateo!=head->DATEO) {
                  App_Log(ERROR,"Found different DATEO (%8i): NOMVAR=%4s TYPVAR=%4s IP1=%8i IP2=%8i IP3=%8i DATEO=%8i\n",dateo,head->NOMVAR,head->TYPVAR,head->IP1,head->IP2,head->IP3,head->DATEO);
                  err++;
               }

               // Check for compression
               if ((head->DATYP==2 || head->DATYP==4 || head->DATYP==5)) {
                  App_Log(ERROR,"Record %i (%s) is not compressed\n",head->KEY,head->NOMVAR);
                  err++;
               }

               // Check if it is duplicated
               for(h=0;h<headnb;h++) {
                  if (!strcmp(head->NOMVAR,headtable[h].NOMVAR) && !strcmp(head->TYPVAR,headtable[h].TYPVAR) && !strcmp(head->ETIKET,headtable[h].ETIKET)
                     && head->DATEV==headtable[h].DATEV && head->IP1==headtable[h].IP1 && head->IP2==headtable[h].IP2 && head->IP3==headtable[h].IP3
                     && head->GRTYP[0]==headtable[h].GRTYP[0] && head->IG1==headtable[h].IG1 && head->IG2==headtable[h].IG2 && head->IG3==headtable[h].IG3 && head->IG4==headtable[h].IG4) {

                     App_Log(ERROR,"Field is duplicated: NOMVAR=%4s TYPVAR=%4s IP1=%8i IP2=%8i IP3=%8i DATEV=%8i\n",head->NOMVAR,head->TYPVAR,head->IP1,head->IP2,head->IP3,head->DATEV);
                     err++;
                     break;
                  }
               }

               // Check horizontal grid
               for(h=0;h<hrefnb;h++) {
                  if (href[h].IP1==head->IG1 && href[h].IP2==head->IG2 && href[h].IP3==head->IG3) {
                     href[h].NRef++;
                     break;
                  }
               }

               // This is a new HRef
               if (h>=DICT_MAXREF-1) {
                  App_Log(ERROR,"Maximum horizontal grid definitions attained: DICT_MAXREF\n");
                  err++;
               } else if (h==hrefnb) {
                  href[h].IP1=head->IG1;
                  href[h].IP2=head->IG2;
                  href[h].IP3=head->IG3;
                  href[h].NRef=0;
                  href[h].GRTYP=head->GRTYP[0];
                  hrefnb++;
               }

               // Check vertical grid
               ZRef_IP2Level(head->IP1,&ztyp);
               z=0;
               while(zref[z]!=ztyp && z<zrefnb) {
                  z++;
               }

               // This is a new ZRef
               if (z>=DICT_MAXREF-1) {
                  App_Log(ERROR,"Maximum vertical grid definitions attained: DICT_MAXREF\n");
                  err++;
               } else if (z==zrefnb) {
                  zref[z]=ztyp;
                  zrefnb++;
               }
            }

            // Check for descriptor
            if (!strncmp(head->NOMVAR,"P0",2)) {
               p0++;
            } else if (!strncmp(head->NOMVAR,"PT",2)) {
               pt++;
            } else if (!strncmp(head->NOMVAR,"HY",2)) {
               hy++;
            } else if (!strncmp(head->NOMVAR,"!!",2)) {
               zver=head->IG1;
               toc++;
            } else if (!strncmp(head->NOMVAR,"^^",2)) {
               tic++;
            } else if (!strncmp(head->NOMVAR,">>",2)) {
               tac++;
            } else if (!strncmp(head->NOMVAR,"^>",2)) {
               tuc++;
            }
         }
         head=&headtable[++headnb];
         head->KEY=c_fstsui(fid,&ni,&nj,&nk);
      }

      if (CheckOPS) {

         // Check horizontal grid descriptor
         nh=0;
         for(h=0;h<hrefnb;h++) {
            if (href[h].GRTYP=='Z' || href[h].GRTYP=='Y' || href[h].GRTYP=='E') {
               nhtic=nhtac=nhtoc=nhtuc=0;
               nh++;
               for(z=0;z<headnb;z++) {
                  if (href[h].IP1=headtable[z].IP1 && href[h].IP2==headtable[z].IP2 && href[h].IP3==headtable[z].IP3) {
                     if (!strcmp(">>  ",headtable[z].NOMVAR)) nhtic=1;
                     if (!strcmp("^^  ",headtable[z].NOMVAR)) nhtac=1;
                     if (!strcmp("!!  ",headtable[z].NOMVAR)) nhtoc=1;
                     if (!strcmp("^>  ",headtable[z].NOMVAR)) nhtuc=1;

                     // check for valid ETIKET and DATEO
                     if (nhtic || nhtac || nhtoc || nhtuc) {

                        if (strcmp(etiket,headtable[z].ETIKET)) {
                           App_Log(ERROR,"Descriptor %s ETIKET is different from fields (\"%s\"!=\"%s\")\n",headtable[z].NOMVAR,etiket,headtable[h].ETIKET);
                           err++;
                        }
//                        if (dateo!=headtable[z].DATEO) {
//                           App_Log(ERROR,"Descriptor %s DATEO is different from fields (%i!=%i)\n",headtable[z].NOMVAR,dateo,headtable[h].DATEO);
//                           err++;
//                        }
                     }
                  }
               }

               // Check for necessary ^^,>>
               if (!nhtic) {
                  App_Log(ERROR,"Missing >> for grid GRTYP=%c IP1=%-8i IP2=%-8i IP3=%-8i\n",href[h].GRTYP,href[h].IP1,href[h].IP2,href[h].IP3);
                  err++;
               }
               if (!nhtac) {
                  App_Log(ERROR,"Missing ^^ for grid GRTYP=%c IP1=%-8i IP2=%-8i IP3=%-8i\n",href[h].GRTYP,href[h].IP1,href[h].IP2,href[h].IP3);
                  err++;
               }
            }
         }

         // Check for too many grid descriptor
         if (tic>nh) {
            App_Log(ERROR,"Found too many horizontal grid descriptor ^^\n");
            err++;
         }
         if (tac>nh) {
            App_Log(ERROR,"Found too many horizontal grid descriptor >>\n");
            err++;
         }
         if (tuc>nh) {
            App_Log(ERROR,"Found too many horizontal grid descriptor ^>\n");
            err++;
         }
         if (toc>1) {
            App_Log(ERROR,"Found too many horizontal grid descriptor !!\n");
            err++;
         }

         // Check vertical grid descriptor
         for(z=0;z<zrefnb;z++) {
            if (!zver) {
               // Check vertical coordinate with legacy version
               if (ztyp==LVL_SIGMA && (!p0 || !pt)) {
                  App_Log(ERROR,"Missing P0 or PT for ETA level definition\n");
                  err++;
               }
               if (ztyp==LVL_HYBRID && (!p0 || !hy)) {
                  App_Log(ERROR,"Missing P0 or HY hor HYBRID level definition\n");
                  err++;
               }
            }
         }

         if (zver) {
            // Check vertical coordinate with vcode definition
            switch(zver) {
               case 1001: if (!p0) { App_Log(ERROR,"Missing P0 for SIGMA level definition\n"); err++; }; break;
               case 1002:
               case 1003: if (!p0 || !pt) { App_Log(ERROR,"Missing P0 or PT for ETA level definition\n"); err++; }; break;
               case 2001: break;
               case 5001: if (!p0 || !hy) { App_Log(ERROR,"Missing P0 or HY for HYBRID level definition\n"); err++; }; break;
               case 5002:
               case 5003:
               case 5005: if (!p0) { App_Log(ERROR,"Missing P0 for HYBRID level definition\n"); err++; }; break;
            }
         }
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

         // Afficher la variable et les 10 premiers IP1
         printf("%-4s  ",var->Name);
         for(n=0;n<(nidx>10?10:nidx);n++) {
            c_fstprm(idxs[n],&head->DATEO,&head->DEET,&head->NPAS,&ni,&nj,&nk,&head->NBITS,&head->DATYP,&head->IP1,&head->IP2,&head->IP3,head->TYPVAR,head->NOMVAR,
                    head->ETIKET,head->GRTYP,&head->IG1,&head->IG2,&head->IG3,&head->IG4,&head->SWA,&head->LNG,&head->DLTF,&head->UBC,&head->EX1,&head->EX2,&head->EX3);
            printf ("%8i ",head->IP1);
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

   return(!nb_unknown && !err);
}
