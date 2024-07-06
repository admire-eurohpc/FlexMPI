/**
* @version        FlexMPI v3.1
* @copyright    Copyright (C) 2018 Universidad Carlos III de Madrid. All rights reserved.
* @license        GNU/GPL, see LICENSE.txt
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You have received a copy of the GNU General Public License in LICENSE.txt
* also available in <http://www.gnu.org/licenses/gpl.html>.
*
* See COPYRIGHT.txt for copyright notices and details.
*/

/****************************************************************************************************************************************
 *                                                                                                                                        *
 *    FLEX-MPI                                                                                                                            *
 *                                                                                                                                        *
 *    File:       process.c                                                                                                                    *
 *                                                                                                                                        *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

/* include */
#include <empi.h>
// CHANGE TEMP
#include <unistd.h>
// END CHANGE TEMP
/* headers */

/****************************************************************************************************************************************
*
*    'EMPI_Spawn_process'
*
****************************************************************************************************************************************/
static int EMPI_Spawn_process (char *argv[], char *bin, MPI_Info info, int nprocs);

/****************************************************************************************************************************************
*
*    'EMPI_Remove_processes'
*
****************************************************************************************************************************************/
static int EMPI_Remove_processes (int rank);

/****************************************************************************************************************************************
*
*    'EMPI_Remove_process_orig'
*
****************************************************************************************************************************************/
static int EMPI_Remove_process_orig (int rank);

/* implementation */
/****************************************************************************************************************************************
*
*    'EMPI_Spawn_comms_orig'
*
****************************************************************************************************************************************/
int EMPI_Spawn_comms_orig (char *argv[], char *bin, MPI_Info info, int nprocs)
{
    int *mpierr, err;
    MPI_Comm childcomm;
    int old_rank,old_size;

    mpierr=(int*)malloc(nprocs*sizeof(int));
    if(mpierr==NULL) {
        fprintf (stderr, "EMPI_Spawn_comms_orig(%d): Error in EMPI_Spawn_comms_orig: not enough memory\n", getpid());
        return -1;
    }
    
    MPI_Comm_rank (EMPI_COMM_WORLD, &old_rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &old_size);
    
    err = MPI_Comm_spawn (bin, argv, nprocs, info, EMPI_root, EMPI_COMM_WORLD, &childcomm, mpierr);
    if (err) {
        fprintf (stderr, "EMPI_Spawn_comms_orig(%d): Error in MPI_Comm_spawn\n", getpid());
        return err;
    }
    
    // CHANGE
    //Disconnect EMPI_COMM_WORLD communicator
    fprintf (stderr, "EMPI_Spawn_comms_orig(%d): MPI_Comm_disconnect (PTR_EMPI_COMM_WORLD)\n", getpid());
    err = MPI_Comm_disconnect (PTR_EMPI_COMM_WORLD);
    if (err) {
        fprintf (stderr, "EMPI_Spawn_comms_orig(%d): Error in MPI_Comm_disconnect\n", getpid());
        return err;
    }
    
    //Merge intercommunicator
    fprintf (stderr, "EMPI_Spawn_comms_orig(%d): MPI_Intercomm_merge (childcomm, 0, PTR_EMPI_COMM_WORLD)\n", getpid());
    err = MPI_Intercomm_merge (childcomm, 0, PTR_EMPI_COMM_WORLD);
    if (err) {
        fprintf (stderr, "EMPI_Spawn_comms_orig(%d): Error in MPI_Intercomm_merge\n", getpid());
        return err;
    }
    
    //Disconnect aux communicator
    fprintf (stderr, "EMPI_Spawn_comms_orig(%d): MPI_Comm_disconnect (&childcomm)\n", getpid());
    err = MPI_Comm_disconnect (&childcomm);
    if (err) {
        fprintf (stderr, "EMPI_Spawn_comms_orig(%d): Error in MPI_Comm_disconnect\n", getpid());
        return err;
    }
    
    fprintf (stderr, "EMPI_Spawn_comms_orig(%d): DONE MPI_Comm_disconnect (&childcomm)\n", getpid());
    int rank,size;
    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);
    fprintf (stderr, "EMPI_Spawn_comms_orig(%d): EMPI_Spawn_comms_orig rank=%d,%d; size=%d,%d\n", getpid(), old_rank, rank, old_size, size);
    fprintf (stderr, "EMPI_Spawn_comms_orig(%d): MPI_Barrier(EMPI_COMM_WORLD) rank=%d\n", getpid(), rank);
    MPI_Barrier(EMPI_COMM_WORLD);
    fprintf (stderr, "EMPI_Spawn_comms_orig(%d): DONE MPI_Barrier(EMPI_COMM_WORLD) rank=%d\n", getpid(), rank);
    
    free(mpierr);
    
    sleep(2);

    return err;
}

