#include "BinaryFile.h"
#include "App.h"

#define MAX(a,b) ((a)>=(b)?(a):(b))

const int32_t BF_MAGIC=0x45454642; //BFEE (Binary File Env. Emergencies) in little endian
const int32_t BF_VERSION=1;

static int BFTypeSize[] = {1,1,1,2,4,8,1,2,4,8,4,8,0};

TBFFile* BinaryFile_New() {
   TBFFile *file = malloc(sizeof(*file));

   if( file ) {
      file->FD    = NULL;
      file->Header= (TBFFileHeader){sizeof(TBFFileHeader),0,BF_MAGIC,BF_VERSION,sizeof(TBFFileHeader),sizeof(TBFFldHeader)};
      file->Index = (TBFIndex){NULL,0};
      file->Flags = 0;
   }

   return file;
}

void BinaryFile_Free(TBFFile *File) {
   if( File ) {
      if( File->FD ) {
         fclose(File->FD);
         File->FD=NULL;
      }

      APP_FREE(File->Index.Headers);

      free(File);
   }
}

TBFFile* BinaryFile_Open(const char *FileName,TBFFlag Mode) {
   TBFFile        *file=BinaryFile_New();
   const char     *mode;

   // Make sure we had enough memory
   if( !file ) {
      App_Log(ERROR,"BinaryFile: Could not allocate needed memory\n",FileName);
      goto error;
   }

   // Get the string attached to the selected mode
   switch(Mode) {
      case BF_READ:           mode="rb";  break;
      case BF_WRITE:          mode="wb";  break;
      case BF_READ|BF_WRITE:  mode="rb+"; break;
      default:
         App_Log(ERROR,"BinaryFile: Unsupported mode : %d\n",Mode);
         goto error;
   }

   file->Flags |= Mode;
   
   // Open the file
   if( !(file->FD=fopen(FileName,mode)) ) {
      App_Log(ERROR,"BinaryFile: Problem opening file %s in mode %s\n",FileName,mode);
      goto error;
   }

   if( Mode&BF_READ ) {
      // Read the file header
      if( fread(&file->Header,sizeof(file->Header),1,file->FD)!=1 ) {
         App_Log(ERROR,"BinaryFile: Problem reading header for file %s\n",FileName);
         goto error;
      }

      // Make sure we have a valid BinaryFile
      if( file->Header.Magic != BF_MAGIC ) {
         App_Log(ERROR,"BinaryFile: File %s is not of BinaryFile type\n",FileName);
         goto error;
      }

      // Make sure we will have the right binary offsets
      if( file->Header.FSize!=sizeof(TBFFileHeader) || file->Header.HSize!=sizeof(TBFFldHeader) ) {
         App_Log(ERROR,"BinaryFile: This library is incompatible with this BinaryFile (%s) FileHeader is %d bytes (library: %d) and the FldHeader is %u bytes (library: %d).\n",
               FileName,file->Header.FSize,sizeof(TBFFileHeader),file->Header.HSize,sizeof(TBFFldHeader));
         goto error;
      }
      
      // Seek to the index
      if( fseek(file->FD,file->Header.IOffset,SEEK_SET) ) {
         App_Log(ERROR,"BinaryFile: Could not seek to index in file %s\n",FileName);
         goto error;
      }

      // Read the index size
      if( fread(&file->Index.N,sizeof(file->Index.N),1,file->FD)!=1 ) {
         App_Log(ERROR,"BinaryFile: Problem reading index size for file %s\n",FileName);
         goto error;
      }

      // Allocate the memory for the index
      if( !(file->Index.Headers=malloc(file->Index.N*sizeof(*file->Index.Headers))) ) {
         App_Log(ERROR,"BinaryFile: Could not allocate memory for index\n");
         goto error;
      }

      // Read the index
      if( fread(file->Index.Headers,sizeof(*file->Index.Headers),file->Index.N,file->FD)!=file->Index.N ) {
         App_Log(ERROR,"BinaryFile: Problem reading index for file %s\n",FileName);
         goto error;
      }

      // Mark the fact that we are not in the right position to write a file
      file->Flags |= BF_SEEKED;
   } else {
      // Write an invalid header as a place holder
      if( fwrite(&file->Header,sizeof(file->Header),1,file->FD)!=1 ) {
         App_Log(ERROR,"BinaryFile: Problem writing header for file %s\n",FileName);
         goto error;
      }

      // Mark the fact that the file header will need to be written at closing time
      file->Flags |= BF_DIRTY;
   }

   return file;
error:
   BinaryFile_Free(file);
   return NULL;
}

