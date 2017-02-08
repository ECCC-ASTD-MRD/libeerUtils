#ifndef _BINARYFILE_H
#define _BINARYFILE_H

#include <stdio.h>
#include <inttypes.h>

typedef enum TBFFlag {BF_READ=1,BF_WRITE=2,BF_DIRTY=4,BF_SEEKED=8} TBFFlag;
typedef enum TBFType {BF_STRING,BF_BINARY,BF_INT8,BF_INT16,BF_INT32,BF_INT64,BF_UINT8,BF_UINT16,BF_UINT32,BF_UINT64,BF_FLOAT32,BF_FLOAT64,BF_NOTYPE} TBFType;

// Field header
typedef struct TBFFldHeader {
   int64_t  KEY;              // Cle du champs
   int64_t  NBYTES;           // Taille du champ en octets
   int32_t  DATEO;            // Date d'origine du champs
   int32_t  DEET;             // Duree d'un pas de temps
   int32_t  NPAS;             // Pas de temps
   int32_t  NBITS;            // Nombre de bits du champs
   int32_t  DATYP;            // Type de donnees
   int32_t  IP1,IP2,IP3;      // Specificateur du champs
   int32_t  NI,NJ,NK;         // Dimensions
   int32_t  IG1,IG2,IG3,IG4;  // Descripteur de grille
   char     TYPVAR[2];        // Type de variable
   char     NOMVAR[4];        // Nom de la variable
   char     ETIKET[12];       // Etiquette du champs
   char     GRTYP;            // Type de grilles
   char     BUF;              // Buffer to get to 96 bytes
} TBFFldHeader;

// File header
typedef struct TBFFileHeader {
   int64_t  IOffset;    // Index Offset
   int64_t  Size;       // Total file size (bytes)
   int32_t  Magic;      // Magic number
   uint32_t Version;    // Version of the file
   uint32_t FSize;      // File header size
   uint32_t HSize;      // Header size
} TBFFileHeader;

// Index
typedef struct TBFIndex {
   TBFFldHeader   *Headers;   // Field headers
   int32_t        N;          // Number of headers
} TBFIndex;

typedef struct TBFFile {
   FILE           *FD;     // File descriptor
   TBFFileHeader  Header;  // File header
   TBFIndex       Index;   // File index
   int            Flags;   // Mode, dirty flags
} TBFFile;


TBFFile* BinaryFile_Open(const char *FileName,TBFFlag Mode);
int BinaryFile_Close(TBFFile *File);

TBFType BinaryFile_Type(int DaTyp,int NBytes);

int BinaryFile_Write(void *Data,TBFType DataType,TBFFile *File,int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,const char *Etiket,const char *GrTyp,int IG1,int IG2,int IG3,int IG4);
int BinaryFile_WriteFSTD(void *Data,int NPak,TBFFile *File,int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,const char *Etiket,const char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over);

int32_t BinaryFile_Find(TBFFile *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar);

int32_t BinaryFile_ReadIndex(void *Buf,int32_t Key,TBFFile *File);
int32_t BinaryFile_Read(void *Buf,TBFFile *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar);
int32_t BinaryFile_ReadIndexInto(void *Buf,int32_t Key,TBFFile *File,TBFType DestType);
int32_t BinaryFile_ReadInto(void *Buf,TBFFile *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,TBFType DestType);

#endif // _BINARYFILE_H
