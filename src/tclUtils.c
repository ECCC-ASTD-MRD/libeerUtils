/*=========================================================
 * Environnement Canada
 * Centre Meteorologique Canadien
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Lecture et traitements de divers fichiers de donnees
 * Fichier      : tclUtils.c
 * Creation     : Avril 2007 - J.P. Gauthier
 *
 * Description  : Fonctions generales d'utilites courantes.
 *
 * Remarques    :
 *
 * License      :
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
 *=========================================================
 */

#ifndef _AIX_

#include <malloc.h>
#include <string.h>

#include "tclUtils.h"

#include "tclPort.h"
#include "tclInt.h"
#include "tclIO.h"

TCL_DECLARE_MUTEX(MUTEX_HASH)

// Tcl Internal stucture needed for some extension functions
typedef struct TcpState TcpState;

typedef struct TcpFdList {
    TcpState *statePtr;
    int fd;
    struct TcpFdList *next;
} TcpFdList;

struct TcpState {
    Tcl_Channel channel;        /* Channel associated with this file. */
    TcpFdList fds;              /* The file descriptors of the sockets. */
    int flags;                  /* ORed combination of the bitfields defined
                                 * below. */
    /*
     * Only needed for server sockets
     */

    Tcl_TcpAcceptProc *acceptProc;
                                /* Proc to call on accept. */
    ClientData acceptProcData;  /* The data for the accept proc. */

    /*
     * Only needed for client sockets
     */

    struct addrinfo *addrlist;  /* Addresses to connect to. */
    struct addrinfo *addr;      /* Iterator over addrlist. */
    struct addrinfo *myaddrlist;/* Local address. */
    struct addrinfo *myaddr;    /* Iterator over myaddrlist. */
    int filehandlers;           /* Caches FileHandlers that get set up while
                                 * an async socket is not yet connected. */
    int status;                 /* Cache status of async socket. */
    int cachedBlocking;         /* Cache blocking mode of async socket. */
};

int TclY_Get0IntFromObj(Tcl_Interp *Interp,Tcl_Obj *Obj,int *Var) {

   if (Tcl_GetString(Obj)[0]=='0') {
       sscanf(Tcl_GetString(Obj),"%d",Var);
       return(TCL_OK);
   } else {
       return(Tcl_GetIntFromObj(Interp,Obj,Var));
   }
}

int TclY_Get0LongFromObj(Tcl_Interp *Interp,Tcl_Obj *Obj,long *Var) {

   if (Tcl_GetString(Obj)[0]=='0') {
       sscanf(Tcl_GetString(Obj),"%ld",Var);
       return(TCL_OK);
   } else {
       return(Tcl_GetLongFromObj(Interp,Obj,Var));
   }
}

int TclY_Get0DoubleFromObj(Tcl_Interp *Interp,Tcl_Obj *Obj,double *Var) {

   if (Tcl_GetString(Obj)[0]=='0') {
      sscanf(Tcl_GetString(Obj),"%lf",Var);
      return(TCL_OK);
   } else {
      return(Tcl_GetDoubleFromObj(Interp,Obj,Var));
   }
}

FILE* TclY_ChannelOrSocketOpen(Tcl_Interp *Interp,Tcl_Obj *Obj,char *Mode) {

   Tcl_Channel  sock=NULL;
   FILE        *fid=NULL;
   int          mode;

   if ((sock=Tcl_GetChannel(Interp,Tcl_GetString(Obj),&mode))) {
      if (!(fid=fdopen(((TcpState*)((Channel*)sock)->instanceData)->fds.fd,Mode))) {
         Tcl_AppendResult(Interp,"TclY_ChannelOrSocketOpen : Unable to open socket \"",Tcl_GetString(Obj),"\"",(char*)NULL);
         return(NULL);
      }
   } else {
      if (!(fid=fopen(Tcl_GetString(Obj),Mode))) {
         Tcl_AppendResult(Interp,"TclY_ChannelOrSocketOpen : Unable to open file or Invalid template \"",Tcl_GetString(Obj),"\"",(char*)NULL);
         return(NULL);
      }
   }
   return(fid);
}

