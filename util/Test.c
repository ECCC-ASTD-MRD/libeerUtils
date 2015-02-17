/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie de fonctions utiles
 * Creation     : Janvier 2008
 * Auteur       : Jean-Philippe Gauthier
 * Revision     : $Id$
 *
 * Description: RPN fstd field tiler tester
 *
 * License:
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
#include "EZTile.h"

int main(int argc, char *argv[]) {
  
   TGrid *fld0,*fld1,*fldn;
   float  val,vals[58];
   int    i,j,n=0,type,in,out;

   /*Test*/
   c_fnom(10,"/data/cmoex7/afsr005/tile/glb/2008010712_000","STD+RND+R/O",0);
   c_fstouv(10,"RND");

   /*Read the field (not really)*/
   fld0=EZGrid_Read(10,"GZ","","",-1,12000,-1,1);

//   fld0=EZGrid_Read(10,"GZ","","",-1,12000,-1,1);
//   EZGrid_TileGrid(20,i,j,2,fld0);

   /*Parse some values*/
   fprintf(stderr,"(DEBUG) Some GZ values at IJ :\n");
   EZGrid_GetValue(fld0,200,200,0,0,&val);
   fprintf(stderr,"   Value is %f\n",val);
   EZGrid_GetValue(fld0,201,200,0,0,&val);
   fprintf(stderr,"   Value is %f\n",val);
   EZGrid_GetValue(fld0,195,195,0,0,&val);
   fprintf(stderr,"   Value is %f\n",val);
   EZGrid_GetValue(fld0,600,500,0,0,&val);
   fprintf(stderr,"   Value is %f\n",val);

   /*Get a profile*/
   fprintf(stderr,"(DEBUG) Profile GZ values at IJ:\n   ");
   EZGrid_GetRange(fld0,195,195,0,195,195,57,vals);
   for(j=0;j<=57;j++) {
      printf("%f ",vals[j]);
   }
   printf("\n");

   /*Get the levels*/
   EZGrid_GetLevels(fld0,vals,&type);
   fprintf(stderr,"(DEBUG) Levels of GZ (type=%i):\n   ",type);
   for(j=0;j<=57;j++) {
      printf("%f ",vals[j]);
   }
   printf("\n");

   EZGrid_Free(fld0);

   /*Read the field (not really)*/
   fld0=EZGrid_Read(10,"UU","","",-1,-1,-1,1);
   /*Parse some values*/
   fprintf(stderr,"(DEBUG) Some UU values at IJ:\n");
   EZGrid_GetValue(fld0,200,200,0,0,&val);
   fprintf(stderr,"(DEBUG)   Value is %f\n",val);
   EZGrid_GetValue(fld0,201,200,0,0,&val);
   fprintf(stderr,"(DEBUG)   Value is %f\n",val);
   EZGrid_GetValue(fld0,195,195,0,0,&val);
   fprintf(stderr,"(DEBUG)   Value is %f\n",val);
   EZGrid_GetValue(fld0,600,500,0,0,&val);
   fprintf(stderr,"(DEBUG)   Value is %f\n",val);

   /*Parse a range of values*/
   EZGrid_GetRange(fld0,195,195,0,200,200,0,vals);
   n=0;
   fprintf(stderr,"(DEBUG) Range of UU values at IJ:\n   ");
   for(j=0;j<=5;j++) {
      for(i=0;i<=5;i++) {
         printf("%f ",vals[n++]);
      }
      printf("\n   ");
   }

   fprintf(stderr,"(DEBUG) Profile UU values at LL:\n   ");
   EZGrid_LLGetValue(fld0,45.39,-79.9,0,57,vals);
   for(j=0;j<=57;j++) {
      printf("%f ",vals[j]);
   }
   printf("\n");

   fprintf(stderr,"(DEBUG) Time interp:\n");
   c_fnom(11,"/data/cmoex7/afsr005/tile/glb/2008010712_005","STD+RND+R/O",0);
   c_fstouv(11,"RND");

   fld1=EZGrid_Read(11,"UU","","",-1,-1,-1,1);

   EZGrid_Load(fld0,300,310,0,310,320,10);
   EZGrid_Load(fld1,300,310,0,310,320,10);
   fldn=EZGrid_InterpTime(fld0,fld1,344244050);
   EZGrid_GetValue(fld0,300,300,0,0,&val);
   fprintf(stderr,"(DEBUG)   t0=%f",val);
   EZGrid_GetValue(fld1,300,300,0,0,&val);
   fprintf(stderr,"(DEBUG) t1=%f",val);
   EZGrid_GetValue(fldn,300,300,0,0,&val);
   fprintf(stderr,"(DEBUG) tn=%f\n",val);

   EZGrid_Free(fld0);
   EZGrid_Free(fld1);
   EZGrid_Free(fldn);

   c_fstfrm(10);
   c_fclos(10);
   c_fstfrm(11);
   c_fclos(11);
}
