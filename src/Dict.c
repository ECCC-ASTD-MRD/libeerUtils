/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Manipulation des fichiers xml dictionnaire des variables
 * Fichier      : Dict.c
 * Creation     : Mai 2014 - J.P. Gauthier
 * Revision     : $Id$
 *
 * Description  : Basé fortement sur le code d'Yves Chartier afin de convertir
 *                Le binaire en fonctions de librairies.
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
#include <libxml/xmlmemory.h>
#include <libxml/encoding.h>
#include <libxml/parser.h>

#include "Dict.h"
#include "RPN.h"

char *TSHORT[]      = { "Description courte ","Short Description  " };
char *TLONG[]       = { "Description longue ","Long  Description  " };
char *TUNITES[]     = { "Unités             ","Units              " };
char *TDATE[]       = { "Date du status     ","Date of state      " };
char *TORIGIN[]     = { "Origine            ","Origin             " };
char *TSTATE[]      = { "Status             ","State              " };
char *TTYPE[]       = { "Représentation     ","Representation     " };
char *TMAG[]        = { "Ordre de grandeur  ","Magnitude          " };
char *TPREC[]       = { "Précision requise  ","Required precision " };
char *TPACK[]       = { "Compaction optimale","Optimal compaction " };
char *TRANGE[]      = { "Amplitude          ","Range              " };

char *TINT[]        = { "Variable entière"  ,"Integer Variable" };
char *TREAL[]       = { "Variable réelle"   ,"Real Variable"    };
char *TLOGIC[]      = { "Variable logique"  ,"Logical Variable" };
char *TCODE[]       = { "Variable codée"    ,"Coded Variable"   };
char *TVAL[]        = { "Valeur"            ,"Value"            };

char *TOBSOLETE[]   = { "Obsolète"   ,"Obsolete"   };
char *TFUTURE[]     = { "Futur"      ,"Future"     };
char *TCURRENT[]    = { "Courante"   ,"Current"    };
char *TINCOMPLETE[] = { "Incomplète" ,"Icompletee" };

char *TCENTILE[]    = { "e centile"                   ,"th percentile" };
char *TMIN[]        = { "(minimum)"                   ,"(minimum)" };
char *TMAX[]        = { "(maximum)"                   ,"(maximum)" };
char *TSSTD[]       = { "(écart-type (population))"   ,"(standard deviation (population))" };
char *TPSTD[]       = { "(écart-type (échantillon))"  ,"(standard deviation (sample))" };
char *TMEAN[]       = { "(moyenne)"                   ,"(mean)" };
char *TEFI[]        = { "(index de prévision extrème)","(extreme forecast index)" };
char *TBETWEEN[]    = { "entre","between" };
char *TAND[]        = { "et","and" };
char *TDAY[]        = { "le jour","day" };
char *THOUR[]       = { "l'heure","hour" };

typedef struct {
   char          *Name,*Date,*Version,String[64];     // Dictionnary metadata
   TList         *Vars;                               // List of dictionnary variables
   TList         *Types;                              // List of dictionnary types
} TDict;

typedef struct {
   char          *Modifier;                           // Variable modifier (ETIKET)
   int            Mode;                               // Search mode (DICT_EXACT,DICT_GLOB)
   int            State;                              // Search state (DICT_OBSOLETE,DICT_CURRENT,DICT_FUTURE,DICT_INCOMPLETE)
   char          *Origin;                             // Search origin
   char          *ETIKET;                             // Search etiket
   int            IP1,IP2,IP3;                        // IP to look for
   int            AltIP1,AltIP2,AltIP3;               // Alternate IP to look for (OLD/NEW)
} TDictSearch;

static          TDict       Dict;                     // Global dictionnary
static __thread TDictSearch DictSearch;               // Per thread search params

static int Dict_ParseVar(xmlDocPtr Doc,xmlNsPtr NS,xmlNodePtr Node,TDict_Encoding Encoding);
static int Dict_ParseType(xmlDocPtr Doc,xmlNsPtr NS,xmlNodePtr Node,TDict_Encoding Encoding);

