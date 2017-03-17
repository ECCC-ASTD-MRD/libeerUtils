#include "BinaryFile.h"
#include "App.h"
#include <string.h>

const int32_t BF_MAGIC=0x45454642; //BFEE (Binary File Env. Emergencies) in little endian
const int32_t BF_VERSION=1;

static int BFTypeSize[] = {1,1,1,2,4,8,1,2,4,8,4,8,0};

size_t FtnStrSize(const char *Str,size_t Max) {
   size_t n;

   if( !Str )
      return 0;

   for(n=strnlen(Str,Max); n>0&&Str[n-1]==' '; --n)
      ;
   return n;
}

TBFFiles* BinaryFile_New(int N) {
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

void BinaryFile_Free(TBFFiles *Files) {
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

TBFFiles* BinaryFile_OpenFiles(const char **FileNames,int N,TBFFlag Mode) {
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

TBFFiles* BinaryFile_Open(const char *FileName,TBFFlag Mode) {
   return BinaryFile_OpenFiles(&FileName,1,Mode);
}

TBFFiles* BinaryFile_Link(const char** FileNames) {
   int n;

   if( !FileNames )
      return NULL;

   for(n=0; FileNames[n]; ++n)
      ;
   return BinaryFile_OpenFiles(FileNames,n,BF_READ);
}

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

TBFKey BinaryFile_MakeKey(int32_t FileIdx,int32_t IndexIdx) {
   return FileIdx>=0&&IndexIdx>=0 ? ((TBFKey)IndexIdx)<<32|(TBFKey)FileIdx : -1;
}

TBFFile* BinaryFile_GetFile(TBFFiles* Files,TBFKey Key) {
   int32_t idx = (int32_t)(Key&0x7fffffff);

   return Files && Key>=0 && idx>=0 && Files->N>0 && idx<Files->N ? &Files->Files[idx] : NULL;
}

TBFFldHeader* BinaryFile_GetHeader(TBFFiles* Files,TBFKey Key) {
   TBFFile *restrict file;
   int32_t idx = (int32_t)((Key&0x7fffffff00000000)>>32);

   return (file=BinaryFile_GetFile(Files,Key)) && idx>=0 && idx<file->Index.N ? &file->Index.Headers[idx] : NULL;
}

TBFType BinaryFile_Type(int DaTyp,int NBytes) {
   // Compressed types
   if( DaTyp >= 128 )
      DaTyp -= 128;

   switch( DaTyp ) {
      // Binary
      case 0:  return BF_BINARY;
      // Unsigned int
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
      // Caracter string
      case 3:
      case 7:  return BF_STRING;
      // Unsupported
      default: return BF_NOTYPE;
   }
}

int BinaryFile_Write(void *Data,TBFType DataType,TBFFiles *File,int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,const char *Etiket,const char *GrTyp,int IG1,int IG2,int IG3,int IG4) {
   int size = NI*NJ*NK;
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
   if( fwrite(Data,BFTypeSize[DataType],size,file->FD)!=size ) {
      App_Log(ERROR,"BinaryFile: Problem writing field. The file will be corrupt\n");
      return APP_ERR;
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
   h->NBYTES= BFTypeSize[DataType]*size;
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
   file->Header.IOffset += size*BFTypeSize[DataType];

   // Flag the update to the index
   file->Flags |= BF_DIRTY;

   return APP_OK;
}

int BinaryFile_WriteFSTD(void *Data,int NPak,TBFFiles *File,int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,const char *Etiket,const char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over) {
   TBFType type=0;
    
   // Check the data type
   if( (type=BinaryFile_Type(DaTyp,-1)) == BF_NOTYPE ) {
      App_Log(ERROR,"BinaryFile: Could not write field : %d is an unsupported data type (DATYP).\n",DaTyp);
      return APP_ERR;
   }

   return BinaryFile_Write(Data,type,File,DateO,Deet,NPas,NI,NJ,NK,IP1,IP2,IP3,TypVar,NomVar,Etiket,GrTyp,IG1,IG2,IG3,IG4);
}

TBFKey BinaryFile_Find(TBFFiles *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar) {
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
         //printf("Comparing dateo=(%d|%d) ip1=(%d|%d) ip2=(%d|%d) ip3=(%d|%d) NomVar=(%.*s|%.4s) TypVar=(%.*s|%.2s) Etiket=(%.*s|%.12s)\n",DateO,h->DATEO,IP1,h->IP1,IP2,h->IP2,IP3,h->IP3,nnv,NomVar,h->NOMVAR,ntv,TypVar,h->TYPVAR,net,Etiket,h->ETIKET);
         //printf("DateO[%d] IP1[%d] IP2[%d] IP3[%d] NOMVAR[%d] TYPVAR[%d] ETIKET[%d]\n",(DateO==-1 || DateO==h->DATEO),(IP1==-1 || IP1==h->IP1),(IP2==-1 || IP2==h->IP2),(IP3==-1 || IP3==h->IP3),(nonv || nnv==strnlen(h->NOMVAR,4) && !strncmp(NomVar,h->NOMVAR,nnv)),(notv || ntv==strnlen(h->TYPVAR,2) && !strncmp(TypVar,h->TYPVAR,ntv)),(noet || net==strnlen(h->ETIKET,12) && !strncmp(Etiket,h->ETIKET,net)));
         if( (DateO==-1 || DateO==h->DATEO)
               && (IP1==-1 || IP1==h->IP1)
               && (IP2==-1 || IP2==h->IP2)
               && (IP3==-1 || IP3==h->IP3)
               && (nonv || nnv==strnlen(h->NOMVAR,4) && !strncmp(NomVar,h->NOMVAR,nnv))
               && (notv || ntv==strnlen(h->TYPVAR,2) && !strncmp(TypVar,h->TYPVAR,ntv))
               && (noet || net==strnlen(h->ETIKET,12) && !strncmp(Etiket,h->ETIKET,net)) ) {
            // We found the field, return its key
            //printf("Found field dateo=(%d|%d) ip1=(%d|%d) ip2=(%d|%d) ip3=(%d|%d) NomVar=(%.*s|%.4s) TypVar=(%.*s|%.2s) Etiket=(%.*s|%.12s)\n",DateO,h->DATEO,IP1,h->IP1,IP2,h->IP2,IP3,h->IP3,nnv,NomVar,h->NOMVAR,ntv,TypVar,h->TYPVAR,net,Etiket,h->ETIKET);
            //printf("Note that nnv=%d ntv=%d net=%d nlenNV=%d nlenTV=%d nlenET=%d\n",nnv,ntv,net,(int)strnlen(h->NOMVAR,4),(int)strnlen(h->TYPVAR,2),(int)strnlen(h->ETIKET,12));
            *NI=h->NI; *NJ=h->NJ; *NK=h->NK;
            return BinaryFile_MakeKey(f,i);
         }
      }
   }

   return -1;
}

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
   // TODO the size of each item may vary based on the packing/compression/etc.
   size_t n = h->NI*h->NJ*h->NK;
   if( fread(Buf,BFTypeSize[h->DATYP],n,file->FD)!=n ) {
      App_Log(ERROR,"BinaryFile: Could not seek to field position\n");
      return -1;
   }

   return Key;
}

TBFKey BinaryFile_Read(void *Buf,TBFFiles *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar) {
   TBFKey key;
   
   if( (key=BinaryFile_Find(File,NI,NJ,NK,DateO,Etiket,IP1,IP2,IP3,TypVar,NomVar))>=0 ) {
      return BinaryFile_ReadIndex(Buf,key,File);
   }
   return key;
}

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
int BinaryFile_Convert(TBFType DestType,void* DestBuf,TBFType SrcType,void* SrcBuf,size_t N) {
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

TBFKey BinaryFile_ReadInto(void *Buf,TBFFiles *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,TBFType DestType) {
   TBFKey key;
   
   if( (key=BinaryFile_Find(File,NI,NJ,NK,DateO,Etiket,IP1,IP2,IP3,TypVar,NomVar))!=-1 ) {
      return BinaryFile_ReadIndexInto(Buf,key,File,DestType);
   }
   return -1;
}
