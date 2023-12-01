#include <stdlib.h>
#include <string.h>
#include "gpc_ext.h"

/*----------------------------------------------------------------------------
 * Nom      : <gpce_vect_proj_vect>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Retourne la projection du vecteur B0->B1 sur le vecteur A0->A1
 *
 * Parametres :
 *  <A0>    : Premier point du vecteur A
 *  <A1>    : Deuxième point du vecteur A
 *  <B0>    : Premier point du vecteur B
 *  <B1>    : Deuxième point du vecteur B
 *  <Pt>    : [OUT|Opt] Le point sur le vecteur A correspondant à la projection
 *            du vecteur B
 *
 * Retour: La projection du vecteur B sur le vecteur A (scalaire)
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
double gpce_vect_proj_vect(gpc_vertex A0,gpc_vertex A1,gpc_vertex B0,gpc_vertex B1,gpc_vertex *restrict Pt) {
    double ax,ay,f;

    ax = A1.x-A0.x;
    ay = A1.y-A0.y;

    f = (ax*(B1.x-B0.x) + ay*(B1.y-B0.y)) / (ax*ax + ay*ay);

    if( Pt ) {
        Pt->x = A0.x + f*ax;
        Pt->x = A0.y + f*ay;
    }

    return f;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_line_intersects_line>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Trouver s'il y a intersection entre deux lignes et retourner les
 *            facteurs le cas échéant
 *
 * Parametres :
 *  <A0>    : Premier point de la ligne A
 *  <A1>    : Deuxième point de la ligne A
 *  <B0>    : Premier point de la ligne B
 *  <B1>    : Deuxième point de la ligne B
 *  <FA>    : [OUT|Opt] Facteur de l'intersection à partir du point A0 sur la ligne A0-A1
 *  <FB>    : [OUT|Opt] Facteur de l'intersection à partir du point B0 sur la ligne B0-B1
 *
 * Retour:
 *  - 0 : aucune intersection (FA et FB inchangés)
 *  - 1 : intersection simple (FA et FB représente les facteurs du point d'intersection
 *        tel que A0+FA(A1-A0) = B0+FB(B1-B0)
 *  - 2 : Les lignes sont colinéaires (facteur A tel que A0+FA(A1-A0)=closest(B0,B1) et
 *        facteur B tel que B0+B(B1-B0)=closest(A0,A1)
 *  - 3 : B est un point situé sur la ligne A (FA tel que A0+FA(A1-A0)=B0=B1, FB=0)
 *  - 4 : A est un point situé sur la ligne B (FB tel que B0+FB(B1-B0)=A0=A1, FA=0)
 *  - 5 : A et B sont des points identiques (FA=0, FB=0)
 *
 * Remarques :
 *  - Les facteurs sont toujours calculés selon le premier point de la ligne.
 *    Ainsi, intersection = A0+FA(A1-A0) = B0+FB(B1-B0)
 *  - Si le facteur A ou B est entre 0 et 1, alors l'intersection a lieu entre les
 *    points A0,A1 et B0,B1, respectivement (à l'intérieur des segments).
 *----------------------------------------------------------------------------
 */
