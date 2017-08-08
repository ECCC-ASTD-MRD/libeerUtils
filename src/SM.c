#include "SM.h"
#include "App.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MPI
#include <mpi.h>
#endif //_MPI

#define SM_FILENAME 128

/*----------------------------------------------------------------------------
 * Nom      : <SM_GetSize>
 * Creation : Août 2017 - E. Legault-Ouellet
 *
 * But      : Retourne la taille de la mémoire sauvegardée dans le header
 *
 * Parametres :
 *  <Addr>    : Adresse de la mémoire dont on veut la taille.
 *
 * Retour     : La taille de la mémoire SANS le header
 *
 * Remarques  :
 *----------------------------------------------------------------------------
 */
static size_t SM_GetSize(void *Addr) {
   return *((size_t*)Addr-1);
}

/*----------------------------------------------------------------------------
 * Nom      : <SM_RoundPageSize>
 * Creation : Août 2017 - E. Legault-Ouellet
 *
 * But      : Arrondir à l'entier supérieur qui est un multiple de la taille
 *            d'une page mémoire
 *
 * Parametres :
 *  <Size>    : Taille minimum voulue
 *
 * Retour     : Taille arrondie au multiple supérieur de la taille de la page
 *              mémoire
 *
 * Remarques  :
 *----------------------------------------------------------------------------
 */
size_t SM_RoundPageSize(size_t Size) {
   static size_t pmask = 0;
   if( !pmask )
      pmask = (size_t) ~(sysconf(_SC_PAGESIZE)-1);
   return Size & pmask;
}

/*----------------------------------------------------------------------------
 * Nom      : <SM_Alloc>
 * Creation : Août 2017 - E. Legault-Ouellet
 *
 * But      : Allouer un espace mémoire partagé par tous les noeuds MPI
 *
 * Parametres :
 *  <Size>    : Taille de l'espace mémoire allouée
 *
 * Retour     : Un pointeur vers l'adresse mémoire ou NULL si erreur.
 *
 * Remarques  :
 *      - La mémoire est toujours initialisée à 0
 *      - S'il n'y a qu'un seul process MPI ou si le proces MPI est seul sur
 *        son noeud, alors ceci est équivalent à faire un calloc
 *      - Seul les headnodes de chaque noeuds peuvent modifier la mémoire
 *        partagée, les autres noeuds map la mémoire en R/O
 *----------------------------------------------------------------------------
 */
