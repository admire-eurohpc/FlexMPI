program main

!*****************************************************************************80
!
!! MAIN is the main program for PRIME_MPI.
!
!  Discussion:
!
!    This program calls a version of PRIME_NUMBER that includes
!    MPI calls for parallel processing.
!
!  Licensing:
!
!    This code is distributed under the GNU LGPL license. 
!
!  Modified:
!
!    07 August 2009
!
!  Author:
!
!    John Burkardt
!
  !use mpi
  include 'mpif.h'

  integer ( kind = 4 ) i
  integer ( kind = 4 ) id
  integer ( kind = 4 ) ierr
  integer ( kind = 4 ) n
  integer ( kind = 4 ) n_factor
  integer ( kind = 4 ) n_hi
  integer ( kind = 4 ) n_lo
  integer ( kind = 4 ) p
  integer ( kind = 4 ) primes
  integer ( kind = 4 ) primes_part
  real ( kind = 8 ) wtime
  
  integer :: rank,procs,tag,ierror, res
  integer :: counter,idapp
  character(len=128) :: argv
  character* (128) mpi_name
  integer :: resultlen
  integer :: despl, counts
  integer :: proctype
  integer :: COMM
  integer :: it

  n_lo = 1
  n_hi = 8192
  n_factor = 2 

!PARSE ARGS TO FILE FOR FLEXMPI
  idapp=2 !Primes
  open(unit=1,file='/tmp/primes.args',status='replace', form='formatted')
  counter=0
  do 
    call get_command_argument(counter, argv)
    if (len_trim(argv) == 0) EXIT
    write(1,'(g0)') trim(argv) !print *, "Cmd line: ",trim(argv)
    counter = counter + 1
  end do
  close(1)

  ! FLEXMPI INITIALIZATION 
  CALL FMPI_INIT2(ierror, counter, idapp)
  CALL FMPI_GET_COMM(COMM)
  CALL MPI_COMM_RANK(COMM,rank,ierror)
  CALL MPI_COMM_SIZE(COMM,procs,ierror)
  print *, "Rank = ",rank
  print *, "Size = ",procs
  id=rank
  p=procs

  CALL MPI_Get_processor_name (mpi_name, resultlen, ierror);

  !Get proc type and barrier. Let developers redistribute de data
  CALL FMPI_GET_TYPE(proctype);
  IF (proctype == 941490) THEN !error accessing global empi vars
      print *, "Native proc"
      n = n_lo
      CALL MPI_Barrier(COMM, ierror)
  ELSE IF (proctype == 938594) THEN
      print *, "Malleable proc"
      CALL FMPI_GET_SHARED(it)
      n=it
  END IF


  if ( id == 0 ) then
    call timestamp ( )
    write ( *, '(a)' ) ' '
    write ( *, '(a)' ) 'PRIME_MPI'
    write ( *, '(a)' ) '  FORTRAN90/MPI version'
    write ( *, '(a)' ) ' '
    write ( *, '(a)' ) '  An MPI example program to count the number of primes.'
    write ( *, '(a,i8)' ) '  The number of processes is ', p
    write ( *, '(a)' ) ' '
    write ( *, '(a)' ) '         N        Pi          Time'
    write ( *, '(a)' ) ' '
  end if

  do while ( n <= n_hi )

    CALL FMPI_MONITOR_INIT()

    if ( id == 0 ) then
      wtime = MPI_Wtime ( )
    end if

    !print *, "Proc ", id, " bcast: ", n
    !call MPI_Bcast ( n, 1, MPI_INTEGER, 0, COMM, ierr )

    call prime_number ( n, id, p, primes_part )

    call MPI_Reduce ( primes_part, primes, 1, MPI_INTEGER, MPI_SUM, 0, &
      COMM, ierr )

    if ( id == 0 ) then
      wtime = MPI_Wtime ( ) - wtime
      write ( *, '(2x,i8,2x,i8,g14.6)' ) n, primes, wtime
    end if

    n = n * n_factor

    CALL sleep(2)
    CALL FMPI_MONITOR_END2(rank, procs, n, n_hi, NULL, NULL, counter, idapp)

  end do

!  Terminate MPI.
  !call MPI_Finalize ( ierr )
  CALL FMPI_FINALIZE(ierror)


  if ( id == 0 ) then
    write ( *, '(a)' ) ' '
    write ( *, '(a)' ) 'PRIME_MPI:'
    write ( *, '(a)' ) '  Normal end of execution.'
    write ( *, '(a)' ) ' '
    call timestamp ( )
  end if

  stop
end


subroutine prime_number ( n, id, p, total )
!  Parameters:
!
!    Input, integer ( kind = 4 ) N, the maximum number to check.
!
!    Input, integer ( kind = 4 ) ID, the ID of this process,
!    between 0 and P-1.
!
!    Input, integer ( kind = 4 ) P, the number of processes.
!
!    Output, integer ( kind = 4 ) TOTAL, the number of prime numbers up to N,
!    starting at 2+ID and skipping by P.
!
  implicit none

  integer ( kind = 4 ) i
  integer ( kind = 4 ) id
  integer ( kind = 4 ) j
  integer ( kind = 4 ) n
  integer ( kind = 4 ) p
  integer ( kind = 4 ) prime
  integer ( kind = 4 ) total

  total = 0

  do i = 2+id, n, p

    prime = 1

    do j = 2, i - 1
      if ( mod ( i, j ) == 0 ) then
        prime = 0
        exit
      end if
    end do

    total = total + prime

  end do

  return
end



subroutine timestamp ( )
  implicit none

  character ( len = 8 ) ampm
  integer ( kind = 4 ) d
  integer ( kind = 4 ) h
  integer ( kind = 4 ) m
  integer ( kind = 4 ) mm
  character ( len = 9 ), parameter, dimension(12) :: month = (/ &
    'January  ', 'February ', 'March    ', 'April    ', &
    'May      ', 'June     ', 'July     ', 'August   ', &
    'September', 'October  ', 'November ', 'December ' /)
  integer ( kind = 4 ) n
  integer ( kind = 4 ) s
  integer ( kind = 4 ) values(8)
  integer ( kind = 4 ) y

  call date_and_time ( values = values )

  y = values(1)
  m = values(2)
  d = values(3)
  h = values(5)
  n = values(6)
  s = values(7)
  mm = values(8)

  if ( h < 12 ) then
    ampm = 'AM'
  else if ( h == 12 ) then
    if ( n == 0 .and. s == 0 ) then
      ampm = 'Noon'
    else
      ampm = 'PM'
    end if
  else
    h = h - 12
    if ( h < 12 ) then
      ampm = 'PM'
    else if ( h == 12 ) then
      if ( n == 0 .and. s == 0 ) then
        ampm = 'Midnight'
      else
        ampm = 'AM'
      end if
    end if
  end if

  write ( *, '(i2,1x,a,1x,i4,2x,i2,a1,i2.2,a1,i2.2,a1,i3.3,1x,a)' ) &
    d, trim ( month(m) ), y, h, ':', n, ':', s, '.', mm, trim ( ampm )

  return
end
