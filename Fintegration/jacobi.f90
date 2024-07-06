program jacobi
    use wrap_init
    use iso_c_binding

    include 'mpif.h'

    integer, parameter :: n=5000
    integer :: i_local,i_global,j,k,ni,s,m,seed
    double precision :: tol,t,t2,sig
    double precision, dimension(:,:), ALLOCATABLE :: A_local
    double precision, dimension(:), ALLOCATABLE :: B_local
    double precision, dimension(:), ALLOCATABLE :: x_local
    double precision, dimension(:), ALLOCATABLE :: x_temp1
    double precision, dimension(:), ALLOCATABLE :: x_old
    double precision, dimension(:), ALLOCATABLE :: x_new
    double precision, dimension(:), ALLOCATABLE :: buff
    integer :: rank,procs,tag, i

    !********* FMPI params **********
    integer :: counter, status
    integer :: ierror
    character* (128) mpi_name
    integer :: proctype
    integer :: ADM_COMM_WORLD
    integer :: it = 0
    integer :: argc
    character(len=600) :: name, argv
    integer :: mon_start = 150520
    integer :: mon_stop = 203459
    integer :: procs_hint = 0
    integer :: excl_node_hint = 0


    !********** PARSE ARGS BY PARAM TO FLEXMPI ************
    argc = COMMAND_ARGUMENT_COUNT()
    counter=0
    do counter = 0, argc
        call GET_COMMAND_ARGUMENT(counter, argv)
        !print *, "Argv: ", trim(argv),getpid()
        call fmpi_buildArgv(trim(argv))
    end do

    !********** FLEXMPI INITIALIZATION **************
    print *, "MPI initializing...",getpid()
    call fmpi_init(ierror)
    print *, "MPI Init done...",getpid()
    
    call adm_GetComm(ADM_COMM_WORLD)
    CALL MPI_COMM_RANK(ADM_COMM_WORLD,rank,ierror)
    CALL MPI_COMM_SIZE(ADM_COMM_WORLD,procs,ierror)
    print *, "Rank = ",rank,getpid()
    print *, "Size = ",procs,getpid()

    ! copy variables to the specific original code variables.
    id=rank
    p=procs

    ni=200
    m=n/procs

    !********* DEPENDING ON IF IT IS NATIVE OR SPAWNED *******
    CALL FMPI_GET_TYPE(proctype);
    IF (proctype == 941490) THEN
        print *, "Native proc",getpid()
        it = 1 !it starts in 1
        ALLOCATE (A_local(0:n-1,0:n-1))
        ALLOCATE (B_local(0:n-1))
        ALLOCATE (x_local(0:n-1))
        ALLOCATE (x_temp1(0:n-1))
        ALLOCATE (x_new(0:n-1))

        seed=time()
        call srand(seed)

        !matrix initialization
        do k=0, n-1
            do j=0, n-1
                A_local(k,j)=rand(0)
                B_local(k)=rand(0)
            end do
        end do

        do i_global = 0, m-1 !despl, counts-1 !0, m-1
            A_local(i_global,i_global) = sum(A_local(i_global,:)) + n
        end do

        CALL MPI_ALLGATHER(B_local, m, MPI_DOUBLE,x_temp1,m, MPI_DOUBLE, ADM_COMM_WORLD, ierror)

        x_new=x_temp1

        call MPI_Barrier(ADM_COMM_WORLD, ierror)
        print *, "JACOBI: Post-Barrier in Native Procs",getpid()

    ELSE IF (proctype == 938594) THEN
        print *, "Spawned proc",getpid()
        ALLOCATE (A_local(0:n-1,0:n-1))
        ALLOCATE (B_local(0:n-1))
        ALLOCATE (x_local(0:n-1))
        ALLOCATE (x_temp1(0:n-1))
        ALLOCATE (x_new(0:n-1))

        seed=time()
        call srand(seed)

        !matrix initialization
        do k=0, n-1
            do j=0, n-1
                A_local(k,j)=rand(0)
                B_local(k)=rand(0)
            end do
        end do

        do i_global = 0, m-1 !despl, counts-1 !0, m-1
            A_local(i_global,i_global) = sum(A_local(i_global,:)) + n
        end do

        x_new=x_temp1

        !CALL FMPI_GET_SHARED(it)
        !it = it+1
        !print *, "JACOBI: Malleable proc since it: ", it,getpid()
    END IF

    print *, "main: ADM_GLOBAL_MAX_ITERATION ", ni,getpid()
    call adm_RegisterSysAttributesInt("ADM_GLOBAL_MAX_ITERATION", ni)

    print *, "main: ADM_GLOBAL_INTERATION ", it,getpid()
    call adm_GetSysAttributesInt("ADM_GLOBAL_ITERATION", it)

    call adm_MonitoringService(mon_start)

    ! force to go inside reconfiguration section in main loop
    lastprocs = 0
    maxiters = ni

    ! *********** MAIN LOOP ***********
    t=mpi_wtime()
    do k=it,ni
        call adm_SyncProcesses(ierror)

        call adm_RegisterSysAttributesInt("ADM_GLOBAL_ITERATION", k);
        print *, "JACOBI main: ADM_GLOBAL_ITERATION ",k,getpid()

        call adm_GetComm(ADM_COMM_WORLD)
        call MPI_COMM_RANK(ADM_COMM_WORLD,rank,ierror)
        call MPI_COMM_SIZE(ADM_COMM_WORLD,procs,ierror)

        it = k
        print *, "JACOBI: Rank ",rank,"/",procs," in main loop. Iter=", it,getpid()

        if (k == 20) then
            print *, "Sending HINT + 2 procs en it = ", k,getpid()
            procs_hint = 2
            excl_node_hint = 1
            call adm_RegisterSysAttributesInt("ADM_GLOBAL_HINT_NUM_PROCESS", procs_hint)
            call adm_RegisterSysAttributesInt("ADM_GLOBAL_HINT_EXCL_NODES", excl_node_hint)
        else if (k == 80) then
            print *, "Sending HINT - 2 procs en it = ", k,getpid()
            procs_hint = -2
            excl_node_hint = 1
            call adm_RegisterSysAttributesInt("ADM_GLOBAL_HINT_NUM_PROCESS", procs_hint)
            call adm_RegisterSysAttributesInt("ADM_GLOBAL_HINT_EXCL_NODES", excl_node_hint)
        else
            print *, "Sending HINT 0 procs en it ", k,getpid()
            procs_hint = 0
            call adm_RegisterSysAttributesInt("ADM_GLOBAL_HINT_NUM_PROCESS", procs_hint)
        end if



        !** Reconfigure native procs if there are new spawned processes 
        if (lastprocs /= procs) then
            print *, "JACOBI: Reconfiguring ",rank,"/",procs," in iteration=",it,getpid()

            call MPI_COMM_RANK(ADM_COMM_WORLD,rank,ierror)
            call MPI_COMM_SIZE(ADM_COMM_WORLD,procs,ierror)
            print *, "JACOBI: RECONF BARRIER rank ",rank,"/",procs,getpid()
            call MPI_BARRIER(ADM_COMM_WORLD, ierror)

            lastprocs = procs
            m=n/procs

            print *, "JACOBI: Rank ", rank,"/",procs," BEFORE ALLGATHER",getpid()
            CALL MPI_ALLGATHER(B_local, m, MPI_DOUBLE,x_temp1,m, MPI_DOUBLE, ADM_COMM_WORLD, ierror)

        end if
        !** dynamic reconf end 

        print *, "JACOBI MALLEABLEREGION START: rank:",rank,"/",procs,getpid()
        call adm_MalleableRegion(mon_start, status)

        x_old=x_new
        do i_local=0, m-1 !despl,counts-1 !0,m-1 -> new distribution
            i_global=i_local+rank*m
            !x_local(i_local)=b_local(i_local)
            s=0
            do j=0,n-1
                if (j/=i_local) then
                    s=s+A_local(i_local,j)*x_old(j)
                endif
            end do
            x_local(i_local)=(B_local(i_local)-s)/A_local(i_local,i_global)
        end do

        CALL MPI_ALLGATHER(x_local,m, MPI_DOUBLE, x_new, m, MPI_DOUBLE, ADM_COMM_WORLD,ierror)

        do j=0,n-1
            sig=(x_new(j)-x_old(j))*(x_new(j)-x_old(j))
            tol=tol+sig
            tol=sqrt(tol)
        end do

        !print *, "x", x_local,getpid()
        !print *, "tol=", tol,getpid()
        !print *, "JACOBI: iter =",k,getpid()

        call adm_MalleableRegion(mon_stop, status)
        !print *, "ADM_MalleableRegion end status = ", status,getpid()
        call fmpi_get_status(status);
        if (status == 1) exit

        print *, "JACOBI: Rank ",rank,"/",procs," finishing iteration ",it,getpid()

        !*** FINALIZE REMOVED PROCS ?
        if (procs == 1) then
            if (rank /= 0) then
                exit
            end if
        end if

        !if (tol<1.01) then
        !    print *, "tol < 1.01 --> proc end.",getpid()
        !    EXIT
        !endif
        if (k==(ni-1)) then
            print *, "Max iterations",getpid()
            EXIT
        endif
    end do
    ! end loop

    t2=mpi_wtime()-t;
    print *, "Elapsed time = ",t2,getpid()

    call adm_MonitoringService(mon_stop)

    CALL FMPI_FINALIZE(ierror)

    print *, "End process",getpid()


end program jacobi