int BinaryFile_Close(TBFFile *File) {
   int code=APP_ERR;

   if( !File )
      return code;

   // Only write something if something changed
   if( File->Flags&BF_DIRTY ) {
      // If we are at another place than the end of the file
      if( File->Flags&BF_SEEKED ) {
         // Seek to the place where we need to write the FileHeader
         if( fseek(File->FD,File->Header.IOffset,SEEK_SET) ) {
            App_Log(ERROR,"BinaryFile: Could not seek to file index. The file will be corrupt\n");
            goto end;
         }
      }

      // Write the index
      if( fwrite(&File->Index.N,sizeof(File->Index.N),1,File->FD)!=1 ) {
         App_Log(ERROR,"BinaryFile: Problem writing file index. The file will be corrupt\n");
         goto end;
      }
      if( fwrite(File->Index.Headers,sizeof(*File->Index.Headers),File->Index.N,File->FD)!=File->Index.N ) {
         App_Log(ERROR,"BinaryFile: Problem writing file index. The file will be corrupt\n");
         goto end;
      }

      // Update the total file size
      File->Header.Size = File->Header.IOffset+sizeof(File->Index.N)+sizeof(*File->Index.Headers)*File->Index.N;

      // Seek at the start of the file
      if( fseek(File->FD,0,SEEK_SET) ) {
         App_Log(ERROR,"BinaryFile: Could not seek to file header. The file will be corrupt\n");
         goto end;
      }

      // Update the file header
      if( fwrite(&File->Header,sizeof(File->Header),1,File->FD)!=1 ) {
         App_Log(ERROR,"BinaryFile: Problem writing file header. The file will be corrupt\n");
         goto end;
      }
   }

   // Close the file descriptor
   if( fclose(File->FD) ) {
      File->FD = NULL;
      App_Log(ERROR,"BinaryFile: Problem closing file. The file might be corrupt\n");
      goto end;
   }
   File->FD = NULL;

   code = APP_OK;
end:
   BinaryFile_Free(File);
   return code;
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
      case 7:  return BF_STRING;
      // Unsupported
      default: return BF_NOTYPE;
   }
}

int BinaryFile_Write(void *Data,TBFType DataType,TBFFile *File,int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,const char *Etiket,const char *GrTyp,int IG1,int IG2,int IG3,int IG4) {
   int size = NI*NJ*NK;
   void *buf;
   TBFFldHeader *restrict h;

   // Make sure the file is open for writing
   if( !File || !(File->Flags&BF_WRITE) ) {
      App_Log(ERROR,"BinaryFile: Invalid file given or file is not openned for writing\n");
      return APP_ERR;
   }

   // Make sure we are at the "end" of the file
   if( File->Flags&BF_SEEKED ) {
      // Seek to the place where we need to write the Index
      if( fseek(File->FD,File->Header.IOffset,SEEK_SET) ) {
         App_Log(ERROR,"BinaryFile: Could not seek to the end of the file, aborting.\n");
         return APP_ERR;
      }
      // Reset the flag
      File->Flags &= ~BF_SEEKED;
   }

   // Write the field
   if( fwrite(Data,BFTypeSize[DataType],size,File->FD)!=size ) {
      App_Log(ERROR,"BinaryFile: Problem writing field. The file will be corrupt\n");
      return APP_ERR;
   }

   // Make space in the index
   ++File->Index.N;
   if( !(buf=realloc(File->Index.Headers,File->Index.N*sizeof(*File->Index.Headers))) ) {
      --File->Index.N;
      App_Log(ERROR,"BinaryFile: Could not allocate memory for index, field will be ignored.\n");
      return APP_ERR;
   }
   File->Index.Headers = buf;

   h = &File->Index.Headers[File->Index.N-1];

   // Fill the header
   h->KEY   = File->Header.IOffset;
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

   memcpy(h->TYPVAR,TypVar,MAX(2,strlen(TypVar)));
   memcpy(h->NOMVAR,NomVar,MAX(4,strlen(NomVar)));
   memcpy(h->ETIKET,Etiket,MAX(12,strlen(Etiket)));
   h->GRTYP = GrTyp[0];

   // Update the offset
   File->Header.IOffset += size*BFTypeSize[DataType];

   // Flag the update to the index
   File->Flags |= BF_DIRTY;

   return APP_OK;
}

