/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairies de fonctions utiles
 * Fichier      : ZRef.c
 * Creation     : Octobre 2011 - J.P. Gauthier
 *
 * Description  : Fonctions generales d'utilites courantes.
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

#include "ZRef.h"
#include "eerUtils.h"

int ZREF_IP1MODE=3;

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_Init>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Initialize a vertical reference.
 *
 * Parametres  :
 *  <ZRef>     : Vertical reference to initialize
 *
 * Retour:
 *  <Ok>       : (1=Ok, 0=Bad)
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_Init(TZRef *ZRef) {

   ZRef->Levels=NULL;
   ZRef->Type=LVL_UNDEF;
   ZRef->LevelNb=0;
   ZRef->PTop=ZRef->PRef=ZRef->ETop=0.0;
   ZRef->RCoef[0]=ZRef->RCoef[1]=1.0;
   ZRef->P0=ZRef->A=ZRef->B=NULL;
   ZRef->Version=0;

   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_Free>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Free a vertical reference.
 *
 * Parametres  :
 *  <ZRef>     : Vertical reference to free
 *
 * Retour:
 *  <Ok>       : (1=Ok, 0=Bad)
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_Free(TZRef *ZRef) {

   if (ZRef->Levels) free(ZRef->Levels); ZRef->Levels=NULL;
   if (ZRef->A)      free(ZRef->A);      ZRef->A=NULL;
   if (ZRef->B)      free(ZRef->B);      ZRef->B=NULL;
   if (ZRef->P0)     free(ZRef->P0);     ZRef->P0=NULL;

   ZRef->Version=0;
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_Equal>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Check for vertical reference equality
 *
 * Parametres  :
 *  <ZRef0>     : Vertical reference compare
 *  <ZRef1>     : Vertical reference compare
 *
 * Retour:
 *  <Ok>       : (1=Same, 0=not same)
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_Equal(TZRef *ZRef0,TZRef *ZRef1) {

   if ((ZRef0->LevelNb!=ZRef1->LevelNb) || (ZRef0->Type!=ZRef1->Type) || (ZRef0->Levels && memcmp(ZRef0->Levels,ZRef1->Levels,ZRef0->LevelNb*sizeof(float))!=0))
      return(0);

   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_Copy>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Copy a vertical reference into another one
 *
 * Parametres  :
 *  <ZRef0>     : Vertical reference destination
 *  <ZRef1>     : Vertical reference source
 *  <Level>     : (Level to copy or -1 for all)
 *
 * Retour:
 *  <Ok>       : (1=Ok, 0=Bad)
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_Copy(TZRef *ZRef0,TZRef *ZRef1,int Level) {

   if (!ZRef0 || !ZRef1 || Level>ZRef1->LevelNb-1)
      return(FALSE);

      if (Level<0) {
      ZRef0->LevelNb=ZRef1->LevelNb;
      ZRef0->Levels=(float*)malloc(ZRef1->LevelNb*sizeof(float));
      memcpy(ZRef0->Levels,ZRef1->Levels,ZRef1->LevelNb*sizeof(float));
   } else {
      ZRef0->LevelNb=1;
      ZRef0->Levels=(float*)malloc(sizeof(float));
      ZRef0->Levels[0]=ZRef1->Levels[Level];
   }
   ZRef0->Type=ZRef1->Type;
   ZRef0->PTop=ZRef0->PRef=ZRef0->ETop=0.0;
   ZRef0->RCoef[0]=ZRef0->RCoef[1]=1.0;
   ZRef0->P0=ZRef0->A=ZRef0->B=NULL;
   ZRef0->Version=0;

   return(TRUE);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_DecodeRPN>
 * Creation : Novembre 2008 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Decoder les parametres des niveaux a partir du champs HY ou !!
 *
 * Parametres   :
 *  <ZRef>      : Vertical referencer
 *  <Unit>      : FSTD file unit
 *
 * Retour:
 *  <Ok>        : (Index du champs <0=erreur).
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_DecodeRPN(TZRef *ZRef,int Unit) {

   TRPNHeader h;
   int        cd,key,skip,j,k,kind,flag=0,mode=-1;
   char       format;
   double    *buf=NULL;
   float     *pt=NULL;

   if (ZRef->Type==LVL_PRES) {
     return(1);
   }

   // Check fo regular hybrid (field HY)
   key = c_fstinf(Unit,&h.NI,&h.NJ,&h.NK,-1,"",-1,-1,-1,"X","HY");
   if (key>=0) {
      cd=c_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,h.GRTYP,&h.IG1,
                   &h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);
      if (cd>=0) {
         f77name(convip)(&h.IP1,&ZRef->PTop,&kind,&mode,&format,&flag);
         ZRef->RCoef[0]=h.IG2/1000.0f;
         ZRef->RCoef[1]=0.0f;
         ZRef->PRef=h.IG1;
         ZRef->Version=0;
      } else {
         fprintf(stdout,"(WARNING) ZRef_DecodeRPN: Could not get info on HY field (c_fstprm).\n");
      }

      /*There might be an HY field with coordinate sigma, in which case it really is ETA*/
      if (ZRef->Type==LVL_SIGMA) {
         ZRef->Type=LVL_ETA;
      }

   } else {

      key=c_fstinf(Unit,&h.NI,&h.NJ,&h.NK,-1,"",-1,-1,-1,"X","!!");
      if (key>=0) {
         cd=c_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,h.GRTYP,&h.IG1,
                      &h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);
         if (cd>=0) {
            ZRef->Version=h.IG1;
            ZRef->PRef=10.0;
            ZRef->PTop=h.IG2/10.0;
            ZRef->ETop=0.0;
            ZRef->RCoef[0]=0.0f;
            ZRef->RCoef[1]=0.0f;

            buf=(double*)malloc(h.NI*h.NJ*sizeof(double));
            if (!ZRef->A) ZRef->A=(float*)malloc(ZRef->LevelNb*sizeof(float));
            if (!ZRef->B) ZRef->B=(float*)malloc(ZRef->LevelNb*sizeof(float));

            cd=c_fstluk(buf,key,&h.NI,&h.NJ,&h.NK);
            if (cd>=0) {
               /* Read in header info*/
               switch(ZRef->Version) {
                  case 1001: skip=2; break;
                  case 1002: skip=2; ZRef->PTop=buf[h.NI]*0.01; break;
                  case 2001: skip=1; break;
                  case 5001: skip=3; ZRef->PTop=buf[h.NI]*0.01; ZRef->PRef=buf[h.NI+1]*0.01; ZRef->RCoef[0]=buf[h.NI+2]; break;
                  case 5002: skip=3; ZRef->PTop=buf[h.NI]*0.01; ZRef->PRef=buf[h.NI+1]*0.01; ZRef->RCoef[0]=buf[h.NI+2]; ZRef->RCoef[1]=buf[h.NI+h.NI]; break;
               }

               /* Find corresponding level */
               for(k=0;k<ZRef->LevelNb;k++) {
                  for(j=skip;j<h.NJ;j++) {
                     if (buf[j*h.NI]==ZRef_Level2IP(ZRef->Levels[k],ZRef->Type)) {
                        ZRef->A[k]=buf[j*h.NI+1];
                        ZRef->B[k]=buf[j*h.NI+2];
                        break;
                     }
                  }
               }
            } else {
               fprintf(stdout,"(WARNING) ZRef_DecodeRPN: Could not read !! field (c_fstluk).\n");
            }
         } else {
            fprintf(stdout,"(WARNING) ZRef_DecodeRPN: Could not get info on !! field (c_fstprm).\n");
         }
      } else {
         // Try to figure out if it's SIGMA or ETA
         if (ZRef->Type==LVL_SIGMA || (ZRef->Type==LVL_ETA && ZRef->PTop==0.0)) {
            /*If we find a PT field, we have ETA coordinate otherwise, its'SIGMA*/
            key=c_fstinf(Unit,&h.NI,&h.NJ,&h.NK,-1,"",-1,-1,-1,"","PT");
            ZRef->PTop=10.0;
            if (key>=0) {
               ZRef->Type=LVL_ETA;
               if (!(pt=(float*)malloc(h.NI*h.NJ*h.NK*sizeof(float)))) {
                  fprintf(stderr,"(WARNING) ZRef_DecodeRPN: Could not allocate memory for top pressure.\n");
               } else {
                  cd=c_fstluk(pt,key,&h.NI,&h.NJ,&h.NK);
                  if (cd>=0) {
                     ZRef->PTop=pt[0];
                  } else {
                     fprintf(stdout,"(WARNING) ZRef_DecodeRPN: Could not read PT field (c_fstluk).\n");
                  }
               }
            }
         }
      }
   }

   if (buf) free(buf);
   if (pt)  free(pt);

   return(key>=0);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_K2Pressure>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Convert a level into pressure.
 *
 * Parametres  :
 *  <ZRef>     : Vertical reference to free
 *  <P0>       : Pressure at surface in
 *  <K>        : Level index to convert
 *
 * Retour:
 *  <Pres>     : Pressure in mb
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
double ZRef_K2Pressure(TZRef* restrict const ZRef,double P0,int K) {

   double pres=-1.0,pref,ptop,rtop;

   pref=ZRef->PRef;
   ptop=ZRef->PTop;

   switch(ZRef->Version) {
      case 0:
         switch(ZRef->Type) {
            case LVL_PRES:
               pres=ZRef->Levels[K];
               break;

            case LVL_SIGMA:
               pres=P0*ZRef->Levels[K];
               break;

            case LVL_ETA:
               pres=ptop+(P0-ptop)*ZRef->Levels[K];
               break;

            case LVL_HYBRID:
               rtop=ptop/pref;
               pres=pref*ZRef->Levels[K]+(P0-pref)*pow((ZRef->Levels[K]-rtop)/(1.0-rtop),ZRef->RCoef[0]);
               break;

            default:
               fprintf(stderr,"(ERROR) ZRef_Level2Pressure: invalid level type (%i)",ZRef->Type);
         }
         break;

      case 1001: pres=ZRef->B[K]*P0; break;                                      // Sigma
      case 1002:                                                                 // Eta
      case 1003:                                                                 // Hybrid normalized
      case 5001: pres=(ZRef->A[K]+ZRef->B[K]*P0*100)*0.01; break;                // Hybrid
      case 2001: pres=ZRef->A[K]*0.01; break;                                    // Pressure
      case 5002: pres=exp(ZRef->A[K]+ZRef->B[K]*log(P0/pref))*0.01; break; // Hybrid momentum
      default:
         fprintf(stderr,"(ERROR) ZRef_Level2Pressure: invalid level type (%i)",ZRef->Type);
   }

   return(pres);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_KCube2Pressure>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Convert each coordinate in a 3D cube into pressure.
 *
 * Parametres  :
 *  <ZRef>     : Vertical reference to free
 *  <P0>       : Pressure at surface in mb
 *  <NIJ>      : 2D dimension of grid
 *  <Log>      : Calculate log of pressure
 *  <Pres>     : Output pressure in mb
 *
 * Retour:
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_KCube2Pressure(TZRef* restrict const ZRef,float *P0,int NIJ,int Log,float *Pres) {

   unsigned int k,idxk=0,ij;
   float        pref,ptop,rtop,pk,pr;

   if (!P0 && ZRef->Type!=LVL_PRES) {
      fprintf(stderr,"(ERROR) ZRef_KCube2Pressure: Surface pressure is required\n");
      return(0);
   }

   pref=ZRef->PRef;
   ptop=ZRef->PTop;

   switch(ZRef->Version) {
      case 0:
         switch(ZRef->Type) {
            case LVL_PRES:
               for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
                  for (ij=0;ij<NIJ;ij++) {
                     Pres[idxk+ij]=ZRef->Levels[k];
                  }
               }
               break;

            case LVL_SIGMA:
               for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
                  for (ij=0;ij<NIJ;ij++) {
                     Pres[idxk+ij]=P0[ij]*ZRef->Levels[k];
                  }
               }
               break;

            case LVL_ETA:
               for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
                  for (ij=0;ij<NIJ;ij++) {
                     Pres[idxk+ij]=ptop+(P0[ij]-ptop)*ZRef->Levels[k];
                  }
               }
               break;

            case LVL_HYBRID:
               rtop=ptop/pref;
               for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
                  pk=pref*ZRef->Levels[k];
                  pr=powf(((ZRef->Levels[k]-rtop)/(1.0-rtop)),ZRef->RCoef[0]);
                  for (ij=0;ij<NIJ;ij++) {
                     Pres[idxk+ij]=pk+(P0[ij]-pref)*pr;
                  }
               }
               break;

            default:
               fprintf(stderr,"(ERROR) ZRef_KCube2Pressure: invalid level type (%i)",ZRef->Type);
         }
         break;

      case 1001:                                                      // Sigma
         for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
            for (ij=0;ij<NIJ;ij++) {
               Pres[idxk+ij]=ZRef->B[k]*P0[ij];
            }
         }
         break;

      case 1002:                                                      // Eta
      case 1003:                                                      // Hybrid normalized
      case 5001:                                                      // Hybrid
         for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
            for (ij=0;ij<NIJ;ij++) {
               Pres[idxk+ij]=(ZRef->A[k]+ZRef->B[k]*P0[ij]*100.0)*0.01;
            }
         }
         break;

      case 2001:                                                      // Pressure
         for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
            for (ij=0;ij<NIJ;ij++) {
               Pres[idxk+ij]=ZRef->A[k]*0.01;
            }
         }
         break;
      case 5002:                                                     // Hybrid momentum
         for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
            for (ij=0;ij<NIJ;ij++) {
               Pres[idxk+ij]=exp(ZRef->A[k]+ZRef->B[k]*log(P0[ij]/pref))*0.01;
            }
         }
         break;

      default:
         fprintf(stderr,"(ERROR) ZRef_Level2Pressure: invalid level type (%i)",ZRef->Type);
   }

   if (Log) for (ij=0;ij<NIJ*ZRef->LevelNb;ij++) Pres[ij]=logf(Pres[ij]);

   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_KCube2Meter>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Convert each coordinate in a 3D cube into MASL.
 *
 * Parametres  :
 *  <ZRef>     : Vertical reference to free
 *  <GZ>       : Geopotential height 3D
 *  <NIJ>      : 2D dimension of grid
 *  <Height>   : Output height
 *
 * Retour:
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_KCube2Meter(TZRef* restrict const ZRef,float *GZ,const int NIJ,float *Height) {

   unsigned int k,idxk=0,ij;
   float        topo;

   switch (ZRef->Type) {
      case LVL_PRES:
      case LVL_HYBRID:
      case LVL_SIGMA:
      case LVL_ETA:
      case LVL_UNDEF:
      case LVL_THETA:
         for (k=0;k<NIJ*ZRef->LevelNb;k++) {
            Height[k]=GZ[k]*10.0f;
         }
         break;

      case LVL_MASL:
         for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
            for (ij=0;ij<NIJ;ij++) {
               Height[idxk+ij]=ZRef->Levels[k];
            }
         }
         break;

      case LVL_MAGL:
         /*Add the topography to the gz to get the heigth above the sea*/
         for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
            for (ij=0;ij<NIJ;ij++) {
               Height[idxk+ij]=(GZ[ij]*10.0f)+ZRef->Levels[k];
            }
         }
         break;

      case LVL_GALCHEN:
         /* Height = GALCHEN * (1 - h0/H) + h0
          * Where
          *  - GALCHEN is the level in gal-chen meters
          *  - h0 is the topography
          *  - H is the Height of the top of the atmosphere
          */
         for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
            for (ij=0;ij<NIJ;ij++) {
               topo=GZ[ij]*10.0f;
               Height[idxk+ij]=ZRef->Levels[k]*(1.0f-topo/ZRef->PTop)+topo;
            }
         }
         break;

      default:
         fprintf(stderr,"(ERROR) ZRef_KCube2Meter: invalid level type (%i)",ZRef->Type);
         return(0);
   }
   return(1);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_Level2Pressure>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Convert a level into pressure.
 *
 * Parametres  :
 *  <ZRef>     : Vertical reference to free
 *  <P0>       : Pressure at surface in mb
 *  <K>        : Level index to convert
 *
 * Retour:
 *  <Pres>     : Pressure in mb
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
double ZRef_Level2Pressure(TZRef* restrict const ZRef,double P0,double Level) {

   double pres=-1.0,pref,ptop,rtop;
   int    z,z0,z1,o;

   pref=ZRef->PRef;
   ptop=ZRef->PTop;

   switch(ZRef->Type) {
      case LVL_PRES:
         pres=Level;
         break;

      case LVL_SIGMA:
         pres=P0*Level;
         break;

      case LVL_ETA:
         pres=ptop+(P0-ptop)*Level;
         break;

      case LVL_HYBRID:
         //TODO need to add analytical function for GEM4 type
         if (ZRef->Version==5002) {
            // Check for level ordering
            o=(ZRef->Levels[0]<ZRef->Levels[ZRef->LevelNb]);

            // Find enclosing levels
            for(z=0;z<ZRef->LevelNb;z++) {
               if ((o && Level<ZRef->Levels[z]) || (!o && Level>ZRef->Levels[z])) {
                  z1=o?z:z-1;
                  z0=o?z-1:z;
                  break;
               }
            }

            // if we are within the vertical limits
            if (z>0 || z<ZRef->LevelNb) {

               // Interpolate between levels (in log(p))
               z=(z1-z);
               z0=ZRef->A[z0]+ZRef->B[z0]*log(P0/pref);
               z1=ZRef->A[z1]+ZRef->B[z1]*log(P0/pref);
               pres=exp(ILIN(z0,z1,z))*0.01;
            }
         } else {
            rtop=ptop/pref;
            pres=pref*Level+(P0-pref)*pow((Level-rtop)/(1.0-rtop),ZRef->RCoef[0]);
         }
         break;

      default:
         fprintf(stderr,"(ERROR) ZRef_Level2Pressure: invalid level type (%i)",ZRef->Type);
   }
   return(pres);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_Pressure2Level>
 * Creation : Octobre 2009 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Calculer la pression a un niveaux specifique
 *
 * Parametres :
 *   <Grid>       : Grille
 *   <Pressure>   : Pression du niveau an mb
 *   <P0>         : Pression au sol en mb
 *
 * Retour:
 *   <Level>      : Niveau du modele
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
double ZRef_Pressure2Level(TZRef* restrict const ZRef,double P0,double Pressure) {

   double level=-1.0;
   double a,b,c,d,r,l,err;
   float  p,pres,pres0=0,pres1;
   int    z;

   switch(ZRef->Type) {
      case LVL_PRES:
         level=Pressure;
         break;

      case LVL_SIGMA:
         level=Pressure/P0;
         break;

      case LVL_ETA:
         level=(Pressure-ZRef->PTop)/(P0-ZRef->PTop);
         break;

      case LVL_HYBRID:
         //TODO need to add analytical function for GEM4 type
         if (ZRef->Version==5002) {
            // Interpolate in log(p)
            pres=log(Pressure*100.0);

            // Find enclosing levels
            for(z=0;z<ZRef->LevelNb;z++) {
               pres1=pres0;
               pres0=ZRef->A[z]+ZRef->B[z]*log(P0/ZRef->PRef*100.0);
               if (pres>=pres0 && pres<=pres1) {
                  level=(pres1-pres)/(pres1-pres0);
                  level=ILIN(ZRef->Levels[z],ZRef->Levels[z-1],level);
                  break;
               }
               if (pres>=pres1 && pres<=pres0) {
                  level=(pres0-pres)/(pres0-pres1);
                  level=ILIN(ZRef->Levels[z-1],ZRef->Levels[z],level);
                  break;
               }
            }
         } else {
            a=ZRef->PRef*100.0;
            b=(P0-ZRef->PRef)*100.0;
            c=(ZRef->PTop/ZRef->PRef)*100.0;
            d=Pressure*100.0;
            r=ZRef->RCoef[0];

            /*Use iterative method Newton-Raphson (developped by Alain Malo)*/
            level=0.5;
            err=1.0;
            while(err>0.0001) {
               // TODO: above 12 mb, this code fails with a nan on the pow(level-c,r-1)
               l=level-((a*level+b*pow((level-c)/(1-c),r)-d)/(a+b*r/(pow(1-c,r))*pow(level-c,r-1)));
               err=fabs(l-level)/level;
               level=l;
            }
         }
         break;

      default:
         fprintf(stderr,"(ERROR) ZRef_Level2Pressure: invalid level type (%i)",ZRef->Type);
   }
   return(level);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_Level2Meter>
 * Creation : Avril 2004 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Derminer le niveaux en metre a d'un niveaux d'un autre type
 *
 * Parametres :
 *  <Type>    : Type de niveau.
 *  <Level>   : Valeur du niveau.
 *
 * Retour:
 *  <niveau>  : Niveau en metres
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
double ZRef_Level2Meter(double Level,int Type) {

   /* Dans le cas d'un niveau 0 (Mauvaise habitude premise pour sigma et eta a 0)*/
   if (Type==LVL_PRES && Level==0) {
      return(0);
   }

   switch(Type) {
      case LVL_MASL    : return (Level); break;
      case LVL_ETA     :
      case LVL_SIGMA   : return (SIGMA2METER(Level)); break;
      case LVL_PRES    : return (PRESS2METER(Level)); break;
      case LVL_UNDEF   : return (Level); break;
      case LVL_MAGL    : return (Level); break;
      case LVL_HYBRID  : return (SIGMA2METER(Level)); break;
      case LVL_THETA   : return (SIGMA2METER(Level)); break;
      case LVL_GALCHEN : return (Level); break;
      case LVL_ANGLE   : return (Level); break;
   }

   return(0.0);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_IP2Meter>
 * Creation : Octobre 2001 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Transfomer une valeur IP1 en elevation en metres.
 *
 * Parametres :
 *  <IP>      : Valeur IP1
 *
 * Retour:
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
double ZRef_IP2Meter(int IP) {

   int   mode=-1,flag=0,kind;
   float level=0.0;
   char  format;

   /*Si c'est 0 on force a 0 metre car 0 mb=outter space*/
   if (IP==0)
      return(0);

   /*Convertir en niveau reel*/
   f77name(convip)(&IP,&level,&kind,&mode,&format,&flag);

   return(ZRef_Level2Meter(level,kind));
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_IP2Level>
 * Creation : Mai 2003 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Transfomer un IP1 en niveau.
 *
 * Parametres :
 *  <Level>   : Valeur du niveau
 *  <Type>    : Type de niveau (Coordonnees)
 *
 * Retour:
 *  <IP>      : Niveau
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
double ZRef_IP2Level(int IP,int *Type) {

   int    mode=-1,flag=0;
   float  level=0.0;
   char   format;

   /*Convertir en niveau reel*/
   f77name(convip)(&IP,&level,Type,&mode,&format,&flag);

   return(level);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_Level2IP>
 * Creation : Mai 2003 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Transfomer un niveau en IP1.
 *
 * Parametres :
 *  <Level>   : Valeur du niveau
 *  <Type>    : Type de niveau (Coordonnees)
 *
 * Retour:
 *  <IP>      : IP1
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_Level2IP(float Level,int Type) {

   int    flag=0,ip=0,mode;
   char   format;

   if (Type<0) {
      return(-1);
   } else {
      /*ETA | THETA -> SIGMA*/
      if (Type==LVL_ETA || Type==LVL_THETA) {
         Type=LVL_SIGMA;
      }

      /*GALCHEN -> MASL*/
      if (Type==LVL_GALCHEN) {
         Type=LVL_MAGL;
      }

      /*Convertir en niveau reel*/
      if (Type==LVL_HYBRID || Type==LVL_MAGL) {
         mode=2;
      } else {
         mode=ZREF_IP1MODE;
      }

      f77name(convip)(&ip,&Level,&Type,&mode,&format,&flag);

      return(ip);
   }
}
