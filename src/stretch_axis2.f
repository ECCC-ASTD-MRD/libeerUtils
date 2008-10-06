***function: STRETCH_AXIS2 - will return a stretched axis given the
*                           parameters NX,F_nxla,F_xbeg,F_xend,F_dxla
*
      integer function stretch_axis2
     $     ( F_x_8, F_dxla, F_xbeg, F_xend, F_margin, NX, F_nxla, F_amp,
     $       F_stagger_L, F_print_L, F_dxmax, F_nimax, F_gauss_L)
*
      implicit none
*
      integer F_margin, NX, F_nxla, F_nimax
      real    F_amp, F_dxla, F_xbeg, F_xend, F_dxmax
      real*8  F_x_8(NX)
      logical F_stagger_L, F_print_L, F_gauss_L
*author  Vivian Lee - July 1999
*
*revision
* v2_00 - Lee V.            - initial MPI version
* v2_20 - Lee V.            - converted input F_x to F_x_8 (real*8)
* v2_30 - A. Methot         - introduction of a new stretch grid design
* v2_30 -                     with upper limits on grid point spacing
* v3_11 - M. Tanguay        - Introduce Grd_gauss_L
*
*object
*        see above id
*
*arguments
*  Name        I/O                 Description
*----------------------------------------------------------------
* stretch_axis O     - value of 0(no error), -1 (error)
* F_x_8        O     - axis containing values for each grid point
* F_dxla       I/O   - on input,number of degrees between grid points in
*                      the uniform domain.
*                    - on output, calculated number of degrees between grid
*                      points if the axis is uniform
* F_xbeg       I     - starting (latitude/longitude) degree of the axis
* F_xend       I     - ending (latitude/longitude) degree of the axis
* F_margin     O     - number of points between the border of the variable
*                      variable grid to the border of the uniform domain
* NX           I     - total number of points of the grid in the axis
* F_nxla       I     - number of points in the uniform domain of the axis
* F_amp        O     - the amplification factor used to determine the grid
*                      points outside of the uniform domain.
* F_stagger_L  I     - .TRUE.if grid points do not lie on the end points
*                      of the axis (F_xbeg,F_xend)
* F_print_L    I     - .TRUE. to print comments,values of this function
* F_dxmax      I     - upper limit on grid spacing (degrees)
* F_nimax      O     - number of points having upper limit grid spacing
* F_gauss_L    I     - TRUE if GEM grid is set as Gaussian
*
*
*notes
*      The function qqqroot3 is included in this deck (written by J.Cote)
*
*modules
      real*8 qqqroot3
      external qqqroot3
*
      logical sampar
      integer nit, it, i
      real*8 a0, am, eps, xdist_8, e, guess, hx, x1, x2
      real*8 amp_8, F_dxla_8
      real*8 hxmax
*
      real*8  deg2rad_8
      real*8, parameter :: CLXXX_8 = 180.0
      real*8, parameter :: ONE_8   = 1.0
*
      real groots(NX)
      real ay(NX)
      real*8 G_ygauss_8(NX+1)
      integer status,j
*
      F_dxla_8 = F_dxla
      hxmax    = F_dxmax
*
      if (F_print_L) then
      print *,'*'
      print *,'*** stretch_axis(begin) *******'
      print *,'*'
      print *,'NX = ',NX,' F_nxla = ',F_nxla,' F_dxla = ',F_dxla,' F_sta
     %gger_L = ',
     % F_stagger_L
      print *,'F_xbeg = ',F_xbeg,' F_xend = ',F_xend
      endif
      xdist_8    = F_xend - F_xbeg
      guess = - 1.0
      eps   = 1.0e-15
      nit   = 10
*
*     Evaluate Gaussian latitudes if requested
*     ----------------------------------------
      if(F_gauss_L.and.F_stagger_L) then
*
        if( NX .ne. F_nxla) then
           stretch_axis2=-1
           return
*            call gem_stop('STRETCH_AXIS2 NOT VALID',-1)
        endif
*
        deg2rad_8 = acos( -ONE_8 )/CLXXX_8
*
        call ez_glat (ay,groots,NX,0)
*
        do j=1,NX
           G_ygauss_8(j) = ay(j)
        enddo
           G_ygauss_8(NX+1) = G_ygauss_8(1) + 180.0
*
      endif
*
      if ( NX .lt. F_nxla ) then
           print *,'*'
           print *, 'STRETCH_AXIS ERROR: NX = ',NX,' < ',' F_nxla = ',F_
     %nxla
           stretch_axis2=-1
           return
      else if ( NX .eq. F_nxla ) then
