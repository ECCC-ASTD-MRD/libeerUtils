/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie d'écriture/lecture de fichier binaire à la FSTD
 * Fichier   : BinaryFile.c
 * Creation  : Février 2017
 * Auteur    : Eric Legault-Ouellet
 *
 * Description: Librairie d'écriture/lecture de fichier binaire à la FSTD
 *
 * Note: Cette librairie vise à remplacer temporairement les FSTD là où les
 *       limites de ces derniers rendent les choses difficiles.
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
#include "BinaryFile.h"
#include "FPFC.h"
#include "App.h"
#include "RPN.h"
#include <string.h>
#include <glob.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

const int32_t BF_MAGIC=0x45454642; //BFEE (Binary File Env. Emergencies) in little endian
const int32_t BF_VERSION=2;

static int BFTypeSize[] = {1,1,1,2,4,8,1,2,4,8,4,8,4,8,0};

#define AddrAt(Addr,bytes) ( (char*)(Addr) + (bytes) )

/*----------------------------------------------------------------------------
 * Nom      : <FtnStrSize>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Retourne la taille d'un string fortran (sans les trailing spaces)
 *
 * Parametres :
 *
 * Retour   : La taille d'un string fortran (sans les trailing spaces)
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static size_t FtnStrSize(const char *Str,size_t Max) {
   size_t n;

   if( !Str )
      return 0;

   for(n=strnlen(Str,Max); n>0&&Str[n-1]==' '; --n)
      ;
   return n;
}

/*----------------------------------------------------------------------------
 * Nom      : <PageSizeCeil>
 * Creation : Août 2017 - E. Legault-Ouellet
 *
 * But      : Arrondir à l'entier inférieur qui est un multiple de la taille
 *            d'une page mémoire
 *
 * Parametres :
 *  <Size>    : Taille minimum voulue
 *
 * Retour     : Taille arrondie au multiple inférieur de la taille de la page
 *              mémoire
 *
 * Remarques  :
 *----------------------------------------------------------------------------
 */
