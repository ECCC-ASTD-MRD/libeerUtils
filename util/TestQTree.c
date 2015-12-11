/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de fonctions utiles
 * Creation     : Janvier 2008
 * Auteur       : Jean-Philippe Gauthier
 * Revision     : $Id$
 *
 * Description: QTree tester
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
#include "GeoRef.h"
#include "RPN.h"
#include "Def.h"
#include "QTree.h"
#include "Triangle.h"

#define APP_NAME "TestQTree"
#define APP_DESC "QTree testing tool."

void process(void *Data) {
   
   fprintf(stdout,"Tree parse proc test: %s\n",(char*)Data);   
}

int QTree_TestBasic(void) {
   
   TQTree         *tree,*node;
   TQTreeIterator *it;
   int            depth;
   
   App_Log(INFO,"Tree build test:\n");
   tree=QTree_New(-100,-100,100,100,NULL);
   
   depth=QTREE_INFINITE;
//   depth=2;

   // Add some data
   QTree_Add(tree,-20,-20,depth,"Yoman 1");
   QTree_Add(tree,-18,-29,depth,"Yoman 1");
   QTree_Add(tree,20,-20,depth,"Yoman 2");
   QTree_Add(tree,20,20,depth,"titi");
   QTree_Add(tree,22,22,depth,"toto");
   QTree_Add(tree,22,22,depth,"toto2");
   
   // Parse the tree
   App_Log(INFO,"Tree printout test:\n");
   QTree_Parse(tree,NULL,0);
   QTree_Parse(tree,process,0);

   // Find the node at a specific point
   if ((node=QTree_Find(tree,20,20))) {
      fprintf(stdout,"Found %i data in node\n",node->NbData);
   } else {
      fprintf(stdout,"Out of tree\n");
   }
   
   // Iterate over each leaf
   App_Log(INFO,"Tree iterator test:\n");
   it=QTree_IteratorNew();
   
   node=QTree_Iterate(tree,it);
   QTree_Parse(node,NULL,0);

   while ((node=QTree_Iterate(NULL,it))) {
      QTree_Parse(node,NULL,0);
   }
}

TQTree* EZGrid_Build(TGeoRef *Ref) {

   unsigned int  n,nt,res=0;
   unsigned int *idx;
   double        lat0,lon0,lat1,lon1;
   Vect2d        tr[3];
   TQTree       *tree;
   
   // Check data limits   
   lat0=lon0=1e10;
   lat1=lon1=-1e10;
   
   for(n=0;n<Ref->NX;n++) {
      lat0=FMIN(lat0,Ref->Lat[n]);
      lon0=FMIN(lon0,Ref->Lon[n]);
      lat1=FMAX(lat1,Ref->Lat[n]);
      lon1=FMAX(lon1,Ref->Lon[n]);
   }
      
   // Create the tree on the data limits
   if (!(tree=QTree_New(lat0,lon0,lat1,lon1,NULL))) {
      App_Log(ERROR,"%s: failed to create QTree index\n",__func__);
      return(NULL);
   }

   // Loop on triangles
   idx=Ref->Idx;
   for(nt=0;nt<Ref->NIdx-3;nt+=3) {     
      
      tr[0][0]=Ref->Lat[idx[nt]];     tr[0][1]=Ref->Lon[idx[nt]];
      tr[1][0]=Ref->Lat[idx[nt+1]];   tr[1][1]=Ref->Lon[idx[nt+1]];
      tr[2][0]=Ref->Lat[idx[nt+2]];   tr[2][1]=Ref->Lon[idx[nt+2]];
      
      // Put it in the quadtree, in any child nodes intersected
      res=8;
      if (!QTree_AddTriangle(tree,tr,res,&Ref->Idx[nt])) {
         App_Log(ERROR,"%s: failed to add node\n",__func__);
         return(NULL);
      }      
   }
   
   return(tree);
}