void* SM_Alloc(size_t Size) {
   if( App_IsAloneNode() ) {
      // No sharing possibilities, just allocate normal memory
      return calloc(Size,1);
   } else {
      void *addr = NULL;
#ifdef _MPI
      char fn[SM_FILENAME] = {0};
      int fd=-1,status=0;

      // Increase the size enough to contain our size header
      Size += sizeof(size_t);

      if( !App->NodeRankMPI ) {
         static int seqno = 0;

         // Generate a unique filename
         snprintf(fn,SM_FILENAME,"/MPI_SM_%d_%d.shmem",(int)getpid(),seqno++);

         // Open the pseudo-file backed shared memory
         if( (fd=shm_open(fn,O_RDWR|O_CREAT|O_EXCL,00600)) == -1 ) {
            App_Log(ERROR,"An error occured while opening the pseudo-file backed shared memory\n");
            status = 1;
         }

         // Set its size (Note: this is equivalent to calloc since the memory is zeroed out)
         if( !status && ftruncate(fd,Size) ) {
            App_Log(ERROR,"An error occured while executing ftruncate for size %zd\n",Size);
            status = 1;
         }

         // Map the file
         if( !status && (addr=mmap(NULL,Size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0)) == MAP_FAILED ) {
            App_Log(ERROR,"An error occured while mapping the pseudo-file backed shared memory\n");
            status = 1;
            addr = NULL;
         }

         if( status ) {
            // Some error handling
            shm_unlink(fn);
            memset(fn,0,SM_FILENAME);
         } else {
            // Store the size (without header) as a header
            *(size_t*)addr = Size-sizeof(size_t);
         }

         // Send the filename
         if( MPI_Bcast(fn,SM_FILENAME,MPI_BYTE,0,App->NodeComm) != MPI_SUCCESS ) {
            App_Log(ERROR,"Could not Bcast the filename of the pseudo-file backed shared memory\n");
            status = 1;
         }
      } else {
         // Receive the filename
         if( MPI_Bcast(fn,SM_FILENAME,MPI_BYTE,0,App->NodeComm) != MPI_SUCCESS ) {
            App_Log(ERROR,"Could not receive the filename of the pseudo-file backed shared memory\n");
            status = 1;
         }

         if( !status && fn[0] ) {
            // Open the pseudo-file backed shared memory
            if( (fd=shm_open(fn,O_RDONLY,0)) == -1 ) {
               App_Log(ERROR,"An error occured while opening the pseudo-file backed shared memory\n");
               status = 1;
            }

            // Map the file
            if( (addr=mmap(NULL,Size,PROT_READ,MAP_SHARED,fd,0)) == MAP_FAILED ) {
               App_Log(ERROR,"An error occured while mapping the pseudo-file backed shared memory\n");
               status = 1;
               addr = NULL;
            }
         }
      }

      // Gather the status code of all processes
      if( MPI_Allreduce(MPI_IN_PLACE,&status,1,MPI_INT,MPI_SUM,App->NodeComm) != MPI_SUCCESS ) {
         App_Log(ERROR,"Could not reduce the status of all nodes in the shared mem circle\n");
         status = 1;
      }

      // Unlink the file. Note that this doesn't unmap the file and the data is still available to every process that mapped it
      if( fd != -1 )
         close(fd);
      if( fn[0] )
         shm_unlink(fn);

      // Unmap the file if the status of any MPI process returned an error
      if( status && addr ) {
         munmap(addr,Size);
         addr = NULL;
      }
#endif //_MPI
      return addr ? (size_t*)addr+1 : addr;
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <SM_Calloc>
 * Creation : Août 2017 - E. Legault-Ouellet
 *
 * But      : Wrapper autour de SM_Alloc pour faire un "drop in replacement"
 *            de calloc
 *
 * Parametres :
 *  <Num>     : Nombre d'élément à allouer
 *  <Size>    : Taille de de cahque élément
 *
 * Retour     : Un pointeur vers l'adresse mémoire ou NULL si erreur.
 *
 * Remarques  :
 *      - La mémoire est toujours initialisée à 0
 *      - S'il n'y a qu'un seul process MPI ou si le proces MPI est seul sur
 *        son noeud, alors ceci est équivalent à faire un calloc
 *----------------------------------------------------------------------------
 */
void* SM_Calloc(size_t Num,size_t Size) {
   return SM_Alloc(Num*Size);
}

/*----------------------------------------------------------------------------
 * Nom      : <SM_Sync>
 * Creation : Août 2017 - E. Legault-Ouellet
 *
 * But      : Synchronize the data in the sahred memory between all nodes and
 *            makes sure every MPI process waits for the shared memory to be
 *            updated before they can use it
 *
 * Parametres :
 *  <Addr>    : Adresse de la mémoire à synchroniser.
 *  <Rank>    : Rank du NodeHead (NodeHeadComm) du noeud envoyant les données
 *
 * Retour     : APP_OK si ok, APP_ERR sinon
 *
 * Remarques  :
 *----------------------------------------------------------------------------
 */
int SM_Sync(void *Addr,int NodeHeadRank) {
   if( App_IsMPI() ) {
#ifdef _MPI
      // If there is more than one node, the head of each node must exchange the data
      if( !App_IsSingleNode() && !App->NodeRankMPI ) {
         APP_MPI_ASRT( MPI_Bcast(Addr,SM_GetSize(Addr),MPI_BYTE,NodeHeadRank,App->NodeHeadComm) );
      }
      // If there is more than one MPI process on this node, every process of this node must wait for the data to be available
      if( !App_IsAloneNode() ) {
         APP_MPI_ASRT( MPI_Barrier(App->NodeComm) );
      }
#endif //_MPI
   }
   return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <SM_Free>
 * Creation : Août 2017 - E. Legault-Ouellet
 *
 * But      : Libère la mémoire partagée
 *
 * Parametres :
 *  <Addr>    : Adresse de la mémoire à libérer
 *
 * Retour     :
 *
 * Remarques  :
 *----------------------------------------------------------------------------
 */
void SM_Free(void *Addr) {
   if( App_IsAloneNode() ) {
      free(Addr);
   } else {
#ifdef _MPI
      Addr = (size_t*)Addr-1;
      munmap(Addr,*(size_t*)Addr+sizeof(size_t));
#endif //_MPI
   }
}
