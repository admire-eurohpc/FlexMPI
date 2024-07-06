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
#include <assert.h>
#include <pthread.h>
#include <mpi.h>
#include <empi.h>

#include <comm_data.h>
#include <malleability_tools.h>

//int parse_one_hostlist(char hostlist[HOSTNAME_MAXLEN], char firsthost[HOSTNAME_MAXLEN])
//{
//    char str_aux[HOSTNAME_MAXLEN]="";
//    char str_hostname[HOSTNAME_MAXLEN]="";
//    int numproc_host = 0;
//    int ret = 0;
//
//    strcpy(firsthost,"");
//    if (strlen(hostlist) > 0) {
//        ret = sscanf(hostlist,"%[^:]:%d,%s",str_hostname, &numproc_host, str_aux);
//        if ((ret >= 2) && (numproc_host > 0)) {
//            sprintf(firsthost,"%s:%d",str_hostname,numproc_host);
//            numproc_host--;
//            if (numproc_host == 0) {
//                strcpy(hostlist,str_aux);
//            } else {
//                if (strlen(str_aux) > 0) {
//                    sprintf(hostlist,"%s:%d,%s",str_hostname,numproc_host,str_aux);
//                } else {
//                    sprintf(hostlist,"%s:%d",str_hostname,numproc_host);
//                }
//            }
//        }
//    }
//    return 0;
//}

//int reorder_hostlist(char hostlist[HOSTNAME_MAXLEN])
//{
//    char str_hostlist[HOSTNAME_MAXLEN]="";
//    while (strlen(hostlist) > 0) {
//        char str_aux1[HOSTNAME_MAXLEN]="";
//        char str_aux2[HOSTNAME_MAXLEN]="";
//
//        sscanf(hostlist,"%[^,],%s",str_aux1, str_aux2);
//        strcpy(hostlist,str_aux2);
//        if (strlen(hostlist) > 0) {
//            sprintf(str_aux2,",%s",str_aux1);
//            strcpy(str_aux1,str_aux2);
//        }
//        strcpy(str_aux2,str_hostlist);
//        sprintf(str_hostlist,"%s%s",str_aux1,str_aux2);
//        fprintf(stderr, "Ireorder_hostlist: %s\n", str_hostlist);
//    }
//    strcpy(hostlist,str_hostlist);
//    return 0;
//}

int parse_one_hostlist(char *hostlist, int len_hostlist, char firsthost[HOSTNAME_MAXLEN])
{
    char *str_aux = NULL;
    char str_hostname[HOSTNAME_MAXLEN]="";
    int ret = 0;
    
    assert (strlen(hostlist)<=(ulong)len_hostlist);
    
    // separate first hostname:numprocs from hostlist
    fprintf(stderr,"parse_one_hostlist(%d): malloc(%d)\n",getpid(),len_hostlist+1);
    str_aux = (char *)malloc(len_hostlist+1);
    if (str_aux == NULL) {
        fprintf(stderr,"parse_one_hostlist(%d): ERROR in malloc\n",getpid());
        return -1;
    }
    bzero(str_aux,len_hostlist+1);
    bzero(firsthost,HOSTNAME_MAXLEN);
    if (strlen(hostlist) > 0) {
        ret = sscanf(hostlist,"%[^,],%s",str_hostname, str_aux);
        if (ret >= 1) {
            strcpy(firsthost,str_hostname);
            strcpy(hostlist,str_aux);
        }
    }
    fprintf(stderr,"parse_one_hostlist(%d): free\n",getpid());
    free(str_aux);
    return 0;
}

