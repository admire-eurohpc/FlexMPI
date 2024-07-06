/**
* @version        FlexMPI v3.1
* @copyright    Copyright (C) 2018 Universidad Carlos III de Madrid. All rights reserved.
* @license        GNU/GPL, see LICENSE.txt
* This program is free software: you can redistrie it and/or modify
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
 *  File:       monitor.c                                                                                                               *
 *                                                                                                                                      *
 ****************************************************************************************************************************************/

/* include */
#include <empi.h>
#include <papi.h>

#include <hiredis/hiredis.h>


/* headers */

/****************************************************************************************************************************************
*
*   'EMPI_Iteration_init'
*
****************************************************************************************************************************************/
static void EMPI_Iteration_init (void);

/****************************************************************************************************************************************
*
*   'EMPI_Iteration_end'
*
****************************************************************************************************************************************/
static void EMPI_Iteration_end (long long *rtime, long long *ptime, long long *flops, long long *hwpc_1, long long *hwpc_2, double *ctime, double *iotime);

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_efficiency'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_efficiency (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin);

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_efficiency_irregular'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_efficiency_irregular (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin);

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_cost'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_cost (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin);

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_cost_irregular'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_cost_irregular (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin);

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_malleability'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_malleability (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin);

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_malleability_conditional'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_malleability_conditional (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin);


/****************************************************************************************************************************************
*
*   'EMPI_Monitor_malleability_triggered'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_malleability_triggered (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin);


/****************************************************************************************************************************************
*
*   'EMPI_Comm_cost'
*
****************************************************************************************************************************************/
static void EMPI_Comm_cost (double *cost, int size, int newsize, int niter);

/****************************************************************************************************************************************
*
*   'EMPI_Comp_cost'
*
****************************************************************************************************************************************/
static void EMPI_Comp_cost (double *cost_flops, double *cost_rtime, long long *flop, int mflops, int newsize);

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_spawn'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_spawn (int *rank, int *size, int nprocs, int count, int disp, char *argv[], char *bin, EMPI_Monitor_type *smonitor, int *hostid, int* fc);

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_remove'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_remove (int *rank, int *size, int nprocs, int count, int disp, EMPI_Monitor_type *smonitor, int* rremvs, int* fc);

/****************************************************************************************************************************************
*
*   'EMPI_Sort'
*
****************************************************************************************************************************************/
static void EMPI_Sort (int* unsortedArray, int* index, int left, int right);

/*
 *  CHANGE JAVIER
 */


/****************************************************************************************************************************************
*
*   'EMPI_Inc_Cost_struct'
*
****************************************************************************************************************************************/

void EMPI_Inc_Cost_struct (int rank, double timestep, int lastiter) {
    
    EMPI_Class_type *class = NULL;

    if (rank == EMPI_root) {

        //registrar configuracion para costes
        EMPI_GLOBAL_cost[EMPI_GLOBAL_nc].timestep = timestep;

        class = EMPI_GLOBAL_system_classes;

        for (int n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

            EMPI_GLOBAL_cost[EMPI_GLOBAL_nc].classes[n] = (class->nprocs+class->iniprocs);

            class = class->next;
        }

        //NOT last iter, increment iter number
        if (lastiter == 0) {
            EMPI_GLOBAL_nc ++;
            
        //last iter, get cumulative results
        } else {
            //Funcion que hace calculo de coste acumulado desde nc=0 hasta nc<EMPI_GLOBAL_nc
            for (int n = 0; n < EMPI_GLOBAL_nc; n ++) {

                class = EMPI_GLOBAL_system_classes;

                for (int c = 0; c < EMPI_GLOBAL_nhclasses; c ++) {

                    EMPI_GLOBAL_cum_cost += (EMPI_GLOBAL_cost[n+1].classes[c] * (EMPI_GLOBAL_cost[n+1].timestep-EMPI_GLOBAL_cost[n].timestep) * class->icost);

                    class = class->next;
                }
            }
        }
    }
}


/****************************************************************************************************************************************
*
*   'EMPI_NoMonitor_init'
*
****************************************************************************************************************************************/
void EMPI_NoMonitor_init (int *rank, int procs_hint, int excl_nodes_hint) {

    int retval;
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_NoMonitor_init in <%s> ***\n", __FILE__);
    #endif
    //wait for all processes
    if (EMPI_GLOBAL_concurrency == EMPI_TRUE) {

        /* CHANGE: begin */
        PMPI_Barrier (EMPI_COMM_WORLD);
        /* CHANGE: end */

        EMPI_GLOBAL_concurrency = EMPI_FALSE;

        //capture communications
        EMPI_GLOBAL_capture_comms = 1;
    }

    /* CHANGE: BEGIN */
    if (*rank == EMPI_root) {
        signal_thread_rpc_malleability_enter_region(procs_hint, excl_nodes_hint);
    }
    /* CHANGE: END */

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_NoMonitor_init in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_NoMonitor_end_Increment'
*
****************************************************************************************************************************************/
void EMPI_InitAndInc_NoMonitor_end (int rank, int iter, int maxiter) {
    
    long long rtime = 0.0, ptime = 0.0, flops = 0.0, hwpc_1 = 0.0, hwpc_2 = 0.0;
    double ctime = 0.0,iotime=0.0;
    EMPI_Class_type *class = NULL;
    
    //Iniciar Estructura de coste ¡SOLO LA PRIMERA VEZ y solo el root!
    if ((rank == EMPI_root)&&(EMPI_GLOBAL_initnc == EMPI_TRUE)) {

        EMPI_GLOBAL_initnc = EMPI_FALSE;

        int nc = 2;

        if (EMPI_GLOBAL_mpolicy != EMPI_NULL) {

            if ((maxiter % EMPI_GLOBAL_niter) == 0)

                nc += ((maxiter / EMPI_GLOBAL_niter) - 1);
            else
                nc += (maxiter / EMPI_GLOBAL_niter);
        }

        //crear estructura para coste
        EMPI_GLOBAL_cost = (EMPI_Cost_type*) malloc (nc * sizeof(EMPI_Cost_type));
        assert (EMPI_GLOBAL_cost);

        EMPI_GLOBAL_nc = 0;

        // init the values of the cost struct
        EMPI_Inc_Cost_struct (rank, EMPI_GLOBAL_iterative_ini,0);
    }
    
    //set iteration number
    EMPI_GLOBAL_iteration = iter;
    
    //Iteration end
    EMPI_Iteration_end (&rtime, &ptime, &flops, &hwpc_1, &hwpc_2, &ctime,&iotime);

    // Init monitoring_temp
    EMPI_GLOBAL_ENERGY_monitoring_temp[0] = -1;
    EMPI_GLOBAL_ENERGY_monitoring_temp[1] = -1;
    EMPI_GLOBAL_ENERGY_monitoring_temp[2] = -1;
    EMPI_GLOBAL_ENERGY_monitoring_temp[3] = -1;
    
    //Overhead time of the monitor function
    EMPI_GLOBAL_tover_ini = PAPI_get_real_usec();

    if (EMPI_GLOBAL_mpolicy == EMPI_MONITORDBG) {
        //set variables
        EMPI_GLOBAL_PAPI_rtime += rtime;
        EMPI_GLOBAL_PAPI_ptime += ptime;
        EMPI_GLOBAL_PAPI_flops += flops;
        EMPI_GLOBAL_PAPI_hwpc_1  += hwpc_1;
        EMPI_GLOBAL_PAPI_hwpc_2  += hwpc_2;
        EMPI_GLOBAL_tcomm_interval += ctime;
        EMPI_GLOBAL_tio_interval   += iotime;
        
    } else if (EMPI_GLOBAL_mpolicy != EMPI_NULL) {

        //set variables
        EMPI_GLOBAL_PAPI_rtime += rtime;
        EMPI_GLOBAL_PAPI_ptime += ptime;
        EMPI_GLOBAL_PAPI_flops += flops;
        EMPI_GLOBAL_PAPI_hwpc_1  += hwpc_1;
        EMPI_GLOBAL_PAPI_hwpc_2  += hwpc_2;
        EMPI_GLOBAL_tcomm_interval += ctime;
        EMPI_GLOBAL_tio_interval   += iotime;

        //more set variables
        EMPI_GLOBAL_PAPI_flops_iteration += flops;
        EMPI_GLOBAL_PAPI_real_flops_iteration += hwpc_1;
        EMPI_GLOBAL_PAPI_it_time += rtime;
        
        //set variables
        EMPI_GLOBAL_PAPI_rtime_lb += rtime;
        EMPI_GLOBAL_PAPI_ptime_lb += ptime;
        EMPI_GLOBAL_PAPI_flops_lb += flops;
        EMPI_GLOBAL_PAPI_hwpc_1_lb  += hwpc_1;
        EMPI_GLOBAL_PAPI_hwpc_2_lb  += hwpc_2;
        EMPI_GLOBAL_tcomm_interval_lb += ctime;
    }
}
    
/****************************************************************************************************************************************
*
*   'EMPI_get_monitor_count'
*
****************************************************************************************************************************************/
int EMPI_get_monitor_count (int count) {
    int *addr_row = NULL, *addr_col = NULL;
    void *addr_val = NULL;
    int ret_count = 0;

    //set count parameter
    /* CHANGE: begin */
    //if (EMPI_GLOBAL_Data->stype == EMPI_SPARSE) {
    if ((EMPI_GLOBAL_Data != NULL) && (EMPI_GLOBAL_Data->stype == EMPI_SPARSE)) {
    /* CHANGE: end */

        if (EMPI_GLOBAL_allocation == EMPI_NULL)
            EMPI_GLOBAL_allocation = EMPI_NNZ;

        if ((EMPI_GLOBAL_allocation == EMPI_NNZ)||(EMPI_GLOBAL_allocation == EMPI_FCOST)) {

            //get nnz count
            EMPI_Get_addr_sparse (EMPI_GLOBAL_Data->id, (void*)&addr_row, (void*)&addr_col, (void*)&addr_val);
            ret_count = addr_row[count];

        } else if (EMPI_GLOBAL_allocation == EMPI_ROWS) {

            //get rows count
            ret_count = count;
        }

    /* CHANGE: begin */
    //} else if ((EMPI_GLOBAL_Data->stype == EMPI_DENSE)||(EMPI_GLOBAL_Data->stype == EMPI_VECTOR)) {
    } else if ( (EMPI_GLOBAL_Data != NULL) &&
        ((EMPI_GLOBAL_Data->stype == EMPI_DENSE)||(EMPI_GLOBAL_Data->stype == EMPI_VECTOR)) ) {
    /* CHANGE: end */

        if (EMPI_GLOBAL_allocation == EMPI_NULL)
            EMPI_GLOBAL_allocation = EMPI_ROWS;

        //get rows count
        ret_count = count;

    } else {

        //non allocated data structure

        //get rows count
        ret_count = count;
    }
    
    return ret_count;

}
/****************************************************************************************************************************************
*
*   'EMPI_fill_smonitor'
*
****************************************************************************************************************************************/
void EMPI_fill_smonitor (int rank, int count, int size, EMPI_Monitor_type **smonitor_orig, int *new_PAPI_policy) {

    size_t len0,len1;
    
    EMPI_Monitor_type monitor;
    EMPI_Monitor_type *smonitor = NULL;
    
    //memory allocation
    (*smonitor_orig) = (EMPI_Monitor_type*) malloc (size * sizeof(EMPI_Monitor_type));
    assert ((*smonitor_orig));
    smonitor = (*smonitor_orig);

    //compute the slice per process depending on data storage type
    monitor.count = EMPI_get_monitor_count (count);
    
    //set monitor parameters
    monitor.flops               = EMPI_GLOBAL_PAPI_flops;
    monitor.rtime               = EMPI_GLOBAL_PAPI_rtime;
    monitor.ptime               = EMPI_GLOBAL_PAPI_ptime;
    monitor.hostid              = EMPI_GLOBAL_hostid;
    monitor.ctime               = EMPI_GLOBAL_tcomm_interval;
    monitor.flops_iteration     = EMPI_GLOBAL_PAPI_flops;
    monitor.it_time             = EMPI_GLOBAL_PAPI_rtime;
    monitor.hwpc_1              = EMPI_GLOBAL_PAPI_hwpc_1;
    monitor.hwpc_2              = EMPI_GLOBAL_PAPI_hwpc_2;
    monitor.iotime              = EMPI_GLOBAL_tio_interval;
    monitor.EMPI_array_alloc    = EMPI_array_alloc;
    len0=strlen(monitor.nhwpc_1)+1;
    len1=strlen(monitor.nhwpc_2)+1;
    
    
    // Only rank0 access to the server
    if(rank == EMPI_root){
        // We need to protect it because the server access to EMPI_GLOBAL_PAPI_nhwpc_* when option 7 is used
        pthread_mutex_lock(&EMPI_GLOBAL_server_lock); // Only the server has an attached thread

            // This works for the rank=0 process. Detects if there are PAPI event values
            if (strncmp(EMPI_GLOBAL_PAPI_nhwpc_1,EMPI_GLOBAL_monitoring_data.nhwpc_1,len0)!=0 || strncmp(EMPI_GLOBAL_PAPI_nhwpc_2,EMPI_GLOBAL_monitoring_data.nhwpc_2,len1)!=0){
                (*new_PAPI_policy)=1;
            }
            else{
                (*new_PAPI_policy)=0;
            }
                        
                        
            // Rank0 adquires the PAPI event names
            memcpy(monitor.nhwpc_1,EMPI_GLOBAL_PAPI_nhwpc_1,(size_t)EMPI_Monitor_string_size);
            memcpy(monitor.nhwpc_2,EMPI_GLOBAL_PAPI_nhwpc_2,(size_t)EMPI_Monitor_string_size);
            
            // Rank 0 adquires the PAPI the core binding status
            monitor.corebinding=EMPI_GLOBAL_corebinding;
            
            // Rank 0 adquires the termination condition
            monitor.termination=EMPI_GLOBAL_monitoring_data.termination;
            
            // Rank 0 adquires the termination condition
            monitor.lbalance=EMPI_GLOBAL_monitoring_data.lbalance;
            
        pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
    }
    
    
    //monitoring data
    PMPI_Allgather (&monitor, 1, EMPI_Monitor_Datatype, smonitor, 1, EMPI_Monitor_Datatype, EMPI_COMM_WORLD);

    
    // Check for termination condition (case 5 in command_listener)
    if(smonitor[0].termination==1){
        fprintf(stderr, "\n Terminating abruptly \n\n");
        sleep(60);
        MPI_Finalize();
        exit(0);
    }

}

/****************************************************************************************************************************************
*
*   'EMPI_fill_smonitor_lb'
*
****************************************************************************************************************************************/
void EMPI_fill_smonitor_lb  (int count, int size, EMPI_Monitor_type **smonitor_orig) {
        
    EMPI_Monitor_type monitor;
    EMPI_Monitor_type *smonitor = NULL;
    
    //memory allocation
    (*smonitor_orig) = (EMPI_Monitor_type*) malloc (size * sizeof(EMPI_Monitor_type));
    assert ((*smonitor_orig));
    smonitor = (*smonitor_orig);

    //compute the slice per process depending on data storage type
    monitor.count = EMPI_get_monitor_count (count);

    //set monitor parameters
    monitor.flops = EMPI_GLOBAL_PAPI_flops_lb;
    monitor.rtime = EMPI_GLOBAL_PAPI_rtime_lb;
    monitor.ptime = EMPI_GLOBAL_PAPI_ptime_lb;
    monitor.ctime = EMPI_GLOBAL_tcomm_interval_lb;
    monitor.hostid = EMPI_GLOBAL_hostid;

    //monitoring data
    PMPI_Allgather (&monitor, 1, EMPI_Monitor_Datatype, smonitor, 1, EMPI_Monitor_Datatype, EMPI_COMM_WORLD);
    
}

/****************************************************************************************************************************************
*
*   'EMPI_Check_to_Reinit_PAPI'
*
****************************************************************************************************************************************/
void EMPI_Check_to_Reinit_PAPI (int new_PAPI_policy, EMPI_Monitor_type *smonitor) {
    
    long long values[3] = {0, 0, 0};
    int eventcode_hwpc_1,eventcode_hwpc_2;
    size_t len0,len1;

    len0=strlen(smonitor[0].nhwpc_1)+1;
    len1=strlen(smonitor[0].nhwpc_2)+1;

    // If there are new PAPI events it is necessary to reinitialize PAPI
    if (new_PAPI_policy==1 ||strncmp(EMPI_GLOBAL_PAPI_nhwpc_1,smonitor[0].nhwpc_1,len0)!=0 || strncmp(EMPI_GLOBAL_PAPI_nhwpc_2,smonitor[0].nhwpc_2,len1)!=0) {


        #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
             fprintf (stderr, "\n*** DEBUG_MSG:: Restarting PAPI for using new event values <%s> ***\n", __FILE__);
             fflush(NULL);
         #endif
         
         EMPI_GLOBAL_PAPI_init = EMPI_FALSE;  // Forces PAPI restart
         
         //PAPI stop
         PAPI_stop (EMPI_GLOBAL_PAPI_eventSet, values);

         //PAPI remove event
         PAPI_remove_event (EMPI_GLOBAL_PAPI_eventSet, PAPI_FP_OPS);

         //PAPI remove event
         PAPI_event_name_to_code(EMPI_GLOBAL_PAPI_nhwpc_2, &eventcode_hwpc_1 );
         PAPI_remove_event (EMPI_GLOBAL_PAPI_eventSet, eventcode_hwpc_1);

         //PAPI remove event
         PAPI_event_name_to_code(EMPI_GLOBAL_PAPI_nhwpc_2, &eventcode_hwpc_2 );
         PAPI_remove_event (EMPI_GLOBAL_PAPI_eventSet, eventcode_hwpc_2);
         
         // Updates the new monitoring metrics using the root values
         memcpy(EMPI_GLOBAL_PAPI_nhwpc_1,smonitor [0].nhwpc_1,(size_t)EMPI_Monitor_string_size);
         memcpy(EMPI_GLOBAL_PAPI_nhwpc_2,smonitor[0].nhwpc_2,(size_t)EMPI_Monitor_string_size);
     }
}

/****************************************************************************************************************************************
*
*   'EMPI_core_binding'
*
****************************************************************************************************************************************/
void EMPI_core_binding (int rank, EMPI_Monitor_type *smonitor) {
    
    if(smonitor[0].corebinding==1){
        cpu_set_t mask;
        char path[1000];
        char shell_cmd[100];
        FILE *fp;
        char processor_name[MPI_MAX_PROCESSOR_NAME];
        int namelen;
        int bsize;

        // Unserts the variable
        pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
        EMPI_GLOBAL_corebinding=0;
         pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
        
        MPI_Get_processor_name(processor_name, &namelen);
        sprintf(shell_cmd, "cat /proc/%d/stat | awk '{print $39}'", getpid());
        fp = popen(shell_cmd, "r");
        while (fgets(path, 1000, fp) != NULL) {
            fprintf(stderr, "%s[%d]: before running on CPU %s", processor_name, rank, path);
        }
        pclose(fp);
        
        bsize=EMPI_max_process*32; // 32 is the maximum number of cores, that is, the lenght of the binding-list per process
        
        MPI_Bcast(EMPI_GLOBAL_corebindlist,bsize, MPI_INT, EMPI_root, MPI_COMM_WORLD);
          
        // Core binding
        if (EMPI_GLOBAL_corebindlist[rank][0]>0){
            CPU_ZERO(&mask);
            for(int n=0;n<EMPI_GLOBAL_corebindlist[rank][0];n++){
                CPU_SET(EMPI_GLOBAL_corebindlist[rank][n+1], &mask);
            }
            sched_setaffinity(0, sizeof(cpu_set_t), &mask);
        }
        
        sprintf(shell_cmd, "cat /proc/%d/stat | awk '{print $39}'", getpid());
        fp = popen(shell_cmd, "r");
        while (fgets(path, 1000, fp) != NULL) {
            fprintf(stderr, "%s[%d]: after running on CPU %s", processor_name, rank, path);
        }
        pclose(fp);
        
    }
}


/****************************************************************************************************************************************
*
*   'EMPI_store_smonitor_global'
*
****************************************************************************************************************************************/
void EMPI_store_smonitor_global (int rank, int size, EMPI_Monitor_type *smonitor) {

    double sctime = 0.0, smflops = 0.0;
    long long tflops = 0, srtime = 0;
    int local_termination;

     // ToDo: check if it has to be only executed by rank 0 process
     // Copies to the global variable the monitoring information. This global variable is used by the server to send the data via sockets
     pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
     
     local_termination=EMPI_GLOBAL_monitoring_data.termination;
     EMPI_GLOBAL_monitoring_data = smonitor[0]; // All the threads use the monitor variable of rank0. Including the termination condition
     // Note: 14 lines ahead the perf. counter values are replaced by the process spawn/removal overheads
     if(rank == EMPI_root) EMPI_GLOBAL_monitoring_data.termination=local_termination;
     
     // Aggregates the data of all the processes
     for(int n=1;n<size;n++){
       EMPI_GLOBAL_monitoring_data.flops +=smonitor[n].flops;
       //EMPI_GLOBAL_monitoring_data.rtime +=smonitor[n].rtime;
       //EMPI_GLOBAL_monitoring_data.ptime +=smonitor[n].ptime;
       //EMPI_GLOBAL_monitoring_data.ctime +=smonitor[n].ctime;
       EMPI_GLOBAL_monitoring_data.flops_iteration +=smonitor[n].flops_iteration;
       EMPI_GLOBAL_monitoring_data.hwpc_1 +=smonitor[n].hwpc_1;
       EMPI_GLOBAL_monitoring_data.hwpc_2 +=smonitor[n].hwpc_2;
     }
     
     // Overwrites the monitoring metrics with process spawn/removal overheads
     /*
     EMPI_GLOBAL_monitoring_data.hwpc_1=(long long int)ceil(EMPI_GLOBAL_lastoverhead_processes*1000); // In ms
     EMPI_GLOBAL_monitoring_data.hwpc_2=(long long int)ceil(EMPI_GLOBAL_lastoverhead_rdata*1000);      // In ms
     strcpy(EMPI_GLOBAL_monitoring_data.nhwpc_1,"ProcessOverhead");
     strcpy(EMPI_GLOBAL_monitoring_data.nhwpc_2,"DataRedistOverhead");
     */
     EMPI_GLOBAL_monitoring_data.lbalance = 0; // Resets the counter
                 
     pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);

     int sflops;

     for (int n = 0, sflops = 0; n < size; n ++) {
         //fprintf(stderr, "2.1. %lld, %lld \n",smonitor[n].flops, smonitor[n].rtime );
         sflops += (int)(smonitor[n].flops/smonitor[n].rtime);

         //fprintf(stderr, "3. Sflops is %d\n", sflops);
     }


     //set performance
     if (rank == EMPI_root) {

         //set host performance
         EMPI_Set_host_perf (smonitor, size);

         for (int n = 0; n < size; n ++) {

             tflops += smonitor[n].flops;

             srtime += smonitor[n].rtime;

             smflops += (double)((double)smonitor[n].flops/(double)smonitor[n].rtime);

             sctime += smonitor[n].ctime;
         }

         //set flops
         EMPI_GLOBAL_track_flops[0] = EMPI_GLOBAL_track_flops[1];

         EMPI_GLOBAL_track_flops[1] = tflops;

         //set rtime
         EMPI_GLOBAL_track_rtime[0] = EMPI_GLOBAL_track_rtime[1];

         EMPI_GLOBAL_track_rtime[1] = srtime;
     }

}