int Dict_Encoding(char *string,TDict_Encoding Encoding) {

   char tmpString[DICT_MAXLEN];
   int  tmplenout, tmplenin;
   int  i,i2;

   tmplenin = strlen(string);
   tmplenout = tmplenin;
   memset(tmpString,'\0',DICT_MAXLEN);

   switch (Encoding) {

      case DICT_ASCII:
         i= i2=0;
         while (i<tmplenout && i2<DICT_MAXLEN-1) {

            switch ((unsigned char)string[i]) {
               case 0xE2:
                  i++;
                  i++;
                  switch ((unsigned char) string[i]) {
                     case 0xBB:
                        tmpString[i2] = '-';
                        break;
                  }
                  break;
                 
               case 0xC2:
                  i++;
                  switch ((unsigned char) string[i]) {
                     case 0x2D:
                        tmpString[i2] = '-';
                        break;
                        
                     case 0xB0:
                        tmpString[i2] = ' ';
                        break;

                     case 0xB2:
                        tmpString[i2] = '2';
                        break;

                     case 0xB3:
                        tmpString[i2] = '3';
                        break;

                     case 0xB5:
                        tmpString[i2] = 'u';
                        break;
                        
                     case 0xB9:
                        tmpString[i2] = '1';
                        break;
                  }
                  break;

               case 0xC3:
                  i++;
                  
                  switch ((unsigned char) string[i]) {
                     case 0xB5:
                        tmpString[i2] = 'u';
                        break;

                     case 0x80:
                     case 0x81:
                     case 0x82:
                        tmpString[i2] = 'A';
                        break;

                     case 0x88:
                     case 0x89:
                     case 0x90:
                        tmpString[i2] = 'E';
                        break;

                     case 0xA0:
                     case 0xA1:
                     case 0xA2:
                        tmpString[i2] = 'a';
                        break;

                     case 0xA7:
                        tmpString[i2] = 'c';
                        break;

                     case 0xA8:
                     case 0xA9:
                     case 0xAA:
                     case 0xAB:
                        tmpString[i2] = 'e';
                        break;

                     case 0xEE:
                     case 0xAF:
                        tmpString[i2] = 'i';
                        break;

                     case 0xB4:
                     case 0xB6:
                        tmpString[i2] = 'o';
                        break;

                     case 0xB9:
                     case 0xBA:
                     case 0xBB:
                        tmpString[i2] = 'u';
                        break;

                     default:
                        break;
                  }
                  break;

               default:
                  tmpString[i2] = string[i];
                  break;
            }
            i++;
            i2++;
         }
         strcpy(string,tmpString);
         break;

      case DICT_ISO8859_1:
         tmplenout=tmplenin;
         UTF8Toisolat1(tmpString,&tmplenout,string,&tmplenin);
         strcpy(string,tmpString);
         break;

      case DICT_UTF8:
         break;
    }
}

