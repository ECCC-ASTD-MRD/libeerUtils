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

// Comment to use FILE IO
#define FPFC_USE_MEM_IO

typedef unsigned char TBufByte;

#ifdef FPFC_USE_MEM_IO
#define FPFC_IO_PARAM TBufByte *restrict CData,unsigned long CBufSize
#else //FPFC_USE_MEM_IO
#define FPFC_IO_PARAM FILE *restrict FD
#endif //FPFC_USE_MEM_IO

// Double functions
int FPFC_Compressl(double *restrict Data,unsigned long N,FPFC_IO_PARAM,unsigned long *CSize);
int FPFC_Inflatel(double *restrict Data,unsigned long N,FPFC_IO_PARAM);

// Float functions
int FPFC_Compress(float *restrict Data,unsigned long N,FPFC_IO_PARAM,unsigned long *CSize);
int FPFC_Inflate(float *restrict Data,unsigned long N,FPFC_IO_PARAM);

#endif // _FPFC_H
