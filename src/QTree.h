/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Project      : Generic Quad Tree library
 * Creation     : November 2008
 * Author       : Gilles Mercier - CMC/CMOE
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
             
#ifndef _QTree_h
#define _QTree_h

#include "eerUtils.h"
#include "Vector.h"

#define QTREE_INFINITE 0xFFFFFFFF
#define QTREE_SIZEINCR 5

typedef struct TQTreeData {
   char    *Ptr;                // Data pointer
   TPoint2D Pos;                // XY data position
} TQTreeData;

typedef struct TQTree {
   struct TQTree* Parent;       // Pointer to parent cell
   struct TQTree* Childs[4];    // Array of the sub quad pointers, handy for iterations
   TQTreeData*    Data;         // Data payload

   TPoint2D       BBox[2];      // South West XY position limit of the quad bounding box
   int            NbData;       // Number of data in the payload
   int            Size;         // Current size of data payload
} TQTree;

typedef struct TQTreeIterator {
   struct TQTree *Node;         // Next iteration restart node
   unsigned long long Path;     // Current node path (3 last bits are parsed childs left shifted as we go down)
} TQTreeIterator;

typedef void (QTree_ParseProc) (void *Data);

TQTree* QTree_New(double X0,double Y0,double X1,double Y1,TQTree *Parent);
TQTree* QTree_Add(TQTree* restrict Node,double X,double Y,unsigned int MaxDepth,void* restrict Data);
TQTree* QTree_AddTriangle(TQTree* restrict Node,Vect2d T[3],unsigned int MaxDepth,void* restrict Data);
void    QTree_Del(TQTree* restrict Node);
TQTree* QTree_Find(TQTree* restrict Node,double X,double Y);
void    QTree_Parse(TQTree* restrict Node,QTree_ParseProc *Proc,unsigned Depth);

TQTree*         QTree_Iterate(TQTree* restrict Node,TQTreeIterator *Iter);
TQTree*         QTree_IterateFilled(TQTree* restrict Node,TQTreeIterator *Iter);
TQTreeIterator* QTree_IteratorNew(void);

static inline char* QTree_GetData(TQTree* const restrict Node,int Index) {

   if (Node && Node->Data && Index<Node->NbData && Index>=0) {
      return(Node->Data[Index].Ptr);
   }
   return(NULL);
}

#endif
  