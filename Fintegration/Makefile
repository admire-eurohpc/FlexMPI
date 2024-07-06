MPICC=mpicc.mpich
includedir := /usr/local/include
librarydir := /usr/local/lib
UNAME := $(shell uname -m)
INCLANDLIBS=-I/usr/include/ -L/usr/lib/$(UNAME)-linux-gnu -I$(includedir) -L$(librarydir) -lhiredis
F77=mpifort.mpich -fcommon

HOME=/home/admin/

all:
	#make wrap_init
	make jacobi
	#make primes
	#make gather
	#make reduce
	@echo "+++MAKE COMPLETE+++"

jacobi: wrap_init.o jacobi.o
	$(F77) -o jacobi jacobi.o ../src/init.o ../src/adm_app_manager.o wrap_init.o $(INCLANDLIBS) -lempi -lpapi -lglpk -lstdc++ -lpthread -licc

primes: primes.o 
	$(F77) -o primes primes.o $(INCLANDLIBS) -lempi -lpapi -lglpk -lstdc++ -lpthread -licc

gather: gather.o 
	$(F77) -o gather gather.o $(INCLANDLIBS) -lempi -lpapi -lglpk -lstdc++ -lpthread -licc

reduce: reduce.o 
	$(F77) -o reduce reduce.o $(INCLANDLIBS) -lempi -lpapi -lglpk -lstdc++ -lpthread -licc

primes.o: primes.f90
	$(F77) -c primes.f90 $(INCLANDLIBS)

gather.o: gather.f90
	$(F77) -c gather.f90 $(INCLANDLIBS)

reduce.o: reduce.f90
	$(F77) -c reduce.f90 $(INCLANDLIBS)

jacobi.o: jacobi.f90
	$(F77) -c jacobi.f90 $(INCLANDLIBS)

wrap_init.o: wrap_init.f90
	$(F77) -c wrap_init.f90 $(INCLANDLIBS)

clean:
	rm -f ./*/*~
	rm *.o
	rm wrap_init.mod 
	rm jacobi
	rm primes
	rm gather
	rm reduce

	@echo "+++ MAKE CLEAN COMPLETE+++"
