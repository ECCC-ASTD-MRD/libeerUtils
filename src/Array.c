/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Projection diverses de la carte vectorielle.
 * Fichier   : Array.c
 * Creation  : Janvier 2011 - J.P. Gauthier
 *
 * Description: Fonctions de manipulation de tableau dynamiques
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
#include "Array.h"

/*----------------------------------------------------------------------------
 * Nom      : <T3DArray_Alloc>
 * Creation : Aout 1998 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Allocation d'un tableau de coordonnee 3D pour une valeur.
 *
 * Parametres :
 *  <Value>   : Valeur de reference.
 *  <Size>    : Dimension du tableau (Nombre de coordonnee)
 *
 * Retour     :
 *  <TArray*> : Pointeru su le tableau.
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
T3DArray *T3DArray_Alloc(double Value,unsigned long Size) {

   T3DArray *array=(T3DArray*)malloc(sizeof(T3DArray));

   if (array) {
      array->Data=(Vect3d*)calloc(Size,sizeof(Vect3d));

      if (array->Data) {
         array->Size=Size;
         array->Value=Value;
      } else {
         free(array);
         array=NULL;
      }
   }
   return(array);
}

/*----------------------------------------------------------------------------
 * Nom      : <T3DArray_Free>
 * Creation : Aout 1998 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Liberation d'un tableau de coordonnee 3D.
 *
 * Parametres :
 *  <Array>   : Tableau a liberer.
 *
 * Retour     :
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
*/
void T3DArray_Free(T3DArray *Array) {

   if (Array) {
      if (Array->Data)
         free(Array->Data);
      free(Array);
   }
}
