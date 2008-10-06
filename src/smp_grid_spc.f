***s/r smp_grid_spc - Compute grid spacings in x and y directions
*                     (based on GEM's e_pretopo)
*
      subroutine smp_grid_spc (dx, dy, x, y, ni, nj, lagrd, grdtyp)
*
      implicit none
*
      logical     lagrd
      integer     ni, nj
      real        dx(ni), dy(nj), x(ni), y(nj)
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
* dx           | output: grid spacing (in degrees) in the x direction
*              |
* dy           | output: grid spacing (in degrees) in the y direction
*              |
*--------------|------------------------------------------------------
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
*______________|______________________________________________________
*
      integer i, j
*
*************************************************************************
*
      if ( grdtyp.eq.'U' ) then
         do i=2,ni
            dx(i) = x(i) - x(i-1)
         enddo
         if (lagrd) then
            dx(1) = dx(2)
         else
            dx(1) = x(1) + 360. - x(ni)
         endif
      else
         do i=1,ni-1
            dx(i) = x(i+1) - x(i)
         enddo
         if (lagrd) then
            dx(ni) = dx(ni-1)
         else
            dx(ni) = x(1) + 360. - x(ni)
         endif
      endif
*
*-----------------------------------------------------------------------
      if ( grdtyp.eq.'V' ) then
         do j=2,nj
            dy(j) = y(j) - y(j-1)
         enddo
         if (lagrd) then
            dy(1) = dy(2)
         else
            dy(1) = y(1) + 90.
         endif
      else
         do j=1,nj-1
            dy(j) = y(j+1) - y(j)
         enddo
         dy(nj) = dy(nj-1)
      endif
*
*------------------------------------------------------------------
*
      return
      end