/****************************************************************************************************************************************
*
*    'EMPI_Spawn_process'
*
****************************************************************************************************************************************/
static int EMPI_Spawn_process (char *argv[], char *bin, MPI_Info info, int nprocs) {

    int flag = 0;
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Spawn_process in <%s> ***\n", __FILE__);
    #endif

    int err;
    int i=0;

        
    // Manages the input argument: if the allocation is set, the input argument is changed to the new value
    
    while(argv!=MPI_ARGV_NULL && argv[i]!=NULL){
        if(strcmp(argv[i],"-alloc:0")==0)
        {
         if(EMPI_array_alloc>0){
             flag = 1;
             argv[i][7]='1'; // The dynamic allocation is done in the main function
         }
        }
        if(strcmp(argv[i],"-alloc:1")==0)
        {
            flag = 1;
        }      
        i++;
    }
    if(EMPI_array_alloc>0 && flag==0){
        fprintf(stderr, "EMPI_Spawn_process(%d): Error in EMPI_Spawn_process: -alloc:0 option not found and dynamic allocation was set. Use -alloc:0 for enabling dynamic allocation. \n", getpid());
    }
        
    // CHANGE: START_NEWVER
    // if EMPI_MALLEABILITY_TRIG -> get reconfiguration data
    if  (EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY_TRIG) {
        err = EMPI_Spawn_comms (&EMPI_GLOBAL_reconfig_data);
    } else {
        err = EMPI_Spawn_comms_orig (argv, bin, info, nprocs);
    }
    // CHANGE: END_NEWVER

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Spawn_process in <%s> ***\n", __FILE__);
    #endif

    return err;
}

