#include <stdlib.h>
#include "gpc_ext.h"

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
double gpce_get_ring_area(const gpc_vertex_list *restrict const Ring) {
    int i,j;
    double area;

    for(i=0,j=1,area=0.0; i<Ring->num_vertices; i=j++) {
        area += (Ring->vertex[j].x-Ring->vertex[i].x) * (Ring->vertex[j].y-Ring->vertex[i].y);
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
 *  <Ring>  : Le polygone
 *
 * Retour: L'aire du polygone
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
double gpce_get_area(gpc_polygon *restrict const Poly) {
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
 * Nom      : <gpce_point_inside_ring>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Vérifie si le point est contenu dans le contour
 *
 * Parametres :
 *  <Ring>  : Le contur
 *  <P>     : Le point
 *
 * Retour: 1 si contenu, 0 sinon
 *
 * Remarques :
 *----------------------------------------------------------------------------
 */
int gpce_point_inside_ring(const gpc_vertex_list *restrict const Ring,gpc_vertex P) {
    int i,j,in;

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
 * Nom      : <gpce_contains_point>
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
int gpce_contains_point(gpc_polygon *restrict const Poly,gpce_envelope *restrict const PEnv,gpc_vertex P) {
    int r,in;

    // Loop on the rings
    for(r=0,in=0; r<Poly->num_contours; ++r) {
        // If we have an interior ring (hole)
        if( Poly->hole[r] ) {
            // If we are in the hole, then we are outside of the polygon
            if( (!PEnv||GPCE_PT_IN_ENV(PEnv[r],P)) && gpce_point_inside_ring(&Poly->contour[r],P) ) {
                return 0;
            }
        } else if( !in ) {
            // We have an exterior ring (shell) and we are not already in another exterior ring
            if( (!PEnv||GPCE_PT_IN_ENV(PEnv[r],P)) && gpce_point_inside_ring(&Poly->contour[r],P) ) {
                in = 1;
            }
        }
    }

    return in;
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
static void gpce_get_ring_envelope(gpc_vertex_list *restrict const Ring,gpce_envelope *restrict PEnv) {
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
gpce_envelope* gpce_get_envelope(gpc_polygon *restrict const Poly) {
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
 * Nom      : <gpce_intersect_line_line>
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
 *  <FA>    : [OUT] Facteur de l'intersection à partir du point A0 sur la ligne A0-A1
 *  <FB>    : [OUT] Facteur de l'intersection à partir du point B0 sur la ligne B0-B1
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
int gpce_intersect_line_line(gpc_vertex A0,gpc_vertex A1,gpc_vertex B0,gpc_vertex B1,double *restrict FA,double *restrict FB) {
    double c,adx,ady,bdx,bdy,dx,dy;

    adx = A1.x - A0.x;
    ady = A1.y - A0.y;
    bdx = B1.x - B0.x;
    bdy = B1.y - B0.y;

    dx = A0.x - B0.x;
    dy = B0.y - A0.y;

    // Note that if c==0.0, the lines are parallel, which means that we either have
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
 * Nom      : <gpce_intersects_line_envelope>
 * Creation : Juillet 2018 - E. Legault-Ouellet
 *
 * But      : Trouver s'il y a intersection entre deux lignes et retourner les
 *            facteurs le cas échéant
 *
 * Parametres :
 *  <PEnv>  : Enveloppe
 *  <L0>    : Premier point de la ligne L
 *  <L1>    : Deuxième point de la ligne L
 *  <Dir>   : 0 si aucune (on considère la ligne), 1 si dans le sens de la ligne
 *            (on considère le vecteur)
 *
 * Retour: 0 si intersection, 1 sinon
 *
 * Remarques :
 *  - Les facteurs sont toujours calculés selon le premier point de la ligne.
 *    Ainsi, intersection = A0+A(A1-A0) = B0+B(B1-B0)
 *  - Si le facteur A ou B est entre 0 et 1, alors l'intersection a lieu entre les
 *    points A0,A1 et B0,B1, respectivement (à l'intérieur des segments).
 *----------------------------------------------------------------------------
 */
int gpce_intersects_line_envelope(gpce_envelope *restrict const PEnv,gpc_vertex L0,gpc_vertex L1,const int Dir) {
    double f,s;
    gpc_vertex tl=GPC_MK_VERTEX(PEnv->min.x,PEnv->max.y),br=GPC_MK_VERTEX(PEnv->max.x,PEnv->min.y);

    if( Dir ) {
        return gpce_intersect_line_line(L0,L1,PEnv->min,br,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0
            || gpce_intersect_line_line(L0,L1,PEnv->min,tl,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0
            || gpce_intersect_line_line(L0,L1,br,PEnv->max,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0
            || gpce_intersect_line_line(L0,L1,tl,PEnv->max,&f,&s) && 0.0<=s && s<=1.0 && f>=0.0;
    } else {
        return gpce_intersect_line_line(L0,L1,PEnv->min,br,NULL,&s) && 0.0<=s && s<=1.0
            || gpce_intersect_line_line(L0,L1,PEnv->min,tl,NULL,&s) && 0.0<=s && s<=1.0
            || gpce_intersect_line_line(L0,L1,br,PEnv->max,NULL,&s) && 0.0<=s && s<=1.0
            || gpce_intersect_line_line(L0,L1,tl,PEnv->max,NULL,&s) && 0.0<=s && s<=1.0;
    }
}
