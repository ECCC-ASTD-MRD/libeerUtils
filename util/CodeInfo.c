/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Librairie de fonctions utiles
 * Creation     : Octobre 2012
 * Auteur       : Jean-Philippe Gauthier
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
#include "RPN.h"
#include "eerUtils_build_info.h"

#define APP_NAME "CodeInfo"
#define APP_DESC "SMC/CMC/EERS Coder/Decoder of pool information into/from RPN fields."

/*----------------------------------------------------------------------------
 * Nom      : <Codec>
 * Creation : Octobre 2012 - J.P. Gauthier
 *
 * But      : Code/Decode pool information from/to an RPN field.
 *
 * Parametres  :
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
int Codec(char *Pool,char *FST,char *Var,int Code) {

   int code=APP_ERR;

 #ifdef HAVE_RMN
   char      *buf=NULL;
   int       *fld=NULL,fstid,fldid,i,len,err;
   long       size;
   FILE      *fid=NULL;
   TRPNHeader h;

   if (Code) {
      if (!(fid=fopen(Pool,"r"))) {
         App_Log(APP_ERROR,"Unable to open information file (%s)\n",Pool);
         goto end;
      }

      // Get the pool size
      fseek(fid,0,SEEK_END);
      size=ftell(fid);
      fseek(fid,0,SEEK_SET);

      buf=(char*) malloc(size+1);
      fld=(int*) malloc(sizeof(int)*(size+1));

      // Get the pool line
      if (fgets(buf,size+1,fid)) {
         buf[size+1]='\0';

         // Get rid of trailing spaces
         strtrim(buf,' ');
         len=strlen(buf);

         // Get rid of trailing \n
         if (buf[len-1]=='\n') len--;

         App_Log(APP_INFO,"Encoding %i character\n",len);
         for(i=0;i<len;i++) {
            fld[i]=buf[i];
         }

         if ((fstid=cs_fstouv(FST,"STD+RND+R/W"))<0) {
            App_Log(APP_ERROR,"Problems opening output file %s\n",FST);
            goto end;
         }
         App_Log(APP_INFO,"Encoding into %s\n",FST);

         err=cs_fstecr(fld,-8,fstid,0,0,0,len,1,1,0,0,0,"X",Var,"DESCRIPTION","X",0,0,0,0,2,TRUE);
         if (err<0) {
            App_Log(APP_ERROR,"Could not write encoded pool record\n");
            goto end;
         }
      }
   } else {
      if ((fstid=cs_fstouv(FST,"STD+RND+R/O"))<0) {
         App_Log(APP_ERROR,"Problems opening output file %s\n",FST);
         goto end;
      }

      // Find the record
      fldid=cs_fstinf(fstid,&h.NI,&h.NJ,&h.NK,-1,"",-1,-1,-1,"",Var);
      buf=(char*) malloc(h.NI+1);
      fld=(int*) malloc(sizeof(int)*(h.NI+1));
      err=cs_fstluk(fld,fldid,&h.NI,&h.NJ,&h.NK);
      fld[h.NI+1]='\0';
      if (err<0) {
         App_Log(APP_ERROR,"Could not find encoded pool record\n");
         goto end;
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

   code=APP_OK;
end:
   if (buf) {
      free(buf);
      free(fld);
   }
#endif

   return(code);
}

int main(int argc, char *argv[]) {

   int       ok=0,code,decode,ckey;
   char     *pool,*val,*fst,*var;

   TApp_Arg appargs[]=
      { { APP_CHAR,  &pool,   1, "i", "info",   "Information file" },
        { APP_CHAR,  &fst,    1, "f", "fstd",   "RPN standard file file" },
        { APP_FLAG,  &code,   1, "c", "code",   "Code information into RPN record" },
        { APP_FLAG,  &decode, 1, "d", "decode", "Decode information from RPN record" },
        { APP_CHAR,  &var,    1, "n", "nomvar", "Name of information variable ("APP_COLOR_GREEN"INFO"APP_COLOR_RESET")" },
        { 0 } };

   pool=val=fst=var=NULL;
   code=decode=ckey=0;

   App_Init(APP_MASTER,APP_NAME,VERSION,APP_DESC,BUILD_TIMESTAMP);

   if (!App_ParseArgs(appargs,argc,argv,APP_NOARGSFAIL|APP_ARGSLOG)) {
      exit(EXIT_FAILURE);      
   }

   /*Error checking*/
   ckey=code?1:0;

   if (fst==NULL) {
      App_Log(APP_ERROR,"No standard file specified\n");
      exit(EXIT_FAILURE);
   }
   if (var==NULL) {
      var=strdup("INFO");
   }

   /*Launch the app*/
   App_Start();
   ok=Codec(pool,fst,var,ckey);
   code=App_End(ok?-1:EXIT_FAILURE);
   App_Free();

   exit(code);
}