int malleability_spawn_loop(reconfig_data_t *data, char *hostlist, int len_hostlist)
{
    int ret = 0;
    
    fprintf(stderr,"malleability_spawn_loop(%d): Hostlist: %s\n", getpid(), hostlist);
        
    while (1) {
        MPI_Info hostinfo;
        char host_numprocs[HOSTNAME_MAXLEN+12]="";
        char firsthost[HOSTNAME_MAXLEN]="";
        int num_procs = 0;
        int num_booked_procs = 0;
        int rank, size;
        
        // get latest intracomm
        MPI_Comm *ptr_intracomm = comm_getptr_intracomm(&(data->comm));
        
        // update rank and size with new EMPI_COMM_WORLD
        fprintf(stderr,"malleability_spawn_loop(%d): MPI_Comm_rank(ptr_intracomm, &rank)\n",getpid());
        MPI_Comm_rank((*ptr_intracomm), &rank);
        MPI_Comm_size((*ptr_intracomm), &size);
        fprintf(stderr,"malleability_spawn_loop(%d): rank/size = %d/%d\n", getpid(), rank, size);

        // if rank zero, separate first hostname:numprocs from hostlist
        if (rank == 0) {
            parse_one_hostlist(hostlist, len_hostlist, host_numprocs);
            fprintf(stderr,"malleability_spawn_loop(%d): Hostlist: %s\n", getpid(), host_numprocs);
        }
              
        // broadcast hostname:numprocs from zero in latest intracomm
        fprintf(stderr,"malleability_spawn_loop(%d): MPI_Bcast(host_numprocs, HOSTNAME_MAXLEN, MPI_CHAR, 0, (*ptr_intracomm))\n",getpid());
        MPI_Bcast(host_numprocs, HOSTNAME_MAXLEN, MPI_CHAR, 0, (*ptr_intracomm));
        // broadcast hostname:numprocs from zero in latest intracomm
        fprintf(stderr,"malleability_spawn_loop(%d): MPI_Bcast(&(data->excl_nodes_hint, 1, MPI_INT, 0, (*ptr_intracomm))\n",getpid());
        MPI_Bcast(&(data->excl_nodes_hint), 1, MPI_INT, 0, (*ptr_intracomm));

        fprintf(stderr,"malleability_spawn_loop(%d): original host_numprocs = %s excl_nodes_hint = %d\n", getpid(), host_numprocs, data->excl_nodes_hint);

        // if hostname:numprocs was the end one; finish the loop
        if (strlen(host_numprocs) == 0) {
            // end the loop
            fprintf(stderr,"malleability_spawn_loop(%d): End the loop\n",getpid());
            break;
        }

        // separate hostname and num_booked_procs
        ret = sscanf(host_numprocs,"%[^:]:%d",firsthost, &num_booked_procs);
        if (ret != 2) {
            // end the loop
            fprintf(stderr,"malleability_spawn_loop(%d): Error Wrong host_numprocs = %s\n",getpid(),host_numprocs);
            return -1;
        }

        // set num_procs
        if (data->excl_nodes_hint == 1) {
            num_procs = 1;
        } else {
            num_procs = num_booked_procs;
        }
        
        // reset host_numprocs using final num_procs
        sprintf(host_numprocs,"%s:%d",firsthost, num_procs);

        fprintf(stderr,"malleability_spawn_loop(%d): final host_numprocs = %s\n",getpid(),host_numprocs);

        // create MPI_Info to spawn new processes in hostname:numprocs
        MPI_Info_create(&hostinfo);
        MPI_Info_set(hostinfo, "host", host_numprocs);
        //ALBERTO: add hostfile
        //ALBERTO MPI_Info_set(info, "add-hostfile", "jacobi_IO_hostfile2");
        //ALBERTO: Update hostfile
        //system("sed -i -e 's/slurm-node-1/slurm-node-2/' jacobi_IO_hostfile");  // modify hostfile for new spawn

        // add new comm entry and get new comm ptrs
        comm_add_empty(&(data->comm));
        MPI_Comm *new_intercomm = comm_getptr_intercomm(&(data->comm));
        MPI_Comm *new_intracomm = comm_getptr_intracomm(&(data->comm));
        // spawn the new set of processes and get new intercomm
        fprintf(stderr,"malleability_spawn_loop(%d): MPI_Comm_spawn %s - %s\n", getpid(), host_numprocs, hostlist);
        //MPI_Comm_spawn(data->command, MPI_ARGV_NULL, num_procs, hostinfo, 0, (*ptr_intracomm), new_intercomm, MPI_ERRCODES_IGNORE);
        MPI_Comm_spawn(data->command, data->argv, num_procs, hostinfo, 0, (*ptr_intracomm), new_intercomm, MPI_ERRCODES_IGNORE);

        // merger new intercomm to create new intracomm
        fprintf(stderr,"malleability_spawn_loop(%d): MPI_Intercomm_merge ((*new_intercomm), 0, new_intracomm);\n", getpid());
        int err = MPI_Intercomm_merge ((*new_intercomm), 0, new_intracomm);
        if (err) {
            fprintf (stderr, "Error in MPI_Intercomm_merge\n");
            return err;
        }
        // test (*ptr_intracomm)
        MPI_Comm_rank((*new_intracomm), &rank);
        int prueba1 = 0;
        if (rank == 0) {
            prueba1 = 12345;
        }
        fprintf(stderr,"malleability_spawn_loop(%d): MPI_Bcast(&prueba1, 1, MPI_INT, 0, (*new_intracomm)) rank=%d\n",getpid(), rank);
        MPI_Bcast(&prueba1, 1, MPI_INT, 0, (*new_intracomm));
        //MPI_Bcast(&prueba1, 1, MPI_INT, 0, EMPI_COMM_WORLD);
        fprintf(stderr,"malleability_spawn_loop(%d): prueba1 = %d new_intracomm=%p\n", getpid(), prueba1, new_intracomm);

        
        
        // add hostname and num procs to comm entry
        comm_set_hostname(&(data->comm), firsthost);
        comm_set_numprocs(&(data->comm), num_procs);
        comm_set_numbookedprocs(&(data->comm), num_booked_procs);

        // destroy MPI_Info
        MPI_Info_free(&hostinfo);
    }
    
    fprintf(stderr,"malleability_spawn_loop(%d): END\n", getpid());
    return 0;
}

