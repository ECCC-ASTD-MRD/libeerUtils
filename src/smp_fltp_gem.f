***s/r smp_fltp_gem - Simplified filter of topography field 
*                     (combines GEM's digital + 2-delta-xy filters) 
*
      subroutine smp_fltp_gem (topo, x, y, ni, nj,
     *                         lagrd, grdtyp,
     *                         digfil, dgfm, lcfac, mlr, mapfac, norm,
     *                         tdxfil, frco)
      implicit none
*
      logical     lagrd, digfil, tdxfil, mapfac, norm
      integer     ni, nj, dgfm
      real        lcfac, mlr, frco
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
* digfil       | if .true. then apply digital filter
*              |
* dgfm         | 2*(dgfm-1) = maximum number of neighbour points
*              |              to be used by the digital filter
*              |              (dgfm >= 1)
*              |
* lcfac        | factor that controls the critical wavelength
*              |
* mlr          | minimum value of mesh-length ratio necessary to 
*              | activate the digital filter (mlr >= 1.0)
*              |
* mapfac       | if .true. then consider the map-scale factor
*              |
* norm         | if .true. then use normalized coefficients for
*              |                digital filter
*              |
*--------------|------------------------------------------------------
* tdxfil       | if .true. then apply 2-delta-xy filter
*              |
* frco         | 2-delta-xy filter coefficient (0.0 <= frco <= 0.5)
*______________|______________________________________________________
*
*
      integer i, j, nio, njo
      real , dimension (:,:) , allocatable :: w1
**********************************************************************
*     Transfer topography field topo, of size (ni X nj) and provided 
*     by the entry, to a work field w1 of size (nio x njo), where 
*     (nio, njo) are the model's conventional horizontal dimensions.
*
      if (lagrd) then
         nio = ni
      else
         nio = ni - 1
      endif
      njo = nj
*
      allocate ( w1(nio,njo) )
*
      do j=1,njo
         do i=1,nio
            w1(i,j) = topo(i,j)
         enddo
      enddo
*
**********************************************************************
*     Apply digital filter
*
      if (digfil) then
*
         do j=1,njo
            do i=1,nio
               w1(i,j) = max( 0.0, w1(i,j) )
            enddo
         enddo
*
         call smp_digt_flt (w1, x, y, nio, njo,
     *                      lagrd, grdtyp,
     *                      dgfm, lcfac, mlr, mapfac, norm)
*
      endif
*
***********************************************************************
*     Apply 2-delta-xy filter
*
      if (tdxfil) then
*
         do j=1,njo
            do i=1,nio
               w1(i,j) = max( 0.0, w1(i,j) )
            enddo
         enddo
*
         call smp_2del_flt (w1, x, y, nio, njo,
     *                      lagrd, grdtyp,
     *                      frco)
*
      endif
*
**********************************************************************
*     Update topography with the filtered field
*
      do j=1,nj
         do i=1,nio
            topo(i,j)  = w1(i,j)
         enddo
      enddo
      if (.not.lagrd) then
         do j=1,nj
            topo(ni,j) = topo(1,j)
         enddo
      endif
*------------------------------------------------------------------
*
      return
      end
