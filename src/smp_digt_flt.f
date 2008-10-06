***s/r smp_digt_flt - Simplified digital filter of topography field
*                     (based on GEM's digital filter e_setdgf)
*
      subroutine smp_digt_flt (topo, x, y, ni, nj,
     *                         lagrd, grdtyp,
     *                         dgfm, lcfac, mlr, mapfac, norm)
      implicit none
*
      logical     lagrd, mapfac, norm
      integer     ni, nj, dgfm
      real        lcfac, mlr
      real        topo(ni,nj), x(ni), y(nj)
      character*1 grdtyp
*
*author - Ayrton Zadra  - Jul 2004
*
*object
*       See above ID.
*
*arguments
*
*_____________________________________________________________________
* NAME         | DESCRIPTION
*--------------|------------------------------------------------------
* topo         | input/output topography field 
*              | (input: non-filtered | output: filtered field)
*              |
* x            | longitude (in degrees) of grid point i
*              |
* y            | latitude  (in degrees) of grid point j
*              |
* ni           | number of grid points along x axis
*              |
* nj           | number of grid points along y axis
*              |
*--------------|------------------------------------------------------
* lagrd        | if .true.  then grid is 'limited-area'
*              | if .false. then grid is global
*              |
* grdtyp       | 3 options only: 'G' if topography is on PHI grid
*              |                 'U' if topography is on U grid
*              |                 'V' if topography is on V grid
*              |
*--------------|------------------------------------------------------
* dgfm         | 2*(dgfm-1) = maximum number of neighbour points
*              |              to be used by the digital filter
*              |              (dgfm >= 2)
*              |
* lcfac        | factor that controls the critical wavelength
*              |
* mlr          | minimum value of mesh-length ratio necessary to 
*              | activate the digital filter (mlr >= 1.0)
*              |
* mapfac       | if .true. then consider the map-scale factor
*______________|______________________________________________________
*
      integer i, j, n, im, ip, jm, jp, pt
      real    hx, hy
      real    w1(ni,nj), dx(ni), dy(nj)
      real*8  c(dgfm)
      real*8  r, d2r_8
*
*************************************************************************
*
      d2r_8  = acos( -1.0 ) / 180.0
*
*************************************************************************
*     compute grid spacings
*
      call smp_grid_spc (dx, dy, x, y, ni, nj, lagrd, grdtyp)
*
************************************************************************
*     apply filter
*
      do j=1,nj
         do i=1,ni
*
         hy = dy(j)
         if (mapfac) then
            hx = dx(i) * cos( y(j) * d2r_8 )
         else
            hx = dx(i)
         endif
*
         c(1) = 1.
         if ( dgfm .ge. 1 ) then
            do n=1,dgfm-1
               c(n+1) = 0.
            enddo
         endif
*
*-----------------------------------------------------------------------
         if ( hy .gt. ( mlr*hx ) ) then
*
            r  = 0.5 * lcfac * hy / hx
            pt = nint( 4.0 * r )
            pt = min0( dgfm-1, pt )
            if (lagrd) then
               pt = min0( ni-i, pt )
               pt = min0(  i-1, pt )
            endif
*
            if (pt .ge. 1) then
               call smp_coef_dgt (c, pt, r, norm)
               w1(i,j) = c(1) * topo(i,j)
               do n=1,pt
                  im = i - n
                  ip = i + n
                  if ( .not.lagrd .and. im .lt. 1  ) im = ni + im
                  if ( .not.lagrd .and. ip .gt. ni ) ip = ip - ni
                  w1(i,j) = w1(i,j) + c(n+1) *
     %                      ( topo(im,j) + topo(ip,j) )
               enddo
            else
               w1(i,j) = topo(i,j)
            endif
*
*-----------------------------------------------------------------------
         elseif ( hx .gt. ( mlr*hy ) ) then
*
            r  = 0.5 * lcfac * hx / hy
            pt = nint( 4.0 * r )
            pt = min0( dgfm-1, pt )
            pt = min0( nj-j, pt )
            pt = min0(  j-1, pt )
*
            if (pt .ge. 1) then
               call smp_coef_dgt (c, pt, r, norm)
               w1(i,j) = c(1) * topo(i,j)
               do n=1,pt
                  jm = j - n
                  jp = j + n
                  w1(i,j) = w1(i,j) + c(n+1) *
     %                      (topo(i,jm) + topo(i,jp) )
               enddo
            else
               w1(i,j) = topo(i,j)
            endif
*
*-----------------------------------------------------------------------
         else
*
            w1(i,j) = topo(i,j)
*
         endif
*
         enddo
      enddo
*
************************************************************************
*     update topography
*
      do j=1,nj
         do i=1,ni
            topo(i,j)  = w1(i,j)
         enddo
      enddo
*
*------------------------------------------------------------------
*
      return
      end
