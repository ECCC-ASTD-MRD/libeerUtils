/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairies de fonctions utiles
 * Fichier      : Lookup.h
 * Creation     : Juillet 2024 - E. Legault-Ouellet
 * Description  : Fonctions de gestion de lookup
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
 *=========================================================
 */

#ifndef _Lookup_h
#define _Lookup_h

typedef struct TPLU {
   double   F[3];    // Polynomial factors ([0] + [1]x + [2]x²)
   int      N;       // Order (dimension) of the polynomial (1 or 2)
} TPLU;

typedef struct TLUT {
   int   *Table;     // Lookup table
   int   NL;         // Size of the lookup table and of the value array
   float Fac;        // Lookup table factor
} TLUT;

typedef struct TLookup TLookup;

typedef int (*TLookupFct_GetIdxf)(TLookup*,float);
typedef void (*TLookupFct_Free)(TLookup*);

typedef struct TLookup {
   TLookupFct_GetIdxf   GetIdx; // Function to get the index
   TLookupFct_Free      Free;

   void  *Vals;      // Values on which to build the lookup (just a reference, not owned)
   int   NV;         // Number of values
   int   Asc;        // Whether the values are in ascending or descending order

   // Type-specific data
   union {
      TPLU PLU;
      TLUT LUT;
   } LData;
} TLookup;

extern TLookup LookupNULL;

int Lookup_Init1Df(TLookup *LU,float *Vals,int N);

#endif // _Lookup_h