/****************************************************************************************************************************************
*
*    'EMPI_Spawn'
*
****************************************************************************************************************************************/
int EMPI_Spawn (int nprocs, char *argv[], char *bin, int *hostid, MPI_Info *info) {

    //TODO: volver a permitir MPI_Info

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Spawn in <%s> ***\n", __FILE__);
    #endif

    int n, m, rank, size, rprocs, tag=997, sdata[4], err,cnt,last_host;
    int nhost;
    int *nhost_list;
    char *string_hostlist,tmpchar[100];
    EMPI_host_type *hostlist = NULL;
    MPI_Info infoc;

    nhost_list=(int*)malloc(nprocs*sizeof(int));
    string_hostlist=(char *)malloc(nprocs*40*sizeof(char));
    if(string_hostlist==NULL || nhost_list==NULL){
          fprintf(stderr,"Error in EMPI_Spawn: not enough memory\n");
          exit(1);        
    }
        
    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);
      
    //reset global variables
    EMPI_GLOBAL_concurrency = EMPI_TRUE;
    if (EMPI_GLOBAL_lbalance_disabled == EMPI_FALSE) EMPI_GLOBAL_lbalance = EMPI_TRUE;

    // Only for root: for each new created process generates the destination list
    if (rank == EMPI_root) {
        fprintf(stderr, "EMPI_Spawn(%d): i am root\n", getpid());

        //Create info     
        MPI_Info_create (&infoc);
        
        cnt=0;
        last_host=-1;
        
        for (n = 0; n < nprocs; n ++) {
            
            // Nhost is the compute node sellected to spawm the process
            if (hostid != NULL)
                nhost = hostid[n];
            else{
                EMPI_Sched_spawn (&nhost);
            }
                
            
            nhost_list[n]=nhost;            
            hostlist = EMPI_GLOBAL_hostlist;

            for (m = 0; m < EMPI_GLOBAL_nhosts; m ++) {

                fprintf(stderr, "EMPI_Spawn(%d): hostlist->hostname: %s \n", getpid(), hostlist->hostname);

                // New node is included in the list
                if ((hostlist != NULL)&&(nhost == m)){                        
            
                    // The new node is different than the previous one
                    if(last_host!=m ){
                        
                        // Updates values of the previous coincidence
                        if(last_host!=-1){
                            strcat(string_hostlist, ":");        
                            sprintf(tmpchar,"%d",cnt);
                            strcat(string_hostlist,tmpchar);    
                            cnt=0;
                        }
                        last_host=m;
                        
                        // Creates the string with the list of hosts
                        if(n==0) {
                            strcpy(string_hostlist, hostlist->hostname);
                        }
                        else{
                            strcat(string_hostlist, ",");                            
                            strcat(string_hostlist, hostlist->hostname);
                        }                            
                    }

                    
                     // Counts the number of processs in the class
                    cnt++;
                    //Increases the number of processes of the class
                    hostlist->nprocs++;
                    
                }                    
                else{
                    fprintf(stderr, "EMPI_Spawn(%d): hostlist: %p \n", getpid(), hostlist);

                    hostlist = hostlist->next;
                }
            }
        }
        
        // Adds last values
        strcat(string_hostlist, ":");        
        sprintf(tmpchar,"%d",cnt);
        strcat(string_hostlist,tmpchar);        
        
        fprintf(stderr, "EMPI_Spawn(%d): Host list: %s \n", getpid(), string_hostlist);
        MPI_Info_set (infoc, "hosts",string_hostlist);
    }
        
    //Spawn new process
    err = EMPI_Spawn_process (argv, bin, infoc,nprocs);
    
    // Only for root: for each new created process
    if (rank == EMPI_root) {
        MPI_Info_free (&infoc);

        for (n = 0; n < nprocs; n ++) {
        
            rprocs = nprocs - n - 1;

            //set number of remain  ing spawns, minprocs and maxprocs
            sdata[0] = rprocs;
            sdata[1] = EMPI_GLOBAL_minprocs;
            sdata[2] = (EMPI_GLOBAL_iteration+1); //next iteration
            sdata[3] = nhost_list[n];

            //send number of remaining spawns, minprocs and maxprocs. Size is the comm size before the spawn
            fprintf (stderr, "EMPI_Spawn(%d): BEGIN PMPI_Send\n",getpid());
            PMPI_Send (sdata, 4, MPI_INT, size+n, tag, EMPI_COMM_WORLD);
            fprintf (stderr, "EMPI_Spawn(%d): END PMPI_Send\n",getpid());
        }
    }

    free(string_hostlist);
    free(nhost_list);
    
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Spawn in <%s> ***\n", __FILE__);
    #endif

    return err;
}

/****************************************************************************************************************************************
*
*    'EMPI_Remove_processes'
*
****************************************************************************************************************************************/
static int EMPI_Remove_processes (int procs_to_erase) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Remove_processes in <%s> ***\n", __FILE__);
    #endif
    fprintf (stderr, "EMPI_Remove_processes(%d): START\n", getpid());

    int isRemoved = -1;
    int size, rank;
    
    //Get rank & size
    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);
    
    
    if (size > EMPI_GLOBAL_minprocs) {

        
        //remove
        EMPI_GLOBAL_concurrency = EMPI_TRUE;

        fprintf (stderr, "EMPI_Remove_processes(%d): EMPI_Remove_comms\n", getpid());
        isRemoved = EMPI_Remove_comms (&EMPI_GLOBAL_reconfig_data, procs_to_erase, &rank, &size);

        if (isRemoved == 1) {
            //set process status
            fprintf (stderr, "EMPI_Remove_processes(%d): status=EMPI_REMOVED\n", getpid());
            EMPI_GLOBAL_status = EMPI_REMOVED;
        }
    } else {

        fprintf (stderr, "EMPI_Remove_processes(%d): Error attempting to remove an initial process\n", getpid());

        MPI_Abort (EMPI_COMM_WORLD, -1);
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Remove_processes in <%s> ***\n", __FILE__);
    #endif
    fprintf (stderr, "EMPI_Remove_processes(%d): END\n", getpid());

    return isRemoved;
}

