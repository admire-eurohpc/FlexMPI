        program jacobi
        use test_mod
        use mpi
implicit none

integer, parameter :: n=1000
integer :: i_local,i_global,j,k,ni,s,m,seed
double precision :: tol,t,t2,sig
double precision, dimension(:,:), ALLOCATABLE :: A_local
double precision, dimension(:), ALLOCATABLE :: B_local, x_local, x_temp1,x_old,x_new, buff
INTEGER, DIMENSION (MPI_STATUS_SIZE) :: STATUS
integer :: rank,procs,tag,ierror
integer :: COMM, argc, counter
character(len=600) :: argv

!**** ARGV***
argc = COMMAND_ARGUMENT_COUNT()
counter=0
do counter = 0, argc
  call GET_COMMAND_ARGUMENT(counter, argv)
  !print *, "Argv: ", trim(argv)
  call fmpi_buildArgv(trim(argv))
end do


!CALL MPI_INIT(ierror)
call fmpi_init(ierror)

call MPI_Comm_c2f(COMM)
CALL MPI_COMM_RANK(COMM,rank,ierror)
CALL MPI_COMM_SIZE(COMM,procs,ierror)

ni=1000
m=n/procs

ALLOCATE (A_local(0:n-1,0:n-1))
ALLOCATE (B_local(0:n-1))
ALLOCATE (x_local(0:n-1))
ALLOCATE (x_temp1(0:n-1))
ALLOCATE (x_new(0:n-1))

!A_local=23
!B_local=47

seed=time()
call srand(seed)

do k=0, n-1
 do j=0, n-1
    A_local(k,j)=rand(0)
    B_local(k)=rand(0)
 end do
end do

do i_global = 0, m-1
 A_local(i_global,i_global) = sum(A_local(i_global,:)) + n
enddo

CALL MPI_ALLGATHER(B_local, m, MPI_DOUBLE, x_temp1, m, MPI_DOUBLE, COMM,ierror)

x_new=x_temp1

!print *, "a", A_local
!print *, "b", B_local


t=mpi_wtime()
do k=1,ni
 x_old=x_new
 do i_local=0,m-1
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
 CALL MPI_ALLGATHER(x_local,m, MPI_DOUBLE, x_new, m, MPI_DOUBLE, COMM,ierror)
 do j=0,n-1
   sig=(x_new(j)-x_old(j))*(x_new(j)-x_old(j))
   tol=tol+sig
   tol=sqrt(tol)
 end do

 !print *, "x", x_local

 !print *, "tol=", tol

 print *, "iter =",k

 if (tol<1.01) EXIT
 if (k==(ni-1)) then
    print *, "Numero Maximo de Iteracoes" 
    EXIT
 endif
end do

 t2=mpi_wtime()-t;
 print *, "t=",t2

CALL MPI_FINALIZE(ierror)
end
