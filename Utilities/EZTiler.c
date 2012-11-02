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
#define CMDLINE "Bad option: %s\nUsage:\n\t-v, --verbose\t verbose [0-3]\n\t-i, --input\t input file\n\t-o, --output\t output file\n\t-l, --log\t log file\n"

int Tile(TApp *App,char *In,char *Out,int Size,char *Vars) {

   TGrid *fld0,*fld1,*fldn;
   float  val,vals[58];
   int    i,j,n=0,type,in,out;
   char  *str,*tok;

   App_Log(App,INFO,"Tiling file %s to %s\n",In,Out);
   if ((in=cs_fstouv(In,"STD+RND+R/O"))<0) {
      App_Log(App,ERROR,"Problems opening input file %s\n",In);
      return(0);
   }

   if ((out=cs_fstouv(Out,"STD+RND+R/W"))<0) {
      App_Log(App,ERROR,"Problems opening output file %s\n",Out);
      return(0);
   }

//   fld0=EZGrid_Read(10,"GZ","","",-1,12000,-1,1);
//   EZGrid_TileGrid(20,i,j,2,fld0);

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

   /*Test*/
   c_fnom(10,"/data/cmoex7/afsr005/tile/glb/2008010712_000","STD+RND+R/O",0);
   c_fstouv(10,"RND");

   /*Read the field (not really)*/
   fld0=EZGrid_Read(10,"GZ","","",-1,12000,-1,1);

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

int main(int argc, char *argv[]) {

   TApp     *app;
   int       i=-1,size=0;
   char     *tok,*env=NULL,*str,*in,*out,*val,*vars;

   in=out=NULL;

   str=env=getenv("APP_PARAMS");

   if (argc==1 && !env) {
      printf(DESC,CMDLINE,"");
      printf(CMDLINE,"");
      exit(EXIT_FAILURE);
   }

   app=App_New(NAME,VERSION);

   // Parse parameters either on command line or through environment variable
   i=1;
   while((i<argc && (tok=argv[i])) || (env && (tok=strtok(str," ")))) {
      str=NULL;

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
         printf(CMDLINE,tok);
         exit(EXIT_FAILURE);
      }
      ++i;
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