int EMPI_Spawn_Init (char **argv, reconfig_data_t *data)
{
    int err = 0;
    int rank, size;
    
    // get application comman line for future spawn
    data->command = argv[0];
    data->argv = argv;

    // add new comm entry and get comm ptrs
    comm_add_empty(&(data->comm));
    MPI_Comm *ptr_intercomm = comm_getptr_intercomm(&(data->comm));
    MPI_Comm *ptr_intracomm = comm_getptr_intracomm(&(data->comm));

    // get parent intercom
    fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Comm_get_parent(ptr_intercomm)\n",getpid());
    MPI_Comm_get_parent(ptr_intercomm);
    
    // construct correct intercomm and intracomm
    if ((*ptr_intercomm) != MPI_COMM_NULL) {
        // it is a child process,  merge parent intercomm to intracomm
        data->ischild = 1;
        fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Intercomm_merge ((*ptr_intercomm), 1, ptr_intracomm)\n",getpid());
        int err = MPI_Intercomm_merge ((*ptr_intercomm), 1, ptr_intracomm);
        if (err) fprintf (stderr, "EMPI_Spawn_Init: Error in MPI_Intercomm_merge\n");
        
        // update rank and size with new EMPI_COMM_WORLD
        MPI_Comm_rank((*ptr_intracomm), &rank);
        MPI_Comm_size((*ptr_intracomm), &size);
        fprintf(stderr,"EMPI_Spawn_Init(%d): rank/size = %d/%d\n", getpid(), rank, size);

        // test (*ptr_intracomm)
        int prueba1 = 0;
        if (rank == 0) {
            prueba1 = 12345;
        }
        fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Bcast(&prueba1, 1, MPI_INT, 0, (*ptr_intracomm))\n",getpid());
        MPI_Bcast(&prueba1, 1, MPI_INT, 0, (*ptr_intracomm));
        //MPI_Bcast(&prueba1, 1, MPI_INT, 0, EMPI_COMM_WORLD);
        fprintf(stderr,"EMPI_Spawn_Init(%d): prueba1 = %d ptr_intracomm=%p\n", getpid(), prueba1, ptr_intracomm);

    } else {
        // it is an original process, use MPI_WORLD_COMM for both
        data->ischild = 0;
        fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Comm_dup (MPI_COMM_WORLD, ptr_intercomm)\n",getpid());
        err = MPI_Comm_dup (MPI_COMM_WORLD, ptr_intercomm);
        if (err) fprintf (stderr, "Error in MPI_Comm_dup\n");
        fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Comm_dup (MPI_COMM_WORLD, ptr_intracomm)\n",getpid());
        err = MPI_Comm_dup (MPI_COMM_WORLD, ptr_intracomm);
        if (err) fprintf (stderr, "Error in MPI_Comm_dup\n");
    }
    
    // get MPI_COMM_WORLD size
    int aux_size;
    fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Comm_size(MPI_COMM_WORLD, &aux_size)\n",getpid());
    MPI_Comm_size(MPI_COMM_WORLD, &aux_size);

    // add hostname and num procs to comm entry
    comm_set_hostname(&(data->comm), "");
    comm_set_numprocs(&(data->comm), aux_size);
    comm_set_numbookedprocs(&(data->comm), aux_size);
    
    // perform the malleability loop for next nodes
    fprintf(stderr,"EMPI_Spawn_Init(%d): malleability_spawn_loop(data, \"\")\n", getpid());
    malleability_spawn_loop(data, "", 0);
    fprintf(stderr,"EMPI_Spawn_Init(%d): END malleability_spawn_loop(data, \"\")\n", getpid());

//    // Disconnect EMPI_COMM_WORLD to reconnect it
//    if (EMPI_COMM_WORLD != MPI_COMM_NULL) {
//        fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Comm_disconnect(PTR_EMPI_COMM_WORLD)\n", getpid());
//        err=MPI_Comm_disconnect(PTR_EMPI_COMM_WORLD);
//        if (err) fprintf (stderr, "Error in MPI_Comm_disconnect(EMPI_COMM_WORLD)\n");
//    }
    
    // get latest comm intracomm
    ptr_intracomm = comm_getptr_intracomm(&(data->comm));
    
    // update rank and size with new EMPI_COMM_WORLD
    fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Comm_rank((*ptr_intracomm), &rank)\n",getpid());
    MPI_Comm_rank((*ptr_intracomm), &rank);
    MPI_Comm_size((*ptr_intracomm), &size);
    fprintf(stderr,"EMPI_Spawn_Init(%d): rank/size = %d/%d\n", getpid(), rank, size);

    // test (*ptr_intracomm)
    int prueba = 0;
    if (rank == 0) {
        prueba = 12345;
    }
    fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Bcast(&prueba, 1, MPI_INT, 0, (*ptr_intracomm)) ptr_intracomm=%p\n",getpid(), ptr_intracomm);
    MPI_Bcast(&prueba, 1, MPI_INT, 0, (*ptr_intracomm));
    fprintf(stderr,"EMPI_Spawn_Init(%d): prueba = %d ptr_intracomm=%p\n", getpid(), prueba, ptr_intracomm);


    // test barrier
    fprintf(stderr, "EMPI_Spawn_Init(%d):: MPI_Barrier((*ptr_intracomm))\n",getpid());
    MPI_Barrier((*ptr_intracomm));
    fprintf(stderr, "EMPI_Spawn_Init(%d):: END MPI_Barrier((*ptr_intracomm))\n",getpid());

//    // reconnect EMPI_COMM_WORLD
//    fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Comm_dup ((*ptr_intracomm), PTR_EMPI_COMM_WORLD)\n",getpid());
//    err = MPI_Comm_dup ((*ptr_intracomm), PTR_EMPI_COMM_WORLD);
//    if (err) fprintf (stderr, "Error in MPI_Comm_dup\n");

    // update rank and size with new EMPI_COMM_WORLD
    fprintf(stderr,"EMPI_Spawn_Init(%d): MPI_Comm_rank((*ptr_intracomm), &rank)\n",getpid());
    //MPI_Comm_rank(EMPI_COMM_WORLD, &rank);
    //MPI_Comm_size(EMPI_COMM_WORLD, &size);
    MPI_Comm_rank((*ptr_intracomm), &rank);
    MPI_Comm_size((*ptr_intracomm), &size);
    fprintf(stderr,"EMPI_Spawn_Init(%d): rank/size = %d/%d\n", getpid(), rank, size);
    
    return data->ischild;
}

