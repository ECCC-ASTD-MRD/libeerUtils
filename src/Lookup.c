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

#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stdio.h>

#include "App.h"
#include "Lookup.h"

const TLookup LookupNULL = DLookupNULL;

/*----------------------------------------------------------------------------
 * Nom      : <Lookup_PLU_GetIdxf>
 * Creation : Juillet 2024 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Search for the given value using the polynomial lookup, returning
 *            the index so that the value is contained in [idx,idx+1[.
 *            Note that the returned index will be -1 or NbVals-1 if
 *            outside of the covered items.
 *
 * Parametres  :
 *  <LU>       : The lookup structure
 *  <Val>      : Value to get the index of
 *
 * Retour:     The index so that the given value is contained in [idx,idx+1[, or -1
 *             or NbVals-1 if outside the array one way or the other
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static int Lookup_PLU_GetIdxf(TLookup *restrict LU,float Val) {
   double *f = LU->LData.PLU.F;
   float *vals=LU->Vals;
   int i,imax=LU->NV-1;

   // Calculate the lookup index
   double v = Val;
   switch( LU->LData.PLU.N ) {
      case 1:  i = (int)round(f[0] + f[1]*v);            break;
      case 2:  i = (int)round(f[0] + f[1]*v + f[2]*v*v); break;
      default: return -2;
   }

   if( i <= 0 ) {
      // Make some adjustments because the lookup polynomial is not exact but is garantied to be within one index
      return Val<vals[0] ? -1 : 0;
   } else if( i >= imax ) {
      // Make some adjustments because the lookup polynomial is not exact but is garantied to be within one index
      return Val>=vals[imax] ? imax : imax-1;
   }

   // Make some adjustments because the lookup polynomial is not exact but is garantied to be within one index,
   // so adjusting by one index one way or the other should always give the right index
   if( LU->Asc ) {
      i += (i<imax&&Val>=vals[i+1]) - (i>0&&Val<vals[i]);
   } else {
      i += (i<imax&&Val<=vals[i+1]) - (i>0&&Val>vals[i]);
   }

   return i;
}


/*----------------------------------------------------------------------------
 * Nom      : <Lookup_PLU_Init1f>
 * Creation : Juillet 2024 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Initialize a 1st-order polynomial lookup (PLU)
 *
 * Parametres  :
 *  <LU>       : The lookup structure to initialize
 *  <Vals>     : Sorted list of unique values to build the PLU for
 *  <N>        : Number of values
 *
 * Retour:     APP_OK if ok, APP_ERR otherwise
 *
 * Remarques : This function needs at least 2 values. The values MUST be unique and sorted.
 *
 *----------------------------------------------------------------------------
 */
