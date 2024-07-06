/**
 * Example MPI application using MPI_Comm_spawn.
 *
 * TODO:
 * XX Multithreading needs to be addressed
 */
#ifndef _MALLEABILTY_TOOLS_H_
#define _MALLEABILTY_TOOLS_H_

#include <stdio.h>
#include <stdlib.h>             /* getenv */
#include <mpi.h>
#include <comm_data.h>

#define HOSTNAME_MAXLEN 1000
#define STRING_MAXLEN   100000

struct malleable_basic_data{
    int             done;
    char            *hostlist;
    int             len_hostlist;
    int             shrink;
    int             numprocs;
};
typedef struct malleable_basic_data malleable_basic_data_t;
#define MALLEABLE_BASIC_DATA_DEFAULT {1,NULL,-1,0,0}

struct malleable_data{
    malleable_basic_data_t  orig;
    malleable_basic_data_t  local;
    pthread_mutex_t lock;
};
typedef struct malleable_data malleable_data_t;
#define MALLEABLE_DATA_DEFAULT {MALLEABLE_BASIC_DATA_DEFAULT, MALLEABLE_BASIC_DATA_DEFAULT, PTHREAD_MUTEX_INITIALIZER}


struct reconfig_data{
    comm_data_t         comm;
    int                 ischild;
    char                *command;
    char                **argv;
    int                 procs_hint;
    int                 excl_nodes_hint;
    malleable_data_t    malleable_data;
    struct icc_context  *icc;
};
typedef struct reconfig_data reconfig_data_t;
#define RECONFIG_DATA_DEFAULT  {COMM_DATA_DEFAULT,0,NULL,NULL,0,0,MALLEABLE_DATA_DEFAULT,NULL}

//extern MPI_Comm EMPI_COMM_WORLD;


int malleability_spawn_loop(reconfig_data_t *data, char *hostlist, int len_hostlist);

int EMPI_Spawn_Init (char **argv, reconfig_data_t *data);

int EMPI_Spawn_comms (reconfig_data_t *data);

int EMPI_Remove_comms (reconfig_data_t *data, int procs_to_erase, int *rank, int *size);

int EMPI_broadcast_reconfigure_data (reconfig_data_t *data);

int EMPI_reconfigure(int shrink, uint32_t maxprocs, const char *hostlist, void *data);

#endif
