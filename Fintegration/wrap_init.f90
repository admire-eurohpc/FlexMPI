! wrap_init.f90
module wrap_init
    implicit none

    integer, parameter, public :: ADM_SERVICE_START = 150520
    integer, parameter, public :: ADM_SERVICE_STOP = 203459
    integer, parameter, public :: ADM_NATIVE = 941490
    integer, parameter, public :: ADM_SPAWNED = 938594
    integer, parameter, public :: ADM_ACTIVE = 776765
    integer, parameter, public :: ADM_REMOVED = 787634

    private
    public :: fmpi_init
    public :: fmpi_buildArgv
    public :: adm_MonitoringService
    public :: adm_MalleableRegion
    public :: adm_RegisterSysAttributesInt
    public :: adm_RegisterSysAttributesIntP
    public :: adm_RegisterSysAttributesIntArr
    public :: adm_RegisterSysAttributesDouble
    public :: adm_RegisterSysAttributesDoubleArr
    public :: adm_RegisterSysAttributesStr
    public :: adm_GetSysAttributesInt
    public :: adm_GetSysAttributesIntArr
    public :: adm_GetSysAttributesDouble
    public :: adm_GetSysAttributesDoubleArr
    public :: adm_GetSysAttributesStr
    public :: adm_SyncProcesses
    public :: adm_GetComm
    public :: adm_CheckpointConfirmation
    public :: adm_DoCheckpoint
    public :: adm_Restart
    public :: adm_ICregistration
    public :: adm_malleability
    public :: adm_SetReconfigData
    public :: adm_IccFini

    !****************** INTERFACES

    ! Set reconfig data from the application. 
    interface
        subroutine c_adm_iccfini() bind(C, name="ADM_IccFini_")
            use iso_c_binding
        end subroutine c_adm_iccfini
    end interface

    ! Set reconfig data from the application. 
    interface
        subroutine c_adm_SetReconfigData(nprocs, excl_nodes) bind(C, name="ADM_SetReconfigData_")
            use iso_c_binding
            integer :: nprocs, excl_nodes
        end subroutine c_adm_SetReconfigData
    end interface

    ! Register app on IC
    interface
        subroutine c_adm_ICregistration(err) bind(C, name="ADM_ICregistration_")
            use iso_c_binding
            integer :: err
        end subroutine c_adm_ICregistration
    end interface

    ! Execute againt an mpirun with the application
    interface
        subroutine c_adm_Restart(err) bind(C, name="ADM_Restart_")
            use iso_c_binding
            integer :: err
        end subroutine c_adm_Restart
    end interface

    ! Ask if processes should execute the checkpointing
    interface
        subroutine c_adm_DoCheckpoint(err) bind(C, name="ADM_DoCheckpoint_")
            use iso_c_binding
            integer :: err
        end subroutine c_adm_DoCheckpoint
    end interface
    
    ! Interface that queries IC if the app has to do a checkpoint
    interface
        subroutine c_adm_CheckpointConfirmation(err) bind(C, name="ADM_CheckpointConfirmation_")
            use iso_c_binding
            integer :: err
        end subroutine c_adm_CheckpointConfirmation
    end interface

    ! Ask IC if the app has to do a checkpoint and new malleability values
    interface
        subroutine c_adm_Malleability(decision, nnodes, hostlist) bind(C, name="ADM_Malleability_")
            use iso_c_binding
            character(kind=c_char), dimension(*) :: hostlist
            integer :: nnodes
            integer :: decision
        end subroutine c_adm_Malleability
    end interface

    ! SYNC FORTRAN PROCESSES IN C SIDE
    interface
        subroutine c_adm_SyncProcesses(err) bind(C, name="ADM_SyncProcesses_")
            use iso_c_binding
            integer :: err
        end subroutine c_adm_SyncProcesses
    end interface


    ! WRAP FMPI_INIT WITH ARGUMENTS PASSED BY FILE (DEPRECATED)
    interface
        subroutine c_fmpi_init(err) bind(C, name="fmpi_init_")
            use iso_c_binding, only: c_char
            integer :: err
        end subroutine c_fmpi_init
    end interface

    ! WRAP FMPI_FINALIZE
    interface
        subroutine c_fmpi_finalize(err) bind(C, name="fmpi_finalize_")
            use iso_c_binding, only: c_char
            integer :: err
        end subroutine c_fmpi_finalize
    end interface
    

    ! SEND ARGUMENTS FROM FORTRAN TO C TO BUILD THE ARGV FOR FLEXMPI
    interface
        subroutine c_fmpi_buildArgv(name) bind(C, name="fmpi_buildArgv_")
            use iso_c_binding, only: c_char
            character(kind=c_char), dimension(*) :: name
        end subroutine c_fmpi_buildArgv
    end interface


    ! WRAP ADM_MonitoringService FROM FLEXMPI
    interface
        subroutine c_adm_MonitoringService(cmd) bind(C, name="ADM_MonitoringService_")
            use iso_c_binding
            integer :: cmd
        end subroutine c_adm_MonitoringService
    end interface


    ! WRAP ADM_MallebleRegion FROM FLEXMPI
    interface
        subroutine c_adm_MalleableRegion(cmd, ret) bind(C, name="ADM_MalleableRegion_")
            use iso_c_binding
            integer :: cmd
            integer :: ret
        end subroutine c_adm_MalleableRegion
    end interface


    ! WRAP ADM_RegisterSysAttributes WITH INTEGER [] AS ARGUMENT
    interface
        subroutine c_adm_RegisterSysAttributesIntArr(key,val,size) bind(C, name="ADM_RegisterSysAttributesIntArr_")
            use iso_c_binding, only: c_char, c_int
            character(kind=c_char), dimension(*) :: key
            integer(kind=c_int), dimension(*) :: val
            integer :: size
        end subroutine c_adm_RegisterSysAttributesIntArr
    end interface


    ! WRAP ADM_RegisterSysAttributes WITH DOUBLE [] AS ARGUMENT
    interface
        subroutine c_adm_RegisterSysAttributesDoubleArr(key,val,size) bind(C, name="ADM_RegisterSysAttributesDoubleArr_")
            use iso_c_binding, only: c_char, c_double
            character(kind=c_char), dimension(*) :: key
            real(kind=c_double), dimension(*) :: val
            integer :: size
        end subroutine c_adm_RegisterSysAttributesDoubleArr
    end interface


    ! WRAP ADM_RegisterSysAttributes WITH INTEGER AS ARGUMENT
    interface
        subroutine c_adm_RegisterSysAttributesInt(key, val) bind(C, name="ADM_RegisterSysAttributesInt_")
            use iso_c_binding, only: c_char, c_int
            character(kind=c_char), dimension(*) :: key
            integer(kind=c_int) :: val
        end subroutine c_adm_RegisterSysAttributesInt
    end interface

    ! WRAP ADM_RegisterSysAttributes WITH INTEGER* AS ARGUMENT
    interface
        subroutine c_adm_RegisterSysAttributesIntP(key, val) bind(C, name="ADM_RegisterSysAttributesInt_")
            use iso_c_binding, only: c_char, c_int
            character(kind=c_char), dimension(*) :: key
            integer(kind=c_int), intent(inout) :: val
        end subroutine c_adm_RegisterSysAttributesIntP
    end interface


    ! WRAP ADM_RegisterSysAttributes WITH FLOAT AS ARGUMENT
    interface
        subroutine c_adm_RegisterSysAttributesDouble(key, val) bind(C, name="ADM_RegisterSysAttributesDouble_")
            use iso_c_binding, only: c_char, c_double
            character(kind=c_char), dimension(*) :: key
            real(kind=c_double) :: val
        end subroutine c_adm_RegisterSysAttributesDouble
    end interface


    ! WRAP ADM_RegisterSysAttributes WITH STRING AS ARGUMENT
    interface
        subroutine c_adm_RegisterSysAttributesStr(key, val, size) bind(C, name="ADM_RegisterSysAttributesStr_")
            use iso_c_binding, only: c_char
            character(kind=c_char), dimension(*) :: key
            character(kind=c_char), dimension(*) :: val
            integer :: size
        end subroutine c_adm_RegisterSysAttributesStr
    end interface


    ! WRAP ADM_GetSysAttributes WITH INTEGER [] AS ARGUMENT
    interface
        subroutine c_adm_GetSysAttributesIntArr(key,val,size,ret) bind(C, name="ADM_GetSysAttributesIntArr_")
            use iso_c_binding, only: c_char, c_int
            character(kind=c_char), dimension(*) :: key
            integer(kind=c_int), dimension(*) :: val
            integer :: size
            integer :: ret
        end subroutine c_adm_GetSysAttributesIntArr
    end interface


    ! WRAP ADM_GetSysAttributes WITH DOUBLE [] AS ARGUMENT
    interface
        subroutine c_adm_GetSysAttributesDoubleArr(key,val,size, ret) bind(C, name="ADM_GetSysAttributesDoubleArr_")
            use iso_c_binding, only: c_char, c_double
            character(kind=c_char), dimension(*) :: key
            real(kind=c_double), dimension(*) :: val
            integer :: size
            integer :: ret
        end subroutine c_adm_GetSysAttributesDoubleArr
    end interface


    ! WRAP ADM_GetSysAttributes WITH INTEGER AS ARGUMENT
    interface
        subroutine c_adm_GetSysAttributesInt(key, val) bind(C, name="ADM_GetSysAttributesInt_")
            use iso_c_binding, only: c_char, c_int
            character(kind=c_char), dimension(*) :: key
            integer(kind=c_int) :: val
        end subroutine c_adm_GetSysAttributesInt
    end interface


    ! WRAP ADM_GetSysAttributes WITH FLOAT AS ARGUMENT
    interface
        subroutine c_adm_GetSysAttributesDouble(key, val) bind(C, name="ADM_GetSysAttributesDouble_")
            use iso_c_binding, only: c_char, c_double
            character(kind=c_char), dimension(*) :: key
            real(kind=c_double) :: val
        end subroutine c_adm_GetSysAttributesDouble
    end interface


    ! WRAP ADM_GetSysAttributes WITH STRING AS ARGUMENT
    interface
        subroutine c_adm_GetSysAttributesStr(key, val, size, ret) bind(C, name="ADM_GetSysAttributesStr_")
            use iso_c_binding, only: c_char
            character(kind=c_char), dimension(*) :: key
            character(kind=c_char), dimension(*) :: val
            integer :: size
            integer :: ret
        end subroutine c_adm_GetSysAttributesStr
    end interface

    ! MPI_Comm from C to Fortran
    interface
        subroutine c_adm_getcomm(c_handle) bind (C, name="f_mpi_comm_c2f_")
            integer :: c_handle
        end subroutine c_adm_getcomm
    end interface


