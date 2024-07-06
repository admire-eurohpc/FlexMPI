# For a system-level (non-local) installation of the libraries
MPICC=mpicc

UNAME := $(shell uname -m)
MPICH=-I/usr/include/$(UNAME)-linux-gnu

HOME=/home/admin/
# Uncomment for a local installation of the libraries
F77 	= mpifort 
	
all: jacobi_mpi
	@echo "+++MAKE COMPLETE+++"

	
jacobi_mpi: jacobi_mpi.o testmod.o testmodc.o
	$(F77) -o jacobi_mpi jacobi_mpi.o testmod.o testmodc.o -lstdc++ -lpthread  

jacobi_mpi.o: jacobi_mpi.f90
	 $(F77) -c jacobi_mpi.f90  

testmod.o: testmod.f90
	$(F77) -c testmod.f90

testmodc.o: testmodc.c
	$(MPICC) -c testmodc.c  

postclean:
	rm -f $(EMPI_DIR)/src/*.o

cleanlib:
	rm -f $(EMPI_DIR)/lib/*.so

cleantemp:
	find . -type f -name '*~' -exec rm -f {} \;

clean:
	rm -f ./*/*~
	rm *.o
	rm jacobi_mpi

	@echo "+++ MAKE CLEAN COMPLETE+++"