*        the grid is uniform
         amp_8 = 1.0
         e = 0.0
         if ( F_stagger_L ) then
            F_dxla_8 = xdist_8/(NX)
         else
            F_dxla_8 = xdist_8/(NX-1)
         endif
         F_margin = 0
         F_nimax  = 0
         if (F_print_L) then
         print *
         print *,'Uniform Grid Detected: F_dxla=',F_dxla_8
         endif
      else
*        the grid is variable
         if (F_print_L) then
         print *
         print *,'Variable Grid Detected'
         endif
         F_nimax  = 0
         if ( (NX-1)*F_dxla .ge. xdist_8 ) then
         print *,'*'
         print *,'STRETCH_AXIS ERROR: ',
     %  '(NX-1)*F_dxla = ',(NX-1)*F_dxla,' ge F_xend-F_xbeg = ',xdist_8
         stretch_axis2=-1
         return
         endif
       endif
*C-----------------------------------------------------------------------
*C    -BEGIN ITERATION LOOP IF UPPER LIMIT GRID SPACING IS REACHED -------
*C-----------------------------------------------------------------------
8888     continue
       if (  NX .gt. F_nxla ) then
           if ( F_nimax .gt. ( NX - F_nxla - 1 ) /2 ) then
              print *,'STRETCH_AXIS ERROR: no convergence in ',
     %                ' in nimax iteration: nimax=', F_nimax
              stretch_axis2=-1
              return
           endif
           a0 = 0.5 *( ( F_nxla-1 ) -
     %                ( xdist_8 - 2. * F_nimax * hxmax )/F_dxla_8
     %               )
           if ( a0 .gt. 0.0 ) then
             print *
             print *,'STRETCH_AXIS ERROR: ',
     %     'illegal values for F_nxla and F_dxla, a0 = ',a0,' < 0'
             stretch_axis2 = -1
             return
           endif
*
           if (F_print_L) print *,'a0 = ',a0
           sampar = mod( NX - F_nxla, 2 ) .eq. 0
           if (F_print_L) print *,'sampar = ',sampar,' NX=',NX,' F_nxla=
     %',F_nxla
*
           if ( .not. sampar ) then
              print *
              print *,'STRETCH_AXIS ERROR: sampar must be true'
              print *,'Cannot have equal points (F_margins) on either',
     %                ' side of uniform grid'
              stretch_axis2 = -1
              return
           endif
            F_margin = ( NX - F_nxla )/2 - F_nimax
            if (F_print_L) print *,'F_margin = ',F_margin
           if ( .not. F_stagger_L ) then
              am = 1.0
              amp_8 = qqqroot3 ( guess,a0,am,F_margin,nit,eps,it,e )
              if ( it .lt. 0 ) then
                 print *
                 print *,'STRETCH_AXIS: ERROR in QQQROOT3 function'
                 stretch_axis2 = -1
                 return
              endif
             else
              am = 1.5
              amp_8 = qqqroot3 ( guess,a0,am,F_margin,nit,eps,it,e)
             if ( it .lt. 0 ) then
               print *
               print *,'STRETCH_AXIS: ERROR in QQQROOT3 function'
               stretch_axis2 = -1
               return
             endif
           endif
      endif
*
      if (F_print_L) print *,'amp_8 = ',amp_8,' estimate = ',e
      if (F_print_L) print *,'nimax = ',F_nimax
*
*     phi-grid
*
      x1  = - 0.5 * ( F_nxla - 1 ) * F_dxla_8 + (F_xbeg+F_xend)/2.0
      F_x_8(F_nimax+F_margin+1) = x1
      if(F_gauss_L.and.F_stagger_L) F_x_8(F_nimax+F_margin+1) = G_ygauss
     %_8(1)
*
C                               computed grid points in the stretched sector
C                                      to the left or bottom of central area
      x2 = x1
      hx = F_dxla_8
      do i=F_nimax+F_margin,F_nimax+1,-1
         hx   = amp_8 * hx
         x2   = x2 - hx
         F_x_8(i) = x2
      enddo
      if ( hx .gt. hxmax ) then
         F_nimax=F_nimax+1
         go to 8888
      endif
*C-----------------------------------------------------------------------
*C    ---END ITERATION LOOP IF UPPER LIMIT GRID SPACING IS REACHED -------
*C-----------------------------------------------------------------------
C                               compute grid points in the central area
C                                                  of uniform resolution
      if(F_gauss_L.and.F_stagger_L) then
         do i=F_nimax+F_margin+1,F_nimax+F_margin+F_nxla-1
            F_x_8(i+1) = G_ygauss_8(i+1)
         enddo
      else
         do i=F_nimax+F_margin+1,F_nimax+F_margin+F_nxla-1
         x2     = x1 + ( i - F_margin - F_nimax ) * F_dxla_8
         F_x_8(i+1) = x2
         enddo
      endif
*
      hx = F_dxla_8