static int Lookup_PLU_Init1f(TLookup *restrict LU,float *restrict Vals,int N) {
   TPLU *plu = &LU->LData.PLU;
   double v,vx,sx[3],syx[2],det,inv[3];
   int i;

   if( N < 2 ) {
      return APP_ERR;
   }

   // Uses the partial sum to calculate the sum of all indices
   // Note: the formula is n(n+1)/2, but since indices start at 0, we use (N-1)N/2
   syx[0] = ((N-1)*N)/2;
   sx[0] = N;

   // Calculate the needed values for the 1st-order polynomial regression by calculating the result of
   //    ┌         ┐┌    ┐   ┌      ┐
   //    │ Σx⁰ Σx¹ ││ F₀ │   │ Σyx⁰ │
   //    │ Σx¹ Σx² ││ F₁ │ = │ Σyx¹ │
   //    └         ┘└    ┘   └      ┘
   // Where x are the values (Vals), y are the indexes and F are the polynomial factors
   for(i=0; i<N; ++i) {
      v = Vals[i];

      vx = v;  sx[1] += vx; syx[1] += i*vx;
      vx *= v; sx[2] += vx;
   }

   // Calculate the determinant of the matrix
   //    ┌         ┐
   //    │ sx0 sx1 │
   //    │ sx1 sx2 │
   //    └         ┘
   det = sx[0]*sx[2] - sx[1]*sx[1];

   // Calculate the inverse of the matrix using it's adjugate
   // Note: since the original matrix is symmetric, so is this one, therefore we can skip calculations for 1 value)
   det = 1.0/det;
   inv[0] = sx[2] * det;
   inv[1] = -sx[1] * det;
   inv[2] = sx[0] * det;


   // Calculate the factors
   // Note that since the inverse matrix is packed, the indices are as follows:
   //    ┌    ┐   ┌               ┐┌      ┐
   //    │ F₀ │   │ inv[0] inv[1] ││ Σyx⁰ │
   //    │ F₁ │ = │ inv[1] inv[2] ││ Σyx¹ │
   //    └    ┘   └               ┘└      ┘
   plu->F[0] = inv[0]*syx[0] + inv[1]*syx[1];
   plu->F[1] = inv[1]*syx[0] + inv[2]*syx[1];

   // Make sure none of the residuals are over 1 (to guaranty that we'll never be further that 1 index either direction)
   for(i=0; i<N; ++i) {
      v = Vals[i];
      if( fabs(i - (plu->F[0] + plu->F[1]*v)) >= 1.0 ) {
         return APP_ERR;
      }
   }

   // Mark as first order
   plu->N = 1;

   LU->GetIdx  = &Lookup_PLU_GetIdxf;
   LU->Free    = NULL;

   return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <Lookup_PLU_Init2f>
 * Creation : Juillet 2024 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Initialize a 2nd-order polynomial lookup (PLU)
 *
 * Parametres  :
 *  <LU>       : The lookup structure to initialize
 *  <Vals>     : Sorted list of unique values to build the PLU for
 *  <N>        : Number of values
 *
 * Retour:     APP_OK if ok, APP_ERR otherwise
 *
 * Remarques : This function needs at least 3 values. The values MUST be unique and sorted.
 *
 *----------------------------------------------------------------------------
 */
static int Lookup_PLU_Init2f(TLookup *restrict LU,float *restrict Vals,int N) {
   TPLU *plu = &LU->LData.PLU;
   double v,vx,sx[5],syx[3],det,inv[6];
   int i;

   if( N < 3 ) {
      return APP_ERR;
   }

   // Uses the partial sum to calculate the sum of all indices
   // Note: the formula is n(n+1)/2, but since indices start at 0, we use (N-1)N/2
   syx[0] = ((N-1)*N)/2;
   sx[0] = N;

   // Calculate the needed values for the 2nd-order polynomial regression by calculating the result of
   //    ┌             ┐┌    ┐   ┌      ┐
   //    │ Σx⁰ Σx¹ Σx² ││ F₀ │   │ Σyx⁰ │
   //    │ Σx¹ Σx² Σx³ ││ F₁ │ = │ Σyx¹ │
   //    │ Σx² Σx³ Σx⁴ ││ F₂ │   │ Σyx² │
   //    └             ┘└    ┘   └      ┘
   // Where x are the values (Vals), y are the indexes and F are the polynomial factors
   for(i=0; i<N; ++i) {
      v = Vals[i];

      vx = v;  sx[1] += vx; syx[1] += i*vx;
      vx *= v; sx[2] += vx; syx[2] += i*vx;
      vx *= v; sx[3] += vx;
      vx *= v; sx[4] += vx;
   }

   // Calculate the determinant of the matrix
   //    ┌             ┐
   //    │ sx0 sx1 sx2 │
   //    │ sx1 sx2 sx3 │
   //    │ sx2 sx3 sx4 │
   //    └             ┘
   det = sx[0]*(sx[2]*sx[4] - sx[3]*sx[3]) - sx[2]*(sx[2]*sx[4] - sx[2]*sx[3]) + sx[2]*(sx[2]*sx[3] - sx[2]*sx[2]);

   // Calculate the inverse of the matrix using it's adjugate
   // Note: since the original matrix is symmetric, so is this one, therefore we can skip calculations for 3 values)
   det = 1.0/det;
   inv[0] = ( sx[2]*sx[4] - sx[3]*sx[3] ) * det;
   inv[1] = ( sx[2]*sx[3] - sx[1]*sx[4] ) * det;
   inv[2] = ( sx[1]*sx[3] - sx[2]*sx[2] ) * det;
   inv[3] = ( sx[0]*sx[4] - sx[2]*sx[2] ) * det;
   inv[4] = ( sx[2]*sx[1] - sx[0]*sx[3] ) * det;
   inv[5] = ( sx[0]*sx[2] - sx[1]*sx[1] ) * det;


   // Calculate the factors
   // Note that since the inverse matrix is packed, the indices are as follows:
   //    ┌    ┐   ┌                      ┐┌      ┐
   //    │ F₀ │   │ inv[0] inv[1] inv[2] ││ Σyx⁰ │
   //    │ F₁ │ = │ inv[1] inv[3] inv[4] ││ Σyx¹ │
   //    │ F₂ │   │ inv[2] inv[4] inv[5] ││ Σyx² │
   //    └    ┘   └                      ┘└      ┘
   plu->F[0] = inv[0]*syx[0] + inv[1]*syx[1] + inv[2]*syx[2];
   plu->F[1] = inv[1]*syx[0] + inv[3]*syx[1] + inv[4]*syx[2];
   plu->F[2] = inv[2]*syx[0] + inv[4]*syx[1] + inv[5]*syx[2];

   // Make sure none of the residuals are over 1 (to guaranty that we'll never be further that 1 index either direction)
   for(i=0; i<N; ++i) {
      v = Vals[i];
      if( fabs(i - (plu->F[0] + plu->F[1]*v + plu->F[2]*v*v)) >= 1.0 ) {
         return APP_ERR;
      }
   }

   // Mark as second order
   plu->N = 2;

   LU->GetIdx  = &Lookup_PLU_GetIdxf;
   LU->Free    = NULL;

   return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <Lookup_LUT_GetIdxf>
 * Creation : Juillet 2024 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Search for the given value in the LUT returning the index so
 *            that the value is contained in [idx,idx+1[.
 *            Note that the returned index will be -1 or NbVals-1 if
 *            outside of the covered items.
 *
 * Parametres  :
 *  <LU>       : The lookup structure
 *  <Val>      : Value to get the index of
 *
 * Retour:     The index so that the given value is contained in [idx,idx+1[, or -1
 *             or NbVals-1 if outside the array one way or the other
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static int Lookup_LUT_GetIdxf(TLookup *restrict LU,float Val) {
   TLUT *lut=&LU->LData.LUT;
   float *vals=LU->Vals,lui;
   int i,imax=LU->NV-1;

   // Calculate the lookup index
   lui = (Val-vals[0])*lut->Fac;

   // Before the first level
   if( lui <= 0.0f ) {
      return lui==0.0f ? 0 : -1;
   }

   // Past (or equal to) the last level of lookup
   i = (int)lui;
   if( i >= lut->NL ) {
      return imax;
   }

   // Get the level from the LUT
   i = lut->Table[i];

   // Make some adjustments because the lookup table is not exact but is garantied to be within one index,
   // so adjusting by one index one way or the other should always give the right index
   if( LU->Asc ) {
      i += (i<imax&&Val>=vals[i+1]) - (i&&Val<vals[i]);
   } else {
      i += (i<imax&&Val<=vals[i+1]) - (i&&Val>vals[i]);
   }

   return i;
}


/*----------------------------------------------------------------------------
 * Nom      : <LUT_Free>
 * Creation : Juillet 2024 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Free a lookup table (LUT)
 *
 * Parametres  :
 *  <LUT>      : The lookup table to free
 *
 * Retour:
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void Lookup_LUT_Free(TLookup *LU) {
   if( LU ) {
      if( LU->LData.LUT.Table ) {
         free(LU->LData.LUT.Table);
         LU->LData.LUT.Table = NULL;
      }
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <Lookup_LUT_Initf>
 * Creation : Juillet 2024 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Initialize a lookup table (LUT)
 *
 * Parametres  :
 *  <LU>       : The lookup structure to initialize
 *  <Vals>     : Sorted list of unique values to build the LUT for
 *  <N>        : Number of values
 *
 * Retour:     APP_OK if ok, APP_ERR otherwise
 *
 * Remarques : This function needs at least 2 values. The values MUST be unique and sorted.
 *
 *----------------------------------------------------------------------------
 */
static int Lookup_LUT_Initf(TLookup *restrict LU,float *restrict Vals,int N) {
   TLUT *lut=&LU->LData.LUT;
   float dvmin=FLT_MAX,dv0,val;
   int i,idx;

   if( N < 2 ) {
      return APP_ERR;
   }

   // Get the smallest delta between values that will dictate the minimum gap we need to leave between LUT items
   for(i=1; i<N; ++i) {
      dvmin = fminf(fabsf(Vals[i]-Vals[i-1]),dvmin);
   }

   if( dvmin <= 0.0f ) {
      return APP_ERR;
   }

   // Get the number of LUT levels we need
   lut->NL = (int)(fabsf(Vals[N-1]-Vals[0])/dvmin)+1;

   // Allocate lookup table memory
   if( !(lut->Table=malloc(lut->NL*sizeof(*lut->Table))) ) {
      return APP_ERR;
   }

   // Set the LUT factor
   lut->Fac = 1.0f/dvmin;
   if( Vals[0] > Vals[N-1] )
      lut->Fac = -lut->Fac;

   // Build the lookup table
   dv0 = fabsf(Vals[1]-Vals[0]);
   for(i=0,idx=0; i<lut->NL; ++i) {
      val = i*dvmin;
      if( val >= dv0 ) {
         ++idx;
         // Adjust the next threshold value (or infinite if no more thresholds)
         dv0 = idx<N-1 ? fabsf(Vals[idx+1]-Vals[0]) : FLT_MAX;
      }
      lut->Table[i] = idx;
   }

   LU->GetIdx  = &Lookup_LUT_GetIdxf;
   LU->Free    = &Lookup_LUT_Free;

   return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <Lookup_BiS_GetIdxf>
 * Creation : Juillet 2024 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Make a binary search of the given values, returning the index so
 *            that the level is contained in [idx,idx+1[.
 *            Note that the returned index will be -1 or NbVals-1 if
 *            outside of the covered values.
 *
 * Parametres  :
 *  <LU>       : The lookup structure
 *  <Val>      : Value to get the index of
 *
 * Retour:     The index so that the value is contained in [idx,idx+1[, or -1
 *             or LevelNb-1 if outside the values one way or the other
 *
 * Remarques : This is needs at least 2 values
 *
 *----------------------------------------------------------------------------
 */
static int Lookup_BiS_GetIdxf(TLookup *restrict LU,float Val) {
   float *vals=LU->Vals;
   int idx=0,min=0,max=LU->NV-1;
   const int inv = !LU->Asc;

   // Corner cases
   if( inv&&Val>=vals[0] || !inv&&Val<=vals[0] ) return Val==vals[0]?0:-1;
   if( inv&&Val<=vals[max] || !inv&&Val>=vals[max] ) return max;

   // Binary search
   while( min <= max ) {
      idx = (max+min)>>1;

      if( inv&&Val>vals[idx] || !inv&&Val<vals[idx] ) {
         max = idx-1;
      } else if( inv&&Val<vals[idx] || !inv&&Val>vals[idx] ) {
         min = idx+1;
      } else {
         return idx;
      }
   }

   return max;
}

/*----------------------------------------------------------------------------
 * Nom      : <Lookup_BiS_Initf>
 * Creation : Juillet 2024 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Initialize a binary search (BiS) lookup
 *
 * Parametres  :
 *  <LU>       : The lookup structure to initialize
 *  <Vals>     : Sorted list of unique values to build the lookup for
 *  <N>        : Number of values
 *
 * Retour:     APP_OK if ok, APP_ERR otherwise
 *
 * Remarques : This function needs at least 2 values. The values MUST be unique and sorted.
 *
 *----------------------------------------------------------------------------
 */
static int Lookup_BiS_Initf(TLookup *restrict LU,float *restrict Vals,int N) {
   LU->GetIdx  = &Lookup_BiS_GetIdxf;
   LU->Free    = NULL;

   return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <Lookup_Init1Df>
 * Creation : Juillet 2024 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Initialize a 1D lookup on the given array
 *
 * Parametres  :
 *  <LU>       : The lookup structure to initialize
 *  <Vals>     : Sorted list of unique values to build the lookup for
 *  <N>        : Number of values
 *
 * Retour:     APP_OK if ok, APP_ERR otherwise
 *
 * Remarques : This function needs at least 2 values. The values MUST be unique and sorted.
 *
 *----------------------------------------------------------------------------
 */
int Lookup_Init1Df(TLookup *restrict LU,float *restrict Vals,int N) {
   if( Lookup_PLU_Init1f(LU,Vals,N)!=APP_OK
         && Lookup_PLU_Init2f(LU,Vals,N)!=APP_OK
         && Lookup_LUT_Initf(LU,Vals,N)!=APP_OK
         && Lookup_BiS_Initf(LU,Vals,N)!=APP_OK
         ) {
      *LU = LookupNULL;
      return APP_ERR;
   }

   LU->Vals   = Vals;
   LU->NV     = N;
   LU->Asc    = Vals[0]<Vals[1];

   return APP_OK;
}