char* Dict_Version(void) {
   return(Dict.String);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_SetSearch>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Define search params
 *
 * Parametres  :
 *  <SearchMode>   :
 *  <SearchState>  :
 *  <SearchOrigin> :
 *  <SearchIP1>    :
 *  <SearchIP2>    :
 *  <SearchIP3>    :
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_SetSearch(int SearchMode,int SearchState,char *SearchOrigin,int SearchIP1,int SearchIP2,int SearchIP3,char *SearchETIKET) {

   int    mode=-1,flag=0,type;
   float  level=0.0;
   char   format;

   DictSearch.State=SearchState;
   DictSearch.Mode=SearchMode;
   DictSearch.Origin=SearchOrigin;
   DictSearch.IP1=SearchIP1;
   DictSearch.IP2=SearchIP2;
   DictSearch.IP3=SearchIP3;
   DictSearch.ETIKET=SearchETIKET;

#ifdef HAVE_RMN
   if (DictSearch.IP1>0) {
      // Convert to real level/value
      f77name(convip)(&SearchIP1,&level,&type,&mode,&format,&flag);
      // Get alternate representation (OLD/NEW)
      mode=(SearchIP1<32000)?2:3;
      f77name(convip)(&DictSearch.AltIP1,&level,&type,&mode,&format,&flag);
   }

//   f77name(convip)(&SearchIP2,&level,Type,&mode,&format,&flag);
//   f77name(convip)(&SearchIP3,&level,Type,&mode,&format,&flag);

//   mode=(SearchIP2<32000)?2:3;
//   f77name(convip)(&DictSearch.AltIP2,&level,Type,&mode,&format,&flag);

//   mode=(SearchIP3<32000)?2:3;
//   f77name(convip)(&DictSearch.AltIP3,&level,Type,&mode,&format,&flag);
#else
   App_Log(ERROR,"%s: Need RMNLIB\n",__func__);
#endif
}

void Dict_SetModifier(char *Modifier) {

   if (DictSearch.Modifier)
      free(DictSearch.Modifier);

   DictSearch.Modifier=NULL;

   if (Modifier)
      DictSearch.Modifier=strdup(Modifier);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_Parse>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Read and parse an XML dictionnary file.
 *
 * Parametres  :
 *  <Filename> : XML dictionnary file
 *  <Encoding> : Encoding mode (DICT_ASCII,DICT_UTF8,DICT_ISO8859_1)
 *
 * Retour:
 *
 * Remarques :
 *    - Based heavily on r.dict code
 *----------------------------------------------------------------------------
*/
int Dict_Parse(char *Filename,TDict_Encoding Encoding) {

   xmlDocPtr    doc;
   xmlNsPtr     ns;
   xmlNodePtr   node;
   xmlDtdPtr    dtd ;
   xmlValidCtxt ctxt;
   xmlChar     *tmpc;
   char        *c,ok,dtdfile[256];

   xmlDoValidityCheckingDefaultValue=1;
   LIBXML_TEST_VERSION
//   xmlKeepBlanksDefault(0);

   // Build the XML tree from the file
   if (!(doc=xmlParseFile(Filename))) {
      App_Log(ERROR,"%s: Invalid dictionnary file: %s\n",__func__,Filename);
      return(0);
   }

   // Check the document is of the right kind
   if (!(node = xmlDocGetRootElement(doc))) {
      App_Log(ERROR,"%s: Empty document\n",__func__);
      xmlFreeDoc(doc);
      return(0);
   }

   Dict.Name=NULL;
   Dict.Date=strdup(xmlGetProp(node,"date"));
   Dict.Version=strdup(xmlGetProp(node,"version_number"));

   if (tmpc=(char*)xmlGetProp(node,"name")) {
      Dict.Name=strdup(tmpc);
      sprintf(Dict.String,"%s %s %s version %s",node->name,Dict.Name,Dict.Date,Dict.Version);
   } else {
      sprintf(Dict.String,"%s %s version %s",node->name,Dict.Date,Dict.Version);
   }

  // Parse the DTD
//    strcpy(dtdfile,"");
//    strcat(dtdfile,getenv("AFSISIO"));
//    strcat(dtdfile,"/datafiles/constants/dict-2.0.dtd");
//
//    if (!(dtd=xmlParseDTD(NULL,dtdfile))) {
//       App_Log(ERROR,"%s: Could not parse DTD %s\n",__func__,dtdfile);
//       return (1);
//    }
//
//    // Set up xmlValidCtxt for error reporting when validating
//    ctxt.userData = stderr;
//    ctxt.error    = (xmlValidityErrorFunc) fprintf;   // register error function
//    ctxt.warning  = (xmlValidityWarningFunc) fprintf; // register warning function
//
//    if (!xmlValidateDtd(&ctxt,doc,dtd)) {
//       App_Log(ERROR,"%s: DTD validation error\n",__func__);
//       return (0);
//    }

   node=node->children;

   // Now, walk the tree
   ok=1;
   while (node) {

      if (!strcmp(node->name,"metvar")) {
         if (!(ok=Dict_ParseVar(doc,ns,node,Encoding))) break;
      }

      if (!strcmp(node->name,"typvar")) {
         if (!(ok=Dict_ParseType(doc,ns,node,Encoding))) break;
      }

      node=node->next;
   }

    xmlFreeDtd(dtd);
    xmlFreeDoc(doc);

    return(ok);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_ParseVar>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Read and parse an XML node nofr NOMVAR info.
 *
 * Parametres  :
 *  <Doc>      : XML document
 *  <NS>       :
 *  <Node>     : XML node to parse
 *  <Encoding> : Encoding mode (DICT_ASCII,DICT_UTF8,DICT_ISO8859_1)
 *
 * Retour:
 *
 * Remarques :
 *    - Based heavily on r.dict code
 *----------------------------------------------------------------------------
*/
static int Dict_ParseVar(xmlDocPtr Doc,xmlNsPtr NS,xmlNodePtr Node,TDict_Encoding Encoding) {

   TDictVar  *metvar;
   xmlNodePtr trotteur,trotteur1;
   xmlChar   *tmpc;
   int        i,y,m,d;

   metvar=(TDictVar*)calloc(1,sizeof(TDictVar));
   metvar->IP1=metvar->IP2=metvar->IP3=metvar->Pack=-1;
   metvar->Min=metvar->Max=metvar->Magnitude=metvar->Precision=DICT_NOTSET;

   if ((tmpc=(char*)xmlGetProp(Node,"origin"))) {
      strncpy(metvar->Origin,tmpc,32);
   }

   if ((tmpc=(char*)xmlGetProp(Node,"pack"))) {
      metvar->Pack=atoi(tmpc);
   }

   if ((tmpc=(char*)xmlGetProp(Node,"usage"))) {
      if (!strcmp(tmpc,"obsolete")) {
         metvar->Nature|=DICT_OBSOLETE;
      } else if (!strcmp(tmpc,"current")) {
         metvar->Nature|=DICT_CURRENT;
      } else if (!strcmp(tmpc,"future")) {
         metvar->Nature|=DICT_FUTURE;
      } else if (!strcmp(tmpc,"incomplete")) {
         metvar->Nature|=DICT_INCOMPLETE;
      }
   }

   if ((tmpc=(char*)xmlGetProp(Node,"date"))) {
      sscanf(tmpc,"%i-%i-%i",&y,&m,&d);
      metvar->Date=System_DateTime2Seconds(y*10000+m*100+d,0,TRUE);
   }

   Node=Node->children;
   while (Node) {

      if (!strcmp((char*)Node->name,"nomvar")) {
         if (!xmlNodeListGetString(Doc,Node->children,1)) {
            App_Log(ERROR,"%s: Empty variable definition\n",__func__);
            return(0);
         }
         strncpy(metvar->Name,xmlNodeListGetString(Doc,Node->children,1),5);
         if (tmpc=(char*)xmlGetProp(Node,"ip1")) {
            metvar->IP1=atoi(tmpc);
         }
         if (tmpc=(char*)xmlGetProp(Node,"ip2")) {
            metvar->IP2=atoi(tmpc);
         }
         if (tmpc=(char*)xmlGetProp(Node,"ip3")) {
            metvar->IP3=atoi(tmpc);
         }
         if (tmpc=(char*)xmlGetProp(Node,"etiket")) {
            strncpy(metvar->ETIKET,tmpc,13);
         }
      } else

      if (!strcmp((char*)Node->name,"description")) {
         trotteur1=Node->children;
         while (trotteur1) {
            if (!strcmp((char*)trotteur1->name,"short") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
               if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"en")) {
                  strncpy(metvar->Short[1], tmpc,128);
                  Dict_Encoding(metvar->Short[1],Encoding);
               } else if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"fr")) {
                  strncpy(metvar->Short[0], tmpc,128);
                  Dict_Encoding(metvar->Short[0],Encoding);
               }
            } else if (!strcmp((char*)trotteur1->name,"long") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
               if (xmlGetProp(trotteur1,"lang")) {
                  if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"fr")) {
                     strncpy(metvar->Long[0],tmpc,DICT_MAXLEN);
                     Dict_Encoding(metvar->Long[0],Encoding);
                  } else if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"en")) {
                     strncpy(metvar->Long[1],tmpc,DICT_MAXLEN);
                     Dict_Encoding(metvar->Long[1],Encoding);
                  }
               } else {
                  strncpy(metvar->Long[0],tmpc,DICT_MAXLEN);
                  Dict_Encoding(metvar->Long[0],Encoding);
                  strncpy(metvar->Long[1],tmpc,DICT_MAXLEN);
                  Dict_Encoding(metvar->Long[1],Encoding);
               }
            }
            trotteur1=trotteur1->next;
         }
      } else

      if (!strcmp((char*)Node->name,"measure")) {
         trotteur=Node->children->next;

         // Integer
         if (!strcmp((char*)trotteur->name,"integer")) {
            metvar->Nature|=DICT_INTEGER;

            trotteur1=trotteur->children;
            while (trotteur1) {
               if (!strcmp((char*)trotteur1->name,"units") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  strncpy(metvar->Units, tmpc,32);
                  Dict_Encoding(metvar->Units,Encoding);
               }

               if (!strcmp((char*)trotteur1->name,"magnitude") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  metvar->Magnitude=atof(tmpc);
               }

               if (!strcmp((char*)trotteur1->name,"min") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  metvar->Min=atof(tmpc);
               }

               if (!strcmp((char*)trotteur1->name,"max") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  metvar->Max=atof(tmpc);
               }

               trotteur1=trotteur1->next;
            }
         } else

         // Real
         if (!strcmp((char*)trotteur->name,"real")) {
            metvar->Nature|=DICT_REAL;

            trotteur1=trotteur->children;
            while (trotteur1) {
               if (!strcmp((char*)trotteur1->name,"units") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  strncpy(metvar->Units,tmpc,32);
                  Dict_Encoding(metvar->Units,Encoding);
               }

               if (!strcmp((char*)trotteur1->name,"magnitude") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  metvar->Magnitude=atof(tmpc);
               }

               if (!strcmp((char*)trotteur1->name,"precision") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  metvar->Precision=atof(tmpc);
               }

               if (!strcmp((char*)trotteur1->name,"min") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  metvar->Min=atof(tmpc);
               }

               if (!strcmp((char*)trotteur1->name,"max") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  metvar->Max=atof(tmpc);
               }

               trotteur1=trotteur1->next;
            }
         } else

         // Logical
         if (!strcmp((char*)trotteur->name,"logical")) {
            strcpy(metvar->Units,"bool");
            metvar->Nature|=DICT_LOGICAL;

         } else

         // Code
         if (!strcmp((char*)trotteur->name,"code")) {
            strcpy(metvar->Units,"code");
            metvar->Nature|=DICT_CODE;

            trotteur1=trotteur->children;
            i=0;
            while (trotteur1) {
               if (!strcmp((char*)trotteur1->name,"value") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  tmpc=xmlNodeListGetString(Doc,trotteur1->children,1);
                  metvar->Codes[i]=atoi(tmpc);
               }
               if (!strcmp((char*)trotteur1->name,"meaning") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                  strncpy(metvar->Meanings[i], tmpc,64);
                  Dict_Encoding(metvar->Meanings[i],Encoding);
                  i++;
               }
               trotteur1=trotteur1->next;
            }
            metvar->NCodes=i;
         }
      }

      Node=Node->next;
   }

   Dict.Vars=TList_AddSorted(Dict.Vars,Dict_SortVar,metvar);

   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_ParseType>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Read and parse an XML node for TYPVAR info.
 *
 * Parametres  :
 *  <Doc>      : XML document
 *  <NS>       :
 *  <Node>     : XML node to parse
 *  <Encoding> : Encoding mode (DICT_ASCII,DICT_UTF8,DICT_ISO8859_1)
 *
 * Retour:
 *
 * Remarques :
 *    - Based heavily on r.dict code
 *----------------------------------------------------------------------------
