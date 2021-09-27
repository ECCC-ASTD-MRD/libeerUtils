/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de fonctions utiles
 * Fichier   : ogr_stub.h
 * Creation  : Septembre 2008 - J.P. Gauthier
 * Revision  : $Id$
 *
 * Description: Fonctions stub pour GDAL/OGR
 *
 * Remarques :
 *
 * License      :
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
#ifndef _ogr_stub_h
#define _ogr_stub_h

#define OGRGeometryH                 char
#define OGRwkbGeometryType           char
#define OGRCoordinateTransformationH char*
#define OGRSpatialReferenceH         char*
#define OGRLayerH                    char*
#define OGRFeatureH                  char*
#define OGRFeatureDefnH              char*
#define OGRDataSourceH               char*
#define OGRSFDriverH                 char*
#define GDALDatasetH                 char*
#define OGRFieldDefnH                char*
#define wkbPolygon                   0
#define wkbLinearRing                0
#define wkbPoint                     0
#define ODrCCreateDataSource         char*

#define GCI_YCbCr_YBand              0
#define GCI_CyanBand                 0
#define GCI_HueBand                  0
#define GCI_RedBand                  0
#define OGRERR_NONE                  0

#define GDALDriverH                  char*
#define GDALColorInterp              char
#define GDALColorEntry               char
#define GDALRasterBandH              char*
#define GDALRPCInfo                  char
#define GA_Update                    0
#define GA_ReadOnly                  0
#define GDAL_GCP                     char
#define GDT_Unknown                  0
#define GDAL_OF_VECTOR               0
#define GDAL_OF_UPDATE               0
#define GDAL_OF_READONLY             0

typedef struct OGREnvelope {
    double      MinX;
    double      MaxX;
    double      MinY;
    double      MaxY;
} OGREnvelope;

#endif