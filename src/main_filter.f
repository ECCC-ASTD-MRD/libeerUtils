      subroutine main_filter
      implicit none
c     Declarations for CCARDs
      integer       nkey
      parameter   ( nkey = 6 )
      CHARACTER*8   cles(nkey)
      character*128 nam(nkey),def(nkey)
      integer       npos
c     Files names
      character*128 ifile, ofile
c     External functions 
      integer   fnom,fclos,fstouv,fstprm,fstecr,fstfrm,fstinf
      external  fnom,fclos,fstouv,fstprm,fstecr,fstfrm,fstinf
      integer   fstlir, fstluk, wkoffit, remove_c
      external  fstlir, fstluk, wkoffit, remove_c, ccard
      integer   longueur
      external  longueur
      external  smp_grid_spc,smp_fltp_gem,smp_digt_flt,smp_coef_dgt
      external  smp_2del_flt
c
      integer   key, ier, ni, nj, nk, ip1, ip2, ip3, ig1, ig2, ig3, ig4
      integer   dateo, datyp, deet, nbits, npas, swa, lng, dltf, ubc
      integer   extra1, extra2, extra3
      character etiket*12, nomvar*4, typvar*2, grtyp*1
      integer   iun, oun, npak
      integer   handle, ftyp, nifi, njfi, ig1fi, ig2fi
      logical   LAM,Topo_filmx_L,Topo_dgfmx_L,Topo_dgfms_L
      REAL,   dimension (:,:),     allocatable :: topoin,work
      REAL,   dimension (:),       allocatable :: xfi,yfi
      DATA cles /'S.','D.','TYP.','FILMX.','DGFMX.','DGFMS.'/
      DATA nam  /'  ','  ','GU+' ,'false','false','false'/
      DATA def  /'  ','  ','GU+' ,'true','true','true'/
*
c     Read input parameters
      npos = -1
      CALL CCARD(CLES,DEF,NAM,nkey,npos)
c
      if ( nam(3) == 'LU' ) then
        LAM = .true.
      else
        LAM = .false.
      end if
      Topo_filmx_L = ( nam(4) == 'true' )
      Topo_dgfmx_L = ( nam(5) == 'true' )
      Topo_dgfms_L = ( nam(6) == 'true' )
c     Open all input files
c     ====================
      ifile = nam(1)
      if (ifile .ne. '  ') then
c       Open input file
        iun = 10
        ier = fnom(iun, ifile, 'STD+RND+R/O', 0)
c       Check file type of input file
        ftyp = WKOFFIT(ifile)
        if (ftyp.ne.1.and.ftyp.ne.2.and.ftyp.ne.33.and.ftyp.ne.34) then
          print *,'Wrong input file type, ABORT!'
          stop
        end if
        ier = fstouv(iun,'STD+RND')
      else
        print *,'Please define input file'
        stop
      end if
c     Open output file
c     ================
      if (nam(2) .ne. '  ') then
        ofile = nam(2)
      else
        ofile = ifile(1:longueur(ifile)) // '_filtered'
      end if
      ier = wkoffit(ofile)
      if (ier.gt.0) then
        print *,'REMOVE OUTPUT FILE ' // ofile
        ier = remove_c(ofile)
      endif
      oun = 11
      ier = fnom(oun, ofile, 'STD+RND', 0)
      ier= fstouv(oun, 'STD+RND')
      if (ier.lt.0) then
         print *, 'Fatal error while opening the file (FNOM)'
         stop
      endif
c     Read non-filtered ME
c     ------------------------
      handle = fstinf(iun,ni,nj,nk,-1,' ',1200,-1,-1,' ','ME')
      ier = fstprm(handle,dateo,deet,npas,ni,nj,nk,nbits,datyp,ip1,
     1        ip2,ip3,typvar,nomvar,etiket,grtyp,ig1,ig2,ig3,ig4,swa,
     2        lng,dltf,ubc,extra1,extra2,extra3)
      nifi = ni
      njfi = nj
      ig1fi = ig1
      ig2fi = ig2
c
c     Read tic-tacs
c     ------------------------
      allocate (xfi(nifi),work(nifi,1))
      handle = fstlir(xfi,iun,ni,nj,nk,-1,' ',ig1fi,ig2fi,-1,' ','>>')
      ier = fstprm(handle,dateo,deet,npas,ni,nj,nk,nbits,datyp,ip1,
     1        ip2,ip3,typvar,nomvar,etiket,grtyp,ig1,ig2,ig3,ig4,swa,
     2        lng,dltf,ubc,extra1,extra2,extra3)
      ier = fstecr(xfi, work, -nbits, oun, dateo, deet, npas, ni,
     1             nj,nk, ip1, ip2, ip3, typvar, nomvar, etiket, grtyp,
     2             ig1, ig2, ig3, ig4, datyp, .true.)
      deallocate (work)
      allocate (yfi(njfi),work(1,njfi))
      handle = fstlir(yfi,iun,ni,nj,nk,-1,' ',ig1fi,ig2fi,-1,' ','^^')
      ier = fstprm(handle,dateo,deet,npas,ni,nj,nk,nbits,datyp,ip1,
     1        ip2,ip3,typvar,nomvar,etiket,grtyp,ig1,ig2,ig3,ig4,swa,
     2        lng,dltf,ubc,extra1,extra2,extra3)
      ier = fstecr(yfi, work, -nbits, oun, dateo, deet, npas, ni,
     1             nj,nk, ip1, ip2, ip3, typvar, nomvar, etiket, grtyp,
     2             ig1, ig2, ig3, ig4, datyp, .true.)
      deallocate (work)
      allocate (topoin(nifi,njfi),work(nifi,njfi))
      handle = fstlir(topoin,iun,ni,nj,nk,-1,' ',1200,-1,-1,' ','ME')
      ier = fstprm(handle,dateo,deet,npas,ni,nj,nk,nbits,datyp,ip1,
     1        ip2,ip3,typvar,nomvar,etiket,grtyp,ig1,ig2,ig3,ig4,swa,
     2        lng,dltf,ubc,extra1,extra2,extra3)
      ier = fstecr(topoin, work, -nbits, oun, dateo, deet, npas, ni,
     1             nj,nk, ip1, ip2, ip3, typvar, nomvar, etiket, grtyp,
     2             ig1, ig2, ig3, ig4, datyp, .true.)
      print *, 'ME(103,79) = 28.9423 ? ', topoin(103,79)
c
      call smp_fltp_gem(topoin, xfi, yfi, nifi, njfi,
     %                  LAM, 'G',
     %                  Topo_dgfmx_L, 5, 2.0, 3.0, Topo_dgfms_L, .true.,
     %                  Topo_filmx_L, 0.5)
      print *, 'ME(103,79) = 28.9423 ? ', topoin(103,79)
      ier = fstecr(topoin, work, -nbits, oun, dateo, deet, npas, ni,
     1             nj,nk, ip1, ip2, ip3, typvar, 'MEF', etiket, grtyp,
     2             ig1, ig2, ig3, ig4, datyp, .true.)
c
      deallocate (topoin,xfi,yfi,work)
      ier = fstfrm(iun)
      ier = fclos (iun)
      ier = fstfrm(oun)
      ier = fclos (oun)
      end
