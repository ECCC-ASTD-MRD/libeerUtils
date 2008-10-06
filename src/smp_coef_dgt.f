***s/r smp_coef_dgt - Simplified calculation of coefficients for 
*                     digital filter of topography field
*                     (based on GEM's e_coefdgf)
*                     
*
      subroutine smp_coef_dgt (c, pt, r, norm)
*
      implicit none
*
      integer     pt
      real*8      c(pt+1), r
      logical     norm
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
* c(n)         | output: coefficients, n = 1, ..., pt+1 
*              |
*--------------|------------------------------------------------------
* pt           | input: 2*pt = number of neighbour points
*              |        to be used by the digital filter
*              |        ( pt >= 1)
*              |
* r            | input: cut-off wavelength 
*              |
* norm         | input: if .true. then normalize coefficients 
*              |        such that
*              |          c(1) + 2*c(2) +...+ 2*c(pt+1) = 1
*______________|______________________________________________________
*
      integer n
      real*8  tot, k1, k2, pi_8
*
**********************************************************************
*
      pi_8 = acos( -1.0 )
*
*     compute normalized coefficients
*
      c(1) = 2.0 / r
      tot = c(1)
      do n = 1, pt
         k1     = real(n) * pi_8 / real(pt + 1)
         k2     = real(n) * pi_8 / r
         c(n+1) = c(1) * ( sin(k1) / k1 ) * ( sin(k2) / k2 )
         tot    = tot + 2.0 * c(n+1)
      enddo
*      
      if (norm) then
         do n = 1, pt+1
            c(n) = c(n) / tot
         enddo
      endif
*
*------------------------------------------------------------------
*
      return
      end