contains

    !********************  SUBROUTINES

    subroutine adm_IccFini()
        use iso_c_binding
        implicit none

        call c_adm_iccfini()
    end subroutine adm_IccFini    

    subroutine adm_SetReconfigData(nprocs, excl_nodes)
        use iso_c_binding
        implicit none
        integer :: nprocs, excl_nodes

        call c_adm_SetReconfigData(nprocs, excl_nodes)
    end subroutine adm_SetReconfigData

    subroutine adm_GetComm(handle)
        use iso_c_binding
        implicit none
        integer :: handle

        call c_adm_getcomm(handle)
    end subroutine adm_GetComm

    subroutine adm_ICregistration(err)
        use iso_c_binding
        implicit none
        integer :: err

        call c_adm_ICregistration(err)
    end subroutine adm_ICregistration

    subroutine adm_Restart(err)
        use iso_c_binding
        implicit none
        integer :: err

        call c_adm_Restart(err)
    end subroutine adm_Restart

    subroutine adm_DoCheckpoint(err)
        use iso_c_binding
        implicit none
        integer :: err

        call c_adm_DoCheckpoint(err)
    end subroutine adm_DoCheckpoint

    subroutine adm_CheckpointConfirmation(err)
        use iso_c_binding
        implicit none
        integer :: err

        call c_adm_CheckpointConfirmation(err)
    end subroutine adm_CheckpointConfirmation

    subroutine adm_Malleability(decision, nnodes, hostlist)
        use iso_c_binding
        implicit none
        character(kind=c_char), dimension(*) :: hostlist
        integer :: nnodes
        integer :: decision

        call c_adm_Malleability(decision, nnodes, hostlist)
    end subroutine adm_Malleability

    ! SYNC FORTRAN PROCESSES IN C SIDE
    subroutine adm_SyncProcesses(err)
        use iso_c_binding
        implicit none
        integer :: err

        call c_adm_SyncProcesses(err)
    end subroutine adm_SyncProcesses


    ! SUBROUTINE TO WRAP RegisterSysAttributes PASSING INT[] AS ARGUMENT
    subroutine adm_RegisterSysAttributesIntArr(key, val, size)
        use iso_c_binding, only: c_char, c_null_char, c_int
        implicit none
        character(kind=c_char), dimension(*) :: key
        integer(kind=c_int), dimension(*) :: val
        integer :: size

        call c_adm_RegisterSysAttributesIntArr(key, val, size)
    end subroutine adm_RegisterSysAttributesIntArr


    ! SUBROUTINE TO WRAP RegisterSysAttributes PASSING INT[] AS ARGUMENT
    subroutine adm_RegisterSysAttributesDoubleArr(key, val, size)
        use iso_c_binding, only: c_char, c_null_char, c_double
        implicit none
        character(kind=c_char), dimension(*) :: key
        real(kind=c_double), dimension(*) :: val
        integer :: size

        call c_adm_RegisterSysAttributesDoubleArr(key, val, size)
    end subroutine adm_RegisterSysAttributesDoubleArr


    ! SUBROUTINE TO WRAP RegisterSysAttributes PASSING STRING AS ARGUMENT
    subroutine adm_RegisterSysAttributesStr(key, val, size)
        use iso_c_binding, only: c_char, c_null_char
        character(len=*), intent(in) :: key
        character(len=*), intent(in) :: val
        integer :: size

        character(kind=c_char), dimension(len(key)+1) :: c_key
        character(kind=c_char), dimension(len(val)+1) :: c_val
        integer :: i

        do i = 1, len(key)
            c_key(i) = key(i:i)
        end do
        c_key(len(key)+1) = c_null_char

        do i = 1, len(val)
            c_val(i) = val(i:i)
        end do
        c_val(len(val)+1) = c_null_char

        call c_adm_RegisterSysAttributesStr(c_key, c_val, len(val)+1)
    end subroutine adm_RegisterSysAttributesStr


    ! SUBROUTINE TO WRAP RegisterSysAttributes PASSING DOUBLE AS ARGUMENT
    subroutine adm_RegisterSysAttributesDouble(key, val)
        use iso_c_binding, only: c_char, c_null_char, c_double
        character(len=*), intent(in) :: key
        real(kind=c_double) :: val

        character(kind=c_char), dimension(len(key)+1) :: c_key
        integer :: i

        do i = 1, len(key)
            c_key(i) = key(i:i)
        end do
        c_key(len(key)+1) = c_null_char

        call c_adm_RegisterSysAttributesDouble(c_key, val)
    end subroutine adm_RegisterSysAttributesDouble

        ! SUBROUTINE TO WRAP RegisterSysAttributes PASSING INT* AS ARGUMENT
    subroutine adm_RegisterSysAttributesIntP(key, val)
        use iso_c_binding, only: c_char, c_null_char, c_int
        character(len=*), intent(in) :: key
        integer(c_int), intent(inout) :: val
        integer :: valint

        character(kind=c_char), dimension(len(key)+1) :: c_key
        integer :: i

        valint = val

        do i = 1, len(key)
            c_key(i) = key(i:i)
        end do
        c_key(len(key)+1) = c_null_char

        call c_adm_RegisterSysAttributesIntP(c_key, valint)
    end subroutine adm_RegisterSysAttributesIntP


    ! SUBROUTINE TO WRAP RegisterSysAttributes PASSING INT AS ARGUMENT
    subroutine adm_RegisterSysAttributesInt(key, val)
        use iso_c_binding, only: c_char, c_null_char, c_int
        character(len=*), intent(in) :: key
        integer(c_int) :: val
        
        character(kind=c_char), dimension(len(key)+1) :: c_key
        integer :: i

        do i = 1, len(key)
            c_key(i) = key(i:i)
        end do
        c_key(len(key)+1) = c_null_char

        call c_adm_RegisterSysAttributesInt(c_key, val)
    end subroutine adm_RegisterSysAttributesInt


    ! SUBROUTINE TO WRAP GetSysAttributes PASSING INT[] AS ARGUMENT
    subroutine adm_GetSysAttributesIntArr(key, val, size, ret)
        use iso_c_binding, only: c_char, c_null_char, c_int
        implicit none
        character(kind=c_char), dimension(*) :: key
        integer(kind=c_int), dimension(*) :: val
        integer :: size
        integer :: ret

        call c_adm_GetSysAttributesIntArr(key, val, size, ret)
    end subroutine adm_GetSysAttributesIntArr


    ! SUBROUTINE TO WRAP GetSysAttributes PASSING INT[] AS ARGUMENT
    subroutine adm_GetSysAttributesDoubleArr(key, val, size, ret)
        use iso_c_binding, only: c_char, c_null_char, c_double
        implicit none
        character(kind=c_char), dimension(*) :: key
        real(kind=c_double), dimension(*) :: val
        integer :: size
        integer :: ret

        call c_adm_GetSysAttributesDoubleArr(key, val, size, ret)
    end subroutine adm_GetSysAttributesDoubleArr


    ! SUBROUTINE TO WRAP GetSysAttributes PASSING STRING AS ARGUMENT
    subroutine adm_GetSysAttributesStr(key, val, size, ret)
        use iso_c_binding, only: c_char, c_null_char
        character(len=*), intent(in) :: key
        character(len=*), intent(in) :: val
        integer :: size
        integer :: ret

        character(kind=c_char), dimension(len(key)+1) :: c_key
        character(kind=c_char), dimension(len(val)+1) :: c_val
        integer :: i

        do i = 1, len(key)
            c_key(i) = key(i:i)
        end do
        c_key(len(key)+1) = c_null_char

        do i = 1, len(val)
            c_val(i) = val(i:i)
        end do
        c_val(len(val)+1) = c_null_char

        call c_adm_GetSysAttributesStr(c_key, c_val, len(val)+1, ret)
    end subroutine adm_GetSysAttributesStr


    ! SUBROUTINE TO WRAP GetSysAttributes PASSING DOUBLE AS ARGUMENT
    subroutine adm_GetSysAttributesDouble(key, val)
        use iso_c_binding, only: c_char, c_null_char, c_double
        character(len=*), intent(in) :: key
        real(kind=c_double) :: val

        character(kind=c_char), dimension(len(key)+1) :: c_key
        integer :: i

        do i = 1, len(key)
            c_key(i) = key(i:i)
        end do
        c_key(len(key)+1) = c_null_char

        call c_adm_GetSysAttributesDouble(c_key, val)
    end subroutine adm_GetSysAttributesDouble


    ! SUBROUTINE TO WRAP RegisterSysAttributes PASSING INT AS ARGUMENT
    subroutine adm_GetSysAttributesInt(key, val)
        use iso_c_binding, only: c_char, c_null_char, c_int
        character(len=*), intent(in) :: key
        integer :: val

        character(kind=c_char), dimension(len(key)+1) :: c_key
        integer :: i

        do i = 1, len(key)
            c_key(i) = key(i:i)
        end do
        c_key(len(key)+1) = c_null_char

        call c_adm_GetSysAttributesInt(c_key, val)
    end subroutine adm_GetSysAttributesInt



    ! SUBROUTINE TO WRAP MalleableRegion
    subroutine adm_MalleableRegion(cmd, ret)
        use iso_c_binding
        integer :: cmd
        integer :: ret

        call c_adm_MalleableRegion(cmd, ret)
    end subroutine adm_MalleableRegion


    ! SUBROUTINE TO WRAP MonitoringService
    subroutine adm_MonitoringService(cmd)
        use iso_c_binding
        integer cmd

        call c_adm_MonitoringService(cmd)
    end subroutine adm_MonitoringService


    ! SUBROUTINE TO WRAP FLEXMPI_INIT USING FILES TO PASS ARGS
    subroutine fmpi_init(err)
        use iso_c_binding
        integer :: err

        call c_fmpi_init(err)
    end subroutine fmpi_init

    ! SUBROUTINE TO WRAP FLEXMPI_FINALIZE
    subroutine fmpi_finalize(err)
        use iso_c_binding
        integer :: err

        call c_fmpi_finalize(err)
    end subroutine fmpi_finalize

    ! SUBROUTINE TO BUILD THE ARGV IN FLEXMPI
    subroutine fmpi_buildArgv(name)
        use iso_c_binding, only: c_char, c_null_char
        character(len=*), intent(in) :: name

        character(kind=c_char), dimension(len(name)+1) :: c_name
        integer :: i

        do i = 1, len(name)
            c_name(i) = name(i:i)
        end do
        c_name(len(name)+1) = c_null_char

        call c_fmpi_buildArgv(c_name)
    end subroutine fmpi_buildArgv


end module wrap_init
