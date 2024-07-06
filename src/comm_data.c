/**
 * Example MPI application using MPI_Comm_spawn.
 *
 * TODO:
 * XX Multithreading needs to be addressed
 */
#include <stdio.h>
#include <stdlib.h>             /* getenv */
#include <string.h>             /* str(n)len */
#include <unistd.h>             /* sleep */
#include <mpi.h>
#include <comm_data.h>


#define HOSTNAME_MAXLEN 10000
#define INIT_COMM_DATA_SIZE 100

int comm_add_empty(comm_data_t *data) {
    if (data->size_comm < 0) {
        fprintf(stderr,"comm_add_empty(%d): malloc\n",getpid());
        data->comm = (struct inner_comm_data *) malloc(INIT_COMM_DATA_SIZE * sizeof(struct inner_comm_data));
        if (data->comm == NULL) {
            fprintf(stderr,"comm_add_empty(%d): ERROR in malloc\n",getpid());
            return -1;
        }
        data->size_comm = INIT_COMM_DATA_SIZE;
    }
    if (data->size_comm <= data->last_comm) {
        data->size_comm = 2 * data->size_comm;
        fprintf(stderr,"comm_add_empty(%d): realloc\n",getpid());
        data->comm = (struct inner_comm_data *) realloc(data->comm, data->size_comm * sizeof(struct inner_comm_data));
        if (data->comm == NULL) {
            data->size_comm = -1;
            data->last_comm = -1;
            fprintf(stderr,"comm_add_empty(%d): ERROR in realloc\n",getpid());
            return -1;
        }
    }
    data->last_comm = data->last_comm +1;
    data->comm[data->last_comm].intercomm = NULL;
    data->comm[data->last_comm].intracomm = NULL;
    data->comm[data->last_comm].hostname = NULL;
    data->comm[data->last_comm].numprocs = 0;
    data->comm[data->last_comm].numbookedprocs = 0;
    fprintf(stderr,"comm_add_empty(%d): last_comm = %d\n",getpid(), data->last_comm);
    return 0;
}

int comm_remove(comm_data_t *data) {
    if (data->last_comm < 0) {
        fprintf(stderr,"comm_remove(%d): ERROR try to remove more that needed\n",getpid());
        return -1;
    }
    if (data->comm[data->last_comm].intercomm != NULL) {
        fprintf(stderr,"comm_remove(%d): free intercomm \n",getpid());
        free(data->comm[data->last_comm].intercomm);
    }
    data->comm[data->last_comm].intercomm = NULL;
    if (data->comm[data->last_comm].intracomm != NULL) {
        fprintf(stderr,"comm_remove(%d): free intracomm \n",getpid());
        free(data->comm[data->last_comm].intracomm);
    }
    data->comm[data->last_comm].intracomm = NULL;
    if (data->comm[data->last_comm].hostname != NULL) {
        fprintf(stderr,"comm_remove(%d): free hostname\n",getpid());
        free(data->comm[data->last_comm].hostname);
    }
    data->comm[data->last_comm].hostname = NULL;
    data->comm[data->last_comm].numprocs = 0;
    data->comm[data->last_comm].numbookedprocs = 0;

    // free memory if no entry left
    if (data->last_comm == 0) {
        fprintf(stderr,"comm_remove(%d): remove entries memory\n",getpid());
        free(data->comm);
        data->comm = NULL;
    }
    data->last_comm = data->last_comm -1;
    fprintf(stderr,"comm_remove(%d): last_comm = %d\n",getpid(), data->last_comm);
    return 0;
}

int comm_get_last_comm(comm_data_t *data) {
    fprintf(stderr,"comm_get_last_comm(%d): last_comm = %d\n",getpid(), data->last_comm);
    return data->last_comm;
}


MPI_Comm *comm_getptr_intercomm(comm_data_t *data) {
    if (data->last_comm < 0) {
        fprintf(stderr,"comm_getptr_intercomm(%d): data->last_comm < 0\n",getpid());
        return NULL;
    } else if (data->comm[data->last_comm].intercomm == NULL) {
        fprintf(stderr,"comm_getptr_intercomm(%d): malloc\n",getpid());
        data->comm[data->last_comm].intercomm = (MPI_Comm *) malloc(sizeof(MPI_Comm));
        if (data->comm[data->last_comm].intercomm == NULL) {
            fprintf(stderr,"comm_getptr_intercomm(%d): ERROR in malloc\n",getpid());
            return NULL;
        }
    }
    fprintf(stderr,"comm_getptr_intercomm(%d): last_comm = %d\n",getpid(), data->last_comm);
    return data->comm[data->last_comm].intercomm;
}

MPI_Comm *comm_getptr_intracomm(comm_data_t *data) {
    if (data->last_comm < 0) {
        fprintf(stderr,"comm_getptr_intracomm(%d): data->last_comm < 0\n",getpid());
        return NULL;
    } else if (data->comm[data->last_comm].intracomm == NULL) {
        fprintf(stderr,"comm_getptr_intracomm(%d): malloc\n",getpid());
        data->comm[data->last_comm].intracomm = (MPI_Comm *) malloc(sizeof(MPI_Comm));
        if (data->comm[data->last_comm].intracomm == NULL) {
            fprintf(stderr,"comm_getptr_intracomm(%d): ERROR in malloc\n",getpid());
            return NULL;
        }
    }
    fprintf(stderr,"comm_getptr_intracomm(%d): last_comm = %d\n",getpid(), data->last_comm);
    return data->comm[data->last_comm].intracomm;
}

char * comm_get_hostname(comm_data_t *data) {
    if (data->last_comm < 0) {
        fprintf(stderr,"comm_get_hostname(%d): data->last_comm < 0\n",getpid());
        return NULL;
    }
    fprintf(stderr,"comm_add_empty(%d): last_comm = %d\n",getpid(), data->last_comm);
    return data->comm[data->last_comm].hostname;
}