int EMPI_Spawn_comms (reconfig_data_t *data)
{
    //int err;
    
    // get the new latest comm data and last index
    MPI_Comm *ptr_intracomm = comm_getptr_intracomm(&(data->comm));
       
    // Execute the malleability loop
    fprintf(stderr,"EMPI_Spawn_comms(%d): malleability_spawn_loop(data, data->malleable_data.local.hostlist, data->malleable_data.local.len_hostlist)\n", getpid());
    // NOTE: data is changed reload comm data
    malleability_spawn_loop(data, data->malleable_data.local.hostlist, data->malleable_data.local.len_hostlist);
    
//    // disconnect EMPI_COMM_WORLD
//    if (EMPI_COMM_WORLD != MPI_COMM_NULL) {
//        fprintf(stderr,"EMPI_Spawn_comms(%d): MPI_Comm_disconnect(PTR_EMPI_COMM_WORLD)\n", getpid());
//        err=MPI_Comm_disconnect(PTR_EMPI_COMM_WORLD);
//        if (err) fprintf (stderr, "Error in MPI_Comm_disconnect(EMPI_COMM_WORLD)\n");
//    }
    
    fprintf(stderr, "EMPI_Spawn_comms(%d):: comm_getptr_intracomm\n",getpid());
    // get the new latest intracomm
    ptr_intracomm = comm_getptr_intracomm(&(data->comm));
    
    int rank,size;
    // update rank and size with new EMPI_COMM_WORLD
    fprintf(stderr,"EMPI_Spawn_comms(%d): MPI_Comm_rank((*ptr_intracomm), &rank)\n",getpid());
    MPI_Comm_rank((*ptr_intracomm), &rank);
    MPI_Comm_size((*ptr_intracomm), &size);
    fprintf(stderr,"EMPI_Spawn_comms(%d): rank/size = %d/%d\n", getpid(), rank, size);

    // test (*ptr_intracomm)
    int prueba = 0;
    if (rank == 0) {
        prueba = 12345;
    }
    fprintf(stderr,"EMPI_Spawn_comms(%d): MPI_Bcast(&prueba, 1, MPI_INT, 0, (*ptr_intracomm)) ptr_intracomm=%p\n",getpid(), ptr_intracomm);
    MPI_Bcast(&prueba, 1, MPI_INT, 0, (*ptr_intracomm));
    fprintf(stderr,"EMPI_Spawn_comms(%d): prueba = %d ptr_intracomm=%p\n", getpid(), prueba, ptr_intracomm);

    // test barrier
    fprintf(stderr, "EMPI_Spawn_comms(%d):: MPI_Barrier((*ptr_intracomm))\n",getpid());
    MPI_Barrier((*ptr_intracomm));
    fprintf(stderr, "EMPI_Spawn_comms(%d):: END MPI_Barrier((*ptr_intracomm))\n",getpid());
    
//    // recreate the new EMPI_COMM_WORLD from new latest intracomm
//    fprintf(stderr,"EMPI_Spawn_comms(%d): MPI_Comm_dup ((*ptr_intracomm), PTR_EMPI_COMM_WORLD)\n",getpid());
//    err = MPI_Comm_dup ((*ptr_intracomm), PTR_EMPI_COMM_WORLD);
//    if (err) fprintf (stderr, "Error in MPI_Comm_dup\n");
    
    return 0;
}

