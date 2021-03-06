/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Projection diverses de la carte vectorielle.
 * Fichier      : List.c
 * Creation     : Janvier 2011 - J.P. Gauthier
 *
 * Description: Fonctions de manipulations de listes
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

#include <malloc.h>
#include "List.h"

/*----------------------------------------------------------------------------
 * Nom      : <TList_Add>
 * Creation : Aout 1998 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Ajout d'un noeud a une liste doublement chainee
 *
 * Parametres :
 *  <List>    : List a laquelle ajouter un noeud.
 *  <Data>    : Pointeur sur la donnee associe au noeud
 *
 * Retour     :
 *  <TList*>  : Pointeur sur la tete de la liste (nouveau noeud)
 *
 * Remarques :
 *   - La liste est doublement chainee
 *   - Les ajouts de font en tete de liste ce qui permet de l'utiliser comme
 *     une queue
 *----------------------------------------------------------------------------
*/
TList* TList_Add(TList *List,void *Data) {

   TList *node=(TList*)malloc(sizeof(TList));

   if (node) {
      node->Next=List;
      node->Prev=NULL;
      node->Data=Data;

      if (List)
         List->Prev=node;
   }
   
   return(node);
}

/*----------------------------------------------------------------------------
 * Nom      : <TList_AddSorted>
 * Creation : Mai 2014 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Ajout d'un noeud a une liste doublement chainee de maniere ordonnee
 *
 * Parametres :
 *  <List>    : List a laquelle ajouter un noeud.
 *  <Proc>    : Procedure de comparaison
 *  <Data>    : Pointeur sur la donnee associe au noeud
 *
 * Retour     :
 *  <TList*>  : Pointeur sur la tete de la liste (nouveau noeud)
 *
 * Remarques :
 *   - La liste est doublement chainee
 *   - Les ajouts se font selon l'ordre obtenue par la fonction Proc
 *----------------------------------------------------------------------------
*/
TList* TList_AddSorted(TList *List,TList_CompareProc *Proc,void *Data) {

   TList *insert,*prev=NULL,*node=(TList*)calloc(1,sizeof(TList));

   if (node) {
      node->Data=Data;      
      
      insert=List;
      
      while (insert) {
         if (Proc(insert->Data,Data)>=0) {
            node->Next=insert;
            node->Prev=insert->Prev;
            if (insert->Prev) {
               insert->Prev->Next=node;
            } else {
               List=node;               
            }  
            insert->Prev=node;
            break;
         }
         prev=insert;
         insert=insert->Next;
      }

      if (!insert) {
         if (prev) {
            prev->Next=node;
            node->Prev=prev;           
         } else {
            List=node;
         }
      }
   }
   
   return(List);
}

/*----------------------------------------------------------------------------
 * Nom      : <TList_Del>
 * Creation : Aout 1998 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Supprimer un noeud d'une liste doublement chainee
 *
 * Parametres :
 *  <List>    : List a laquelle enlever le noeud.
 *  <Data>    : Pointeur sur la donnee du noeud a supprimer
 *
 * Retour     :
 *  <TList*>  : Pointeur sur la tete de la liste
 *
 * Remarques :
 *   - On recherche le noeud ayant la donnee specifie et on le suprimme
 *----------------------------------------------------------------------------
*/
TList* TList_Del(TList *List,void *Data) {

  TList *node=List;

  while(node) {
      if (node->Data==Data) {
         if (node->Prev)
            node->Prev->Next=node->Next;
         if (node->Next)
            node->Next->Prev=node->Prev;
         if (node==List) {
            List=List->Next;
         }
         free(node);
         
         break;
      }
      node=node->Next;
   }
   return(List);
}

/*----------------------------------------------------------------------------
 * Nom      : <TList_Find>
 * Creation : Aout 1998 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Rechercher un noeud contenant la donnee specifie
 *
 * Parametres :
 *  <List>    : List a laquelle enlever le noeud.
 *  <Proc>    : Procedure de comparaison
 *  <Data>    : Pointeur sur la donnee du noeud a supprimer
 *
 * Retour     :
 *  <TList*>  : Pointeur sur le noeud trouve ou NULL
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
TList* TList_Find(TList *List,TList_CompareProc *Proc,void *Data) {

   while(List) {
      if (!Proc || Proc(List->Data,Data)) {
         return(List);
      }
      List=List->Next;
   }
   return(NULL);
}

/*----------------------------------------------------------------------------
 * Nom      : <TList_Clear>
 * Creation : Aout 1998 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Libere une liste
 *
 * Parametres :
 *  <List>    : List a laquelle enlever le noeud.
 *  <Proc>    : Procedure de liberation de memoire des donnees associe a un noeud
 *
 * Retour     :
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void TList_Clear(TList *List,TList_FreeProc *Proc) {

   TList *tmp;

   while(List) {
      tmp=List;
      List=List->Next;
      if (Proc)
         Proc(tmp->Data);
      free(tmp);
   }
}
