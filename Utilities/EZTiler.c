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
#include "EZTile.h"

#define APP_NAME "EZTiler"
#define APP_DESC "SMC/CMC/EERS RPN fstd field tiler."

int Tile(TApp *App,char *In,char *Out,int Size,char *Vars) {

   int  in,out;
   char *str,*tok;

   App_Log(App,INFO,"Tiling file %s to %s\n",In,Out);
   if ((in=cs_fstouv(In,"STD+RND+R/O"))<0) {
      App_Log(App,ERROR,"Problems opening input file %s\n",In);
      return(0);
   }

   if ((out=cs_fstouv(Out,"STD+RND+R/W"))<0) {
      App_Log(App,ERROR,"Problems opening output file %s\n",Out);
      return(0);
   }

   if (!Vars) {
      App_Log(App,DEBUG,"Tiling everything\n");
      EZGrid_Tile(out,Size,Size,2,in,"","","",-1,-1,-1);
   } else {
      str=Vars;
      while((tok=strtok(str," "))) {
         str=NULL;
         App_Log(App,DEBUG,"Tiling var %s\n",tok);
         EZGrid_Tile(out,Size,Size,2,in,tok,"","",-1,-1,-1);
      }
   }

   cs_fstfrm(in);
   cs_fstfrm(out);

   return(1);
}

int main(int argc, char *argv[]) {

   TApp     *app;
   int      ok=0,size=0;
   char     *in=NULL,*out=NULL,*val=NULL,*vars=NULL;

   TApp_Arg appargs[]=
      { { APP_CHAR,  (void**)&in,   "i", "input",  "Input file" },
        { APP_CHAR,  (void**)&out,  "o", "output", "Output file" },
        { APP_INT32, (void**)&size, "s", "size",   "Tile size in gridpoint" },
        { APP_CHAR,  (void**)&vars, "n", "nomvar", "List of variable to process" },
        { 0 } };

   app=App_New(APP_NAME,VERSION,APP_DESC,__TIMESTAMP__);

   if (!App_ParseArgs(app,appargs,argc,argv)) {
      exit(EXIT_FAILURE);      
   }
   
   /*Error checking*/
   if (in==NULL) {
      App_Log(app,ERROR,"No input standard file specified\n");
      exit(EXIT_FAILURE);
   }
   if (out==NULL) {
      App_Log(app,ERROR,"No output standard file specified\n");
      exit(EXIT_FAILURE);
   }
   if (!size) {
      App_Log(app,ERROR,"No tile size specified\n");
      exit(EXIT_FAILURE);
   }

   /*Launch the app*/
   App_Start(app);
   ok=Tile(app,in,out,size,vars);
   App_End(app,ok==1);
   App_Free(app);

   if (!ok) {
      exit(EXIT_FAILURE);
   } else {
      exit(EXIT_SUCCESS);
   }
}