/****************************************************************************************************************************************
*
*    'EMPI_Remove_process_orig'
*
****************************************************************************************************************************************/
static int EMPI_Remove_process_orig (int rank) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Remove_process_orig in <%s> ***\n", __FILE__);
    #endif
    fprintf (stderr, "EMPI_Remove_process_orig(%d): START\n", getpid());

    int err, size, myrank,rank2;
    
    MPI_Comm aux;
    
    MPI_Group ggroup, exclgroup;
    
    rank2=rank;

    //Get rank & size
    MPI_Comm_rank (EMPI_COMM_WORLD, &myrank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);
    
    
    if (size > EMPI_GLOBAL_minprocs) {

        
        //remove
        EMPI_GLOBAL_concurrency = EMPI_TRUE;

        //Create group
        MPI_Comm_group (EMPI_COMM_WORLD, &ggroup);


        //Group of excluded rank
        MPI_Group_excl (ggroup, 1, &rank, &exclgroup);

        //Create new communicator
        // CHANGE BEGIN
        fprintf (stderr, "EMPI_Remove_process_orig(%d) Executing MPI_Comm_dup (EMPI_COMM_WORLD, &aux2) RankRem/myrank = (%d,%d)\n", getpid(), rank, myrank);
        err = MPI_Comm_create (EMPI_COMM_WORLD, exclgroup, &aux);
        // CHANGE END
        if (err) {fprintf (stderr, "Error in MPI_Comm_create\n"); return err;}
        
        


        
        // Disconnect original communicator
        fprintf (stderr, "EMPI_Remove_process_orig(%d) Executing MPI_Comm_free(PTR_EMPI_COMM_WORLD) RankRem/myrank = (%d,%d)\n", getpid(), rank, myrank);
        err = MPI_Comm_free(PTR_EMPI_COMM_WORLD);
        // END CHANGE
        if (err) {fprintf (stderr, "Error in MPI_Comm_disconnect\n"); return err;}

        //Group free
        MPI_Group_free (&ggroup);
        MPI_Group_free (&exclgroup);
        
        if (myrank != rank2) {

            //Dupplicate communicator
            err = MPI_Comm_dup (aux, PTR_EMPI_COMM_WORLD);
            if (err) {fprintf (stderr, "Error in MPI_Comm_dup\n"); return err;}

            //Disconnect communicator
            // CHANGE
            fprintf (stderr, "EMPI_Remove_process_orig(%d) Executing MPI_Comm_disconnect(&aux) RankRem/myrank = (%d,%d)\n",getpid(), rank, myrank);
            // END CHANGE
            err = MPI_Comm_disconnect (&aux);
            if (err) {fprintf (stderr, "Error in MPI_Comm_disconnect\n"); return err;}

        } else{
            
            //set process status
            fprintf (stderr, "EMPI_Remove_process_orig(%d) process.c: EMPI_Remove_process_orig: status=EMPI_REMOVED\n", getpid());
            EMPI_GLOBAL_status = EMPI_REMOVED;
        }
    } else {

        fprintf (stderr, "EMPI_Remove_process_orig(%d) Error in EMPI_Remove_process_orig: attempting to remove an initial process\n", getpid());

        MPI_Abort (EMPI_COMM_WORLD, -1);
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Remove_process_orig in <%s> ***\n", __FILE__);
    #endif
    fprintf (stderr, "EMPI_Remove_process_orig(%d): END\n", getpid());

    return err;
}

