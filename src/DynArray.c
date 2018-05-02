/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Tableau de mémoire avec extension automatique (dynamic array)
 * Fichier   : DynArray.c
 * Creation  : Mai 2018 - J.P. Gauthier
 *
 * Description: Tableau de mémoire avec extension automatique (dynamic array)
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

#include "DynArray.h"
#include <stdlib.h>

// Default array size in bytes (4096 = 512 doubles)
#define DYNARRAY_DEFAULT_SIZE 4096

/*----------------------------------------------------------------------------
 * Nom      : <DynArray_Check>
 * Creation : Mai 2018 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : S'assure que la taille d'un tableau est suffisante ou aggrandit
 *            le tableau si cela n'est pas le cas
 *
 * Parametres :
 *  <DA>      : Le tableau dynamique à vérifier
 *  <Size>    : La taille minimale que doit avoir le tableau
 *
 * Retour     : 1 si OK, 0 en cas d'erreur
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
static int DynArray_Check(DynArray *restrict DA,size_t Size) {
   if( Size > DA->Size ) {
      size_t size = DA->Size*2>=Size ? DA->Size*2 : Size;
      void *arr = realloc(DA->Arr,size);
      if( !arr )
         return 0;
      DA->Size = size;
      DA->Arr = arr;
   }
   return 1;
}

/*----------------------------------------------------------------------------
 * Nom      : <DynArray_Init>
 * Creation : Mai 2018 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Initialiser un tableau dynamique
 *
 * Parametres :
 *  <DA>      : Le tableau dynamique à initialiser
 *  <Size>    : La taille initiale du tableau, 0 pour taille par défaut
 *
 * Retour     : 1 si OK, 0 en cas d'erreur
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int DynArray_Init(DynArray *restrict DA,size_t Size) {
   DA->N = 0;
   DA->Size = Size?Size:DYNARRAY_DEFAULT_SIZE;
   DA->Arr = malloc(DA->Size);
   return DA->Arr==NULL;
}

/*----------------------------------------------------------------------------
 * Nom      : <DynArray_Free>
 * Creation : Mai 2018 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Libérer un tableau dynamique
 *
 * Parametres :
 *  <DA>      : Le tableau dynamique à libérer
 *
 * Retour     :
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
void DynArray_Free(DynArray *restrict DA) {
   if( DA->Arr )
      free(DA->Arr);
   DA->Arr = NULL;
   DA->Size = 0;
   DA->N = 0;
}

/*----------------------------------------------------------------------------
 * Nom      : <DynArray_Push>
 * Creation : Mai 2018 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Initialiser un tableau dynamique
 *
 * Parametres :
 *  <DA>      : Le tableau dynamique auquel ajouter les bytes
 *  <Bytes>   : Bytes à ajouter
 *  <Size>    : Nombre de bytes à ajouter
 *
 * Retour     : 1 si OK, 0 en cas d'erreur
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int DynArray_Push(DynArray *restrict DA,const void *restrict Bytes,size_t Size) {
   size_t n = DA->N+Size;
   if( !DynArray_Check(DA,n) )
      return 0;
   memcpy((char*)DA->Arr+DA->N,Bytes,Size);
   DA->N = n;
   return 1;
}

/*----------------------------------------------------------------------------
 * Nom      : <DynArray_Push*>
 * Creation : Mai 2018 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Wrapper pour faciliter l'ajout de double, float ou int
 *
 * Parametres :
 *  <DA>      : Le tableau dynamique auquel ajouter la valeur
 *  <Val>     : Valeur à ajouter dans le tableau
 *
 * Retour     : 1 si OK, 0 en cas d'erreur
 *
 * Remarques :
 *----------------------------------------------------------------------------
*/
int DynArray_Pushd(DynArray *restrict DA,double Val) {
   return DynArray_Push(DA,&Val,sizeof(Val));
}

int DynArray_Pushf(DynArray *restrict DA,float Val) {
   return DynArray_Push(DA,&Val,sizeof(Val));
}

int DynArray_Pushi(DynArray *restrict DA,int Val) {
   return DynArray_Push(DA,&Val,sizeof(Val));
}
