/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de compression de nombre flottants
 * Fichier   : FPFC.h
 * Creation  : Octobre 2017
 * Auteur    : Eric Legault-Ouellet
 *
 * Description: Floating Point Fast Compress : Compress and inflate floating point
 *              numbers using a faster technique
 *
 * Note:
        This is an implementation of the paper
 *      "Fast lossless compression of scientific floating-point data" written
 *      by P. Ratanaworabhan J. Ke and M. Burtscher, published in
 *      Data Compression Conference 2006 pp. 133-142.
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
#ifndef _FPFC_H
#define _FPFC_H

#include <stdio.h>

typedef unsigned char TBufByte;

typedef struct TFPFCBuf TFPFCBuf;
typedef struct TFPFCBuf {
    TBufByte        *Buf;   // Buffer where to read/write the half-bytes
    FILE            *FD;    // File descriptor
    unsigned long   NB;     // Number of bytes written to the buffer
    unsigned long   Size;   // Total size of the buffer (in half-bytes)

    // Buffer functions
    void    (*WriteByte)    (TFPFCBuf*);
    void    (*ReadByte)     (TFPFCBuf*);

    TBufByte    Byte;   // Half byte temp storage
    TBufByte    Half;   // Flag indicating if there is a half byte in the storage
} TFPFCBuf;

#define FPFC_BufFree(Buf) if(Buf){free(Buf);Buf=NULL;}
TFPFCBuf* FPFC_BufNewMem(void* Data,unsigned long N);
TFPFCBuf* FPFC_BufNewIO(FILE* FD);

// Double functions
int FPFC_Compressl(double *restrict Data,unsigned long N,TFPFCBuf *restrict Buf,unsigned long *CSize);
int FPFC_Inflatel(double *restrict Data,unsigned long N,TFPFCBuf *restrict Buf);

// Float functions
int FPFC_Compress(float *restrict Data,unsigned long N,TFPFCBuf *restrict Buf,unsigned long *CSize);
int FPFC_Inflate(float *restrict Data,unsigned long N,TFPFCBuf *restrict Buf);

#endif // _FPFC_H
