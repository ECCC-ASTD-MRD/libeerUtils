***s/r smp_2del_flt - Simplified 2-delta-xy filter of topography field
*                     (based on GEM's filter e_ntrxyfil) 
*
      subroutine smp_2del_flt (topo, x, y, ni, nj,
     *                         lagrd, grdtyp,
     *                         frco)
      implicit none
*
      logical     lagrd
      integer     ni, nj
      real        frco
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
* frco         | filter coefficient (0.0 <= frco <= 0.5)
*______________|______________________________________________________
*
      integer i, j
      real*8  a, b, ms, mn, mb
      real w1(ni,nj), dx(ni), dy(nj)
*
**********************************************************************
*     Compute grid spacings
*
      call smp_grid_spc (dx, dy, x, y, ni, nj, lagrd, grdtyp)
*
**********************************************************************
*     Apply filter along X
*
      if ( lagrd ) then
         do j=1,nj
            w1(1,j)  = frco*0.5*( topo(1,j) + topo(2,j) )
     %                 + (1. - frco)*topo(1,j)
            do i=2,ni-1
               a = dx(i) / ( dx(i) + dx(i-1) )
               w1(i,j) = frco*( a*topo(i-1,j) + (1. - a)*topo(i+1,j) )
     %                   + (1. - frco)*topo(i,j)
            enddo
            w1(ni,j) = frco*0.5*( topo(ni-1,j) + topo(ni,j) )
     %                 + (1. - frco)*topo(ni,j)
         enddo
*--------------------------------------------------------------------
      else
         do j=1,nj
            a = dx(1) / ( dx(1) + dx(ni) )
            w1(1,j) = frco*( a*topo(ni,j) + (1. - a)*topo(2,j) )
     %                + (1. - frco)*topo(1,j)
            do i=2,ni-1
               a = dx(i) / ( dx(i) + dx(i-1) )
               w1(i,j) = frco*( a*topo(i-1,j) + (1. - a)*topo(i+1,j) )
     %                   + (1. - frco)*topo(i,j)
            enddo
            a = dx(ni) / ( dx(ni) + dx(ni-1) )
            w1(ni,j) = frco*( a*topo(ni-1,j) + (1. - a)*topo(1,j) )
     %                 + (1. - frco)*topo(ni,j)
         enddo
      endif
*
**********************************************************************
*     Apply filter along Y
*
      if ( lagrd ) then
         do i=1,ni
            topo(i,1)  = frco*0.5*( w1(i,1) + w1(i,2) )
     %                   + (1. - frco)*w1(i,1)
            do j=2,nj-1
               a = dy(j) / ( dy(j) + dy(j-1) )
               topo(i,j) = frco*( a*w1(i,j-1) + (1. - a)*w1(i,j+1) )
     %                     + (1. - frco)*w1(i,j)
            enddo
            topo(i,nj) = frco*0.5*( w1(i,nj-1) + w1(i,nj) )
     %                   + (1. - frco)*w1(i,nj)
         enddo
*--------------------------------------------------------------------
      else
         b  = dx(1) + dx(ni)
         ms = w1(1,1)  * b
         mn = w1(1,nj) * b
         mb = b
         do i=2,ni
            b  = dx(i) + dx(i-1)
            ms = ms + w1(i,1)  * b
            mn = mn + w1(i,nj) * b
            mb = mb + b
         enddo
         ms = ms/mb
         mn = mn/mb
         do i=1,ni
            topo(i,1)  = frco*0.5*( ms + w1(i,2) )
     %                   + (1. - frco)*w1(i,1)
            do j=2,nj-1
               a = dy(j) / ( dy(j) + dy(j-1) )
               topo(i,j) = frco*( a*w1(i,j-1) + (1. - a)*w1(i,j+1) )
     %                     + (1. - frco)*w1(i,j)
            enddo
            topo(i,nj) = frco*0.5*( mn + w1(i,nj-1) )
     %                   + (1. - frco)*w1(i,nj)
         enddo
      endif
*
*--------------------------------------------------------------------
*
      return
      end