int gpce_line_intersects_line(gpc_vertex A0,gpc_vertex A1,gpc_vertex B0,gpc_vertex B1,double *restrict FA,double *restrict FB) {
    double c,adx,ady,bdx,bdy,dx,dy;

    adx = A1.x - A0.x;
    ady = A1.y - A0.y;
    bdx = B1.x - B0.x;
    bdy = B1.y - B0.y;

    dx = A0.x - B0.x;
    dy = B0.y - A0.y;

    // Note that if c==0.0, the slopes are the same and the lines are parallel, which means that we either have
    // no intersection or an infinity of it (if both lines overlap).
    c = bdx*ady - bdy*adx;
    if( c != 0.0 ) {
        c = 1.0/c;
        // The lines intersects somewhere, return the factors from the start point.
        // A value between 0 and 1 means that the intersection is between the points
        if( FA ) *FA = (dx*bdy + dy*bdx) * c;
        if( FB ) *FB = (dx*ady + dy*adx) * c;
        return 1;
    } else {
        switch( (adx==0.0&&ady==0.0)<<1 | (bdx==0.0&&bdy==0.0) ) {
            case 0:
                // A and B are both lines, check if A0 lies on line B
                if( dx*bdy + dy*bdx == 0.0 ) {
                    if( FA ) *FA = ady!=0.0 ? fmin(dy,B1.y-A0.y)/ady : fmin(-dx,B1.x-A0.x)/adx;
                    if( FB ) *FB = bdx!=0.0 ? fmin(dx,A1.x-B0.x)/bdx : fmin(-dy,A1.y-B0.y)/bdy;
                    return 2;
                }
                break;
            case 1:
                // B is a point, A is a line, check if B lies on A
                if( dx*ady + dy*adx == 0.0 ) {
                    if( FA ) *FA = ady!=0.0 ? dy/ady : -dx/adx;
                    if( FB ) *FB = 0.0;
                    return 3;
                }
                break;
            case 2:
                // A is a point, B is a line, check if A lies on B
                if( dx*bdy + dy*bdx == 0.0 ) {
                    if( FA ) *FA = 0.0;
                    if( FB ) *FB = bdx!=0.0 ? dx/bdx : -dy/bdy;
                    return 4;
                }
                break;
            case 3:
                // A and B are both points, check if they are one and the same
                if( A0.x==B0.x && A0.y==B0.y) {
                    if( FA ) *FA = 0.0;
                    if( FB ) *FB = 0.0;
                    return 5;
                }
                break;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_segment_intersects_segment>
 * Creation : Août 2022 - E. Legault-Ouellet
 *
 * But      : Trouver s'il y a intersection entre deux segment
 *
 * Parametres :
 *  <A0>    : Premier point du segment A
 *  <A1>    : Deuxième point du segment A
 *  <B0>    : Premier point du segment B
 *  <B1>    : Deuxième point du segment B
 *  <FA>    : [OUT|Opt] Facteur de l'intersection à partir du point A0 sur la ligne A0-A1
 *  <FB>    : [OUT|Opt] Facteur de l'intersection à partir du point B0 sur la ligne B0-B1
 *
 * Retour: 0 si aucune intersection, la même valeur que gpce_line_intersects_line sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int gpce_segment_intersects_segment(gpc_vertex A0,gpc_vertex A1,gpc_vertex B0,gpc_vertex B1,double *restrict FA,double *restrict FB) {
   int code;
   double fa,fb;

   if( !GPCE_SEG_DISJOINT(A0,A1,B0,B1) && (code=gpce_line_intersects_line(A0,A1,B0,B1,&fa,&fb)) && 0.0<=fa && fa<=1.0 && 0.0<=fb && fb<=1.0 ) {
      if( FA ) *FA = fa;
      if( FB ) *FB = fb;
      return code;
   }

   return 0;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_get_num_polygon>
 * Creation : Août 2022 - E. Legault-Ouellet
 *
 * But      : Retourne le nombre de point total d'un polygone
 *
 * Parametres :
 *  <Poly>  : Le polygone
 *
 * Retour: Le nombre de point total du polygone
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int gpce_get_num_vertices(const gpc_polygon *restrict Poly) {
    int r,n;

    for(r=0,n=0; r<Poly->num_contours; ++r) {
       n += Poly->contour[r].num_vertices;
    }

    return n;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_get_num_polygon>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Retourne si le polygone est un multi polygone
 *
 * Parametres :
 *  <Poly>  : Le polygone
 *
 * Retour: 1 si Poly a plus d'un contour extérieur, 0 sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int gpce_get_num_polygon(const gpc_polygon *restrict Poly) {
    int r,n;

    for(r=0,n=0; r<Poly->num_contours; ++r) {
        if( !Poly->hole[r] ) {
            ++n;
        }
    }

    return n;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_get_ring_area>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Retourne l'aire d'un contour
 *
 * Parametres :
 *  <Ring>  : Le contur
 *
 * Retour: L'aire du contour
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
double gpce_get_ring_area(const gpc_vertex_list *restrict Ring) {
    int i,j;
    double area;

    for(i=Ring->num_vertices-1,j=0,area=0.0; j<Ring->num_vertices; i=j++) {
        area += (Ring->vertex[i].x*Ring->vertex[j].y) - (Ring->vertex[j].x*Ring->vertex[i].y);
    }

    return fabs(area*0.5);
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_get_area>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Retourne l'aire d'un polygone
 *
 * Parametres :
 *  <Poly>  : Le polygone
 *
 * Retour: L'aire du polygone
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
double gpce_get_area(const gpc_polygon *restrict Poly) {
    double area;
    int r;

    // Loop on the rings
    for(r=0,area=0.0; r<Poly->num_contours; ++r) {
        // Substract holes and add areas that count
        if( Poly->hole[r] ) {
            area -= gpce_get_ring_area(&Poly->contour[r]);
        } else {
            area += gpce_get_ring_area(&Poly->contour[r]);
        }
    }

    return area;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_copy_polygon>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Copie un polygone
 *
 * Parametres :
 *  <Dest>  : Le polygone de destination
 *  <Srcy>  : Le polygone source
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
static void gpce_copy_ring(gpc_vertex_list *restrict Dest,const gpc_vertex_list *restrict Src) {
    Dest->vertex = malloc(Src->num_vertices*sizeof(*Dest->vertex));
    memcpy(Dest->vertex,Src->vertex,Src->num_vertices*sizeof(*Dest->vertex));

    Dest->num_vertices = Src->num_vertices;
}
void gpce_copy_polygon(gpc_polygon *restrict Dest,const gpc_polygon *restrict Src) {
    int r;

    // Copy contours
    Dest->contour = malloc(Src->num_contours*sizeof(*Dest->contour));
    for(r=0; r<Src->num_contours; ++r) {
        gpce_copy_ring(Dest->contour+r,Src->contour+r);
    }

    // Copy hole flags
    Dest->hole = malloc(Src->num_contours*sizeof(*Dest->hole));
    memcpy(Dest->hole,Src->hole,Src->num_contours*sizeof(*Dest->hole));

    // Copy the rest
    Dest->num_contours = Src->num_contours;
}


/*----------------------------------------------------------------------------
 * Nom      : <gpce_ring_crosses_ring>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Détermine si un contour en touche/croise un autre
 *
 * Parametres :
 *  <RingA> : Le premier contour
 *  <EnvA>  : [Opt] L'enveloppe du premier contour
 *  <RingB> : Le deuxième contour
 *  <EnvB>  : [Opt] L'enveloppe du deuxième contour
 *
 * Retour: 1 si les deux contours se touchent ou croisent, 0 sinon
 *
 * Remarques : Un contour complètement inclu dans un autre n'est pas considéré
 *             comme croisant ce contour si aucun de leurs segments ne se croisent
 *----------------------------------------------------------------------------
 */
int gpce_ring_crosses_ring(const gpc_vertex_list *restrict RingA,const gpce_envelope *restrict EnvA,const gpc_vertex_list *restrict RingB,const gpce_envelope *restrict EnvB) {
    int a0,a1,b0,b1;

    // Make sure we have points
    if( RingA->num_vertices<=0 || RingB->num_vertices<=0 ) {
        return 0;
    }

    // Check if the envelope are disjoint
    if( EnvA && EnvB && GPCE_ENV_DISJOINT(*EnvA,*EnvB) ) {
        return 0;
    }

    // Check if there is an intersection between each segments
    for(a0=RingA->num_vertices-1,a1=0; a1<RingA->num_vertices; a0=a1++) {
        for(b0=RingB->num_vertices-1,b1=0; b1<RingB->num_vertices; b0=b1++) {
           if( gpce_segment_intersects_segment(RingA->vertex[a0],RingA->vertex[a1],RingB->vertex[b0],RingB->vertex[b1],NULL,NULL) ) {
                return 1;
            }
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_polygon_crosses_ring>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Détermine si un polygone touche/croise un contour
 *
 * Parametres :
 *  <Poly>  : Le polygone
 *  <PEnv>  : [Opt] L'enveloppe du polygone
 *  <Ring>  : Le contour
 *  <REnv>  : [Opt] L'enveloppe du contour
 *
 * Retour: 1 si le polygone touche ou croise le contour, 0 sinon
 *
 * Remarques : Un contour complètement inclu dans un polygone n'est pas considéré
 *             comme croisant ce polygone si aucun de ses segments ne croisent
 *             ceux du polygone
 *----------------------------------------------------------------------------
 */
int gpce_polygon_crosses_ring(const gpc_polygon *restrict Poly,const gpce_envelope *restrict PEnv,const gpc_vertex_list *restrict Ring,const gpce_envelope *restrict REnv) {
    int r;

    // Check if one of the rings of the polygon crosses the ring
    for(r=0; r<Poly->num_contours; ++r) {
        if( gpce_ring_crosses_ring(Poly->contour+r,PEnv?PEnv+r:NULL,Ring,REnv) ) {
            // We cross one of the ring of the polygon, it's settled
            return 1;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_ring_crosses_ring>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Détermine si un polygone en croise un autre
 *
 * Parametres :
 *  <PolyA> : Le premier polygone
 *  <EnvA>  : [Opt] L'enveloppe du premier polygone
 *  <PolyB> : Le deuxième polygone
 *  <EnvB>  : [Opt] L'enveloppe du deuxième polygone
 *
 * Retour: 1 si les deux polygone se touchent ou croisent, 0 sinon
 *
 * Remarques : Un polygone complètement inclu dans un autre n'est pas considéré
 *             comme croisant un autre polygone si aucun de leurs segments ne
 *             se croisent
 *----------------------------------------------------------------------------
 */
int gpce_polygon_crosses_polygon(const gpc_polygon *restrict PolyA,const gpce_envelope *restrict EnvA,const gpc_polygon *restrict PolyB,const gpce_envelope *restrict EnvB) {
    int r;

    // Check if one of the rings of the polygon crosses one of the rings of the other polygon
    for(r=0; r<PolyB->num_contours; ++r) {
        if( gpce_polygon_crosses_ring(PolyA,EnvA,PolyB->contour+r,EnvB?EnvB+r:NULL) ) {
            // We crossed one of the ring of the other polygon, it's settled
            return 1;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_ring_contains_point>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Vérifie si le point est contenu dans le contour
 *
 * Parametres :
 *  <Ring>  : Le contour
 *  <REnv>  : [Opt] L'enveloppe du contour
 *  <P>     : Le point
 *
 * Retour: 1 si contenu, 0 sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int gpce_ring_contains_point(const gpc_vertex_list *restrict Ring,const gpce_envelope *restrict REnv,gpc_vertex P) {
    int i,j,in;

    // Check if we are in the envelope
    if( REnv && !GPCE_PT_IN_ENV(*REnv,P) )
        return 0;

    // Loop on the segment
    for(i=Ring->num_vertices-1,j=0,in=0; j<Ring->num_vertices; i=j++) {
        // Check if the segment intersects with an horizontal line that crosses X,Y
        // Count only intersections that occurs before we reach the X
        if( (Ring->vertex[i].y>P.y)!=(Ring->vertex[j].y>P.y)
                && (P.x < (Ring->vertex[j].x-Ring->vertex[i].x)*(P.y-Ring->vertex[i].y)/(Ring->vertex[j].y-Ring->vertex[i].y) + Ring->vertex[i].x) ) {
            in = !in;
        }
    }

    return in;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_ring_contains_ring>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Vérifie si un contour est contenu dans un autre contour
 *
 * Parametres :
 *  <OutRing>   : Le contour extérieur
 *  <OEnv>      : [Opt] L'enveloppe du contour extérieur
 *  <InRing>    : Le contour intérieur
 *  <IEnv>      : [Opt] L'enveloppe du contour intérieur
 *
 * Retour: 1 si InRing est contenu dans OutRing, 0 sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int gpce_ring_contains_ring(const gpc_vertex_list *restrict OutRing,const gpce_envelope *restrict OEnv,const gpc_vertex_list *restrict InRing,const gpce_envelope *restrict IEnv) {
   int i;

    // Make sure we have points
    if( OutRing->num_vertices<=0 || InRing->num_vertices<=0 ) {
        return 0;
    }

    // Check if the envelope is contained
    if( OEnv && IEnv && (OEnv->min.x>=IEnv->min.x || OEnv->min.y>=IEnv->min.y || OEnv->max.x<=IEnv->max.x || OEnv->max.y<=IEnv->max.y) ) {
        return 0;
    }

    // Make sure no point is outside the ring
    for(i=0; i<InRing->num_vertices; ++i) {
       if( !gpce_ring_contains_point(OutRing,OEnv,InRing->vertex[i]) ) {
          return 0;
       }
    }

    return 1;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_polygon_contains_point>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Vérifie si le point est contenu dans le polygone
 *
 * Parametres :
 *  <Poly>  : Le polygone
 *  <PEnv>  : [Opt] L'enveloppe du polygone
 *  <P>     : Le point
 *
 * Retour: 1 si contenu, 0 sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int gpce_polygon_contains_point(const gpc_polygon *restrict Poly,const gpce_envelope *restrict PEnv,gpc_vertex P) {
    int r,in=0,np;

    // Check whether or not we have a multi-polygon
    np = gpce_get_num_polygon(Poly);

    // Loop on the rings
    for(r=0; r<Poly->num_contours; ++r) {
        // If we have an interior ring (hole)
        if( Poly->hole[r] ) {
            // If we are in the hole, then we are outside of the polygon
            if( gpce_ring_contains_point(Poly->contour+r,PEnv?PEnv+r:NULL,P) ) {
                return 0;
            }
        } else {
            // We have an outer-ring
            if( gpce_ring_contains_point(Poly->contour+r,PEnv?PEnv+r:NULL,P) ) {
                in = 1;
            } else if( np <= 1 ) {
                // Since we made sure to only have one exterior ring, we need to be contained in that external ring if we want to have a chance to be considered in
                return 0;
            }
        }
    }

    return in;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_polygon_contains_ring>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Vérifie si un contour est contenu dans un polygone
 *
 * Parametres :
 *  <Poly>  : Le polygone
 *  <PEnv>  : [Opt] L'enveloppe du polygone
 *  <Ring>  : Le contour
 *  <REnv>  : [Opt] L'enveloppe du contour
 *
 * Retour: 1 si Poly contient Ring, 0 sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int gpce_polygon_contains_ring(const gpc_polygon *restrict Poly,const gpce_envelope *restrict PEnv,const gpc_vertex_list *restrict Ring,const gpce_envelope *restrict REnv) {
    int r,in=0,np,partial=0;

    if( Ring->num_vertices <= 0 )
        return 0;

    // Check whether or not we have a multi-polygon
    np = gpce_get_num_polygon(Poly);

    // Loop on the rings
    for(r=0; r<Poly->num_contours; ++r) {
        // If we have an interior ring (hole)
        if( Poly->hole[r] ) {
            // If the first point is inside the hole or we intersect the hole in any way, then the ring is not fully contained in the polygon
            if( gpce_ring_contains_point(Poly->contour+r,PEnv?PEnv+r:NULL,Ring->vertex[0])
                    || gpce_ring_crosses_ring(Poly->contour+r,PEnv?PEnv+r:NULL,Ring,REnv) ) {
                return 0;
            }
        } else {
            // We have an outer-ring
            if( gpce_ring_contains_ring(Poly->contour+r,PEnv?PEnv+r:NULL,Ring,REnv) ) {
                in = 1;
            } else if( np <= 1 ) {
                // Since we made sure to only have one exterior ring, we need to be fully contained in the external ring, otherwise we're doomed
                return 0;
            } else if( gpce_ring_crosses_ring(Poly->contour+r,PEnv?PEnv+r:NULL,Ring,REnv) ) {
                // Signal that we might have a partial match (if another polygon unioned with this one contains the entire ring)
                ++partial;
            }
        }
    }

    // Extra step : maybe our multi-polygon is joined around that ring?
    if( !in && partial>=2 ) {
        gpc_polygon poly,join,tmp;
        int i,hole=0;

        // Create the polygon that we want to join
        poly.hole = &hole;
        poly.num_contours = 1;

        // Join all external rings together
        for(r=0,i=0; r<Poly->num_contours&&i<np; ++r) {
            if( !Poly->hole[r] ) {
                if( i ) {
                    poly.contour = Poly->contour+r;
                    gpc_polygon_clip(GPC_UNION,&join,&poly,&tmp);
                    if( i>1 )
                        gpc_free_polygon(&join);
                    join = tmp;
                } else {
                    join.num_contours = 1;
                    join.hole = &hole;
                    join.contour = Poly->contour+r;
                }
                ++i;
            }
        }

        // No need to continue if the number of polygons didn't change
        if( gpce_get_num_polygon(&join) != np ) {
            // Loop on the rings (note that it can only be external rings)
            for(r=0; r<join.num_contours; ++r) {
                if( gpce_ring_contains_ring(join.contour+r,NULL,Ring,REnv) ) {
                    in = 1;
                    break;
                }
            }
        }

        gpc_free_polygon(&join);
    }

    return in;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_polygon_contains_polygon>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Vérifie si un contour est contenu dans un polygone
 *
 * Parametres :
 *  <OPoly> : Le polygone exteriéur
 *  <OEnv>  : [Opt] L'enveloppe du polygone extérieur
 *  <IPoly> : Le polygone intérieur
 *  <IEnv>  : [Opt] L'enveloppe du polygone intérieur
 *
 * Retour: 1 si OPoly contient IPoly, 0 sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int gpce_polygon_contains_polygon(const gpc_polygon *restrict OPoly,const gpce_envelope *restrict OEnv,const gpc_polygon *restrict IPoly,const gpce_envelope *restrict IEnv) {
    int r,in=0;

    // IPoly is contained in OPoly if all exterior rings of IPoly are contained in OPoly
    for(r=0; r<IPoly->num_contours; ++r) {
        if( !IPoly->hole[r] ) {
            if( !gpce_polygon_contains_ring(OPoly,OEnv,IPoly->contour+r,IEnv?IEnv+r:NULL) ) {
                return 0;
            }
            in = 1;
        }
    }

    return in;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_split_polygons>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Retourne une liste de polygone simple à partir d'un multi-polygone
 *
 * Parametres :
 *  <Poly>  : Le multi-polygone
 *  <PEnv>  : [Opt] L'enveloppe du multi-polygone
 *  <PList> : [OUT|ToFree] La liste retournée des polygones
 *  <EList> : [OUT|ToFree|Opt] La liste retournée des enveloppes des polygones
 *
 * Retour: Le nombre de polygones dans la liste retournées
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
struct IdxE {
   int idx;
   gpc_vertex_list *ring;
   gpce_envelope *env;
};
int QSort_IdxE(const void *A,const void *B) {
   const struct IdxE *a=A,*b=B;
   double asqA=(a->env->max.x-a->env->min.x)*(a->env->max.y-a->env->min.y);
   double asqB=(b->env->max.x-b->env->min.x)*(b->env->max.y-b->env->min.y);
   if( asqA < asqB )       return -1;
   else if( asqA > asqB )  return 1;
   else                    return 0;
   //return (asqA>asqB)-(asqA<asqB);
}
int gpce_explode_multi_polygon(const gpc_polygon *restrict Poly,const gpce_envelope *restrict PEnv,gpc_polygon **PList,gpce_envelope ***EList) {
    const gpce_envelope *restrict penv;
    gpc_polygon         *plist=NULL;
    gpce_envelope       **elist=NULL;
    int                 r,n,e,c;

    if( (n=gpce_get_num_polygon(Poly)) && (plist=calloc(n,sizeof(*plist))) && (!EList||(elist=malloc(n*sizeof(*elist)))) ) {
        if( n == 1 ) {
            // We only have one polygon, just copy it
            gpce_copy_polygon(plist,Poly);

            // Setup the envelope if requested
            if( elist ) {
                if( PEnv ) {
                    *elist = malloc(Poly->num_contours*sizeof(**elist));
                    memcpy(*elist,PEnv,Poly->num_contours*sizeof(**elist));
                } else {
                    *elist = gpce_get_envelopes(Poly);
                }
            }
        } else {
            struct IdxE idxe[n];

            // Make sure we have an envelope because that has the potential to greatly speedup the process
            penv = PEnv?PEnv:gpce_get_envelopes(Poly);

            // Loop on the exterior rings
            for(r=0,e=0; e<n; ++r) {
                if( !Poly->hole[r] ) {
                    // Allocate as much memory as possibly needed
                    plist[e].contour = calloc(Poly->num_contours-n+1,sizeof(*plist[e].contour));
                    plist[e].num_contours = 1;

                    // Copy the exterior ring
                    gpce_copy_ring(plist[e].contour,Poly->contour+r);

                    // Also copy the envelope if it was requested
                    if( elist ) {
                        elist[e] = malloc((Poly->num_contours-n+1)*sizeof(*elist[e]));
                        elist[e][0] = penv[r];
                    }

                    idxe[e] = (struct IdxE){e,Poly->contour+r,(gpce_envelope*)penv+r};
                    ++e;
                }
            }

            // Internaly sort the outside polygons from the smallest to the biggest (in term of envelope)
            // This is done so that if a polygon has a hole that contains another polygon, the rings are added correctly
            // otherwise a ring that fits on the interior polygon could be added to the exterior one
            qsort(idxe,n,sizeof(*idxe),QSort_IdxE);

            // Loop on the interior ring and attach them to the right polygon
            for(r=0; r<Poly->num_contours; ++r) {
                if( Poly->hole[r] && Poly->contour[r].num_vertices ) {
                   for(e=0; e<n; ++e) {
                      // Since we expect our polygons to already be well behaved, a hole having a point inside an exterior ring
                      // means it belongs to that ring (which is the right ring because the polygons are sorted on envelope size)
                      if( gpce_ring_contains_point(idxe[e].ring,idxe[e].env,Poly->contour[r].vertex[0]) ) {
                         if( elist ) {
                            elist[idxe[e].idx][plist[idxe[e].idx].num_contours] = penv[r];
                         }
                         gpce_copy_ring(plist[idxe[e].idx].contour+plist[idxe[e].idx].num_contours++,Poly->contour+r);
                         break;
                      }
                   }
                   if( e >= n ) {
                      fprintf(stderr,"%s: found a hole without parent (%d)\n",__func__,r);
                   }
                }
            }

            // Finalize the polygons
            for(e=0; e<n; ++e) {
               // Resize the memory to the right size
               plist[e].contour = realloc(plist[e].contour,plist[e].num_contours*sizeof(*plist[e].contour));
               if( elist ) {
                  elist[e] = realloc(elist[e],plist[e].num_contours*sizeof(*elist[e]));
               }

               // Allocate the hole array and set all but the first ring as holes
               plist[e].hole = malloc(plist[e].num_contours*sizeof(*plist[e].hole));
               plist[e].hole[0] = 0;
               for(c=1; c<plist[e].num_contours; ++c) {
                  plist[e].hole[c] = 1;
               }
            }

            // Free the envelope if it was not already provided
            if( penv != PEnv )
                free((gpce_envelope*)penv);
        }
    } else {
        if( plist )
            free(plist);
        if( elist )
            free(elist);
    }

    *PList = plist;
    if( EList )
        *EList = elist;

    return n;
}

/*----------------------------------------------------------------------------
 * Name     : <gpce_poly_split_tile>
 * Creation : August 2022 - E. Legault-Ouellet - CMC/CMOE
 *
 * Purpose  : Recursive function to split (multi)polygons into smaller polygons
 *
 * Args :
 *   <Poly>    : The (multi)polygon to split
 *   <MaxPoints: The maximum number of vertices a polygon can have (it is split otherwise)
 *   <Split>   : [OUT] List of generated polygons
 *   <NbSplit> : [OUT] Number of polygon generated
 *   <PolyIdx> : [OUT/OPT] Optional index indicating which polygon each splitted polygon
 *               came from (only useful if the initial polygon is a multi-polygon)
 *   <Size>    : [Internal, call with NULL] Size of the memory vector used to
 *               store the splitted polygons
 *
 * Return: The number of generated polygons (0 if error)
 *
 * Remarks :
 *----------------------------------------------------------------------------
 */
#define REALLOC(Ptr,Size) { \
   void *tmpptr; \
   if( (tmpptr=realloc((Ptr),(Size)*sizeof(*(Ptr)))) ) { \
      (Ptr) = tmpptr; \
   } else { \
      fprintf(stderr,"%s: Could not allocate memory\n",__func__); \
      goto err; \
   } \
}
int gpce_poly_split_tile(const gpc_polygon *restrict Poly,const int MaxPoints,gpc_polygon **restrict Split,unsigned int *restrict NbSplit,unsigned int **restrict PolyIdx,unsigned int *restrict Size) {
   unsigned int   size=0,toplvl=0,pidx=0;

   // A NULL size indicates that we are the top level fct
   if( !Size ) {
      Size = &size;
      toplvl = 1;
      *Split = NULL;
      *NbSplit = 0;
   }

   // Check if we have a multi polygon
   if( gpce_get_num_polygon(Poly) > 1 ) {
      gpc_polygon    *polys;
      unsigned int   i,n;

      // Split the multi-polygon into single polygons
      n = gpce_explode_multi_polygon(Poly,NULL,&polys,NULL);

      // Process those polygons individually
      for(i=0; i<n; ++i) {
         if( !gpce_poly_split_tile(&polys[i],MaxPoints,Split,NbSplit,PolyIdx,Size) ) {
            // In case of error, free everything and return
            for(; i<n; ++i)
               gpc_free_polygon(&polys[i]);
            free(polys);
            goto err;
         }
         gpc_free_polygon(&polys[i]);

         // Set the sub-polygon index
         if( PolyIdx && toplvl ) {
            while( pidx < *NbSplit ) {
               (*PolyIdx)[pidx++] = i;
            }
         }
      }

      free(polys);
   } else if( gpce_get_num_vertices(Poly) >= MaxPoints ) { // Check if we need to split the polygon again
      gpc_polygon    clip0,clip1,res;
      gpce_envelope  penv;

      // Get the exterior envelope of the polygon
      gpce_get_envelope(Poly,&penv);

      // Check whether we should split the polygon vertically (X) or horizontally (Y)
      if( penv.max.x-penv.min.x >= penv.max.y-penv.min.y ) {
         // Bigger extent in X, split vertically
         double x = (penv.min.x+penv.max.x)/2.0;
         clip0 = GPCE_MK_RECT2(penv.min.x,penv.min.y,x,penv.max.y);
         clip1 = GPCE_MK_RECT2(x,penv.min.y,penv.max.x,penv.max.y);
      } else {
         // Bigger extent in Y, split horizontally
         double y = (penv.min.y+penv.max.y)/2.0;
         clip0 = GPCE_MK_RECT2(penv.min.x,penv.min.y,penv.max.x,y);
         clip1 = GPCE_MK_RECT2(penv.min.x,y,penv.max.x,penv.max.y);
      }

      // Clip and process the first half
      gpc_polygon_clip(GPC_INT,(gpc_polygon*)Poly,&clip0,&res);
      if( !gpce_poly_split_tile(&res,MaxPoints,Split,NbSplit,PolyIdx,Size) ) {
         gpc_free_polygon(&res);
         goto err;
      }
      gpc_free_polygon(&res);

      // Clip and process the second half
      gpc_polygon_clip(GPC_INT,(gpc_polygon*)Poly,&clip1,&res);
      if( !gpce_poly_split_tile(&res,MaxPoints,Split,NbSplit,PolyIdx,Size) ) {
         gpc_free_polygon(&res);
         goto err;
      }
      gpc_free_polygon(&res);
   } else if( Poly->num_contours ) {
      // Make sure we have enough place for the new polygon
      if( *Size <= *NbSplit ) {
         *Size += 1024;
         REALLOC(*Split,*Size);

         if( PolyIdx )
            REALLOC(*PolyIdx,*Size);
      }

      // Add the polygon to the list
      gpce_copy_polygon(*Split+*NbSplit,Poly);

      // Make sure the outer ring is the first ring int the list
      // Note: we are sure to have a polygon and not a multi-polygon, so there can only be one exterior ring
      if( Poly->hole[0] ) {
         gpc_vertex_list swp;
         int r;

         for(r=1; r<Poly->num_contours; ++r) {
            if( !Poly->hole[r] ) {
               swp = (*Split)[*NbSplit].contour[0];
               (*Split)[*NbSplit].contour[0] = (*Split)[*NbSplit].contour[r];
               (*Split)[*NbSplit].contour[r] = swp;
               (*Split)[*NbSplit].hole[0] = 0;
               (*Split)[*NbSplit].hole[r] = 1;
               break;
            }
         }
      }

      ++(*NbSplit);
   }

   // Shrink the array down to the right size
   if( toplvl ) {
      if( size != *NbSplit ) {
         REALLOC(*Split,*NbSplit);

         if( PolyIdx )
            REALLOC(*PolyIdx,*NbSplit);
      }

      // Set the polygon index to 0 (will only execute if we didn't have a multi-polygon initially, which mean we need to set everything to 0)
      if( PolyIdx && pidx<*NbSplit ) {
         memset(*PolyIdx+pidx,0,*NbSplit-pidx);
      }
      return *NbSplit;
   }

   return 1;

err:
   if( *Split ) free(*Split);
   *Split = NULL;
   *NbSplit = 0;
   return 0;
}

/*----------------------------------------------------------------------------
 * Name     : <gpce_poly_wrap_split>
 * Creation : December 2023 - E. Legault-Ouellet - CMC/CMOE
 *
 * Purpose  : Splits polygon on the wrapping line (ex: -180 -> 180 in longitude)
 *            making them multi-polygons suitable for calculations
 *
 * Args :
 *   <Poly>    : The (multi)polygon to split at the wrapping line
 *   <Env>     : [Optional] Envolope of the given polygon (will be calculated otherwise)
 *   <Dim>     : Dimension of the wrap. 0=X, 1=Y
 *   <R0>      : Inferior point for the wrapping in the given dimension (ex: -180)
 *   <R1>      : Superior point for the wrapping in the given dimension (ex: +180)
 *   <Wrapped> : [OUT] Resulting (multi)polygon IF any changes were done
 *
 * Return: The number of adjustements made (number of crossing treated).
 *         0 means that nothing is changed (and no wrapped polygon will be returned)
 *
 * Remarks :
 *----------------------------------------------------------------------------
 */
int gpce_poly_wrap_split(const gpc_polygon *restrict Poly,const gpce_envelope *restrict Env,int Dim,double R0,double R1,gpc_polygon *restrict Wrapped) {
   gpc_polygon poly = GPC_NULL_POLY;
   gpc_vertex  *v;
   int         c,i,n,adjed=0;
   double      adjust,xy,lxy,dr,mid,amin=0.0,amax=0.0;

   *Wrapped = GPC_NULL_POLY;

   // Make sure we have the L0 and L1 in the right order
   if( R0 > R1 ) {
      dr = R0;
      R0 = R1;
      R1 = dr;
   }

   // Calculate the extent of the range (total range) and middle point
   dr = R1-R0;
   mid = 0.5*(R0+R1);

   // Adjust the x or y coordinate from the [R0,R1] range to the [L0,L1+dr] range.
   // This makes sure no poly wraps around (makes the quantic leap from R0 to R1 or vice versa)
   for(c=0; c<Poly->num_contours; ++c) {
      adjust = 0.0;

      if( (n=Poly->contour[c].num_vertices) > 0 ) {
         // Note: we loop on n-1 values because the first point can't have crossed the wrapping line by itself
         for(i=1,v=Poly->contour[c].vertex+i,lxy=(Dim?(v-1)->y:(v-1)->x); i<n; ++i,++v) {
            xy = Dim ? v->y : v->x;

            // If we travel more than half the total range between two points, then we are probably crossing the wrapping line
            if( fabs(xy-lxy) >= 0.5*dr ) {
               // Update the adjustment by one range extent depending on were the previous point was compared to the adjusted midpoint
               // (by adjusting the midpoint too, we ensure that the crossing back will be handled)
               if( lxy >= mid+adjust ) {
                  adjust += dr;
               } else {
                  adjust -= dr;
               }

               // Update the adjusting range
               amin = fmin(adjust,amin);
               amax = fmax(adjust,amax);

               // Create a copy of the polygon (as modifications will be needed)
               if( !poly.num_contours ) {
                  gpce_copy_polygon(&poly,Poly);
               }

               // Increment the number of adjustments made
               ++adjed;
            }

            // Make adjustments if needed. Note that the poly will exist if an adjustment is needed.
            if( adjust != 0.0 ) {
               if( Dim ) {
                  poly.contour[c].vertex[i].y += adjust;
               } else {
                  poly.contour[c].vertex[i].x += adjust;
               }
            }

            // Update last value
            lxy = xy;
         }
      }
   }

   // Make the clippings if need be
   if( adjed ) {
      gpc_polygon    res,clip,resn;
      gpce_envelope  env;
      int            i0,i1;

      // Clipping rectangle
      int               hole  = 0;
      gpc_vertex        rv[]  = {{0.0,0.0},{0.0,0.0},{0.0,0.0},{0.0,0.0}};
      gpc_vertex_list   rvlst = (gpc_vertex_list){rv,4};
      gpc_polygon       rect  = (gpc_polygon){&rvlst,&hole,1};

      // Calculate the envelope if none was provided (only used to know the extent of the non-wrapping dimension
      if( Env ) {
         env = *Env;
      } else {
         gpce_get_envelope(&poly,&env);
      }

      // Make sure the non-wrapping dimension is never clipped by putting the clipping rectangle way out of reach
      // Note: we use a multiple of the range instead of a fixed value for numerical stability)
      if( Dim ) {
         rv[0].x = rv[3].x = env.min.x - 0.5*(env.max.x-env.min.x);
         rv[1].x = rv[2].x = env.max.x + 0.5*(env.max.x-env.min.x);
      } else {
         rv[0].y = rv[1].y = env.min.y - 0.5*(env.max.y-env.min.y);
         rv[2].y = rv[3].y = env.max.y + 0.5*(env.max.y-env.min.y);
      }

      // Calculate our clipping windows (making sure to not be affected by numerical instability)
      i0 = (int)round(amin/dr);
      i1 = (int)round(amax/dr);

      // Make the clippings
      for(i=i0; i<=i1; ++i) {
         // Adjust the clipping rectangle in the wrapping dimension
         if( Dim ) {
            rv[0].y = rv[1].y = R0 + i*dr;
            rv[2].y = rv[3].y = R0 + (i+1)*dr;
         } else {
            rv[0].x = rv[3].x = R0 + i*dr;
            rv[1].x = rv[2].x = R0 + (i+1)*dr;
         }

         // Make the clipping
         gpc_polygon_clip(GPC_INT,&poly,&rect,&clip);

         // Bring everything back in the given range if needed
         if( i ) {
            adjust = i*dr;
            for(c=0; c<clip.num_contours; ++c) {
               for(n=clip.contour[c].num_vertices,v=clip.contour[c].vertex; n; --n,++v) {
                  if( Dim ) {
                     v->y -= adjust;
                  } else {
                     v->x -= adjust;
                  }
               }
            }
         }

         if( i == i0 ) {
            // First round over, we take the clipping as the result, no need to free anything
            res = clip;
         } else {
            // Merge clipping with previous result
            gpc_polygon_clip(GPC_UNION,&res,&clip,&resn);
            gpc_free_polygon(&res);
            gpc_free_polygon(&clip);
            res = resn;
         }
      }

      // Free our polygon copy
      gpc_free_polygon(&poly);

      // Set the result
      *Wrapped = res;
   }

   return adjed;
}
/*----------------------------------------------------------------------------
 * Name     : <gpce_poly_wrap_clamp>
 * Creation : December 2023 - E. Legault-Ouellet - CMC/CMOE
 *
 * Purpose  : Clamp polygons on the wrapping line wrapping the points accordingly
 *            For example, for the given wrapping range [-180,180], -182 becomes 178
 *
 * Args :
 *   <Poly>    : [IN/OUT] The (multi)polygon to clamp at the wrapping line (modified in place)
 *   <Dim>     : Dimension of the wrap. 0=X, 1=Y
 *   <R0>      : Inferior point for the wrapping in the given dimension (ex: -180)
 *   <R1>      : Superior point for the wrapping in the given dimension (ex: +180)
 *
 * Return:
 *
 * Remarks :
 *----------------------------------------------------------------------------
 */
void gpce_poly_wrap_clamp(gpc_polygon *restrict Poly,int Dim,double R0,double R1) {
   gpc_vertex  *v;
   double      dr;
   int         n,c;

   // Make sure we have the L0 and L1 in the right order
   if( R0 > R1 ) {
      dr = R0;
      R0 = R1;
      R1 = dr;
   }

   // Calculate the extent of the range (total range) and middle point
   dr = R1-R0;

   for(c=0; c<Poly->num_contours; ++c) {
      for(n=Poly->contour[c].num_vertices,v=Poly->contour[c].vertex; n; --n,++v) {
         if( Dim ) {
            while( v->y < R0 ) v->y += dr;
            while( v->y > R1 ) v->y -= dr;
         } else {
            while( v->x < R0 ) v->x += dr;
            while( v->x > R1 ) v->x -= dr;
         }
      }
   }
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_get_ring_envelope>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Retourne une enveloppe pour le contour donné
 *
 * Parametres :
 *  <Ring>  : Le contour dont on veut l'enveloppe
 *  <PEnv>  : L'enveloppe retourné
 *
 * Retour:
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
void gpce_get_ring_envelope(const gpc_vertex_list *restrict Ring,gpce_envelope *restrict PEnv) {
    int i;

    *PEnv = GPCE_MK_ENVELOPE(DBL_MAX,DBL_MAX,-DBL_MAX,-DBL_MAX);

    for(i=0; i<Ring->num_vertices; ++i) {
        PEnv->min.x = fmin(PEnv->min.x,Ring->vertex[i].x);
        PEnv->min.y = fmin(PEnv->min.y,Ring->vertex[i].y);
        PEnv->max.x = fmax(PEnv->max.x,Ring->vertex[i].x);
        PEnv->max.y = fmax(PEnv->max.y,Ring->vertex[i].y);
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_get_envelope>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Retourne l'enveloppe extérieure du polygone
 *
 * Parametres :
 *  <Poly>  : Le polygone dont on veut l'enveloppe
 *
 * Retour: Les enveloppes ou NULL si erreur.
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
void gpce_get_envelope(const gpc_polygon *restrict Poly,gpce_envelope *restrict PEnv) {
   int r,i;

    *PEnv = GPCE_MK_ENVELOPE(DBL_MAX,DBL_MAX,-DBL_MAX,-DBL_MAX);

    for(r=0; r<Poly->num_contours; ++r) {
       if( !Poly->hole[r] ) {
          for(i=0; i<Poly->contour[r].num_vertices; ++i) {
             PEnv->min.x = fmin(PEnv->min.x,Poly->contour[r].vertex[i].x);
             PEnv->min.y = fmin(PEnv->min.y,Poly->contour[r].vertex[i].y);
             PEnv->max.x = fmax(PEnv->max.x,Poly->contour[r].vertex[i].x);
             PEnv->max.y = fmax(PEnv->max.y,Poly->contour[r].vertex[i].y);
          }
       }
    }
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_get_envelopes>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Retourne une enveloppe pour chaque contour d'un polygone
 *
 * Parametres :
 *  <Poly>  : Le polygone dont on veut l'enveloppe
 *
 * Retour: Les enveloppes ou NULL si erreur.
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
gpce_envelope* gpce_get_envelopes(const gpc_polygon *restrict Poly) {
    int r;
    gpce_envelope *penv = NULL;

    if( (penv=malloc(Poly->num_contours*sizeof(*penv))) ) {
        for(r=0; r<Poly->num_contours; ++r) {
            gpce_get_ring_envelope(&Poly->contour[r],&penv[r]);
        }
    }

    return penv;
}

/*----------------------------------------------------------------------------
 * Nom      : <gpce_envelope_intersects_line>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Trouver s'il y a intersection entre une ligne et une enveloppe
 *
 * Parametres :
 *  <PEnv>  : Enveloppe
 *  <L0>    : Premier point de la ligne L
 *  <L1>    : Deuxième point de la ligne L
 *  <Dir>   : Si la direction est prise en compte pour l'intersection.
 *            0 : la direction n'est pas importante
 *            1 : Seule les enveloppes dans la direction du vecteur L0->L1 seront
 *                prises en compte
 *
 * Retour: 1 si intersection, 0 sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int gpce_envelope_intersects_line(const gpce_envelope *restrict PEnv,gpc_vertex L0,gpc_vertex L1,const int Dir) {
    double f,s;
    gpc_vertex tl=GPC_MK_VERTEX(PEnv->min.x,PEnv->max.y),br=GPC_MK_VERTEX(PEnv->max.x,PEnv->min.y);

    if( Dir ) {
        return (gpce_line_intersects_line(L0,L1,PEnv->min,br,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0
            || gpce_line_intersects_line(L0,L1,PEnv->min,tl,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0
            || gpce_line_intersects_line(L0,L1,br,PEnv->max,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0
            || gpce_line_intersects_line(L0,L1,tl,PEnv->max,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0);
    } else {
        return (gpce_line_intersects_line(L0,L1,PEnv->min,br,NULL,&s) && 0.0<=s && s<=1.0
            || gpce_line_intersects_line(L0,L1,PEnv->min,tl,NULL,&s) && 0.0<=s && s<=1.0
            || gpce_line_intersects_line(L0,L1,br,PEnv->max,NULL,&s) && 0.0<=s && s<=1.0
            || gpce_line_intersects_line(L0,L1,tl,PEnv->max,NULL,&s) && 0.0<=s && s<=1.0);
    }
}
