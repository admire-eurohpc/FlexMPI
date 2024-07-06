CC=gcc
LIBS=-lpthread -lm
CFLAGS= -g -Wall

.PHONY: controller 

controller:
	$(CC) $(CFLAGS) -o controller controller.c ioscheduler.c $(LIBS)
	@echo "+++MAKE CONTROLLER COMPLETED" 

	$(CC) $(CFLAGS) -o workloadgen workloadgen.c $(LIBS) 
	@echo "+++MAKE WORK LOAD GENERATOR COMPLETED" 
