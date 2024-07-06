     ! test_mod.f90
      module test_mod
      implicit none

      private
      public :: MPI_Comm_c2f
      public :: fmpi_init
      public :: fmpi_buildArgv

      !****************** INTERFACES 




      interface
        subroutine c_MPI_Comm_c2f(c_handle) bind (C, name="f_mpi_comm_c2f_")
         !"
         integer :: c_handle
        end subroutine
      end interface

      interface
        subroutine c_fmpi_init(err) bind(C, name="fmpi_init_")
        !"
        use iso_c_binding
          integer :: err
        end subroutine c_fmpi_init
      end interface

        ! SEND ARGUMENTS FROM FORTRAN TO C TO BUILD THE ARGV FOR FLEXMPI
      interface
        subroutine c_fmpi_buildArgv(name) bind(C, name="fmpi_buildArgv_")
          use iso_c_binding, only: c_char
          character(kind=c_char), dimension(*) :: name
        end subroutine c_fmpi_buildArgv
      end interface

contains

      subroutine fmpi_init(err)
        use iso_c_binding
          integer :: err
          call c_fmpi_init(err)
      end subroutine fmpi_init


      subroutine MPI_Comm_c2f(c_handle)
        use iso_c_binding
          integer :: c_handle
          call c_MPI_Comm_c2f(c_handle)
      end subroutine MPI_Comm_c2f


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

      end module test_mod
