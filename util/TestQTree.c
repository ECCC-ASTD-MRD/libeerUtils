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
#include "QTree.h"

void process(void *Data) {
   
   fprintf(stdout,"%s\n",(char*)Data);   
}

int main(int argc, char *argv[]) {

   TQTree         *tree,*node;
   TQTreeIterator *it;
   int            depth;
   
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
   QTree_Parse(tree,NULL,0);
   QTree_Parse(tree,process,0);

   // Find the node at a specific point
   if ((node=QTree_Find(tree,20,20))) {
      fprintf(stdout,"%i data in node\n",node->NbData);
   } else {
      fprintf(stdout,"Out of tree\n");
   }
   
   // Iterate over each leaf
   it=QTree_IteratorNew();
   
   node=QTree_Iterate(tree,it);
   QTree_Parse(node,NULL,0);

   while ((node=QTree_Iterate(NULL,it))) {
      QTree_Parse(node,NULL,0);
   }
 }