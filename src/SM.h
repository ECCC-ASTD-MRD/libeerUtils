/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de fonctions utiles
 * Fichier   : SM.h
 * Creation  : Juillet 2017 - E. Legault-Ouellet
 *
 * Description: Fonctions pour faciliter l'utilisation de mémoire partagée entre
 *              les processus MPI partageant un même noeud de calcul
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
#ifndef _SM_H
#define _SM_H

#include <stddef.h>

#define SM_FREE(Addr) if(Addr) { SM_Free(Addr); Addr=NULL; }

size_t SM_RoundPageSize(size_t Size);
void* SM_Alloc(size_t Size);
void* SM_Calloc(size_t Num,size_t Size);
int SM_Sync(void *Addr,int NodeHeadRank);
void SM_Free(void *Addr);

#endif // _SM_H