int TclY_SocketTimeOut(Tcl_Interp *Interp,Tcl_Obj *Obj,int *Receive,int *Send) {

   Tcl_Channel    sock=NULL;
   int            mode;
   struct timeval timeout;      

   timeout.tv_usec = 0;

   if ((sock=Tcl_GetChannel(Interp,Tcl_GetString(Obj),&mode))) {
      mode=sizeof(timeout);
      
      if (*Receive==0) {
         if (getsockopt(((TcpState*)((Channel*)sock)->instanceData)->fds.fd,SOL_SOCKET,SO_RCVTIMEO,(void * restrict)&timeout,&mode)<0) {
            Tcl_AppendResult(Interp,"TclY_SocketTimeOut: Unable to get receive timeout, getsockopt failed",(char*)NULL);
            return(TCL_ERROR);
         }
         *Receive=timeout.tv_sec;
         
      } else {
         timeout.tv_sec = *Receive;
         if (setsockopt(((TcpState*)((Channel*)sock)->instanceData)->fds.fd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,mode)<0) {
            Tcl_AppendResult(Interp,"TclY_SocketTimeOut: Unable to change receive timeout, setsockopt failed",(char*)NULL);
            return(TCL_ERROR);
         }
      }

      if (*Send==0) {
         if (getsockopt(((TcpState*)((Channel*)sock)->instanceData)->fds.fd,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,&mode)<0) {
            Tcl_AppendResult(Interp,"TclY_SocketTimeOut: Unable to get send timeout, getsockopt failed",(char*)NULL);
            return(TCL_ERROR);
         }
         *Send=timeout.tv_sec;
         
      } else {
         timeout.tv_sec = *Send;

         if (setsockopt(((TcpState*)((Channel*)sock)->instanceData)->fds.fd,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,mode)<0) {
            Tcl_AppendResult(Interp,"TclY_SocketTimeOut: Unable to change send timeout, setsockopt failed",(char*)NULL);
            return(TCL_ERROR);
         }
      }
   } else {
      Tcl_AppendResult(Interp,"TclY_SocketTimeOut: Invalid socket/channel \"",Tcl_GetString(Obj),"\"",(char*)NULL);
      return(TCL_ERROR);    
   }

   return(TCL_OK);
}

int TclY_ListObjFind(Tcl_Interp *Interp,Tcl_Obj *List,Tcl_Obj *Item) {

   Tcl_Obj *obj;
   int      n,nb,idx=-1;

   Tcl_ListObjLength(Interp,List,&nb);

   for(n=0;n<nb;n++) {
      Tcl_ListObjIndex(Interp,List,n,&obj);
      if (strcmp(Tcl_GetString(obj),Tcl_GetString(Item))==0) {
         idx=n;
         break;
      }
   }
   return(idx);
}

void TclY_LockHash(){
   Tcl_MutexLock(&MUTEX_HASH);
}
void TclY_UnlockHash(){
   Tcl_MutexUnlock(&MUTEX_HASH);
}

Tcl_HashEntry* TclY_CreateHashEntry(Tcl_HashTable *Table,const char *Name,int *new) {

   Tcl_HashEntry *entry;

   Tcl_MutexLock(&MUTEX_HASH);
   entry=Tcl_CreateHashEntry(Table,Name,new);
   Tcl_MutexUnlock(&MUTEX_HASH);

   return(entry);
}

Tcl_HashEntry* TclY_FindHashEntry(Tcl_HashTable *Table,const char *Name) {

   Tcl_HashEntry *entry;

   Tcl_MutexLock(&MUTEX_HASH);
   entry=Tcl_FindHashEntry(Table,Name);
   Tcl_MutexUnlock(&MUTEX_HASH);

   return(entry);
}

void TclY_DeleteHashEntry(Tcl_HashEntry *Entry) {

   Tcl_MutexLock(&MUTEX_HASH);
   Tcl_DeleteHashEntry(Entry);
   Tcl_MutexUnlock(&MUTEX_HASH);
}

