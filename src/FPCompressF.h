/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de compression de nombre flotants
 * Fichier   : FPCompressF.h
 * Creation  : Mars 2017
 * Auteur    : Eric Legault-Ouellet
 *
 * Description: Compress and inflate floating point numbers
 *
 * Note:
        This is an implementation of the paper
 *      "Fast and Efficient Compression of Floating-Point Data" written
 *      by Peter Lindstrom and Martin Isenburg, published in
 *      IEEE Transactions on Visualization and Computer Graphics 12(5):1245-50,
 *      September 2006
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
#ifndef _FPCOMPRESSF_H
#define _FPCOMPRESSF_H

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

int FPC_InflateF(void *restrict CData,int FD,float *restrict Data,int NI,int NJ,int NK);
int FPC_CompressF(void *restrict CData,int FD,float *restrict Data,int NI,int NJ,int NK,size_t *restrict CSize);

#endif //_FPCOMPRESSF_H