int LLGetValue(TQTree *Tree,TGeoRef *Ref,float *Data,double Lat,double Lon,float *Value) {
   
   TQTree       *node;
   Vect3d        bary;
   int           n,t,i;
   unsigned int *idx;
   
   struct timeval time0,time1;
   gettimeofday(&time0,NULL);

   // Find enclosing triangle
   if ((node=QTree_Find(Tree,Lat,Lon)) && node->NbData) {
      
      // Loop on this nodes data payload
      for(n=0;n<node->NbData;n++) {
         idx=(unsigned int*)QTree_GetData(node,n);
         
         // if the Barycentric coordinates are within this triangle, get its interpolated value
         if (Bary_Get(bary,Lat,Lon,Ref->Lat[*idx],Ref->Lon[*idx],Ref->Lat[*(idx+1)],Ref->Lon[*(idx+1)],Ref->Lat[*(idx+2)],Ref->Lon[*(idx+2)])) {
            *Value=Bary_Interp(bary,Data[*idx],Data[*(idx+1)],Data[*(idx+2)]);                    

            gettimeofday(&time1,NULL);
            System_TimeValSubtract(&time0,&time1,&time0);
            fprintf(stderr,"(TIME) %.8f s\n",time0.tv_sec+((double)time0.tv_usec)*1e-6);
            
            return(TRUE);
         }
      }
   }
      
   // We must be out of the tin
   return(FALSE);
}

//#define TINFILE "/home/afsr/005/public_html/SPI/Script/DataOut/FSTD_TIN2FSTD.fstd"
#define TINFILE "/fs/cetus/fs2/ops/cmoe/afsr005/Projects/SPILL/Cornwall/cornwall.fst"
//#define TINFILE "/fs/cetus/fs2/ops/cmoe/afsr005/Projects/SPILL/Cornwall/cnd8500/2015072306_012"

int QTree_TestTIN(void) {

   TQTree   *tree;
   TGeoRef   ref;
   TRPNField *in;
   int  fin,ni,nj,nk;
   float value;
   
   if ((fin=cs_fstouv(TINFILE,"STD+RND+R/O"))<0) {
      App_Log(ERROR,"Problems opening input file %s\n",TINFILE);
      return(0);
   }

   if (!(in=RPN_FieldRead(fin,-1,"",-1,-1,-1,"","UUW"))) {
      App_Log(ERROR,"Problems reading tin field\n");
      return(0);    
   }
   
//   ref.NIdx=76545;
   ref.NIdx=197139;
   ref.Idx=(unsigned int*)malloc(ref.NIdx*sizeof(unsigned int));
//   ref.NX=13430;
   ref.NX=135010;
   ref.Lat=(float*)malloc(ref.NX*sizeof(float));
   ref.Lon=(float*)malloc(ref.NX*sizeof(float));

   cs_fstlir(ref.Lat,fin,&ni,&nj,&nk,-1,"",-1,-1,-1,"","^^");
   cs_fstlir(ref.Lon,fin,&ni,&nj,&nk,-1,"",-1,-1,-1,"",">>");
   cs_fstlir(ref.Idx,fin,&ni,&nj,&nk,-1,"",-1,-1,-1,"","##");

   tree=EZGrid_Build(&ref);  
//   QTree_Parse(tree,NULL,0);

   
   App_Log(INFO,"Testing Cornwall domain:\n");
   LLGetValue(tree,&ref,(float*)in->Def->Data[0],45.0084972,-74.7374668,&value);
   App_Log(INFO,"   Tree TIN value test: %.4f == 1.8570\n",value);

   LLGetValue(tree,&ref,(float*)in->Def->Data[0],45.0081266,-74.7395578,&value);
   App_Log(INFO,"   Tree TIN value test: %.4f == 0.0198\n",value);

   if (!LLGetValue(tree,&ref,(float*)in->Def->Data[0],45.0081325,-74.7396231,&value)) {
      App_Log(INFO,"   Tree TIN value test outside: OK\n");
   }

   
   
   
   App_Log(INFO,"Testing Hudson bay domain:\n");
   LLGetValue(tree,&ref,(float*)in->Def->Data[0],59.58349,-82.84078,&value);
   App_Log(INFO,"   Tree TIN value test: %.2f == 229.66\n",value);
 
   LLGetValue(tree,&ref,(float*)in->Def->Data[0],61.00896,-66.35393,&value);
   App_Log(INFO,"   Tree TIN value test: %.2f == 802.84\n",value);

   LLGetValue(tree,&ref,(float*)in->Def->Data[0],62.70096,-78.11559,&value);
   App_Log(INFO,"   Tree TIN value test: %.2f == 425.89\n",value);
   
   if (!LLGetValue(tree,&ref,(float*)in->Def->Data[0],62.43475,-77.47023,&value)) {
      App_Log(INFO,"   Tree TIN value test outside: OK\n");
   }
   if (!LLGetValue(tree,&ref,(float*)in->Def->Data[0],62.66643,-74.15499,&value)) {
      App_Log(INFO,"   Tree TIN value test outside: OK\n");
   }
}