int EMPI_Remove_comms (reconfig_data_t *data, int procs_to_erase, int *rank, int *size)
{
    int isEnd = 0;
    int err;
    
    // get the new latest comm data and last index
    MPI_Comm *ptr_intercomm = comm_getptr_intercomm(&(data->comm));
    MPI_Comm *ptr_intracomm = comm_getptr_intracomm(&(data->comm));
    int aux_numprocs = comm_get_numprocs(&(data->comm));
    int last_intercom = comm_get_last_comm(&(data->comm));
    
    // if the number of processes to erase is enough for the next node and there is still a malleability node to remove
    while( ( ((data->ischild==1) && (last_intercom >=0)) ||
             ((data->ischild==0) && (last_intercom > 0)) )
         &&(procs_to_erase >= aux_numprocs)) {
        
        fprintf(stderr,"EMPI_Remove_comms(%d): loop: last_intercom=%d procs_to_erase/numprocs %d/%d\n", getpid(), last_intercom, procs_to_erase, aux_numprocs);
                  
        // update the number of processes to remove in the future
        procs_to_erase = procs_to_erase - aux_numprocs;
        
        //disconect latest intercomm
        fprintf(stderr,"EMPI_Remove_comms(%d): MPI_Comm_disconnect(ptr_intercomm)\n", getpid());
        err=MPI_Comm_disconnect(ptr_intercomm);
        if (err) fprintf (stderr, "Error in MPI_Comm_disconnect(inter)\n");
        
        //disconect latest intracomm
        fprintf(stderr,"EMPI_Remove_comms(%d): MPI_Comm_disconnect(ptr_intracomm)\n", getpid());
        err=MPI_Comm_disconnect(ptr_intracomm);
        if (err) fprintf (stderr, "Error in MPI_Comm_disconnect(intra)\n");
        
        // remove the latest node comm data
        comm_remove(&(data->comm));

        // get the new latest intracomm and last index
        ptr_intercomm = comm_getptr_intercomm(&(data->comm));
        ptr_intracomm = comm_getptr_intracomm(&(data->comm));
        aux_numprocs = comm_get_numprocs(&(data->comm));
        last_intercom = comm_get_last_comm(&(data->comm));
    }
    
    // if all comm data is removed
    if (last_intercom < 0) {
        // set the End flag to be returned
        isEnd = 1;
        
//        //disconnect EMPI_COMM_WORLD
//        if (EMPI_COMM_WORLD != MPI_COMM_NULL) {
//            fprintf(stderr,"EMPI_Remove_comms(%d): MPI_Comm_disconnect(PTR_EMPI_COMM_WORLD)\n", getpid());
//            err=MPI_Comm_disconnect(PTR_EMPI_COMM_WORLD);
//            if (err) fprintf (stderr, "Error in MPI_Comm_disconnect(EMPI_COMM_WORLD)\n");
//        }
        // set invalid rank and size
        (*rank) = -1;
        (*size) = -1;
        
        fprintf(stderr,"EMPI_Remove_comms(%d): IT IS THE END\n", getpid());

    // else if latest comm data is valid
    } else if (last_intercom >= 0) {

//        if (EMPI_COMM_WORLD != MPI_COMM_NULL) {
//            //disconnect EMPI_COMM_WORLD
//            fprintf(stderr,"EMPI_Remove_comms(%d): MPI_Comm_disconnect(PTR_EMPI_COMM_WORLD)\n", getpid());
//            err=MPI_Comm_disconnect(PTR_EMPI_COMM_WORLD);
//            if (err) fprintf (stderr, "Error in MPI_Comm_disconnect(EMPI_COMM_WORLD)\n");
//        }
//        // reconnect EMPI_COMM_WORLD to the latest intracomm
//        fprintf(stderr,"EMPI_Remove_comms(%d): MPI_Comm_dup ((*ptr_intracomm), PTR_EMPI_COMM_WORLD)\n",getpid());
//        err = MPI_Comm_dup ((*ptr_intracomm), PTR_EMPI_COMM_WORLD);
//        if (err) fprintf (stderr, "Error in MPI_Comm_dup\n");
        
        // update rank and size for the new EMPI_COMM_WORLD
        //MPI_Comm_rank(EMPI_COMM_WORLD, rank);
        //MPI_Comm_size(EMPI_COMM_WORLD, size);
        MPI_Comm_rank((*ptr_intracomm), rank);
        MPI_Comm_size((*ptr_intracomm), size);
        fprintf(stderr,"EMPI_Remove_comms(%d): rank/size = %d/%d\n", getpid(), *rank, *size);
    }
    
    return isEnd;
}
int EMPI_broadcast_reconfigure_data (reconfig_data_t *data)
{
    
    int rank, size;
    
    // get the new latest comm data and last index
    MPI_Comm *ptr_intracomm = comm_getptr_intracomm(&(data->comm));

    // get rank and size for the new EMPI_COMM_WORLD
    MPI_Comm_rank((*ptr_intracomm), &rank);
    MPI_Comm_size((*ptr_intracomm), &size);
    fprintf(stderr,"EMPI_broadcast_reconfigure_data(%d): rank/size = %d/%d\n", getpid(), rank, size);

    if (rank == 0) {
        // access data exclusively
        fprintf(stderr,"EMPI_broadcast_reconfigure_data(%d): pthread_mutex_lock\n",getpid());
        pthread_mutex_lock(&(data->malleable_data.lock));
        
        // local copy of shrink flag, numprocs
        data->malleable_data.local.shrink = data->malleable_data.orig.shrink;
        data->malleable_data.orig.shrink = 0;
        data->malleable_data.local.numprocs = data->malleable_data.orig.numprocs;
        data->malleable_data.orig.numprocs = 0;
        
        
        //local copy of hostlist and its length
        data->malleable_data.local.len_hostlist = data->malleable_data.orig.len_hostlist;
        if (data->malleable_data.local.len_hostlist > 0) {
            data->malleable_data.local.hostlist = data->malleable_data.orig.hostlist;
            data->malleable_data.orig.hostlist = NULL;
            data->malleable_data.orig.len_hostlist = -1;
        }
        
        // get done flag and set it to TRUE (nothing to do) for future
        data->malleable_data.local.done = data->malleable_data.orig.done;
        data->malleable_data.orig.done = 1;
        
        // stop access data exclusively
        fprintf(stderr,"EMPI_broadcast_reconfigure_data(%d): pthread_mutex_unlock\n",getpid());
        
        pthread_mutex_unlock(&(data->malleable_data.lock));
    }
    
    // broadcast done flag so everyone knows if to do something
    fprintf(stderr,"EMPI_broadcast_reconfigure_data(%d): MPI_Bcast(&done, 1, MPI_INT, 0, (*ptr_intracomm))\n",getpid());
    MPI_Bcast(&(data->malleable_data.local.done), 1, MPI_INT, 0, (*ptr_intracomm));
     
    // broadcast shrink flag and num_procs
    fprintf(stderr,"EMPI_broadcast_reconfigure_data(%d): MPI_Bcast(&shrink, 1, MPI_INT, 0, (*ptr_intracomm))\n",getpid());
    MPI_Bcast(&(data->malleable_data.local.shrink), 1, MPI_INT, 0, (*ptr_intracomm));
    fprintf(stderr,"EMPI_broadcast_reconfigure_data(%d): MPI_Bcast(&maxprocs, 1, MPI_INT, 0, (*ptr_intracomm))\n",getpid());
    MPI_Bcast(&(data->malleable_data.local.numprocs), 1, MPI_INT, 0, (*ptr_intracomm));

    return data->malleable_data.local.done;
}