int TclY_HashAll(Tcl_Interp *Interp,Tcl_HashTable *Table) {

   Tcl_Obj       *lst;
   Tcl_HashSearch ptr;
   Tcl_HashEntry  *entry=NULL;

   lst=Tcl_NewListObj(0,NULL);

   Tcl_MutexLock(&MUTEX_HASH);
   entry=Tcl_FirstHashEntry(Table,&ptr);

   while (entry) {
      Tcl_ListObjAppendElement(Interp,lst,Tcl_NewStringObj(Tcl_GetHashKey(Table,entry),-1));
      entry=Tcl_NextHashEntry(&ptr);
   }
   Tcl_MutexUnlock(&MUTEX_HASH);

   Tcl_SetObjResult(Interp,lst);
   return(TCL_OK);
}

void* TclY_HashGet(Tcl_HashTable *Table,const char *Name) {

   Tcl_HashEntry *entry;

   if (Name && strlen(Name)>0) {
      entry=TclY_FindHashEntry(Table,Name);
      if (entry) {
         return (void*)(Tcl_GetHashValue(entry));
      }
   }
   return(NULL);
}

int TclY_HashSet(Tcl_Interp *Interp,Tcl_HashTable *Table,const char *Name,void *Data) {

   Tcl_HashEntry *entry;
   int            new;

   entry=TclY_CreateHashEntry(Table,Name,&new);
   if (!new) {
      if (Interp) Tcl_AppendResult(Interp,"TclY_HashSet: Name already used \"",Name,"\"",(char*)NULL);
      return(TCL_ERROR);
   }

   Tcl_SetHashValue(entry,Data);
   return(TCL_OK);
}

void* TclY_HashPut(Tcl_Interp *Interp,Tcl_HashTable *Table,const char *Name,unsigned int Size) {

   void           *data=NULL;
   Tcl_HashEntry  *entry;
   int             new;

   entry=TclY_CreateHashEntry(Table,Name,&new);

   if (!new) {
      if (Interp) Tcl_AppendResult(Interp,"TclY_HashSet: Name already used \"",Name,"\"",(char*)NULL);
   } else {

      if (!(data=(void*)malloc(Size))) {
         if (Interp) Tcl_AppendResult(Interp,"TclY_HashPut: Unable to allocate memory",(char *)NULL);
      } else {
         Tcl_SetHashValue(entry,data);
         if (Interp) {
            Tcl_ResetResult(Interp);
            Tcl_SetObjResult(Interp,Tcl_NewStringObj(Name,-1));
         }
      }
   }

   return(data);
}

void* TclY_HashReplace(Tcl_Interp *Interp,Tcl_HashTable *Table,const char *Name,void *Data) {

   void           *data=NULL;
   Tcl_HashEntry  *entry;
   int             new;

   entry=TclY_FindHashEntry(Table,Name);

   if (!entry) {
      entry=TclY_CreateHashEntry(Table,Name,&new);
      data=NULL;
   } else {
      data=Tcl_GetHashValue(entry);
   }
   Tcl_SetHashValue(entry,Data);

   return(data);
}

void* TclY_HashDel(Tcl_HashTable *Table,const char *Name) {

   Tcl_HashEntry *entry;
   void          *item=NULL;

   if (Name) {
      Tcl_MutexLock(&MUTEX_HASH);
      entry=Tcl_FindHashEntry(Table,Name);

      if (entry) {
         item=Tcl_GetHashValue(entry);
         Tcl_DeleteHashEntry(entry);
      }
      Tcl_MutexUnlock(&MUTEX_HASH);
   }
   return(item);
}

void TclY_HashWipe(Tcl_HashTable *Table,TclY_HashFreeEntryDataFunc *TclY_HashFreeEntryData) {

   Tcl_HashSearch ptr;
   Tcl_HashEntry  *entry=NULL;

   Tcl_MutexLock(&MUTEX_HASH);
   entry=Tcl_FirstHashEntry(Table,&ptr);

   while(entry) {
      if (TclY_HashFreeEntryData) {
         Tcl_MutexUnlock(&MUTEX_HASH);
         TclY_HashFreeEntryData(Tcl_GetHashValue(entry));
         Tcl_MutexLock(&MUTEX_HASH);
      }
      Tcl_DeleteHashEntry(entry);
      entry=Tcl_FirstHashEntry(Table,&ptr);
   }

//   Tcl_DeleteHashTable(Table);
   Tcl_MutexUnlock(&MUTEX_HASH);
}
#endif