#define XC 700.0
#define YC 300.0
#define QRES    100
#define QDIM    1000
#define QSAMPLE 10000000

int QTree_TestCloud(void) {
   
   TQTree   *tree,*node;
   int      axy,d,x,y,xd,yd,n,nc,nn,res=9;
   double   l,len,dx,dy,rx,ry,nx,ny;
   Vect2d  *pts;
   struct timeval time0,time1;
   time_t  t;
   
   App_Log(INFO,"Testing point cloud with %i samples:\n",QSAMPLE);
   
   // Create the array on the data limits
   if (!(tree=(TQTree*)calloc((QRES+1)*(QRES+1),sizeof(TQTree)))) {
      App_Log(ERROR,"%s: failed to create QTree index\n",__func__);
      return(0);
   }
     
   // Add random data
   dy=QDIM/QRES;
   dx=QDIM/QRES;
      
   // Loop on points
   pts=(Vect2d*)malloc(QSAMPLE*sizeof(Vect2d));
//   srand((unsigned) time(&t));
   for(n=0;n<QSAMPLE;n++) {     
            
      pts[n][0]=(double)rand()/RAND_MAX*QDIM;
      pts[n][1]=(double)rand()/RAND_MAX*QDIM;
      x=pts[n][0]/dx;
      y=pts[n][1]/dy;

      node=&tree[y*QRES+x];
      QTree_AddData(node,pts[n][0],pts[n][1],(void*)(n+1));
   }
   
   // Search loop
   gettimeofday(&time0,NULL);
   
   axy=-1;
   l=len=1e32;
   for(nn=0;nn<QSAMPLE;nn++) {     
      nx=XC-pts[nn][0];
      ny=YC-pts[nn][1];
      l=nx*nx+ny*ny;
      if (l<len) {
         len=l;
         axy=nn;
      }     
   }
   gettimeofday(&time1,NULL);
   System_TimeValSubtract(&time0,&time1,&time0);
   App_Log(INFO,"   Loop mode: %.5f s, closest is (%f,%f)\n",time0.tv_sec+((double)time0.tv_usec)*1e-6,pts[axy][0],pts[axy][1]);
   
   // Search Circling
   gettimeofday(&time0,NULL);

   d=0;
   axy=-1;
   l=len=1e32;
   xd=(XC)/dx;
   yd=(YC)/dy;

   while(1) {
      for(y=yd-d;y<=yd+d;y++) {
          if (y<0)    continue;
         if (y>QRES) break;
         
         for(x=xd-d;x<=xd+d;x+=((y==yd-d||y==yd+d)?1:(d+d))) {
            if (x<0)    continue;
            if (x>QRES) break;
            node=&tree[y*QRES+x];

            for(n=0;n<node->NbData;n++) {
               nn=(int)node->Data[n].Ptr-1;
               nx=XC-pts[nn][0];
               ny=YC-pts[nn][1];
               l=nx*nx+ny*ny;
               if (l<len) {
                  len=l;
                  axy=nn;
               }
            }
         }            
      }
      if (axy>-1 && d) 
         break;
      d++;
   }
   
   gettimeofday(&time1,NULL);
   System_TimeValSubtract(&time0,&time1,&time0);
   App_Log(INFO,"   Circ mode: %.5f s, closest is (%f,%f)\n",time0.tv_sec+((double)time0.tv_usec)*1e-6,pts[axy][0],pts[axy][1]);
}

int main(int argc, char *argv[]) {

   int      ok=TRUE;
   
   App_Init(APP_MASTER,APP_NAME,VERSION,APP_DESC,__TIMESTAMP__);

   App_Start();
   
   QTree_TestBasic();
   QTree_TestTIN();
   QTree_TestCloud();
   
   App_End(ok!=1);
   App_Free();

   if (!ok) {
      exit(EXIT_FAILURE);
   } else {
      exit(EXIT_SUCCESS);
   }
}
