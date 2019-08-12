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

#include "App.h"
#include "RPN.h"
#include "eerUtils.h"
#include "ZRef.h"

#ifdef HAVE_VGRID
#include "vgrid.h"
#endif

static float       *ZRef_Levels   = NULL;
static unsigned int ZRef_LevelsNb = 0;
static const char  *ZRef_Names[]  = { "MASL","SIGMA","PRESSURE","UNDEFINED","MAGL","HYBRID","THETA","ETA","GALCHEN","COUNT","HOUR","ANGLE","NIL","NIL","NIL","INT","NIL","IDX","NIL","NIL","NIL","MPRES",NULL };
static const char  *ZRef_Units[]  = { "m","sg","mb","-","m","hy","th","sg","m","nb","hr","dg","--","--","--","i","--","x","--","--","--","mp",NULL };

int ZREF_IP1MODE=3;

const char **ZRef_LevelNames() {
   return(ZRef_Names);
}

const char *ZRef_LevelName(int Type) {
   return(ZRef_Names[Type<0||Type>21?22:Type]);
}

const char **ZRef_LevelUnits() {
   return(ZRef_Units);
}

const char *ZRef_LevelUnit(int Type) {
   return(ZRef_Units[Type<0||Type>21?22:Type]);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_New>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Initialize a new vertical reference.
 *
 * Parametres  :
 *
 * Retour:
 *  <ZRef>     : Vertical reference (NULL on error)
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TZRef* ZRef_New(void) {

   TZRef *zref=NULL;

   if (!(zref=(TZRef*)calloc(1,sizeof(TZRef)))) {
      App_Log(ERROR,"%s: Unable to allocate memory\n",__func__);
   }
   
   zref->VGD=NULL;
   zref->Levels=NULL;
   zref->Style=NEW;
   zref->Type=LVL_UNDEF;
   zref->LevelNb=0;
   zref->POff=zref->PTop=zref->PRef=zref->ETop=0.0;
   zref->RCoef[0]=zref->RCoef[1]=1.0;
   zref->P0=zref->PCube=zref->A=zref->B=NULL;
   zref->Version=-1;
   zref->NRef=1;
   
   return(zref);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_Define>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Initialize a vertical reference.
 *
 * Parametres  :
 *  <ZRef>     : Vertical reference to initialize
 *  <Type>     : Type of levels
 *  <NbLevels> : Number of levels passed in
 *  <Levels>   : List of levels to assign
 *
 * Retour:
 *  <Ok>       : (1=Ok, 0=Bad)
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TZRef* ZRef_Define(int Type,int NbLevels,float *Levels) {

   TZRef *zref=NULL;
   
   if ((zref=ZRef_New())) {

      zref->Type=Type;
      zref->LevelNb=NbLevels;
      
      if (zref->Levels && zref->LevelNb!=NbLevels) {
         zref->Levels=(float*)realloc(zref->Levels,zref->LevelNb*sizeof(float));
      } else {
         zref->Levels=(float*)calloc(zref->LevelNb,sizeof(float));
      }
      
      if (!zref->Levels) {
         App_Log(ERROR,"%s: Unable to allocate memory\n",__func__);
      } else if (Levels) {
         memcpy(zref->Levels,Levels,zref->LevelNb*sizeof(float));
      }
   }
   
   return(zref);
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

   if (ZRef && !ZRef_Decr(ZRef)) {

#ifdef HAVE_VGRID
      if (ZRef->VGD)    Cvgd_free((vgrid_descriptor**)&ZRef->VGD); ZRef->VGD=NULL;
#endif
      if (ZRef->Levels) free(ZRef->Levels);    ZRef->Levels=NULL;
      if (ZRef->A)      free(ZRef->A);         ZRef->A=NULL;
      if (ZRef->B)      free(ZRef->B);         ZRef->B=NULL;
// P0 is owned by other packages
//      if (ZRef->P0)     free(ZRef->P0);     ZRef->P0=NULL;
      if (ZRef->PCube)  free(ZRef->PCube);     ZRef->PCube=NULL;

      ZRef->Version=-1;
      ZRef->LevelNb=0;

      free(ZRef);
   }
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

   if (!ZRef0 || !ZRef1)
      return(0);
      
#ifdef HAVE_VGRID
   if (ZRef0->VGD && ZRef1->VGD)
      return(!Cvgd_vgdcmp((vgrid_descriptor*)ZRef0->VGD,(vgrid_descriptor*)ZRef1->VGD));
#endif
   
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
 *  <ZRef>     : Vertical reference to copy
 *
 * Retour:
 *  <ZRef>     : Copied vertical reference (Error=NULL)
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TZRef* ZRef_Copy(TZRef *ZRef) {

   if (ZRef) ZRef_Incr(ZRef);
   
   return(ZRef);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_HardCCopy>
 * Creation : Octobre 2011 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Copy a vertical reference into another one
 *
 * Parametres  :
 *  <ZRef>     : Vertical reference to copy
 *
 * Retour:
 *  <ZRef>     : Copied vertical reference (Error=NULL)
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TZRef *ZRef_HardCopy(TZRef *ZRef) {

   TZRef *zref=NULL;
   
   if (ZRef && (zref=ZRef_New())) {
   
      zref->LevelNb=ZRef->LevelNb;
      zref->Levels=(float*)malloc(ZRef->LevelNb*sizeof(float));
      memcpy(zref->Levels,ZRef->Levels,ZRef->LevelNb*sizeof(float));
      
      zref->Type=ZRef->Type;
      zref->PTop=ZRef->PTop;
      zref->PRef=ZRef->PRef;
      zref->ETop=ZRef->ETop;
      zref->RCoef[0]=ZRef->RCoef[0];
      zref->RCoef[1]=ZRef->RCoef[1];
      zref->P0=zref->A=zref->B=NULL;
      zref->Version=-1;
      zref->NRef=1;
      zref->Style=ZRef->Style;
   }
   return(zref);
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
   int        cd,key=0,skip,j,k,kind,ip;
   double    *buf=NULL;
   float     *pt=NULL;

   if (!ZRef) {
      return(0);
   }
      
   if (ZRef->Type==LVL_PRES || ZRef->Type==LVL_UNDEF) {
     return(1);
   }

   memset(&h,0,sizeof(TRPNHeader));
   
#ifdef HAVE_RMN

   RPN_FieldLock();

   // Check for toctoc (field !!)   
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

#ifdef HAVE_VGRID
         if (Cvgd_new_read((vgrid_descriptor**)&ZRef->VGD,Unit,-1,-1,-1,-1)==VGD_ERROR) {
            App_Log(ERROR,"%s: Unable to initialize vgrid descriptor.\n",__func__);
            return(0);
         }
#endif                  
         cd=c_fstluk(buf,key,&h.NI,&h.NJ,&h.NK);
         if (cd>=0) {

            /* Read in header info*/
            switch(ZRef->Version) {
               case 1001: ZRef->Type=LVL_SIGMA;  break;
               case 1002: ZRef->Type=LVL_ETA;    ZRef->PTop=buf[h.NI]*0.01; break;
               case 2001: ZRef->Type=LVL_PRES;   break;
               case 1003: ZRef->Type=LVL_ETA;    ZRef->PTop=buf[h.NI]*0.01; ZRef->PRef=buf[h.NI+1]*0.01; ZRef->RCoef[0]=buf[h.NI+2]; break;
               case 5001: ZRef->Type=LVL_HYBRID; ZRef->PTop=buf[h.NI]*0.01; ZRef->PRef=buf[h.NI+1]*0.01; ZRef->RCoef[0]=buf[h.NI+2]; break;
               case 5002:
               case 5003: ZRef->Type=LVL_HYBRID; ZRef->PTop=buf[h.NI]*0.01; ZRef->PRef=buf[h.NI+1]*0.01; ZRef->RCoef[0]=buf[h.NI+2]; ZRef->RCoef[1]=buf[h.NI+h.NI]; break;
            }
            skip=buf[2];

            /* Find corresponding level */
            for(k=0;k<ZRef->LevelNb;k++) {
               ip=ZRef_Level2IP(ZRef->Levels[k],ZRef->Type,ZRef->Style);
               for(j=skip;j<h.NJ;j++) {
                  if (buf[j*h.NI]==ip) {
                     ZRef->A[k]=buf[j*h.NI+1];
                     ZRef->B[k]=buf[j*h.NI+2];
                     break;
                  }
               }
               if (j==h.NJ) {
                  App_Log(WARNING,"%s: Could not find level definition for %i.\n",__func__,ip);
               }
            }
         } else {
            App_Log(WARNING,"%s: Could not read !! field (c_fstluk).\n",__func__);
         }
      } else {
         App_Log(WARNING,"%s: Could not get info on !! field (c_fstprm).\n",__func__);
      }
   } else {

      // Check fo regular hybrid (field HY)
      key = c_fstinf(Unit,&h.NI,&h.NJ,&h.NK,-1,"",-1,-1,-1,"X","HY");
      if (key>=0) {
         cd=c_fstprm(key,&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&h.NK,&h.NBITS,&h.DATYP,&h.IP1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,h.GRTYP,&h.IG1,
                     &h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,&h.UBC,&h.EX1,&h.EX2,&h.EX3);
         if (cd>=0) {
            ZRef->PTop=ZRef_IP2Level(h.IP1,&kind);
            ZRef->RCoef[0]=h.IG2/1000.0f;
            ZRef->RCoef[1]=0.0f;
            ZRef->PRef=h.IG1;
//            ZRef->Type=LVL_HYBRID;
         } else {
            App_Log(WARNING,"%s: Could not get info on HY field (c_fstprm).\n",__func__);
         }
         // It might be ETA
         if (ZRef->Type==LVL_SIGMA) {
            ZRef->Type=LVL_ETA;
         }

      } else {

         // Try to figure out if it's SIGMA or ETA
         if (ZRef->Type==LVL_SIGMA || (ZRef->Type==LVL_ETA && ZRef->Version==-1)) {
            /*If we find a PT field, we have ETA coordinate otherwise, its'SIGMA*/
            key=c_fstinf(Unit,&h.NI,&h.NJ,&h.NK,-1,"",-1,-1,-1,"","PT");
            ZRef->PTop=10.0;
            if (key>=0) {
               ZRef->Type=LVL_ETA;
               if (!(pt=(float*)malloc(h.NI*h.NJ*h.NK*sizeof(float)))) {
                  App_Log(WARNING,"%s: Could not allocate memory for top pressure, using default PTOP=10.0.\n",__func__);
               } else {
                  cd=c_fstluk(pt,key,&h.NI,&h.NJ,&h.NK);
                  if (cd>=0) {
                     ZRef->PTop=pt[0];
                  } else {
                     App_Log(WARNING,"%s: Could not read PT field (c_fstluk), using default PTOP=10.0.\n",__func__);
                  }
               }
            }
         }
      }

      ZRef->Version=0;
   }

   RPN_FieldUnlock();

   if (ZRef->PCube)  free(ZRef->PCube);  ZRef->PCube=NULL;
   if (buf) free(buf);
   if (pt)  free(pt);
#else
   App_Log(ERROR,"%s: Need RMNLIB\n",__func__);
#endif

   return(key>=0);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_SetRestrictLevels>
 * Creation : Janvier 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Definir une liste de restriction de niveaux
 *
 * Parametres   :
 *  <Levels>    : Liste des niveaux
 *  <NbLevels>  : Nombre de niveaux
 *
 * Retour:
 *  <Ok>        : (Index du champs <0=erreur).
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_SetRestrictLevels(float *Levels,int NbLevels) {

   ZRef_LevelsNb=NbLevels;
   ZRef_Levels=(float*)realloc(ZRef_Levels,ZRef_LevelsNb*sizeof(float));

   memcpy(ZRef_Levels,Levels,ZRef_LevelsNb);
   qsort(ZRef_Levels,ZRef_LevelsNb,sizeof(float),QSort_Float);

   return(ZRef_LevelsNb);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_AddRestrictLevel>
 * Creation : Janvier 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Definir une liste de restriction de niveaux
 *
 * Parametres   :
 *  <Level>     : Niveaux
 *
 * Retour:
 *  <Ok>        : (Index du champs <0=erreur).
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_AddRestrictLevel(float Level) {

   ZRef_LevelsNb++;
   ZRef_Levels=(float*)realloc(ZRef_Levels,ZRef_LevelsNb*sizeof(float));
   ZRef_Levels[ZRef_LevelsNb-1]=Level;

   qsort(ZRef_Levels,ZRef_LevelsNb,sizeof(float),QSort_Float);

   return(ZRef_LevelsNb);
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_GetLevels>
 * Creation : Janvier 2012 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Lire la liste des niveaux disponibles
 *
 * Parametres   :
 *  <ZRef>      : Vertical referencer
 *  <H>         : RPN header
 *  <Order>     : Ordre de tri des niveaux (IP1) (-1=decroissant, 0=1 seul niveau, 1=croissant)
 *
 * Retour:
 *  <Ok>        : (Index du champs <0=erreur).
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_GetLevels(TZRef *ZRef,const TRPNHeader* restrict const H,int Order) {

   TRPNHeader h;
   int        l,key,ip1=0,idlst[RPNMAX];
   int        k,k2,kx;

#ifdef HAVE_RMN

   if (Order) {
      /*Get the number of levels*/
      /*In case of # grid, set IP3 to 1 to get NK just for the first tile*/
      memcpy(&h,H,sizeof(TRPNHeader));
      h.IP3=H->GRTYP[0]=='#'?1:-1;
      c_fstinl(h.FID,&h.NI,&h.NJ,&h.NK,h.DATEV,h.ETIKET,-1,h.IP2,h.IP3,h.TYPVAR,h.NOMVAR,idlst,&h.NK,RPNMAX);
      if (!(ZRef->Levels=(float*)malloc(h.NK*sizeof(float)))) {
         return(0);
      }

      /*Get the levels*/
      for(k=k2=0;k<h.NK;k++) {
         key=c_fstprm(idlst[k],&h.DATEO,&h.DEET,&h.NPAS,&h.NI,&h.NJ,&kx,&h.NBITS,
               &h.DATYP,&ip1,&h.IP2,&h.IP3,h.TYPVAR,h.NOMVAR,h.ETIKET,
               h.GRTYP,&h.IG1,&h.IG2,&h.IG3,&h.IG4,&h.SWA,&h.LNG,&h.DLTF,
               &h.UBC,&h.EX1,&h.EX2,&h.EX3);

         ZRef->Levels[k2]=ZRef_IP2Level(ip1,&l);
         if (k==0) ZRef->Type=l;

         /* If a list of restrictive levels is defined, check for validity*/
         if (h.NK>10 && ZRef_Levels) {
            if (!bsearch(&ZRef->Levels[k2],ZRef_Levels,ZRef_LevelsNb,sizeof(float),QSort_Float)) {
               continue;
            }
         }

         /*Make sure we use a single type of level, the first we get*/
         if (l==ZRef->Type) {
            k2++;
         }
      }
      ZRef->LevelNb=k2;

      /*Sort the levels from ground up and remove duplicates*/
      qsort(ZRef->Levels,ZRef->LevelNb,sizeof(float),Order==-1?QSort_DecFloat:QSort_Float);
      Unique(ZRef->Levels,&ZRef->LevelNb,sizeof(ZRef->Levels[0]));
   } else {
      if (!(ZRef->Levels=(float*)malloc(sizeof(float)))) {
         return(0);
      }
      ZRef->LevelNb=1;
      ZRef->Levels[0]=ZRef_IP2Level(H->IP1,&ZRef->Type);
   }

   ZRef->Style=H->IP1>32768?NEW:OLD;
   if (ZRef->PCube)  free(ZRef->PCube);  ZRef->PCube=NULL;
#else
   App_Log(ERROR,"%s: Need RMNLIB\n",__func__);
#endif

   return(ZRef->LevelNb);
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

   return(ZRef_Level2Pressure(ZRef,P0,ZRef->Levels[K]));
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
   int          *ips;
   float        pref,ptop,*p0;

   if (!P0 && ZRef->Type!=LVL_PRES) {
      App_Log(ERROR,"%s: Surface pressure is required\n",__func__);
      return(0);
   }

   pref=ZRef->PRef;
   ptop=ZRef->PTop;

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

      case LVL_UNDEF:
         for (k=0;k<ZRef->LevelNb;k++,idxk+=NIJ) {
            for (ij=0;ij<NIJ;ij++) {
               Pres[idxk+ij]=P0[idxk+ij];
            }
         }
         break;
         
      case LVL_HYBRID:
#ifdef HAVE_RMN         
         if (ZRef->Version<=0) {
            ij=1;
            f77name(hyb_to_pres)(Pres,ZRef->Levels,&ptop,&ZRef->RCoef[0],&pref,&ij,P0,&NIJ,&ij,&ZRef->LevelNb);
         } else {
            ips=malloc(ZRef->LevelNb*sizeof(int));
            for (k=0;k<ZRef->LevelNb;k++) {
               ips[k]=ZRef_Level2IP(ZRef->Levels[k],ZRef->Type,ZRef->Style);
            }
            
            // Really not optimal but Cvgd needs pascals
            p0=(float*)malloc(NIJ*sizeof(float));
            for (ij=0;ij<NIJ;ij++) {
               p0[ij]=P0[ij]*MB2PA;
            }
            
#ifdef HAVE_VGRID
            if (Cvgd_levels((vgrid_descriptor*)ZRef->VGD,NIJ,1,ZRef->LevelNb,ips,Pres,p0,0)) {
               App_Log(ERROR,"%s: Problems in Cvgd_levels\n",__func__);
               return(0);
            }
#else
            App_Log(ERROR,"%s: Library not built with VGRID\n",__func__);
#endif
            for (ij=0;ij<NIJ*ZRef->LevelNb;ij++) Pres[ij]*=PA2MB;
            free(ips);
            free(p0);            
         }
#endif
         break;
         
      default:
         App_Log(ERROR,"%s: Invalid level type (%i)\n",__func__,ZRef->Type);
         return(0);
   }
   
   if (ZRef->POff!=0.0) for (ij=0;ij<NIJ*ZRef->LevelNb;ij++) Pres[ij]+=ZRef->POff;
   if (Log)             for (ij=0;ij<NIJ*ZRef->LevelNb;ij++) Pres[ij]=log(Pres[ij]);
   
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
         App_Log(ERROR,"%s: Invalid level type (%i)\n",__func__,ZRef->Type);
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

   double pres=-1.0,p0,pref,ptop,rtop;
   int    ip;

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
         if (ZRef->Version<=0) {
            rtop=ptop/pref;
            pres=pref*Level+(P0-pref)*pow((Level-rtop)/(1.0-rtop),ZRef->RCoef[0]);
         } else {
            ip=ZRef_Level2IP(Level,ZRef->Type,ZRef->Style);
            p0=P0*MB2PA;
#ifdef HAVE_VGRID
            if (Cvgd_levels_8((vgrid_descriptor*)ZRef->VGD,1,1,1,&ip,&pres,&p0,0)) {
               App_Log(ERROR,"%s: Problems in Cvgd_levels_8\n",__func__);
               return(0);
            }
#else
            App_Log(ERROR,"%s: Library not built with VGRID\n",__func__);
#endif
            pres*=PA2MB;
         }
         break;

      default:
         App_Log(ERROR,"%s: Invalid level type (%i)\n",__func__,ZRef->Type);
         return(0);
   }
   
   return(pres+ZRef->POff);
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
   float  pres,pres0=0,pres1;
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
               pres0=ZRef->A[z]+ZRef->B[z]*log(P0/ZRef->PRef);
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
         App_Log(ERROR,"%s: Invalid level type (%i)\n",__func__,ZRef->Type);
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

   double m=0.0;
   
   // Dans le cas d'un niveau 0 et type SIGMA, on force a ETA
   if (Level==0 && Type==LVL_SIGMA) {
      Type=LVL_ETA;
   }

   switch(Type) {
      case LVL_MASL    : m=Level; break;
      
      case LVL_ETA     : m=ETA2METER(Level); break;
      
      case LVL_SIGMA   : m=SIGMA2METER(Level); break;
      
      case LVL_PRES    : if (Level>226.3203) {
                            m=44330.8*(1-pow(Level/1013.25,0.190263));
                         } else if (Level>54.749) {
                            m=11000.0-6341.624*log(Level/226.3202);
                         } else if (Level>8.68) {
                            m=20000.0+216650.0*(pow(Level/54.749,-0.0292173)-1);
                         } else if (Level>0) {
                            m=32000.0+81660.7*(pow(Level/8.680157,-0.0819469)-1);
                         } else {
                            m=0;
                         }
                         break;
                         
      case LVL_UNDEF   : m=Level; break;
      
      case LVL_MAGL    : m=Level; break;
      
      case LVL_HYBRID  : m=ETA2METER(Level); break;
      
      case LVL_THETA   : m=0; break;
      
      case LVL_GALCHEN : m=Level; break;
      
      case LVL_ANGLE   : m=Level; break;
   }

   return(m);
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

   int   mode=-1,flag=0,kind=LVL_MASL;
   float level=0.0;
   char  format;

   // Si c'est 0 on force a 0 metre car 0 mb=outter space
   if (IP==0)
      return(0);

#ifdef HAVE_RMN
   // Convertir en niveau reel
   f77name(convip_plus)(&IP,&level,&kind,&mode,&format,&flag);
#else
   App_Log(ERROR,"%s: Need RMNLIB\n",__func__);
#endif

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

#ifdef HAVE_RMN
   // Convertir en niveau reel
   f77name(convip_plus)(&IP,&level,Type,&mode,&format,&flag);
#else
   App_Log(ERROR,"%s: Need RMNLIB\n",__func__);
#endif

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
 *  <Mode>    : Mode (NEW,OLD,DEFAULT)
 *
 * Retour:
 *  <IP>      : IP1
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */

int ZRef_Level2IP(float Level,int Type,TZRef_IP1Mode Mode) {

   int    flag=0,ip=0,mode;
   char   format;

   if (Type<0) {
      return(Level);
   } else {
      mode=Mode==DEFAULT?ZREF_IP1MODE:Mode;
         
      // ETA -> SIGMA
      if (Type==LVL_ETA) {
         Type=LVL_SIGMA;
      }

      // GALCHEN -> MAGL
      if (Type==LVL_GALCHEN) {
         // MAGL can't be encoded in old style so set to MASL in this galchen case
         Type=mode==3?LVL_MASL:LVL_MAGL;
      }

      // Hybrid,MAGL and Teta can't be encoded in old style so force new style
      if (Type==LVL_HYBRID || Type==LVL_MAGL || Type==LVL_THETA) {
         mode=2;
      }

#ifdef HAVE_RMN
      f77name(convip_plus)(&ip,&Level,&Type,&mode,&format,&flag);
#else
   App_Log(ERROR,"%s: Need RMNLIB\n",__func__);
#endif

      return(ip);
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <ZRef_IPFormat>
 * Creation : Mai 2003 - J.P. Gauthier - CMC/CMOE
 *
 * But      : Formatter un IP en niveau et unite
 *
 * Parametres :
 *  <Buf>     : Chaine a remplir
 *  <IP>      : IP a formatter
 *
 * Retour:
 *  <Type>    : Type de niveau
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int ZRef_IPFormat(char *Buf,int IP,int Interval) {

   int   type=-1;
   float lvl;
   
   if (Interval && IP<=32000) {
      sprintf(Buf," %8i %-2s",IP,ZRef_Units[LVL_UNDEF]);
   } else {
      lvl=ZRef_IP2Level(IP,&type);

      switch(type) {
         case LVL_MASL  : sprintf(Buf," %8.1f %-2s",lvl,ZRef_Units[type]); break;
         case LVL_SIGMA : sprintf(Buf," %8.4f %-2s",lvl,ZRef_Units[type]); break;
         case LVL_PRES  : sprintf(Buf," %8.1f %-2s",lvl,ZRef_Units[type]); break;
         case LVL_UNDEF : sprintf(Buf," %8.1f %-2s",lvl,ZRef_Units[type]); break;
         case LVL_MAGL  : sprintf(Buf," %8.1f %-2s",lvl,ZRef_Units[type]); break;
         case LVL_HYBRID: sprintf(Buf," %8.6f %-2s",lvl,ZRef_Units[type]); break;
         case LVL_THETA : sprintf(Buf," %8.4f %-2s",lvl,ZRef_Units[type]); break;
         case LVL_HOUR  : sprintf(Buf," %8.1f %-2s",lvl,ZRef_Units[type]); break;
      }
   }
   return(type);
}