int comm_get_numprocs(comm_data_t *data) {
    if (data->last_comm < 0) {
        fprintf(stderr,"comm_get_numprocs(%d): data->last_comm < 0\n",getpid());
        return 0;
    }
    fprintf(stderr,"comm_get_numprocs(%d): last_comm = %d\n",getpid(), data->last_comm);
    return data->comm[data->last_comm].numprocs;
}

int comm_get_numbookedprocs(comm_data_t *data) {
    if (data->last_comm < 0) {
        fprintf(stderr,"comm_get_numbookedprocs(%d): data->last_comm < 0\n",getpid());
        return 0;
    }
    fprintf(stderr,"comm_get_numbookedprocs(%d): last_comm = %d\n",getpid(), data->last_comm);
    return data->comm[data->last_comm].numbookedprocs;
}


int comm_set_hostname(comm_data_t *data, char *hostname) {
    if (data->last_comm < 0) {
        fprintf(stderr,"comm_set_hostname(%d): data->last_comm < 0\n",getpid());
        return -1;
    }
    int len = strlen(hostname)+1;
    if (len > HOSTNAME_MAXLEN) {
        fprintf(stderr,"comm_set_hostname(%d): strlen(hostname)+1 > %d\n",getpid(),HOSTNAME_MAXLEN);
        return -1;
    }
    if (data->comm[data->last_comm].hostname != NULL) {
        fprintf(stderr,"comm_set_hostname(%d): free\n",getpid());
        free(data->comm[data->last_comm].hostname);
    }
    fprintf(stderr,"comm_set_hostname(%d): malloc\n",getpid());
    data->comm[data->last_comm].hostname = (char *) malloc(len);
    if (data->comm[data->last_comm].hostname == NULL) {
        fprintf(stderr,"comm_set_hostname(%d): ERROR in malloc\n",getpid());
        return -1;
    }
    bzero(data->comm[data->last_comm].hostname,len);
    strncpy(data->comm[data->last_comm].hostname, hostname, len);
    fprintf(stderr,"comm_set_hostname(%d): last_comm = %d\n",getpid(), data->last_comm);
    return 0;
}

int comm_set_numprocs(comm_data_t *data, int numprocs) {
    if (data->last_comm < 0) {
        fprintf(stderr,"comm_set_numprocs(%d): data->last_comm < 0\n",getpid());
        return -1;
    }
    fprintf(stderr,"comm_set_numprocs(%d): last_comm = %d\n",getpid(), data->last_comm);
    data->comm[data->last_comm].numprocs = numprocs;
    return 0;
}

int comm_set_numbookedprocs(comm_data_t *data, int numbookedprocs) {
    if (data->last_comm < 0) {
        fprintf(stderr,"comm_set_numbookedprocs(%d): data->last_comm < 0\n",getpid());
        return -1;
    }
    fprintf(stderr,"comm_set_numbookedprocs(%d): last_comm = %d\n",getpid(), data->last_comm);
    data->comm[data->last_comm].numbookedprocs = numbookedprocs;
    return 0;
}

int comm_get_numremovableprocs(comm_data_t *data, int max_procs_to_remove, int *removable_procs, int *num_iter) {
    if (data->last_comm <= 0) { // CHANGE: can not remove the first communicator
        fprintf(stderr,"comm_get_numremovableprocs(%d): data->last_comm <= 0\n",getpid());
        return 0;
    }
    int total_procs = 0;
    (*num_iter) = 0;
    for (int i=data->last_comm; i>0; i--) {
        if ((total_procs + data->comm[i].numprocs) > max_procs_to_remove) {
            break;
        }
        fprintf(stderr,"comm_get_numremovableprocs(%d): remove iteration -> last_comm=%d\n",getpid(), i);
        total_procs = total_procs + data->comm[i].numprocs;
        (*num_iter)++;
    }
    (*removable_procs) = total_procs;
    fprintf(stderr,"comm_get_numremovableprocs(%d): total procs to remove/Max procs to remove %d/%d,  num iterations: %d\n",getpid(), (*removable_procs), max_procs_to_remove,(*num_iter));
    return 0;
}


int comm_get_iterationdata(comm_data_t *data, int iter, MPI_Comm **intercomm, MPI_Comm **intracomm, char **hostname, int *numprocs, int *numbookedprocs) {
    
    // get desired position
    int pos = data->last_comm - iter;

    // if desired iter do not exist
    if (pos < 0) {
        fprintf(stderr,"comm_get_iterationdata(%d): BAD ITER: data->last_comm/iter = %d/%d, position = %d\n", getpid(), data->last_comm, iter, pos);
        // set initial return values
        if (intercomm!=NULL) (*intercomm)=NULL;
        if (intracomm!=NULL) (*intracomm)=NULL;
        if (hostname!=NULL) (*hostname)=NULL;
        if (numprocs!=NULL) (*numprocs)=0;
        if (numbookedprocs!=NULL) (*numbookedprocs)=0;
    } else {
        // get return values
        if (intercomm!=NULL) (*intercomm) = data->comm[pos].intercomm;
        if (intracomm!=NULL) (*intracomm) = data->comm[pos].intracomm;
        if (hostname!=NULL) (*hostname) = data->comm[pos].hostname;
        if (numprocs!=NULL) (*numprocs) = data->comm[pos].numprocs;
        if (numbookedprocs!=NULL) (*numbookedprocs) = data->comm[pos].numbookedprocs;
        fprintf(stderr,"comm_get_iterationdata(%d): GOOD ITER: data->last_comm/iter = %d/%d, position = %d\n", getpid(), data->last_comm, iter, pos);
    }
    return 0;
}

