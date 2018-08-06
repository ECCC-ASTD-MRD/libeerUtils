#ifndef _gpc_ext_h
#define _gpc_ext_h

#include <math.h>
#include <float.h>
#include "gpc.h"

#define GPC_NULL_VERTEX         (gpc_vertex){NAN,NAN}
#define GPC_NULL_VERTEX_LIST    (gpc_vertex_list){NULL,0}
#define GPC_NULL_POLY           (gpc_polygon){NULL,NULL,0}
#define GPC_NULL_TRISTRIP       (gpc_tristrip){NULL,0}
#define GPCE_NULL_ENVELOPE      (gpce_envelope){GPC_NULL_VERTEX,GPC_NULL_VERTEX};

#define GPC_MK_VERTEX(X,Y)                      (gpc_vertex){(X),(Y)}
#define GPCE_MK_ENVELOPE(MinX,MinY,MaxX,MaxY)   (gpce_envelope){{(MinX),(MinY)},{(MaxX),(MaxY)}}
#define GPCE_MK_RECT2(X0,Y0,X1,Y1)              (gpc_polygon){{{{(X0),(Y0)},{(X1),(Y0)},{(X1),(Y1)},{(X0),(Y1)}}},4},{0},1}
#define GPCE_MK_RECT4(X0,Y0,X1,Y1,X2,Y2,X3,Y3)  (gpc_polygon){{{{(X0),(Y0)},{(X1),(Y1)},{(X2),(Y2)},{(X3),(Y3)}},4},{0},1}

#define GPCE_PT_IN_ENV(ENV,P)           ((ENV).min.x<=(P).x && (P).x<=(ENV).max.x && (ENV).min.y<=(P).y && (P).y<=(ENV).max.y)
#define GPCE_ENV_DISJOINT(ENVA,ENVB)    ((ENVA).min.x>(ENVB).max.x || (ENVA).max.x<(ENVB).min.x || (ENVA).min.y>(ENVB).max.y || (ENVA).max.y<(ENVB).min.y)

typedef struct gpce_envelope {
    gpc_vertex min;
    gpc_vertex max;
} gpce_envelope;

// General geometry fct
double gpce_vect_proj_vect(gpc_vertex A0,gpc_vertex A1,gpc_vertex B0,gpc_vertex B1,gpc_vertex *restrict Pt);
int gpce_line_intersects_line(gpc_vertex A0,gpc_vertex A1,gpc_vertex B0,gpc_vertex B1,double *restrict FA,double *restrict FB);

// Ring fct
double gpce_get_ring_area(const gpc_vertex_list *restrict Ring);
int gpce_ring_crosses_ring(const gpc_vertex_list *restrict RingA,const gpce_envelope *restrict EnvA,const gpc_vertex_list *restrict RingB,const gpce_envelope *restrict EnvB);
int gpce_ring_contains_point(const gpc_vertex_list *restrict Ring,const gpce_envelope *restrict REnv,gpc_vertex P);
int gpce_ring_contains_ring(const gpc_vertex_list *restrict OutRing,const gpce_envelope *restrict OEnv,const gpc_vertex_list *restrict InRing,const gpce_envelope *restrict IEnv);

// Poly fct
int gpce_get_num_polygon(const gpc_polygon *restrict Poly);
double gpce_get_area(const gpc_polygon *restrict Poly);
int gpce_polygon_crosses_ring(const gpc_polygon *restrict Poly,const gpce_envelope *restrict PEnv,const gpc_vertex_list *restrict Ring,const gpce_envelope *restrict REnv);
int gpce_polygon_crosses_polygon(const gpc_polygon *restrict PolyA,const gpce_envelope *restrict EnvA,const gpc_polygon *restrict PolyB,const gpce_envelope *restrict EnvB);
int gpce_polygon_contains_point(const gpc_polygon *restrict Poly,const gpce_envelope *restrict Env,gpc_vertex P);
int gpce_polygon_contains_ring(const gpc_polygon *restrict Poly,const gpce_envelope *restrict PEnv,const gpc_vertex_list *restrict Ring,const gpce_envelope *restrict REnv);
int gpce_polygon_contains_polygon(const gpc_polygon *restrict OPoly,const gpce_envelope *restrict OEnv,const gpc_polygon *restrict IPoly,const gpce_envelope *restrict IEnv);
int gpce_split_polygons(const gpc_polygon *restrict Poly,const gpce_envelope *restrict PEnv,gpc_polygon **PList,gpce_envelope ***EList);

// Envelope related
gpce_envelope* gpce_get_envelope(const gpc_polygon *restrict Poly);
int gpce_envelope_intersects_line(const gpce_envelope *restrict PEnv,gpc_vertex L0,gpc_vertex L1,const int Dir);


#endif //_gpc_ext_h
