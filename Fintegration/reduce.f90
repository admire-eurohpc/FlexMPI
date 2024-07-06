      program gather

      use iso_c_binding

      include 'mpif.h'      

      !********** APP params **********
      integer irecv(40), rank, procs, lasprocs, vsize, i, vini, vend
      integer n, maxiters, pos, itmax
      integer isend(40) 

      !********* FMPI params **********
      integer :: counter,idapp
      character(len=128) :: argv
      integer :: ierror, res
      character* (128) mpi_name
      integer :: resultlen
      integer :: despl, counts
      integer :: proctype
      integer :: COMM, COMM2
      type(c_ptr) :: comm_ptr
      integer :: it

      interface
    ! Intentionally returning integer and not integer(c_int).
    ! `c_handle` is a pointer to a C comm, not a C comm itself!
    ! We cannot be sure what Fortran type a C comm is!
        integer function MPI_Comm_c2f(c_handle) bind(C, name="f_mpi_comm_c2f_")
          use iso_c_binding
          type(c_ptr), value :: c_handle
        end function
      end interface

      !********** PARSE ARGS TO FILE FOR FLEXMPI ************
      idapp=3 !gather
      open(unit=1,file='/tmp/gather.args',status='replace', form='formatted')
      counter=0
      do 
        call get_command_argument(counter, argv)
        if (len_trim(argv) == 0) EXIT
        write(1,'(g0)') trim(argv) !print *, "Cmd line: ",trim(argv)
        counter = counter + 1
      end do
      close(1)

      !********** FLEXMPI INITIALIZATION **************
      CALL FMPI_INIT2(ierror, counter, idapp)
      !CALL FMPI_GET_COMM(COMM)
      COMM2 = MPI_Comm_c2f(comm_ptr) !********C2FCOMM
      !c_comm = F_MPI_COMM_C2F(COMM2) !****** C2FCOMM
      print *, "Comm: ", COMM2
      CALL MPI_COMM_RANK(COMM2,rank,ierror)
      CALL MPI_COMM_SIZE(COMM2,procs,ierror)
      print *, "Rank = ",rank
      print *, "Size = ",procs
      CALL MPI_Get_processor_name (mpi_name, resultlen, ierror)

      ! copy variables to the specific original code variables.
      id=rank
      p=procs

      ! define vars
      maxiters=20
      vsize = 40
      it=0

      !********* DEPENDING ON IF IT IS NATIVE OR SPAWNED *******
      CALL FMPI_GET_TYPE(proctype);
      IF (proctype == 941490) THEN 
          print *, "Native proc"
          it = 1
          ! vec initialization
          if (rank == 0) then
            do n=1,vsize
              isend(n)=0
              !print *, isend(n)
            end do
          end if
          call MPI_Bcast(isend, vsize, MPI_INTEGER, 0, COMM2, ierror)
          call MPI_Barrier(COMM2, ierror)
      ELSE IF (proctype == 938594) THEN
          ! vec initialization
          do n=1,vsize
            isend(n)=0
            !print *, isend(n)
          end do
          CALL FMPI_GET_SHARED(it)
          it = it -1 !sync with native procs
          print *, "Malleable proc since it: ", it
      END IF

      lastprocs = 0 ! force to go inside reconfiguration section in main loop

      ! *********** MAIN LOOP ***********
      do while ( it <= maxiters )

        !************ Reconfigure native procs if there are new spawned processes ************
        if (lastprocs /= procs) then

          !******** Get comm from FlexPMI ********
          COMM2 = MPI_Comm_c2f(comm_ptr) 
          !print *, rank,"- Comm: ", COMM2

          lastprocs = procs
          pos=vsize/procs
          rest=mod(vsize,procs)
          ! Rank 0 processes more data in case of vsize%procs != 0
          if(rank == 0) then
              n=1
              itmax=pos
              vini=n
              vend=itmax
          else
              n=pos*rank !root limit * rank
              itmax=n+pos
              vini=n+1
              vend=itmax
          end if

          !rest of data for the last process
          if (rank == (procs-1)) then
              itmax = itmax + rest
              vend=itmax
          end if
          print *, rank, " access [ ", vini, ", ", vend, " ]"
        end if
        !*********** dynamic reconf end **********

        !*** EMPI MONITOR INIT ****
        CALL FMPI_MONITOR_INIT()

        ! update vector
        do i=vini, vend
            isend(i) = isend(i) + rank
            !print *, rank, " set ", isend(i), " in: ", i
        end do

        call MPI_ALLREDUCE(isend, irecv, vsize, MPI_INTEGER, MPI_SUM, COMM2, ierror)
        isend = irecv 

        ! rank 0 print results
        if (rank == 0) then
            do i=1, vsize
              print *, irecv(i)
            end do
        end if
        it = it +1
        call sleep(2)

        !*** EMPI MONITOR END ****
        call FMPI_MONITOR_END2(rank, procs, it, maxiters, NULL, NULL, counter, idapp)

        !*** FINALIZE REMOVED PROCS
        if (procs == 1) then
            if (rank /= 0) then
                exit
            end if
        end if

      end do

      ! ****** END OF THE APP ********
      if(rank == 0) then
        print *, "Finishing..."
      endif

      !  Terminate MPI.
      !call MPI_Finalize ( ierr )
      CALL FMPI_FINALIZE(ierror)

      end    
