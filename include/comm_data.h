/**
 * Example MPI application using MPI_Comm_spawn.
 *
 * TODO:
 * XX Multithreading needs to be addressed
 */
#ifndef _COMM_DATA_H_
#define _COMM_DATA_H_

#include <stdio.h>
#include <stdlib.h>             /* getenv */
#include <mpi.h>

struct inner_comm_data{
    MPI_Comm                *intercomm;
    MPI_Comm                *intracomm;
    char                    *hostname;
    int                     numprocs;
    int                     numbookedprocs;
};

struct comm_data {
    struct inner_comm_data * comm;
    int size_comm;
    int last_comm;
};
typedef struct comm_data comm_data_t;
#define COMM_DATA_DEFAULT {NULL,-1,-1}

int comm_add_empty(comm_data_t *data);
int comm_remove(comm_data_t *data);

int comm_get_last_comm(comm_data_t *data);

MPI_Comm *comm_getptr_intercomm(comm_data_t *data);
MPI_Comm *comm_getptr_intracomm(comm_data_t *data);

char * comm_get_hostname(comm_data_t *data);
int comm_get_numprocs(comm_data_t *data);
int comm_get_numbookedprocs(comm_data_t *data);

int comm_set_hostname(comm_data_t *data, char *hostname);
int comm_set_numprocs(comm_data_t *data, int numprocs);
int comm_set_numbookedprocs(comm_data_t *data, int numbookedprocs);
int comm_get_numremovableprocs(comm_data_t *data, int max_procs_to_remove, int *removable_procs, int *num_iter);
int comm_get_iterationdata(comm_data_t *data, int iter, MPI_Comm **intercomm, MPI_Comm **intracomm, char **hostname, int *numprocs, int *numbookedprocs);

#endif
