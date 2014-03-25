/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de fonctions utiles
 * Creation  : Octobre 2012
 * Auteur    : Jean-Philippe Gauthier
 *
 * Description: Coder/Decoder of pool information into/from RPN fields
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
#include "EZTile.h"

#define NAME    "CodeInfo"
#define DESC    "Coder/Decoder of pool information into/from RPN fields\n\n"
#define CMDLINE "Usage:\n\t-n, --nomvar\t variable name\n\t-i, --info\t input pool file\n\t-f, --fstd\t RPN standard file\n\t-c, --code\t Encodage\n\t-d, --decode\t Decodage\n\t-v, --verbose\t verbose [0-3]\n\t-l, --log\t log file\n"

/*----------------------------------------------------------------------------
 * Nom      : <Codec>
 * Creation : Octobre 2012 - J.P. Gauthier
 *
 * But      : Code/Decode pool information from/to an RPN field.
 *
 * Parametres  :
 *  <App>      : Application parameters
 *  <Pool>     : ASCII pool file
 *  <FST>      : RPN FST file
 *  <Var>      : NOMVAR of the pool field
 *  <Code>     : Code or decode flag
 *
 * Retour:
 *  <Done>     : Finished (0,1)
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int Codec(TApp *App,char *Pool,char *FST,char *Var,int Code) {

   char       buf[APP_BUFMAX],*c;
   int        fld[APP_BUFMAX],len,err,fstid,i;
   FILE      *fid=NULL;
   TRPNHeader h;

   if (Code) {
      if (!(fid=fopen(Pool,"r"))) {
         App_Log(App,ERROR,"Unable to open input file (%s)\n",Pool);
         return(0);
      }

      // Get the pool line
      c=fgets(buf,APP_BUFMAX,fid);

      // Get rid of trailing spaces
      strtrim(buf,' ');
      len=strlen(buf);

      // Get rid of trailing \n
      if (buf[len-1]=='\n') len--;
      
      App_Log(App,INFO,"Encoding %i character\n",len);
      for(i=0;i<len;i++) {
         fld[i]=buf[i];
      }
      
      if ((fstid=cs_fstouv(FST,"STD+RND+R/W"))<0) {
         App_Log(App,ERROR,"Problems opening output file %s\n",FST);
         return(0);
      }
      App_Log(App,INFO,"Encoding into %s\n",FST);

      err=cs_fstecr(fld,-8,fstid,0,0,0,len,1,1,0,0,0,"X",Var,"DESCRIPTION","X",0,0,0,0,2,TRUE);
      if (err<0) {
         App_Log(App,ERROR,"Could not write encoded pool record\n");
         return(0);
      }
   } else {
      if ((fstid=cs_fstouv(FST,"STD+RND+R/O"))<0) {
         App_Log(App,ERROR,"Problems opening output file %s\n",FST);
         return(0);
      }
      err=cs_fstlir(fld,fstid,&h.NI,&h.NJ,&h.NK,-1,"",-1,-1,-1,"",Var);
      if (err<0) {
         App_Log(App,ERROR,"Could not find encoded pool record\n");
         return(0);
      }

      for(i=0;i<h.NI;i++) {
         buf[i]=fld[i];
      }
      buf[i]='\0';

      if (Pool && (fid=fopen(Pool,"w"))) {
         fprintf(fid,"%s",buf);
      } else {
         fprintf(stderr,"%s",buf);
      }
   }
   cs_fstfrm(fstid);

   return(1);
}

int main(int argc, char *argv[]) {

   TApp     *app;
   int       i=-1,ckey;
   char     *tok,*ptok=NULL,*env=NULL,*str,*pool,*val,*fst,*var;

   pool=val=fst=var=NULL;
   ckey=0;

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

      if (strcasecmp(tok,"-i")==0 || strcasecmp(tok,"--info")==0 || strcasecmp(tok,"-info")==0) {              // Pool file
         val=env?strtok(str," "):argv[++i];
         pool=val;
      } else if (strcasecmp(tok,"-f")==0 || strcasecmp(tok,"--fstd")==0 || strcasecmp(tok,"-fstd")==0) {       // FSTD file
         val=env?strtok(str," "):argv[++i];
         fst=val;
      } else if (strcasecmp(tok,"-n")==0 || strcasecmp(tok,"--nomvar")==0 || strcasecmp(tok,"-nomvar")==0) {   // NOMVAR to use
         val=env?strtok(str," "):argv[++i];
         var=val;
      } else if (strcasecmp(tok,"-c")==0 || strcasecmp(tok,"--code")==0 || strcasecmp(tok,"-ckey")==0) {      // Code key
         ++i;
         ckey=1;
      } else if (strcasecmp(tok,"-d")==0 || strcasecmp(tok,"--decode")==0 ) {                                 // Decode key
         ++i;
         ckey=0;
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
   if (fst==NULL) {
      App_Log(app,ERROR,"No standard file specified\n");
      exit(EXIT_FAILURE);
   }
   if (var==NULL) {
      var=strdup("INFO");
   }

   /*Launch the app*/
   App_Start(app);
   i=Codec(app,pool,fst,var,ckey);
   App_End(app,i==1);
   App_Free(app);

   if (!i) {
      exit(EXIT_FAILURE);
   } else {
      exit(EXIT_SUCCESS);
   }
}