*/
static int Dict_ParseType(xmlDocPtr Doc, xmlNsPtr NS, xmlNodePtr Node,TDict_Encoding Encoding) {

   TDictType *type;
   xmlNodePtr trotteur,trotteur1;
   xmlChar   *tmpc;
   int       y,m,d;

   type=(TDictType*)calloc(1,sizeof(TDictType));

   if (tmpc=(char*)xmlGetProp(Node,"origin")) {
      strncpy(type->Origin,tmpc,32);
   }

   if ((tmpc=(char*)xmlGetProp(Node,"usage"))) {
      if (!strcmp(tmpc,"obsolete")) {
         type->Nature|=DICT_OBSOLETE;
      } else if (!strcmp(tmpc,"current")) {
         type->Nature|=DICT_CURRENT;
      } else if (!strcmp(tmpc,"future")) {
         type->Nature|=DICT_FUTURE;
      } else if (!strcmp(tmpc,"incomplete")) {
         type->Nature|=DICT_INCOMPLETE;
      }
   }

   if ((tmpc=(char*)xmlGetProp(Node,"date"))) {
      sscanf(tmpc,"%i-%i-%i",&y,&m,&d);
      type->Date=System_DateTime2Seconds(y*10000+m*100+d,0,TRUE);
   }

   Node=Node->children;
   while (Node) {
      if (!strcmp((char*)Node->name,"nomtype")) {
         strncpy(type->Name,xmlNodeListGetString(Doc,Node->children,1),3);
      }

      // DESCRIPTION
      if (!strcmp((char*)Node->name,"description")) {
         trotteur1=Node->children;
         while (trotteur1) {
            if (!strcmp((char*)trotteur1->name,"short") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
               if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"en")) {
                  strncpy(type->Short[1],tmpc,128);
                  Dict_Encoding(type->Short[1],Encoding);
               } else {
                  if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"fr")) {
                     strncpy(type->Short[0],tmpc,128);
                     Dict_Encoding(type->Short[0],Encoding);
                  }
               }
            } else if (!strcmp((char*)trotteur1->name,"long") && (tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
               if (xmlGetProp(trotteur1,"lang")) {
                  if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"fr")) {
                     strncpy(type->Long[0],tmpc,DICT_MAXLEN);
                     Dict_Encoding(type->Long[0],Encoding);
                  } else if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"en")) {
                     strncpy(type->Long[1],tmpc,DICT_MAXLEN);
                     Dict_Encoding(type->Long[1],Encoding);
                  }
               } else {
                  strncpy(type->Long[0],tmpc,DICT_MAXLEN);
                  Dict_Encoding(type->Long[0],Encoding);
                  strncpy(type->Long[1],tmpc,DICT_MAXLEN);
                  Dict_Encoding(type->Long[1],Encoding);
               }
            }

            trotteur1=trotteur1->next;
         }
      }

      Node=Node->next;
   }

   Dict.Types=TList_AddSorted(Dict.Types,Dict_SortType,type);

   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_AddVar>
 * Creation : Juin 2014 - J.P. Gauthier
 *
 * But      : Add a type.
 *
 * Parametres  :
 *  <Var>      : Variable to add to dictionnary
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_AddVar(TDictVar *Var) {
   Dict.Vars=TList_AddSorted(Dict.Vars,Dict_SortVar,Var);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_SortVar>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Order two variables.
 *
 * Parametres  :
 *  <Data0>    : Metvar to compare to
 *  <Data1>    : Metvar to compare
 *
 * Retour:
 *   0 si egal, -1 si plus petit 1 si plus grand
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int Dict_SortVar(void *Data0,void *Data1){
   return(strcasecmp(((TDictVar*)Data0)->Name,((TDictVar*)Data1)->Name));
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_CheckVar>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Check if a var satisfies search parameters
 *
 * Parametres  :
 *  <Data0>    : Metvar to compare to
 *  <Data1>    : Metvar to compare
 *
 * Retour:
 *   0 = no, 1 = yes
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int Dict_CheckVar(void *Data0,void *Data1){

   if (!Data0) {
      return(0);
   }

   if (DictSearch.State && !(((TDictVar*)Data0)->Nature&DictSearch.State)) {
      return(0);
   }

   if (DictSearch.Origin && strcasecmp(((TDictVar*)Data0)->Origin,DictSearch.Origin)) {
      return(0);
   }

   if (DictSearch.ETIKET && strcasecmp(((TDictVar*)Data0)->ETIKET,DictSearch.ETIKET)) {
      return(0);
   }

   if (DictSearch.IP1>0 && ((TDictVar*)Data0)->IP1!=DictSearch.IP1 && ((TDictVar*)Data0)->IP1!=DictSearch.AltIP1) {
      return(0);
   }

   if (DictSearch.IP2>0 && ((TDictVar*)Data0)->IP2!=DictSearch.IP2) {
      return(0);
   }

   if (DictSearch.IP3>0 && ((TDictVar*)Data0)->IP3!=DictSearch.IP3) {
      return(0);
   }

   if (DictSearch.Mode==DICT_GLOB) {
      return(Data1?(!strmatch(((TDictVar*)Data0)->Name,(char*)Data1)):1);
   } else {
      return(Data1?(!strcasecmp(((TDictVar*)Data0)->Name,(char*)Data1)):0);
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_GetVar>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Search for a var
 *
 * Parametres  :
 *  <Var>      : Variable name
 *
 * Retour:
 *  <TDictVar> : Variable info
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
TDictVar *Dict_GetVar(char *Var) {

   TList *list;

   list=TList_Find(Dict.Vars,Dict_CheckVar,Var);
   return(list?(TDictVar*)(list->Data):NULL);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_IterateVar>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Iterator over a list of TDictVar
 *
 * Parametres  :
 *  <ITerator> : List pointer start (NULL for beginning of list)
 *  <Var>      : Var to look for (if NULL, iterate over all)
 *
 * Retour:
 *  <TDictVar> : Variable info
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
TDictVar *Dict_IterateVar(TList **Iterator,char *Var) {

   TDictVar *var=NULL;

   if (*Iterator!=(TList*)0x01) {

      // If NULL as iterator, start at beginning
      if (!(*Iterator)) {
         *Iterator=Dict.Vars;
      }

      // If a search proc and var is specified
      if ((*Iterator=TList_Find((*Iterator),Dict_CheckVar,Var))) {
         var=(TDictVar*)((*Iterator)->Data);
         *Iterator=(*Iterator)->Next;
      }
      *Iterator=(*Iterator==NULL?(TList*)0x01:*Iterator);
   }
   return(var);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_AddType>
 * Creation : Juin 2014 - J.P. Gauthier
 *
 * But      : Add a type.
 *
 * Parametres  :
 *  <Type>     : Typevar to add to dictionnary
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_AddType(TDictType *Type) {
   Dict.Vars=TList_AddSorted(Dict.Types,Dict_SortType,Type);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_SortType>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Order two types.
 *
 * Parametres  :
 *  <Data0>    : Typevar to compare to
 *  <Data1>    : Typevar to compare
 *
 * Retour:
 *   0 si egal, -1 si plus petitm 1 si plus grand
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int Dict_SortType(void *Data0,void *Data1){
   return(strcasecmp(((TDictType*)Data0)->Name,((TDictType*)Data1)->Name));
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_CheckType>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Check if a type satisfies search parameters
 *
 * Parametres  :
 *  <Data0>    : Metvar to compare to
 *  <Data1>    : Metvar to compare
 *
 * Retour:
 *   0 = no, 1 = yes
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int Dict_CheckType(void *Data0,void *Data1){

   if (!Data0) {
      return(0);
   }

   if (DictSearch.State && !(((TDictType*)Data0)->Nature&DictSearch.State)) {
      return(0);
   }

   if (DictSearch.Origin && strcasecmp(((TDictType*)Data0)->Origin,DictSearch.Origin)) {
      return(0);
   }

   if (DictSearch.Mode==DICT_GLOB) {
      return(Data1?(!strmatch(((TDictType*)Data0)->Name,(char*)Data1)):1);
   } else {
      return(Data1?(!strcasecmp(((TDictType*)Data0)->Name,(char*)Data1) || !strcasecmp(&((TDictType*)Data0)->Name[1],(char*)Data1)) :0);
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_GetType>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Search for a type
 *
 * Parametres  :
 *  <Type>     : Type name
 *
 * Retour:
 *  <TDictType>: Type info
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
TDictType *Dict_GetType(char *Type) {

   TList *list;

   list=TList_Find(Dict.Types,Dict_CheckType,Type);
   return(list?(TDictType*)(list->Data):NULL);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_IterateType>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Iterator over a list of TDictType
 *
 * Parametres  :
 *  <ITerator> : List pointer start (NULL for beginning of list)
 *  <Type>     : Type to look for (if NULL, iterate over all)
 *
 * Retour:
 *  <TDictType> : Type info
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
TDictType *Dict_IterateType(TList **Iterator,char *Type) {

   TDictType *type=NULL;

   if (*Iterator!=(TList*)0x01) {

      // If NULL as iterator, start at beginning
      if (!(*Iterator)) {
         *Iterator=Dict.Types;
      }

      // If a search proc and var is specified
      if ((*Iterator=TList_Find((*Iterator),Dict_CheckType,Type))) {
         type=(TDictType*)((*Iterator)->Data);
         *Iterator=(*Iterator)->Next;
      }
      *Iterator=(*Iterator==NULL?(TList*)0x01:*Iterator);
   }

   return(type);
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_PrintVars>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Print info about a list of variables
 *
 * Parametres :
 *  <Var>     : Variable to look fortement
 *  <Format>  : Print format (DICT_SHORT,DICT_LONG)
 *  <Lang>    : Language (APP_FR,APP_EN)
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_PrintVars(char *Var,int Format,TApp_Lang Lang) {

   TList *list;

   list=Dict.Vars;

   while(list=TList_Find(list,Dict_CheckVar,Var)) {
       Dict_PrintVar((TDictVar*)list->Data,Format,Lang);
       list=list->Next;
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_PrintVar>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Print info about a variable
 *
 * Parametres :
 *  <Var>     : Variable to look fortement
 *  <Format>  : Print format (DICT_SHORT,DICT_LONG)
 *  <Lang>    : Language (APP_FR,APP_EN)
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_PrintVar(TDictVar *DVar,int Format,TApp_Lang Lang) {

   int         i;
   struct tm *tm;
   TDictVar  *var;

//   #include <sys/ioctl.h>
//   struct winsize w;
//   ioctl(0, TIOCGWINSZ,&w);
//    printf ("lines %d\n", w.ws_row);
//    printf ("columns %d\n", w.ws_col);

   if (DVar) {
      if (!(var=Dict_ApplyModifier(DVar,DictSearch.Modifier))) {
         return;
      }
      if (var->Nature&DICT_OBSOLETE) printf(APP_COLOR_RED);

      switch(Format) {
         case DICT_SHORT:
            printf("%-4s\t%-70s\t%-s",var->Name,var->Short[Lang],var->Units);
            if (var->Nature&DICT_OBSOLETE)
               printf(" \t%s\n",TOBSOLETE[Lang]);
            else
               printf("\n");

            break;

         case DICT_LONG:
            printf("--------------------------------------------------------------------------------\n");
            printf("Nomvar              : %-s", var->Name);
            if (var->IP1>=0)          printf(" IP1(%i)",var->IP1);
            if (var->IP2>=0)          printf(" IP2(%i)",var->IP2);
            if (var->IP3>=0)          printf(" IP3(%i)",var->IP3);
            if (var->ETIKET[0]!='\0') printf(" ETIKET(%s)",var->ETIKET);

            printf("\n%-s : %-s\n", TSHORT[Lang],var->Short[Lang]);
            printf("%-s : %-s\n", TLONG[Lang],var->Long[Lang][0]!='\0'?var->Long[Lang]:"-");
            printf("%-s : %-s\n", TORIGIN[Lang],var->Origin[0]!='\0'?var->Origin:"-");
            printf("%-s : ", TSTATE[Lang]);

            if (var->Nature&DICT_OBSOLETE)
               printf("%s\n",TOBSOLETE[Lang]);
            else if (var->Nature&DICT_CURRENT)
               printf("%s\n",TCURRENT[Lang]);
            else if (var->Nature&DICT_FUTURE)
               printf("%s\n",TFUTURE[Lang]);
             else if (var->Nature&DICT_INCOMPLETE)
               printf("%s\n",TINCOMPLETE[Lang]);

            if (var->Date) {
               tm=gmtime(&(var->Date));
               printf("%-s : %04i-%02i-%02i\n", TDATE[Lang],1900+tm->tm_year,tm->tm_mon+1,tm->tm_mday);
            } else {
               printf("%-s : %-s\n", TDATE[Lang],"-");
            }

            if (var->Pack>0)  {
               printf("%-s : %i\n",TPACK[Lang],var->Pack);
            } else {
               printf("%-s : %-s\n",TPACK[Lang],"-");
            }

            if (var->Nature & DICT_INTEGER) {
                  printf("%-s : %-s\n",TTYPE[Lang],TINT[Lang]);
                  printf("%-s : %s\n",TUNITES[Lang],var->Units);
                  if (var->Magnitude!=DICT_NOTSET)     printf("%-s : %e\n",TMAG[Lang],var->Magnitude);
                  if (var->Min!=var->Max) {
                     printf("%-s : ",TRANGE[Lang]);
                     if (var->Min!=DICT_NOTSET) printf("[%.0f ",var->Min);
                     if (var->Max!=DICT_NOTSET) printf("%.0f]\n",var->Max);
                     printf("\n");
                  }
                  break;

            } else if (var->Nature & DICT_REAL) {
                  printf("%-s : %-s\n",TTYPE[Lang],TREAL[Lang]);
                  printf("%-s : %s\n",TUNITES[Lang],var->Units);
                  if (var->Precision!=DICT_NOTSET)    printf("%-s : %e\n",TPREC[Lang],var->Precision);
                  if (var->Magnitude!=DICT_NOTSET)    printf("%-s : %e\n",TMAG[Lang],var->Magnitude);
                  if (var->Min!=var->Max) {
                     printf("%-s : ",TRANGE[Lang]);
                     if (var->Min!=DICT_NOTSET) printf("[%.0f ",var->Min);
                     if (var->Max!=DICT_NOTSET) printf("%.0f]\n",var->Max);
                     printf("\n");
                  }
                  break;

            } else if (var->Nature & DICT_LOGICAL) {
                  printf("%-s : %-s\n",TTYPE[Lang],TLOGIC[Lang]);
                  break;

            } else if (var->Nature & DICT_CODE) {
                  printf("%-s : %-s\n",TTYPE[Lang],TCODE[Lang]);
                  printf("\tCode\t\t%s\n",TVAL[Lang]);
                  printf("\t----\t\t----------------\n");
                  for (i=0; i < var->NCodes; i++) {
                     printf("\t%i\t\t%-s\n", var->Codes[i], var->Meanings[i]);
                  }
                  break;
            }
            break;
      }
      if (var!=DVar) free(var);
      if (var->Nature&DICT_OBSOLETE) printf(APP_COLOR_RESET);
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_PrintTypes>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Print info about a list of Types
 *
 * Parametres :
 *  <Var>     : Variable to look fortement
 *  <Format>  : Print format (DICT_SHORT,DICT_LONG)
 *  <Lang>    : Language (APP_FR,APP_EN)
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_PrintTypes(char *Type,int Format,TApp_Lang Lang) {

   TList *list;
   int   head=0;

   list=Dict.Types;

   while(list=TList_Find(list,Dict_CheckType,Type)) {
       if (!head) {
          printf("\n12-TYPVAR-----------------------------------------------------------------------\n");
          head++;
       }
       Dict_PrintType((TDictType*)list->Data,Format,Lang);
       list=list->Next;
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_PrintType>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Print info about a type
 *
 * Parametres :
 *  <Var>     : Variable to look fortement
 *  <Format>  : Print format (DICT_SHORT,DICT_LONG)
 *  <Lang>    : Language (APP_FR,APP_EN)
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_PrintType(TDictType *DType,int Format,TApp_Lang Lang) {

   int        i,lang;
   struct tm *tm;

   if (DType) {

      switch(Format) {
         case DICT_SHORT:
            printf("%-2s  %-60s\n", DType->Name, DType->Short[Lang]);
            break;

         case DICT_LONG:
            printf("--------------------------------------------------------------------------------\n");
            printf("Typvar             : %-s", DType->Name);
            printf("\n%-s : %-s\n", TSHORT[Lang],DType->Short[Lang]);
            printf("%-s : %-s\n", TLONG[Lang],DType->Long[Lang][0]!='\0'?DType->Long[Lang]:"-");

            if (DType->Date) {
               tm=gmtime(&(DType->Date));
               printf("%-s : %04i-%02i-%02i\n", TDATE[Lang],1900+tm->tm_year,tm->tm_mon+1,tm->tm_mday);
            } else {
               printf("%-s : %-s\n", TDATE[Lang],"-");
            }

            printf("%-s : %-s\n", TORIGIN[Lang],DType->Origin[0]!='\0'?DType->Origin:"-");
            printf("%-s : ", TSTATE[Lang]);

            if (DType->Nature&DICT_OBSOLETE)
               printf("%s\n",TOBSOLETE[Lang]);
            else if (DType->Nature&DICT_CURRENT)
               printf("%s\n",TCURRENT[Lang]);
            else if (DType->Nature&DICT_FUTURE)
               printf("%s\n",TFUTURE[Lang]);
            else if (DType->Nature&DICT_INCOMPLETE)
               printf("%s\n",TINCOMPLETE[Lang]);

            break;
      }
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_ApplyModifier>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Decode les étiquette au format RRPPPPPP[N,P,X]MMM
 *
 * Parametres :
 *  <Var>     : Variable to look fortement
 *  <Format>  : Print format (DICT_SHORT,DICT_LONG)
 *
 * Retour:
 *
 * Remarques :
 *
 *    RR     = No de passe
 *    PPPPPP = produit
 *    N,P,X  = Opérationnel, Parallèle, Expérimental
 *    MMM    = Membre (ou ALL pour tous)
 *----------------------------------------------------------------------------
*/
TDictVar* Dict_ApplyModifier(TDictVar *Var,char *Modifier) {

   char     *c,*l,lang;
   TDictVar *var=NULL;

   if (!Modifier || Modifier[0]=='\0') {
      return(Var);
   }

   // Copy var since we'll be changing a few things
   if ((var=(TDictVar*)calloc(1,sizeof(TDictVar)))) {
      memcpy(var,Var,sizeof(TDictVar));

      // Loop on languages
      for(lang=0;lang<2;lang++) {

         c=&Modifier[2];
         l=var->Short[lang];
         l+=strlen(var->Short[lang]);
         (*l++)=' ';

         if (!strncmp(c,"MIN___",6)) {
            strcpy(l,TMIN[lang]); l+=strlen(TMIN[lang]);
         } else if (!strncmp(c,"MAX___",6)) {
            strcpy(l,TMAX[lang]); l+=strlen(TMIN[lang]);
         } else if (!strncmp(c,"SSTD__",6)) {
            strcpy(l,TSSTD[lang]); l+=strlen(TMIN[lang]);
         } else if (!strncmp(c,"PSTD__",6)) {
            strcpy(l,TPSTD[lang]); l+=strlen(TMIN[lang]);
         } else if (!strncmp(c,"MEAN__",6)) {
            strcpy(l,TMEAN[lang]); l+=strlen(TMIN[lang]);
         } else if (!strncmp(c,"EFI___",6)) {
            strcpy(l,TEFI[lang]); l+=strlen(TMIN[lang]);
         } else {

            // Decode operator
            if (c[0]=='C') {

               c+=1;
               (*l++)='(';
               while(*c!='_') (*l++)=*c++;
               strcpy(l,TCENTILE[lang]); l+=strlen(TCENTILE[lang]);   // Centile operator
               (*l++)=')';

            } else {

               if  (c[0]=='G' && c[1]=='T') {                         // Greater than
                  c+=2; (*l++)='>';
               } else if  (c[0]=='L' && c[1]=='T') {                  // Less than
                  c+=2; (*l++)='<';
               } else if  (c[0]=='E' && c[1]=='Q') {                  // Equal to
                  c+=2; (*l++)='=';
               } else if  (c[0]=='G' && c[1]=='E') {                  // Greater or equal
                  c+=2; (*l++)='>'; (*l++)='=';
               } else if  (c[0]=='L' && c[1]=='E') {                  // Less or equal
                  c+=2; (*l++)='<'; (*l++)='=';
               } else {
                  App_Log(ERROR,"%s: Invalid modifier: %s\n",__func__,Modifier);
                  return(NULL);
               }
               (*l++)=' ';

               // Decode value of operator
               while(*c!='_') (*l++)=*c++;

               // Add variable unit
               (*l++)=' ';
               strcpy(l,Var->Units); l+=strlen(Var->Units);

               // New units -> %
               var->Units[0]='%';var->Units[1]='\0';
               var->Min=0.0;
               var->Max=100.0;
            }
         }
         
         // Check for range of time
         c=&Modifier[8];
         if (c[0]!='N') {
            (*l++)=' ';
            strcpy(l,TBETWEEN[lang]); l+=strlen(TBETWEEN[lang]);
            (*l++)=' ';
            
            if (c[0]=='D') {
               strcpy(l,TDAY[lang]); l+=strlen(TDAY[lang]);
            } else if (c[0]=='H') {
               strcpy(l,THOUR[lang]); l+=strlen(THOUR[lang]);
            }
            (*l++)=' ';
            (*l++)='1';
            (*l++)=' ';
            strcpy(l,TAND[lang]); l+=strlen(TAND[lang]);
            (*l++)=' ';
            if (c[1]!='0') (*l++)=c[1];
            if (c[1]!='0' || c[2]!='0') (*l++)=c[2];
            (*l++)=c[3];
         }
                  
         (*l++)='\0';
      }
   }
   return(var);
}
