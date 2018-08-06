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
                    if( adx != 0.0 ) {
                    }
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
    double fa,fb;
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
            if( gpce_line_intersects_line(RingA->vertex[a0],RingA->vertex[a1],RingB->vertex[b0],RingB->vertex[b1],&fa,&fb) && 0.0<=fa && fa<=1.0 && 0.0<=fb && fb<=1.0 ) {
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
    for(i=0,j=1,in=0; j<Ring->num_vertices; i=j++) {
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
    // Make sure we have points
    if( OutRing->num_vertices<=0 || InRing->num_vertices<=0 ) {
        return 0;
    }

    // Check if the envelope is contained
    if( OEnv && IEnv && (OEnv->min.x>=IEnv->min.x || OEnv->min.y>=IEnv->min.y || OEnv->max.x<=IEnv->max.x || OEnv->max.y<=IEnv->max.y) ) {
        return 0;
    }

    // Check if the first point is inside the ring
    if( !gpce_ring_contains_point(OutRing,OEnv,InRing->vertex[0]) ) {
        return 0;
    }

    // Check if there is an intersection between each segments
    if( gpce_ring_crosses_ring(OutRing,OEnv,InRing,IEnv) ) {
        // An intersection means at least some part is outside
        return 0;
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
    int r,in=0;

    // Check if we have a multi-polygon
    if( gpce_get_num_polygon(Poly)<=1 ) {
        // Loop on the rings
        for(r=0; r<Poly->num_contours; ++r) {
            // If we have an interior ring (hole)
            if( Poly->hole[r] ) {
                // If we are in the hole, then we are outside of the polygon
                if( gpce_ring_contains_point(Poly->contour+r,PEnv?PEnv+r:NULL,P) ) {
                    return 0;
                }
            } else {
                // Since we made sure to only have one exterior ring, we need to be inside that ring
                if( !gpce_ring_contains_point(Poly->contour+r,PEnv?PEnv+r:NULL,P) ) {
                    return 0;
                }
                in = 1;
            }
        }
    } else {
        gpc_polygon     *poly;
        gpce_envelope   **penv;
        int             n;

        // Split the multi polygon in individual polygons
        n = gpce_split_polygons(Poly,PEnv,&poly,&penv);

        // Loop on the polygons. If one polygon contains the ring, then it is contained
        for(r=0; r<n; ++r) {
            if( gpce_polygon_contains_point(poly+r,penv[r],P) ) {
                return 1;
            }
        }
        free(poly);
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
    int r,in=0;

    if( Ring->num_vertices <= 0 )
        return 0;

    // Check if we have a multi-polygon
    if( gpce_get_num_polygon(Poly)<=1 ) {
        // Loop on the rings
        for(r=0; r<Poly->num_contours; ++r) {
            // If we have an interior ring (hole)
            if( Poly->hole[r] ) {
                // If the first point is inside the hole or we intersect the hole in any way, than the ring is not fully contained in the polygon
                if( gpce_ring_contains_point(Poly->contour+r,PEnv?PEnv+r:NULL,Ring->vertex[0])
                        || gpce_ring_crosses_ring(Poly->contour+r,PEnv?PEnv+r:NULL,Ring,REnv) ) {
                    return 0;
                }
            } else {
                // Since we made sure to only have one exterior ring, we need to be fully contained in the ring
                if( !gpce_ring_contains_ring(Poly->contour+r,PEnv?PEnv+r:NULL,Ring,REnv) ) {
                    return 0;
                }
                in = 1;
            }
        }
    } else {
        gpc_polygon     *poly;
        gpce_envelope   **penv;
        int             n;

        // Split the multi polygon in individual polygons
        n = gpce_split_polygons(Poly,PEnv,&poly,&penv);

        // Loop on the polygons. If one polygon contains the ring, then it is contained
        for(r=0; r<n; ++r) {
            if( gpce_polygon_contains_ring(poly+r,penv[r],Ring,REnv) ) {
                return 1;
            }
        }
        free(poly);
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

static void gpce_copy_ring(gpc_vertex_list *restrict Dest,const gpc_vertex_list *restrict Src) {
    Dest->vertex = malloc(Src->num_vertices*sizeof(*Dest->vertex));
    memcpy(Dest->vertex,Src->vertex,Src->num_vertices*sizeof(*Dest->vertex));

    Dest->num_vertices = Src->num_vertices;
}
static void gpce_copy_polygon(gpc_polygon *restrict Dest,const gpc_polygon *restrict Src) {
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
int gpce_split_polygons(const gpc_polygon *restrict Poly,const gpce_envelope *restrict PEnv,gpc_polygon **PList,gpce_envelope ***EList) {
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
                    *elist = gpce_get_envelope(Poly);
                }
            }
        } else {
            // Make sure we have an envelope because that has the potential to greatly speedup the process
            penv = PEnv?PEnv:gpce_get_envelope(Poly);

            // Loop on the exterior rings
            for(r=0,e=0; r<Poly->num_contours; ++r) {
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

                    // Find all interior rings contained in this exterior ring and copy them
                    for(c=0; c<Poly->num_contours; ++c) {
                        if( Poly->hole[c] && gpce_ring_contains_ring(Poly->contour+r,penv+r,Poly->contour+c,penv) ) {
                            if( elist ) {
                                elist[e][plist[e].num_contours] = penv[r];
                            }
                            gpce_copy_ring(plist[e].contour+plist[e].num_contours++,Poly->contour+c);
                        }
                    }

                    // Resize the memory to the right size
                    plist[e].contour = realloc(plist[e].contour,plist[e].num_contours*sizeof(*plist[e].contour));
                    if( elist ) {
                        elist[e] = realloc(elist[e],plist[e].num_contours*sizeof(*elist[e]));
                    }

                    // Set all but the first ring as holes
                    plist[e].hole = malloc(plist[e].num_contours*sizeof(*plist[e].contour));
                    plist[e].hole[0] = 0;
                    for(c=1; c<Poly->num_contours; ++c) {
                        plist[e].hole[c] = 1;
                    }

                    ++e;
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
 * Nom      : <gpce_get_envelope>
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
static void gpce_get_ring_envelope(const gpc_vertex_list *restrict Ring,gpce_envelope *restrict PEnv) {
    int i;

    *PEnv = GPCE_MK_ENVELOPE(DBL_MAX,DBL_MAX,DBL_MIN,DBL_MIN);

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
gpce_envelope* gpce_get_envelope(const gpc_polygon *restrict Poly) {
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
        return gpce_line_intersects_line(L0,L1,PEnv->min,br,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0
            || gpce_line_intersects_line(L0,L1,PEnv->min,tl,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0
            || gpce_line_intersects_line(L0,L1,br,PEnv->max,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0
            || gpce_line_intersects_line(L0,L1,tl,PEnv->max,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0;
    } else {
        return gpce_line_intersects_line(L0,L1,PEnv->min,br,NULL,&s) && 0.0<=s && s<=1.0
            || gpce_line_intersects_line(L0,L1,PEnv->min,tl,NULL,&s) && 0.0<=s && s<=1.0
            || gpce_line_intersects_line(L0,L1,br,PEnv->max,NULL,&s) && 0.0<=s && s<=1.0
            || gpce_line_intersects_line(L0,L1,tl,PEnv->max,NULL,&s) && 0.0<=s && s<=1.0;
    }
}