int EMPI_reconfigure(int shrink, uint32_t maxprocs, const char *hostlist, void *data)
{
    reconfig_data_t *d = (reconfig_data_t *)data;

    fprintf(stderr,"EMPI_reconfigure(%d): start\n",getpid());

    // if hostlist it too large, ERROR
    assert (shrink ||  (strlen(hostlist) <= STRING_MAXLEN));
    fprintf(stderr,"EMPI_reconfigure(%d): end assert\n",getpid());

    // access data exclusively
    fprintf(stderr,"EMPI_reconfigure(%d): pthread_mutex_lock\n",getpid());
    pthread_mutex_lock(&(d->malleable_data.lock));
    
    // copy shrink flag, numprocs
    d->malleable_data.orig.shrink = shrink;
    d->malleable_data.orig.numprocs = maxprocs;

    // copy hostlist with its length
    if (d->malleable_data.orig.hostlist != NULL) {
        fprintf(stderr,"EMPI_reconfigure(%d): free\n",getpid());
        free(d->malleable_data.orig.hostlist);
    }
    // if increases processes copy the hostslist
    if (!shrink) {
        int len_hostlist = strlen(hostlist)+1;

        fprintf(stderr,"EMPI_reconfigure(%d): malloc(%d)\n",getpid(),len_hostlist);
        d->malleable_data.orig.hostlist = (char *) malloc(len_hostlist);
        if (d->malleable_data.orig.hostlist == NULL) {
            fprintf(stderr,"EMPI_reconfigure(%d): ERROR in malloc\n",getpid());
            return -1;
        }
        fprintf(stderr,"EMPI_reconfigure(%d): bzero\n",getpid());
        bzero(d->malleable_data.orig.hostlist,len_hostlist);
        fprintf(stderr,"EMPI_reconfigure(%d): strncpy\n",getpid());
        strncpy(d->malleable_data.orig.hostlist,hostlist,len_hostlist);
        d->malleable_data.orig.len_hostlist = len_hostlist;
    
    // if decrease set an empty hostlist
    } else {
        d->malleable_data.orig.hostlist = NULL;
        d->malleable_data.orig.len_hostlist = -1;
    }
    // set done flag to FALSE (malleability needs to be done)
    d->malleable_data.orig.done = 0;

    // stop access data exclusively
    fprintf(stderr,"EMPI_reconfigure(%d): pthread_mutex_unlock\n",getpid());
    pthread_mutex_unlock(&(d->malleable_data.lock));
    
    return 0;
}