/****************************************************************************************************************************************
*
*    'EMPI_Remove'
*
****************************************************************************************************************************************/
int EMPI_Remove (int nprocs, int *rank) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Remove in <%s> ***\n", __FILE__);
    #endif
    fprintf (stderr, "EMPI_Remove(%d): START\n", getpid());


    //FIXME: cuando un host se queda con 0 nprocs poner sus mflops a -1, para que funcione bien Set_host_perf.

    int n, m, nrank, size, status, err, myrank, nhost, tag = 891;

    MPI_Status mpi_status;

    MPI_Comm_rank (EMPI_COMM_WORLD, &myrank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);

    EMPI_host_type *hostlist = NULL;
    
    if ((size - nprocs) < EMPI_GLOBAL_minprocs) {

        fprintf (stderr, "\nError in EMPI_Sched_remove: attempting to remove native processes\n");

        MPI_Abort (EMPI_COMM_WORLD, -1);
    }

    // CHANGE: START_NEWVER
    // if EMPI_MALLEABILITY_TRIG -> get reconfiguration data
    if ((rank != NULL) && (EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY_TRIG)) {
        
        for (n = 0; n < nprocs; n ++) {
            
            // check that removed rank is not a native rank
            if (rank[n] < EMPI_GLOBAL_minprocs) {
                fprintf (stderr, "\nError in EMPI_Remove: attempting to remove a native process\n");
                MPI_Abort (EMPI_COMM_WORLD, -1);
            }
            
            // check that removed rank is outside remaining size
            fprintf (stderr, "EMPI_Remove(%d):  rank to remove = %d\n", getpid(), rank[n]);
            if (rank[n] < size - nprocs) {
                fprintf (stderr, "EMPI_Remove(%d):  ERROR removing invalid rank: remov_rank/remain_size =%d/%d\n", getpid(), rank[n], size - nprocs);
                MPI_Abort (EMPI_COMM_WORLD, -1);
            }
                     
            // if removed, send notification to rank zero
            if (rank[n] == myrank) {
                PMPI_Send (&EMPI_GLOBAL_hostid, 1, MPI_INT, EMPI_root, tag, EMPI_COMM_WORLD);
            }
            // if rank zero, receive notification
            if (myrank == EMPI_root) {
                PMPI_Recv(&nhost, 1, MPI_INT, rank[n], tag, EMPI_COMM_WORLD, &mpi_status);
                
                //update list of processes to be removed
                hostlist = EMPI_GLOBAL_hostlist;
                for (m = 0; m < EMPI_GLOBAL_nhosts; m ++) {
                    if (nhost == hostlist->id) {
                        hostlist->nprocs --;
                    }
                    hostlist = hostlist->next;
                }
            }
        }
       
        //Remove set of processes
        fprintf (stderr, "EMPI_Remove rank(%d): EMPI_Remove_processes nprocs=%d\n", getpid(), nprocs);
        err = EMPI_Remove_processes (nprocs);
        if (err < 0) fprintf (stderr, "EMPI_Remove rank(%d): Error in EMPI_Remove_processes\n", getpid());
        // CHANGE: END_NEWVER
        
    } else if (rank != NULL) {

        for (n = 0; n < nprocs; n ++) {

            if (rank[n] < EMPI_GLOBAL_minprocs) {

                fprintf (stderr, "\nError in EMPI_Remove: attempting to remove a native process\n");

                MPI_Abort (EMPI_COMM_WORLD, -1);
            }

            EMPI_Get_status (&status);

            if (status == EMPI_ACTIVE) {

                if (rank[n] == myrank) PMPI_Send (&EMPI_GLOBAL_hostid, 1, MPI_INT, EMPI_root, tag, EMPI_COMM_WORLD);

                if (myrank == EMPI_root) {

                    PMPI_Recv(&nhost, 1, MPI_INT, rank[n], tag, EMPI_COMM_WORLD, &mpi_status);

                    //pointer to global variable
                    hostlist = EMPI_GLOBAL_hostlist;

                    for (m = 0; m < EMPI_GLOBAL_nhosts; m ++) {

                        if (nhost == hostlist->id) {

                            hostlist->nprocs --;
                        }

                        hostlist = hostlist->next;
                    }
                }

                //Remove process
                err = EMPI_Remove_process_orig (rank[n]);
                fprintf (stderr, "EMPI_Remove rank %d REMOVED (pid:%d,rank:%d),status:%d=%d,nprocs=%d\n", rank[n],getpid(),myrank,EMPI_GLOBAL_status,EMPI_REMOVED,nprocs);
            }
            // CHANGE TEMP
            else {
                fprintf (stderr, "EMPI_Remove rank %d REMAIN (pid:%d,rank:%d),status:%d=%d,nprocs=%d\n", rank[n],getpid(),myrank,EMPI_GLOBAL_status,EMPI_REMOVED,nprocs);
            }
            // END CHANGE TEMP
        }

    } else {

        for (n = 0; n < nprocs; n ++) {

            EMPI_Get_status (&status);
            if (status == EMPI_ACTIVE) {

                //schedule process to being removed
                EMPI_Sched_remove (&nrank, &nhost);

                if (myrank == EMPI_root) {

                    //pointer to global variable
                    hostlist = EMPI_GLOBAL_hostlist;

                    for (m = 0; m < EMPI_GLOBAL_nhosts; m ++) {

                        if (nhost == hostlist->id) {

                            hostlist->nprocs --;
                        }

                        hostlist = hostlist->next;
                    }
                }

                //Remove process
                
                err = EMPI_Remove_process_orig (nrank);
            }
        }
    }

    hostlist = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Remove in <%s> ***\n", __FILE__);
    #endif
    fprintf (stderr, "EMPI_Remove(%d): END\n", getpid());

    return err;
}