/****************************************************************************************************************************************
*
*   'EMPI_Exec_Malleability'
*
****************************************************************************************************************************************/
void EMPI_Exec_Malleability  (int *rank, int *size, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin, int newsize, int mflops, int **hostid_orig, int **rremvs_orig, int doLBalance) {
        
    int *hostid = (*hostid_orig);
    int *rremvs = (*rremvs_orig);
    
    // CHANGE: START_NEWVER
    // if EMPI_MALLEABILITY_TRIG -> get reconfiguration data
    if  (EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY_TRIG) {
        EMPI_broadcast_reconfigure_data(&EMPI_GLOBAL_reconfig_data);
    }
    // CHANGE: END_NEWVER

    fprintf (stderr, "EMPI_Exec_Malleability(%d): BEGIN, size/newsize = %d/%d\n", getpid(), *size, newsize);

    //reconfiguring the number of processes
    if (newsize > *size) {
        
        fprintf (stderr, "EMPI_Exec_Malleability(%d): SPAWN, size/newsize = %d/%d\n", getpid(), *size, newsize);

        //spawn processes
        EMPI_Monitor_spawn (rank, size, (newsize-*size), count, disp, argv, bin, smonitor, hostid, fc);

        // CHANGE: begin
        // erase hints in globals reconfig data
        EMPI_GLOBAL_reconfig_data.procs_hint = 0;
        EMPI_GLOBAL_reconfig_data.excl_nodes_hint = 0;
        fprintf (stderr, "EMPI_Exec_Malleability(%d): SPAWN, reseting procs_hint=%d and excl_nodes_hint=%d\n", getpid(), EMPI_GLOBAL_reconfig_data.procs_hint, EMPI_GLOBAL_reconfig_data.excl_nodes_hint);
        // CHANGE: end

    } else if ((newsize < *size)&&(newsize > 0)) {
        
        fprintf (stderr, "EMPI_Exec_Malleability(%d): REMOVE, size/newsize = %d/%d\n", getpid(), *size, newsize);

        if (*rank > EMPI_root) {
            rremvs = (int*) malloc ((*size-newsize) * sizeof(int));
            assert (rremvs);
        }
        
        //Bcast rremvs
        PMPI_Bcast (rremvs, (*size-newsize), MPI_INT, EMPI_root, EMPI_COMM_WORLD);
        
        if ((*size-newsize) >= 2) {
            fprintf (stderr, "EMPI_Exec_Malleability(%d): REMOVE, rremvs[0]/rremvs[1] = %d/%d\n", getpid(), rremvs[0], rremvs[1]);
        }
        
        /* CHANGE: begin */
        fprintf (stderr, "\n*** DEBUG_MSG::PMPI_Barrier(begin)::EMPI_Exec_Malleability in <%s> ***\n", __FILE__);
        PMPI_Barrier (EMPI_COMM_WORLD);
        fprintf (stderr, "\n*** DEBUG_MSG::PMPI_Barrier(end)::EMPI_Exec_Malleability in <%s> ***\n", __FILE__);
        /* CHANGE: end */

        //remove processes
        EMPI_Monitor_remove (rank, size, (*size-newsize), count, disp, smonitor, rremvs, fc);
        fprintf (stderr, "EMPI_Exec_Malleability(%d): EMPI_Monitor_remove\n", getpid());
        
        /* CHANGE: begin */
        if (*rank == EMPI_root) {
            fprintf(stderr, "EMPI_Exec_Malleability(%d): sleep and call command_rpc_release_nodes \n", getpid());
            //for (int i=0;i<20;i++) {
            //    sleep(1);
            //    fprintf (stderr, "release_nodes: %d sec\n",i+1);
            //}
            // CHANGE TEMP
            signal_thread_rpc_release_nodes();
            // END CHANGE TEMP

        }

        // erase hints in globals reconfig data
        EMPI_GLOBAL_reconfig_data.procs_hint = 0;
        EMPI_GLOBAL_reconfig_data.excl_nodes_hint = 0;
        fprintf (stderr, "EMPI_Exec_Malleability(%d): REMOVE, reseting procs_hint=%d and excl_nodes_hint=%d\n", getpid(), EMPI_GLOBAL_reconfig_data.procs_hint, EMPI_GLOBAL_reconfig_data.excl_nodes_hint);


        /* CHANGE: end */
    } else {
        fprintf (stderr, "EMPI_Exec_Malleability(%d): NOTHING, size/newsize = %d/%d\n", getpid(), *size, newsize);

        if (doLBalance == 1) {
            int lbalance = EMPI_NULL;
            lbalance = EMPI_LBalance (rank, size, count, disp, fc, smonitor);
        }
    }
    
    if ((*hostid_orig) != NULL) {
        free ((*hostid_orig));
        (*hostid_orig)=NULL;
    }
    if ((*rremvs_orig) != NULL) {
        free ((*rremvs_orig));
        (*rremvs_orig)=NULL;
    }
}

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_end'
*
****************************************************************************************************************************************/
void EMPI_Monitor_end (int *rank, int *size, int iter, int maxiter, int *count, int *disp, int **vcount, int **vdisp, int *fc, char *argv[], char *bin) {


    //TODO: Schedule load balance accoridng to flag
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_NoMonitor_end in <%s> ***\n", __FILE__);
        fflush(NULL);
    #endif

    int n = 0,new_PAPI_policy=0;
    
    MPI_Status status;

    EMPI_Class_type *class = NULL;

    EMPI_Monitor_type monitor, *smonitor = NULL;

    /* CHANGE: BEGIN */
    if (*rank == EMPI_root) {
        fprintf(stderr, "EMPI_Monitor_end(%d): ITER %d BEGIN signal_thread_rpc_malleability_leave_region\n",getpid(), iter);
        signal_thread_rpc_malleability_leave_region();
        fprintf(stderr, "EMPI_Monitor_end(%d): ITER %d END signal_thread_rpc_malleability_leave_region\n",getpid(), iter);
    }
    /* CHANGE: END */

    // init cost structure the first time and do the monitoring
    // increments per iteration
    EMPI_InitAndInc_NoMonitor_end (*rank, iter, maxiter);
       
    if (EMPI_GLOBAL_mpolicy == EMPI_MONITORDBG) {
        
        if ((iter > 0)&&((iter % EMPI_GLOBAL_niter) == 0)) {

            // Increment the values of the cost struct
            EMPI_Inc_Cost_struct (*rank, MPI_Wtime(),0);

            fprintf(stderr, "[%i-%i] FLOP %lld INS %lld CYC %lld Rtime %lld Ptime %lld ctime %lf FLOP/S %lf\n", *rank, iter, EMPI_GLOBAL_PAPI_flops, EMPI_GLOBAL_PAPI_hwpc_1, EMPI_GLOBAL_PAPI_hwpc_2, EMPI_GLOBAL_PAPI_rtime, EMPI_GLOBAL_PAPI_ptime, EMPI_GLOBAL_tcomm_interval, (double)(EMPI_GLOBAL_PAPI_flops/EMPI_GLOBAL_PAPI_ptime));

            //reset variables
            EMPI_GLOBAL_PAPI_rtime = EMPI_GLOBAL_PAPI_ptime = EMPI_GLOBAL_PAPI_flops = EMPI_GLOBAL_PAPI_hwpc_1 = EMPI_GLOBAL_PAPI_hwpc_2 = 0;

            EMPI_GLOBAL_tcomm_interval = EMPI_GLOBAL_tio_interval= 0;
        }

    } else if (EMPI_GLOBAL_mpolicy != EMPI_NULL) {
        
       fprintf(stderr, "EMPI_Monitor_end(%d): ANTES ITERACION %d EMPI_GLOBAL_niter=%d\n",getpid(), iter, EMPI_GLOBAL_niter);

       if ((iter > 0)&&((iter % EMPI_GLOBAL_niter) == 0)) {

           fprintf(stderr, "EMPI_Monitor_end(%d): ITERACION %d\n",getpid(), iter);

           // Increment the values of the cost struct
           EMPI_Inc_Cost_struct (*rank, MPI_Wtime(),0);
           
           // fill smonitor
           EMPI_fill_smonitor ((*rank), (*count), (*size), &smonitor,  &new_PAPI_policy);

           // check if you need to reinitializate PAPI
           EMPI_Check_to_Reinit_PAPI (new_PAPI_policy, smonitor);

           // Updates the allocation policy
           EMPI_array_alloc =  smonitor[0].EMPI_array_alloc;
            
           // Detects whether a global load balance operation has to be performed in the next callo
           if(smonitor[0].lbalance == 1){
               EMPI_GLOBAL_perform_load_balance=1;
           }
            
           // CHANGE: THIS SHOULD NOT WORK IN MORE THAN ONE NODE
           // Performs the core binding
           // EMPI_core_binding ((*rank), smonitor);
           // END CHANGE

           // store the smonitor data on the global variables
           EMPI_store_smonitor_global ((*rank), (*size), smonitor);

            //evaluate policy (efficiency, cost, lbalance)
            switch (EMPI_GLOBAL_mpolicy) {
                /*//ALBERTO 1306: keep just trig
                 case EMPI_EFFICIENCY:

                    //enhance efficiency with performance constraint
                     if(*rank == EMPI_root) fprintf(stderr, "EMPI_Monitor_end(%d): Policy is %s\n", getpid(), "EMPI_Monitor_efficiency");
                    EMPI_Monitor_efficiency (rank, size, iter, maxiter, *count, *disp, smonitor, fc, argv, bin);

                    break;

                case EMPI_EFFICIENCY_IRR:

                    //enhance efficiency with performance constraint
                     if(*rank == EMPI_root) fprintf(stderr, "EMPI_Monitor_end(%d): Policy is %s\n", getpid(), "EMPI_Monitor_efficiency_irregular");
                    EMPI_Monitor_efficiency_irregular (rank, size, iter, maxiter, *count, *disp, smonitor, fc, argv, bin);

                    break;

                case EMPI_COST:

                    //minimize cost with performance constraint
                     if(*rank == EMPI_root) fprintf(stderr, "EMPI_Monitor_end(%d): Policy is %s\n", getpid(), "EMPI_Monitor_cost");
                    EMPI_Monitor_cost (rank, size, iter, maxiter, *count, *disp, smonitor, fc, argv, bin);

                    break;

                case EMPI_COST_IRR:

                    //minimize cost with performance constraint
                     if(*rank == EMPI_root) fprintf(stderr, "EMPI_Monitor_end(%d): Policy is %s\n", getpid(), "EMPI_Monitor_cost_irregular");
                    EMPI_Monitor_cost_irregular (rank, size, iter, maxiter, *count, *disp, smonitor, fc, argv, bin);

                    break;

 
                case EMPI_MALLEABILITY:

                    //malleability
                     if(*rank == EMPI_root) fprintf(stderr, "EMPI_Monitor_end(%d): Policy is %s \n", getpid(), "EMPI_Monitor_malleability");
                    EMPI_Monitor_malleability (rank, size, iter, maxiter, *count, *disp, smonitor, fc, argv, bin);

                    break;

                case EMPI_MALLEABILITY_COND:

                    //conditional malleability
                     if(*rank == EMPI_root) fprintf(stderr, "EMPI_Monitor_end(%d): Policy is %s\n", getpid(),  "EMPI_Monitor_malleability_conditional");
                    EMPI_Monitor_malleability_conditional (rank, size, iter, maxiter, *count, *disp, smonitor, fc, argv, bin);

                    break;
                    
                */case EMPI_MALLEABILITY_TRIG:

                    //conditional malleability
                    //if(*rank == EMPI_root) printf("Policy is %s\n", "EMPI_Monitor_malleability_triggered");
                    EMPI_Monitor_malleability_triggered (rank, size, iter, maxiter, *count, *disp, smonitor, fc, argv, bin);
                    break;


                /*//ALBERTO 1306: case EMPI_LBALANCE:

                    if (EMPI_GLOBAL_lbalance == EMPI_TRUE) {

                        //load balancing
                        EMPI_LBalance (rank, size, *count, *disp, fc, smonitor);
                    }

                    break;
                */
            }

            fprintf(stderr, "[DEBUG] End of policy selection.\n");
            // David
            if (*rank == EMPI_root) {
                EMPI_Monitor_type aux_monitor;
                for (int i=0;i<(*size);i++) {
                    aux_monitor.flops = smonitor[i].flops;
                    aux_monitor.rtime = smonitor[i].rtime;
                    aux_monitor.ptime = smonitor[i].ptime;
                    aux_monitor.ctime = smonitor[i].ctime;
                    aux_monitor.iotime = smonitor[i].iotime;
                }
                aux_monitor.flops = aux_monitor.flops/((long long)(*size));
                aux_monitor.rtime = aux_monitor.rtime/((long long)(*size));
                aux_monitor.ptime = aux_monitor.ptime/((long long)(*size));
                aux_monitor.ctime = aux_monitor.ctime/((double)(*size));
                aux_monitor.iotime = aux_monitor.iotime/((double)(*size));

                char * ip = get_addr_ic_str();
                char * clid = get_client_id();
                assert(ip);
                fprintf (stderr, "EMPI_Monitor_end(%d): Iter: %d \t FLOPs: %lld \t MFLOPS:: %lf \t RTIME:: %lf \t PTIME:: %lf \t CTIME:: %lf \t IOTime:: %lf \t Size: %i\n", getpid(), iter, aux_monitor.flops, (double)((double)aux_monitor.flops)/((double)aux_monitor.rtime), ((double)aux_monitor.rtime)/1000000,((double)aux_monitor.ptime)/1000000, aux_monitor.ctime, aux_monitor.iotime, *size);

                redisContext *context = redisConnect(ip, 6379); //localhost for debug
                if (context == NULL || context->err) {
                  if (context) {
                        fprintf(stderr, "Redis connection error: %s\n", context->errstr);
                        redisFree(context);
                    } else {
                        fprintf(stderr, "Redis connection error: can't allocate redis context\n");
                    }
                } else {
                    char val_str[128];
                    sprintf(val_str, "%lld %lf %lf %lf %lf %lf %i", aux_monitor.flops, (double)((double)aux_monitor.flops)/((double)aux_monitor.rtime),
                            ((double)aux_monitor.rtime)/1000000,((double)aux_monitor.ptime)/1000000, aux_monitor.ctime, aux_monitor.iotime, *size);
                    redisReply *reply = (redisReply *)redisCommand(context, "SET monitorFlexMPI:%s:%s:%d %s", clid, ip, iter, val_str);
                    if (reply == NULL) fprintf(stderr, "Failed to execute Redis command\n");
                    fprintf(stderr, "[DEBUG] SET: %s\n", reply->str);
                    freeReplyObject(reply);
                    
                    reply = (redisReply *)redisCommand(context, "SET monitorFlexMPI:%s:%s:current %s", clid, ip, val_str);
                    if (reply == NULL) fprintf(stderr, "Failed to execute Redis command\n");
                    fprintf(stderr, "[DEBUG] SET: %s\n", reply->str);
                    freeReplyObject(reply);

                    redisFree(context);
                }

            }
            fflush(stderr);
            
            //reset variables
            EMPI_GLOBAL_PAPI_rtime = EMPI_GLOBAL_PAPI_ptime = EMPI_GLOBAL_PAPI_flops = EMPI_GLOBAL_PAPI_hwpc_1 = EMPI_GLOBAL_PAPI_hwpc_2 = 0;
            
            EMPI_GLOBAL_tcomm_interval = EMPI_GLOBAL_tio_interval = 0;
            
            //if(*rank == EMPI_root)  printf("END EMPI_GLOBAL_PAPI_flops is %lld, EMPI_GLOBAL_PAPI_flops_iteration is %lld\n", EMPI_GLOBAL_PAPI_flops, EMPI_GLOBAL_PAPI_flops_iteration);

           free (smonitor);

        //} else if ((iter > 0)&&((iter % EMPI_GLOBAL_niter_lb) == 0)&&((EMPI_GLOBAL_mpolicy == EMPI_EFFICIENCY_IRR)||(EMPI_GLOBAL_mpolicy == EMPI_COST_IRR))) {
        }
        else if (EMPI_GLOBAL_perform_load_balance == 1 ||  ((iter > 0) && (iter % EMPI_GLOBAL_niter_lb) == 0)) {
                       
            EMPI_GLOBAL_perform_load_balance=0;

            // fill smonitor
            EMPI_fill_smonitor_lb  ((*count), (*size), &smonitor);
            
            //Load balancing
            if (*rank == EMPI_root){
                fprintf(stderr, "EMPI_Monitor_end(%d): FLEXMPI->Monitor:: Performing load balance %d \n", getpid(), (*size));
            }
            
            EMPI_LBalance (rank, size, *count, *disp, fc, smonitor);
            
            //reset variables
            EMPI_GLOBAL_PAPI_rtime_lb = EMPI_GLOBAL_PAPI_ptime_lb = EMPI_GLOBAL_PAPI_flops_lb = EMPI_GLOBAL_PAPI_hwpc_1_lb = EMPI_GLOBAL_PAPI_hwpc_2_lb = 0;

            EMPI_GLOBAL_tcomm_interval_lb = 0;

            free (smonitor);
        }
    }

    //disabling capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_GLOBAL_capture_comms = 0;

    //reset EMPI_GLOBAL_capture_comms
    if (((iter+1) % EMPI_GLOBAL_niter) == 0) {

        if (*rank == EMPI_root) EMPI_Destroy_comms ();

        EMPI_GLOBAL_capture_comms = 1;
    }

    fprintf(stderr, "[DEBUG] Getting worksize\n");
    //Get worksize
    if ((count != NULL) && (disp != NULL) && (vcount != NULL) && (vdisp != NULL)) {

        //free mem
        free (*vdisp);
        free (*vcount);

        //malloc vcount and vdisp arrays
        assert ((*vdisp = (int*) malloc (*size * sizeof(int))) != NULL);
        assert ((*vcount = (int*) malloc (*size * sizeof(int))) != NULL);

        //get new worksize
        EMPI_Get_wsize (*rank, *size, EMPI_GLOBAL_Data->size, disp, count, *vcount, *vdisp);
        
    } else if ((count != NULL) && (disp != NULL) && (vcount == NULL) && (vdisp == NULL)) {

        //get new worksize
        EMPI_Get_wsize (*rank, *size, EMPI_GLOBAL_Data->size, disp, count, NULL, NULL);

    }

    fprintf(stderr, "[DEBUG] last iteration? \n");
    //Last iteration
    if (iter == (maxiter-1)) {
        // Increment and get the cumulative values of the cost struct
        EMPI_GLOBAL_iterative_end = MPI_Wtime();
        EMPI_Inc_Cost_struct (*rank,  EMPI_GLOBAL_iterative_end, 1);
    }

    EMPI_GLOBAL_tover += ((PAPI_get_real_usec() - EMPI_GLOBAL_tover_ini) * 1.0E-6);

    fprintf(stderr, "[DEBUG] END of EMPI_Monitor_end - rank %d\n", *rank);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_NoMonitor_end in <%s> ***\n", __FILE__);
    #endif
}
/*
 *  END CHANGE JAVIER
 */



/****************************************************************************************************************************************
*
*   'EMPI_Get_aggregated_tcomp'
*
****************************************************************************************************************************************/
void EMPI_Get_aggregated_tcomp (double *tcomp) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Get_aggregated_tcomp in <%s> ***\n", __FILE__);
    #endif

    *tcomp = EMPI_GLOBAL_tcomp;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Get_aggregated_tcomp in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Get_aggregated_tcomm'
*
****************************************************************************************************************************************/
void EMPI_Get_aggregated_tcomm (double *tcomm) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Get_aggregated_tcomm in <%s> ***\n", __FILE__);
    #endif

    *tcomm = EMPI_GLOBAL_tcomm;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Get_aggregated_tcomm in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Get_exec_time'
*
****************************************************************************************************************************************/
void EMPI_Get_exec_time (double *extime) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Get_exec_time in <%s> ***\n", __FILE__);
    #endif

    *extime = MPI_Wtime() - EMPI_GLOBAL_tcomp_ini;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Get_exec_time in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Iteration_init'
