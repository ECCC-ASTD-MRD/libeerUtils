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

#define NAME    "EZTiler"
#define DESC    "RPN fstd field tiler\n\n"
#define CMDLINE "Usage:\n\t-v, --verbose\t verbose [0-3]\n\t-i, --input\t input file\n\t-o, --output\t output file\n\t-l, --log\t log file\n"

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
   int       i=-1,size=0;
   char     *tok,*ptok=NULL,*env=NULL,*str,*in,*out,*val,*vars;

   in=out=NULL;

   str=env=getenv("APP_PARAMS");

   if (argc==1 && !env) {
      printf(DESC);
      printf(CMDLINE);
      exit(EXIT_FAILURE);
   }

   app=App_New(NAME,VERSION);

   // Parse parameters either on command line or through environment variable
   i=1;
   while((i<argc && (tok=argv[i])) || (env && (tok=strtok(str," ")))) {
      str=NULL;

      // Check if token is a flag or a value (for multi-value parameters)
      if (tok[0]!='-') {
         tok=ptok;
         --i;
      }

      if (strcasecmp(tok,"-i")==0 || strcasecmp(tok,"--input")==0) {             // Input file
         val=env?strtok(str," "):argv[++i];
         in=val;
      } else if (strcasecmp(tok,"-o")==0 || strcasecmp(tok,"--output")==0) {     // Output file
         val=env?strtok(str," "):argv[++i];
         out=val;
      } else if (strcasecmp(tok,"-s")==0 || strcasecmp(tok,"--size")==0) {       // Tile size
         val=env?strtok(str," "):argv[++i];
         size=atoi(val);
      } else if (strcasecmp(tok,"-n")==0 || strcasecmp(tok,"--nomvar")==0) {     // variables
         vars=env?strtok(str," "):argv[++i];
      } else if (strcasecmp(tok,"-l")==0 || strcasecmp(tok,"--log")==0) {        // Log file
         val=env?strtok(str," "):argv[++i];
         app->LogFile=val;
      } else if (strcasecmp(tok,"-v")==0 || strcasecmp(tok,"--verbose")==0) {    // Verbose degree
         val=env?strtok(str," "):argv[++i];
         App_LogLevel(app,val);
      } else {
         printf(APP_BADOPTION,tok);
         printf(CMDLINE);
         exit(EXIT_FAILURE);
      }
      ++i;
      ptok=tok;
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
   i=Tile(app,in,out,size,vars);
   App_End(app,i==1);
   App_Free(app);

   if (!i) {
      exit(EXIT_FAILURE);
   } else {
      exit(EXIT_SUCCESS);
   }
}
