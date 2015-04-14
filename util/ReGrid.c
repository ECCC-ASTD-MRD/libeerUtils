/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie de fonctions utiles
 * Creation     : Janvier 2015
 * Auteur       : Jean-Philippe Gauthier
 * Revision     : $Id$
 *
 * Description: RPN fstd interpolation test
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
#include "Def.h"
#include "GeoRef.h"
#include "RPN.h"

#define APP_NAME "ReGrid"
#define APP_DESC "SMC/CMC/EERS RPN fstd re-gridding tool."


int ReGrid(TApp *App,char *In,char *Out,char *Grid,char **Vars) {

   TRPNField *in,*grid,*out,*idx;
   int  fin,fout,fgrid,n;
   char *var,*tok;
   float *index=NULL;
   
  if ((fin=cs_fstouv(In,"STD+RND+R/O"))<0) {
      App_Log(App,ERROR,"Problems opening input file %s\n",In);
      return(0);
   }

   if ((fout=cs_fstouv(Out,"STD+RND+R/W"))<0) {
      App_Log(App,ERROR,"Problems opening output file %s\n",Out);
      return(0);
   }

   if ((fgrid=cs_fstouv(Grid,"STD+RND+R/O"))<0) {
      App_Log(App,ERROR,"Problems opening grid file %s\n",Grid);
      return(0);
   }

   if (!(grid=RPN_FieldRead(fgrid,-1,"",-1,-1,-1,"","P0"))) {
      App_Log(App,ERROR,"Problems reading grid field\n");
      return(0);    
   }

   if (!(in=RPN_FieldRead(fin,-1,"",-1,-1,-1,"",Vars[0]))) {
      App_Log(App,ERROR,"Problems reading input field\n");
      return(0);  
   }
 
   // Create index field
   if (!(idx=RPN_FieldNew(in->Def->NIJ*100,1,1,1,TD_Float32))) {
      return(0);       
   }
   
   memcpy(&idx->Head,&grid->Head,sizeof(TRPNHeader));
   strcpy(idx->Head.NOMVAR,"#%");
   idx->Head.NJ=1;
   idx->Head.GRTYP[0]='X';
   idx->Head.NBITS=32;
   idx->Head.DATYP=5;
   index=(float*)idx->Def->Data[0];

   // Initialize index to empty
   index[0]=DEF_INDEX_EMPTY;
   grid->Def->NoData=0.0;
 
   while(in->Head.KEY>0) {
      App_Log(App,INFO,"Processing %s %i\n",Vars[0],in->Head.KEY);

      // Reset result grid
      Def_Clear(grid->Def);
     
      // Proceed with interpolation
      if (!(idx->Head.NI=Def_GridInterpConservative(grid->Ref,grid->Def,in->Ref,in->Def,IR_CONSERVATIVE,TRUE,1,index))) {
         App_Log(App,ERROR,"Problems interpolating: %s\n",App_ErrorGet());
         return(0);    
      }
      
      // Write results
      RPN_CopyHead(&grid->Head,&in->Head);
      RPN_FieldWrite(fout,grid);
      
      if ((in->Head.KEY=cs_fstsui(fin,&in->Head.NI,&in->Head.NJ,&in->Head.NK))>0) {     
         if (!RPN_FieldReadIndex(fin,in->Head.KEY,in)) {
            return(0);
         }
      }
   }
   
   // Write index
   if (index[0]!=DEF_INDEX_EMPTY) {
      App_Log(App,DEBUG,"Saving index containing %i items\n",idx->Head.NI);
      
      if (!RPN_FieldWrite(fout,idx)) {
         return(0);
      }
   }
   
   cs_fstfrm(fin);
   cs_fstfrm(fout);
   cs_fstfrm(fgrid);

   return(1);
}

int main(int argc, char *argv[]) {

   TApp     *app;
   int      ok=0;
   char     *in=NULL,*out=NULL,*grid=NULL,*type=NULL,*vars[APP_LISTMAX];

   TApp_Arg appargs[]=
      { { APP_CHAR,  &in,   1,             "i", "input",  "Input file" },
        { APP_CHAR,  &out,  1,             "o", "output", "Output file" },
        { APP_CHAR,  &grid, 1,             "g", "grid",   "Grid file" },
        { APP_CHAR,  &type, 1,             "t", "type",   "Interpolation type ("APP_COLOR_GREEN"CONSERVATIVE"APP_COLOR_RESET",NORMALIZED_CONSERVATIVE)" },
        { APP_CHAR,  vars,  APP_LISTMAX-1, "n", "nomvar", "List of variable to process" },
        { APP_NIL } };

   memset(vars,0x0,APP_LISTMAX*sizeof(vars[0]));
   app=App_New(APP_NAME,VERSION,APP_DESC,__TIMESTAMP__);

   if (!App_ParseArgs(app,appargs,argc,argv,APP_NOARGSFAIL|APP_ARGSLOG)) {
      exit(EXIT_FAILURE);      
   }
   
   /*Error checking*/
   if (!in) {
      App_Log(app,ERROR,"No input standard file specified\n");
      exit(EXIT_FAILURE);
   }
   if (!out) {
      App_Log(app,ERROR,"No output standard file specified\n");
      exit(EXIT_FAILURE);
   }
   if (!grid) {
      App_Log(app,ERROR,"No grid standard file specified\n");
      exit(EXIT_FAILURE);
   }

   /*Launch the app*/
   App_Start(app);
   ok=ReGrid(app,in,out,grid,vars);
   App_End(app,ok!=1);
   App_Free(app);

   if (!ok) {
      exit(EXIT_FAILURE);
   } else {
      exit(EXIT_SUCCESS);
   }
}