C                               computed grid points in the stretched sector
C                                       to the right or top of central area
      do i=F_nimax+F_margin+F_nxla,NX-F_nimax
         hx     = amp_8 * hx
         x2     = x2 + hx
         F_x_8(i+1) = x2
      enddo
      if ( hxmax/hx .gt. amp_8 .and. F_nimax .ne. 0 ) then
             print *,'STRETCH_AXIS ERROR: ',
     %   ' problem with amplification factor '
             stretch_axis2 = -1
             return
      endif
C                 NOTING IS DONE HERE IF F_nimax=0
C                        compute grid points in the upper limit uniform
C                        resolution area to the left or bottom end of grid
      do i=F_nimax,1,-1
         F_x_8(i)=F_x_8(i+1)-hxmax
      enddo
C                 NOTING IS DONE HERE IF F_nimax=0
C                        compute grid points in the upper limit uniform
C                        resolution area to the right or top end of grid
      do i=NX-F_nimax+1,NX
         F_x_8(i)=F_x_8(i-1)+hxmax
      enddo
      if (F_print_L) then
      print *,'x  = ',(F_x_8(i),i=1,NX)
      print *,'F_x_8(end)-F_x_8(beg) = ', (F_x_8(NX) - F_x_8(1))
      print *,'*'
      print *,'*** stretch_axis(end) *******'
      print *,'*'
      endif
      stretch_axis2 = 0
      F_amp = amp_8
      F_dxla = F_dxla_8
      F_margin = F_margin + F_nimax
      return
      end
*
***function: QQQROOT3 - finds a root of a0+r+r**2+...+r**(m-1)+am*r**m=0.,
*                    using at most nit iterations of bodewigs method,
*                    with initial guess r = x or computed.
*
      real*8 function qqqroot3( x, a0, am, m, nit, eps, it, e )
      implicit none
      real*8 a0, am, x, eps
      integer m, nit, it
*
*author  j. cote  - august 1995 - modification of root2x -> root2
*
*arguments
*   in     - x   - first guess, if x > 0
*          - a0  - constant coefficient of equation
*          - am  - coefficient of r ** m
*          - m   - degree of polynomial equation
*          - nit - max no. of iterations
*          - eps - accuracy of the root
*   out    - it  - no of iter. taken, failure flag if < 0
*
**
*----------------------------------------------------------------------
      real*8 f, fp, fs, e, de, fm
*----------------------------------------------------------------------
*
      it = 0
      if ( x .gt. 0.0 ) then
*
         qqqroot3 = x
         e = qqqroot3 - 1.0
*
      else
*
*        compute first guess assuming e is near 0.0
*
*        coefficients of power series
*
*        c0 =  a0 + m - 1 + am
*        c1 =  m * ( m - 1 + 2 * am )/2
*        c2 =  m * ( m - 1 ) * ( m - 2 + 3 * am )/6
*        c3 =  m * ( m - 1 ) * ( m - 2 ) * ( m - 3 + 4 * am )/24
*
         fm = m
*        f  = - c0/c1
         f  = - 2.0 * ( a0 + am + fm - 1.0 )/
     %        ( fm * ( fm - 1.0 + 2.0 * am ) )
*        fp = + c2/c1
         fp =  ( fm - 1.0 ) * ( fm + 3.0 * am - 2.0 )/
     %         ( 3.0 * ( fm - 1.0 + 2.0 * am ) )
*        fs = + c3/c1
         fs =  ( fm - 1.0 ) * ( fm - 2.0 ) * ( fm - 3.0 + 4.0 * am )/
     %        ( 12.0 * ( fm - 1.0 + 2.0 * am )  )
*
*      first order estimate
*
*      second order estimate
*
         e = f/( 0.5 + sqrt( 0.25 + fp * f ) )
*
*      third order estimate
*
         e = f/( 1.0 + e * ( fp + e * fs ) )
         qqqroot3 = 1.0 + e
*
      endif
*
      do 200 it=1,nit
*
         de = qqqroot3 ** ( m - 2 )
         f  = a0 + qqqroot3 * ( ( qqqroot3 * de - 1.0 )/e +
     %        am * qqqroot3 * de )
         fs = ( ( ( fm - 1.0 ) * e - 1.0 ) * qqqroot3 * de + 1.0 )/
     %        (e**2)
         fp = fs + am * fm * qqqroot3 * de
         fs = ( fm * ( fm - 1.0 ) * ( 1.0 + am * e ) * de - 2.0 * fs )/e
*
*      bodewigs method for correcting the root ( 3rd order convergence )
*
         de = - f / fp
         de = - f / ( fp + 0.5 * fs * de )
         e = e + de
         qqqroot3 = 1.0 + e
*
         if ( abs(de) .le. eps ) return
*
  200 continue
      it = - nit
