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
 *                                                                                                                                      *
 *  FLEX-MPI                                                                                                                            *
 *                                                                                                                                      *
 *  File:       adm_app_manager.c                                                                                                                *
 *                                                                                                                                      *
 ****************************************************************************************************************************************/

/*
 * INCLUDES
 */
#include <empi.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <fcntl.h>

/*
 * CONSTANTS
 */

#define ADM_DEFAULT_WORKSIZE    100
#define ADM_DEFAULT_MAX_ITER    100

/*
 * TYPES
 */

struct ADM_attributes {
    char *key;
    void *value;
    int size;
    struct ADM_attributes * next;
};

/*
 * GLOBAL VARIABLES
 */
struct ADM_attributes * ADM_GLOBAL_attributes_list = NULL;
int ADM_GLOBAL_attributes_list_size = 0;

/* STOP AND RESTART vars */
int ADM_GLOBAL_checkpoint_active = 20;


/*
 * FORTRAN HEADERS 
 */
#ifdef __cplusplus
extern "C" {
#endif
    void ADM_MonitoringService_(int *cmd);
    void ADM_MalleableRegion_(int *cmd, int *ret);
    void ADM_RegisterSysAttributesInt_(char* key, int *val);
    void ADM_RegisterSysAttributesIntArr_(char* key, int *val, int *size);
    void ADM_RegisterSysAttributesDouble_(char* key, double *val);
    void ADM_RegisterSysAttributesDoubleArr_(char* key, double *val, int *size);
    void ADM_RegisterSysAttributesStr_(char* key, char *val, int *size);
    void ADM_GetSysAttributesInt_(char* key, int *val);
    void ADM_GetSysAttributesIntArr_(char* key, int *val, int *size, int *ret);
    void ADM_GetSysAttributesDouble_(char* key, double *val);
    void ADM_GetSysAttributesDoubleArr_(char* key, double *val, int *size, int *ret);
    void ADM_GetSysAttributesStr_(char* key, char *val, int *size, int *ret);
    void ADM_SyncProcesses_(int *err);
    void ADM_CheckpointConfirmation_(int *ret);
    void ADM_DoCheckpoint_(int *ret);
    void ADM_Restart_(int *ret);
    void ADM_ICregistration_(int *ret);
    void ADM_Malleability_(int *decision, int *nnodes, char* hostlist);
    void ADM_SetReconfigData_(int *procs_hint, int *excl_nodes_hint);
    void ADM_IccFini_();
#ifdef __cplusplus
}
#endif




/**
 * Calls from the IC to the Application Manager
 */

/**
 * ADM_SpawnThread
 *  input : int *threadList: list of the number of threads created in each compute node
 *  input : char **computeNodes: list of the compute nodes where the threads are created
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_SpawnThread (int *threadList, char **computeNodes)
{
    return 0;
}

/**
 * ADM_RemoveThread
 *  input : int *threadList: list of the number of threads removed in each compute node
 *  input : char **computeNodes: list of the compute nodes where the threads are destroyed
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RemoveThread (int *threadList, char **computeNodes)
{
    return 0;
}

/**
 * ADM_SpawnProcess
 *  input : int *threadList: list of the number of threads created in each compute node
 *  input : char **computeNodes: list of the compute nodes where the threads are created
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_SpawnProcess (process_list_t *processNodes)
{
    int ret = 0;
    
    pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
    
    if(EMPI_GLOBAL_listrm[0] >0) {
        fprintf(stderr, "ADM_SpawnProcess(%d): Command ignored: previous command already being processed. Try again later \n", getpid());
        ret = -1;
        
    } else{ // Asigna el incremento de procesos (deltaP) a cada clase
        int proc_increment = 0;
        do {
            int id = add_host (processNodes[proc_increment].name, 0, 0, processNodes[proc_increment].num_proc, processNodes[proc_increment].name);
            fprintf(stderr, "ADM_SpawnProcess(%d): proc_increment:%d, computeNodes[proc_increment]: %s,processList[proc_increment]: %d \n", getpid(), proc_increment, processNodes[proc_increment].name, processNodes[proc_increment].num_proc);
            for (int n_classes = 0; n_classes < EMPI_GLOBAL_nhclasses; n_classes++) {
                fprintf(stderr, "ADM_SpawnProcess(%d): EMPI_GLOBAL_nprocs_class[0][n_classes]: %d,EMPI_GLOBAL_hclasses[n_classes]: %s \n", getpid(), EMPI_GLOBAL_nprocs_class[0][n_classes], EMPI_GLOBAL_hclasses[n_classes]);
                
                if (strcmp (EMPI_GLOBAL_hclasses[n_classes], processNodes[proc_increment].name) == 0 && processNodes[proc_increment].num_proc!=0) {
                    EMPI_GLOBAL_nprocs_class[0][n_classes] = processNodes[proc_increment].num_proc;
                    // Delta proc (increment/decrement in the proc number)
                    fprintf(stderr, "ADM_SpawnProcess(%d): Create %d processes in compute node: %s \n", getpid(), EMPI_GLOBAL_nprocs_class[0][n_classes], EMPI_GLOBAL_hclasses[n_classes]);
                    EMPI_GLOBAL_listrm[0] = 1;
                }
            }
            proc_increment++;
        } while (0!=strcmp(processNodes[proc_increment].name,""));
    }
    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
    return ret;
}

/**
 * ADM_RemoveProcess
 *  input : int *threadList: list of the number of threads removed in each compute node
 *  input : char **computeNodes: list of the compute nodes where the threads are destroyed
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RemoveProcess (process_list_t *processNodes)
{
    int ret = 0;
    
    pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
    
    if(EMPI_GLOBAL_listrm[0] >0) {
        fprintf(stderr, "ADM_RemoveProcess(%d): Command ignored: previous command already being processed. Try again later \n", getpid());
        ret = -1;
        
    } else{ // Asigna el incremento de procesos (deltaP) a cada clase
        int proc_increment = 0;
        do {
            fprintf(stderr, "ADM_RemoveProcess(%d): proc_increment:%d, computeNodes[proc_increment]: %s,processList[proc_increment]: %d \n", getpid(), proc_increment,processNodes[proc_increment].name, processNodes[proc_increment].num_proc);
            for (int n_classes = 0; n_classes < EMPI_GLOBAL_nhclasses; n_classes++) {
                fprintf(stderr, "ADM_RemoveProcess(%d): EMPI_GLOBAL_nprocs_class[0][n_classes]: %d,EMPI_GLOBAL_hclasses[n_classes]: %s \n", getpid(), EMPI_GLOBAL_nprocs_class[0][n_classes], EMPI_GLOBAL_hclasses[n_classes]);
                
                if (strcmp (EMPI_GLOBAL_hclasses[n_classes], processNodes[proc_increment].name) == 0 && processNodes[proc_increment].num_proc!=0) {
                    EMPI_GLOBAL_nprocs_class[0][n_classes] = (-1) * processNodes[proc_increment].num_proc;
                    // Delta proc (increment/decrement in the proc number)
                    fprintf(stderr, "ADM_RemoveProcess(%d): Remove %d processes in compute node: %s \n", getpid(), EMPI_GLOBAL_nprocs_class[0][n_classes], EMPI_GLOBAL_hclasses[n_classes]);
                    signal_thread_rpc_release_register(EMPI_GLOBAL_hclasses[n_classes], processNodes[proc_increment].num_booked_proc);
                    fprintf(stderr, "ADM_RemoveProcess(%d): ICC release %d nodes in compute node: %s \n", getpid(), processNodes[proc_increment].num_proc, EMPI_GLOBAL_hclasses[n_classes]);
                }
                EMPI_GLOBAL_listrm[0] = 1;
            }
            proc_increment++;
        } while (0!=strcmp(processNodes[proc_increment].name,""));
    }
    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
    return ret;
}

/**
 * ADM_ReleaseProcesses
 *  input : int *threadList: list of the number of threads removed in each compute node
 *  input : char **computeNodes: list of the compute nodes where the threads are destroyed
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_ReleaseProcesses (process_list_t *processNodes)
{
    int ret = 0;
    int proc_increment = 0;
    do {
        fprintf(stderr, "ADM_ReleaseProcesses(%d): proc_increment:%d, computeNodes[proc_increment]: %s,processList[proc_increment]: %d \n", getpid(), proc_increment,processNodes[proc_increment].name, processNodes[proc_increment].num_proc);
        signal_thread_rpc_release_register(processNodes[proc_increment].name, processNodes[proc_increment].num_booked_proc);
        proc_increment++;
    } while (0!=strcmp(processNodes[proc_increment].name,""));

    return ret;
}


/**
 * Init Call from MPI_Init
 */
