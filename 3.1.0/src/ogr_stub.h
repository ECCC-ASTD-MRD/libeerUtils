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
#define OGRCoordinateTransformationH char*
#define OGRSpatialReferenceH         char*
#define OGRLayerH                    char*
#define OGRFeatureH                  char*
#define OGRFeatureDefnH              char*
#define OGRDataSourceH               char*
#define OGRSFDriverH                 char*

typedef struct OGREnvelope {
    double      MinX;
    double      MaxX;
    double      MinY;
    double      MaxY;
} OGREnvelope;

#endif