/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie de fonctions utiles
 * Creation     : Janvier 2008
 * Auteur       : Jean-Philippe Gauthier
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
#include "EZGrid.h"
#include "eerUtils_build_info.h"

#define APP_NAME "EZTiler"
#define APP_DESC "SMC/CMC/EERS RPN fstd field tiler."

int TileVar(int FIdTo,int NI, int NJ,int Halo,int FIdFrom,char* Var,char* TypVar,char* Etiket,int DateV,int IP1,int IP2) {

#ifdef HAVE_RMN
   TRPNField *fld;
   int        idlst[RPNMAX],n,ni,nj,nk,nid;

   // Get the number of fields
   cs_fstinl(FIdFrom,&ni,&nj,&nk,DateV,Etiket,IP1,IP2,-1,TypVar,Var,idlst,&nid,RPNMAX);

   if (nid<=0) {
      App_Log(APP_ERROR,"TileVar: Specified fields do not exist");
      return(FALSE);
   }

   // Loop on all found fields
   for(n=0;n<nid;n++) {
      
      if (!(fld=RPN_FieldReadIndex(FIdFrom,idlst[n],NULL))) {
         App_Log(APP_ERROR,"TileVar: Problem loading field %i",idlst[n]);
         return(FALSE);
      }
       
      // On first pass, copy grid descriptor
      if (n==0)
         RPN_CopyDesc(FIdTo,&fld->Head);

      if (!RPN_FieldTile(FIdTo,fld->Def,&fld->Head,fld->GRef,fld->ZRef,0,NI,NJ,Halo,fld->Head.DATYP,-fld->Head.NBITS,FALSE,FALSE)) {
         App_Log(APP_ERROR,"TileVar: Unable to tile field %i",idlst[n]);         
         return(FALSE);
      }
      
      RPN_FieldFree(fld);
   }

   return(nid);
#endif
}


int Tile(char *In,char *Out,int Size,int Halo,char **Vars) {

#ifdef HAVE_RMN
   int  in,out,v=0;
   char *var;
   
   App_Log(APP_INFO,"Tiling file %s to %s\n",In,Out);
   if ((in=cs_fstouv(In,"STD+RND+R/O"))<0) {
      App_Log(APP_ERROR,"Problems opening input file %s\n",In);
      return(0);
   }

   if ((out=cs_fstouv(Out,"STD+RND+R/W"))<0) {
      App_Log(APP_ERROR,"Problems opening output file %s\n",Out);
      return(0);
   }

   if (!Vars[0]) {
      App_Log(APP_DEBUG,"Tiling everything\n");
      TileVar(out,Size,Size,Halo,in,"","","",-1,-1,-1);
   } else {
      while((var=Vars[v++])) {
         App_Log(APP_DEBUG,"Tiling var %s\n",var);
         TileVar(out,Size,Size,Halo,in,var,"","",-1,-1,-1);
      }
   }

   cs_fstfrm(in);
   cs_fstfrm(out);
#endif

   return(1);
}

int main(int argc, char *argv[]) {

   int      ok=0,size=0,halo=0,code=EXIT_FAILURE;
   char     *in=NULL,*out=NULL,*vars[APP_LISTMAX];

   TApp_Arg appargs[]=
      { { APP_CHAR,  &in,   1,             "i", "input",  "Input file" },
        { APP_CHAR,  &out,  1,             "o", "output", "Output file" },
        { APP_INT32, &size, 1,             "s", "size",   "Tile size in gridpoint" },
        { APP_INT32, &halo, 1,             "a", "halo",   "Halo size around the tiles ("APP_COLOR_GREEN"0"APP_COLOR_RESET",1 or 2)" },
        { APP_CHAR,  vars,  APP_LISTMAX-1, "n", "nomvar", "List of variable to process" },
        { 0 } };

   memset(vars,0x0,APP_LISTMAX*sizeof(vars[0]));
   App_Init(APP_MASTER,APP_NAME,VERSION,APP_DESC,BUILD_TIMESTAMP);

   if (!App_ParseArgs(appargs,argc,argv,APP_NOARGSFAIL|APP_ARGSLOG)) {
      exit(EXIT_FAILURE);      
   }
   
   /*Error checking*/
   if (in==NULL) {
      App_Log(APP_ERROR,"No input standard file specified\n");
      exit(EXIT_FAILURE);
   }
   if (out==NULL) {
      App_Log(APP_ERROR,"No output standard file specified\n");
      exit(EXIT_FAILURE);
   }
   if (!size) {
      App_Log(APP_ERROR,"No tile size specified\n");
      exit(EXIT_FAILURE);
   }

   /*Launch the app*/
   App_Start();
   ok=Tile(in,out,size,halo,vars);
   code=App_End(ok?-1:EXIT_FAILURE);
   App_Free();

   exit(code);
}
