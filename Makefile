# For a system-level (non-local) installation of the libraries
#MPICC   =mpicc.mpich
MPICC   =mpicc
MPICH   =/usr/mpich/include/

# Uncomment for a local installation of the libraries
#PAPI_DIR=$(HOME)/LIBS/papi
#GLPK_DIR=$(HOME)/LIBS/glpk
#MPICC   =$(HOME)/LIBS/mpich/bin/mpicc
#MPICH   =$(HOME)/LIBS/mpich/include/
#PAPI=-I$(PAPI_DIR)/include/
#GLPK=-I$(GLPK_DIR)/include/ -L$(GLPK_DIR)/lib/libglpk.a

# Other options
MATH=-lm
DEBUG=-DEMPI_DBGMODE=EMPI_DBG_QUIET
CFLAGS=-Wall -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unused-function

all:
	make cleanlib
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/wrapper.o ./src/wrapper.c -I./include/ -I$(MPICH)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/lbalance.o ./src/lbalance.c -I./include/ -I$(MPICH)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/monitor_lp.o ./src/monitor_lp.c -I./include/ -I$(MPICH) $(GLPK)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/monitor.o ./src/monitor.c -I./include/ -I$(MPICH) $(PAPI)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/scheduler.o ./src/scheduler.c -I./include/ -I$(MPICH)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/rdata.o ./src/rdata.c -I./include/ -I$(MPICH)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/memalloc.o ./src/memalloc.c -I./include -I$(MPICH)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/process.o ./src/process.c -I./include -I$(MPICH)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/server.o ./src/server.c -I./include -I$(MPICH)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/init.o ./src/init.c -I./include/ -I$(MPICH) $(PAPI)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/adm_app_manager.o ./src/adm_app_manager.c -I./include/ -I$(MPICH) $(PAPI)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/comm_data.o ./src/comm_data.c -I./include/ -I$(MPICH) $(PAPI)
	$(MPICC) $(CFLAGS) $(DEBUG) -fPIC -c -o ./src/malleability_tools.o ./src/malleability_tools.c -I./include/ -I$(MPICH) $(PAPI)
	$(MPICC) $(CFLAGS) -shared -fPIC -o ./lib/libempi.so ./src/comm_data.o ./src/malleability_tools.o ./src/adm_app_manager.o ./src/init.o ./src/server.o ./src/process.o ./src/wrapper.o ./src/lbalance.o ./src/monitor_lp.o ./src/monitor.o ./src/scheduler.o ./src/rdata.o ./src/memalloc.o $(MATH) -lhiredis
	make postclean
	@echo "+++MAKE COMPLETE+++"

postclean:
	#rm -f ./src/*.o
	@echo "+++MAKE POSTCLEAN COMPLETE+++"

cleanlib:
	rm -f ./lib/*.so
	@echo "+++ MAKE CLEANLIB COMPLETE+++"

cleantemp:
	find . -type f -name '*~' -exec rm -f {} \;
	@echo "+++ MAKE CLEANTEMP COMPLETE+++"

clean:
	rm -f ./*/*~
	@echo "+++ MAKE CLEAN COMPLETE+++"
