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

  return(X>=Node->BBox[0].X && X<Node->BBox[1].X && Y>=Node->BBox[0].Y && Y<Node->BBox[1].Y );
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
 *    Box numbering is
 *         ----- 
 *       | 0 | 1 |
 *       |---+---| 
 *       | 2 | 3 |  
 *         ----- 
 *----------------------------------------------------------------------------
 */
static inline int QTree_Split(TQTree* const restrict Node,const TPoint2D* const restrict Center) {
   
   Node->Childs[0] = QTree_New(Node->BBox[0].X,Node->BBox[0].Y, Center->X, Center->Y,Node);
   Node->Childs[1] = QTree_New(Center->X,Node->BBox[0].Y,Node->BBox[1].X, Center->Y,Node); 
   Node->Childs[2] = QTree_New(Node->BBox[0].X,Center->Y,Center->X, Node->BBox[1].Y,Node);
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
int QTree_AddData(TQTree* const restrict Node,double X,double Y,void *Data) {

  if (Data) {
     Node->NbData++;
     if (Node->NbData>Node->Size) {
        Node->Size+=QTREE_SIZEINCR;
        if (!(Node->Data=(TQTreeData*)realloc(Node->Data,Node->Size*sizeof(TQTreeData)))) {
           return(0);
        }
     }
     Node->Data[Node->NbData-1].Ptr=Data;
     Node->Data[Node->NbData-1].Pos.X=X;
     Node->Data[Node->NbData-1].Pos.Y=Y;
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
void QTree_DelData(TQTree* const Node) {

   Node->NbData=0;
   Node->Size=0;
 
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

      node->Size      = 0;
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
               center.X = Node->BBox[0].X+(Node->BBox[1].X-Node->BBox[0].X)*0.5;
               center.Y = Node->BBox[0].Y+(Node->BBox[1].Y-Node->BBox[0].Y)*0.5;

               if (!QTree_Split(Node,&center)) {
                  return(NULL);
               }
                     
               // Reassign previous node data into splitted node
               new = QTree_FindChild(Node,Node->Data[0].Pos.X,Node->Data[0].Pos.Y);
               new->Data=Node->Data;
               new->NbData=Node->NbData;
               Node->Data=NULL;
               Node->NbData=0;
               Node->Size=0;
               
               // Assign new data
               new = QTree_FindChild(Node,X,Y);
               new = QTree_Add(new,X,Y,MaxDepth,Data);
            }
         }
      }
   }

   return(ok?new:NULL);
}

// Vertice codes for Cohen–Sutherland bbox intersection algorithm check
#define CS_INSIDE 0x0;             // 0000
#define CS_LEFT   0x1;             // 0001
#define CS_RIGHT  0x2;             // 0010
#define CS_BOTTOM 0x4;             // 0100
#define CS_TOP    0x8;             // 1000
#define CS_Intersect(A,B) (!(A&B)) // Do theses code intersect