static size_t PageSizeFloor(size_t Size) {
   return Size & ~((size_t)sysconf(_SC_PAGESIZE)-1);
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_New>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Retourne une structure TBFFiles initialisée
 *
 * Parametres :
 *    <N>   : Le jnombre de fichiers liés
 *
 * Retour   : Une structure TBFFiles initialisée ou NULL en cas d'erreur
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static TBFFiles* BinaryFile_New(int N) {
   TBFFiles *files;
   TBFFile  *restrict file;

   if( N<=0 )
      return NULL;

   // Allocate memory for the list of files
   if( !(files=malloc(sizeof(*files)) ) )
      return NULL;

   // Initialize the members
   files->N = N;
   files->Flags = 0;

   // Allocate memory for the files themselves
   if( !(files->Files=malloc(N*sizeof(*files->Files))) ) {
      free(files);
      return NULL;
   }

   // Init the files
   for(file=files->Files; N; --N,++file) {
      file->Addr  = MAP_FAILED;
      file->Header= (TBFFileHeader){sizeof(TBFFileHeader),0,BF_MAGIC,BF_VERSION,sizeof(TBFFileHeader),sizeof(TBFFldHeader)};
      file->Index = (TBFIndex){NULL,0};
      file->FD    = -1;
      file->Flags = 0;
   }

   return files;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_Free>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Libère la mémoire d'un structure TBFFiles
 *
 * Parametres :
 *    <Files>  : La structure dont il faut libérer la mémoire
 *
 * Retour   : 
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static void BinaryFile_Free(TBFFiles *Files) {
   TBFFile  *restrict file;

   if( Files ) {
      for(file=Files->Files; Files->N; --Files->N) {
         if( file->FD >= 0 ) {
            close(file->FD);
         }

         if( file->Addr != MAP_FAILED ) {
            munmap(file->Addr,file->Header.Size);
         }

         APP_FREE(file->Index.Headers);
      }

      APP_FREE(Files->Files);
      free(Files);
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_OpenFiles>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Ouvre et lie les fichiers donnés
 *
 * Parametres :
 *    <FileNames> : Le nom des fichiers à ouvrir et lier
 *    <N>         : Le nombre de fichier à ouvrir et lier
 *    <Mode>      : Le mode (BF_READ,BF_WRITE,BF_CLEAR)
 *
 * Retour   : Un pointeur vers les fichiers ouvert
 *
 * Remarques : Fonction interne, pas accessible de l'extérieur
 *
 *----------------------------------------------------------------------------
 */
static TBFFiles* BinaryFile_OpenFiles(const char **FileNames,int N,TBFFlag Mode) {
   TBFFiles *restrict files = BinaryFile_New(N);
   TBFFile  *restrict file;
   struct stat statbuf;
   int      i,mode,mmode;

   // If memory was not allocated, abort
   if( !files ) {
      App_Log(ERROR,"BinaryFile: Could not allocate needed memory\n");
      goto error;
   }

   // Limit to relevant flags
   Mode &= BF_READ|BF_WRITE|BF_CLEAR;
   files->Flags = Mode;

   // Make sure we have at least one of READ or WRITE
   if( !(Mode&(BF_READ|BF_WRITE)) ) {
      App_Log(ERROR,"BinaryFile: Either read or write mode must be selected when opening files");
      goto error;
   }

   // Translate our flags into flags accepted by open
   mode = O_CREAT|O_CLOEXEC;
   switch( Mode&(BF_READ|BF_WRITE) ) {
      case BF_WRITE:
         mode |= (Mode&BF_CLEAR)?O_RDWR|O_TRUNC:O_RDWR;
         mmode = PROT_READ|PROT_WRITE;
         break;
      case BF_READ:
         mode |= O_RDONLY;
         mmode = PROT_READ;
         break;
      case BF_READ|BF_WRITE:
         mode |= O_RDWR;
         mmode = PROT_READ|PROT_WRITE;
         break;
   }
   
   for(i=0,file=files->Files; i<N; ++i,++file) {
      file->Flags |= Mode;

      // Open the file
      if( (file->FD=open(FileNames[i],mode,00666)) < 0 ) {
         App_Log(ERROR,"BinaryFile: Problem opening file %s in mode %s%s%s\n",FileNames[i],Mode&BF_READ?"r":"",Mode&BF_WRITE?"w":"",Mode&BF_CLEAR?"*":"");
         goto error;
      }

      // Stat the file
      if( fstat(file->FD,&statbuf) ) {
         App_Log(ERROR,"BinaryFile: Could not stat file %s : %s\n",FileNames[i],strerror(errno));
         goto error;
      }

      // Make sure that, if the file is not empty, it has at least enough data for a valid header
      if( statbuf.st_size && statbuf.st_size<sizeof(file->Header) ) {
         App_Log(ERROR,"BinaryFile: File is not empty but isn't big enough for a BF header %s. (Size=%zd bytes, header is %zd bytes)\n",FileNames[i],(size_t)statbuf.st_size,sizeof(file->Header));
         goto error;
      }

      // If the file is empty, the write flag needs to be there
      if( !statbuf.st_size && !(Mode&BF_WRITE) ) {
         App_Log(ERROR,"BinaryFile: File %s is empty, there is nothing to read.\n",FileNames[i]);
         goto error;
      }

      if( statbuf.st_size ) {
         // Read the file header
         if( Mode&BF_WRITE ) {
            // We'll need to write to the file later on, just use read to read the file header
            if( read(file->FD,&file->Header,sizeof(file->Header))!=sizeof(file->Header) ) {
               App_Log(ERROR,"BinaryFile: Problem reading header for file %s\n",FileNames[i]);
               goto error;
            }
         } else {
            // Map the file (or just the file header if in write-only mode)
            if( (file->Addr=mmap(NULL,statbuf.st_size,mmode,MAP_SHARED,file->FD,0)) == MAP_FAILED ) {
               App_Log(ERROR,"BinaryFile: Could not map file %s\n",FileNames[i]);
               goto error;
            }

            // Read the file header
            file->Header = *((TBFFileHeader*)file->Addr);
         }

         // Make sure we have a valid BinaryFile
         if( file->Header.Magic != BF_MAGIC ) {
            App_Log(ERROR,"BinaryFile: File %s is not of BinaryFile type\n",FileNames[i]);
            goto error;
         }

         // Make sure the filesystem agrees with our header on the size
         if( file->Header.Size != statbuf.st_size ) {
            App_Log(ERROR,"BinaryFile: The filesystem says the file is %zd bytes != %zd bytes per the BF file header for file %s\n",(size_t)statbuf.st_size,file->Header.Size,FileNames[i]);
            // This is to allow the unmap to unmap the right amount
            file->Header.Size = statbuf.st_size;
            goto error;
         }

         // Make sure we will have the right binary offsets
         if( file->Header.FSize!=sizeof(TBFFileHeader) || file->Header.HSize!=sizeof(TBFFldHeader) ) {
            App_Log(ERROR,"BinaryFile: This library is incompatible with this BinaryFile (%s) FileHeader is %d bytes (library: %d) and the FldHeader is %u bytes (library: %d).\n",
                  FileNames[i],file->Header.FSize,sizeof(TBFFileHeader),file->Header.HSize,sizeof(TBFFldHeader));
            goto error;
         }

         // Read the index size
         if( Mode&BF_WRITE ) {
            // Seek to the index
            if( lseek(file->FD,file->Header.IOffset,SEEK_SET) == -1 ) {
               App_Log(ERROR,"BinaryFile: Could not seek to index in file %s : %s\n",FileNames[i],strerror(errno));
               goto error;
            }

            // Read the index size without moving the position in the file
            if( pread(file->FD,&file->Index.N,sizeof(file->Index.N),file->Header.IOffset)!=sizeof(file->Index.N) ) {
               App_Log(ERROR,"BinaryFile: Problem reading index size for file %s : %s\n",FileNames[i],strerror(errno));
               goto error;
            }
         } else {
            file->Index.N = *((int32_t*)AddrAt(file->Addr,file->Header.IOffset));
         }

         // Allocate the memory for the index
         if( !(file->Index.Headers=malloc(file->Index.N*sizeof(*file->Index.Headers))) ) {
            App_Log(ERROR,"BinaryFile: Could not allocate memory for index\n");
            goto error;
         }

         // Read the index
         if( Mode&BF_WRITE ) {
            // iUse pread to prevent moving the position of the cursor in the file
            if( pread(file->FD,file->Index.Headers,sizeof(*file->Index.Headers)*file->Index.N,file->Header.IOffset+sizeof(file->Index.N))!=sizeof(*file->Index.Headers)*file->Index.N ) {
               App_Log(ERROR,"BinaryFile: Problem reading index for file %s : %s\n",FileNames[i],strerror(errno));
               goto error;
            }
         } else {
            memcpy(file->Index.Headers,AddrAt(file->Addr,file->Header.IOffset+sizeof(file->Index.N)),sizeof(*file->Index.Headers)*file->Index.N);
         }
      } else {
         // Write an invalid header as a place holder
         if( write(file->FD,&file->Header,sizeof(file->Header))!=sizeof(file->Header) ) {
            App_Log(ERROR,"BinaryFile: Problem writing header for file %s : %s\n",FileNames[i],strerror(errno));
            goto error;
         }

         // Mark the fact that the file header will need to be written at closing time
         file->Flags |= BF_DIRTY;
      }
   }

   return files;
error:
   BinaryFile_Free(files);
   return NULL;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_Open>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Ouvre un fichier BinaryFile
 *
 * Parametres :
 *    <FileName>  : Le nom du fichier à ouvrir
 *    <Mode>      : Le mode (BF_READ,BF_WRITE,BF_CLEAR)
 *
 * Retour   : Un pointeur vers le fichier ouvert
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TBFFiles* BinaryFile_Open(const char *FileName,TBFFlag Mode) {
   return BinaryFile_OpenFiles(&FileName,1,Mode);
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_Link>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Ouvre et lie les fichiers donnés en mode lecture seulement
 *
 * Parametres :
 *    <FileNames> : La liste des noms des fichiers à ouvrir et lier.
 *    <N>         : Le nombre de fichier à lier ou -1 si la liste est NULL-terminated
 *
 * Retour   : Un pointeur vers les fichiers ouvert
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TBFFiles* BinaryFile_Link(const char** FileNames,int N) {
   // Make sure we have at least one file
   if( !FileNames || N==0 || !*FileNames )
      return NULL;

   // Count the number of files
   if( N<0 ) {
      for(N=1; FileNames[N]; ++N)
         ;
   }
   return BinaryFile_OpenFiles(FileNames,N,BF_READ);
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_LinkPattern>
 * Creation : Avril 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Ouvre et lie les fichiers correspondant au pattern donné en
 *            mode lecture seulement
 *
 * Parametres :
 *    <Pattern>   : Le pattern des fichiers à ouvrir et lier.
 *
 * Retour   : Un pointeur vers les fichiers ouvert
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TBFFiles* BinaryFile_LinkPattern(const char* Pattern) {
   // Expand the pattern into a list of files
   glob_t gfiles = (glob_t){0,NULL,0};
   if( glob(Pattern,0,NULL,&gfiles) || !gfiles.gl_pathc ) {
      return NULL;
   }

   TBFFiles *bf = BinaryFile_Link((const char**)gfiles.gl_pathv,(int)gfiles.gl_pathc);
   globfree(&gfiles);
   return bf;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_Close>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Ferme le(s) fichier(s) ouvert(s)
 *
 * Parametres :
 *    <Files>  : Le pointeur vers le(s) fichier(s)
 *
 * Retour   : APP_ERR en cas d'erreur, APP_OK si ok
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int BinaryFile_Close(TBFFiles *Files) {
   int i,code=APP_OK;
   TBFFile  *restrict file;
   size_t bytes;

   if( !Files )
      return APP_ERR;

   for(i=0,file=Files->Files; i<Files->N; ++i,++file) {
      // Only write something if something changed
      if( file->Flags&BF_DIRTY ) {
         // Write the index size
         bytes = sizeof(file->Index.N);
         if( write(file->FD,&file->Index.N,bytes) != bytes ) {
            App_Log(ERROR,"BinaryFile: Problem writing file index. The file will be corrupt\n");
            code = APP_ERR;
            continue;
         }

         // Write the index
         bytes = sizeof(*file->Index.Headers)*file->Index.N;
         if( write(file->FD,file->Index.Headers,bytes) != bytes ) {
            App_Log(ERROR,"BinaryFile: Problem writing file index. The file will be corrupt\n");
            code = APP_ERR;
            continue;
         }

         // Update the total file size
         file->Header.Size = file->Header.IOffset+sizeof(file->Index.N)+sizeof(*file->Index.Headers)*file->Index.N;

         // Write the file header
         bytes = sizeof(file->Header);
         if( pwrite(file->FD,&file->Header,bytes,0) != bytes ) {
            App_Log(ERROR,"BinaryFile: Problem writing file header. The file will be corrupt\n");
            code = APP_ERR;
            goto skip;
         }
      }
skip:

      // Unmap the file
      if( file->Addr != MAP_FAILED ) {
         if( munmap(file->Addr,file->Header.Size) ) {
            App_Log(ERROR,"BinaryFile: Problem unmapping file. The file might be corrupt\n");
            code = APP_ERR;
         }
      }
      file->Addr = MAP_FAILED;

      // Close the file descriptor
      if( close(file->FD) ) {
         App_Log(ERROR,"BinaryFile: Problem closing file. The file might be corrupt\n");
         code = APP_ERR;
      }
      file->FD = -1;
   }

   BinaryFile_Free(Files);
   return code;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_MakeKey>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Génère la clé se référant à un champ
 *
 * Parametres :
 *    <FileIdx>   : L'index du fichier dans la liste de fichiers liés
 *    <IndexIdx>  : L'index du FieldHeader dans l'index de ce fichier
 *
 * Retour   : La clé
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static TBFKey BinaryFile_MakeKey(int32_t FileIdx,int32_t IndexIdx) {
   return FileIdx>=0&&IndexIdx>=0 ? ((TBFKey)IndexIdx)<<32|(TBFKey)FileIdx : -1;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_GetFile>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Retourne le fichier correspondant à la clé
 *
 * Parametres :
 *    <Files>  : Le pointeur vers le(s) fichier(s)
 *    <Key>    : La clé
 *
 * Retour   : NULL en cas d'erreur, le pointeur vers le fichier sinon
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static TBFFile* BinaryFile_GetFile(TBFFiles* Files,TBFKey Key) {
   int32_t idx = (int32_t)(Key&0x7fffffff);

   return Files && Key>=0 && idx>=0 && Files->N>0 && idx<Files->N ? &Files->Files[idx] : NULL;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_GetHeader>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Retourne le field header correspondant à la clé
 *
 * Parametres :
 *    <Files>  : Le pointeur vers le(s) fichier(s)
 *    <Key>    : La clé
 *
 * Retour   : NULL en cas d'erreur, le pointeur vers le field header sinon
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static TBFFldHeader* BinaryFile_GetHeader(TBFFiles* Files,TBFKey Key) {
   TBFFile *restrict file;
   int32_t idx = (int32_t)((Key&0x7fffffff00000000)>>32);

   return (file=BinaryFile_GetFile(Files,Key)) && idx>=0 && idx<file->Index.N ? &file->Index.Headers[idx] : NULL;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_Type>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Retourne le type en fonction d'un datyp FSTD et du nombre de bytes
 *
 * Parametres :
 *    <DaTyp>  : DATYP FSTD
 *    <NBytes> : Nombre de bytes du type (-1 pour default)
 *
 * Retour   : Le BF type, BF_NOTYPE si erreur
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TBFType BinaryFile_Type(int DaTyp,int NBytes) {
   switch( DaTyp ) {
      // Binary
      case 0:  return BF_BINARY;
      // Unsigned int
      case 130:
      case 2: {
         switch( NBytes ) {
            case 1:  return BF_UINT8;
            case 2:  return BF_UINT16;
            case 4:  return BF_UINT32;
            case 8:  return BF_UINT64;
            default: return BF_UINT32;
         }
      }
      // Signed int
      case 132:
      case 4: {
         switch( NBytes ) {
            case 1:  return BF_INT8;
            case 2:  return BF_INT16;
            case 4:  return BF_INT32;
            case 8:  return BF_INT64;
            default: return BF_INT32;
         }
      }
      // Float
      case 1:
      case 5:  {
         switch( NBytes ) {
            case 4:  return BF_FLOAT32;
            case 8:  return BF_FLOAT64;
            default: return BF_FLOAT32;
         }
      }
      // Compressed Float
      case 133:
      case 134: {
         switch( NBytes ) {
            case 4:  return BF_CFLOAT32;
            case 8:  return BF_CFLOAT64;
            default: return BF_CFLOAT32;
         }
      }
      // Caracter string
      case 3:
      case 7:  return BF_STRING;
      // Unsupported
      default: return BF_NOTYPE;
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_Write>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Écrire un champ
 *
 * Parametres :
 *    <Data>      : Le champ à écrire
 *    <DataType>  : Le type de donnée du champ
 *    <File>      : Le BF dans lequel écrire le champ
 *    <DateO>     : Date d'origine
 *    <Deet>      : Pas de temp (s)
 *    <NPas>      : Nombre de pas de temps depuis DateO
 *    <NI>        : Dimension du champ en I
 *    <NJ>        : Dimension du champ en J
 *    <NK>        : Dimension du champ en K
 *    <IP1>       : IP1 du champ
 *    <IP2>       : IP2 du champ
 *    <IP3>       : IP3 du champ
 *    <TypVar>    : Le type de la variable du champ
 *    <NomVar>    : Le nom de la variable du champ
 *    <Etiket>    : L'etiket du champ
 *    <GrTyp>     : Le type de grille du champ
 *    <IG1>       : Le IG1 du champ
 *    <IG2>       : Le IG2 du champ
 *    <IG3>       : Le IG3 du champ
 *    <IG4>       : Le IG4 du champ
 *
 * Retour   : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int BinaryFile_Write(void *Data,TBFType DataType,TBFFiles *File,int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,const char *Etiket,const char *GrTyp,int IG1,int IG2,int IG3,int IG4) {
   size_t size;
   void *buf;
   TBFFldHeader *restrict h;
   TBFFile *restrict file = BinaryFile_GetFile(File,0);

   // Make sure the file is open for writing
   if( !file || !(file->Flags&BF_WRITE) ) {
      App_Log(ERROR,"BinaryFile: Invalid file given or file is not opened for writing\n");
      return APP_ERR;
   }

   // Calculate an upper limit (in bytes) to the size of the field to write
   size = NI*NJ*NK*BFTypeSize[DataType];

   // From this point on, consider the file dirty (if an error occur, the field will just be ignored)
   file->Flags |= BF_DIRTY;

   switch( DataType ) {
      case BF_CFLOAT32:
         // Allocate a temporary buffer for the compressed data
         APP_MEM_ASRT( buf,malloc(size) );
         if( FPFC_Compress(Data,NI*NJ*NK,buf,size,&size) != APP_OK ) {
            // If the compression failed, just write the field uncompressed
            DataType = BF_FLOAT32;
         }
         if( write(file->FD,DataType==BF_FLOAT32?Data:buf,size) != size ) {
            App_Log(ERROR,"BinaryFile: Could not write the field : %s\n",strerror(errno));
            if( lseek(file->FD,file->Header.IOffset,SEEK_SET) == -1 ) {
               App_Log(ERROR,"BinaryFile: Could not seek to previous file position : %s\n",strerror(errno));
            }
            free(buf);
            return APP_ERR;
         }
         free(buf);
         break;
      case BF_CFLOAT64:
         APP_MEM_ASRT( buf,malloc(size) );
         if( FPFC_Compressl(Data,NI*NJ*NK,buf,size,&size) != APP_OK ) {
            // If the compression fails, just write the field uncompressed
            DataType = BF_FLOAT64;
         }
         if( write(file->FD,Data,size) != size ) {
            App_Log(ERROR,"BinaryFile: Could not write the field : %s\n",strerror(errno));
            if( lseek(file->FD,file->Header.IOffset,SEEK_SET) == -1 ) {
               App_Log(ERROR,"BinaryFile: Could not seek to previous file position : %s\n",strerror(errno));
            }
            free(buf);
            return APP_ERR;
         }
         free(buf);
         break;
      default:
         if( write(file->FD,Data,size) != size ) {
            App_Log(ERROR,"BinaryFile: Could not write the field : %s\n",strerror(errno));
            if( lseek(file->FD,file->Header.IOffset,SEEK_SET) == -1 ) {
               App_Log(ERROR,"BinaryFile: Could not seek to previous file position : %s\n",strerror(errno));
            }
            return APP_ERR;
         }
         break;
   }

   // Make space in the index
   ++file->Index.N;
   if( !(buf=realloc(file->Index.Headers,file->Index.N*sizeof(*file->Index.Headers))) ) {
      --file->Index.N;
      App_Log(ERROR,"BinaryFile: Could not allocate memory for index, field will be ignored.\n");
      return APP_ERR;
   }
   file->Index.Headers = buf;

   h = &file->Index.Headers[file->Index.N-1];

   // Fill the header
   h->KEY   = file->Header.IOffset;
   h->NBYTES= size;
   h->DATEO = DateO;
   h->DEET  = Deet;
   h->NPAS  = NPas;
   h->NBITS = BFTypeSize[DataType]*CHAR_BIT;
   h->DATYP = DataType;
   h->IP1   = IP1;
   h->IP2   = IP2;
   h->IP3   = IP3;
   h->NI    = NI;
   h->NJ    = NJ;
   h->NK    = NK;
   h->IG1   = IG1;
   h->IG2   = IG2;
   h->IG3   = IG3;
   h->IG4   = IG4;

   memset(h->TYPVAR,0,2);
   memset(h->NOMVAR,0,4);
   memset(h->ETIKET,0,12);

   memcpy(h->TYPVAR,TypVar,FtnStrSize(TypVar,2));
   memcpy(h->NOMVAR,NomVar,FtnStrSize(NomVar,4));
   memcpy(h->ETIKET,Etiket,FtnStrSize(Etiket,12));
   h->GRTYP = GrTyp[0];

   // Update the offset
   file->Header.IOffset += size;

   return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_WriteFSTD>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Écrire un champ (version compatibilité avec FSTD)
 *
 * Parametres :
 *    <Data>      : Le champ à écrire
 *    <NPak>      : Le packing du champ
 *    <File>      : Le BF dans lequel écrire le champ
 *    <DateO>     : Date d'origine
 *    <Deet>      : Pas de temp (s)
 *    <NPas>      : Nombre de pas de temps depuis DateO
 *    <NI>        : Dimension du champ en I
 *    <NJ>        : Dimension du champ en J
 *    <NK>        : Dimension du champ en K
 *    <IP1>       : IP1 du champ
 *    <IP2>       : IP2 du champ
 *    <IP3>       : IP3 du champ
 *    <TypVar>    : Le type de la variable du champ
 *    <NomVar>    : Le nom de la variable du champ
 *    <Etiket>    : L'etiket du champ
 *    <GrTyp>     : Le type de grille du champ
 *    <IG1>       : Le IG1 du champ
 *    <IG2>       : Le IG2 du champ
 *    <IG3>       : Le IG3 du champ
 *    <IG4>       : Le IG4 du champ
 *    <DaTyp>     : Le data type du champ (format FSTD)
 *    <Over>      : Overwrite? (ignoré; là seulement pour la compatibilité FSTD)
 *
 * Retour   : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
int BinaryFile_WriteFSTD(void *Data,int NPak,TBFFiles *File,int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,const char *Etiket,const char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over) {
   TBFType type=0;
    
   // Check the data type
   if( (type=BinaryFile_Type(DaTyp,-1)) == BF_NOTYPE ) {
      App_Log(ERROR,"BinaryFile: Could not write field : %d is an unsupported data type (DATYP).\n",DaTyp);
      return APP_ERR;
   }

   return BinaryFile_Write(Data,type,File,DateO,Deet,NPas,NI,NJ,NK,IP1,IP2,IP3,TypVar,NomVar,Etiket,GrTyp,IG1,IG2,IG3,IG4);
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_Find>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Recherche un champ dans l'index
 *
 * Parametres :
 *    <File>      : Le BF dans lequel chercher le champ
 *    <NI>        : [OUT] Dimension du champ en I
 *    <NJ>        : [OUT] Dimension du champ en J
 *    <NK>        : [OUT] Dimension du champ en K
 *    <DateV>     : Date valide (-1 pour toutes les dates)
 *    <Etiket>    : L'etiket du champ (String vide ou NULL pour toutes les etiket)
 *    <IP1>       : IP1 du champ (-1 pour tous les IP1)
 *    <IP2>       : IP2 du champ (-1 pour tous les IP2)
 *    <IP3>       : IP3 du champ (-1 pour tous les IP3)
 *    <TypVar>    : Le type de la variable du champ (String vide ou NULL pour tous les typvar)
 *    <NomVar>    : Le nom de la variable du champ (String vide ou NULL pour tous les nomvar)
 *
 * Retour   : La clé du premier champ trouvé matchant tous les critères ou -1 si pas trouvé
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
static int GetDateV(int DateO,int Deet,int Npas) {
#ifdef HAVE_RMN
   if( !DateO ) return 0;
   // Calculer la date de validitee du champs
   int datev;
   double nhour=(Npas*Deet)/3600.0;
   f77name(incdatr)(&datev,&DateO,&nhour);
   return datev!=101010101 ? datev : 0;
#else
   App_Log(ERROR,"%s: Need RMNLIB\n",__func__);
   return 0;
#endif
}
TBFKey BinaryFile_Find(TBFFiles *File,int *NI,int *NJ,int *NK,int DateV,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar) {
   TBFFldHeader *restrict h;
   TBFFile *restrict file;
   int32_t i,f;
   int nnv=0,ntv=0,net=0;
   int nonv,notv,noet;

   if( !File )
      return -1;

   // This is needed because fortran whistespace pads its strings, so giving an fstprm output would be a problem
   nnv = FtnStrSize(NomVar,4);
   ntv = FtnStrSize(TypVar,2);
   net = FtnStrSize(Etiket,12);

   nonv = !NomVar || NomVar[0]=='\0' || !nnv;
   notv = !TypVar || TypVar[0]=='\0' || !ntv;
   noet = !Etiket || Etiket[0]=='\0' || !net;

   // Look for the field in the index
   for(f=0,file=File->Files; f<File->N; ++f,++file) {
      for(i=0,h=file->Index.Headers; i<file->Index.N; ++i,++h) {
         //printf("Comparing datev=(%d|%d) ip1=(%d|%d) ip2=(%d|%d) ip3=(%d|%d) NomVar=(%.*s|%.4s) TypVar=(%.*s|%.2s) Etiket=(%.*s|%.12s)\n",DateV,GetDateV(h->DATEO,h->DEET,h->NPAS),IP1,h->IP1,IP2,h->IP2,IP3,h->IP3,nnv,NomVar,h->NOMVAR,ntv,TypVar,h->TYPVAR,net,Etiket,h->ETIKET);
         //printf("DateV[%d] IP1[%d] IP2[%d] IP3[%d] NOMVAR[%d] TYPVAR[%d] ETIKET[%d]\n",(DateV==-1 || DateV==GetDateV(h->DATEO,h->DEET,h->NPAS),(IP1==-1 || IP1==h->IP1),(IP2==-1 || IP2==h->IP2),(IP3==-1 || IP3==h->IP3),(nonv || nnv==strnlen(h->NOMVAR,4) && !strncmp(NomVar,h->NOMVAR,nnv)),(notv || ntv==strnlen(h->TYPVAR,2) && !strncmp(TypVar,h->TYPVAR,ntv)),(noet || net==strnlen(h->ETIKET,12) && !strncmp(Etiket,h->ETIKET,net)));
         if( (DateV==-1 || DateV==GetDateV(h->DATEO,h->DEET,h->NPAS))
               && (IP1==-1 || IP1==h->IP1)
               && (IP2==-1 || IP2==h->IP2)
               && (IP3==-1 || IP3==h->IP3)
               && (nonv || nnv==strnlen(h->NOMVAR,4) && !strncmp(NomVar,h->NOMVAR,nnv))
               && (notv || ntv==strnlen(h->TYPVAR,2) && !strncmp(TypVar,h->TYPVAR,ntv))
               && (noet || net==strnlen(h->ETIKET,12) && !strncmp(Etiket,h->ETIKET,net)) ) {
            // We found the field, return its key
            //printf("Found field datev=(%d|%d) ip1=(%d|%d) ip2=(%d|%d) ip3=(%d|%d) NomVar=(%.*s|%.4s) TypVar=(%.*s|%.2s) Etiket=(%.*s|%.12s)\n",DateV,GetDateV(h->DATEO,h->DEET,h->NPAS),IP1,h->IP1,IP2,h->IP2,IP3,h->IP3,nnv,NomVar,h->NOMVAR,ntv,TypVar,h->TYPVAR,net,Etiket,h->ETIKET);
            //printf("Note that nnv=%d ntv=%d net=%d nlenNV=%d nlenTV=%d nlenET=%d\n",nnv,ntv,net,(int)strnlen(h->NOMVAR,4),(int)strnlen(h->TYPVAR,2),(int)strnlen(h->ETIKET,12));
            *NI=h->NI; *NJ=h->NJ; *NK=h->NK;
            return BinaryFile_MakeKey(f,i);
         }
      }
   }

   return -1;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_ReadIndex>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Lit un champ
 *
 * Parametres :
 *    <Buf>       : [OUT] Buffer dans lequel lire le champ
 *    <Key>       : Clé du champ à lire
 *    <File>      : Handle vers le fichier BF
 *
 * Retour   : La clé du champ lu ou -1 si pas trouvé
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TBFKey BinaryFile_ReadIndex(void *Buf,TBFKey Key,TBFFiles *File) {
   TBFFldHeader *restrict h;
   TBFFile *restrict file;
   void *addr;

   // Make sure we have a valid file and key
   if( !File || !(File->Flags&BF_READ) || !(file=BinaryFile_GetFile(File,Key)) || !(h=BinaryFile_GetHeader(File,Key)) ) {
      return -1;
   }

   // Check if we have a file mapping or we are using traditionnal means
   if( file->Addr != MAP_FAILED ) {
      // Get the address where the data is
      addr = AddrAt(file->Addr,h->KEY);

      // Read the bytes
      switch( h->DATYP ) {
         case BF_CFLOAT32:
            if( FPFC_Inflate(Buf,h->NI*h->NJ*h->NK,addr,h->NBYTES) != APP_OK ) {
               return -1;
            }
            break;
         case BF_CFLOAT64:
            if( FPFC_Inflatel(Buf,h->NI*h->NJ*h->NK,addr,h->NBYTES) != APP_OK ) {
               return -1;
            }
            break;
         default:
            memcpy(Buf,addr,h->NI*h->NJ*h->NK*BFTypeSize[h->DATYP]);
            break;
      }
   } else {
      // Read the bytes
      if( h->DATYP==BF_CFLOAT32 || h->DATYP==BF_CFLOAT64 ) {
         // Allocate a temporary buffer
         if( !(addr=malloc(h->NBYTES)) ) {
            App_Log(ERROR,"BinaryFile: Could not allocate memory for temporary buffer\n");
            return -1;
         }
         // Read the compressed bytes
         if( pread(file->FD,addr,h->NBYTES,h->KEY) != h->NBYTES ) {
            App_Log(ERROR,"BinaryFile: Could not read compressed field\n");
            free(addr);
            return -1;
         }
         // Uncompress the bytes
         if( h->DATYP==BF_CFLOAT32 && FPFC_Inflate(Buf,h->NI*h->NJ*h->NK,addr,h->NBYTES)!=APP_OK
               || h->DATYP==BF_CFLOAT64 && FPFC_Inflatel(Buf,h->NI*h->NJ*h->NK,addr,h->NBYTES)!=APP_OK ) {
            free(addr);
            return -1;
         }
         free(addr);
      } else {
         if( pread(file->FD,Buf,h->NBYTES,h->KEY) != h->NBYTES ) {
            App_Log(ERROR,"BinaryFile: Could not read field\n");
            return -1;
         }
      }
   }

   return Key;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_Read>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Lit un champ
 *
 * Parametres :
 *    <Buf>       : [OUT] Buffer dans lequel lire le champ
 *    <File>      : Le BF dans lequel lire le champ
 *    <NI>        : [OUT] Dimension du champ en I
 *    <NJ>        : [OUT] Dimension du champ en J
 *    <NK>        : [OUT] Dimension du champ en K
 *    <DateV>     : Date valide (-1 pour toutes les dates)
 *    <Etiket>    : L'etiket du champ (String vide ou NULL pour toutes les etiket)
 *    <IP1>       : IP1 du champ (-1 pour tous les IP1)
 *    <IP2>       : IP2 du champ (-1 pour tous les IP2)
 *    <IP3>       : IP3 du champ (-1 pour tous les IP3)
 *    <TypVar>    : Le type de la variable du champ (String vide ou NULL pour tous les typvar)
 *    <NomVar>    : Le nom de la variable du champ (String vide ou NULL pour tous les nomvar)
 *
 * Retour   : La clé du premier champ trouvé et lu matchant tous les critères ou -1 si pas trouvé
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TBFKey BinaryFile_Read(void *Buf,TBFFiles *File,int *NI,int *NJ,int *NK,int DateV,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar) {
   TBFKey key;
   
   if( (key=BinaryFile_Find(File,NI,NJ,NK,DateV,Etiket,IP1,IP2,IP3,TypVar,NomVar))>=0 ) {
      return BinaryFile_ReadIndex(Buf,key,File);
   }
   return key;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_Convert>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Converti un champ d'un type compatible à un autre
 *
 * Parametres :
 *    <DestType>  : Destination type
 *    <DestBuf>   : [OUT] Destination buffer
 *    <SrcType>   : Source type
 *    <SrcBuf>    : Source buffer
 *    <N>         : Siza of the field
 *
 * Retour   : APP_OK si ok, APP_ERR sinon
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
#define BF_CONVERT_LOOP(DestType,DestBuf,SrcType,SrcBuf,N) { \
   int i; \
   for(i=0; i<N; ++i) { \
      DestBuf[i] = (DestType)((SrcType*)SrcBuf)[i]; \
   } \
}
#define BF_CONVERT_SRC(DestType,DestBuf,SrcBFType,SrcBuf,N) { \
   DestType *restrict dest = DestBuf; \
   switch( SrcBFType ) { \
      case BF_STRING:   BF_CONVERT_LOOP(DestType,dest,char,SrcBuf,N);       break; \
      case BF_BINARY:   BF_CONVERT_LOOP(DestType,dest,char,SrcBuf,N);       break; \
      case BF_INT8:     BF_CONVERT_LOOP(DestType,dest,int8_t,SrcBuf,N);     break; \
      case BF_INT16:    BF_CONVERT_LOOP(DestType,dest,int16_t,SrcBuf,N);    break; \
      case BF_INT32:    BF_CONVERT_LOOP(DestType,dest,int32_t,SrcBuf,N);    break; \
      case BF_INT64:    BF_CONVERT_LOOP(DestType,dest,int64_t,SrcBuf,N);    break; \
      case BF_UINT8:    BF_CONVERT_LOOP(DestType,dest,uint8_t,SrcBuf,N);    break; \
      case BF_UINT16:   BF_CONVERT_LOOP(DestType,dest,uint16_t,SrcBuf,N);   break; \
      case BF_UINT32:   BF_CONVERT_LOOP(DestType,dest,uint32_t,SrcBuf,N);   break; \
      case BF_UINT64:   BF_CONVERT_LOOP(DestType,dest,uint64_t,SrcBuf,N);   break; \
      case BF_FLOAT32:  BF_CONVERT_LOOP(DestType,dest,float,SrcBuf,N);      break; \
      case BF_FLOAT64:  BF_CONVERT_LOOP(DestType,dest,double,SrcBuf,N);     break; \
      case BF_NOTYPE:   \
      default: return APP_ERR;   \
   } \
}
static int BinaryFile_Convert(TBFType DestType,void* DestBuf,TBFType SrcType,void* SrcBuf,size_t N) {
   switch( DestType ) {
      case BF_STRING:   BF_CONVERT_SRC(char,DestBuf,SrcType,SrcBuf,N);      break;
      case BF_BINARY:   BF_CONVERT_SRC(char,DestBuf,SrcType,SrcBuf,N);      break;
      case BF_INT8:     BF_CONVERT_SRC(int8_t,DestBuf,SrcType,SrcBuf,N);    break;
      case BF_INT16:    BF_CONVERT_SRC(int16_t,DestBuf,SrcType,SrcBuf,N);   break;
      case BF_INT32:    BF_CONVERT_SRC(int32_t,DestBuf,SrcType,SrcBuf,N);   break;
      case BF_INT64:    BF_CONVERT_SRC(int64_t,DestBuf,SrcType,SrcBuf,N);   break;
      case BF_UINT8:    BF_CONVERT_SRC(uint8_t,DestBuf,SrcType,SrcBuf,N);   break;
      case BF_UINT16:   BF_CONVERT_SRC(uint16_t,DestBuf,SrcType,SrcBuf,N);  break;
      case BF_UINT32:   BF_CONVERT_SRC(uint32_t,DestBuf,SrcType,SrcBuf,N);  break;
      case BF_UINT64:   BF_CONVERT_SRC(uint64_t,DestBuf,SrcType,SrcBuf,N);  break;
      case BF_FLOAT32:  BF_CONVERT_SRC(float,DestBuf,SrcType,SrcBuf,N);     break;
      case BF_FLOAT64:  BF_CONVERT_SRC(double,DestBuf,SrcType,SrcBuf,N);    break;
      case BF_NOTYPE:
      default: return APP_ERR;
   }

   return APP_OK;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_ReadIndexInto>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Lit un champ et le converti au type spécifié
 *
 * Parametres :
 *    <Buf>       : [OUT] Buffer dans lequel lire le champ
 *    <Key>       : Clé du champ à lire
 *    <File>      : Handle vers le fichier BF
 *    <DestType>  : Type dans lequel on veut le champ
 *
 * Retour   : La clé du champ lu ou -1 si pas trouvé
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TBFKey BinaryFile_ReadIndexInto(void *Buf,TBFKey Key,TBFFiles *File,TBFType DestType) {
   TBFFldHeader *restrict h;
   TBFFile *restrict file;
   void *tbuf=NULL;

   // Make sure we have a valid file and key
   if( !File || !(File->Flags&BF_READ) || !(file=BinaryFile_GetFile(File,Key)) || !(h=BinaryFile_GetHeader(File,Key)) ) {
      return -1;
   }

   // Check if we need to convert
   if( h->DATYP == DestType ) {
      return BinaryFile_ReadIndex(tbuf,Key,File);
   }

   // Allocate a temporary buffer to read the data into
   size_t n = h->NI*h->NJ*h->NK;
   if( !(tbuf=malloc(BFTypeSize[DestType]*n)) ) {
      App_Log(ERROR,"BinaryFile: Could not allocate memory for temporary buffer\n");
      return -1;
   }

   // Read the data
   if( BinaryFile_ReadIndex(tbuf,Key,File) < 0 ) {
      free(tbuf);
      return -1;
   }

   // Convert the data
   if( BinaryFile_Convert(DestType,Buf,h->DATYP,tbuf,n)!=APP_OK ) {
      App_Log(ERROR,"BinaryFile: Could not convert from type %d to type %d\n",h->DATYP,DestType);
      free(tbuf);
      return -1;
   }

   // Success
   free(tbuf);
   return Key;
}

/*----------------------------------------------------------------------------
 * Nom      : <BinaryFile_Read>
 * Creation : Février 2017 - E. Legault-Ouellet - CMC/CMOE
 *
 * But      : Lit un champ et le converti au type spécifié
 *
 * Parametres :
 *    <Buf>       : [OUT] Buffer dans lequel lire le champ
 *    <File>      : Le BF dans lequel lire le champ
 *    <NI>        : [OUT] Dimension du champ en I
 *    <NJ>        : [OUT] Dimension du champ en J
 *    <NK>        : [OUT] Dimension du champ en K
 *    <DateV>     : Date valide (-1 pour toutes les dates)
 *    <Etiket>    : L'etiket du champ (String vide ou NULL pour toutes les etiket)
 *    <IP1>       : IP1 du champ (-1 pour tous les IP1)
 *    <IP2>       : IP2 du champ (-1 pour tous les IP2)
 *    <IP3>       : IP3 du champ (-1 pour tous les IP3)
 *    <TypVar>    : Le type de la variable du champ (String vide ou NULL pour tous les typvar)
 *    <NomVar>    : Le nom de la variable du champ (String vide ou NULL pour tous les nomvar)
 *    <DestType>  : Type dans lequel on veut le champ
 *
 * Retour   : La clé du premier champ trouvé et lu matchant tous les critères ou -1 si pas trouvé
 *
 * Remarques :
 *
 *----------------------------------------------------------------------------
 */
TBFKey BinaryFile_ReadInto(void *Buf,TBFFiles *File,int *NI,int *NJ,int *NK,int DateV,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,TBFType DestType) {
   TBFKey key;
   
   if( (key=BinaryFile_Find(File,NI,NJ,NK,DateV,Etiket,IP1,IP2,IP3,TypVar,NomVar))!=-1 ) {
      return BinaryFile_ReadIndexInto(Buf,key,File,DestType);
   }
   return -1;
}