/**
 * ADM_Init
 *  input : argc: number of command line parameters
 *  input : argv: array of command line parameters
 *  input : world_rank: rank of the process
 *  input : world_size: total number of processes
 */
int ADM_Init (int argc, char **argv, int world_rank, int world_size)
{
    int ret = 0;
    int maxiter=ADM_DEFAULT_MAX_ITER;
    int worksize=ADM_DEFAULT_WORKSIZE;
    int hint_num_process = 0;
    int hint_excl_nodes = 0;
    int offset=0, slice=0;
    int *array_slice = NULL, *array_offset = NULL;
    array_slice = (int*) malloc (world_size * sizeof(int));
    array_offset = (int*) malloc (world_size * sizeof(int));

    //fprintf (stderr, "ADM_Init: begin \n");

    //fprintf (stderr, "ADM_Init: EMPI_Register_size \n");
    EMPI_Register_size(worksize);
    //fprintf (stderr, "ADM_Init: EMPI_Get_wsize \n");
    EMPI_Get_wsize (world_rank, world_size, worksize, &offset, &slice, array_slice, array_offset);
    
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_ITERATION %d \n",EMPI_GLOBAL_iteration);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_ITERATION", (void *)&EMPI_GLOBAL_iteration, sizeof(int));
    assert(ret == 0);
    
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_MAX_ITERATION %d \n",maxiter);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_MAX_ITERATION", (void *)&maxiter, sizeof(int));
    assert(ret == 0);
    
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_WORKSIZE %d \n",worksize);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_WORKSIZE", (void *)&worksize, sizeof(int));
    assert(ret == 0);
    
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_DATA_SLICE %d \n",slice);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_DATA_SLICE", (void *)&slice, sizeof(int));
    assert(ret == 0);
    
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_DATA_OFFSET %d \n",offset);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_DATA_OFFSET", (void *)&offset, sizeof(int));
    assert(ret == 0);
    
    //fprintf (stderr, "ADM_MalleableRegion: ADM_GLOBAL_ARRAY_DATA_SLICE %p(%ld)(%d,%d) \n",array_slice,world_size * sizeof(int),array_slice[0],array_slice[1]);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_ARRAY_DATA_SLICE", (void *)array_slice, world_size * sizeof(int));
    assert(ret == 0);
    
    //fprintf (stderr, "ADM_MalleableRegion: ADM_GLOBAL_ARRAY_DATA_OFFSET %p(%ld)(%d,%d) \n",array_offset,world_size * sizeof(int),array_offset[0],array_offset[1]);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_ARRAY_DATA_OFFSET", (void *)array_offset, world_size * sizeof(int));
    assert(ret == 0);
    
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_ARGV %p \n",argv);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_ARGV", (void *)&argv, sizeof(char *));
    assert(ret == 0);
    
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_WORLD_RANK %d \n",world_rank);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_WORLD_RANK", (void *)&world_rank, sizeof(int));
    assert(ret == 0);
    
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_WORLD_SIZE %d \n",world_size);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_WORLD_SIZE", (void *)&world_size, sizeof(int));
    assert(ret == 0);
    
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_HINT_NUM_PROCESS %d \n",hint_num_process);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_HINT_NUM_PROCESS", (void *)&hint_num_process, sizeof(int));
    assert(ret == 0);
   
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_HINT_EXCL_NODES %d \n",hint_excl_nodes);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_HINT_EXCL_NODES", (void *)&hint_excl_nodes, sizeof(int));
    assert(ret == 0);

    int type;
    EMPI_Get_type (&type);
    //fprintf (stderr, "ADM_Init: ADM_GLOBAL_PROCESS_TYPE %d \n",type);
    ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_PROCESS_TYPE", (void *)&type, sizeof(int));
    assert(ret == 0);

    //fprintf (stderr, "ADM_Init: end \n");

    return 0;
}
/**
 * Calls from the Application to the Application Manager
 */

