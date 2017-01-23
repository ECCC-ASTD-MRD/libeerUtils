/*=============================================================================
 * Environnement Canada
 * Centre Meteorologique Canadian
 * 2121 Trans-Canadienne
 * Dorval, Quebec
 *
 * Projet    : Librairie de modélisation
 * Fichier   : Traj.h
 * Creation  : Septembre 2008 - J.P. Gauthier
 *
 * Description: Module de calcul de trajectoires.
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
#ifndef _Traj_h
#define _Traj_h

typedef struct TParticle {
   time_t Date;
   TCoord Co;
   float  ZPres,ZModel,ZMSL,Dist,Speed;
} TParticle;

typedef struct TTraj {
#ifdef _TK_SOURCE
   Tcl_Obj         *Tag;                 // (Tcl_Obj type)   Tcl identifier
   TDataSpec       *Spec;                // (TDataSpec type) Specification de rendue des donnees
#else
   char            *Tag;                 // (Tcl_Obj type)   Tcl identifier
   char            *Spec;                // (TDataSpec type) Specification de rendue des donnees
#endif

   float      Min,Max;
   int        Lapse;
   time_t     Date;
   float      Height;
   int        AP;
   int        Back;
   int        NPr;
   char       Path[1024];
   char      *Id;
   char       Model[16];
   int        Mode;
   char       Type;
   char       OutDomain;
   TParticle *Pr;
} TTraj;

#endif