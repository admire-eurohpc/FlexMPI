      program gather

      use iso_c_binding

      include 'mpif.h'      

      !********** APP params **********
      integer irecv(80), rank, procs
      integer n, pos, mpi_size
      integer isend(20) 

      !********* FMPI params **********
      integer :: counter,idapp, status, lasprocs
      integer :: vsize, rsize, i, vini, vend
      character(len=128) :: argv
      integer :: ierror, res
      character* (128) mpi_name
      integer :: resultlen
      integer :: despl, counts
      integer :: proctype
      integer :: COMM, COMM2
      type(c_ptr) :: comm_ptr
      integer :: it, itmax, maxiters

      interface
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
      call FMPI_SET_COMM(COMM2)
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
      vsize = 20
      rsize = 80
      it=0

      !********* DEPENDING ON IF IT IS NATIVE OR SPAWNED *******
      CALL FMPI_GET_TYPE(proctype);
      IF (proctype == 941490) THEN 
          print *, "Native proc"
          it = 1 !it starts in 1
          ! vec initialization
          if (rank == 0) then
            do n=1,vsize
              isend(n)=0
            end do
            do n=1,rsize
              irecv(n)=0
            end do
          end if
          call MPI_Bcast(isend, vsize, MPI_INTEGER, 0, COMM2, ierror)
          call MPI_Barrier(COMM2, ierror)
      ELSE IF (proctype == 938594) THEN
          ! vec initialization
          do n=1,vsize
            isend(n)=0
          end do
          do n=1,rsize
              irecv(n)=0
          end do     
          CALL FMPI_GET_SHARED(it)
          it = it -1 !It seems it goes one iteration in advance
          !call sleep(1)
          print *, "Malleable proc since it: ", it
      END IF

      lastprocs = 0 ! force to go inside reconfiguration section in main loop

      ! *********** MAIN LOOP ***********
      do while ( it <= maxiters )
        print *, "RANK ",rank," in main loop. Iter=", it
        !************ Reconfigure native procs if there are new spawned processes ************
        if (lastprocs /= procs) then

          !******** Get comm from FlexPMI ********
          COMM2 = MPI_Comm_c2f(comm_ptr) 
          !print *, rank,"- Comm: ", COMM2

          lastprocs = procs
          vini = 1
          vend = vsize
        end if
        !*********** dynamic reconf end **********

        !*** EMPI MONITOR INIT ****
        CALL FMPI_MONITOR_INIT()

        ! update vector
        do i=vini, vend
            isend(i) = isend(i) + rank
            !print *, rank, " set ", isend(i), " in: ", i
        end do

        !print *, "[",rank,"] in all gather"
        !call MPI_ALLGATHER(isend, vsize, MPI_INTEGER, irecv, vsize, MPI_INTEGER, COMM2, ierror)
        call MPI_GATHER(isend, vsize, MPI_INTEGER, irecv, vsize, MPI_INTEGER, 0, COMM2, ierror)

        ! rank 0 print results
        if (rank == 0) then
            do i=1, rsize
              !print *, irecv(i)
            end do
        end if
        it = it +1
        call sleep(2)

        !*** EMPI MONITOR END ****
        call FMPI_MONITOR_END2(rank, procs, it, maxiters, NULL, NULL, counter, idapp)
        print *, "[",rank,"] outside the Monitor end"

        !*** FINALIZE REMOVED PROCS ?
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