int BinaryFile_WriteFSTD(void *Data,int NPak,TBFFile *File,int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,const char *Etiket,const char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over) {
   TBFType type=0;
    
   // Check the data type
   if( (type=BinaryFile_Type(DaTyp,-1)) == BF_NOTYPE ) {
      App_Log(ERROR,"BinaryFile: Could not write field : %d is an unsupported data type (DATYP).\n",DaTyp);
      return APP_ERR;
   }

   return BinaryFile_Write(Data,type,File,DateO,Deet,NPas,NI,NJ,NK,IP1,IP2,IP3,TypVar,NomVar,Etiket,GrTyp,IG1,IG2,IG3,IG4);
}

int32_t BinaryFile_Find(TBFFile *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar) {
   TBFFldHeader *restrict h;
   int32_t i;

   if( !File )
      return -1;

   // Look for the field in the index
   for(i=0,h=File->Index.Headers; i<File->Index.N; ++i,++h) {
      if( (DateO==-1 || DateO==h->DATEO)
            && (IP1==-1 || IP1==h->IP1)
            && (IP2==-1 || IP2==h->IP2)
            && (IP3==-1 || IP3==h->IP3)
            && (!NomVar || NomVar[0]=='\0' || !strcmp(NomVar,h->NOMVAR))
            && (!TypVar || TypVar[0]=='\0' || !strcmp(TypVar,h->TYPVAR))
            && (!Etiket || Etiket[0]=='\0' || !strcmp(Etiket,h->ETIKET)) ) {
         // We found the field, return its key
         return i;
      }
   }

   return -1;
}

int32_t BinaryFile_ReadIndex(void *Buf,int32_t Key,TBFFile *File) {
   TBFFldHeader *restrict h;

   // Make sure we have a valid file and key
   if( !File || !(File->Flags&BF_READ) || Key<0 || Key>=File->Index.N ) {
      return -1;
   }

   h = &File->Index.Headers[Key];

   // Seek to the field's position in the file
   if( fseek(File->FD,h->KEY,SEEK_SET) ) {
      App_Log(ERROR,"BinaryFile: Could not seek to field position\n");
      return -1;
   }

   // Read the bytes
   // TODO the size of each item may vary based on the packing/compression/etc.
   size_t n = h->NI*h->NJ*h->NK;
   if( fread(Buf,BFTypeSize[h->DATYP],n,File->FD)!=n ) {
      App_Log(ERROR,"BinaryFile: Could not seek to field position\n");
      return -1;
   }

   return Key;
}

int32_t BinaryFile_Read(void *Buf,TBFFile *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar) {
   int32_t key;
   
   if( (key=BinaryFile_Find(File,NI,NJ,NK,DateO,Etiket,IP1,IP2,IP3,TypVar,NomVar))>0 ) {
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

int32_t BinaryFile_ReadIndexInto(void *Buf,int32_t Key,TBFFile *File,TBFType DestType) {
   TBFFldHeader *restrict h;
   void *tbuf=NULL;

   // Make sure we have a valid file and key
   if( !File || !(File->Flags&BF_READ) || Key<0 || Key>=File->Index.N ) {
      return -1;
   }

   h = &File->Index.Headers[Key];

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

int32_t BinaryFile_ReadInto(void *Buf,TBFFile *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,TBFType DestType) {
   int32_t key;
   
   if( (key=BinaryFile_Find(File,NI,NJ,NK,DateO,Etiket,IP1,IP2,IP3,TypVar,NomVar))>0 ) {
      return BinaryFile_ReadIndexInto(Buf,key,File,DestType);
   }
   return key;
}
