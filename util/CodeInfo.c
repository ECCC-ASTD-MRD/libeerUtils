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

#define APP_NAME "CodeInfo"
#define APP_DESC "SMC/CMC/EERS Coder/Decoder of pool information into/from RPN fields."

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
         App_Log(App,ERROR,"Unable to open information file (%s)\n",Pool);
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
   int       ok=0,code,decode,ckey;
   char     *pool,*val,*fst,*var;

   TApp_Arg appargs[]=
      { { APP_CHAR,  (void**)&pool,   "i", "info",   "Information file" },
        { APP_CHAR,  (void**)&fst,    "f", "fstd",   "RPN standard file file" },
        { APP_FLAG,  (void**)&code,   "c", "code",   "Code information into RPN record" },
        { APP_FLAG,  (void**)&decode, "d", "decode", "Decode information from RPN record" },
        { APP_CHAR,  (void**)&var,    "n", "nomvar", "Name of information variable ("APP_COLOR_GREEN"INFO"APP_COLOR_RESET")" },
        { 0 } };

   pool=val=fst=var=NULL;
   code=decode=ckey=0;

   app=App_New(APP_NAME,VERSION,APP_DESC,__TIMESTAMP__);

   if (!App_ParseArgs(app,appargs,argc,argv,APP_NOARGSFAIL)) {
      exit(EXIT_FAILURE);      
   }

   /*Error checking*/
   ckey=code?1:0;
   
   if (fst==NULL) {
      App_Log(app,ERROR,"No standard file specified\n");
      exit(EXIT_FAILURE);
   }
   if (var==NULL) {
      var=strdup("INFO");
   }

   /*Launch the app*/
   App_Start(app);
   ok=Codec(app,pool,fst,var,ckey);
   App_End(app,ok==1);
   App_Free(app);

   if (!ok) {
      exit(EXIT_FAILURE);
   } else {
      exit(EXIT_SUCCESS);
   }
}
