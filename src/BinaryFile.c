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
#include "FPCompressF.h"
#include "FPCompressD.h"
#include "App.h"
#include <string.h>

const int32_t BF_MAGIC=0x45454642; //BFEE (Binary File Env. Emergencies) in little endian
const int32_t BF_VERSION=1;

static int BFTypeSize[] = {1,1,1,2,4,8,1,2,4,8,4,8,4,8,0};

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
      file->FD    = NULL;
      file->Header= (TBFFileHeader){sizeof(TBFFileHeader),0,BF_MAGIC,BF_VERSION,sizeof(TBFFileHeader),sizeof(TBFFldHeader)};
      file->Index = (TBFIndex){NULL,0};
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
         if( file->FD ) {
            fclose(file->FD);
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
   int      i,append=0;
   char     mode[]={'\0','b','\0','\0'};

   // If memory was not allocated, we allocate it
   if( !files ) {
      App_Log(ERROR,"BinaryFile: Could not allocate needed memory\n");
      goto error;
   }

   // Limit to relevant flags
   Mode &= BF_READ|BF_WRITE|BF_CLEAR;

   // Make sure we have at least one of READ or WRITE
   if( !(Mode&(BF_READ|BF_WRITE)) ) {
      App_Log(ERROR,"BinaryFile: Either read or write mode must be selected when opening files");
      goto error;
   }

   // Translate our flags into flags accepted by fopen
   if( Mode&BF_READ || !(Mode&BF_CLEAR) ) {
      mode[0] = 'r';
      // Add write mode if needed
      if( Mode&BF_WRITE )
         mode[2] = '+';
   } else {
      // The file is opened write-only with overwrite on
      mode[0] = 'w';
   }

   files->Flags |= Mode;
   
   for(i=0,file=files->Files; i<N; ++i,++file) {
      // If we are in write-only non-overwrite mode
      if( !(Mode&BF_READ) && !(Mode&BF_CLEAR) ) {
         FILE* fd;
         long pos;
         // Open the file in append mode, creating it if it doesn't exist, and then close it
         // This ensures that the file exists so that we can open it in r+ mode afterwards
         if( !(fd=fopen(FileNames[i],"ab")) ) {
            App_Log(ERROR,"BinaryFile: File %s could not be created or accessed. Are you sure the path exists and you have write permissions on that path/file?\n",FileNames[i]);
            goto error;
         }
         if( (pos=ftell(fd))<0 ) {
            App_Log(ERROR,"BinaryFile: Can't tell where we are in the file %s. Aborting...\n",FileNames[i]);
            goto error;
         }
         append = pos!=0;
         fclose(fd);
      }
      
      // Open the file
      if( !(file->FD=fopen(FileNames[i],mode)) ) {
         App_Log(ERROR,"BinaryFile: Problem opening file %s in mode %s\n",FileNames[i],mode);
         goto error;
      }

      file->Flags |= Mode;

      if( Mode&BF_READ || append ) {
         // Read the file header
         if( fread(&file->Header,sizeof(file->Header),1,file->FD)!=1 ) {
            App_Log(ERROR,"BinaryFile: Problem reading header for file %s\n",FileNames[i]);
            goto error;
         }

         // Make sure we have a valid BinaryFile
         if( file->Header.Magic != BF_MAGIC ) {
            App_Log(ERROR,"BinaryFile: File %s is not of BinaryFile type\n",FileNames[i]);
            goto error;
         }

         // Make sure we will have the right binary offsets
         if( file->Header.FSize!=sizeof(TBFFileHeader) || file->Header.HSize!=sizeof(TBFFldHeader) ) {
            App_Log(ERROR,"BinaryFile: This library is incompatible with this BinaryFile (%s) FileHeader is %d bytes (library: %d) and the FldHeader is %u bytes (library: %d).\n",
                  FileNames[i],file->Header.FSize,sizeof(TBFFileHeader),file->Header.HSize,sizeof(TBFFldHeader));
            goto error;
         }

         // Seek to the index
         if( fseek(file->FD,file->Header.IOffset,SEEK_SET) ) {
            App_Log(ERROR,"BinaryFile: Could not seek to index in file %s\n",FileNames[i]);
            goto error;
         }

         // Read the index size
         if( fread(&file->Index.N,sizeof(file->Index.N),1,file->FD)!=1 ) {
            App_Log(ERROR,"BinaryFile: Problem reading index size for file %s\n",FileNames[i]);
            goto error;
         }

         // Allocate the memory for the index
         if( !(file->Index.Headers=malloc(file->Index.N*sizeof(*file->Index.Headers))) ) {
            App_Log(ERROR,"BinaryFile: Could not allocate memory for index\n");
            goto error;
         }

         // Read the index
         if( fread(file->Index.Headers,sizeof(*file->Index.Headers),file->Index.N,file->FD)!=file->Index.N ) {
            App_Log(ERROR,"BinaryFile: Problem reading index for file %s\n",FileNames[i]);
            goto error;
         }

         // Mark the fact that we are not in the right position to write a file
         file->Flags |= BF_SEEKED;
      } else {
         // Write an invalid header as a place holder
         if( fwrite(&file->Header,sizeof(file->Header),1,file->FD)!=1 ) {
            App_Log(ERROR,"BinaryFile: Problem writing header for file %s\n",FileNames[i]);
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

   if( !Files )
      return APP_ERR;

   for(i=0,file=Files->Files; i<Files->N; ++i,++file) {
      // Only write something if something changed
      if( file->Flags&BF_DIRTY ) {
         // If we are at another place than the end of the file
         if( file->Flags&BF_SEEKED ) {
            // Seek to the place where we need to write the FileHeader
            if( fseek(file->FD,file->Header.IOffset,SEEK_SET) ) {
               App_Log(ERROR,"BinaryFile: Could not seek to file index. The file will be corrupt\n");
               code = APP_ERR;
               continue;
            }
         }

         // Write the index
         if( fwrite(&file->Index.N,sizeof(file->Index.N),1,file->FD)!=1 ) {
            App_Log(ERROR,"BinaryFile: Problem writing file index. The file will be corrupt\n");
            code = APP_ERR;
            continue;
         }
         if( fwrite(file->Index.Headers,sizeof(*file->Index.Headers),file->Index.N,file->FD)!=file->Index.N ) {
            App_Log(ERROR,"BinaryFile: Problem writing file index. The file will be corrupt\n");
            code = APP_ERR;
            continue;
         }

         // Update the total file size
         file->Header.Size = file->Header.IOffset+sizeof(file->Index.N)+sizeof(*file->Index.Headers)*file->Index.N;

         // Seek at the start of the file
         if( fseek(file->FD,0,SEEK_SET) ) {
            App_Log(ERROR,"BinaryFile: Could not seek to file header. The file will be corrupt\n");
            code = APP_ERR;
            continue;
         }

         // Update the file header
         if( fwrite(&file->Header,sizeof(file->Header),1,file->FD)!=1 ) {
            App_Log(ERROR,"BinaryFile: Problem writing file header. The file will be corrupt\n");
            code = APP_ERR;
            continue;
         }
      }

      // Close the file descriptor
      if( fclose(file->FD) ) {
         file->FD = NULL;
         App_Log(ERROR,"BinaryFile: Problem closing file. The file might be corrupt\n");
         code = APP_ERR;
         continue;
      }
      file->FD = NULL;
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
      case 5:  {
         switch( NBytes ) {
            case 4:  return BF_FLOAT32;
            case 8:  return BF_FLOAT64;
            default: return BF_FLOAT32;
         }
      }
      // Compressed Float
      case 133: {
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
      App_Log(ERROR,"BinaryFile: Invalid file given or file is not openned for writing\n");
      return APP_ERR;
   }

   // Make sure we are at the "end" of the file
   if( file->Flags&BF_SEEKED ) {
      // Seek to the place where we need to write the Index
      if( fseek(file->FD,file->Header.IOffset,SEEK_SET) ) {
         App_Log(ERROR,"BinaryFile: Could not seek to the end of the file, aborting.\n");
         return APP_ERR;
      }
      // Reset the flag
      file->Flags &= ~BF_SEEKED;
   }

   // Write the field
   switch( DataType ) {
      case BF_CFLOAT32:
         APP_ASRT_OK( FPC_CompressF(file->FD,Data,NI,NJ,NK,&size) );
         break;
      case BF_CFLOAT64:
         APP_ASRT_OK( FPC_CompressD(file->FD,Data,NI,NJ,NK,&size) );
         break;
      default:
         size = NI*NJ*NK;
         if( fwrite(Data,BFTypeSize[DataType],size,file->FD)!=size ) {
            App_Log(ERROR,"BinaryFile: Problem writing field. The file will be corrupt\n");
            return APP_ERR;
         }
         size *= BFTypeSize[DataType];
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
   h->NBITS = BFTypeSize[DataType]*4;
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

   // Flag the update to the index
   file->Flags |= BF_DIRTY;

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

   // Make sure we have a valid file and key
   if( !File || !(File->Flags&BF_READ) || !(file=BinaryFile_GetFile(File,Key)) || !(h=BinaryFile_GetHeader(File,Key)) ) {
      return -1;
   }

   // Seek to the field's position in the file
   if( fseek(file->FD,h->KEY,SEEK_SET) ) {
      App_Log(ERROR,"BinaryFile: Could not seek to field position\n");
      return -1;
   }

   // Read the bytes
   switch( h->DATYP ) {
      case BF_CFLOAT32:
         APP_ASRT_OK( FPC_InflateF(file->FD,Buf,h->NI,h->NJ,h->NK) );
         break;
      case BF_CFLOAT64:
         APP_ASRT_OK( FPC_InflateD(file->FD,Buf,h->NI,h->NJ,h->NK) );
         break;
      default: {
         size_t n = h->NI*h->NJ*h->NK;
         if( fread(Buf,BFTypeSize[h->DATYP],n,file->FD)!=n ) {
            App_Log(ERROR,"BinaryFile: Could not seek to field position\n");
            return -1;
         }
         break;
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
