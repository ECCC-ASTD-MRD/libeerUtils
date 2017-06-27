/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairies de fonctions utiles
 * Fichier      : ZRef.h
 * Creation     : Octobre 2011 - J.P. Gauthier
 *
 * Description  : Fonctions relectives a la coordonnee verticale.
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
 * Modification :
 *
 *    Nom         :
 *    Date        :
 *    Description :
 *
 *=========================================================
 */

#ifndef _TimeRef_h
#define _TimeRef_h

#include <sys/time.h>

// Time referential definition
typedef struct TTimeRef {
   char  *Name;          // Reference name
   int    RCount;        // Reference count
   time_t Reference;     // Reference time
   time_t Origin;        // Origin time
   time_t Step;          // Time step
   time_t *Times;        // Validity time available
   int     NT;           // Number of validity times
#ifdef HAVE_RPNC   
   int NC_Id,NC_NTDimId; // netCDF identifiers
#endif
} TTimeRef;

#endif