// Cohen–Sutherland algorithm 
// Compute the bit code for a point (x, y) using the clip rectangle
// bounded diagonally by (xmin, ymin), and (xmax, ymax)
int CS_Code(double X, double Y,double X0,double Y0,double X1,double Y1) {
   
   int code=CS_INSIDE;                    // Initialised as being inside of clip window

   if      (X<X0) { code |= CS_LEFT; }     // Left of clip window 
   else if (X>X1) { code |= CS_RIGHT; }    // Right of clip window
            
   if      (Y<Y0) { code |= CS_BOTTOM; }    // Below the clip window           
   else if (Y>Y1) { code |= CS_TOP; }      // Above the clip window
            
   return code;
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_AddTriangle>
 * Creation : July 2015 - G.Mercier - CMC/CMOE
 *
 * Purpose  : Add some triangle data to a TQTree object(usually a head quad at level=1...).
 *
 * Args :
 *   <Node>     : TQTree object pointer which we want to add data information (cannot be NULL).
 *   <T>        : Trinagle's corners (X,Y).
 *   <MaxDepth> : Maximum depth of tree
 *   <Data>     : External Data pointer.
 *
 * Return:
 *  <TQTree *> : A TQTree object pointer(Could be NULL if a problem arise).  
 *
 * Remarks : 
 *   - We insert the triangle in any leaf nodes intersected by the triangle, this implis
 *     some possible duplication but it's the most effective way.
 *----------------------------------------------------------------------------
 */
TQTree* QTree_AddTriangle(TQTree* restrict Node,Vect2d T[3],unsigned int MaxDepth,void* restrict Data) {

   TPoint2D center;
   int      cs[3];
   TQTree  *new=Node;
      
   // Get Cohen-Sutherland code value for each triangle endpoint
   cs[0]=CS_Code(T[0][0],T[0][1],Node->BBox[0].X,Node->BBox[0].Y,Node->BBox[1].X,Node->BBox[1].Y);
   cs[1]=CS_Code(T[1][0],T[1][1],Node->BBox[0].X,Node->BBox[0].Y,Node->BBox[1].X,Node->BBox[1].Y);
   cs[2]=CS_Code(T[2][0],T[2][1],Node->BBox[0].X,Node->BBox[0].Y,Node->BBox[1].X,Node->BBox[1].Y);
      
   // If any segment intersects this node's bbox
   if (CS_Intersect(cs[0],cs[1]) || CS_Intersect(cs[1],cs[2]) || CS_Intersect(cs[2],cs[0])) {
      
      if (!(MaxDepth--)) {
         // We've hit maximum depth, add to this node
         if (QTree_AddData(Node,T[0][0],T[0][1],Data)) {
            new=Node;
         }
      }  else {
         // If there's no child yet at this node and depth
         if (!Node->Childs[0]) {
            // Split node in 4.
            center.X = Node->BBox[0].X+(Node->BBox[1].X-Node->BBox[0].X)*0.5;
            center.Y = Node->BBox[0].Y+(Node->BBox[1].Y-Node->BBox[0].Y)*0.5;

            if (!QTree_Split(Node,&center)) {
               return(NULL);
            }
         }

         // Recursively dig into the tree
         QTree_AddTriangle(Node->Childs[0],T,MaxDepth,Data);
         QTree_AddTriangle(Node->Childs[1],T,MaxDepth,Data);
         QTree_AddTriangle(Node->Childs[2],T,MaxDepth,Data);
         QTree_AddTriangle(Node->Childs[3],T,MaxDepth,Data);
      }
   }

   return(new);
}

/*----------------------------------------------------------------------------
 * Name     : <QTree_Neighbors>
 * Creation : July 2015 - J.P.Gauthier - CMC/CMOE
 *
 * Purpose  : Find all neighbors of a quad cell
 *
 * Args :
 *   <Node>      : TQTree object pointer to the cell from which we want neighbors
 *   <Neighbors> : Neighbors list
 *   <Nb>        : Number of neightbors to find (4 or 8 for corners).
 *
 * Return:
 *
 * Remarks : 
 *   - The neighbor numbering is as follow
 *
 *     4   0   5
 *        --- 
 *     3 |   | 1
 *        --- 
 *     7   2   6
 *----------------------------------------------------------------------------
 */
void QTree_Neighbors(TQTree* Node,TQTree** Neighbors,int Nb) {

   TQTree *root,*node;
   double  dx,dy;
   int     d=0;;
   
   if (Nb!=4 || Nb!=8) {
      for(int n=0;n<Nb;n++) Neighbors[n]=NULL;
   }
   
   dx=(Node->BBox[1].X-Node->BBox[0].X)*0.5;
   dy=(Node->BBox[1].Y-Node->BBox[0].Y)*0.5;

   // Climb back up the tree
   node=Node;
   while(node=node->Parent) { d++; root=node; }      
         
   // Find neighbors
   Neighbors[0]=QTree_Find(root,Node->BBox[0].X+dx,Node->BBox[0].Y-dy);
   Neighbors[1]=QTree_Find(root,Node->BBox[1].X+dx,Node->BBox[0].Y+dy);
   Neighbors[2]=QTree_Find(root,Node->BBox[0].X+dx,Node->BBox[1].Y+dy);
   Neighbors[3]=QTree_Find(root,Node->BBox[0].X-dx,Node->BBox[0].Y+dy);
   
   // If corners asked
   if (Nb==8) {
      Neighbors[4]=QTree_Find(root,Node->BBox[0].X-dx,Node->BBox[0].Y-dy);
      Neighbors[5]=QTree_Find(root,Node->BBox[1].X+dx,Node->BBox[0].Y-dy);
      Neighbors[6]=QTree_Find(root,Node->BBox[1].X+dx,Node->BBox[1].Y+dy);
      Neighbors[7]=QTree_Find(root,Node->BBox[0].X-dx,Node->BBox[0].Y+dy);      
   }
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

   int     cidx=0,f;
   TQTree* node=Node;

   if (Node->Childs[0]) {

      // Navigate through the quad tree recursively to find the terminal quad
      // which bounding box contains the XY position X,Y.
      while (cidx<4 && !(f=QTree_Inside(Node->Childs[cidx++],X,Y)));
      if (!f) 
         return(NULL);

      node=QTree_Find(Node->Childs[cidx-1],X,Y);
   } else {
      // This node is empty
      if (0 && !Node->Data) {
         // Go up and pick first parent's child having data
         while (cidx<4 && !Node->Parent->Childs[cidx++]->Data);
         node=Node->Parent->Childs[cidx-1];
      }    
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
 *   <Depth> : Use 0 for starter
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


/*----------------------------------------------------------------------------
 * Name     : <QArray_Find>
 * Creation : Decembre 2015 - J.P. Gauthier - CMC/CMOE
 *
 * Purpose  : Find nearest neighbors in the QArray
 *
 * Args :
 *   <Tree>  : TQTree object pointer to array
 *   <Res>   : Array resolution (Size)
 *   <X>     : X Position
 *   <Y>     : Y Position
 *   <Idxs>  : Pointer to neighbors index found
 *   <Dists> : Squared distances from the neighbors found
 *   <NbNear>: Numbre of nearest neighbors to find
 *
 * Return:
 *
 * Remarks : 
 *----------------------------------------------------------------------------
 */

int QArray_Find(TQTree *Tree,int Res,double X,double Y,int *Idxs,double *Dists,int NbNear) {
   
   TQTree    *node;
   double     dx,dy,l;
   int        x,y,xd,yd,dxy,n,nn,nnear,nr;

   if (!NbNear || !Idxs || !Dists) return(0);
   
   Dists[0]=l=1e32;
   dxy=nnear=0;
    
   // Find the closest by circling method
   node=&Tree[0];

   dx=(node->BBox[1].X-node->BBox[0].X)/Res;
   dy=(node->BBox[1].Y-node->BBox[0].Y)/Res;
   xd=(X-node->BBox[0].X)/dx;
   yd=(Y-node->BBox[0].Y)/dy;
   
   // Find the closest point(s) by circling larger around cell
   while(dxy<Res<<2) {
      
      // Y circling increment
      for(y=yd-dxy;y<=yd+dxy;y++) {
         if (y<0)    continue;
         if (y>=Res) break;
         
         // X Circling increment (avoid revisiting previous cells)
         for(x=xd-dxy;x<=xd+dxy;x+=((y==yd-dxy||y==yd+dxy)?1:(dxy+dxy))) {
            if (x<0)    continue;
            if (x>=Res) break;
            
            node=&Tree[y*Res+x];

            // Loop on points in this cell and get closest point
            for(n=0;n<node->NbData;n++) {
               dx=X-node->Data[n].Pos.X;
               dy=Y-node->Data[n].Pos.Y;
               l=dx*dx+dy*dy;
               
               // Loop on number of nearest to find
               for(nn=0;nn<NbNear;nn++) {
                  if (l<Dists[nn]) {
                     
                     // Move farther nearest in order
                     for(nr=NbNear-1;nr>nn;nr--) {
                        Dists[nr]=Dists[nr-1];
                        Idxs[nr]=Idxs[nr-1];                       
                     }
                     
                     // Assign found nearest
                     Dists[nn]=l;
                     Idxs[nn]=(int)node->Data[n].Ptr-1; // Remove false pointer increment
                     nnear++;
                     break;
                  }
               }
            }
         }            
      }
      
      // If we made at least one cycle and found enough nearest
      if (nnear>=NbNear && dxy) 
         break;
      dxy++;
   }
   
   return(nnear>NbNear?NbNear:nnear);
}
