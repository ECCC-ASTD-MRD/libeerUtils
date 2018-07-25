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

#define GPCE_PT_IN_ENV(ENV,P) ((ENV).min.x<=(P).x && (P).x<=(ENV).max.x && (ENV).min.y<=(P).y && (P).y<=(ENV).max.y)

typedef struct gpce_envelope {
    gpc_vertex min;
    gpc_vertex max;
} gpce_envelope;

// Ring fct
double gpce_get_ring_area(const gpc_vertex_list *restrict const Ring);
int gpce_point_inside_ring(const gpc_vertex_list *restrict const Ring,gpc_vertex P);

// Poly fct
double gpce_get_area(gpc_polygon *restrict const Poly);
int gpce_contains_point(gpc_polygon *restrict const Poly,gpce_envelope *restrict const Env,gpc_vertex P);

// General geometry fct
int gpce_intersect_line_line(gpc_vertex A0,gpc_vertex A1,gpc_vertex B0,gpc_vertex B1,double *restrict FA,double *restrict FB);

// Envelope related
gpce_envelope* gpce_get_envelope(gpc_polygon *restrict const Poly);
int gpce_intersects_line_envelope(gpce_envelope *restrict const PEnv,gpc_vertex L0,gpc_vertex L1,const int Dir);


#endif //_gpc_ext_h
