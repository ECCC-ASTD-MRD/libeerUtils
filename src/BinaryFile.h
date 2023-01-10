/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie d'écriture/lecture de fichier binaire à la FSTD
 * Fichier   : BinaryFile.h
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
#ifndef _BINARYFILE_H
#define _BINARYFILE_H

#include <stdio.h>
#include <inttypes.h>
#include <limits.h>

typedef enum TBFFlag {BF_READ=1,BF_WRITE=2,BF_CLEAR=4,BF_DIRTY=8,BF_SEEKED=16} TBFFlag;
typedef enum TBFType {BF_STRING,BF_BINARY,BF_INT8,BF_INT16,BF_INT32,BF_INT64,BF_UINT8,BF_UINT16,BF_UINT32,BF_UINT64,BF_FLOAT32,BF_FLOAT64,BF_CFLOAT32,BF_CFLOAT64,BF_NOTYPE} TBFType;

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
   void           *Addr;   // Address of the memory mapping as returned by mmap
   TBFFileHeader  Header;  // File header
   TBFIndex       Index;   // File index
   int            FD;      // File descriptor
   int            Flags;   // Mode, dirty flags
} TBFFile;

typedef struct TBFFiles {
   TBFFile     *Files;  // Files linked
   int         N;       // Number of files linked
   int         Flags;   // Mode of the files (all files must be opened for the same purpose
} TBFFiles;

typedef int64_t TBFKey;

// Function declaration

TBFFiles* BinaryFile_Open(const char *FileName,TBFFlag Mode);
TBFFiles* BinaryFile_Link(const char** FileNames,int N);
TBFFiles* BinaryFile_LinkPattern(const char* Pattern);
int BinaryFile_Close(TBFFiles *File);

TBFType BinaryFile_Type(int DaTyp,int NBytes);

int BinaryFile_Write(void *Data,TBFType DataType,TBFFiles *File,int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,const char *Etiket,const char *GrTyp,int IG1,int IG2,int IG3,int IG4);
int BinaryFile_WriteFSTD(void *Data,int NPak,TBFFiles *File,int DateO,int Deet,int NPas,int NI,int NJ,int NK,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,const char *Etiket,const char *GrTyp,int IG1,int IG2,int IG3,int IG4,int DaTyp,int Over);

TBFKey BinaryFile_Find(TBFFiles *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar);

TBFKey BinaryFile_ReadIndex(void *Buf,TBFKey Key,TBFFiles *File);
TBFKey BinaryFile_Read(void *Buf,TBFFiles *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar);
TBFKey BinaryFile_ReadIndexInto(void *Buf,TBFKey Key,TBFFiles *File,TBFType DestType);
TBFKey BinaryFile_ReadInto(void *Buf,TBFFiles *File,int *NI,int *NJ,int *NK,int DateO,const char *Etiket,int IP1,int IP2,int IP3,const char* TypVar,const char *NomVar,TBFType DestType);

#endif // _BINARYFILE_H