*
****************************************************************************************************************************************/
static void EMPI_Iteration_init (void) {
    //printf("[DEBUG] in iteration init\n");
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Iteration_init in <%s> ***\n", __FILE__);
    #endif
    int i;

    long long values[3] = {0, 0, 0};

    //PAPI reset FLOPS
    PAPI_reset (EMPI_GLOBAL_PAPI_eventSet);
    //PAPI read FLOPS
    PAPI_read (EMPI_GLOBAL_PAPI_eventSet, values);


    //real and processor time
    EMPI_GLOBAL_PAPI_rtime_init = PAPI_get_real_usec();
    EMPI_GLOBAL_PAPI_ptime_init = PAPI_get_virt_usec();


    //tcomm
    EMPI_GLOBAL_tcomm_itinit = EMPI_GLOBAL_tcomm;

    // I/O
    EMPI_GLOBAL_tio_itinit = EMPI_GLOBAL_tio;
    
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Iteration_init in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Iteration_end'
*
****************************************************************************************************************************************/
static void EMPI_Iteration_end (long long *rtime, long long *ptime, long long *flops, long long *hwpc_1, long long *hwpc_2, double *ctime, double *iotime) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Iteration_fin in <%s> ***\n", __FILE__);
    #endif
    double elapsed_time = 0;
    double total_elapsed_time = 0;
    long long values[3] = {0, 0, 0};
    long long aux_time;
    int i;

    //iteration computation time: real time and cpu time
    aux_time =  (long long)PAPI_get_real_usec() - (long long)EMPI_GLOBAL_PAPI_rtime_init;

    EMPI_GLOBAL_PAPI_it_time = aux_time;

    //*rtime = (PAPI_get_real_usec() - EMPI_GLOBAL_PAPI_rtime_init) - ((EMPI_GLOBAL_tcomm - EMPI_GLOBAL_tcomm_itinit) * 1.0E6);
    *rtime = aux_time - (long long)((EMPI_GLOBAL_tcomm - EMPI_GLOBAL_tcomm_itinit) * 1.0E6);
    
    // *ptime = (PAPI_get_virt_usec() - EMPI_GLOBAL_PAPI_ptime_init) - ((EMPI_GLOBAL_tcomm - EMPI_GLOBAL_tcomm_itinit) * 1.0E6); // discounts the communication time
    *ptime = ((long long)PAPI_get_virt_usec() - EMPI_GLOBAL_PAPI_ptime_init); // We also include the communication time
    
    //computation time
    EMPI_GLOBAL_tcomp += (double)(*rtime * 1.0E-6);

    //communication time
    *ctime = (EMPI_GLOBAL_tcomm - EMPI_GLOBAL_tcomm_itinit);
    // IO time
    *iotime=(EMPI_GLOBAL_tio- EMPI_GLOBAL_tio_itinit);
    //PAPI read FLOPS and counters
    PAPI_read (EMPI_GLOBAL_PAPI_eventSet, values);
    
    // Reads HW counter values
    *flops  = values[0];
    *hwpc_1 = values[1];
    *hwpc_2 = values[2];

    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Iteration_fin in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Get_minprocs'
*
****************************************************************************************************************************************/
void EMPI_Get_minprocs (int *nprocs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Get_minprocs in <%s> ***\n", __FILE__);
    #endif

    *nprocs = EMPI_GLOBAL_minprocs;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Get_minprocs in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Set_maxprocs'
*
****************************************************************************************************************************************/
void EMPI_Set_maxprocs (int nprocs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Set_maxprocs in <%s> ***\n", __FILE__);
    #endif

    int size;

    MPI_Comm_size (EMPI_COMM_WORLD, &size);

    //set max number of processes
    if (nprocs < size) {

        EMPI_GLOBAL_maxprocs = size;

        fprintf (stderr, "Number of processes must be greater than the number of running processes. Maxprocs did not set up\n");

    } else

        EMPI_GLOBAL_maxprocs = nprocs;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Set_maxprocs in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Set_policy'
*
****************************************************************************************************************************************/
void EMPI_Set_policy (int policy) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Set_policy in <%s> ***\n", __FILE__);
    #endif

    if ((policy != EMPI_EFFICIENCY)&&(policy != EMPI_EFFICIENCY_IRR)&&(policy != EMPI_COST)&&(policy != EMPI_COST_IRR)&&(policy != EMPI_LBALANCE)&&(policy != EMPI_MALLEABILITY)&&(policy != EMPI_MALLEABILITY_COND)&&(policy != EMPI_MONITORDBG)&&(policy != EMPI_MALLEABILITY_TRIG)) {

        fprintf (stderr, "Invalid malleability policy (set to lbalance, evolving efficiency, evolving cost, malleability, conditional malleability, monitordbg or malleability triggered)\n");

        MPI_Abort (EMPI_COMM_WORLD, -1);
    }

    pthread_mutex_lock(&EMPI_GLOBAL_server_lock);

    //set policy (one of the valid ones)
    EMPI_GLOBAL_mpolicy = policy;

    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);


    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Set_policy in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Set_niter'
*
****************************************************************************************************************************************/
void EMPI_Set_niter (int niter) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Set_niter in <%s> ***\n", __FILE__);
    #endif

    fprintf (stderr, "EMPI_Set_niter(%d): begin\n", getpid());

    EMPI_GLOBAL_niter = niter;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Set_niter in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_init'
*
****************************************************************************************************************************************/
/* CHANGE: begin */
//void EMPI_Monitor_init (void) {
void EMPI_Monitor_init (int *rank, int procs_hint, int excl_nodes_hint) {
/* CHANGE: end */

    int retval;
    int cid,rapl_cid=-1,numcmp;
    int code,eventcode_hwpc_1,eventcode_hwpc_2;
    int num_events = 0;

    //int data_type[MAX_RAPL_EVENTS];
    int r;
    //int r,i;
    //int do_wrap = 0;
    const PAPI_component_info_t *cmpinfo = NULL;
    PAPI_event_info_t evinfo;
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Monitor_init in <%s> ***\n", __FILE__);
    #endif
    
    fprintf (stderr, "EMPI_Monitor_init(%d): Begin\n", getpid());

    //wait for all processes
    if (EMPI_GLOBAL_concurrency == EMPI_TRUE) {

        /* CHANGE: begin */
        PMPI_Barrier (EMPI_COMM_WORLD);
        /* CHANGE: end */

        EMPI_GLOBAL_concurrency = EMPI_FALSE;

        //capture communications
        EMPI_GLOBAL_capture_comms = 1;
    }

    //PAPI intialization
    if (EMPI_GLOBAL_PAPI_init == EMPI_FALSE) {

        //EMPI_GLOBAL_PAPI_eventSet = 0;
        PAPI_cleanup_eventset(EMPI_GLOBAL_PAPI_eventSet);
        EMPI_GLOBAL_PAPI_eventSet = PAPI_NULL;
        //retval = PAPI_create_eventset (&EMPI_GLOBAL_PAPI_eventSet);
        //printf("retval = %d, PAPI_OK = %d\n", retval, PAPI_OK);
        if((retval = PAPI_create_eventset (&EMPI_GLOBAL_PAPI_eventSet)) != PAPI_OK){
            fprintf(stderr, "Error creating eventset\n");
        }


        
        
        //add floating point operations
        //if((retval = PAPI_add_event (EMPI_GLOBAL_PAPI_eventSet, PAPI_FP_OPS)) != PAPI_OK ){
        if((retval = PAPI_add_event (EMPI_GLOBAL_PAPI_eventSet, PAPI_TOT_INS)) != PAPI_OK ){
//           printf("Monitoring: Error adding event 1\n");
//            switch(retval){
//                case PAPI_EINVAL:
//                    printf("Error is PAPI_EINVAL\n");
//                break;
//
//                case PAPI_ENOMEM:
//                    printf("Error is PAPI_ENOMEM\n");
//                break;
//
//                case PAPI_ENOEVST:
//                    printf("Error is PAPI_ENOEVST\n");
//                break;
//
//                case PAPI_EISRUN:
//                    printf("Error is PAPI_EISRUN\n");
//                break;
//
//                case PAPI_ECNFLCT:
//                    printf("Error is PAPI_ECNFLCT\n");
//                break;
//
//                case PAPI_ENOEVNT:
//                    printf("Error is PAPI_ENOEVNT\n");
//                break;
//
//                case PAPI_EBUG:
//                    printf("Error is PAPI_EBUG\n");
//                break;
//
//            }
        } else {
            num_events++;
        }

        //add event eventcode_hwpc_1
        if(strcmp(EMPI_GLOBAL_PAPI_nhwpc_1,"NULL")!=0){
            if((retval = PAPI_event_name_to_code(EMPI_GLOBAL_PAPI_nhwpc_1, &eventcode_hwpc_1 )) != PAPI_OK){
//                printf("Monitoring: Error adding event hwpc_1\n");
//                switch(retval){
//                    case PAPI_EINVAL:
//                        printf("Error is PAPI_EINVAL\n");
//                    break;
//
//                    case PAPI_ENOMEM:
//                        printf("Error is PAPI_ENOMEM\n");
//                    break;
//
//                    case PAPI_ENOEVST:
//                        printf("Error is PAPI_ENOEVST\n");
//                    break;
//
//                    case PAPI_EISRUN:
//                        printf("Error is PAPI_EISRUN\n");
//                    break;
//
//                    case PAPI_ECNFLCT:
//                        printf("Error is PAPI_ECNFLCT\n");
//                    break;
//
//                    case PAPI_ENOEVNT:
//                        printf("Error is PAPI_ENOEVNT\n");
//                    break;
//
//                    case PAPI_EBUG:
//                        printf("Error is PAPI_EBUG\n");
//                    break;
//                }
            }               
            else {        
                if((retval = PAPI_add_event (EMPI_GLOBAL_PAPI_eventSet, eventcode_hwpc_1)) != PAPI_OK){
//                    printf("Monitoring: Error adding hwpc_1\n");
//                    switch(retval){
//                        case PAPI_EINVAL:
//                            printf("Error is PAPI_EINVAL\n");
//                        break;
//
//                        case PAPI_ENOMEM:
//                            printf("Error is PAPI_ENOMEM\n");
//                        break;
//
//                        case PAPI_ENOEVST:
//                            printf("Error is PAPI_ENOEVST\n");
//                        break;
//
//                        case PAPI_EISRUN:
//                            printf("Error is PAPI_EISRUN\n");
//                        break;
//
//                        case PAPI_ECNFLCT:
//                            printf("Error is PAPI_ECNFLCT\n");
//                        break;
//
//                        case PAPI_ENOEVNT:
//                            printf("Error is PAPI_ENOEVNT\n");
//                        break;
//
//                        case PAPI_EBUG:
//                            printf("Error is PAPI_EBUG\n");
//                        break;
//
//                    }
                } else {
                    num_events++;
                }
            }
        }
        
        if(strcmp(EMPI_GLOBAL_PAPI_nhwpc_2,"NULL")!=0){
            //add event eventcode_hwpc_2
            if((retval = PAPI_event_name_to_code(EMPI_GLOBAL_PAPI_nhwpc_2, &eventcode_hwpc_2 )) != PAPI_OK){
//                printf("Monitoring: Error adding event hwpc_2\n");
//                switch(retval){
//                    case PAPI_EINVAL:
//                        printf("Error is PAPI_EINVAL\n");
//                    break;
//
//                    case PAPI_ENOMEM:
//                        printf("Error is PAPI_ENOMEM\n");
//                    break;
//
//                    case PAPI_ENOEVST:
//                        printf("Error is PAPI_ENOEVST\n");
//                    break;
//
//                    case PAPI_EISRUN:
//                        printf("Error is PAPI_EISRUN\n");
//                    break;
//
//                    case PAPI_ECNFLCT:
//                        printf("Error is PAPI_ECNFLCT\n");
//                    break;
//
//                    case PAPI_ENOEVNT:
//                        printf("Error is PAPI_ENOEVNT\n");
//                    break;
//
//                    case PAPI_EBUG:
//                        printf("Error is PAPI_EBUG\n");
//                    break;
//                }
            }               
            else{
                if((retval = PAPI_add_event (EMPI_GLOBAL_PAPI_eventSet, eventcode_hwpc_2)) != PAPI_OK){
//                    printf("Monitoring: Error adding hwpc_2\n");
//                    switch(retval){
//                        case PAPI_EINVAL:
//                            printf("Error is PAPI_EINVAL\n");
//                        break;
//
//                        case PAPI_ENOMEM:
//                            printf("Error is PAPI_ENOMEM\n");
//                        break;
//
//                        case PAPI_ENOEVST:
//                            printf("Error is PAPI_ENOEVST\n");
//                        break;
//
//                        case PAPI_EISRUN:
//                            printf("Error is PAPI_EISRUN\n");
//                        break;
//
//                        case PAPI_ECNFLCT:
//                            printf("Error is PAPI_ECNFLCT\n");
//                        break;
//
//                        case PAPI_ENOEVNT:
//                            printf("Error is PAPI_ENOEVNT\n");
//                        break;
//
//                        case PAPI_EBUG:
//                            printf("Error is PAPI_EBUG\n");
//                        break;
//
//                    }
                } else {
                    num_events++;
                }
            }
        }
        
        //start counting
        if((retval = PAPI_start (EMPI_GLOBAL_PAPI_eventSet)) != PAPI_OK){
//            printf("Monitoring: Error starting count\n");
//            switch(retval){
//                case PAPI_EINVAL:
//                    printf("Error is PAPI_EINVAL\n");
//                break;
//
//                case PAPI_ESYS:
//                    printf("Error is PAPI_ESYS\n");
//                break;
//
//                case PAPI_ENOEVST:
//                    printf("Error is PAPI_ENOEVST\n");
//                break;
//
//                case PAPI_EISRUN:
//                    printf("Error is PAPI_EISRUN\n");
//                break;
//
//                case PAPI_ENOTRUN:
//                    printf("Error is PAPI_ENOTRUN\n");
//                break;
//
//                case PAPI_ECNFLCT:
//                    printf("Error is PAPI_ECNFLCT\n");
//                break;
//
//                case PAPI_ENOEVNT:
//                    printf("Error is PAPI_ENOEVNT\n");
//                break;
//
//
//            }
        }

        //PAPI initialized
        EMPI_GLOBAL_PAPI_init      = EMPI_TRUE;
        EMPI_GLOBAL_PAPI_numevents = num_events;

        //initialize variables
        EMPI_GLOBAL_tcomp          = EMPI_GLOBAL_tcomm = 0;
        EMPI_GLOBAL_tio = 0;
        EMPI_GLOBAL_iterative_ini  = MPI_Wtime();

    }
    //Iteration init
    fprintf (stderr, "EMPI_Monitor_init(%d): start EMPI_Iteration_init\n", getpid());

    EMPI_Iteration_init ();
    
    /* CHANGE: BEGIN */
    if (*rank == EMPI_root) {
        fprintf (stderr, "EMPI_Monitor_init(%d): is EMPI_root\n", getpid());
        if (procs_hint != 0) {
            EMPI_GLOBAL_reconfig_data.procs_hint = procs_hint;
            EMPI_GLOBAL_reconfig_data.excl_nodes_hint = excl_nodes_hint;
            fprintf (stderr, "EMPI_Monitor_init(%d): setting procs_hint=%d and excl_nodes_hint=%d\n", getpid(), EMPI_GLOBAL_reconfig_data.procs_hint, EMPI_GLOBAL_reconfig_data.excl_nodes_hint);
        }
        signal_thread_rpc_malleability_enter_region(procs_hint, excl_nodes_hint);
    }
    /* CHANGE: END */

    fprintf (stderr, "EMPI_Monitor_init(%d): Begin\n", getpid());

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Monitor_init in <%s> ***\n", __FILE__);
    #endif
}

/* 
 * ALBERTO 12072023
 * Set reconfig from stop and restart. 
 */
void SetReconfigDataIterationInit(int procs_hint, int excl_nodes_hint){
    fprintf (stderr, "SetReconfigData Init\n");
    if (procs_hint != 0) {
            EMPI_GLOBAL_reconfig_data.procs_hint = procs_hint;
            EMPI_GLOBAL_reconfig_data.excl_nodes_hint = excl_nodes_hint;
            fprintf (stderr, "SetReconfigData (%d): setting procs_hint=%d and excl_nodes_hint=%d\n", getpid(), EMPI_GLOBAL_reconfig_data.procs_hint, EMPI_GLOBAL_reconfig_data.excl_nodes_hint);
        }
        signal_thread_rpc_malleability_enter_region(procs_hint, excl_nodes_hint);
}

void SetReconfigDataIterationEnd(){
    fprintf (stderr, "SetReconfigData End\n");
    signal_thread_rpc_malleability_leave_region();
}
/* end alberto */




/****************************************************************************************************************************************
*
*   'EMPI_Monitor_efficiency'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_efficiency (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Monitor_efficiency in <%s> ***\n", __FILE__);
    #endif

    double tini = 0;

    int lbalance = EMPI_NULL;

    if (EMPI_GLOBAL_lbalance == EMPI_TRUE) lbalance = EMPI_LBalance (rank, size, count, disp, fc, smonitor);

    if ((lbalance == EMPI_BALANCED)||(lbalance == EMPI_NULL)) {

        tini = MPI_Wtime();

        int sflops, n, m, i, idx, newsize = 0, mflops = 0, reconfig[2] = {0, 0}, *hostid = NULL, *rremvs = NULL;

        double comm_mean = 0.0, sum_ctime = 0, sum_rtime = 0;

        for (n = 0, sflops = 0; n < *size; n ++) {

            sflops += (int)(smonitor[n].flops/smonitor[n].rtime);

            sum_rtime += (smonitor[EMPI_root].rtime*1.0E-6);

            sum_ctime += smonitor[n].ctime;

            comm_mean += smonitor[n].ctime;
        }

        if ((*rank == EMPI_root)&&(sflops > 0)) {

            EMPI_Class_type *class = NULL;

            EMPI_host_type *hostlist = NULL;

            int reqflops, idx_cum, maxmflops_sum, maxmflops_rem, temprank, pcost, p, treqflops, rprocs, status;

            double *class_procs = NULL, otime = 0.0;

            double comm_factor = 0.0;

            double ratio = 0, ratio_si = 0;

            double stime = 0.0, resttime = 0.0, spawn_cost = 0.0, remove_cost = 0.0, pstime, ptime, prtime, diff;

            double comm_cost, comp_cost, comp_cost_time, rdata_cost = 0.0;

            long long flop;

            comm_mean = (comm_mean / *size);

            //Evaluate system status
            EMPI_evaluate_system_status (*size, smonitor, &status);

            if (status == EMPI_NON_DEDICATED) fprintf (stderr, "WARNING: detected non-dedicated at %i\n", iter);

            //Estimated costs for the execution on the current configuration for one iteration
            //Computation estimated cost
            EMPI_Comp_cost (&comp_cost, &comp_cost_time, &flop, sflops, *size);
            //Communication estimated cost
            EMPI_Comm_cost (&comm_cost, *size, *size, EMPI_GLOBAL_niter);

            //Si es una aplicacion con comunicaciones
            if (comm_cost > 0) {
                comm_factor = (comm_mean / comm_cost);
                comm_cost *= comm_factor;
            }

            //MEDIDAS DE TIEMPO

            class = EMPI_GLOBAL_system_classes;

            for (n = 0, maxmflops_sum = 0, maxmflops_rem = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                maxmflops_sum += ((class->maxprocs-class->nprocs) * class->mflops);

                maxmflops_rem += (class->nprocs * class->mflops);

                class = class->next;
            }

            //aggregated overhead time
            otime = EMPI_GLOBAL_tover + ((PAPI_get_real_usec() - EMPI_GLOBAL_tover_ini) * 1.0E-6);

            //tiempo consumido
            EMPI_GLOBAL_cum_time = EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm + otime;

            //duracion estimada del siguiente sampling interval (computo+comunicaciones)
            stime = (comp_cost + comm_cost);

            //duracion que seria asumible para el siguiente sampling interval (ateniendonos al obj_exec)
            resttime = (double)((EMPI_GLOBAL_obj_texec - EMPI_GLOBAL_cum_time) / ((maxiter - iter)/EMPI_GLOBAL_niter));

            //Adding procs - stime (estimated time)
            if ((stime > resttime)&&(stime > (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)))&&(status == EMPI_DEDICATED)) {

                //memory allocation
                class_procs = (double*) calloc (EMPI_GLOBAL_nhclasses, sizeof(double));
                assert (class_procs);

                if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_LP) {

                    //Primera aproximacion PL

                    ratio = (EMPI_GLOBAL_tcomm / (EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm));
                    //ratio_si = (double)(smonitor[EMPI_root].ctime / ((smonitor[EMPI_root].rtime*1.0E-6) + smonitor[EMPI_root].ctime));
                    ratio_si = (double)(sum_ctime / (sum_rtime + sum_ctime));

                    if (ratio_si > (fabs(resttime-stime)/stime)) {

                        //Primera aproximacion PL
                        //reqflops = (int)((flop / (resttime * (1 - ratio))) - sflops);
                        //reqflops = (int)(((stime / resttime) * sflops) - sflops);

                        rprocs = newsize = 0;

                        treqflops = reqflops = 0;

                    } else {

                        //Primera aproximacion PL
                        //reqflops = (int)((flop / (resttime * (1 - ratio))) - sflops);
                        reqflops = (int)(((stime / (resttime * (1 - ratio))) * sflops) - sflops);

                        if (reqflops > maxmflops_sum) reqflops = maxmflops_sum;

                        EMPI_lp_min_procs (reqflops, &newsize, &mflops, class_procs);

                        rprocs = newsize;

                        treqflops = mflops;
                    }

                } else if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_EX) {

                    //Busqueda exhaustiva
                    for (p = 1, rprocs = 0; p <= (EMPI_GLOBAL_maxprocs - *size); p ++) {

                        //Cost of communications
                        EMPI_Comm_cost (&comm_cost, *size, p+*size, EMPI_GLOBAL_niter);
                        comm_cost *= comm_factor;

                        EMPI_Rdata_spawn_cost (*size, p, &rdata_cost);

                        //Cost of spawning 'newsize' procs
                        spawn_cost = EMPI_GLOBAL_spawn_cost * p;

                        //tiempo de computo que se deberia alcanzar (pstime) por iteracion para conseguir el tiempo restante (resttime), considerando todos los costes (spawn, comm) y threshold
                        pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;

                        //para tardar pstime de computo necesito reqflops
                        if (pstime > 0) {

                            reqflops = ((sflops * comp_cost) / pstime);

                            reqflops -= sflops;

                        } else reqflops = maxmflops_sum;

                        memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                        //NOTE: Esto esta bien porque para eficiencia no hay coste, coste = 0;
                        EMPI_lp_min_cost_fixed (reqflops, p, &mflops, &newsize, &pcost, class_procs);

                        //si no se encuentran p procs con reqflops, solicitamos max mflops de p procs con coste indiferente
                        if (newsize != p) EMPI_lp_flops_max (p, &mflops, &pcost, class_procs);

                        EMPI_Comp_cost (&ptime, &prtime, &flop, sflops+mflops, p+*size);

                        diff = fabs(resttime - (ptime + comm_cost + ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter))));

                        //Se selecciona la configuracion con el menor numero de procesos dentro de un threshold
                        if ((rprocs == 0)&&(diff < (resttime * EMPI_GLOBAL_obj_texec_threshold))) {

                            rprocs = p;

                            treqflops = mflops;
                        }
                    }
                }

                if (rprocs > 0) {

                    int *nextprocs = NULL;

                    newsize = rprocs;

                    memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                    EMPI_lp_get_procs_flops_spawn (treqflops, rprocs, class_procs);

                    hostid = (int*) malloc (newsize * sizeof(int));
                    assert (hostid);

                    nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                    assert (nextprocs);

                    for (n = 0; n < newsize; n ++) hostid[n] = -1;

                    class = EMPI_GLOBAL_system_classes;

                    //increase the number of processes
                    for (n = 0, idx = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                        class->nprocs = class->nprocs + class_procs[n];

                        for (m = 0; m < class_procs[n]; m ++) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            for (i = 0; i < EMPI_GLOBAL_nhosts; i ++) {

                                if ((hostlist->idclass == class->idclass)&&(hostlist->maxprocs > (hostlist->nprocs+nextprocs[hostlist->id]))&&(hostid[idx] < 0)) {

                                    hostid[idx] = i;

                                    idx ++;

                                    nextprocs[hostlist->id] ++;

                                    i = EMPI_GLOBAL_nhosts;
                                }

                                hostlist = hostlist->next;
                            }
                        }

                        class = class->next;
                    }

                    free (nextprocs);
                    nextprocs = NULL;

                } else {

                    newsize = 0;
                    mflops = 0;
                }

                //set newsize and mflops
                reconfig[0] = newsize + *size;
                reconfig[1] = mflops + sflops;

                free (class_procs);
                class_procs = NULL;

            //Removing procs - stime (estimated time)
            } else if ((stime < resttime)&&(resttime > (stime * (1 + EMPI_GLOBAL_obj_texec_threshold)))&&(status == EMPI_DEDICATED)) {

                //memory allocation
                class_procs = (double*) calloc (EMPI_GLOBAL_nhclasses, sizeof(double));
                assert (class_procs);

                if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_LP) {

                    //Primera aproximacion PL

                    ratio = (EMPI_GLOBAL_tcomm / (EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm));
                    //ratio_si = (double)(smonitor[EMPI_root].ctime / ((smonitor[EMPI_root].rtime*1.0E-6) + smonitor[EMPI_root].ctime));
                    ratio_si = (double)(sum_ctime / (sum_rtime + sum_ctime));

                    if (ratio_si > (fabs(resttime-stime)/resttime)) {

                        //Primera aproximacion PL
                        //reqflops = (int)(sflops - (flop / (resttime * (1 - ratio))));
                        //reqflops = (int)(sflops - ((stime / (resttime * (1 - ratio))) * sflops));

                        rprocs = newsize = 0;

                        treqflops = reqflops = 0;

                    } else {

                        //Primera aproximacion PL
                        //reqflops = (int)(sflops - (flop / (resttime * (1 - ratio))));
                        reqflops = (int)(sflops - ((stime / (resttime * (1 - ratio))) * sflops));

                        if (reqflops > maxmflops_rem) reqflops = maxmflops_rem;

                        EMPI_lp_max_procs (reqflops, &newsize, &mflops, class_procs);

                        rprocs = newsize;

                        treqflops = mflops;
                    }

                } else if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_EX) {

                    //Busqueda exhaustiva

                    for (p = 1, rprocs = 0; p <= (*size - EMPI_GLOBAL_minprocs); p ++) {

                        //Cost of communications
                        EMPI_Comm_cost (&comm_cost, *size, *size-p, EMPI_GLOBAL_niter);
                        comm_cost *= comm_factor;

                        EMPI_Rdata_remove_cost (*size, p, &rdata_cost);

                        //Cost of removing 'newsize' procs
                        remove_cost = EMPI_GLOBAL_remove_cost * p;

                        //tiempo de computo que se deberia alcanzar (pstime) por iteracion para conseguir el tiempo restante (resttime), considerando todos los costes (remove, comm) y threshold
                        pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;

                        //para tardar pstime necesito disminuir reqflops
                        reqflops = sflops - ((sflops * comp_cost) / pstime);

                        memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                        //NOTE: Esto esta bien porque para eficiencia no hay coste, coste = 0;
                        EMPI_lp_max_cost_fixed (reqflops, p, &mflops, &newsize, &pcost, class_procs);

                        //si no se encuentran p procs con reqflops, solicitamos min mflops de p procs con coste indiferente
                        if (newsize != p) EMPI_lp_flops_min (p, &mflops, &pcost, class_procs);

                        EMPI_Comp_cost (&ptime, &prtime, &flop, sflops-mflops, *size-p);

                        diff = fabs(resttime - (ptime + comm_cost + ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter))));

                        //Se selecciona la configuracion con mayor numero de procesos y que su desviacion no supere el threshold
                        if ((p > rprocs)&&(diff < (resttime * EMPI_GLOBAL_obj_texec_threshold))) {

                            rprocs = p;

                            treqflops = mflops;
                        }
                    }
                }

                if (rprocs > 0) {

                    int *nextprocs = NULL;

                    newsize = rprocs;

                    memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                    EMPI_lp_get_procs_flops_remove (treqflops, rprocs, class_procs);

                    rremvs = (int*) malloc (newsize * sizeof(int));
                    assert (rremvs);

                    nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                    assert (nextprocs);

                    for (n = 0; n < newsize; n ++) rremvs[n] = -1;

                    class = EMPI_GLOBAL_system_classes;

                    //decrease the number of processes
                    for (n = 0, idx = 0, idx_cum = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                        class->nprocs = class->nprocs - class_procs[n];

                        idx_cum += class_procs[n];

                        //decrease the number of processes
                        for (m = 0; m < class_procs[n]; m ++) {

                            for (i = (*size-1); i >= 0; i --) {

                                hostlist = EMPI_GLOBAL_hostlist;

                                while ((hostlist != NULL)&&(hostlist->id < smonitor[i].hostid)) hostlist = hostlist->next;

                                if ((hostlist != NULL)&&(class->idclass == hostlist->idclass)&&((hostlist->nprocs-nextprocs[hostlist->id]) > hostlist->iniprocs)&&(idx < idx_cum)) {

                                    rremvs[idx] = i;

                                    nextprocs[hostlist->id] ++;

                                    idx ++;
                                }
                            }
                        }

                        class = class->next;
                    }

                    free (nextprocs);
                    nextprocs = NULL;

                    //sort rremvs
                    for(n = 0; n < newsize-1; n++){

                        for(m = n+1; m < newsize; m++){

                            if(rremvs[m] > rremvs[n]){

                                temprank = rremvs[m];
                                rremvs[m] = rremvs[n];
                                rremvs[n] = temprank;
                            }
                        }
                    }

                } else {

                    newsize = 0;
                    mflops = 0;
                }

                //set newsize and mflops
                reconfig[0] = *size - newsize;
                reconfig[1] = sflops - mflops;

                free (class_procs);
                class_procs = NULL;
            }
        }

        if (sflops > 0) {

            //Bcast newsize and mflops
            PMPI_Bcast (reconfig, 2, MPI_INT, EMPI_root, EMPI_COMM_WORLD);

            EMPI_GLOBAL_overhead_rpolicy += (MPI_Wtime() - tini);

            //get newsize and mflops
            newsize = reconfig[0];
            mflops = reconfig[1];

            // launch or remove the selected processes
            EMPI_Exec_Malleability  (rank, size, count, disp, smonitor, fc, argv, bin, newsize, mflops, &hostid, &rremvs, 1);

        }
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Monitor_efficiency in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_efficiency_irregular'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_efficiency_irregular (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Monitor_efficiency_irregular in <%s> ***\n", __FILE__);
    #endif

    double tini = 0;

    tini = MPI_Wtime();

    int sflops, n, m, i, idx, newsize = 0, mflops = 0, reconfig[2] = {0, 0}, *hostid = NULL, *rremvs = NULL;

    double sum_ctime = 0, sum_rtime = 0, sdflops = 0, iflops;

    long long sum_tflops = 0;

    for (n = 0, sflops = 0, iflops = 0; n < *size; n ++) {

        sflops += (int)(smonitor[n].flops/smonitor[n].rtime);

        sdflops += (double)((double)smonitor[n].flops/(double)smonitor[n].rtime);

        if (n < EMPI_GLOBAL_minprocs) iflops += (double)((double)smonitor[n].flops/(double)smonitor[n].rtime);

        sum_tflops += smonitor[n].flops;

        sum_rtime += (smonitor[n].rtime*1.0E-6);

        sum_ctime += smonitor[n].ctime;
    }

    if ((*rank == EMPI_root)&&(sdflops > 0)) {

        EMPI_Class_type *class = NULL;

        EMPI_host_type *hostlist = NULL;

        int reqflops, idx_cum, maxmflops_sum, maxmflops_rem, temprank, pcost, p, treqflops, rprocs, status, realflops, rclass;

        double *class_procs = NULL, otime = 0.0, treal_consum = 0.0;

        double comm_factor = 0.0, comp_factor = 0.0, flop_factor = 0.0;

        double ratio = 0, ratio_si = 0;

        double perc_consum = 0.0, sum_perc = 0.0, sum_coef = 0.0;

        double stime = 0.0, resttime = 0.0, spawn_cost = 0.0, remove_cost = 0.0, pstime, ptime, diff, tcomp_orig = 0.0, tcomm_orig = 0.0;

        double comm_cost, comp_cost, rdata_cost = 0.0, coef = 0.0;

        //Evaluate system status
        EMPI_evaluate_system_status (*size, smonitor, &status);

        otime = (EMPI_GLOBAL_tover + ((PAPI_get_real_usec() - EMPI_GLOBAL_tover_ini) * 1.0E-6)) - EMPI_GLOBAL_over_si;

        EMPI_GLOBAL_over_si = EMPI_GLOBAL_tover + ((PAPI_get_real_usec() - EMPI_GLOBAL_tover_ini) * 1.0E-6);

        if (status == EMPI_NON_DEDICATED) fprintf (stderr, "WARNING: detected non-dedicated at %i\n", iter);

        //MEDIDAS DE TIEMPO

        //Recalcular porcentaje global
        for (n = (iter/EMPI_GLOBAL_niter)-1; n < maxiter/EMPI_GLOBAL_niter; n ++) sum_perc += EMPI_GLOBAL_profile_weigth[n];

        sum_coef = sum_perc * (1 - EMPI_GLOBAL_percentage);

        tcomp_orig = (double)((double)(EMPI_GLOBAL_profile_flops[(iter/EMPI_GLOBAL_niter)-1]*1.0E-6)/(double)EMPI_GLOBAL_profile_mflops[(iter/EMPI_GLOBAL_niter)-1]);
        tcomm_orig = EMPI_GLOBAL_profile_comms[(iter/EMPI_GLOBAL_niter)-1];

        resttime = (tcomp_orig + tcomm_orig) * (1 - EMPI_GLOBAL_percentage);

        treal_consum = (((double)((double)sum_tflops)/(double)sdflops) * 1.0E-6) + (sum_ctime / *size) + otime;
        //treal_consum = (((double)((double)sum_tflops)/(double)sdflops) * 1.0E-6) + (sum_ctime / *size);
        fprintf (stderr, "EMPI_Monitor_efficiency_irregular(%d): %i-%i - rtime %lf (comp %lf comm %lf) tflops %lld otime %lf\n", getpid(), iter-EMPI_GLOBAL_niter, iter, treal_consum-otime, (((double)((double)sum_tflops)/(double)sdflops) * 1.0E-6), sum_ctime / *size, sum_tflops, otime);
        perc_consum = ((treal_consum * EMPI_GLOBAL_profile_weigth[(iter/EMPI_GLOBAL_niter)-1]) / resttime);

        if (treal_consum > resttime) {

            coef = (sum_coef - (EMPI_GLOBAL_profile_weigth[(iter/EMPI_GLOBAL_niter)-1] * (1 - EMPI_GLOBAL_percentage))) / (sum_coef - (perc_consum * (1 - EMPI_GLOBAL_percentage)));

        } else if (treal_consum < resttime) {

            coef = (sum_coef - (perc_consum * (1 - EMPI_GLOBAL_percentage))) / (sum_coef - (EMPI_GLOBAL_profile_weigth[(iter/EMPI_GLOBAL_niter)-1] * (1 - EMPI_GLOBAL_percentage)));
        }

        EMPI_GLOBAL_percentage = EMPI_GLOBAL_percentage * coef;

        //Factores de correccion
        flop_factor = (double)((double) sum_tflops / (double)EMPI_GLOBAL_profile_flops[(iter/EMPI_GLOBAL_niter)-1]);

        comp_factor = (double)EMPI_GLOBAL_profile_mflops[(iter/EMPI_GLOBAL_niter)-1] / iflops;

        comp_factor *= (double)((double)EMPI_GLOBAL_profile_mflops[iter/EMPI_GLOBAL_niter] / (double)EMPI_GLOBAL_profile_mflops[(iter/EMPI_GLOBAL_niter)-1]);

        //Tiempo de ejecucion del siguiente intervalo con la configuracion inicial
        tcomp_orig = (double)((double)(EMPI_GLOBAL_profile_flops[iter/EMPI_GLOBAL_niter]*1.0E-6*flop_factor)/(double)EMPI_GLOBAL_profile_mflops[iter/EMPI_GLOBAL_niter]);
        tcomm_orig = (double)(EMPI_GLOBAL_profile_comms[iter/EMPI_GLOBAL_niter]);

        //Tiempo objetivo del siguiente intervalo
        resttime = (tcomp_orig + tcomm_orig) * (1 - EMPI_GLOBAL_percentage);

        //Factor de comunicaciones
        if (EMPI_GLOBAL_comm_prev > 0) {

            comm_factor = (double)((sum_ctime / *size) / EMPI_GLOBAL_comm_prev);

        } else comm_factor = 1;

        EMPI_Comm_cost (&comm_cost, EMPI_GLOBAL_minprocs, EMPI_GLOBAL_minprocs, EMPI_GLOBAL_niter);

        comm_factor *= (double)(EMPI_GLOBAL_profile_comms[iter/EMPI_GLOBAL_niter] / comm_cost);

        //Estimated costs for the execution on the current configuration for one sampling interval
        //Computation estimated cost
        comp_cost = (EMPI_GLOBAL_profile_flops[iter/EMPI_GLOBAL_niter]*1.0E-6*flop_factor) / (sdflops * comp_factor);
        
        //Communication estimated cost
        EMPI_Comm_cost (&comm_cost, EMPI_GLOBAL_minprocs, *size, EMPI_GLOBAL_niter);

        comm_cost *= comm_factor;

        EMPI_GLOBAL_comm_prev = comm_cost;

        //duracion estimada del siguiente sampling interval (computo+comunicaciones)
        stime = (comp_cost + comm_cost);

        class = EMPI_GLOBAL_system_classes;

        for (n = 0, maxmflops_sum = 0, maxmflops_rem = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

            maxmflops_sum += ((class->maxprocs-class->nprocs) * class->mflops);

            maxmflops_rem += (class->nprocs * class->mflops);

            class = class->next;
        }

        //tiempo consumido
        EMPI_GLOBAL_cum_time = EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm + EMPI_GLOBAL_tover;
        fprintf (stderr, "EMPI_Monitor_efficiency_irregular(%d): %i-%i - stime %lf (comp %lf comm %lf) sflops %.0lf - resttime %lf percentage %lf - cum_time %lf\n", getpid(), iter, iter+EMPI_GLOBAL_niter, stime, comp_cost, comm_cost, EMPI_GLOBAL_profile_flops[iter/EMPI_GLOBAL_niter] * flop_factor, resttime, EMPI_GLOBAL_percentage, EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm + EMPI_GLOBAL_tover);
        //Adding procs - stime (estimated time)
        //if (stime > resttime) {
        if ((stime > resttime)&&(stime > (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)))) {

            //memory allocation
            class_procs = (double*) calloc (EMPI_GLOBAL_nhclasses, sizeof(double));
            assert (class_procs);

            if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_LP) {

                //Primera aproximacion PL

                ratio = (EMPI_GLOBAL_tcomm / (EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm));
                //ratio_si = (double)(smonitor[EMPI_root].ctime / ((smonitor[EMPI_root].rtime*1.0E-6) + smonitor[EMPI_root].ctime));
                ratio_si = (double)(sum_ctime / (sum_rtime + sum_ctime));

                if (ratio_si > (fabs(resttime-stime)/stime)) {

                    //Primera aproximacion PL
                    //reqflops = (int)((flop / (resttime * (1 - ratio))) - sflops);
                    //reqflops = (int)(((stime / resttime) * sflops) - sflops);

                    rprocs = newsize = 0;

                    treqflops = reqflops = 0;

                } else {

                    //Primera aproximacion PL
                    //reqflops = (int)((flop / (resttime * (1 - ratio))) - sflops);
                    reqflops = (int)(((stime / (resttime * (1 - ratio))) * sflops) - sflops);

                    if (reqflops > maxmflops_sum) reqflops = maxmflops_sum;

                    EMPI_lp_min_procs (reqflops, &newsize, &mflops, class_procs);

                    rprocs = newsize;

                    treqflops = mflops;
                }

            } else if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_EX) {

                //Busqueda exhaustiva
                for (p = 1, rprocs = 0; p <= (EMPI_GLOBAL_maxprocs - *size); p ++) {

                    //Cost of communications
                    EMPI_Comm_cost (&comm_cost, *size, p+*size, EMPI_GLOBAL_niter);
                    comm_cost *= comm_factor;

                    EMPI_Rdata_spawn_cost (*size, p, &rdata_cost);

                    //Cost of spawning 'newsize' procs
                    spawn_cost = EMPI_GLOBAL_spawn_cost * p;

                    //tiempo de computo que se deberia alcanzar (pstime) por iteracion para conseguir el tiempo restante (resttime), considerando todos los costes (spawn, comm) y threshold
                    //pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;
                    //pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - spawn_cost - rdata_cost - comm_cost;
                    //pstime = resttime - ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;
                    pstime = resttime - ((spawn_cost + rdata_cost)) - comm_cost;

                    //para tardar pstime de computo necesito reqflops
                    if (pstime > 0) {

                        //reqflops = ceil((sdflops * comp_cost) / pstime);

                        reqflops = ceil((iflops * tcomp_orig) / pstime);

                        reqflops -= sdflops;

                        //reqflops -= (sdflops / comp_factor);

                    } else reqflops = maxmflops_sum;

                    memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                    //NOTE: Esto esta bien porque para eficiencia no hay coste, coste = 0;
                    EMPI_lp_min_cost_fixed (reqflops, p, &mflops, &newsize, &pcost, class_procs);

                    //si no se encuentran p procs con reqflops, solicitamos max mflops de p procs con coste indiferente
                    if (newsize != p) EMPI_lp_flops_max (p, &mflops, &pcost, class_procs);

                    ptime = (double)((double)(EMPI_GLOBAL_profile_flops[iter/EMPI_GLOBAL_niter]*1.0E-6*flop_factor)/(double)((sdflops+mflops) * comp_factor));

                    //diff = fabs(resttime - (ptime + comm_cost + ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter))));
                    diff = fabs(resttime - (ptime + comm_cost + spawn_cost + rdata_cost));

                    //Se selecciona la configuracion con el menor numero de procesos dentro de un threshold
                    if ((rprocs == 0)&&(diff < (resttime * EMPI_GLOBAL_obj_texec_threshold))) {
                        fprintf (stderr, "EMPI_Monitor_efficiency_irregular(%d): Estimacion computo %lf estimacion communicaciones %lf overheads %lf - diff %lf maxdiff %lf\n", getpid(), ptime, comm_cost, spawn_cost + rdata_cost, diff, resttime * EMPI_GLOBAL_obj_texec_threshold);
                        EMPI_GLOBAL_comm_prev = comm_cost;

                        rprocs = p;

                        treqflops = mflops;
                    }
                }
            }

            if (rprocs > 0) {

                int *nextprocs = NULL;

                newsize = rprocs;

                memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                EMPI_lp_get_procs_flops_spawn (treqflops, rprocs, class_procs);

                hostid = (int*) malloc (newsize * sizeof(int));
                assert (hostid);

                nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                assert (nextprocs);

                for (n = 0; n < newsize; n ++) hostid[n] = -1;

                class = EMPI_GLOBAL_system_classes;

                //increase the number of processes
                for (n = 0, idx = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                    class->nprocs = class->nprocs + class_procs[n];

                    for (m = 0; m < class_procs[n]; m ++) {

                        hostlist = EMPI_GLOBAL_hostlist;

                        for (i = 0; i < EMPI_GLOBAL_nhosts; i ++) {

                            if ((hostlist->idclass == class->idclass)&&(hostlist->maxprocs > (hostlist->nprocs+nextprocs[hostlist->id]))&&(hostid[idx] < 0)) {

                                hostid[idx] = i;

                                idx ++;

                                nextprocs[hostlist->id] ++;

                                i = EMPI_GLOBAL_nhosts;
                            }

                            hostlist = hostlist->next;
                        }
                    }

                    class = class->next;
                }

                free (nextprocs);
                nextprocs = NULL;

            } else {

                newsize = 0;
                mflops = 0;
            }

            //set newsize and mflops
            reconfig[0] = newsize + *size;
            reconfig[1] = mflops + sflops;

            free (class_procs);
            class_procs = NULL;

        //Removing procs - stime (estimated time)
        //} else if (stime < resttime) {
        } else if ((stime < resttime)&&(resttime > (stime * (1 + EMPI_GLOBAL_obj_texec_threshold)))) {

            //memory allocation
            class_procs = (double*) calloc (EMPI_GLOBAL_nhclasses, sizeof(double));
            assert (class_procs);

            if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_LP) {

                //Primera aproximacion PL

                ratio = (EMPI_GLOBAL_tcomm / (EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm));
                //ratio_si = (double)(smonitor[EMPI_root].ctime / ((smonitor[EMPI_root].rtime*1.0E-6) + smonitor[EMPI_root].ctime));
                ratio_si = (double)(sum_ctime / (sum_rtime + sum_ctime));

                if (ratio_si > (fabs(resttime-stime)/resttime)) {

                    //Primera aproximacion PL
                    //reqflops = (int)(sflops - (flop / (resttime * (1 - ratio))));
                    //reqflops = (int)(sflops - ((stime / (resttime * (1 - ratio))) * sflops));

                    rprocs = newsize = 0;

                    treqflops = reqflops = 0;

                } else {

                    //Primera aproximacion PL
                    //reqflops = (int)(sflops - (flop / (resttime * (1 - ratio))));
                    reqflops = (int)(sflops - ((stime / (resttime * (1 - ratio))) * sflops));

                    if (reqflops > maxmflops_rem) reqflops = maxmflops_rem;

                    EMPI_lp_max_procs (reqflops, &newsize, &mflops, class_procs);

                    rprocs = newsize;

                    treqflops = mflops;
                }

            } else if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_EX) {

                //Busqueda exhaustiva

                for (p = 1, rprocs = 0, realflops = 0; p <= (*size - EMPI_GLOBAL_minprocs); p ++) {

                    //Cost of communications
                    EMPI_Comm_cost (&comm_cost, *size, *size-p, EMPI_GLOBAL_niter);
                    comm_cost *= comm_factor;

                    EMPI_Rdata_remove_cost (*size, p, &rdata_cost);

                    //Cost of removing 'newsize' procs
                    remove_cost = EMPI_GLOBAL_remove_cost * p;

                    //tiempo de computo que se deberia alcanzar (pstime) por iteracion para conseguir el tiempo restante (resttime), considerando todos los costes (remove, comm) y threshold
                    //pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;
                    //pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - remove_cost - rdata_cost - comm_cost;
                    //pstime = resttime - ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;
                    pstime = resttime - (remove_cost + rdata_cost) - comm_cost;

                    //para tardar pstime necesito disminuir reqflops
                    //reqflops = sflops - ((sflops * comp_cost) / pstime);
                    reqflops = sflops - ((iflops * tcomp_orig) / pstime);
                    //reqflops = sflops - (((iflops * tcomp_orig) / pstime) * comp_factor);

                    memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                    //NOTE: Esto esta bien porque para eficiencia no hay coste, coste = 0;
                    EMPI_lp_max_cost_fixed (reqflops, p, &mflops, &newsize, &pcost, class_procs);

                    //si no se encuentran p procs con reqflops, solicitamos min mflops de p procs con coste indiferente
                    if (newsize != p) EMPI_lp_flops_min (p, &mflops, &pcost, class_procs);

                    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                        rclass = class_procs[n];

                        for (m = *size-1; m >= EMPI_GLOBAL_minprocs && rclass > 0; m --) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            while ((hostlist != NULL)&&(hostlist->id < smonitor[m].hostid)) hostlist = hostlist->next;

                            if (hostlist->idclass == n) {

                                realflops += (int)(smonitor[m].flops/smonitor[m].rtime);

                                rclass --;
                            }
                        }
                    }

                    ptime = (double)((double)(EMPI_GLOBAL_profile_flops[iter/EMPI_GLOBAL_niter]*1.0E-6*flop_factor) / (double)((sdflops-realflops) * comp_factor));

                    //diff = fabs(resttime - (ptime + comm_cost + ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter))));
                    diff = fabs(resttime - (ptime + comm_cost + remove_cost + rdata_cost));

                    //Se selecciona la configuracion con mayor numero de procesos y que su desviacion no supere el threshold
                    if ((p > rprocs)&&(diff < (resttime * EMPI_GLOBAL_obj_texec_threshold))) {
                        fprintf (stderr, "EMPI_Monitor_efficiency_irregular(%d): Estimacion computo %lf estimacion communicaciones %lf overheads %lf - diff %lf maxdiff %lf\n", getpid(), ptime, comm_cost, remove_cost + rdata_cost, diff, resttime * EMPI_GLOBAL_obj_texec_threshold);
                        EMPI_GLOBAL_comm_prev = comm_cost;

                        rprocs = p;

                        treqflops = mflops;
                    }
                }
            }

            if (rprocs > 0) {

                int *nextprocs = NULL;

                newsize = rprocs;

                memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                EMPI_lp_get_procs_flops_remove (treqflops, rprocs, class_procs);

                rremvs = (int*) malloc (newsize * sizeof(int));
                assert (rremvs);

                nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                assert (nextprocs);

                for (n = 0; n < newsize; n ++) rremvs[n] = -1;

                class = EMPI_GLOBAL_system_classes;

                //decrease the number of processes
                for (n = 0, idx = 0, idx_cum = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                    class->nprocs = class->nprocs - class_procs[n];

                    idx_cum += class_procs[n];

                    //decrease the number of processes
                    for (m = 0; m < class_procs[n]; m ++) {

                        for (i = (*size-1); i >= 0; i --) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            while ((hostlist != NULL)&&(hostlist->id < smonitor[i].hostid)) hostlist = hostlist->next;

                            if ((hostlist != NULL)&&(class->idclass == hostlist->idclass)&&((hostlist->nprocs-nextprocs[hostlist->id]) > hostlist->iniprocs)&&(idx < idx_cum)) {

                                rremvs[idx] = i;

                                nextprocs[hostlist->id] ++;

                                idx ++;
                            }
                        }
                    }

                    class = class->next;
                }

                free (nextprocs);
                nextprocs = NULL;

                //sort rremvs
                for(n = 0; n < newsize-1; n++){

                    for(m = n+1; m < newsize; m++){

                        if(rremvs[m] > rremvs[n]){

                            temprank = rremvs[m];
                            rremvs[m] = rremvs[n];
                            rremvs[n] = temprank;
                        }
                    }
                }

            } else {

                newsize = 0;
                mflops = 0;
            }

            //set newsize and mflops
            reconfig[0] = *size - newsize;
            reconfig[1] = sflops - mflops;

            free (class_procs);
            class_procs = NULL;
        }
    }

    if (sdflops > 0) {

        //Bcast newsize and mflops
        PMPI_Bcast (reconfig, 2, MPI_INT, EMPI_root, EMPI_COMM_WORLD);

        EMPI_GLOBAL_overhead_rpolicy += (MPI_Wtime() - tini);

        //get newsize and mflops
        newsize = reconfig[0];
        mflops = reconfig[1];

        // launch or remove the selected processes
        EMPI_Exec_Malleability  (rank, size, count, disp, smonitor, fc, argv, bin, newsize, mflops, &hostid, &rremvs, 1);
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Monitor_efficiency_irregular in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_cost'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_cost (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Monitor_cost in <%s> ***\n", __FILE__);
    #endif

    double tini = 0;

    int lbalance = EMPI_NULL;

    if (EMPI_GLOBAL_lbalance == EMPI_TRUE) lbalance = EMPI_LBalance (rank, size, count, disp, fc, smonitor);

    if ((lbalance == EMPI_BALANCED)||(lbalance == EMPI_NULL)) {

        tini = MPI_Wtime();

        int sflops, n, m, i, idx, newsize = 0, mflops = 0, reconfig[2] = {0, 0}, *hostid = NULL, *rremvs = NULL;

        double comm_mean = 0.0, sum_ctime = 0, sum_rtime = 0;

        for (n = 0, sflops = 0; n < *size; n ++) {

            sflops += (int)(smonitor[n].flops/smonitor[n].rtime);

            sum_rtime += (smonitor[EMPI_root].rtime*1.0E-6);

            sum_ctime += smonitor[n].ctime;

            comm_mean += smonitor[n].ctime;
        }

        if ((*rank == EMPI_root)&&(sflops > 0)) {

            EMPI_Class_type *class = NULL;

            EMPI_host_type *hostlist = NULL;

            int reqflops, idx_cum, maxmflops_sum, maxmflops_rem, temprank, pcost, p, treqflops, rprocs, actual_cost, status;

            double *class_procs = NULL, otime = 0.0;

            double ratio = 0, ratio_si = 0;

            double comm_factor = 0.0, mincost, pscost;

            double stime = 0.0, resttime = 0.0, spawn_cost = 0.0, remove_cost = 0.0, pstime, ptime, prtime, diff;

            double comm_cost, comp_cost, comp_cost_time, rdata_cost = 0.0;

            long long flop;

            comm_mean = (comm_mean / *size);

            //Evaluate system status
            EMPI_evaluate_system_status (*size, smonitor, &status);

            if (status == EMPI_NON_DEDICATED) fprintf (stderr, "WARNING: detected non-dedicated at %i\n", iter);

            //Estimated costs for the execution on the current configuration for one iteration
            //Computation estimated cost
            EMPI_Comp_cost (&comp_cost, &comp_cost_time, &flop, sflops, *size);
            //Communication estimated cost
            EMPI_Comm_cost (&comm_cost, *size, *size, EMPI_GLOBAL_niter);

            //Si es una aplicacion con comunicaciones
            if (comm_cost > 0) {
                comm_factor = (comm_mean / comm_cost);
                comm_cost *= comm_factor;
            }

            //MEDIDAS DE TIEMPO

            class = EMPI_GLOBAL_system_classes;

            for (n = 0, maxmflops_sum = 0, maxmflops_rem = 0, actual_cost = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                maxmflops_sum += ((class->maxprocs-class->nprocs) * class->mflops);

                maxmflops_rem += (class->nprocs * class->mflops);

                actual_cost += (class->nprocs + class->iniprocs) * class->cost;

                class = class->next;
            }

            //aggregated overhead time
            otime = EMPI_GLOBAL_tover + ((PAPI_get_real_usec() - EMPI_GLOBAL_tover_ini) * 1.0E-6);

            //tiempo consumido
            EMPI_GLOBAL_cum_time = EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm + otime;

            //duracion estimada del siguiente sampling interval (computo+comunicaciones)
            stime = (comp_cost + comm_cost);

            //duracion que seria asumible para el siguiente sampling interval (ateniendonos al obj_exec)
            resttime = (double)((EMPI_GLOBAL_obj_texec - EMPI_GLOBAL_cum_time) / ((maxiter - iter)/EMPI_GLOBAL_niter));

            //Adding procs - stime (estimated time)
            if ((stime > resttime)&&(stime > (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)))&&(status == EMPI_DEDICATED)) {

                //memory allocation
                class_procs = (double*) calloc (EMPI_GLOBAL_nhclasses, sizeof(double));
                assert (class_procs);

                if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_LP) {

                    //Primera aproximacion PL

                    ratio = (EMPI_GLOBAL_tcomm / (EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm));
                    //ratio_si = (double)(smonitor[EMPI_root].ctime / ((smonitor[EMPI_root].rtime*1.0E-6) + smonitor[EMPI_root].ctime));
                    ratio_si = (double)(sum_ctime / (sum_rtime + sum_ctime));

                    if (ratio_si > (fabs(resttime-stime)/stime)) {

                        //Primera aproximacion PL
                        //reqflops = (int)((flop / (resttime * (1 - ratio))) - sflops);
                        //reqflops = (int)(((stime / resttime) * sflops) - sflops);

                        rprocs = newsize = 0;

                        treqflops = reqflops = 0;

                    } else {

                        //Primera aproximacion PL
                        //reqflops = (int)((flop / (resttime * (1 - ratio))) - sflops);
                        reqflops = (int)(((stime / (resttime * (1 - ratio))) * sflops) - sflops);

                        if (reqflops > maxmflops_sum) reqflops = maxmflops_sum;

                        EMPI_lp_min_cost (reqflops, &newsize, &mflops, &pcost, class_procs);

                        rprocs = newsize;

                        treqflops = mflops;
                    }

                } else if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_EX) {

                    //Busqueda exhaustiva
                    for (p = 1, rprocs = 0, mincost = -1; p <= (EMPI_GLOBAL_maxprocs - *size); p ++) {

                        //Cost of communications
                        EMPI_Comm_cost (&comm_cost, *size, p+*size, EMPI_GLOBAL_niter);
                        comm_cost *= comm_factor;

                        EMPI_Rdata_spawn_cost (*size, p, &rdata_cost);

                        //Cost of spawning 'newsize' procs
                        spawn_cost = EMPI_GLOBAL_spawn_cost * p;

                        //tiempo de computo que se deberia alcanzar (pstime) por iteracion para conseguir el tiempo restante (resttime), considerando todos los costes (spawn, comm) y threshold
                        pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - ((spawn_cost + rdata_cost)/ ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;

                        //para tardar pstime de computo necesito reqflops
                        if (pstime > 0) {

                            reqflops = ((sflops * comp_cost) / pstime);

                            reqflops -= sflops;

                        } else reqflops = maxmflops_sum;

                        memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                        //Se intenta aumentar reqflops con p procesos al minimo coste
                        EMPI_lp_min_cost_fixed (reqflops, p, &mflops, &newsize, &pcost, class_procs);

                        //si no se encuentran p procs con reqflops, solicitamos max mflops de p procs con coste indiferente
                        if (newsize != p) EMPI_lp_flops_max (p, &mflops, &pcost, class_procs);

                        EMPI_Comp_cost (&ptime, &prtime, &flop, sflops+mflops, p+*size);

                        diff = fabs(resttime - (ptime + comm_cost + ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter))));

                        pscost = (pcost + actual_cost) * (ptime + comm_cost + ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)));

                        //Se selecciona la configuracion con el menor coste dentro de un threshold
                        if (((mincost == -1)||(pscost < mincost))&&(diff < (resttime * EMPI_GLOBAL_obj_texec_threshold))) {

                            rprocs = p;

                            treqflops = mflops;

                            mincost = pscost;
                        }
                    }
                }

                if (rprocs > 0) {

                    int *nextprocs = NULL;

                    newsize = rprocs;

                    memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                    EMPI_lp_get_procs_flops_spawn (treqflops, rprocs, class_procs);

                    hostid = (int*) malloc (newsize * sizeof(int));
                    assert (hostid);

                    nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                    assert (nextprocs);

                    for (n = 0; n < newsize; n ++) hostid[n] = -1;

                    class = EMPI_GLOBAL_system_classes;

                    //increase the number of processes
                    for (n = 0, idx = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                        class->nprocs = class->nprocs + class_procs[n];

                        for (m = 0; m < class_procs[n]; m ++) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            for (i = 0; i < EMPI_GLOBAL_nhosts; i ++) {

                                if ((hostlist->idclass == class->idclass)&&(hostlist->maxprocs > (hostlist->nprocs+nextprocs[hostlist->id]))&&(hostid[idx] < 0)) {

                                    hostid[idx] = i;

                                    idx ++;

                                    nextprocs[hostlist->id] ++;

                                    i = EMPI_GLOBAL_nhosts;
                                }

                                hostlist = hostlist->next;
                            }
                        }

                        class = class->next;
                    }

                    free (nextprocs);
                    nextprocs = NULL;

                } else {

                    newsize = 0;
                    mflops = 0;
                }

                //set newsize and mflops
                reconfig[0] = newsize + *size;
                reconfig[1] = mflops + sflops;

                free (class_procs);
                class_procs = NULL;

            //Removing procs - stime (estimated time)
            } else if ((stime < resttime)&&(resttime > (stime * (1 + EMPI_GLOBAL_obj_texec_threshold)))&&(status == EMPI_DEDICATED)) {

                //memory allocation
                class_procs = (double*) calloc (EMPI_GLOBAL_nhclasses, sizeof(double));
                assert (class_procs);

                if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_LP) {

                    //Primera aproximacion PL

                    ratio = (EMPI_GLOBAL_tcomm / (EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm));
                    //ratio_si = (double)(smonitor[EMPI_root].ctime / ((smonitor[EMPI_root].rtime*1.0E-6) + smonitor[EMPI_root].ctime));
                    ratio_si = (double)(sum_ctime / (sum_rtime + sum_ctime));

                    if (ratio_si > (fabs(resttime-stime)/resttime)) {

                        //Primera aproximacion PL
                        //reqflops = (int)(sflops - (flop / (resttime * (1 - ratio))));
                        //reqflops = (int)(sflops - ((stime / (resttime * (1 - ratio))) * sflops));

                        rprocs = newsize = 0;

                        treqflops = reqflops = 0;

                    } else {

                        //Primera aproximacion PL
                        //reqflops = (int)(sflops - (flop / (resttime * (1 - ratio))));
                        reqflops = (int)(sflops - ((stime / (resttime * (1 - ratio))) * sflops));

                        if (reqflops > maxmflops_rem) reqflops = maxmflops_rem;

                        EMPI_lp_max_cost (reqflops, &newsize, &mflops, &pcost, class_procs);

                        rprocs = newsize;

                        treqflops = mflops;
                    }

                } else if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_EX) {

                    //Busqueda exhaustiva

                    for (p = 1, rprocs = 0, mincost = -1; p <= (*size - EMPI_GLOBAL_minprocs); p ++) {

                        //Cost of communications
                        EMPI_Comm_cost (&comm_cost, *size, *size-p, EMPI_GLOBAL_niter);
                        comm_cost *= comm_factor;

                        EMPI_Rdata_remove_cost (*size, p, &rdata_cost);

                        //Cost of removing 'newsize' procs
                        remove_cost = EMPI_GLOBAL_remove_cost * p;

                        //tiempo de computo que se deberia alcanzar (pstime) por iteracion para conseguir el tiempo restante (resttime), considerando todos los costes (remove, comm) y threshold
                        pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;

                        //para tardar pstime necesito disminuir reqflops
                        reqflops = sflops - ((sflops * comp_cost) / pstime);

                        memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                        //remover reqflops con maximo coste
                        EMPI_lp_max_cost_fixed (reqflops, p, &mflops, &newsize, &pcost, class_procs);

                        //si no se encuentran p procs con reqflops, solicitamos min mflops de p procs con coste indiferente
                        if (newsize != p) EMPI_lp_flops_min (p, &mflops, &pcost, class_procs);

                        EMPI_Comp_cost (&ptime, &prtime, &flop, sflops-mflops, p-*size);

                        diff = fabs(resttime - (ptime + comm_cost + ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter))));

                        pscost = (actual_cost - pcost) * (ptime + comm_cost + ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)));

                        //Se selecciona la configuracion que quita mayor coste y que su desviacion no supere el threshold
                        if (((mincost == -1)||(pscost < mincost))&&(diff < (resttime * EMPI_GLOBAL_obj_texec_threshold))) {

                            rprocs = p;

                            treqflops = mflops;

                            mincost = pscost;
                        }
                    }
                }

                if (rprocs > 0) {

                    int *nextprocs = NULL;

                    newsize = rprocs;

                    memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                    EMPI_lp_get_procs_flops_remove (treqflops, rprocs, class_procs);

                    rremvs = (int*) malloc (newsize * sizeof(int));
                    assert (rremvs);

                    nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                    assert (nextprocs);

                    for (n = 0; n < newsize; n ++) rremvs[n] = -1;

                    class = EMPI_GLOBAL_system_classes;

                    //decrease the number of processes
                    for (n = 0, idx = 0, idx_cum = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                        class->nprocs = class->nprocs - class_procs[n];

                        idx_cum += class_procs[n];

                        //decrease the number of processes
                        for (m = 0; m < class_procs[n]; m ++) {

                            for (i = (*size-1); i >= 0; i --) {

                                hostlist = EMPI_GLOBAL_hostlist;

                                while ((hostlist != NULL)&&(hostlist->id < smonitor[i].hostid)) hostlist = hostlist->next;

                                if ((hostlist != NULL)&&(class->idclass == hostlist->idclass)&&((hostlist->nprocs-nextprocs[hostlist->id]) > hostlist->iniprocs)&&(idx < idx_cum)) {

                                    rremvs[idx] = i;

                                    nextprocs[hostlist->id] ++;

                                    idx ++;
                                }
                            }
                        }

                        class = class->next;
                    }

                    free (nextprocs);
                    nextprocs = NULL;

                    //sort rremvs
                    for(n = 0; n < newsize-1; n++){

                        for(m = n+1; m < newsize; m++){

                            if(rremvs[m] > rremvs[n]){

                                temprank = rremvs[m];
                                rremvs[m] = rremvs[n];
                                rremvs[n] = temprank;
                            }
                        }
                    }

                } else {

                    newsize = 0;
                    mflops = 0;
                }

                //set newsize and mflops
                reconfig[0] = *size - newsize;
                reconfig[1] = sflops - mflops;

                free (class_procs);
                class_procs = NULL;
            }
        }

        if (sflops > 0) {

            //Bcast newsize and mflops
            PMPI_Bcast (reconfig, 2, MPI_INT, EMPI_root, EMPI_COMM_WORLD);

            EMPI_GLOBAL_overhead_rpolicy += (MPI_Wtime() - tini);

            //get newsize and mflops
            newsize = reconfig[0];
            mflops = reconfig[1];

            // launch or remove the selected processes
            EMPI_Exec_Malleability  (rank, size, count, disp, smonitor, fc, argv, bin, newsize, mflops, &hostid, &rremvs, 1);

        }
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Monitor_cost in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_cost_irregular'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_cost_irregular (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Monitor_cost_irregular in <%s> ***\n", __FILE__);
    #endif

    double tini = 0;

    tini = MPI_Wtime();

    int sflops, n, m, i, idx, newsize = 0, mflops = 0, reconfig[2] = {0, 0}, *hostid = NULL, *rremvs = NULL;

    double sum_ctime = 0, sum_rtime = 0, sdflops = 0, iflops;

    long long sum_tflops = 0;

    for (n = 0, sflops = 0, iflops = 0; n < *size; n ++) {

        sflops += (int)(smonitor[n].flops/smonitor[n].rtime);

        sdflops += (double)((double)smonitor[n].flops/(double)smonitor[n].rtime);

        if (n < EMPI_GLOBAL_minprocs) iflops += (double)((double)smonitor[n].flops/(double)smonitor[n].rtime);

        sum_tflops += smonitor[n].flops;

        sum_rtime += (smonitor[n].rtime*1.0E-6);

        sum_ctime += smonitor[n].ctime;
    }

    if ((*rank == EMPI_root)&&(sdflops > 0)) {

        EMPI_Class_type *class = NULL;

        EMPI_host_type *hostlist = NULL;

        int reqflops, idx_cum, maxmflops_sum, maxmflops_rem, temprank, pcost, p, treqflops, rprocs, status, actual_cost, realflops, rclass;

        double *class_procs = NULL, otime = 0.0, treal_consum = 0.0;

        double comm_factor = 0.0, comp_factor = 0.0, flop_factor = 0.0;

        double ratio = 0, ratio_si = 0, mincost = 0, pscost = 0;

        double perc_consum = 0.0, sum_perc = 0.0, sum_coef = 0.0;

        double stime = 0.0, resttime = 0.0, spawn_cost = 0.0, remove_cost = 0.0, pstime, ptime, diff, tcomp_orig = 0.0, tcomm_orig = 0.0;

        double comm_cost, comp_cost, rdata_cost = 0.0, coef = 0.0;

        //Evaluate system status
        EMPI_evaluate_system_status (*size, smonitor, &status);

        otime = (EMPI_GLOBAL_tover + ((PAPI_get_real_usec() - EMPI_GLOBAL_tover_ini) * 1.0E-6)) - EMPI_GLOBAL_over_si;

        EMPI_GLOBAL_over_si = EMPI_GLOBAL_tover + ((PAPI_get_real_usec() - EMPI_GLOBAL_tover_ini) * 1.0E-6);

        if (status == EMPI_NON_DEDICATED) fprintf (stderr, "WARNING: detected non-dedicated at %i\n", iter);

        //MEDIDAS DE TIEMPO

        //Recalcular porcentaje global
        for (n = (iter/EMPI_GLOBAL_niter)-1; n < maxiter/EMPI_GLOBAL_niter; n ++) sum_perc += EMPI_GLOBAL_profile_weigth[n];

        sum_coef = sum_perc * (1 - EMPI_GLOBAL_percentage);

        tcomp_orig = (double)((double)(EMPI_GLOBAL_profile_flops[(iter/EMPI_GLOBAL_niter)-1]*1.0E-6)/(double)EMPI_GLOBAL_profile_mflops[(iter/EMPI_GLOBAL_niter)-1]);
        tcomm_orig = EMPI_GLOBAL_profile_comms[(iter/EMPI_GLOBAL_niter)-1];

        resttime = (tcomp_orig + tcomm_orig) * (1 - EMPI_GLOBAL_percentage);

        treal_consum = (((double)((double)sum_tflops)/(double)sdflops) * 1.0E-6) + (sum_ctime / *size) + otime;
        //treal_consum = (((double)((double)sum_tflops)/(double)sdflops) * 1.0E-6) + (sum_ctime / *size);
        fprintf (stderr, "EMPI_Monitor_cost_irregular(%d): %i-%i - rtime %lf (comp %lf comm %lf) tflops %lld otime %lf\n", getpid(), iter-EMPI_GLOBAL_niter, iter, treal_consum-otime, (((double)((double)sum_tflops)/(double)sdflops) * 1.0E-6), sum_ctime / *size, sum_tflops, otime);
        perc_consum = ((treal_consum * EMPI_GLOBAL_profile_weigth[(iter/EMPI_GLOBAL_niter)-1]) / resttime);

        if (treal_consum > resttime) {

            coef = (sum_coef - (EMPI_GLOBAL_profile_weigth[(iter/EMPI_GLOBAL_niter)-1] * (1 - EMPI_GLOBAL_percentage))) / (sum_coef - (perc_consum * (1 - EMPI_GLOBAL_percentage)));

        } else if (treal_consum < resttime) {

            coef = (sum_coef - (perc_consum * (1 - EMPI_GLOBAL_percentage))) / (sum_coef - (EMPI_GLOBAL_profile_weigth[(iter/EMPI_GLOBAL_niter)-1] * (1 - EMPI_GLOBAL_percentage)));
        }

        EMPI_GLOBAL_percentage = EMPI_GLOBAL_percentage * coef;

        //Factores de correccion
        flop_factor = (double)((double) sum_tflops / (double)EMPI_GLOBAL_profile_flops[(iter/EMPI_GLOBAL_niter)-1]);

        comp_factor = (double)EMPI_GLOBAL_profile_mflops[(iter/EMPI_GLOBAL_niter)-1] / iflops;

        comp_factor *= (double)((double)EMPI_GLOBAL_profile_mflops[iter/EMPI_GLOBAL_niter] / (double)EMPI_GLOBAL_profile_mflops[(iter/EMPI_GLOBAL_niter)-1]);

        //Tiempo de ejecucion del siguiente intervalo con la configuracion inicial
        tcomp_orig = (double)((double)(EMPI_GLOBAL_profile_flops[iter/EMPI_GLOBAL_niter]*1.0E-6*flop_factor)/(double)EMPI_GLOBAL_profile_mflops[iter/EMPI_GLOBAL_niter]);
        tcomm_orig = EMPI_GLOBAL_profile_comms[iter/EMPI_GLOBAL_niter];

        //Tiempo objetivo del siguiente intervalo
        resttime = (tcomp_orig + tcomm_orig) * (1 - EMPI_GLOBAL_percentage);

        //Factor de comunicaciones
        if (EMPI_GLOBAL_comm_prev > 0) {

            comm_factor = (double)((sum_ctime / *size) / EMPI_GLOBAL_comm_prev);

        } else comm_factor = 1;

        EMPI_Comm_cost (&comm_cost, EMPI_GLOBAL_minprocs, EMPI_GLOBAL_minprocs, EMPI_GLOBAL_niter);

        comm_factor *= (double)(EMPI_GLOBAL_profile_comms[(iter/EMPI_GLOBAL_niter)] / comm_cost);

        //Estimated costs for the execution on the current configuration for one sampling interval
        //Computation estimated cost
        comp_cost = (EMPI_GLOBAL_profile_flops[iter/EMPI_GLOBAL_niter]*1.0E-6*flop_factor) / (sdflops * comp_factor);
        //Communication estimated cost
        EMPI_Comm_cost (&comm_cost, EMPI_GLOBAL_minprocs, *size, EMPI_GLOBAL_niter);

        comm_cost *= comm_factor;

        EMPI_GLOBAL_comm_prev = comm_cost;

        //duracion estimada del siguiente sampling interval (computo+comunicaciones)
        stime = (comp_cost + comm_cost);

        class = EMPI_GLOBAL_system_classes;

        for (n = 0, maxmflops_sum = 0, maxmflops_rem = 0, actual_cost = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

            maxmflops_sum += ((class->maxprocs-class->nprocs) * class->mflops);

            maxmflops_rem += (class->nprocs * class->mflops);

            actual_cost += (class->nprocs + class->iniprocs) * class->cost;

            class = class->next;
        }

        //tiempo consumido
        EMPI_GLOBAL_cum_time = EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm + EMPI_GLOBAL_tover;
        fprintf (stderr, "EMPI_Monitor_cost_irregular(%d): %i-%i - stime %lf (comp %lf comm %lf) sflops %.0lf - resttime %lf percentage %lf - cum_time %lf\n", getpid(), iter, iter+EMPI_GLOBAL_niter, stime, comp_cost, comm_cost, EMPI_GLOBAL_profile_flops[iter/EMPI_GLOBAL_niter] * flop_factor, resttime, EMPI_GLOBAL_percentage, EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm + EMPI_GLOBAL_tover);
        //Adding procs - stime (estimated time)
        //if (stime > resttime) {
        if ((stime > resttime)&&(stime > (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)))) {

            //memory allocation
            class_procs = (double*) calloc (EMPI_GLOBAL_nhclasses, sizeof(double));
            assert (class_procs);

            if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_LP) {

                //Primera aproximacion PL

                ratio = (EMPI_GLOBAL_tcomm / (EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm));
                //ratio_si = (double)(smonitor[EMPI_root].ctime / ((smonitor[EMPI_root].rtime*1.0E-6) + smonitor[EMPI_root].ctime));
                ratio_si = (double)(sum_ctime / (sum_rtime + sum_ctime));

                if (ratio_si > (fabs(resttime-stime)/stime)) {

                    //Primera aproximacion PL
                    //reqflops = (int)((flop / (resttime * (1 - ratio))) - sflops);
                    //reqflops = (int)(((stime / resttime) * sflops) - sflops);

                    rprocs = newsize = 0;

                    treqflops = reqflops = 0;

                } else {

                    //Primera aproximacion PL
                    //reqflops = (int)((flop / (resttime * (1 - ratio))) - sflops);
                    reqflops = (int)(((stime / (resttime * (1 - ratio))) * sflops) - sflops);

                    if (reqflops > maxmflops_sum) reqflops = maxmflops_sum;

                    EMPI_lp_min_cost (reqflops, &newsize, &mflops, &pcost, class_procs);

                    rprocs = newsize;

                    treqflops = mflops;
                }

            } else if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_EX) {

                //Busqueda exhaustiva
                for (p = 1, rprocs = 0, mincost = -1; p <= (EMPI_GLOBAL_maxprocs - *size); p ++) {

                    //Cost of communications
                    EMPI_Comm_cost (&comm_cost, *size, p+*size, EMPI_GLOBAL_niter);
                    comm_cost *= comm_factor;

                    EMPI_Rdata_spawn_cost (*size, p, &rdata_cost);

                    //Cost of spawning 'newsize' procs
                    spawn_cost = EMPI_GLOBAL_spawn_cost * p;

                    //tiempo de computo que se deberia alcanzar (pstime) por iteracion para conseguir el tiempo restante (resttime), considerando todos los costes (spawn, comm) y threshold
                    //pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;
                    //pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - spawn_cost - rdata_cost - comm_cost;
                    //pstime = resttime - ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;
                    pstime = resttime - ((spawn_cost + rdata_cost)) - comm_cost;

                    //para tardar pstime de computo necesito reqflops
                    if (pstime > 0) {

                        //reqflops = ceil((sdflops * comp_cost) / pstime);

                        reqflops = ceil((iflops * tcomp_orig) / pstime);

                        reqflops -= sdflops;

                        //reqflops -= (sdflops / comp_factor);

                    } else reqflops = maxmflops_sum;

                    memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                    //NOTE: Esto esta bien porque para eficiencia no hay coste, coste = 0;
                    EMPI_lp_min_cost_fixed (reqflops, p, &mflops, &newsize, &pcost, class_procs);

                    //si no se encuentran p procs con reqflops, solicitamos max mflops de p procs con coste indiferente
                    if (newsize != p) EMPI_lp_flops_max (p, &mflops, &pcost, class_procs);

                    ptime = (double)((double)(EMPI_GLOBAL_profile_flops[iter/EMPI_GLOBAL_niter]*1.0E-6*flop_factor)/(double)((sdflops+mflops) * comp_factor));

                    //diff = fabs(resttime - (ptime + comm_cost + ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter))));
                    diff = fabs(resttime - (ptime + comm_cost + spawn_cost + rdata_cost));

                    pscost = (pcost + actual_cost) * (ptime + comm_cost + spawn_cost + rdata_cost);

                    //Se selecciona la configuracion con el menor coste dentro de un threshold
                    if (((mincost == -1)||(pscost < mincost))&&(diff < (resttime * EMPI_GLOBAL_obj_texec_threshold))) {
                        fprintf (stderr, "EMPI_Monitor_cost_irregular(%d): Estimacion computo %lf estimacion communicaciones %lf - diff %lf maxdiff %lf\n", getpid(), ptime, comm_cost, diff, resttime * EMPI_GLOBAL_obj_texec_threshold);
                        EMPI_GLOBAL_comm_prev = comm_cost;

                        rprocs = p;

                        treqflops = mflops;

                        mincost = pscost;
                    }
                }
            }

            if (rprocs > 0) {

                int *nextprocs = NULL;

                newsize = rprocs;

                memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                EMPI_lp_get_procs_flops_spawn (treqflops, rprocs, class_procs);

                hostid = (int*) malloc (newsize * sizeof(int));
                assert (hostid);

                nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                assert (nextprocs);

                for (n = 0; n < newsize; n ++) hostid[n] = -1;

                class = EMPI_GLOBAL_system_classes;

                //increase the number of processes
                for (n = 0, idx = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                    class->nprocs = class->nprocs + class_procs[n];

                    for (m = 0; m < class_procs[n]; m ++) {

                        hostlist = EMPI_GLOBAL_hostlist;

                        for (i = 0; i < EMPI_GLOBAL_nhosts; i ++) {

                            if ((hostlist->idclass == class->idclass)&&(hostlist->maxprocs > (hostlist->nprocs+nextprocs[hostlist->id]))&&(hostid[idx] < 0)) {

                                hostid[idx] = i;

                                idx ++;

                                nextprocs[hostlist->id] ++;

                                i = EMPI_GLOBAL_nhosts;
                            }

                            hostlist = hostlist->next;
                        }
                    }

                    class = class->next;
                }

                free (nextprocs);
                nextprocs = NULL;

            } else {

                newsize = 0;
                mflops = 0;
            }

            //set newsize and mflops
            reconfig[0] = newsize + *size;
            reconfig[1] = mflops + sflops;

            free (class_procs);
            class_procs = NULL;

        //Removing procs - stime (estimated time)
        //} else if (stime < resttime) {
        } else if ((stime < resttime)&&(resttime > (stime * (1 + EMPI_GLOBAL_obj_texec_threshold)))) {

            //memory allocation
            class_procs = (double*) calloc (EMPI_GLOBAL_nhclasses, sizeof(double));
            assert (class_procs);

            if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_LP) {

                //Primera aproximacion PL

                ratio = (EMPI_GLOBAL_tcomm / (EMPI_GLOBAL_tcomp + EMPI_GLOBAL_tcomm));
                //ratio_si = (double)(smonitor[EMPI_root].ctime / ((smonitor[EMPI_root].rtime*1.0E-6) + smonitor[EMPI_root].ctime));
                ratio_si = (double)(sum_ctime / (sum_rtime + sum_ctime));

                if (ratio_si > (fabs(resttime-stime)/resttime)) {

                    //Primera aproximacion PL
                    //reqflops = (int)(sflops - (flop / (resttime * (1 - ratEMPI_MAX_HOST_CLASSESio))));
                    //reqflops = (int)(sflops - ((stime / (resttime * (1 - ratio))) * sflops));

                    rprocs = newsize = 0;

                    treqflops = reqflops = 0;

                } else {

                    //Primera aproximacion PL
                    //reqflops = (int)(sflops - (flop / (resttime * (1 - ratio))));
                    reqflops = (int)(sflops - ((stime / (resttime * (1 - ratio))) * sflops));

                    if (reqflops > maxmflops_rem) reqflops = maxmflops_rem;

                    EMPI_lp_max_cost (reqflops, &newsize, &mflops, &pcost, class_procs);

                    rprocs = newsize;

                    treqflops = mflops;
                }

            } else if (EMPI_GLOBAL_Adaptability_policy == EMPI_ADAPTABILITY_EX) {

                //Busqueda exhaustiva

                for (p = 1, rprocs = 0, mincost = -1; p <= (*size - EMPI_GLOBAL_minprocs); p ++) {

                    //Cost of communications
                    EMPI_Comm_cost (&comm_cost, *size, *size-p, EMPI_GLOBAL_niter);
                    comm_cost *= comm_factor;

                    EMPI_Rdata_remove_cost (*size, p, &rdata_cost);

                    //Cost of removing 'newsize' procs
                    remove_cost = EMPI_GLOBAL_remove_cost * p;

                    //tiempo de computo que se deberia alcanzar (pstime) por iteracion para conseguir el tiempo restante (resttime), considerando todos los costes (remove, comm) y threshold
                    //pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;
                    //pstime = (resttime * (1 + EMPI_GLOBAL_obj_texec_threshold)) - remove_cost - rdata_cost - comm_cost;
                    //pstime = resttime - ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) - comm_cost;
                    pstime = resttime - (remove_cost + rdata_cost) - comm_cost;

                    //para tardar pstime necesito disminuir reqflops
                    //reqflops = sflops - ((sflops * comp_cost) / pstime);
                    reqflops = sflops - ((iflops * tcomp_orig) / pstime);
                    //reqflops = sflops - (((iflops * tcomp_orig) / pstime) * comp_factor);

                    memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                    //NOTE: Esto esta bien porque para eficiencia no hay coste, coste = 0;
                    EMPI_lp_max_cost_fixed (reqflops, p, &mflops, &newsize, &pcost, class_procs);

                    //si no se encuentran p procs con reqflops, solicitamos min mflops de p procs con coste indiferente
                    if (newsize != p) EMPI_lp_flops_min (p, &mflops, &pcost, class_procs);

                    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                        rclass = class_procs[n];

                        for (m = *size-1; m >= EMPI_GLOBAL_minprocs && rclass > 0; m --) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            while ((hostlist != NULL)&&(hostlist->id < smonitor[m].hostid)) hostlist = hostlist->next;

                            if (hostlist->idclass == n) {

                                realflops += (int)(smonitor[m].flops/smonitor[m].rtime);

                                rclass --;
                            }
                        }
                    }

                    ptime = (double)((double)(EMPI_GLOBAL_profile_flops[iter/EMPI_GLOBAL_niter]*1.0E-6*flop_factor) / (double)((sdflops-realflops) * comp_factor));

                    //diff = fabs(resttime - (ptime + comm_cost + ((remove_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter))));
                    diff = fabs(resttime - (ptime + comm_cost + remove_cost + rdata_cost));

                    pscost = (actual_cost - pcost) * (ptime + comm_cost + remove_cost + rdata_cost);

                    //Se selecciona la configuracion que quita mayor coste y que su desviacion no supere el threshold
                    if (((mincost == -1)||(pscost < mincost))&&(diff < (resttime * EMPI_GLOBAL_obj_texec_threshold))) {
                        fprintf (stderr, "EMPI_Monitor_cost_irregular(%d): Estimacion computo %lf estimacion communicaciones %lf - diff %lf maxdiff %lf\n", getpid(), ptime, comm_cost, diff, resttime * EMPI_GLOBAL_obj_texec_threshold);
                        EMPI_GLOBAL_comm_prev = comm_cost;

                        rprocs = p;

                        treqflops = mflops;

                        mincost = pscost;
                    }
                }
            }

            if (rprocs > 0) {

                int *nextprocs = NULL;

                newsize = rprocs;

                memset (class_procs, 0, sizeof(double)*EMPI_GLOBAL_nhclasses);

                EMPI_lp_get_procs_flops_remove (treqflops, rprocs, class_procs);

                rremvs = (int*) malloc (newsize * sizeof(int));
                assert (rremvs);

                nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                assert (nextprocs);

                for (n = 0; n < newsize; n ++) rremvs[n] = -1;

                class = EMPI_GLOBAL_system_classes;

                //decrease the number of processes
                for (n = 0, idx = 0, idx_cum = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                    class->nprocs = class->nprocs - class_procs[n];

                    idx_cum += class_procs[n];

                    //decrease the number of processes
                    for (m = 0; m < class_procs[n]; m ++) {

                        for (i = (*size-1); i >= 0; i --) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            while ((hostlist != NULL)&&(hostlist->id < smonitor[i].hostid)) hostlist = hostlist->next;

                            if ((hostlist != NULL)&&(class->idclass == hostlist->idclass)&&((hostlist->nprocs-nextprocs[hostlist->id]) > hostlist->iniprocs)&&(idx < idx_cum)) {

                                rremvs[idx] = i;

                                nextprocs[hostlist->id] ++;

                                idx ++;
                            }
                        }
                    }

                    class = class->next;
                }

                free (nextprocs);
                nextprocs = NULL;

                //sort rremvs
                for(n = 0; n < newsize-1; n++){

                    for(m = n+1; m < newsize; m++){

                        if(rremvs[m] > rremvs[n]){

                            temprank = rremvs[m];
                            rremvs[m] = rremvs[n];
                            rremvs[n] = temprank;
                        }
                    }
                }

            } else {

                newsize = 0;
                mflops = 0;
            }

            //set newsize and mflops
            reconfig[0] = *size - newsize;
            reconfig[1] = sflops - mflops;

            free (class_procs);
            class_procs = NULL;
        }
    }

    if (sdflops > 0) {

        //Bcast newsize and mflops
        PMPI_Bcast (reconfig, 2, MPI_INT, EMPI_root, EMPI_COMM_WORLD);

        EMPI_GLOBAL_overhead_rpolicy += (MPI_Wtime() - tini);

        //get newsize and mflops
        newsize = reconfig[0];
        mflops = reconfig[1];

        // launch or remove the selected processes
        EMPI_Exec_Malleability  (rank, size, count, disp, smonitor, fc, argv, bin, newsize, mflops, &hostid, &rremvs, 1);
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Monitor_cost_irregular in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_malleability'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_malleability (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Monitor_malleability in <%s> ***\n", __FILE__);
    #endif

    int i = 0, m = 0, n = 0, idx_cum = 0, temprank = 0, totprocs = 0, newsize = 0, mflops = 0, sflops = 0, idx, reconfig[2] = {0, 0}, *hostid = NULL, *rremvs = NULL;

    double tini = 0;

    EMPI_Class_type *class = NULL;

    EMPI_host_type *hostlist = NULL;

    if (EMPI_GLOBAL_listrm[EMPI_GLOBAL_nextrm] == iter) {

        tini = MPI_Wtime();

        if (*rank == EMPI_root) {

            //ver si es de suma o resta de procesos
            for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) totprocs += EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n];

            if (totprocs > 0) {

                //adding processes

                for (n = 0, sflops = 0; n < *size; n ++) sflops += (int)(smonitor[n].flops/smonitor[n].rtime);

                int *nextprocs = NULL;

                newsize = totprocs;

                hostid = (int*) malloc (newsize * sizeof(int));
                assert (hostid);

                nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                assert (nextprocs);

                for (n = 0; n < newsize; n ++) hostid[n] = -1;

                class = EMPI_GLOBAL_system_classes;

                //increase the number of processes
                for (n = 0, idx = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                    class->nprocs = class->nprocs + EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n];

                    mflops += EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n] * class->mflops;

                    for (m = 0; m < EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n]; m ++) {

                        hostlist = EMPI_GLOBAL_hostlist;

                        for (i = 0; i < EMPI_GLOBAL_nhosts; i ++) {

                            if ((hostlist->idclass == class->idclass)&&(hostlist->maxprocs > (hostlist->nprocs+nextprocs[hostlist->id]))&&(hostid[idx] < 0)) {

                                hostid[idx] = i;

                                idx ++;

                                nextprocs[hostlist->id] ++;

                                i = EMPI_GLOBAL_nhosts;
                            }

                            hostlist = hostlist->next;
                        }
                    }

                    class = class->next;
                }

                free (nextprocs);
                nextprocs = NULL;

                //set newsize and mflops
                reconfig[0] = newsize + *size;
                reconfig[1] = mflops + sflops;

            } else if (totprocs < 0) {

                //removing processes

                int *nextprocs = NULL;

                for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n] *= -1;

                newsize = (totprocs*-1);

                rremvs = (int*) malloc (newsize * sizeof(int));
                assert (rremvs);

                nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                assert (nextprocs);

                for (n = 0; n < newsize; n ++) rremvs[n] = -1;

                class = EMPI_GLOBAL_system_classes;

                //decrease the number of processes
                for (n = 0, idx = 0, idx_cum = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                    class->nprocs = class->nprocs - EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n];

                    mflops += EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n] * class->mflops;

                    idx_cum += EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n];

                    //decrease the number of processes
                    for (m = 0; m < EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n]; m ++) {

                        for (i = (*size-1); i >= 0; i --) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            while ((hostlist != NULL)&&(hostlist->id < smonitor[i].hostid)) hostlist = hostlist->next;

                            if ((hostlist != NULL)&&(class->idclass == hostlist->idclass)&&((hostlist->nprocs-nextprocs[hostlist->id]) > hostlist->iniprocs)&&(idx < idx_cum)) {

                                rremvs[idx] = i;

                                nextprocs[hostlist->id] ++;

                                idx ++;
                            }
                        }
                    }

                    class = class->next;
                }

                free (nextprocs);
                nextprocs = NULL;

                //sort rremvs
                for (n = 0; n < newsize-1; n++){

                    for (m = n+1; m < newsize; m++){

                        if(rremvs[m] > rremvs[n]){

                            temprank = rremvs[m];
                            rremvs[m] = rremvs[n];
                            rremvs[n] = temprank;
                        }
                    }
                }

                //set newsize and mflops
                reconfig[0] = *size - newsize;
                reconfig[1] = sflops - mflops;
            }
        }

        //Bcast newsize and mflops
        PMPI_Bcast (reconfig, 2, MPI_INT, EMPI_root, EMPI_COMM_WORLD);

        EMPI_GLOBAL_overhead_rpolicy += (MPI_Wtime() - tini);

        //get newsize and mflops
        newsize = reconfig[0];
        mflops = reconfig[1];

        // launch or remove the selected processes
        EMPI_Exec_Malleability  (rank, size, count, disp, smonitor, fc, argv, bin, newsize, mflops, &hostid, &rremvs, 0);

        EMPI_GLOBAL_nextrm ++;

    } else {

        int lbalance = EMPI_NULL;

        if (EMPI_GLOBAL_lbalance == EMPI_TRUE) lbalance = EMPI_LBalance (rank, size, count, disp, fc, smonitor);

    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Monitor_malleability in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_malleability_conditional'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_malleability_conditional (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Monitor_malleability_conditional in <%s> ***\n", __FILE__);
    #endif

    int i = 0, m = 0, n = 0, idx_cum = 0, temprank = 0, totprocs = 0, mflops = 0, newsize = 0, sflops = 0, idx, reconfig[2] = {0, 0}, *hostid = NULL, *rremvs = NULL;

    double tini = 0;

    EMPI_Class_type *class = NULL;

    EMPI_host_type *hostlist = NULL;

    if (EMPI_GLOBAL_listrm[EMPI_GLOBAL_nextrm] == iter) {

        tini = MPI_Wtime();

        if (*rank == EMPI_root) {

            double stime = 0.0, min_xtime = 0.0, xtime = 0.0, spawn_cost = 0, rdata_cost = 0;

            double comp_cost = 0.0, comp_cost_time = 0.0, comm_cost = 0.0, comm_mean, comm_factor = 1;

            long long flop;

            int rprocs = 0, p, mflops = 0, *iprocs = NULL, *imflops = NULL, omflops = 0, *class_procs = NULL;

            //ver si es de suma o resta de procesos
            for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) totprocs += EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n];

            if (totprocs > 0) {

                //Adding processes

                iprocs = (int*) calloc (totprocs, sizeof(int));
                assert (iprocs);

                imflops = (int*) calloc (totprocs, sizeof (int));
                assert (totprocs);

                class = EMPI_GLOBAL_system_classes;

                for (n = 0, idx = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                    for (p = 0; p < EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n]; p ++) {

                        iprocs[idx] = n;
                        imflops[idx] = class->mflops;

                        idx ++;
                    }

                    class = class->next;
                }

                //ordenar arrays segun mflops
                EMPI_Sort (imflops, iprocs, 0, totprocs-1);

                for (n = 0, sflops = 0; n < *size; n ++) {

                    sflops += (int)(smonitor[n].flops/smonitor[n].rtime);

                    comm_mean += smonitor[n].ctime;
                }

                comm_mean = (comm_mean / *size);

                //Estimated costs for the execution on the current configuration
                //Computation estimated cost
                EMPI_Comp_cost (&comp_cost, &comp_cost_time, &flop, sflops, *size);

                //Communication estimated cost
                EMPI_Comm_cost (&comm_cost, *size, *size, EMPI_GLOBAL_niter);

                if (comm_cost > 0) {
                    comm_factor = (comm_mean / comm_cost);
                    comm_cost *= comm_factor;
                }

                stime = (comp_cost + comm_cost);

                //minimum execution time
                min_xtime = stime;

                for (p = 1, mflops = 0; p < totprocs+1; p ++, mflops = 0) {

                    //mflops de los primeros n procesos mas potentes
                    for (n = 0; n < p; n ++) mflops += imflops[n];

                    EMPI_Rdata_spawn_cost (*size, p, &rdata_cost);

                    spawn_cost = EMPI_GLOBAL_spawn_cost * p;

                    //Cost of communications
                    EMPI_Comm_cost (&comm_cost, *size, p+*size, EMPI_GLOBAL_niter);
                    comm_cost *= comm_factor;

                    EMPI_Comp_cost (&comp_cost, &comp_cost_time, &flop, sflops+mflops, p+*size);

                    xtime = (comp_cost + comm_cost + ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) );

                    if (xtime < min_xtime) {

                        rprocs = p;

                        omflops = mflops;

                        min_xtime = xtime;
                    }
                }

                free (imflops);
                imflops = NULL;

                newsize = mflops = 0;

                if (rprocs > 0) {

                    class_procs = (int*) calloc (EMPI_GLOBAL_nhclasses, sizeof(int));
                    assert (class_procs);

                    for (n = 0; n < rprocs; n ++) class_procs[iprocs[n]] ++;

                    int *nextprocs = NULL;

                    newsize = rprocs;

                    hostid = (int*) malloc (newsize * sizeof(int));
                    assert (hostid);

                    nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                    assert (nextprocs);

                    for (n = 0; n < newsize; n ++) hostid[n] = -1;

                    mflops = omflops;

                    class = EMPI_GLOBAL_system_classes;

                    //increase the number of processes
                    for (n = 0, idx = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                        class->nprocs = class->nprocs + class_procs[n];

                        for (m = 0; m < class_procs[n]; m ++) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            for (i = 0; i < EMPI_GLOBAL_nhosts; i ++) {

                                if ((hostlist->idclass == class->idclass)&&(hostlist->maxprocs > (hostlist->nprocs+nextprocs[hostlist->id]))&&(hostid[idx] < 0)) {

                                    hostid[idx] = i;

                                    idx ++;

                                    nextprocs[hostlist->id] ++;

                                    i = EMPI_GLOBAL_nhosts;
                                }

                                hostlist = hostlist->next;
                            }
                        }

                        class = class->next;
                    }

                    free (nextprocs);
                    nextprocs = NULL;

                    free (class_procs);
                    class_procs = NULL;
                }

                free (iprocs);
                iprocs = NULL;

                //set newsize and mflops
                reconfig[0] = newsize + *size;
                reconfig[1] = mflops + sflops;

            } else if (totprocs < 0) {

                //removing processes

                int *nextprocs = NULL;

                for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n] *= -1;

                newsize = (totprocs*-1);

                rremvs = (int*) malloc (newsize * sizeof(int));
                assert (rremvs);

                nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                assert (nextprocs);

                for (n = 0; n < newsize; n ++) rremvs[n] = -1;

                class = EMPI_GLOBAL_system_classes;

                //decrease the number of processes
                for (n = 0, idx = 0, idx_cum = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                    class->nprocs = class->nprocs - EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n];

                    mflops += EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n] * class->mflops;

                    idx_cum += EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n];

                    //decrease the number of processes
                    for (m = 0; m < EMPI_GLOBAL_nprocs_class[EMPI_GLOBAL_nextrm][n]; m ++) {

                        for (i = (*size-1); i >= 0; i --) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            while ((hostlist != NULL)&&(hostlist->id < smonitor[i].hostid)) hostlist = hostlist->next;

                            if ((hostlist != NULL)&&(class->idclass == hostlist->idclass)&&((hostlist->nprocs-nextprocs[hostlist->id]) > hostlist->iniprocs)&&(idx < idx_cum)) {

                                rremvs[idx] = i;

                                nextprocs[hostlist->id] ++;

                                idx ++;
                            }
                        }
                    }

                    class = class->next;
                }

                free (nextprocs);
                nextprocs = NULL;

                //sort rremvs
                for (n = 0; n < newsize-1; n++){

                    for (m = n+1; m < newsize; m++){

                        if(rremvs[m] > rremvs[n]){

                            temprank = rremvs[m];
                            rremvs[m] = rremvs[n];
                            rremvs[n] = temprank;
                        }
                    }
                }

                //set newsize and mflops
                reconfig[0] = *size - newsize;
                reconfig[1] = sflops - mflops;
            }
        }

        //Bcast newsize and mflops
        if(*rank == EMPI_root) fprintf(stderr, "EMPI_Monitor_malleability_conditional(%d): ===> %d %d", getpid(), reconfig[0], reconfig[1]);
        
        PMPI_Bcast (reconfig, 2, MPI_INT, EMPI_root, EMPI_COMM_WORLD);
        EMPI_GLOBAL_overhead_rpolicy += (MPI_Wtime() - tini);

        //get newsize and mflops
        newsize = reconfig[0];
        mflops = reconfig[1];

        // launch or remove the selected processes
        EMPI_Exec_Malleability  (rank, size, count, disp, smonitor, fc, argv, bin, newsize, mflops, &hostid, &rremvs, 0);

        EMPI_GLOBAL_nextrm ++;

    } else {

        int lbalance = EMPI_NULL;

        if (EMPI_GLOBAL_lbalance == EMPI_TRUE) lbalance = EMPI_LBalance (rank, size, count, disp, fc, smonitor);
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Monitor_malleability_conditional in <%s> ***\n", __FILE__);
    #endif
}



/****************************************************************************************************************************************
*
*   'EMPI_Monitor_malleability_triggered'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_malleability_triggered (int *rank, int *size, int iter, int maxiter, int count, int disp, EMPI_Monitor_type *smonitor, int *fc, char *argv[], char *bin) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Monitor_malleability_triggered in <%s> ***\n", __FILE__);
    #endif

    int i = 0, m = 0, n = 0, idx_cum = 0, temprank = 0, totprocs = 0, newsize = 0 , sflops = 0,mflops = 0, idx, reconfig[2] = {0, 0}, *hostid = NULL, *rremvs = NULL;
    double tini = 0;

    EMPI_Class_type *class = NULL;

    EMPI_host_type *hostlist = NULL;

    
    pthread_mutex_lock(&EMPI_GLOBAL_server_lock); // David: global lock

    // Always checks and broadcasts the new created/destroyed processes
    if(1) { 
        
        tini = MPI_Wtime();

        // if EMPI_GLOBAL_listrm[0] is 1, the action is performed inmediately, otherwise,  when the iteration is reached
        if (*rank == EMPI_root && (EMPI_GLOBAL_listrm[0] == 1 || iter >= EMPI_GLOBAL_listrm[0] )) {

            double stime = 0.0, min_xtime = 0.0, xtime = 0.0, spawn_cost = 0, rdata_cost = 0;

            double comp_cost = 0.0, comp_cost_time = 0.0, comm_cost = 0.0, comm_mean, comm_factor = 1;

            long long flop;

            int rprocs = 0, p, mflops = 0, *iprocs = NULL, *imflops = NULL, omflops = 0, *class_procs = NULL;

            //ver si es de suma o resta de procesos
            for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) totprocs += EMPI_GLOBAL_nprocs_class[0][n];

            fprintf(stderr, "EMPI_Monitor_malleability_triggered(%d):EMPI_GLOBAL_nhclasses: %d, totprocs: %d\n", getpid(), EMPI_GLOBAL_nhclasses, totprocs);

            if (totprocs > 0) {

                //Adding processes

                iprocs = (int*) calloc (totprocs, sizeof(int));
                assert (iprocs);

                imflops = (int*) calloc (totprocs, sizeof (int));
                assert (totprocs);

                class = EMPI_GLOBAL_system_classes;

                for (n = 0, idx = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                    for (p = 0; p < EMPI_GLOBAL_nprocs_class[0][n]; p ++) {

                        iprocs[idx] = n;
                        imflops[idx] = class->mflops;

                        idx ++;
                    }

                    class = class->next;
                }

                //ordenar arrays segun mflops
                EMPI_Sort (imflops, iprocs, 0, totprocs-1);

                for (n = 0, sflops = 0; n < *size; n ++) {

                    sflops += (int)(smonitor[n].flops/smonitor[n].rtime);

                    comm_mean += smonitor[n].ctime;
                }

                comm_mean = (comm_mean / *size);

                //Estimated costs for the execution on the current configuration
                //Computation estimated cost
                EMPI_Comp_cost (&comp_cost, &comp_cost_time, &flop, sflops, *size);

                //Communication estimated cost
                EMPI_Comm_cost (&comm_cost, *size, *size, EMPI_GLOBAL_niter);

                if (comm_cost > 0) {
                    comm_factor = (comm_mean / comm_cost);
                    comm_cost *= comm_factor;
                }

                stime = (comp_cost + comm_cost);

                //minimum execution time
                min_xtime = stime;
                
                p=totprocs;

                //mflops de los primeros n procesos mas potentes
                for (n = 0; n < p; n ++) mflops += imflops[n];

                EMPI_Rdata_spawn_cost (*size, p, &rdata_cost);

                spawn_cost = EMPI_GLOBAL_spawn_cost * p;

                //Cost of communications
                EMPI_Comm_cost (&comm_cost, *size, p+*size, EMPI_GLOBAL_niter);
                comm_cost *= comm_factor;

                EMPI_Comp_cost (&comp_cost, &comp_cost_time, &flop, sflops+mflops, p+*size);

                xtime = (comp_cost + comm_cost + ((spawn_cost + rdata_cost) / ((maxiter - iter)/EMPI_GLOBAL_niter)) );


                rprocs = p;

                omflops = mflops;

                min_xtime = xtime;
                    
                    
                

                free (imflops);
                imflops = NULL;
                
                

                newsize = mflops = 0;

                if (rprocs > 0) {

                    class_procs = (int*) calloc (EMPI_GLOBAL_nhclasses, sizeof(int));
                    assert (class_procs);

                    for (n = 0; n < rprocs; n ++) class_procs[iprocs[n]] ++;

                    int *nextprocs = NULL;

                    newsize = rprocs;

                    hostid = (int*) malloc (newsize * sizeof(int));
                    assert (hostid);

                    nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                    assert (nextprocs);

                    for (n = 0; n < newsize; n ++) hostid[n] = -1;

                    mflops = omflops;

                    class = EMPI_GLOBAL_system_classes;

                    //increase the number of processes
                    for (n = 0, idx = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                        class->nprocs = class->nprocs + class_procs[n];

                        for (m = 0; m < class_procs[n]; m ++) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            for (i = 0; i < EMPI_GLOBAL_nhosts; i ++) {
                                fprintf(stderr, "EMPI_Monitor_malleability_triggered(%d): INCREASE: hostlist->hostname: %s, class->idclass: %d, hostlist->idclass: %d, hostlist->id: %d, hostlist->nprocs: %d, nextprocs[%d]: %d, hostlist->maxprocs: %d, idx: %d, hostid[%d]: %d\n", getpid(), hostlist->hostname, class->idclass, hostlist->idclass, hostlist->id, hostlist->nprocs, hostlist->id, nextprocs[hostlist->id], hostlist->maxprocs, idx, idx, hostid[idx]);

                                if ((hostlist->idclass == class->idclass)&&(hostlist->maxprocs > (hostlist->nprocs+nextprocs[hostlist->id]))&&(hostid[idx] < 0)) {

                                    hostid[idx] = i;

                                    idx ++;

                                    nextprocs[hostlist->id] ++;

                                    i = EMPI_GLOBAL_nhosts;
                                }

                                hostlist = hostlist->next;
                            }
                        }

                        class = class->next;
                    }

                    free (nextprocs);
                    nextprocs = NULL;

                    free (class_procs);
                    class_procs = NULL;
                }

                free (iprocs);
                iprocs = NULL;

                //set newsize and mflops
                reconfig[0] = newsize + *size;
                reconfig[1] = mflops + sflops;
                
                fprintf(stderr, "EMPI_Monitor_malleability_triggered(%d): INCREASE: newsize: %d, *size: %d\n", getpid(), newsize, *size);


            } else if (totprocs < 0) {

                //removing processes

                int *nextprocs = NULL;

                for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) EMPI_GLOBAL_nprocs_class[0][n] *= -1;

                newsize = (totprocs*-1);

                rremvs = (int*) malloc (newsize * sizeof(int));
                assert (rremvs);

                nextprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof(int));
                assert (nextprocs);

                for (n = 0; n < newsize; n ++) rremvs[n] = -1;

                class = EMPI_GLOBAL_system_classes;

                //decrease the number of processes
                for (n = 0, idx = 0, idx_cum = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                    class->nprocs = class->nprocs - EMPI_GLOBAL_nprocs_class[0][n];

                    mflops += EMPI_GLOBAL_nprocs_class[0][n] * class->mflops;

                    idx_cum += EMPI_GLOBAL_nprocs_class[0][n];

                    //decrease the number of processes
                    for (m = 0; m < EMPI_GLOBAL_nprocs_class[0][n]; m ++) {

                        for (i = (*size-1); i >= 0; i --) {

                            hostlist = EMPI_GLOBAL_hostlist;

                            while ((hostlist != NULL)&&(hostlist->id < smonitor[i].hostid)) hostlist = hostlist->next;

                            fprintf(stderr, "EMPI_Monitor_malleability_triggered(%d): DECREASE: hostlist->hostname: %s, class->idclass: %d, hostlist->idclass: %d, hostlist->id: %d, hostlist->nprocs: %d, nextprocs[%d]: %d, hostlist->iniprocs: %d, idx: %d, idx_cum: %d\n", getpid(), hostlist->hostname, class->idclass, hostlist->idclass, hostlist->id, hostlist->nprocs, hostlist->id, nextprocs[hostlist->id], hostlist->iniprocs, idx, idx_cum);

                            if ((hostlist != NULL)&&(class->idclass == hostlist->idclass)&&((hostlist->nprocs-nextprocs[hostlist->id]) > hostlist->iniprocs)&&(idx < idx_cum)) {

                                rremvs[idx] = i;
                                fprintf(stderr, "EMPI_Monitor_malleability_triggered(%d):rremvs[%d] = %d\n", getpid(), idx, i);
                                nextprocs[hostlist->id] ++;

                                idx ++;
                            }
                        }
                    }

                    class = class->next;
                }

                free (nextprocs);
                nextprocs = NULL;

                //sort rremvs
                for (n = 0; n < newsize-1; n++){

                    for (m = n+1; m < newsize; m++){

                        if(rremvs[m] > rremvs[n]){

                            temprank = rremvs[m];
                            rremvs[m] = rremvs[n];
                            rremvs[n] = temprank;
                        }
                    }
                }

                //set newsize and mflops
                reconfig[0] = *size - newsize;
                reconfig[1] = sflops - mflops;
                fprintf(stderr, "EMPI_Monitor_malleability_triggered(%d): DECREASE: newsize: %d, *size: %d\n", getpid(), newsize, *size);
            }
        }
        else if (*rank == EMPI_root && EMPI_GLOBAL_listrm[0] == 0 ) {    
                reconfig[0] = 0;
                reconfig[1] = 0;        
        }
        //Bcast newsize and mflops        
        PMPI_Bcast (reconfig, 2, MPI_INT, EMPI_root, EMPI_COMM_WORLD);

        EMPI_GLOBAL_overhead_rpolicy += (MPI_Wtime() - tini);
        
        //get newsize and mflops
        newsize = reconfig[0];
        mflops = reconfig[1];
        
        // launch or remove the selected processes
        EMPI_Exec_Malleability  (rank, size, count, disp, smonitor, fc, argv, bin, newsize, mflops, &hostid, &rremvs, 0);

    } else {

        int lbalance = EMPI_NULL;

        if (EMPI_GLOBAL_lbalance == EMPI_TRUE) lbalance = EMPI_LBalance (rank, size, count, disp, fc, smonitor);
    }

    // David: Resets the trigger to not produce any action until it is set again
    if(EMPI_GLOBAL_listrm[0] == 1 ||  iter >= EMPI_GLOBAL_listrm[0] ){
         EMPI_GLOBAL_listrm[0] = 0;
    
         for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) EMPI_GLOBAL_nprocs_class[0][n]=0;    
    }
    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock); // David global unlock

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Monitor_malleability_triggered in <%s> ***\n", __FILE__);
    #endif
}



/****************************************************************************************************************************************
*
*   'EMPI_Comm_cost'
*
****************************************************************************************************************************************/
static void EMPI_Comm_cost (double *cost, int size, int newsize, int niter) {
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Comm_cost in <%s> ***\n", __FILE__);
    #endif

    //NOTA: parametro newsize es el nuevo numero de procesos.

    int btype = 0;

    double ratio_datasize, ratio_nmess;

    double d_size = (double) size, d_newsize = (double) newsize;

    EMPI_Comm_type *comms = NULL;

    comms = EMPI_GLOBAL_comms;

    ratio_datasize = d_size/d_newsize;
    ratio_nmess = d_newsize/d_size;
    double aux_cost = 0;
    *cost = 0;

    while (comms != NULL) {

        MPI_Type_size (comms->datatype, &btype);

        switch (comms->mpi_op) {

            //FIXME: estimar costes del resto de operaciones de comunicacion.

            case MPI_SEND:

                if (comms->datasize > 1) {
                    //MPI_Send communication cost
                    aux_cost = ((EMPI_GLOBAL_alpha + ((comms->datasize * ratio_datasize * btype * 8) * EMPI_GLOBAL_beta)) * ratio_nmess);
                    if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_SEND is %f\n", getpid(), aux_cost);
                    *cost += aux_cost;

                }


                else {

                    //MPI_Send communication cost
                    aux_cost = ((EMPI_GLOBAL_alpha + ((comms->datasize * btype * 8) * EMPI_GLOBAL_beta)) * ratio_nmess);
                    if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_SEND is %f\n", getpid(), aux_cost);
                    *cost += aux_cost;

                }


                break;

            case MPI_RECV:

                if (comms->datasize > 1){

                    //MPI_Send communication cost
                    aux_cost = ((EMPI_GLOBAL_alpha + ((comms->datasize * ratio_datasize * btype * 8) * EMPI_GLOBAL_beta)) * ratio_nmess);
                    if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_RECV is %f\n", getpid(), aux_cost);
                    *cost += aux_cost;

                }else{

                    //MPI_Send communication cost
                    aux_cost = ((EMPI_GLOBAL_alpha + ((comms->datasize * btype * 8) * EMPI_GLOBAL_beta)) * ratio_nmess);
                    if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_RECV is %f\n", getpid(), aux_cost);
                    *cost += aux_cost;
                }
                break;

            case MPI_REDUCE:
                aux_cost = log2(newsize) * EMPI_GLOBAL_alpha + (comms->datasize * btype * 8) * log2(newsize) * EMPI_GLOBAL_beta + (comms->datasize * btype * 8) * log2(newsize) * EMPI_GLOBAL_gamma;
                if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_REDUCE is %f\n", getpid(), aux_cost);
                *cost += aux_cost;
                //printf("Communication cost is not defined for operation MPI_REDUCE\n");
                break;

            case MPI_BCAST:
                aux_cost += log2(newsize) * EMPI_GLOBAL_alpha + (comms->datasize * btype * 8) * log2(newsize) * EMPI_GLOBAL_beta;
                *cost += aux_cost;
                if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_BCAST is %f\n", getpid(), aux_cost);

                //printf("Communication cost is not defined for operation MPI_BCAST\n");
                break;

            case MPI_GATHER:
                aux_cost = log2(newsize) * EMPI_GLOBAL_alpha + (comms->datasize * btype * 8) * ((newsize - 1)/newsize) * EMPI_GLOBAL_beta;
                if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_GATHER is %f\n", getpid(), aux_cost);
                *cost += aux_cost;
                //printf("Communication cost is not defined for operation MPI_GATHER\n");
                break;

            case MPI_SCATTER:
                aux_cost = log2(newsize) * EMPI_GLOBAL_alpha + (comms->datasize * btype * 8) * ((newsize - 1)/newsize) *  EMPI_GLOBAL_beta;
                if(EMPI_GLOBAL_debug_comms)fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_SCATTER is %f\n", getpid(), aux_cost);
                *cost += aux_cost;
                //printf("Communication cost is not defined for operation MPI_SCATTER\n");
                break;

            case MPI_ALLTOALL:
                aux_cost = log2(newsize) * EMPI_GLOBAL_alpha + (comms->datasize * btype * 8)/2 * log2(newsize) * EMPI_GLOBAL_beta;
                if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_ALLTOALL is %f\n", getpid(), aux_cost);
                *cost += aux_cost;
                    //printf("Communication cost is not defined for operation MPI_ALLTOALL\n");
                break;

            case MPI_REDUCE_SCATTER:
                if(fmod(log2(newsize), 1.0) == 0){
                    aux_cost = log2(newsize) * EMPI_GLOBAL_alpha + (comms->datasize * btype * 8) * ((newsize - 1)/newsize) * EMPI_GLOBAL_beta + (comms->datasize * btype * 8) * ((newsize - 1)/newsize) * EMPI_GLOBAL_gamma;
                    if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_REDUCE_SCATTER is %f\n", getpid(), aux_cost);
                    *cost += aux_cost;
                } else {
                    aux_cost = floor(log2(newsize) + 2) * EMPI_GLOBAL_alpha + (comms->datasize * btype * 8) * (1 + (newsize - 1 + (comms->datasize * btype * 8))/newsize) * EMPI_GLOBAL_beta + (comms->datasize * btype * 8) * (1 + (newsize - 1)/newsize)*EMPI_GLOBAL_gamma;
                    if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_REDUCE_SCATTER is %f\n", getpid(), aux_cost);
                    *cost += aux_cost;
                }

                //printf("Communication cost is not defined for operation MPI_REDUCE_SCATTER\n");
                break;

            case MPI_ALLREDUCE:
                /*
                    For the power-of-two case, the cost for the reduce-scatter is
                   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
                   allgather lgp.alpha + n.((p-1)/p).beta. Therefore, the
                   total cost is:
                   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

                   For the non-power-of-two case,
                   Cost = (2.floor(lgp)+2).alpha + (2.((p-1)/p) + 2).n.beta + n.(1+(p-1)/p).gamma


                   For short messages, for user-defined ops, and for count < pof2
                   we use a recursive doubling algorithm (similar to the one in
                   MPI_Allgather). We use this algorithm in the case of user-defined ops
                   because in this case derived datatypes are allowed, and the user
                   could pass basic datatypes on one process and derived on another as
                   long as the type maps are the same. Breaking up derived datatypes
                   to do the reduce-scatter is tricky.

                   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma

                */
                if(fmod(log2(newsize), 1.0) == 0){
                    aux_cost = 2 * log2(newsize) * EMPI_GLOBAL_alpha + 2 * (comms->datasize * btype * 8) * ((newsize - 1)/newsize) * EMPI_GLOBAL_beta
                             + (comms->datasize * btype * 8) * ((newsize - 1)/newsize) * EMPI_GLOBAL_gamma;
                    if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_ALLREDUCE is %f\n", getpid(), aux_cost);
                    *cost += aux_cost;
                } else {
                    aux_cost = 2 * floor(log2(newsize) + 2) * EMPI_GLOBAL_alpha + (2 * ((newsize - 1)/newsize) + 2) * (comms->datasize * btype * 8) * EMPI_GLOBAL_beta
                             + (comms->datasize * btype * 8) * (1 + (newsize - 1)/newsize) * EMPI_GLOBAL_gamma;
                    if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_ALLREDUCE is %f\n", getpid(), aux_cost);
                    *cost += aux_cost;
                }
                //MPI_Allreduce communication cost
                /*
                *cost   += (2 * log2(newsize) * EMPI_GLOBAL_alpha) + (2 * (comms->datasize * btype * 8) * ((newsize-1)/newsize) * EMPI_GLOBAL_beta)
                        + (2 * (comms->datasize * btype * 8) * ((newsize-1)/newsize) * EMPI_GLOBAL_gamma);
                */
                break;

            case MPI_ALLGATHER:
            case MPI_ALLGATHERV:

                /*
                For short messages and non-power-of-two no. of processes, we use
               the algorithm from the Jehoshua Bruck et al IEEE TPDS Nov 97
               paper. It is a variant of the disemmination algorithm for
               barrier. It takes ceiling(lg p) steps.

               Cost = lgp.alpha + n.((p-1)/p).beta
               where n is total size of data gathered on each process.

               For short or medium-size messages and power-of-two no. of
               processes, we use the recursive doubling algorithm.

               Cost = lgp.alpha + n.((p-1)/p).beta

               TODO: On TCP, we may want to use recursive doubling instead of the Bruck
               algorithm in all cases because of the pairwise-exchange property of
               recursive doubling (see Benson et al paper in Euro PVM/MPI
               2003).

               It is interesting to note that either of the above algorithms for
               MPI_Allgather has the same cost as the tree algorithm for MPI_Gather!

               For long messages or medium-size messages and non-power-of-two
               no. of processes, we use a ring algorithm. In the first step, each
               process i sends its contribution to process i+1 and receives
               the contribution from process i-1 (with wrap-around). From the
               second step onwards, each process i forwards to process i+1 the
               data it received from process i-1 in the previous step. This takes
               a total of p-1 steps.

               Cost = (p-1).alpha + n.((p-1)/p).beta

               We use this algorithm instead of recursive doubling for long
               messages because we find that this communication pattern (nearest
               neighbor) performs twice as fast as recursive doubling for long
               messages (on Myrinet and IBM SP).
                */

                //MPI_Allgather communication cost
                //gonzalo: *cost   += (2 * (newsize-1) * EMPI_GLOBAL_alpha) + (((newsize-1)/newsize) * 2 * (comms->datasize * btype * 8) * EMPI_GLOBAL_beta);

                if(fmod(log2(newsize), 1.0) == 0){
                    aux_cost = log2(newsize) * EMPI_GLOBAL_alpha + (((newsize-1)/newsize) * (comms->datasize * btype * 8) * EMPI_GLOBAL_beta);
                    if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_ALLGATHER is %f\n", getpid(), aux_cost);
                    *cost += aux_cost;
                } else {
                    aux_cost = (newsize-1) * EMPI_GLOBAL_alpha + (((newsize-1)/newsize) * (comms->datasize * btype * 8) * EMPI_GLOBAL_beta);
                    if(EMPI_GLOBAL_debug_comms) fprintf(stderr, "EMPI_Comm_cost(%d): Estimated communication cost for MPI_ALLGATHER is %f\n", getpid(), aux_cost);
                    *cost += aux_cost;
                }


                break;

            case MPI_BARRIER:

                //MPI_Barrier communication cost
                *cost   += EMPI_GLOBAL_alpha * (newsize-1);

                break;

            default:
                fprintf(stderr, "EMPI_Comm_cost(%d): Communication cost is not defined for the current operation %d\n", getpid(), comms->mpi_op);
                break;
        }

        comms = comms->next;
    }

    //set cost per sampling interval
    *cost *= niter;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Comm_cost in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Comp_cost'
*
****************************************************************************************************************************************/
static void EMPI_Comp_cost (double *cost_flops, double *cost_time, long long *flop, int mflops, int newsize) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Comp_cost in <%s> ***\n", __FILE__);
    #endif

    long long scost = 0;

    if ((EMPI_GLOBAL_track_flops[0] > 0)&&(EMPI_GLOBAL_track_rtime[0] > 0)) {

        //linear extrapolation
        *flop = (2 * (EMPI_GLOBAL_track_flops[1]-EMPI_GLOBAL_track_flops[0]) + EMPI_GLOBAL_track_flops[0]);
        scost = (2 * (EMPI_GLOBAL_track_rtime[1]-EMPI_GLOBAL_track_rtime[0]) + EMPI_GLOBAL_track_rtime[0]);

    } else {

        *flop = EMPI_GLOBAL_track_flops[1];
        scost = EMPI_GLOBAL_track_rtime[1];
    }

    //mflops: mflops of the new configuration
    //cost: cost of the new configuration
    if(mflops!=0)    *cost_flops = (double)((*flop / mflops) * 1.0E-6);
    else *cost_flops = 0;
    if(newsize!=0)   *cost_time = (double)((scost/newsize) * 1.0E-6);
    else *cost_time = 0;
    
    *flop = (*flop * 1.0E-6);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Comp_cost in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Capture_comms'
*
****************************************************************************************************************************************/
void EMPI_Capture_comms (int mpi_op, int *datasize, MPI_Datatype datatype) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Capture_comms in <%s> ***\n", __FILE__);
        fflush(NULL);
    #endif

    int rank;

    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);

    if (rank == EMPI_root) {

        int size, tdatasize = 0, n;

        EMPI_Comm_type *comms = NULL;

        comms = EMPI_GLOBAL_comms;

        MPI_Comm_size (EMPI_COMM_WORLD, &size);

        //Get comm data size
        switch (mpi_op) {

            case MPI_SEND:

                tdatasize = *datasize;

                break;

            case MPI_RECV:

                tdatasize = *datasize;

                break;

            case MPI_REDUCE:

                tdatasize = *datasize;

                break;

            case MPI_BCAST:

                tdatasize = *datasize;

                break;

            case MPI_GATHER:

                tdatasize = *datasize;

                break;

            case MPI_SCATTER:

                tdatasize = *datasize;

                break;

            case MPI_ALLGATHER:

                tdatasize = *datasize;

                break;

            case MPI_ALLTOALL:

                tdatasize = *datasize;

                break;

            case MPI_REDUCE_SCATTER:

                //FIXME: pendiente en wrapper

                break;

            case MPI_ALLREDUCE:

                tdatasize = *datasize;

                break;

            case MPI_ALLGATHERV:

                for (n = 0, tdatasize = 0; n < size; n ++) tdatasize += datasize[n];

                break;

            case MPI_BARRIER:

                tdatasize = 1;

                break;

            default:

                break;
        }

        if (comms == NULL) {

            //initialization
            comms = (EMPI_Comm_type*) malloc (sizeof(EMPI_Comm_type));
            assert (comms);

            //set pointer
            EMPI_GLOBAL_comms = comms;

        } else {

            while (comms->next != NULL) comms = comms->next;

            comms->next = (EMPI_Comm_type*) malloc (sizeof(EMPI_Comm_type));
            assert (comms->next);

            comms = comms->next;
        }

        //Set comms parameters
        comms->mpi_op = mpi_op;
        comms->nprocs = size;
        comms->datasize = tdatasize;
        comms->datatype = datatype;
        comms->next = NULL;
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Capture_comms in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_spawn'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_spawn (int *rank, int *size, int nprocs, int count, int disp, char *argv[], char *bin, EMPI_Monitor_type *smonitor, int *hostid, int* fc) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Monitor_spawn in <%s> ***\n", __FILE__);
    #endif

    int rcount, rdispl, *stflops = NULL, idx, n, *countdispl = NULL;

    double tini,tend_aux,dt_lb;

    EMPI_Class_type *class = NULL;

    EMPI_host_type *hostlist = NULL;

    //Spawn process

    stflops = (int*) calloc ((*size+nprocs), sizeof(int));
    assert (stflops);

    if (*rank == EMPI_root) {

        fprintf (stderr, "EMPI_Monitor_spawn(%d): Spawn %i at %f\n", getpid(), nprocs, MPI_Wtime()-EMPI_GLOBAL_iterative_ini);

        for (n = 0; n < *size; n ++) stflops[n] = (int)(smonitor[n].flops/smonitor[n].rtime);

        for (n = *size, idx = 0; n < (*size+nprocs); n ++, idx ++) {

            hostlist = EMPI_GLOBAL_hostlist;

            while (hostlist->id < hostid[idx]) hostlist = hostlist->next;

            class = EMPI_GLOBAL_system_classes;

            while (class->idclass != hostlist->idclass) class = class->next;

            //set mflops of the new spawned processes
            stflops[n] = class->mflops;
        }
    }

    //Bcast estimated flops
    PMPI_Bcast (stflops, (*size+nprocs), MPI_INT, EMPI_root, EMPI_COMM_WORLD);

    //memory allocation, vcounts and displs combined
    countdispl = (int*) calloc (((*size+nprocs)*2), sizeof (int));
    assert (countdispl);

    tini = MPI_Wtime();

    //Workload estimated balance
    EMPI_LBalance_spawn (*rank, *size, (*size+nprocs), &rcount, &rdispl, countdispl, countdispl+(*size+nprocs), smonitor, stflops, fc);

    EMPI_GLOBAL_overhead_lbalance += (MPI_Wtime() - tini);
    dt_lb=(MPI_Wtime() - tini);
    
    tini = MPI_Wtime();

    //TODO: este NULL es el info. Creo que hay que recibirlo en Monitor_end
    //spawn new processes in host (id)
    //check size of allocated region

    // Begin Clarisse Control point
    /*
    printf("  Native process [%d] Clarisse control point reached \n",*rank);
    extern MPI_File fh;
    int err;
    err = MPI_File_close(&fh);
    if (err != MPI_SUCCESS) printf(" Native processs [%d]: error closing file \n",*rank);
    cls_server_disconnect();
    */
    // End Clarisse Control point

    EMPI_Spawn (nprocs, argv, bin, hostid, NULL);

    tend_aux = MPI_Wtime();

    // printf("Spawn cost is %f seconds\n", tend_aux - tini);

     EMPI_GLOBAL_lastoverhead_processes=(tend_aux - tini);
    EMPI_GLOBAL_overhead_processes += EMPI_GLOBAL_lastoverhead_processes;
    
    //Bcast vcounts and displs
    // CHANGE: JAVI
    int sync_data = 1;
    fprintf(stderr, "EMPI_Monitor_spawn(%d): PMPI_Bcast sync_data\n",getpid());
    PMPI_Bcast (&sync_data, 1, MPI_INT, EMPI_root, EMPI_COMM_WORLD);
    // END CHANGE: JAVI
    PMPI_Bcast (countdispl, ((*size+nprocs)*2), MPI_INT, EMPI_root, EMPI_COMM_WORLD);

    //new rank
    MPI_Comm_rank (EMPI_COMM_WORLD, rank);

    //new size
    *size = *size + nprocs;

    tini = MPI_Wtime();

    //redistribute data
    EMPI_Rdata (count, disp, rcount, rdispl);
    EMPI_GLOBAL_lastoverhead_rdata=MPI_Wtime() - tini;
    EMPI_GLOBAL_overhead_rdata += EMPI_GLOBAL_lastoverhead_rdata;
    
    // Begin Clarisse Control point
    /*
    cls_set_client_intracomm(EMPI_COMM_WORLD);
    MPI_Barrier(EMPI_COMM_WORLD);
    cls_server_connect();
    //char file[100];
    //sprintf(file,"abc\_%d\n",*size); // ToDo:: remove
    //printf("\n Native:: %s",file);
    if(*size==2)    err = MPI_File_open(EMPI_COMM_WORLD, "abc_2", MPI_MODE_CREATE | MPI_MODE_RDWR ,    MPI_INFO_NULL, &fh); // It should happend 
    if(*size==3)    err = MPI_File_open(EMPI_COMM_WORLD, "abc_3", MPI_MODE_CREATE | MPI_MODE_RDWR ,    MPI_INFO_NULL, &fh); // It should happend 
    if(*size==4)    err = MPI_File_open(EMPI_COMM_WORLD, "abc_4", MPI_MODE_CREATE | MPI_MODE_RDWR ,    MPI_INFO_NULL, &fh); // It should happend 
    if (err != MPI_SUCCESS)    printf(" Native processs [%d]: error opening file \n",*rank);
    */
    // End Clarisse Control point
        


    if (*rank == EMPI_root) {
        fprintf (stderr, "EMPI_Monitor_spawn(%d): Spawn cost:: process_creation= %f \t data_redistribution= %f \t LoadBalance_computation= %f\n", getpid(),  EMPI_GLOBAL_lastoverhead_processes,EMPI_GLOBAL_lastoverhead_rdata,dt_lb);
    }        
    
    //set dynamic workload
    EMPI_GLOBAL_wpolicy = EMPI_DYNAMIC;

    if (EMPI_GLOBAL_vcounts != NULL) { free (EMPI_GLOBAL_vcounts); EMPI_GLOBAL_vcounts = NULL; }
    if (EMPI_GLOBAL_displs != NULL) { free (EMPI_GLOBAL_displs); EMPI_GLOBAL_displs = NULL; }

    EMPI_GLOBAL_vcounts = (int*) calloc (*size, sizeof (int));
    assert (EMPI_GLOBAL_vcounts);

    EMPI_GLOBAL_displs = (int*) calloc (*size, sizeof (int));
    assert (EMPI_GLOBAL_displs);

    //memcpy
    memcpy (EMPI_GLOBAL_vcounts, countdispl, *size*sizeof(int));
    memcpy (EMPI_GLOBAL_displs, countdispl+(*size), *size*sizeof(int));

    free (stflops);
    free (countdispl);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Monitor_spawn in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*   'EMPI_Monitor_remove'
*
****************************************************************************************************************************************/
static void EMPI_Monitor_remove (int *rank, int *size, int nprocs, int count, int disp, EMPI_Monitor_type *smonitor, int* rremvs, int* fc) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Monitor_remove in <%s> ***\n", __FILE__);
    #endif

    fprintf (stderr, "EMPI_Monitor_remove(%d): START\n", getpid());

    int rcount, rdispl, n, arank, status, *vcounts = NULL, *displs = NULL;

    double tini,dt_lb;

    EMPI_host_type *hostlist = NULL;

    if (*rank == EMPI_root) fprintf (stderr, "EMPI_Monitor_remove(%d): Remove %i at %f\n", getpid(), nprocs, MPI_Wtime()-EMPI_GLOBAL_iterative_ini);

    //Remove process

    //get new rank
    for (arank = *rank, n = 0; n < nprocs; n ++) {

        if (*rank == EMPI_root) {

            hostlist = EMPI_GLOBAL_hostlist;

            while (hostlist->id != smonitor[rremvs[n]].hostid) hostlist = hostlist->next;

            fprintf (stderr, "EMPI_Monitor_remove(%d): Process [%i] removed from %s\n", getpid(), rremvs[n], hostlist->hostname);
        }

        if (rremvs[n] < *rank) arank --;
    }

    //memory allocation
    vcounts = (int*) calloc ((*size-nprocs), sizeof (int));
    assert (vcounts);
    displs = (int*) calloc ((*size-nprocs), sizeof (int));
    assert (displs);

    tini = MPI_Wtime();

    //Workload estimated balance
    EMPI_LBalance_remove (*rank, *size, (*size-nprocs), &rcount, &rdispl, vcounts, displs, smonitor, rremvs, fc);

    EMPI_GLOBAL_overhead_lbalance += (MPI_Wtime() - tini);
    dt_lb=(MPI_Wtime() - tini);
    
    //new size
    *size = *size - nprocs;

    tini = MPI_Wtime();

    //redistribute data
    EMPI_Rdata (count, disp, rcount, rdispl);

    EMPI_GLOBAL_lastoverhead_rdata= MPI_Wtime() - tini;
    EMPI_GLOBAL_overhead_rdata += EMPI_GLOBAL_lastoverhead_rdata;
    
    // Begin Clarisse Control point
    /*
    printf("  Process [%d] Clarisse control point reached \n",*rank);
    extern MPI_File fh;
    int err;
    err = MPI_File_close(&fh);
    if (err != MPI_SUCCESS) printf(" Native processs [%d]: error closing file \n",*rank);
    cls_server_disconnect();        
    */
    // End Clarisse Control point
    
    
    tini = MPI_Wtime();


    
    //remove processes
    EMPI_Remove (nprocs, rremvs);

    EMPI_GLOBAL_lastoverhead_processes=(MPI_Wtime() - tini);
    EMPI_GLOBAL_overhead_processes += EMPI_GLOBAL_lastoverhead_processes;
    
    EMPI_Get_status (&status);

    if (status == EMPI_ACTIVE) {

        //not removed process
        MPI_Comm_rank (EMPI_COMM_WORLD, rank);

        //set dynamic workload
        EMPI_GLOBAL_wpolicy = EMPI_DYNAMIC;

        if (EMPI_GLOBAL_vcounts != NULL) {free (EMPI_GLOBAL_vcounts); EMPI_GLOBAL_vcounts = NULL;}
        if (EMPI_GLOBAL_displs != NULL) {free (EMPI_GLOBAL_displs); EMPI_GLOBAL_displs = NULL;}

        EMPI_GLOBAL_vcounts = (int*) calloc (*size, sizeof (int));
        assert (EMPI_GLOBAL_vcounts);

        EMPI_GLOBAL_displs = (int*) calloc (*size, sizeof (int));
        assert (EMPI_GLOBAL_displs);

        //memcpy
        memcpy (EMPI_GLOBAL_vcounts, vcounts, (*size)*sizeof(int));
        memcpy (EMPI_GLOBAL_displs, displs, (*size)*sizeof(int));
        
        // Begin Clarisse Control point
        /*
        cls_set_client_intracomm(EMPI_COMM_WORLD);
        MPI_Barrier(EMPI_COMM_WORLD);
        cls_server_connect();
        err = MPI_File_open(EMPI_COMM_WORLD, "abcd", MPI_MODE_CREATE | MPI_MODE_RDWR ,    MPI_INFO_NULL, &fh); // It should happend 
        if (err != MPI_SUCCESS)    printf(" Native processs [%d]: error opening file \n",*rank);
        */
        // End Clarisse Control point

        fprintf (stderr, "EMPI_Monitor_remove(%d): NOT Executing MPI_Finalize (%s)\n", getpid(), __FILE__);

    } else {

        // It is not necessary to disconnet from Clarisse the terminated process: it was already disconnected
        if (EMPI_GLOBAL_vcounts != NULL) { free (EMPI_GLOBAL_vcounts); EMPI_GLOBAL_vcounts = NULL; }
        if (EMPI_GLOBAL_displs != NULL) { free (EMPI_GLOBAL_displs); EMPI_GLOBAL_displs = NULL; }
                
        //removed process
        *rank = MPI_PROC_NULL;
        // removed MPI_Finalize from here
    }    

    free (vcounts);
    free (displs);

    if (*rank == EMPI_root) {
        fprintf (stderr, "EMPI_Monitor_remove(%d): Removal cost:: process_destruction= %f \t data_redistribution= %f \t LoadBalance_computation= %f\n", getpid(),  EMPI_GLOBAL_lastoverhead_processes,EMPI_GLOBAL_lastoverhead_rdata,dt_lb);
    }    
    
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Monitor_remove in <%s> ***\n", __FILE__);
    #endif
    fprintf (stderr, "EMPI_Monitor_remove(%d): END\n", getpid());

}


/****************************************************************************************************************************************
*
*   'EMPI_Sort'
*
****************************************************************************************************************************************/
static void EMPI_Sort (int* unsortedArray, int* index, int left, int right) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Sort in <%s> ***\n", __FILE__);
    #endif

    int i = 0, j = 0, element = 0, aux = 0;

    element = (unsortedArray)[(int) ((left + right)/2)];

    i = left;
    j = right;

    do
    {
        while ( (unsortedArray)[i] > element ) { i++; }
        while ( (unsortedArray)[j] < element ) { j--; }

        if (i <= j)
        {
            aux = (unsortedArray)[i];
            (unsortedArray)[i] = (unsortedArray)[j];
            (unsortedArray)[j] = aux;

            aux = (index)[i];
            (index)[i] = (index)[j];
            (index)[j] = aux;

            i++;
            j--;
        }

    } while (i <= j);

    if (left < j)   { EMPI_Sort (unsortedArray, index, left, j); }
    if (i < right){ EMPI_Sort (unsortedArray, index, i, right); }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Sort in <%s> ***\n", __FILE__);
    #endif
}