/**
 * ADM_MonitoringService
 *  input : int command: Action to be done on the given target nodes. START/STOP
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_MonitoringService (int command)
{
    return 0;
}

/**
 * ADM_MalleableRegion
 *  input : START /STOP flag identifying the begin (START), and the end (ST OP ) of the code region.
 *  output: exitValue: Command==ADM_SERVICE_START -> 0 success, -1 failure
 *               Command==ADM_SERVICE_STOP -> ADM_ACTIVE process active,
 *                                           ADM_REMOVED process removed,
 *                                           -1 failure
 */
int ADM_MalleableRegion (int command)
{
    int ret = 0;
    int world_rank, world_size;
    int iter, maxiter;
    int worksize;
    int proc_status;
    int hint_num_process;
    int hint_excl_nodes;
    int offset, slice;
    int *array_slice = NULL, *array_offset = NULL;
    char **argv = NULL;
        
    int status = -1;
    fprintf (stderr, "ADM_MalleableRegion(%d): begin \n", getpid());

    if (command == ADM_SERVICE_START) {
        fprintf (stderr, "ADM_MalleableRegion(%d): command == ADM_SERVICE_START \n", getpid());

        /* get input values */
        ret = ADM_GetSysAttributes ("ADM_GLOBAL_WORLD_RANK", (void *)&world_rank, sizeof(int));
        assert (ret==0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_WORLD_RANK %d \n", getpid(), world_rank);
        
        ret = ADM_GetSysAttributes ("ADM_GLOBAL_HINT_NUM_PROCESS", (void *)&hint_num_process, sizeof(int));
        assert (ret==0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_HINT_NUM_PROCESS %d \n", getpid(), hint_num_process);

        ret = ADM_GetSysAttributes ("ADM_GLOBAL_HINT_EXCL_NODES", (void *)&hint_excl_nodes, sizeof(int));
        assert (ret==0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_HINT_EXCL_NODES %d \n", getpid(), hint_num_process);

        /* exec monitor init */
        fprintf (stderr, "ADM_MalleableRegion(%d): EMPI_Monitor_init (%d,%d)\n", getpid(), world_rank, hint_num_process);
        EMPI_Monitor_init (&world_rank, hint_num_process, hint_excl_nodes);
        /* NOTE EMPI_Monitor_init do not modify world_rank */

        // reset ADM_GLOBAL_HINT_NUM_PROCESS
        hint_num_process = 0;
        //fprintf (stderr, "ADM_Init: ADM_GLOBAL_HINT_NUM_PROCESS %d \n",hint_num_process);
        ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_HINT_NUM_PROCESS", (void *)&hint_num_process, sizeof(int));
        assert(ret == 0);
       
        // reset ADM_GLOBAL_HINT_EXCL_NODES
        hint_excl_nodes = 0;
        //fprintf (stderr, "ADM_Init: ADM_GLOBAL_HINT_EXCL_NODES %d \n",hint_excl_nodes);
        ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_HINT_EXCL_NODES", (void *)&hint_excl_nodes, sizeof(int));
        assert(ret == 0);

        /* return success status */
        status = -2;
        
    } else if (command == ADM_SERVICE_STOP) {
        fprintf (stderr, "ADM_MalleableRegion(%d): command == ADM_SERVICE_STOP \n", getpid());

        /* get input values */
        ret = ADM_GetSysAttributes ("ADM_GLOBAL_WORLD_RANK", (void *)&world_rank, sizeof(int));
        assert(ret == 0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_WORLD_RANK %d \n", getpid(), world_rank);

        ret = ADM_GetSysAttributes ("ADM_GLOBAL_WORLD_SIZE", (void *)&world_size, sizeof(int));
        assert(ret == 0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_WORLD_SIZE %d \n", getpid(), world_size);

        ret = ADM_GetSysAttributes ("ADM_GLOBAL_ITERATION", (void *)&iter, sizeof(int));
        assert(ret == 0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_ITERATION %d \n", getpid(), iter);
        
        ret = ADM_GetSysAttributes ("ADM_GLOBAL_MAX_ITERATION", (void *)&maxiter, sizeof(int));
        assert(ret == 0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_MAX_ITERATION %d \n", getpid(), maxiter);

        ret = ADM_GetSysAttributes ("ADM_GLOBAL_DATA_SLICE", (void *)&slice, sizeof(int));
        assert(ret == 0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_DATA_SLICE %d \n", getpid(), slice);

        ret = ADM_GetSysAttributes ("ADM_GLOBAL_DATA_OFFSET", (void *)&offset, sizeof(int));
        assert(ret == 0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_DATA_OFFSET %d \n", getpid(), offset);

        /* get ADM_GLOBAL_ARRAY_DATA_SLICE size */
        ret = ADM_GetSysAttributes ("ADM_GLOBAL_ARRAY_DATA_SLICE", NULL, 0);
        assert(ret > 0);
        array_slice = (int*) malloc (ret);
        ret = ADM_GetSysAttributes ("ADM_GLOBAL_ARRAY_DATA_SLICE", (void *)array_slice, ret);
        assert(ret == 0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_ARRAY_DATA_SLICE\n", getpid());
        //fprintf (stderr, "ADM_MalleableRegion: ADM_GLOBAL_ARRAY_DATA_SLICE %p(%ld)(%d,%d) \n",array_slice,world_size * sizeof(int),array_slice[0],array_slice[1]);

        /* get ADM_GLOBAL_ARRAY_DATA_SLICE size */
        ret = ADM_GetSysAttributes ("ADM_GLOBAL_ARRAY_DATA_SLICE", NULL, 0);
        assert(ret > 0);
        array_offset = (int*) malloc (ret);
        ret = ADM_GetSysAttributes ("ADM_GLOBAL_ARRAY_DATA_OFFSET", (void *)array_offset, ret);
        assert(ret == 0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_ARRAY_DATA_OFFSET\n", getpid());
        //fprintf (stderr, "ADM_MalleableRegion: ADM_GLOBAL_ARRAY_DATA_OFFSET %p(%ld)(%d,%d) \n",array_offset,world_size * sizeof(int),array_offset[0],array_offset[1]);

        ret = ADM_GetSysAttributes ("ADM_GLOBAL_ARGV", (void *)&argv, sizeof(char **));
        assert(ret == 0);
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_ARGV %p(%s) \n", getpid(), argv, argv[0]);

        /* exec monitor end */
        fprintf (stderr, "ADM_MalleableRegion(%d): EMPI_Monitor_end rank=%d\n", getpid(), world_rank);
        EMPI_Monitor_end (&world_rank, &world_size, iter, maxiter, &slice, &offset, &array_slice, &array_offset, NULL, argv+1, argv[0]);
        /* NOTE EMPI_Monitor_end do not modify argv */
        
        /* increment number of iteration */
        //iter++ ;
        fprintf (stderr, "ADM_MalleableRegion(%d): iter=%d \n", getpid(), iter);

        /* get output values */
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_WORLD_RANK %d \n", getpid(), world_rank);
        ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_WORLD_RANK", (void *)&world_rank, sizeof(int));
        assert(ret == 0);
        
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_WORLD_SIZE %d \n", getpid(), world_size);
        ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_WORLD_SIZE", (void *)&world_size, sizeof(int));
        assert(ret == 0);
        
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_ITERATION %d \n", getpid(), iter);
        ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_ITERATION", (void *)&iter, sizeof(int));
        assert(ret == 0);
        
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_MAX_ITERATION %d \n", getpid(), maxiter);
        ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_MAX_ITERATION", (void *)&maxiter, sizeof(int));
        assert(ret == 0);
        
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_DATA_SLICE %d \n", getpid(), slice);
        ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_DATA_SLICE", (void *)&slice, sizeof(int));
        assert(ret == 0);
        
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_DATA_OFFSET %d \n", getpid(), offset);
        ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_DATA_OFFSET", (void *)&offset, sizeof(int));
        assert(ret == 0);
        
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_ARRAY_DATA_SLICE %p(%ld)(%d,%d) \n", getpid(), array_slice, world_size * sizeof(int), array_slice[0], array_slice[1]);
        ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_ARRAY_DATA_SLICE", (void *)array_slice, world_size * sizeof(int));
        assert(ret == 0);
        
        //fprintf (stderr, "ADM_MalleableRegion(%d): ADM_GLOBAL_ARRAY_DATA_OFFSET %p(%ld)(%d,%d) \n", getpid(), array_offset,world_size * sizeof(int), array_offset[0], array_offset[1]);
        ret = ADM_RegisterSysAttributes ("ADM_GLOBAL_ARRAY_DATA_OFFSET", (void *)array_offset, world_size * sizeof(int));
        assert(ret == 0);
        
        /* get status active or removed */
        EMPI_Get_status (&status);
        fprintf (stderr, "ADM_MalleableRegion(%d): get status active(%d) or removed(%d): status=%d\n",getpid(), EMPI_ACTIVE, EMPI_REMOVED, status);
        
        /* free arrays */
        free(array_slice);
        free(array_offset);

    }
    fprintf (stderr, "ADM_MalleableRegion(%d): return: status=%d \n", getpid() ,status);
    return status;
}


/**
 * ADM_RegisterSysAttributesInt
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to the integer value
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RegisterSysAttributesInt(char* key, int *val)
{
    return ADM_RegisterSysAttributes(key, (void*)val, sizeof(int));
}

/**
 * ADM_RegisterSysAttributesIntArr
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to the first integer value if the array
 *       size:   number of elements on the array
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RegisterSysAttributesIntArr(char* key, int *val, int size)
{
    return ADM_RegisterSysAttributes(key, (void*)val, sizeof(int)*size);
}

/**
 * ADM_RegisterSysAttributesDouble
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to the double value
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RegisterSysAttributesDouble(char* key, double *val)
{
    return ADM_RegisterSysAttributes(key, (void*)val, sizeof(double));
}

/**
 * ADM_RegisterSysAttributesDoubleArr
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to the first integer value if the array
 *       size:   number of elements on the array
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RegisterSysAttributesDoubleArr(char* key, double *val, int size)
{
    return ADM_RegisterSysAttributes(key, (void*)val, sizeof(double)*size);
}

/**
 * ADM_RegisterSysAttributesStr
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to the first char of the string array
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RegisterSysAttributesStr(char* key, char *val)
{
    int size = strlen(val)+1;
    return ADM_RegisterSysAttributes(key, (void*)val, size);
}

/**
 * ADM_GetSysAttributesInt
 *  input : key: string that determines the key of the key/value pair
 *  input : value: pointer to an existing integer were to a copy the value,
 *  output: value: pointer to an copy of the value,,
 *        exitValue:    0 success;
 *                -1 not found;
 *                -2 wrong size
 */
int ADM_GetSysAttributesInt(char* key, int *val)
{
    // check key and its size
    int size = ADM_GetSysAttributes(key, NULL, 0);
    if (size < 0) {
        //resend the error
        return size;
    }else if (sizeof(int) != size) {
        // size incorrect for int
        return -2;
    } else {
        // return value
        return ADM_GetSysAttributes(key, (void*)val, sizeof(int));
    }
}

/**
 * ADM_GetSysAttributesIntArr
 *  input : key: string that determines the key of the key/value pair
 *  input : value: pointer to an existing integer array were to a copy the value,
 *       size: size of memory buffer in integer elements
 *  output: value: pointer to an copy of the value,,
 *        exitValue:  >0 success (number of array elements)
 *                -1 not found;
 *                -2 wrong size
 */
int ADM_GetSysAttributesIntArr(char* key, int *val, int size)
{
    // check key and its size
    int aux_size = ADM_GetSysAttributes(key, NULL, 0);
    if (aux_size < 0) {
        //resend the error
        return aux_size;
    }else if ((aux_size % sizeof(int) != 0) ||
              (sizeof(int)*size < aux_size) ) {
        // wrong size
        return -2;
    } else {
        // return value
        ADM_GetSysAttributes(key, (void*)val, aux_size);
        return aux_size/sizeof(int);
    }
}

/**
 * ADM_GetSysAttributesInt
 *  input : key: string that determines the key of the key/value pair
 *  input : value: pointer to an existing double were to a copy the value,
 *  output: value: pointer to an copy of the value,,
 *        exitValue:    0 success;
 *                -1 not found;
 *                -2 wrong size
 */
int ADM_GetSysAttributesDouble(char* key, double *val)
{
    // check key and its size
    int size = ADM_GetSysAttributes(key, NULL, 0);
    if (size < 0) {
        //resend the error
        return size;
    }else if (sizeof(double) != size) {
        // size incorrect for int
        return -2;
    } else {
        // return value
        return ADM_GetSysAttributes(key, (void*)val, sizeof(double));
    }}

/**
 * ADM_GetSysAttributesDoubleArr
 *  input : key: string that determines the key of the key/value pair
 *  input : value: pointer to an existing double array were to a copy the value,
 *       size: size of memory buffer in double elements
 *  output: value: pointer to a copy of the value,,
 *        exitValue:  >0 success (number of array elements)
 *                -1 not found;
 *                -2 wrong size
 */
int ADM_GetSysAttributesDoubleArr(char* key, double *val, int size)
{
    // check key and its size
    int aux_size = ADM_GetSysAttributes(key, NULL, 0);
    if (aux_size < 0) {
        //resend the error
        return aux_size;
    }else if ((aux_size % sizeof(double) != 0) ||
              (sizeof(double)*size < aux_size) ) {
        // wrong size
        return -2;
    } else {
        // return value
        ADM_GetSysAttributes(key, (void*)val, aux_size);
        return aux_size/sizeof(double);
    }
}

/**
 * ADM_GetSysAttributesStr_
 *  input : key: string that determines the key of the key/value pair
 *  input : value: pointer to an existing double array were to a copy the value,
 *       size: size of memory buffer in integer elements
 *  output: value: pointer to a copy of the value,,
 *        exitValue:  >0 success (number of array elements)
 *                -1 not found;
 *                -2 wrong size
 */
int ADM_GetSysAttributesStr(char* key, char *val, int size)
{
    // check key and its size
    int aux_size = ADM_GetSysAttributes(key, NULL, 0);
    if (aux_size < 0) {
        //resend the error
        return aux_size;
    }else if (size < aux_size)  {
        // wrong size
        return -2;
    } else {
        // return value
        ADM_GetSysAttributes(key, (void*)val, aux_size);
        return aux_size;
    }
}

/**
 * ADM_RegisterSysAttributes
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to a memory buffer to store the value
 *       size:  size of the memory buffer that store the value
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  Example of keys:
 *  • IO_BANDWIDTH. Aggregate I/O system bandwidth currently available expressed in Gb/s.
 *  • APP_MANAGER_STATUS. Status of the application manager.
 *  • SLURM_MALLEABLE_CAPABILITIES. Reports whether the SLURM component has malleable features enabled.
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RegisterSysAttributes (char *key, void *value, int size)
{
    struct ADM_attributes *data = NULL;
    int count = 0;

    assert ((key != NULL) && (value != NULL));

    //fprintf (stderr, "ADM_RegisterSysAttributes: (%s) begin\n", key);

    /* if list is empty */
    if (ADM_GLOBAL_attributes_list == NULL) {
        //fprintf (stderr, "ADM_RegisterSysAttributes: list is empty \n");
        //fprintf (stderr, "ADM_RegisterSysAttributes: before malloc \n");

        //alloc first element
        data = ADM_GLOBAL_attributes_list = (struct ADM_attributes *) malloc (sizeof (struct ADM_attributes));
        //fprintf (stderr, "ADM_RegisterSysAttributes: after malloc \n");

        assert (ADM_GLOBAL_attributes_list != NULL);
        //fprintf (stderr, "ADM_RegisterSysAttributes: after assert \n");

        //set empty data
        data->key = NULL;
        data->value = NULL;
        data->size = 0;
        // set as last element
        data->next = NULL;
        //fprintf (stderr, "ADM_RegisterSysAttributes: list is empty (end) \n");

        
    } else  { /* list is not empty*/
        //fprintf (stderr, "ADM_RegisterSysAttributes: list is not empty \n");
        data = ADM_GLOBAL_attributes_list;
    
        // loop until last element, empty space or found key */
        while ((data->next != NULL)&&(data->key!=NULL)&&(strcmp(data->key, key)!=0)) {
            data = data->next;
            count++;
        }
        
        //fprintf (stderr, "ADM_RegisterSysAttributes: position = %d \n",count);

        //if it is last element, it is not and empty space and it is not equal to key
        if ((data->next == NULL)&&(data->key!=NULL)&&(strcmp(data->key, key)!=0)) {
            //fprintf (stderr, "ADM_RegisterSysAttributes: is last element, it is not and empty space and it is not equal to key \n");

            // alloc another space
            data->next = (struct ADM_attributes *) malloc (sizeof (struct ADM_attributes));
            assert (data->next != NULL);

            // go to next (last) element
            data = data->next;

            //set empty data
            data->key = NULL;
            data->value = NULL;
            data->size = 0;
            // set as last element
            data->next = NULL;
        }
    }
    
    //if the element key already exist
    if ((data->key!=NULL)&&(strcmp(data->key, key)==0)) {
        //fprintf (stderr, "ADM_RegisterSysAttributes: the element key already existy \n");
        // if value == NULL erase whole element
        if (value == NULL) {
            //fprintf (stderr, "ADM_RegisterSysAttributes: value == NULL erase whole element \n");
            //erase key, value and size
            free (data->key);
            data->key = NULL;
            free (data->value);
            data->value = NULL;
            data->size = 0;
            // decrease list size
            ADM_GLOBAL_attributes_list_size--;
        }
        // if size is different
        if (data->size != size) {
            //fprintf (stderr, "ADM_RegisterSysAttributes: if size is different \n");
            //realloc value
            data->value = (void *) realloc (data->value, size);
            assert (data->value != NULL);
        }
        //fprintf (stderr, "ADM_RegisterSysAttributes: copy value and size \n");
        //copy value
        memcpy (data->value, value, size);
        // copy size
        data->size = size;

    } else if (value != NULL) { // it is an empty space and value is not NULL
        //fprintf (stderr, "ADM_RegisterSysAttributes: is an empty space and value is not NULL\n");
        // copy key
        int len = strlen (key);
        data->key = (char *) malloc ((len+1) * sizeof (char));
        assert (data->key != NULL);
        strcpy (data->key, key);
        //copy value
        data->value = (void *) malloc (size);
        assert (data->value != NULL);
        memcpy (data->value, value, size);
        // copy size
        data->size = size;
        // increase list size
        ADM_GLOBAL_attributes_list_size++;
    }
    //fprintf (stderr, "ADM_RegisterSysAttributes: (%s) end\n", key);

    return 0;
}

/**
 * ADM_GetSysAttributes
 *  input :  key: string that determines the key of the key/value pair
 *  input :  value: pointer to an existing buffer were to a copy the value,
 *            if =NULL then value wont be copied and size does not matter
 *        size:  size of the memory buffer set to store the value
 *  output: value: pointer to an copy of the value,,
 *        exitValue: >0 (and value == NULL) value size; 0 success;
 *                -1 not found; -2 wrong size
 */
int ADM_GetSysAttributes (char *key, void *value, int size)
{
    struct ADM_attributes *data = NULL;
    int ret = 0;
    int count = 0;

    assert ((key != NULL));
    
    //fprintf (stderr, "ADM_GetSysAttributes: (%s) begin\n", key);

    data = ADM_GLOBAL_attributes_list;
    
    // loop until empty list or found key */
    while ((data!= NULL)&&(data->key!=NULL)&&(strcmp(data->key, key)!=0)) {
        data = data->next;
        count++;
    }
    
    //fprintf (stderr, "ADM_GetSysAttributes: position = %d \n",count);

    //if the key id found
    if ((data != NULL)&&(data->key!=NULL)&&(strcmp(data->key, key)==0)) {
        //fprintf (stderr, "ADM_GetSysAttributes: he key id found \n");
        if (value != NULL) { /* return value */
            //fprintf (stderr, "ADM_GetSysAttributes: value != NULL return value \n");
            if (size == data->size) { /* correct size buffer */
                //fprintf (stderr, "ADM_GetSysAttributes: correct size buffer \n");
                memcpy (value, data->value, size);
            } else { /* error in buffer size */
                //fprintf (stderr, "ADM_GetSysAttributes: error in buffer size \n");
                ret = -2;
            }
        } else { /* return size */
            //fprintf (stderr, "ADM_GetSysAttributes: value == NULL return size \n");
            ret = data->size;
        }
    } else { /* key not found */
        //fprintf (stderr, "ADM_GetSysAttributes: key not found \n");
        ret = -1;
    }

    //fprintf (stderr, "ADM_GetSysAttributes: (%s) end\n", key);
    
    return ret;
}

/**
 * ADM_ListSysAttributes
 *  input  key_list: previous returned list to be erased (or empty buffer->NULL)
 *  output  key_list:  new allocated buffer array of stored keys
 *        list_size: number of keys stored
 */
void ADM_ListSysAttributes (char ***key_list, int *list_size)
{
    struct ADM_attributes *data = NULL;
    char ** aux_key_list;
    int aux_list_size = 0;
    
    assert ((key_list != NULL) && (list_size != NULL));
    
    //fprintf (stderr, "ADM_ListSysAttributes: begin\n");
    
    // erase previous list
    ADM_EraseListSysAttributes (key_list);

    aux_key_list = (char **) malloc ((ADM_GLOBAL_attributes_list_size+1) * sizeof (char *));
    assert (aux_key_list != NULL);

    data = ADM_GLOBAL_attributes_list;
 
    // loop until last element of found key */
    while (data!= NULL) {
        if ((data->key!=NULL)) {
            // copy key
            int len = strlen (data->key);
            aux_key_list[aux_list_size] = (char *) malloc ((len+1) * sizeof (char));
            assert (aux_key_list[aux_list_size] != NULL);
            strcpy (aux_key_list[aux_list_size], data->key);
            aux_list_size++;
        }
        data = data->next;
    }
    // set last to NULL
    aux_key_list[aux_list_size] = NULL;
    
    // return values;
    (*key_list) = aux_key_list;
    (*list_size) = aux_list_size;
    
    //fprintf (stderr, "ADM_ListSysAttributes: end\n");
}


/**
 * ADM_EraseListSysAttributes
 *  intput  key_list:  array of stored keys
 *  list_size: number of keys stored
 *  intput  key_list:  array of stored keys
 */
void ADM_EraseListSysAttributes (char ***key_list)
{
    int i=0;
    char **aux_key_list;
    
    assert (key_list != NULL);

    //fprintf (stderr, "ADM_EraseListSysAttributes: begin\n");

    aux_key_list = *key_list;
    
    if (aux_key_list != NULL) {
        while (aux_key_list[i] != NULL) {
            free (aux_key_list[i]);
            i++;
        }
        free (aux_key_list);
        // return values;
        (*key_list) = NULL;
    }
    //fprintf (stderr, "ADM_EraseListSysAttributes: end\n");
}


/**
 * ADM_CheckpointConfirmation
 *  input : none
 *  output : 0 if no checkpoint required; 1 otherwhise
 * Hardcoded : checkpoint every 20 iterations 
*/
int ADM_CheckpointConfirmation(){
    //Query IC
    signal_thread_rpc_check_checkpointing();

    return 0;
}

/**
 * ADM_DoCheckpoint
 *  input : none
 *  output : 0 if no checkpoint required; 1 yes
 * 
*/
int ADM_DoCheckpoint(){

    return (ADM_GLOBAL_checkpointing == 1) ? 1 : 0;
}

/**
 * ADM_Restart
 * Prepare the current local files for the restart (sbatch and *.par files)
*/
void ADM_Restart(){
    //update sbatch file and restart the application
    //system("cat RunNek.sbatch | sed -E 's/procs=2/procs=4/' > RunNek.sbatch2");

    /*char * cmd = (char*)calloc(1, 140);
    ADM_GLOBAL_procs += 2;
    sprintf(cmd, "sleep 2 && mpiexec.openmpi -np %d ./nek5000 -policy-malleability-triggered -lbpolicy-static -ni 10 -ports 5000 5001 -controller slurm-node-1 -IOaction 1 -alloc:0", ADM_GLOBAL_procs);
    fprintf(stderr, cmd);
    system(cmd);*/
}

/**
 * ADM_Malleability
 * Asks IC about pending malleability operations
*/
void ADM_Malleability(int *decision, int *nnodes, char *hostlist){
    //query IC and obtain the value
    signal_thread_rpc_malleability_query();
}



/******************************************
 * FORTRAN FUNCTIONS
 ******************************************/


/**
 * ADM_ICregistration_
*/
void ADM_ICregistration_(int *ret){
    signal_thread_rpc_init_ss("slurm-node-1:3");
}

/**
 * ADM_Restart
 * input : if error
*/
void ADM_Restart_(int *ret){
    ADM_Restart();
}

/**
 * ADM_DoCheckpoint
 * input : none
 * output -> ret : 0 if no checkpoint; 1 if checkpoint
*/
void ADM_DoCheckpoint_(int *ret){
    *ret = ADM_DoCheckpoint();
}

/**
 * ADM_CheckpointConfirmation
 * input :  pointer ret : 0 if no checkpoint; 1 if checkpoint
 * output : none
*/
void ADM_CheckpointConfirmation_(int *ret){
    *ret = ADM_CheckpointConfirmation();
}

/**
 * ADM_Malleability
 * input: pointers to store if malleability required, new nnodes (+/-), and hostlist to add/remove
 * output: node
*/
void ADM_Malleability_(int *decision, int *nnodes, char* hostlist){
    ADM_Malleability(decision, nnodes, hostlist);
}

/**
 * Call ADM_MonitoringService from Fortran
 */
void ADM_MonitoringService_(int *cmd){
    ADM_MonitoringService(*cmd);
}

/**
 * Call ADM_MalleableRegion from Fortran
 */
void ADM_MalleableRegion_(int *cmd, int *ret){
    *ret = ADM_MalleableRegion(*cmd);
}

/**
 * Call ADM_RegisterSysAttributes from Fortran with specific argument INT
 */
void ADM_RegisterSysAttributesInt_(char* key, int *val){
    ADM_RegisterSysAttributesInt(key, val);
}

/**
 * Call ADM_RegisterSysAttributes from Fortran with specific argument INT[]
 */
void ADM_RegisterSysAttributesIntArr_(char* key, int *val, int *size){
    ADM_RegisterSysAttributesIntArr(key, val, *size);
}

/**
 * Call ADM_RegisterSysAttributes from Fortran with specific argument DOUBLE
 */
void ADM_RegisterSysAttributesDouble_(char* key, double *val){
    ADM_RegisterSysAttributesDouble(key, val);
}

/**
 * Call ADM_RegisterSysAttributes from Fortran with specific argument DOUBLE[]
 */
void ADM_RegisterSysAttributesDoubleArr_(char* key, double *val, int *size){
    ADM_RegisterSysAttributesDoubleArr(key, val, *size);
}

/**
 * Call ADM_RegisterSysAttributes from Fortran with specific argument STRING
 */
void ADM_RegisterSysAttributesStr_(char* key, char *val, int *size){
    ADM_RegisterSysAttributesStr(key, val);
    //ADM_RegisterSysAttributes(key, (void*)val, sizeof(char)*(*size));
}

/**
 * Call ADM_GetSysAttributes from Fortran with specific argument INT
 */
void ADM_GetSysAttributesInt_(char* key, int *val){
    ADM_GetSysAttributesInt(key, val);
}

/**
 * Call ADM_GetSysAttributes from Fortran with specific argument INT[]
 */
void ADM_GetSysAttributesIntArr_(char* key, int *val, int *size, int *ret){
    ADM_GetSysAttributesIntArr(key, val, *size);
}

/**
 * Call ADM_GetSysAttributes from Fortran with specific argument DOUBLE
 */
void ADM_GetSysAttributesDouble_(char* key, double *val){
    ADM_GetSysAttributesDouble(key, val);
}

/**
 * Call ADM_GetSysAttributes from Fortran with specific argument DOUBLE[]
 */
void ADM_GetSysAttributesDoubleArr_(char* key, double *val, int *size, int * ret){
    ADM_GetSysAttributesDoubleArr(key, val, *size);
}

/**
 * Call ADM_GetSysAttributes from Fortran with specific argument STRING
 */
void ADM_GetSysAttributesStr_(char* key, char *val, int *size, int *ret){
    ADM_GetSysAttributesStr(key, val, *size);
}

/**
 * Based on different tests, Fortran needs to sync spawned processes by means
 * of a collective. That is why a non-sense Bcast takes place here.
 */
int proccount = 0;
void ADM_SyncProcesses_(int *err){
    //ALBERTO: Sync native and spawned procs
    int rank, size;
    MPI_Comm_rank(EMPI_COMM_WORLD, &rank);
    MPI_Comm_size(EMPI_COMM_WORLD, &size);
    //fprintf(stderr, "SYNC PROCESSES: rank %d/%d in Barrier SyncProcesses \n", rank, size);
    
    //Test bcast
    /*int aux = 0;
    if (rank == size-1) aux = 21;
    PMPI_Bcast(&aux, 1, MPI_INT, size-1, EMPI_COMM_WORLD);
    fprintf(stderr,"SYNC PROCESSES: rank %d/%d get=%d\n", rank, size, aux);
    MPI_Barrier(EMPI_COMM_WORLD); //After spawn 2 procs, spawned procs fails here.*/

    //ALBERTO: Sync native and spawned procs in the communicator.
    int sval[size*2];// = (int*)malloc(2*sizeof(int)*size); //2 per proc (new spawned procs waits for that number...)
    if (rank == 0){
        sval[0] = proccount+1;
        sval[1] = proccount+1;
        proccount++;
    }
    //printf("RANK %d/%d Syncing procs...\n", rank, size);
    //sleep(2);

    int type;
    EMPI_Get_type(&type);
    fprintf(stderr,"SYNC PROCESSES: rank %d/%d with type=%d are going to do Bcast\n", rank, size, type);
    if(type == EMPI_NATIVE || type == EMPI_SPAWNED_SYNC){
        PMPI_Bcast (&sval, 2, MPI_INT, EMPI_root, EMPI_COMM_WORLD);
    } else {
        //PMPI_Bcast (&sval, 2*size, MPI_INT, EMPI_root, EMPI_COMM_WORLD); //sync
        PMPI_Bcast (&sval, 2, MPI_INT, EMPI_root, EMPI_COMM_WORLD); //official collective bcast
        EMPI_Set_type(EMPI_SPAWNED_SYNC);
    }

    fprintf(stderr, "SYNC PROCESSES: rank %d/%d synchronized\n", rank, size);
    //free(sval);
}

/**
 * This call sets the reconfiguration data for malleability in stop-start applications
*/
void ADM_SetReconfigData_(int *procs_hint, int *excl_nodes_hint){
    static int iter = 0;
    /*SetReconfigDataIterationInit(*procs_hint, *excl_nodes_hint);
    SetReconfigDataIterationEnd();*/
    
    ADM_RegisterSysAttributesInt("ADM_GLOBAL_HINT_NUM_PROCESS", procs_hint);
    ADM_RegisterSysAttributesInt("ADM_GLOBAL_HINT_EXCL_NODES", excl_nodes_hint);
    if(iter == 0){
        ADM_RegisterSysAttributesInt("ADM_GLOBAL_ITERATION", &iter);
        ADM_MalleableRegion(ADM_SERVICE_START);
    }

    ADM_MalleableRegion(ADM_SERVICE_STOP);
    iter++;
    ADM_RegisterSysAttributesInt("ADM_GLOBAL_ITERATION", &iter);    
    ADM_MalleableRegion(ADM_SERVICE_START);

}

/**
 * This call removes the client information in the IC database
 * It should be called after the MPI_Finalize to clear all the information.
*/
void ADM_IccFini_(){
    signal_thread_rpc_icc_fini();
}
