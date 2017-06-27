/*==============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Fonctions et definitions relatives aux fichiers standards et rmnlib
 * Fichier      : RPNC.h
 * Creation     : Juin 2015 - J.P. Gauthier
 * Revision     : $Id: RPN.h 1045 2015-06-09 18:58:51Z gauthierjp $
 *
 * Description:
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
#ifndef _RPNC_h
#define _RPNC_h

#include <netcdf.h>
#include "RPN.h"
#include "TimeRef.h"
#include "GeoRef.h"
#include "ZRef.h"

#define RPNC_ALL     -1    // Index value for all available

typedef struct TRPNC_File {
   char *Name;             // Path complet du fichier
   char Mode;              // Mode d'ouverture du fichier (r,w,a)
   int  NRef;              // Nombre de reference

   int  NC_Id;            // Identificateur netCDF du fichier
} TRPNC_File;

TRPNC_File* RPNC_Open(char *Filename,char *Mode);
int         RPNC_Close(TRPNC_File *File);

int RPNC_Find(TRPNC_File *File,char *VarName,int DateV,float Level);
int RPNC_List(TRPNC_File *File);
int RPNC_Read(TRPNC_File *File,int VarId,int T,int K,char *Data);

int RPNC_Write(TRPNC_File *File,TGeoRef *GRef,TZRef *ZRef,TTimeRef *TRef,char *Data,int TimeStep,int Level,char *VarName,nc_type Type,int NPak);
int RPNC_WriteGeoRef(TRPNC_File *File,TGeoRef *GRef);
int RPNC_WriteZRef(TRPNC_File *File,TZRef *ZRef);
int RPNC_WriteTimeRef(TRPNC_File *File,TTimeRef *TRef);
int RPNC_WriteContent(TRPNC_File *File,char *Title,char *Institution,char *Source,char *History,char *Reference,char *Comment,char *Convention);

int RPNC_PutFSTHead(TRPNC_File *File,int VarId,TRPNHeader *Head);

#endif

