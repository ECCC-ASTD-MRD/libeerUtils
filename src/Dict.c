/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Manipulation des fichiers xml dictionnaire des variables
 * Fichier      : Dict.c
 * Creation     : Mai 2014 - J.P. Gauthier
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
#include "App.h"
#include "Dict.h"

typedef struct {
   TList         *Vars;                            // List of dictionnary variables
   TList         *Types;                           // List of dictionnary types
   int            SearchMode;                      // Search mode (DICT_EXACT,DICT_GLOB)
   int            SearchState;                     // Search state (DICT_OBSOLETE,DICT_CURRENT,DICT_FUTURE,DICT_INCOMPLETE)
   char          *SearchOrigin;                    // Search origin
   int            SearchIP1,SearchIP2,SearchIP3;   // Ip to look for
   TDict_Encoding Encoding;                        // Encoding mode (DICT_ASCII,DICT_UTF8,DICT_ISO8859_1)
   char          *Name,*Date,*Version,String[64];  // Dictionnary metadata
} TDict;

static TDict Dict;

static int Dict_ParseVar(xmlDocPtr Doc,xmlNsPtr NS,xmlNodePtr Node);
static int Dict_ParseType(xmlDocPtr Doc,xmlNsPtr NS,xmlNodePtr Node);

int Dict_Encoding(char *string,TDict_Encoding Encoding) {
   
   char tmpString[256];
   int  tmplenout, tmplenin;
   int  i,i2;
  
   tmplenin = strlen(string);
   tmplenout = tmplenin;
   memset(tmpString,'\0',256);
   
 /* TODO
      <!ENTITY  amp      '&#38;'  >
     <!ENTITY  lt       '&#60;'  >
     <!ENTITY  gt       '&#62;'  >
     <!ENTITY  amp      '&#38;'  >
     <!ENTITY  quot     '&#34;'  >
     <!ENTITY  nbsp     '&#160;' >
   */
 
   switch (Encoding) {
     
      case DICT_ASCII:
         i= i2=0;
         while (i < tmplenout && i2 < 255) {
            
            switch ((unsigned char)string[i]) {        
               case 0xC2:
                  i++;
                  switch ((unsigned char) string[i]) {
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
                     
                     case 0xF9:
                     case 0xFA:
                     case 0xFB:
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
void Dict_SetSearch(int SearchMode,int SearchState,char *SearchOrigin,int SearchIP1,int SearchIP2,int SearchIP3) { 
   Dict.SearchState=SearchState;
   Dict.SearchMode=SearchMode;
   Dict.SearchOrigin=SearchOrigin;
   Dict.SearchIP1=SearchIP1;
   Dict.SearchIP2=SearchIP2;
   Dict.SearchIP3=SearchIP3;
}

void Dict_SetEncoding(TDict_Encoding Encoding)           { Dict.Encoding=Encoding; }

/*----------------------------------------------------------------------------
 * Nom      : <Dict_Parse>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Read and parse an XML dictionnary file.
 *
 * Parametres  :
 *  <Filename> : XML dictionnary file
 *
 * Retour:
 *
 * Remarques :
 *    - Based heavily on r.dict code
 *----------------------------------------------------------------------------
*/
int Dict_Parse(char *Filename) {
   
   xmlDocPtr    doc;
   xmlNsPtr     ns;
   xmlNodePtr   node;
   xmlDtdPtr    dtd ;
   xmlValidCtxt ctxt;
   char        *c,dtdfile[256];
   
   xmlDoValidityCheckingDefaultValue=1;
   LIBXML_TEST_VERSION
//   xmlKeepBlanksDefault(0);
 
   // Build the XML tree from the file
   if (!(doc=xmlParseFile(Filename))) {
      fprintf(stderr,"(ERROR) Invalid dictionnary file: %s\n",Filename);
      return(0);
   }

   // Check the document is of the right kind
   if (!(node = xmlDocGetRootElement(doc))) {
      fprintf(stderr,"(ERROR) Empty document\n");
      xmlFreeDoc(doc);
      return(0);
   }

   Dict.Name=strdup(node->name);
   Dict.Date=strdup(xmlGetProp(node,"date"));
   Dict.Version=strdup(xmlGetProp(node,"version_number"));

   sprintf(Dict.String,"%s %s version %s",Dict.Name,Dict.Date,Dict.Version);
//   Dict.Encoding=XML_CHAR_ENCODING_ASCII;
   
  // Parse the DTD
//   strcpy(dtdfile,"");
//   strcat(dtdfile,getenv("AFSISIO"));
//   strcat(dtdfile,"/datafiles/constants/dict.dtd");
    
//   if (!(dtd=xmlParseDTD(NULL,dtdfile))) {
//      fprintf(stderr,"(ERROR) Could not parse DTD %s\n",dtdfile);
//      return (1);
//   }

   // Set up xmlValidCtxt for error reporting when validating 
   ctxt.userData = stderr; 
   ctxt.error    = (xmlValidityErrorFunc) fprintf;   /* register error function */ 
   ctxt.warning  = (xmlValidityWarningFunc) fprintf; /* register warning function */ 

//   if (!xmlValidateDtd(&ctxt,doc,dtd)) {
//      fprintf(stderr,"(ERROR) DTD validation error\n");
//      return (0);
//   }
 
   node=node->children;
  
   // Now, walk the tree
   while (node) { 

      if (!strcmp(node->name,"metvar")) {
         if (!Dict_ParseVar(doc,ns,node)) break;
      } 
       
      if (!strcmp(node->name,"typvar")) {
         if (!Dict_ParseType(doc,ns,node)) break;
      } 
 
      node=node->next;
   }

    xmlFreeDtd(dtd);
    xmlFreeDoc(doc);

    return(1);
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
 *
 * Retour:
 *
 * Remarques :
 *    - Based heavily on r.dict code
 *----------------------------------------------------------------------------
*/
static int Dict_ParseVar(xmlDocPtr Doc,xmlNsPtr NS,xmlNodePtr Node) {
   
   TDictVar  *metvar;
   xmlNodePtr trotteur,trotteur1;
   xmlChar   *tmpc;
   int        i,y,m,d;
 
   metvar=(TDictVar*)calloc(1,sizeof(TDictVar));
   metvar->IP1=metvar->IP2=metvar->IP3=-1;
   metvar->Min=metvar->Max=metvar->Magnitude=DICT_NOTSET;
   
   if ((tmpc=(char*)xmlGetProp(Node,"origin"))) {
      strncpy(metvar->Origin,tmpc,32);
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
      } else 

      if (!strcmp((char*)Node->name,"description")) {
         trotteur1=Node->children;
         while (trotteur1) {
            if (!strcmp((char*)trotteur1->name,"short")) {
               if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"en")) {
                  strncpy(metvar->Short[1], xmlNodeListGetString(Doc,trotteur1->children,1),128);
                  Dict_Encoding(metvar->Short[1],Dict.Encoding);
               } else {
                 if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"fr")) {
                    strncpy(metvar->Short[0], xmlNodeListGetString(Doc,trotteur1->children,1),128);
                     Dict_Encoding(metvar->Short[0],Dict.Encoding);
                 } 
               }           
            } else 

            if (!strcmp((char*)trotteur1->name,"long") && xmlNodeListGetString(Doc,trotteur1->children,1)) {
               strncpy(metvar->Long[0],xmlNodeListGetString(Doc,trotteur1->children,1),128);
               strncpy(metvar->Long[1],xmlNodeListGetString(Doc,trotteur1->children,1),128);
               Dict_Encoding(metvar->Long[0],Dict.Encoding);
               Dict_Encoding(metvar->Long[1],Dict.Encoding);
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
               if (!strcmp((char*)trotteur1->name,"units")) {
                  strncpy(metvar->Units, xmlNodeListGetString(Doc,trotteur1->children,1),32);   
                  Dict_Encoding(metvar->Units,Dict.Encoding);
               }            

               if (!strcmp((char*)trotteur1->name,"magnitude")) { 
                  if ((tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                     metvar->Magnitude=atof(tmpc);
                  } 
               }

               if (!strcmp((char*)trotteur1->name,"min")) { 
                  if ((tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                     metvar->Min=atof(tmpc);
                  }
               }  

               if (!strcmp((char*)trotteur1->name,"max")) { 
                  if ((tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                     metvar->Max=atof(tmpc);
                  } 
               }

               trotteur1=trotteur1->next;
            }
         } else 

         // Real
         if (!strcmp((char*)trotteur->name,"real")) {
            metvar->Nature|=DICT_REAL;
            
            trotteur1=trotteur->children;
            while (trotteur1) {
               if (!strcmp((char*)trotteur1->name,"units")) {
                  if ((tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {   
                     strncpy(metvar->Units,tmpc,32);
                     Dict_Encoding(metvar->Units,Dict.Encoding);
                  }
               }

               if (!strcmp((char*)trotteur1->name,"magnitude")) { 
                  if ((tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                     metvar->Magnitude=atof(tmpc);
                  } 
               }

               if (!strcmp((char*)trotteur1->name,"min")) {             
                  if ((tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                     metvar->Min=atof(tmpc);
                  }
               } 

               if (!strcmp((char*)trotteur1->name,"max")) {         
                  if ((tmpc=xmlNodeListGetString(Doc,trotteur1->children,1))) {
                     metvar->Max=atof(tmpc);
                  } 
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
               if (!strcmp((char*)trotteur1->name,"value")) {
                  tmpc=xmlNodeListGetString(Doc,trotteur1->children,1);   
                  metvar->Codes[i]=atoi(tmpc);
               }
               if (!strcmp((char*)trotteur1->name,"meaning")) {
                  strncpy(metvar->Meanings[i], (xmlChar *) xmlNodeListGetString(Doc,trotteur1->children,1),64);                      
                  Dict_Encoding(metvar->Meanings[i],Dict.Encoding);
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
//   Dict.Vars=TList_Add(Dict.Vars,metvar);
   
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
 *
 * Retour:
 *
 * Remarques :
 *    - Based heavily on r.dict code
 *----------------------------------------------------------------------------
*/
static int Dict_ParseType(xmlDocPtr Doc, xmlNsPtr NS, xmlNodePtr Node) {

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
            if (!strcmp((char*)trotteur1->name,"short")) {
               if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"en")) {
                  strncpy(type->Short[1],xmlNodeListGetString(Doc,trotteur1->children,1),128);
                  Dict_Encoding(type->Short[1],Dict.Encoding);
               } else {
                  if (!strcmp((char*)xmlGetProp(trotteur1,"lang"),"fr")) {
                     strncpy(type->Short[0],xmlNodeListGetString(Doc,trotteur1->children,1),128);
                     Dict_Encoding(type->Short[0],Dict.Encoding);
                  }           
               }          
            }

            if (!strcmp((char*)trotteur1->name,"long") && xmlNodeListGetString(Doc,trotteur1->children,1)) {
               strncpy(type->Long[0],xmlNodeListGetString(Doc,trotteur1->children,1),128);
               strncpy(type->Long[1],xmlNodeListGetString(Doc,trotteur1->children,1),128);
               Dict_Encoding(type->Long[0],Dict.Encoding);
               Dict_Encoding(type->Long[1],Dict.Encoding);
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
   
   if (Dict.SearchState && !(((TDictVar*)Data0)->Nature&Dict.SearchState)) {
      return(0);
   }
   
   if (Dict.SearchOrigin && strcasecmp(((TDictVar*)Data0)->Origin,Dict.SearchOrigin)) {
      return(0);
   }
   
   if (Dict.SearchIP1>=0 && (((TDictVar*)Data0)->IP1!=Dict.SearchIP1)) {
      return(0);
   }
   
   if (Dict.SearchIP2>=0 && (((TDictVar*)Data0)->IP2!=Dict.SearchIP2)) {
      return(0);
   }
   
   if (Dict.SearchIP3>=0 && (((TDictVar*)Data0)->IP3!=Dict.SearchIP3)) {
      return(0);
   }
   
   if (Dict.SearchMode==DICT_GLOB) {
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

   if (Dict.SearchState && !(((TDictType*)Data0)->Nature&Dict.SearchState)) {
      return(0);
   }
   
   if (Dict.SearchOrigin && strcasecmp(((TDictType*)Data0)->Origin,Dict.SearchOrigin)) {
      return(0);
   }
   
   if (Dict.SearchMode==DICT_GLOB) {
      return(Data1?(!strmatch(((TDictType*)Data0)->Name,(char*)Data1)):1);
   } else {
      return(Data1?(!strcasecmp(((TDictType*)Data0)->Name,(char*)Data1)):0);
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
 * Parametres  :
 *  <Var>      : Variable to look fortement
 *  <Format>   : Print format (DICT_SHORT,DICT_LONG)
 *  <Language> : Language (Fr,En)
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_PrintVars(char *Var,int Format,char *Language) {
   
   TList *list;
   
   list=Dict.Vars;

   while(list=TList_Find(list,Dict_CheckVar,Var)) {
       Dict_PrintVar((TDictVar*)list->Data,Format,Language);
       list=list->Next;
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_PrintVar>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Print info about a variable
 *
 * Parametres  :
 *  <Var>      : Variable to look fortement
 *  <Format>   : Print format (DICT_SHORT,DICT_LONG)
 *  <Language> : Language (Fr,En)
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_PrintVar(TDictVar *DVar,int Format,char *Language) {
 
   int  i,lang;
   struct tm *tm;
   char *tshort[] = { "Description courte","Short Description " };
   char *tlong[]  = { "Description longue","Long  Description " };
   char *unites[] = { "Unites            ","Units             " };
   char *tdate[]  = { "Date              ","Date              " };
   char *tori[]   = { "Origine           ","Origin            " };
   char *tuse[]   = { "Etat              ","State             " };
   char *mag[]    = { "Ordre de grandeur ","Magnitude         " };
   char *range[]  = { "Amplitude         ","Range             " };
   char *tint[]   = { "Variable entière"  ,"Integer Variable"   };
   char *treal[]  = { "Variable réelle"   ,"Real Variable"      };
   char *tlogic[] = { "Variable logique"  ,"Logical Variable"   };
   char *tcode[]  = { "Variable codée"    ,"Coded Variable"     };
   char *tval[]   = { "Valeur"            ,"Value"              };
   
   char *tobsolete[]   = { "Obsolète"   ,"Obsolete"   };
   char *tfuture[]     = { "Futur"      ,"Future"     };
   char *tcurrent[]    = { "Courante"   ,"Current"    };
   char *tincomplete[] = { "Incomplète" ,"Icompletee" };
 
   if (DVar) {
      lang=(Language[0]=='f' || Language[0]=='F')?0:1;
      
      if (DVar->Nature&DICT_OBSOLETE) printf(APP_COLOR_RED);
      
      switch(Format) {
         case DICT_SHORT:
            printf("%-4s\t%-70s\t%-s",DVar->Name,DVar->Short[lang],DVar->Units);
            if (DVar->Nature&DICT_OBSOLETE)
               printf(" \t%s\n",tobsolete[lang]);
            else 
               printf("\n");
            
            break;

         case DICT_LONG:
            printf("--------------------------------------------------------------------------------\n");
            printf("Nomvar             : %-s", DVar->Name);
            if (DVar->IP1>=0) printf(" IP1(%i)",DVar->IP1);
            if (DVar->IP2>=0) printf(" IP2(%i)",DVar->IP2);
            if (DVar->IP3>=0) printf(" IP3(%i)",DVar->IP3);
               
            printf("\n%-s : %-s\n", tshort[lang],DVar->Short[lang]);
            printf("%-s : %-s\n", tlong[lang],DVar->Long[lang][0]!='\0'?DVar->Long[lang]:"-");
            
            if (DVar->Date) {
               tm=gmtime(&(DVar->Date));
               printf("%-s : %04i-%02i-%02i\n", tdate[lang],1900+tm->tm_year,tm->tm_mon+1,tm->tm_mday);
            } else {
               printf("%-s : %-s\n", tdate[lang],"-");
            }
            
            printf("%-s : %-s\n", tori[lang],DVar->Origin[0]!='\0'?DVar->Origin:"-");
            printf("%-s : ", tuse[lang]);
            
            if (DVar->Nature&DICT_OBSOLETE)
               printf("%s\n",tobsolete[lang]);
            else if (DVar->Nature&DICT_CURRENT)
               printf("%s\n",tcurrent[lang]);
            else if (DVar->Nature&DICT_FUTURE)
               printf("%s\n",tfuture[lang]);
             else if (DVar->Nature&DICT_INCOMPLETE)
               printf("%s\n",tincomplete[lang]);
                
            if (DVar->Nature & DICT_INTEGER) {
                  printf("Representation     : %-s\n",tint[lang]);
                  printf("%-s : %s\n",unites[lang],DVar->Units);
                  if (DVar->Magnitude!=DICT_NOTSET)     printf("%-s : %e\n",mag[lang],DVar->Magnitude);
                  if (DVar->Min!=DVar->Max) {
                     printf("%-s : ",range[lang]);
                     if (DVar->Min!=DICT_NOTSET) printf("[%.0f ",DVar->Min);
                     if (DVar->Max!=DICT_NOTSET) printf("%.0f]\n",DVar->Max);
                     printf("\n");
                  }
                  break;
                  
            } else if (DVar->Nature & DICT_REAL) {
                  printf("Representation     : %-s\n", treal[lang]);
                  printf("%-s : %s\n",unites[lang],DVar->Units);
                  if (DVar->Magnitude!=DICT_NOTSET)    printf("%-s : %e\n",mag[lang],DVar->Magnitude);
                  if (DVar->Min!=DVar->Max) {
                     printf("%-s : ",range[lang]);
                     if (DVar->Min!=DICT_NOTSET) printf("[%.0f ",DVar->Min);
                     if (DVar->Max!=DICT_NOTSET) printf("%.0f]\n",DVar->Max);
                     printf("\n");
                  }
                  break;
            
            } else if (DVar->Nature & DICT_LOGICAL) {
                  printf("Representation     : %-s\n", tlogic[lang]);
                  break;
            
            } else if (DVar->Nature & DICT_CODE) {
                  printf("Representation     : %-s\n", tcode[lang]);
                  printf("\tCode\t\t%s\n",tval[lang]);
                  printf("\t----\t\t----------------\n");
                  for (i=0; i < DVar->NCodes; i++) {
                     printf("\t%i\t\t%-s\n", DVar->Codes[i], DVar->Meanings[i]);
                  }
                  break;
            }
            break;
      }  
      if (DVar->Nature&DICT_OBSOLETE) printf(APP_COLOR_RESET);
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_PrintTypes>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Print info about a list of Types
 *
 * Parametres  :
 *  <Var>      : Variable to look fortement
 *  <Format>   : Print format (DICT_SHORT,DICT_LONG)
 *  <Language> : Language (Fr,En)
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_PrintTypes(char *Type,int Format,char *Language) {
   
   TList *list;
   
   list=Dict.Types;
   
   printf("---TYPVAR-----------------------------------------------------------------------\n");
   
   while(list=TList_Find(list,Dict_CheckType,Type)) {
       Dict_PrintType((TDictType*)list->Data,Format,Language);
       list=list->Next;
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Dict_PrintType>
 * Creation : Mai 2014 - J.P. Gauthier
 *
 * But      : Print info about a type
 *
 * Parametres  :
 *  <Var>      : Variable to look fortement
 *  <Format>   : Print format (DICT_SHORT,DICT_LONG)
 *  <Language> : Language (Fr,En)
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void Dict_PrintType(TDictType *DType,int Format,char *Language) {
 
   int  i,lang;
   
   if (DType) {
      lang=(Language[0]=='f' || Language[0]=='F')?0:1;
      
      switch(Format) {
         case DICT_SHORT:
            printf("%s  %-60s\n", DType->Name, DType->Short[lang]);
            break;

      }  
   }
}
