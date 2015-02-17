/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Project      : Generic 2D Quad Tree library
 * Creation     : November 2008
 * Author       : Gilles Mercier && J.P. Gauthier- CMC/CMOE
 * Revision     : $Id$
 *
 * Description: Library of functions to store data(ex. geo-referenced
 *              Finite Element data) information in a 2D Quad Tree
 *              structure. Generic Void pointers are used to access
 *              the data information.
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

#include <malloc.h>
#include "QTree.h"

/*----------------------------------------------------------------------------
 * Name      : <QTree_Inside>
 * Creation : November 2008 - G.Mercier - CMC/CMOE
 *
 * Purpose  : Check if a XY data position is inside the bounding box
 *            of a TQTree object.
 *
 * Args :
 *   <Node> : TQTree object pointer.
 *   <X>    : X data position.
 *   <Y>    : Y data position.
 *
 * Return:
 *  <int> : 0(False) if outside, not 0(True) if inside.
 *
 * Remarks : 
 *----------------------------------------------------------------------------
 */
static inline int QTree_Inside(TQTree * const Node,double X,double Y) {

  return(X>=Node->BBox[0].X && X<=Node->BBox[1].X && Y<=Node->BBox[1].Y && Y>=Node->BBox[0].Y);
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_FindChild>
 * Creation : November 2008 - G.Mercier - CMC/CMOE
 *
 * Purpose  : Find the node of a tree which contains
 *            the data XY position in it's bounding box.
 *
 * Args :
 *   <Node> : TQTree object pointer.
 *   <X>    : X data position.
 *   <Y>    : Y data position.
 *
 * Return:
 *  <TQTree*> : The pointer to the node  
 *
 * Remarks : 
 *----------------------------------------------------------------------------
 */
static inline TQTree* QTree_FindChild(const TQTree* const restrict Node,double X,double Y) {

  int cidx=0;

  while(!QTree_Inside(Node->Childs[cidx++],X,Y));

  return(Node->Childs[cidx-1]);
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_Split>
 * Creation : November 2008 - G.Mercier - CMC/CMOE
 *
 * Purpose  : Create and initialize all sub-quads objects of a quad.
 *
 * Args :
 *   <Node>   : TQTree object pointer to split.
 *   <Center> : Point2D object pointer XY center.
 *
 * Return:
 *  <Bool>   : False on failed allocation  
 *
 * Remarks : 
 *      
 *----------------------------------------------------------------------------
 */
static inline int QTree_Split(TQTree* const restrict Node,const TPoint2D* const restrict Center) {
   
   Node->Childs[0] = QTree_New(Node->BBox[0].X,Node->BBox[0].Y, Center->X, Center->Y,Node);
   Node->Childs[1] = QTree_New(Node->BBox[0].X,Center->Y,Center->X, Node->BBox[1].Y,Node);
   Node->Childs[2] = QTree_New(Center->X,Node->BBox[0].Y,Node->BBox[1].X, Center->Y,Node); 
   Node->Childs[3] = QTree_New(Center->X,Center->Y,Node->BBox[1].X, Node->BBox[1].Y,Node);

   return(Node->Childs[0] && Node->Childs[1] && Node->Childs[2] && Node->Childs[3]);
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_AddData>
 * Creation : November 2014 - J.P. Gauthier - CMC/CMOE
 *
 * Purpose  : Add data information of a node.
 *
 * Args :
 *   <Node>   : QTree node object pointer.
 *   <X>      : X data position.
 *   <Y>      : Y data position.
 *   <Data>   : data pointer.
 *
 * Return:
 *  <Bool>    : False on failed allocation  
 *
 * Remarks : -
 *              
 *----------------------------------------------------------------------------
 */
static inline int QTree_AddData(TQTree* const restrict Node,double X,double Y,void *Data) {

  if (Data) {
     if (!(Node->Data=(TQTreeData*)realloc(Node->Data,(Node->NbData+1)*sizeof(TQTreeData)))) {
        return(0);
     }
     Node->Data[Node->NbData].Ptr=Data;
     Node->Data[Node->NbData].Pos.X=X;
     Node->Data[Node->NbData].Pos.Y=Y;
     Node->NbData++;
  }
  
  return(1);
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_DelData>
 * Creation : November 2008 - G.Mercier - CMC/CMOE
 *
 * Purpose  : Delete the data information of a QTree node.
 *
 * Args :
 *   <Node> : TQTree object pointer with some data info..
 *
 * Return:
 *
 * Remarks :
 *      
 *----------------------------------------------------------------------------
 */
static inline void QTree_DelData(TQTree* const Node) {

   Node->NbData=0;
 
   if (Node->Data) {
      free(Node->Data);
      Node->Data=NULL;
   }
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_New>
 * Creation : November 2008 - G.Mercier - CMC/CMOE
 *
 * Purpose  : Create and initialize a new TQTree object.
 *
 * Args :
 *   <X0>   : South West X bounding box limit for the quad.
 *   <Y0>   : South West Y bounding box limit for the quad.
 *   <X1>   : North East X bounding box limit for the quad.
 *   <Y1>   : North East Y bounding box limit for the quad.
 *
 * Return:
 *  <TQTree*> : New and initialized TQTree object pointer.  
 *
 * Remarks
 *
 *----------------------------------------------------------------------------
 */
TQTree* QTree_New(double X0,double Y0,double X1,double Y1,TQTree *Parent) {

   TQTree *node=NULL;
   
   if ((node=(TQTree*)malloc(sizeof(TQTree)))) {

      node->NbData    = 0;
      node->Data      = NULL;

      node->Parent    = Parent;
      node->Childs[0] =
      node->Childs[1] =
      node->Childs[2] =
      node->Childs[3] = NULL;

      node->BBox[0].X = X0;
      node->BBox[0].Y = Y0;
      node->BBox[1].X = X1;
      node->BBox[1].Y = Y1;
   }
   
   return(node);
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_Add>
 * Creation : November 2008 - G.Mercier - CMC/CMOE
 *
 * Purpose  : Add some data to a TQTree object(usually a head quad at level=1...).
 *
 * Args :
 *   <Node>     : TQTree object pointer which we want to add data information (cannot be NULL).
 *   <X>        : X data position.
 *   <Y>        : Y data position.
 *   <MaxDepth> : Maximum depth of tree
 *   <Data>     : External Data pointer.
 *
 * Return:
 *  <TQTree *> : A TQTree object pointer(Could be NULL if a problem arise).  
 *
 * Remarks : 
 *----------------------------------------------------------------------------
 */
TQTree* QTree_Add(TQTree* restrict Node,double X,double Y,unsigned int MaxDepth,void* restrict Data) {

   int      d,ok=1;
   TPoint2D center;
   TQTree  *new=Node;

   if (!(MaxDepth--)) {
       // We've hit maximum depth, add to this node
       ok=QTree_AddData(Node,X,Y,Data);
   } else {
      if (Node->Childs[0]) {

         // This is a not a leaf, find sub node recursively
         new = QTree_FindChild(Node,X,Y);
         new = QTree_Add(new,X,Y,MaxDepth,Data);

      } else {

         if (!Node->NbData) {
            // Empty leaf, use it
            ok=QTree_AddData(Node,X,Y,Data);  
         } else {
            // Check if it's same coordinate to avoid infinite recursion
            if (Node->Data[0].Pos.X==X && Node->Data[0].Pos.Y==Y) {
               ok=QTree_AddData(Node,X,Y,Data);
            } else {
            
               // Leaf node but there is data already stored here, split.
               center.X = Node->BBox[0].X+(Node->BBox[1].X-Node->BBox[0].X)/2.0;
               center.Y = Node->BBox[0].Y+(Node->BBox[1].Y-Node->BBox[0].Y)/2.0;

               if (!QTree_Split(Node,&center)) {
                  return(NULL);
               }
                     
               // Reassign previous node data into splitted node
               new = QTree_FindChild(Node,Node->Data[0].Pos.X,Node->Data[0].Pos.Y);
               new->Data=Node->Data;
               new->NbData=Node->NbData;
               Node->Data=NULL;
               Node->NbData=0;
               
               // Assign new data
               new = QTree_FindChild(Node,X,Y);
               new = QTree_Add(new,X,Y,MaxDepth,Data);
            }
         }
      }
   }

   return(ok?new:NULL);
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_Del>
 * Creation : November 2008 - G.Mercier - CMC/CMOE
 *
 * Purpose  : Delete(free) all nodes of a TQTree object.
 *
 * Args :
 *   <Node> : TQTree object pointer to release.
 *
 * Return: - 
 *
 * Remarks :
 *----------------------------------------------------------------------------
 */
void QTree_Del(TQTree* restrict Node) {
   
   int c;
   
   if (Node) {

      for(c=0;c<4;c++) {
         if (Node->Childs[c]) {
            QTree_Del(Node->Childs[c]);
            free(Node->Childs[c]); 
            Node->Childs[c]=NULL;
         }
      }
      QTree_DelData(Node);
   }
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_Find>
 * Creation : November 2008 - G.Mercier - CMC/CMOE
 *
 * Purpose  : Find the leaf node for an abitrary XY position.
 *
 * Args :
 *   <Node>   : TQTree object pointer.
 *   <X>      : X position
 *   <Y>      : Y position
 *
 * Return:
 *  <TQTree*> : Child node
 *       
 * Remarks :   
 *----------------------------------------------------------------------------
 */
TQTree* QTree_Find(TQTree* restrict Node,double X,double Y) {

   int     cidx=0;
   TQTree* node=Node;

   if (Node->Childs[0]) {

      // Navigate through the quad tree recursively to find the terminal quad
      // which bounding box contains the XY position X,Y.
      while (cidx<4 && !QTree_Inside(Node->Childs[cidx++],X,Y));

      node=QTree_Find(Node->Childs[cidx-1],X,Y);
   }

   return(node);
}

/*----------------------------------------------------------------------------
 * Name     : <Tree_IteratorNew>
 * Creation : November 2014 - J.P. Gauthier - CMC/CMOE
 *
 * Purpose  : Initialiaze a new iterator
 *
 * Args :
 *   <Iter>  : Tree iterator
 *
 * Return:
 *
 * Remarks : 
 *----------------------------------------------------------------------------
 */
TQTreeIterator* QTree_IteratorNew(void) {
   
   TQTreeIterator *iter;
   
   iter=(TQTreeIterator*)calloc(1,sizeof(TQTreeIterator));
   return(iter);
}


/*----------------------------------------------------------------------------
 * Name     : <QTree_Iterate>
 * Creation : November 2014 - J.P. Gauthier - CMC/CMOE
 *
 * Purpose  : Parse only leaf nodes. Each call will return the next leaf
 *
 * Args :
 *   <Node>  : TQTree object pointer which we want to iterate into, fisrt call only, NULL thereafter
 *   <Iter>  : Tree iterator
 *
 * Return:
 *   <Node>  : Next leaf node
 * 
 * Remarks : 
 *----------------------------------------------------------------------------
 */
TQTree* QTree_Iterate(TQTree* restrict Node,TQTreeIterator *Iter) {
   
   unsigned char idx;
   
   // On other than first iteration, use iterator restart point
   if (!Node) {
      Node=Iter->Node;
   }
   
   if (Node) {
      if (Node->Childs[0]) {                               // If this node has childs and not all are visited
         idx=Iter->Path&0x7;                               // Get the next child index (3 first bit)
         
         if ((idx=Iter->Path&0x7)<4) {
            Iter->Path++;                                  // Increment child number
            Iter->Path<<=3;                                // Push on path for next level down
            Iter->Node=Node;                               // Keep this node as the iterator restart
            Node=QTree_Iterate(Node->Childs[idx],Iter);    // Look deeper for a leaf
         } 
         if (idx==3 && !Node->Childs[0]) {                 // If we were on the last leaf of the previous node
            while((Iter->Path&0x7)==4) {                   // Go up to the last unprocessed branch
               Iter->Path>>=3;                             // Pop the child index
               Iter->Node=Iter->Node->Parent;              // Set iterator restart to parent
            }
         }
      } else {
         Iter->Path>>=3;                                   // This is a leaf, return it and pop the path
      }
   }
 
   return(Node);
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_IterateFilled>
 * Creation : November 2014 - J.P. Gauthier - CMC/CMOE
 *
 * Purpose  : Parse only leaf nodes, containg data payload. Each call
 *            will return the next leaf
 *
 * Args :
 *   <Node>  : TQTree object pointer which we want to iterate into, fisrt call only, NULL thereafter
 *   <Iter>  : Tree iterator
 *
 * Return:
 *   <Node>  : Next leaf node
 * 
 * Remarks : 
 *----------------------------------------------------------------------------
 */
TQTree* QTree_IterateFilled(TQTree* restrict Node,TQTreeIterator *Iter) {

   TQTree *node;
   
   while((node=QTree_Iterate(Iter->Node?NULL:Node,Iter)) && !node->NbData);
   
   return(node);
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_Parse>
 * Creation : November 2014 - J.P. Gauthier - CMC/CMOE
 *
 * Purpose  : Parse all nodes af the QTree
 *
 * Args :
 *   <Node>  : TQTree object pointer which we want to parse
 *   <Proc>  : Proc to apply to all nodes data (typedef QTree_ParseProc).
 *   <Depth> : Use 0 for starer
 *
 * Return:
 *
 * Remarks : 
 *----------------------------------------------------------------------------
 */
void QTree_Parse(TQTree* restrict Node,QTree_ParseProc *Proc,unsigned Depth) {
   
   int c=0,d;
   
   if (Node) {
      
      if (Proc) {
         for(d=0;d<Node->NbData;d++) {
            Proc(Node->Data[d].Ptr);
         }
      } else {
         d=Depth;
         while(d--) fprintf(stdout,"   ");
         fprintf(stdout,"Level %i, BBox [%f,%f - %f,%f], Data %i\n",Depth,Node->BBox[0].X,Node->BBox[0].Y,Node->BBox[1].X,Node->BBox[1].Y,Node->NbData);
      }
      for(c=0;c<4;c++) {
         QTree_Parse(Node->Childs[c],Proc,Depth+1);
      }
   }
}
