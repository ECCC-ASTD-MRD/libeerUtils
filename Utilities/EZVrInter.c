/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de fonctions utiles
 * Creation  : Janvier 2008
 * Auteur    : Jean-Philippe Gauthier
 *
 * Description: RPN fstd field tiler
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

#include "App.h"
#include "eerUtils.h"
#include "EZVrInt.h"

int main (int argc, char **argv) {

   float ip1Pression[40] = {
      100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600, 630, 660,
      690, 720, 750, 780, 800, 810, 820, 830, 840, 850, 860, 870, 880,
      890, 900, 910, 920, 930, 940, 950, 960, 970, 980, 990, 1000, 1015, 1030
   };

   float levelSigma[38] = {
      9.9999994E-02, 0.1500000, 0.2000000,
      0.2500000, 0.3000000, 0.3500000, 0.4000000,
      0.4500000, 0.5000000, 0.5500000, 0.6000000,
      0.6300000, 0.6600000, 0.6900000, 0.7200000,
      0.7500000, 0.7800000, 0.8000000, 0.8100000,
      0.8200000, 0.8300000, 0.8400000, 0.8500000,
      0.8600000, 0.8700000, 0.8800000, 0.8900000,
      0.9000000, 0.9100000, 0.9200000, 0.9299999,
      0.9400000, 0.9500000, 0.9600000, 0.9700000,
      0.9800000, 0.9899999, 1.000000
   };

   int indexSrc, indexDest;

   int nrec;
   char nmvar[4];
   char nomvar[4];
   char etikets[13];
   char etiket[13];
   const int NB_DATE = 60;
   int *lsniv, *in;
   float pressionSol[300 * 280];
   float *dataFieldPression;
   float *dataFieldSigma;
   int ivar, k;
   int ni, nj, nk;
   int ig1, ig2, ig3, ig4, swa, lng, dltf, ubc, xt1, xt2, xt3;
   int ip1, ip2, ip3;
   int num;
   int date, deet, npas, nbits;
   char grtyp[2];
   char typvar[3];
   int er;
   char *varList[6];
   int datyp;
   int sigmaEncode;

   dataFieldPression = (float *) malloc (300 * 280 * 40 * sizeof (float));
   dataFieldSigma = (float *) malloc (300 * 280 * 38 * sizeof (float));

   varList[0] = strdup ("UU");
   varList[1] = strdup ("VV");
   varList[2] = strdup ("TT");
   varList[3] = strdup ("GZ");
   varList[4] = strdup ("WW");
   varList[5] = strdup ("HR");

   in = (int *) calloc (NB_DATE, sizeof (int));
   lsniv = (int *) calloc (40 * NB_DATE, sizeof (int));

   c_fnom (10, "03032000_4km.000120.P", "RND", 0);
   c_fnom (20, "test.out", "RND", 0);

   c_fstouv (10, "RND");
   c_fstouv (20, "RND");

   nrec = c_fstnbr (10);
   if (nrec == 0) {
      printf ("ERROR : empty file");
      c_fstfrm (10);
      c_fstfrm (20);

      c_fclos (10);
      c_fclos (20);
   }

   strcpy (nmvar, "p0");
   strcpy (etikets, "etikets");

   indexSrc = c_viqkdef (40, LVL_PRES, ip1Pression, 0, 0, 0);
  indexDest = c_viqkdef (38, LVL_SIGMA, levelSigma, 0, 0, 0);

   er = c_fstlir (pressionSol, 10, &ni, &nj, &nk, -1, "", -1, -1, -1, "", "p0");

   if (c_videfset (300, 280, indexDest, indexSrc, (float *) pressionSol, NULL)
       != 0) {
      fprintf (stderr, "c_videfset erreur");
      exit (-1);
   }

   c_visetopti (LINEAR | LAPSERATE);

   for (ivar = 0; ivar < 5; ivar++) {
      for (k = 0; k < 40; k++) {
         ip1 = (int) ip1Pression[k];

         num = c_fstlir (&dataFieldPression[k * 300 * 280], 10, &ni,
                         &nj, &nk, -1, "", ip1, -1, -1, "", varList[ivar]);
      }

      (void) c_visint (dataFieldSigma, dataFieldPression,
                       NULL, NULL, 0.4, -0.5);

      sprintf (typvar, "  ");
      sprintf (nomvar, "    ");
      sprintf (etiket, "            ");
      (void) c_fstprm (num, &date, &deet, &npas, &ni, &nj, &nk,
                       &nbits, &datyp, &ip1, &ip2, &ip3, typvar,
                       nomvar, etiket, grtyp, &ig1, &ig2, &ig3,
                       &ig4, &swa, &lng, &dltf, &ubc, &xt1, &xt2, &xt3);

      for (k = 0; k < 38; k++) {

         sigmaEncode = levelSigma[k] * 10000 + 2000;

         (void) c_fstecr (&dataFieldSigma[k * 300 * 280],
                          NULL, 1, 20, date, deet, npas, 300, 280, 1,
                          sigmaEncode, ip2, ip3, typvar, varList[ivar],
                          etikets, grtyp, ig1, ig2, ig3, ig4, 1, 1);
      }
   }

   c_fstfrm (10);
   c_fstfrm (20);

   c_fclos (10);
   c_fclos (20);

   exit (EXIT_SUCCESS);
}
