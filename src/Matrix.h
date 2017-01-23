/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2100 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet       : Projection diverses de la carte vectorielle.
 * Fichier      : Matrix.h
 * Creation     : Mars 2011 - J.P. Gauthier
 *
 * Description: Fichier de definition des parametres et fonctions de vecteurs.
 *
 * Remarques :
 *
 * Modification:
 *
 *   Nom         :
 *   Date        :
 *   Description :
 *
 *==============================================================================
 */

#ifndef _Matrix_h
#define _Matrix_h

#include "Vector.h"

#define Matrix_Assign(M1,M2)      memcpy(M1,M2,16*sizeof(double));
#define Matrix_Print(M,Msg)       fprintf(stderr,"%s\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",Msg,\
                                  M[0][0],M[0][1],M[0][2],M[0][3],\
                                  M[1][0],M[1][1],M[1][2],M[1][3],\
                                  M[2][0],M[2][1],M[2][2],M[2][3],\
                                  M[3][0],M[3][1],M[3][2],M[3][3]);

#define Matrix_Identity(M)        M[0][0]=1.0;M[0][1]=0.0;M[0][2]=0.0;M[0][3]=0.0;\
                                  M[1][0]=0.0;M[1][1]=1.0;M[1][2]=0.0;M[1][3]=0.0;\
                                  M[2][0]=0.0;M[2][1]=0.0;M[2][2]=1.0;M[2][3]=0.0;\
                                  M[3][0]=0.0;M[3][1]=0.0;M[3][2]=0.0;M[3][3]=1.0;

#define Matrix_Translate(M,X,Y,Z) M[0][0]=1.0;M[0][1]=0.0;M[0][2]=0.0;M[0][3]=0.0;\
                                  M[1][0]=0.0;M[1][1]=1.0;M[1][2]=0.0;M[1][3]=0.0;\
                                  M[2][0]=0.0;M[2][1]=0.0;M[2][2]=1.0;M[2][3]=0.0;\
                                  M[3][0]=X  ;M[3][1]=Y  ;M[3][2]=Z  ;M[3][3]=1.0;

#define Matrix_Scale(M,X,Y,Z)     M[0][0]=X;  M[0][1]=0.0;M[0][2]=0.0;M[0][3]=0.0;\
                                  M[1][0]=0.0;M[1][1]=Y;  M[1][2]=0.0;M[1][3]=0.0;\
                                  M[2][0]=0.0;M[2][1]=0.0;M[2][2]=Z;  M[2][3]=0.0;\
                                  M[3][0]=0.0;M[3][1]=0.0;M[3][2]=0.0;M[3][3]=1.0;

#define Matrix_Rotate(M,X,Y,Z)    M[0][0]=cos(Y)*cos(Z);M[0][1]=-cos(X)*sin(Z)+sin(X)*sin(Y)*cos(Z);M[0][2]=sin(X)*sin(Z)+cos(X)*sin(Y)*cos(Z);M[0][3]=0.0;\
                                  M[1][0]=cos(Y)*sin(Z);M[1][1]=cos(X)*cos(Z)+sin(X)*sin(Y)*sin(Z);M[1][2]=-sin(X)*cos(Z)+cos(X)*sin(Y)*sin(Z);M[1][3]=0.0;\
                                  M[2][0]=-sin(Y);M[2][1]=sin(X)*cos(Y);M[2][2]=cos(X)*cos(Y);M[2][3]=0.0;\
                                  M[3][0]=0.0;M[3][1]=0.0;M[3][2]=0.0;M[3][3]=1.0;

#define Matrix_RotateX(M,X)       M[0][0]=1.0;M[0][1]=0.0;M[0][2]=0.0;M[0][3]=0.0;\
                                  M[1][0]=0.0;M[1][1]=cos(X);M[1][2]=sin(X);M[1][3]=0.0;\
                                  M[2][0]=0.0;M[2][1]=-sin(X);M[2][2]=cos(X);M[2][3]=0.0;\
                                  M[3][0]=0.0;M[3][1]=0.0;M[3][2]=0.0;M[3][3]=1.0;

#define Matrix_RotateY(M,Y)       M[0][0]=cos(Y);M[0][1]=0.0;M[0][2]=-sin(Y);M[0][3]=0.0;\
                                  M[1][0]=0.0;M[1][1]=1.0;M[1][2]=0.0;M[1][3]=0.0;\
                                  M[2][0]=sin(Y);M[2][1]=0.0;M[2][2]=cos(Y);M[2][3]=0.0;\
                                  M[3][0]=0.0;M[3][1]=0.0;M[3][2]=0.0;M[3][3]=1.0;

#define Matrix_RotateZ(M,Z)       M[0][0]=cos(Z);M[0][1]=sin(Z);M[0][2]=0.0;M[0][3]=0.0;\
                                  M[1][0]=-sin(Z);M[1][1]=cos(Z);M[1][2]=0.0;M[1][3]=0.0;\
                                  M[2][0]=0.0;M[2][1]=0.0;M[2][2]=1.0;M[2][3]=0.0;\
                                  M[3][0]=0.0;M[3][1]=0.0;M[3][2]=0.0;M[3][3]=1.0;

typedef double Matrix4d[4][4];

void Matrix_MxM(Matrix4d MR,Matrix4d M1,Matrix4d M2);
void Matrix_VxM(Vect3d VR,Vect3d V,Matrix4d M);
double Matrix_Determinant(Matrix4d M);

#endif