*
*----------------------------------------------------------------------
*
      return
      end
*
*s/r xpyp_m -- Computes lat-lon of all points of a mercator
*              projection grid (ni,nj)
*
      subroutine xpyp_m ( xp,yp,iref,jref,latref,lonref,deltax,
     $                                        center_phi,ni,nj )
      implicit none
      integer ni,nj,iref,jref
      real*8 lonref,latref,deltax,center_phi,xp(ni),yp(nj)
*
* arguments:
* xp  o  longitudes
* yp  o  latitudes
* iref,jref   i  reference point (1<=iref<=ni) (1<=jref<=nj)
* lonref      i  longitude of the reference point (degrees)
* latref      i  latitude  of the reference point (degrees)
* deltax      i  grid mesh (m)
* center_phi  i  projection true at center_phi latitude
* ni,nj       i  grid dimensions
*
      integer i,j,dum
      real*8 pi, d2r, c1, c2, c3, c4, sum, rayt, zero, two, qvd,
     $       cqv, mille
      real xp1(ni),yp1(nj)
*
      parameter (zero=0.0d0, two=2.0d0, rayt=6.37122d+03, qvd=90.0d0,
     $           cqv=180.0d0,mille=1000.0d0)
*
*-------------------------------------------------------------------
*
      pi  = two*acos(zero)
      d2r = pi / cqv
      c1  = (deltax / mille) / (rayt * cos(center_phi * d2r) * d2r)
      c2  = two / d2r
      c3  = c1 * d2r
      c4  = tan((latref+qvd)/c2)
*
***   Longitudes
      do i = 1, ni
         xp(i) = lonref + dble(i-iref) * c1
         if (xp(i).lt.0.0d0) xp(i)= xp(i)+360.0d0
         xp1(i) = lonref + dble(i-iref) * c1
         if (xp1(i).lt.0.0d0) xp1(i)=xp1(i)+360.0d0
      end do
*
***   Latitudes
      do j = 1, nj
         yp(j) = c2*atan(c4*exp(c3*dble(j-jref)))-qvd
      end do
*
      write (6,98)
      write (6,99)
      write (6,100) deltax,center_phi
      write (6,101) yp(nj),xp(1),yp(nj),xp(ni)
      write (6,102) yp(nj/2+1),xp(ni/2+1)
      write (6,101) yp(1),xp(1),yp(1),xp(ni)
      write (6,105) yp(jref),xp(iref),iref,jref
      write (6,98)
      print*
*
 98   format (60('#'))
 99   format (18x,'GRID DESCRIPTION')
 100  format (5x,'d60(m)= ',f14.4,'   phiref(deg)= 'f7.3)
 101  format (/'(',f10.4,',',f10.4,')',10x,'(',f10.4,',',f10.4,')')
 102  format (//16x,'(',f10.4,',',f10.4,')'//)
 105  format (/6x,'REF: ','(',f10.2,',',f10.2,') at pnt(',i4,',',i4,')')
 106  format ('(',f10.4,',',f10.4,')')
*-------------2------------------------------------------------------
      return
      end
*
*s/r xpyp_p -- Computes positions of all points of a polar
*              stereographic projection grid (ni,nj)
*
      subroutine xpyp_p ( xp,yp,ir,jr,latr,lonr,d60,dgrw,ns,ni,nj )
      implicit none
*
      character* (*) ns
      integer ir,jr,ni,nj
      real latr,lonr,dgrw
      real*8 d60,xp(ni),yp(nj)
*
* arguments:
* xp          o  longitudes or North pole location along-x in km
* yp          o  latitudes  or North pole location along-y in km
* ir,jr       i  reference point (1<=ir<=ni) (1<=jr<=nj)
* lonr        i  longitude of the reference point (degrees)
* latr        i  latitude  of the reference point (degrees)
* d60         i  grid mesh (m)
* dgrw        i  grid rotation (degrees)
* ni,nj       i  grid dimensions
* gt          i  grid type: "Y" ===> will compute lat-lon
*                           "Z" ===> will compute North Pole location
*
      integer i,j,nhem
      real xp1,yp1,d60_4
      real*8 x,y
*
*-------------------------------------------------------------------
*
      if (ns.eq.'N') nhem=1
      if (ns.eq.'S') nhem=2
      d60_4=d60
      call xyfll (xp1,yp1,latr,lonr,d60_4,dgrw,nhem)
      x = (xp1-dble(ir-1))
      y = (yp1-dble(jr-1))
*
      do i=1,ni
         xp(i) = (x + dble(i-1)) * d60 / 1000.0d0
      end do
      do j=1,nj
         yp(j) = (y + dble(j-1)) * d60 / 1000.0d0
      end do
*
*-------------------------------------------------------------------
      return
      end
