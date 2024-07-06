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
 *    File:       init.c                                                                                                                    *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

/* include */
#include <empi.h>
#include <papi.h>

#define MAX_ARGS 40
#define MAX_ARG_LENGTH 80

int * EMPI_COMM_WORLD_F = 0;
//char argv[MAX_ARGS][MAX_ARG_LENGTH];
char ** argv = NULL;
int argc = 0;
pthread_t thread, rpc_thread; // CHANGE FINI
int rpc_thread_flag = 0; // CHANGE FINI

/*********** FORTRAN INTEGRATION ***************/
#ifdef __cplusplus
extern "C" {
#endif
    void fmpi_get_comm_(int *com);	
    void fmpi_barrier_();	
    void fmpi_get_type_(int *type);	
    void fmpi_config_redist_(int *rank, int *size, int *err);
    void fmpi_get_wsize_ini_(int *rank, int *size, int *dim, int *desp, int *count);
    void fmpi_init_(int *err);
    void fmpi_finalize_(int * err);
    void fmpi_set_comm_(int *com);
    void fmpi_get_status_(int *stat); //0 continue, 1 removed
    void fmpi_buildArgv_(char *arg);
    //MPI_Fint f_mpi_comm_c2f_(int *com);
    void f_mpi_comm_c2f_(MPI_Fint *com);
#ifdef __cplusplus
}
#endif


//Headers for the new functions
int FMPI_Init(int *argc, char*** argv);
int FMPI_Finalize();

//Global attributes
int *displs2 = NULL, *vcounts = NULL; //make global

//New functions

/**
 * Build the argv vector with the recived args from Fortran.
 */
void fmpi_buildArgv_(char * arg){
    if (argc == 0){
	/*printf("Initializing interoperability\n");*/
	argv = (char**)malloc(MAX_ARGS * sizeof(char*));
	for (int i = 0; i < MAX_ARGS; i++)
	    argv[i] = (char*)malloc(MAX_ARG_LENGTH * sizeof(char));
    }

    sprintf(argv[argc], "%s", arg);
    argc++;
}


/**
 * Returns EMPI_COMM_WORLD as a Fortran comm.
 */
void f_mpi_comm_c2f_(MPI_Fint *com){//MPI_Comm *comm) {
    *com = MPI_Comm_c2f(EMPI_COMM_WORLD);
    //return MPI_Comm_c2f(EMPI_COMM_WORLD);
    //return *com;
}


/**
 * Get the current status of the process
 */
void fmpi_get_status_(int *stat){
	int status = EMPI_GLOBAL_status;;
        if (status == EMPI_REMOVED)
		*stat = 1;
	else
		*stat = 0;
}


/**
 * Set EMPI_COMM_WORLD to Fortran comm.
 */
/*void fmpi_set_comm_(int *com){
	EMPI_COMM_WORLD = MPI_Comm_f2c(*com);
}*/

/**
 * FlexMPI init v1.0
 */
void fmpi_init_(int  *err){
	argv[argc] = NULL; // C uses strings null-terminated
    int res; 

	/*fprintf(stderr, "fmpi_init: argc=%d\n", argc);
	int i = 0;
        while(argv[i] != NULL){
		fprintf(stderr, "Argv[%d]: %s\n", i, argv[i]);	
		i++;
	}*/

    res = FLEXMPI_Init(&argc, &argv);
    //res = FLEXMPI_Init_ss(&argc, &argv);

	*err = 0;
}

/**
 * FlexMPI get communicator: EMPI_COMM_WORLD
 */
void fmpi_get_comm_(int *com){
	*com = EMPI_COMM_WORLD;
}

/**
 * FlexMPI barrier over EMPI_COMM_WORLD
 */
void fmpi_barrier_(){
	MPI_Barrier(EMPI_COMM_WORLD);
}

/**
 * FlexMPI Finalize
 */
void fmpi_finalize_(int *err){
    FLEXMPI_Finalize();
    //FLEXMPI_Finalize_ss();
}

/**
 * FlexMPI get type of process - Native vs Spawned
 */
void fmpi_get_type_(int *type){
	EMPI_Get_type(type);
}

/**
 * FlexMPI get wsize initial
 * Execute when the processes have been created
 */
void fmpi_get_wsize_ini_(int *rank, int *size, int *dim, int *desp, int *count){
	EMPI_Get_wsize(*rank, *size, *dim, desp, count, NULL, NULL);
}

/**
 * FlexMPI get wsize iterative
 * Execute when the processes have the data already in use
 */
void fmpi_get_wsize_(int *rank, int *size, int *dim, int *desp, int *count){
	EMPI_Get_wsize(*rank, *size, *dim, desp, count, vcounts, displs2);
}

/**
 * Dummy redistribution
 */
void fmpi_config_redist_(int *rank, int *size, int *err){
	//mod: info to redistribute dummy matrix
        int dim=100, despl, count;
        double *A = NULL;
        //int *displs2 = NULL, *vcounts = NULL; //make global
        EMPI_Get_wsize (*rank, *size, dim, &despl, &count, NULL, NULL);
        A = (double*) calloc (count * dim, sizeof (double));
        EMPI_Register_dense ("matrix", A, MPI_DOUBLE, dim, EMPI_DISJOINT);
        displs2 = (int*) malloc ((*size) * sizeof(int));
        vcounts = (int*) malloc ((*size) * sizeof(int));
        EMPI_Get_wsize (*rank, *size, dim, &despl, &count, vcounts, displs2);

}

/********************** END OF FORTRAN INTEGRATION ******************************/

/* headers */
  /****************************************************************************************************************************************
*
*    'EMPI_Parse_gfile'
*
****************************************************************************************************************************************/
static void EMPI_Parse_gfile (int argc, char **argv);

/****************************************************************************************************************************************
*
*    'EMPI_Parse_cfile'
*
****************************************************************************************************************************************/
// CHANGE: JAVI
//static void EMPI_Parse_cfile (int argc, char **argv);
static void EMPI_Parse_cfile (int argc, char **argv, char **init_hostlist);
// END CHANGE: JAVI
    
// CHANGE: JAVI
/****************************************************************************************************************************************
*
*    'EMPI_Expand_processes'
*
****************************************************************************************************************************************/
static void EMPI_Expand_processes (int argc, char **argv, char **hostlist);
// END CHANGE: JAVI

    
/****************************************************************************************************************************************
*
*    'EMPI_Parse_options'
*
****************************************************************************************************************************************/
static void EMPI_Parse_options (int argc, char **argv);

/****************************************************************************************************************************************
*
*    'EMPI_Parse_malleability'
*
****************************************************************************************************************************************/
static void EMPI_Parse_malleability (void);

/****************************************************************************************************************************************
*
*    'EMPI_Print_usage'
*
****************************************************************************************************************************************/
static void EMPI_Print_usage (void);

/****************************************************************************************************************************************
*
*    'EMPI_Destroy_data_structure'
*
****************************************************************************************************************************************/
static void EMPI_Destroy_data_structure (void);

/****************************************************************************************************************************************
*
*    'EMPI_Destroy_system_classes'
*
****************************************************************************************************************************************/
static void EMPI_Destroy_system_classes (void);

/****************************************************************************************************************************************
*
*    'EMPI_Create_system_classes'
*
****************************************************************************************************************************************/
static void EMPI_Create_system_classes (void);

/****************************************************************************************************************************************
*
*    'EMPI_Parse_efile'
*
****************************************************************************************************************************************/
static void EMPI_Parse_efile (int argc, char **argv);

/****************************************************************************************************************************************
*
*    'get_init_hostlist'
*
****************************************************************************************************************************************/
static int get_init_hostlist (char **init_hostlist);

/****************************************************************************************************************************************
*
*    'add_class
*
****************************************************************************************************************************************/
static int add_class (char *classname, int numprocs, int is_initial_proc, int add_maxprocs, int mflops, float cost);

/****************************************************************************************************************************************
*
*    'EMPI_fillup_init_hostlist'
*
****************************************************************************************************************************************/
// CHANGE: JAVI
static void EMPI_fillup_init_hostlist ();

/* implementation */

/****************************************************************************************************************************************
*
*    'get_init_hostlist'
*
****************************************************************************************************************************************/
int get_init_hostlist (char **init_hostlist) {
    
    //pointer to global variable
    EMPI_host_type *hostlist = EMPI_GLOBAL_hostlist;
    int rank,size;

    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);

    fprintf(stderr, "get_init_hostlist(%d): Begin\n",getpid());
    // allocate init_hostlist memory
    (*init_hostlist) = (char *)calloc(size*100,1);
    char *aux_hostlist = (char *)calloc(size*100,1);
    assert ((*init_hostlist));
    assert (aux_hostlist);
        
    //find hostname with processes in hostlist
    while (hostlist != NULL) {
        if (hostlist->iniprocs != 0) {
            if (strcmp((*init_hostlist),"") == 0) {
                sprintf(aux_hostlist, "%s:%d",hostlist->hostname, hostlist->iniprocs);
            } else {
                sprintf(aux_hostlist, "%s,%s:%d", (*init_hostlist), hostlist->hostname, hostlist->iniprocs);
            }
            //swap pointers
            char *aux = (*init_hostlist);
            (*init_hostlist) = aux_hostlist;
            aux_hostlist = aux;
        }
        hostlist = hostlist->next;
    }
    free(aux_hostlist);
    fprintf(stderr, "get_init_hostlist(%d): End\n",getpid());
    // END CHANGE: JAVI
    return 0;
}

/****************************************************************************************************************************************
*
*    'add_class
*
****************************************************************************************************************************************/
static int add_class (char *classname, int numprocs, int is_initial_proc, int add_maxprocs, int mflops, float cost) {
    EMPI_Class_type *class = EMPI_GLOBAL_system_classes;
    EMPI_Class_type *last_class = NULL;
    int is_found = EMPI_FALSE;
    int class_id = 0;
    
    fprintf (stderr, "add_class(%d): begin\n", getpid());

    assert(strlen(classname)<128);
    
    // look for it in global list
    while (class != NULL) {
        fprintf (stderr, "add_class(%d): after while\n", getpid());
        if (strcmp (class->name, classname) == 0) {
            fprintf (stderr, "add_class(%d): classname found\n", getpid());
            assert(is_found == EMPI_FALSE);
            class_id = class->idclass;
            assert(strcmp (class->name, EMPI_GLOBAL_hclasses[class_id])==0);

            class->nprocs = class->nprocs + numprocs;
            class->maxprocs = class->maxprocs + add_maxprocs;
            if (is_initial_proc) {
                class->iniprocs = class->iniprocs + numprocs;
            }
            class->icost = cost;
            if ((EMPI_GLOBAL_mpolicy == EMPI_COST)||(EMPI_GLOBAL_mpolicy == EMPI_COST_IRR)) {
                class->cost = cost;
            } else {
                class->cost = 0.0;
            }
            class->mflops = class->mflops + mflops;
            if (class->mflops < 0) class->mflops = -1; //runtime update
            is_found = EMPI_TRUE;
        }
        fprintf (stderr, "add_class(%d): name:%s, idclass:%d, nprocs:%d, iniprocs:%d, maxprocs:%d; EMPI_GLOBAL_nhclasses: %d\n", getpid(), class->name, class->idclass, class->nprocs, class->iniprocs, class->maxprocs, EMPI_GLOBAL_nhclasses);
        last_class = class;
        fprintf (stderr, "add_class(%d): end found 1\n", getpid());
        class = class->next;
        fprintf (stderr, "add_class(%d): end found 2\n", getpid());
    }
    
    // if not found add it on global list
    if (is_found == EMPI_FALSE) {
        fprintf (stderr, "add_class(%d): new class\n", getpid());
        if (last_class == NULL) {
            //initialization
            class = (EMPI_Class_type*) calloc (sizeof(EMPI_Class_type),1);
            assert (class);
            //set pointer
            EMPI_GLOBAL_system_classes = class;
        } else {
            last_class->next = (EMPI_Class_type*) calloc (sizeof(EMPI_Class_type),1);
            assert (last_class->next);
            class = last_class->next;
        }
        //Set host id
        class->idclass = EMPI_GLOBAL_nhclasses;
        //number of hosts
        EMPI_GLOBAL_nhclasses ++;
        class_id = class->idclass;

        strcpy (class->name, classname);
        strcpy (EMPI_GLOBAL_hclasses[class_id], class->name);
        
        class->nprocs = numprocs;
        class->maxprocs = add_maxprocs;
        class->iniprocs = 0;
        if (is_initial_proc) {
            class->iniprocs = numprocs;
        }
        class->icost = cost;
        if ((EMPI_GLOBAL_mpolicy == EMPI_COST)||(EMPI_GLOBAL_mpolicy == EMPI_COST_IRR)) {
            class->cost = cost;
        } else {
            class->cost = 0.0;
        }
        class->mflops = mflops;
        if (class->mflops < 0) class->mflops = -1; //runtime update
        fprintf (stderr, "add_class(%d): name:%s, idclass:%d, nprocs:%d, iniprocs:%d, maxprocs:%d; EMPI_GLOBAL_nhclasses: %d\n", getpid(), class->name, class->idclass, class->nprocs, class->iniprocs, class->maxprocs, EMPI_GLOBAL_nhclasses);
    }
    
    fprintf (stderr, "add_class(%d): end\n", getpid());

    return class_id;
}

/****************************************************************************************************************************************
*
*    'add_host
*
****************************************************************************************************************************************/
int add_host (char *hostname, int numprocs, int is_initial_proc, int maxprocs, char *classname) {
    
    EMPI_host_type *hostlist = EMPI_GLOBAL_hostlist;
    EMPI_host_type *last_hostlist = NULL;
    int is_found = EMPI_FALSE;
    int add_maxprocs = 0;
    double default_cost = 0.0;
    int default_mflops = 0;
    int hostid = 0;
    
    fprintf (stderr, "add_host(%d): begin\n", getpid());

    assert(strlen(hostname)<128);
    assert(strlen(classname)<128);

    // look for it in global list
    while (hostlist != NULL) {
        if (strcmp (hostlist->hostname, hostname) == 0) {
            fprintf (stderr, "add_host(%d): hostname found\n", getpid());
            assert(is_found == EMPI_FALSE);
            assert(strcmp (hostlist->hclass, classname)==0);
            hostid = hostlist->id;
            hostlist->nprocs = hostlist->nprocs + numprocs;
            if (is_initial_proc) {
                hostlist->iniprocs = hostlist->iniprocs + numprocs;
            }
            if (maxprocs != 0) {
                add_maxprocs = maxprocs - hostlist->maxprocs;
                hostlist->maxprocs = maxprocs;
            } else if (hostlist->maxprocs < hostlist->nprocs) {
                add_maxprocs = hostlist->nprocs - hostlist->maxprocs;
                hostlist->maxprocs = hostlist->nprocs;
            }
            EMPI_GLOBAL_maxprocs = EMPI_GLOBAL_maxprocs + add_maxprocs;
            // add to the class
            add_class (hostname, numprocs, is_initial_proc, add_maxprocs, 0, 0.0);
            is_found = EMPI_TRUE;
        }
        fprintf (stderr, "add_host(%d): hostname:%s, id:%d, classname:%s, classid:%d, nprocs:%d, iniprocs:%d, maxprocs:%d; EMPI_GLOBAL_nhosts: %d; EMPI_GLOBAL_maxprocs: %d\n", getpid(), hostlist->hostname, hostlist->id, hostlist->hclass, hostlist->idclass, hostlist->nprocs, hostlist->iniprocs, hostlist->maxprocs, EMPI_GLOBAL_nhosts, EMPI_GLOBAL_maxprocs);
        last_hostlist = hostlist;
        hostlist = hostlist->next;
    }
    
    // if not found add it on global list
    if (is_found == EMPI_FALSE) {
        fprintf (stderr, "add_host(%d): new host\n", getpid());
        if (last_hostlist == NULL) {
            hostlist = (EMPI_host_type*) calloc (sizeof(EMPI_host_type),1);
            assert (hostlist);
            //set pointer
            EMPI_GLOBAL_hostlist = hostlist;
        } else {
            last_hostlist->next = (EMPI_host_type*) calloc (sizeof(EMPI_host_type),1);
            assert (last_hostlist->next);
            hostlist = last_hostlist->next;
        }
        //Set host id
        hostlist->id = EMPI_GLOBAL_nhosts;
        hostid = hostlist->id;
        //number of hosts
        EMPI_GLOBAL_nhosts ++;
        
        //Set hostname
        strcpy (hostlist->hostname, hostname);
        
        //Set nprocs
        hostlist->nprocs = numprocs;
        hostlist->iniprocs = 0;
        if (is_initial_proc) {
            hostlist->iniprocs = numprocs;
        }
        if (maxprocs != 0) {
            add_maxprocs = maxprocs;
            hostlist->maxprocs =  maxprocs;
        } else {
            add_maxprocs = numprocs;
            hostlist->maxprocs =  numprocs;
        }
        EMPI_GLOBAL_maxprocs = EMPI_GLOBAL_maxprocs + add_maxprocs;

        //add default cost and mpflops
        hostlist->cost = default_cost;
        hostlist->mflops = default_mflops;
        
        // add class
        int class_id = add_class (hostname, numprocs, is_initial_proc,  add_maxprocs, default_mflops, default_cost);
        
        //Set classname and classid
        strcpy (hostlist->hclass, classname);
        hostlist->idclass = class_id;
        
        fprintf (stderr, "add_host(%d): hostname:%s, id:%d, classname:%s, classid:%d, nprocs:%d, iniprocs:%d, maxprocs:%d; EMPI_GLOBAL_nhosts: %d; EMPI_GLOBAL_maxprocs: %d\n", getpid(), hostlist->hostname, hostlist->id, hostlist->hclass, hostlist->idclass, hostlist->nprocs, hostlist->iniprocs, hostlist->maxprocs, EMPI_GLOBAL_nhosts, EMPI_GLOBAL_maxprocs);
    }
    fprintf (stderr, "add_host(%d): end\n", getpid());
    return hostid;
}

/****************************************************************************************************************************************
*
*    'EMPI_Spawn_Init_Orig'
*
****************************************************************************************************************************************/
int EMPI_Spawn_Init_Orig () {
    
    int err, rank, size;
    MPI_Comm parentcomm;
    
    //Get parent
    err = MPI_Comm_get_parent (&parentcomm);
    if (err) fprintf (stderr, "Error in MPI_Comm_get_parent\n");

    // CHANGE
    int spawnedProcess = 0;
    // Spawned process
    if (parentcomm != MPI_COMM_NULL) {

        // set process as spawned
        spawnedProcess = 1;
        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Intercomm_merge in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif
        //Merge evertything under parentcomm intercommunicator, into EMPI_COMM_WORLD
        fprintf (stderr, "EMPI_Spawn_Init_Orig(%d): MPI_Intercomm_merge (parentcomm, 1, PTR_EMPI_COMM_WORLD)\n", getpid());
        err = MPI_Intercomm_merge (parentcomm, 1, PTR_EMPI_COMM_WORLD);
        if (err) fprintf (stderr, "Error in MPI_Intercomm_merge\n");
        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Comm_disconnect in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif
        fprintf (stderr, "EMPI_Spawn_Init_Orig(%d): MPI_Comm_disconnect (&parentcomm)\n", getpid());
        err = MPI_Comm_disconnect (&parentcomm);
        if (err) fprintf (stderr, "Error in MPI_Comm_disconnect\n");
        fprintf (stderr, "EMPI_Spawn_Init_Orig(%d): DONE MPI_Comm_disconnect (&parentcomm)\n", getpid());

        MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
        MPI_Comm_size (EMPI_COMM_WORLD, &size);
        fprintf (stderr, "EMPI_Spawn_Init_Orig(%d): MPI_Init rank=%d; size=%d\n", getpid(), rank,size);
        fprintf (stderr, "EMPI_Spawn_Init_Orig(%d): MPI_Barrier(EMPI_COMM_WORLD) rank=%d\n", getpid(), rank);
        MPI_Barrier(EMPI_COMM_WORLD);
        fprintf (stderr, "EMPI_Spawn_Init_Orig(%d): DONE MPI_Barrier(EMPI_COMM_WORLD) rank=%d\n", getpid(), rank);
    } else {
        // set process as initial process
        spawnedProcess = 0;

        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Comm_dup in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif
        //Dup MPI_COMM_WORLD
        fprintf (stderr, "EMPI_Spawn_Init_Orig(%d): MPI_Comm_dup (MPI_COMM_WORLD, PTR_EMPI_COMM_WORLD)\n", getpid());
        err = MPI_Comm_dup (MPI_COMM_WORLD, PTR_EMPI_COMM_WORLD);
        if (err) fprintf (stderr, "Error in MPI_Comm_dup\n");
    }
    // CHANGE END

    return spawnedProcess;
}

/****************************************************************************************************************************************
*
*    'MPI_Init'
*
****************************************************************************************************************************************/
int MPI_Init (int *argc, char ***argv) {
    return FLEXMPI_Init(argc,argv);
}

/****************************************************************************************************************************************
*
*    'FLEXMPI_Init'
*
****************************************************************************************************************************************/
int FLEXMPI_Init (int *argc, char ***argv) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Init in <%s> ***\n", __FILE__);
    #endif

    fprintf (stderr, "main: getpid: %d\n", getpid());

    char *init_hostlist = NULL;
    char *added_hostlist = NULL;

    int err, rank, size, tag=997, rprocs, rdata[4], n, i;
    int provided;
    
    int blocklen [17] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, EMPI_Monitor_string_size, EMPI_Monitor_string_size,1,1,1,1,1};
    //int blocklen_energy [2] = {1, 1};
    // CHANGE
    //int energy_rank = -1;
    // END CHANGE

    int host_name_length;

    MPI_Aint displs[17], address, start_address;

    MPI_Comm parentcomm;

    MPI_Status mpi_status;

    MPI_Datatype EMPI_monitor_struct_type [17] = {MPI_INT, MPI_LONG_LONG, MPI_LONG_LONG, MPI_DOUBLE, MPI_LONG_LONG, MPI_INT, MPI_LONG_LONG, MPI_LONG_LONG, MPI_LONG_LONG, MPI_LONG_LONG, MPI_CHAR, MPI_CHAR, MPI_INT,MPI_INT, MPI_INT, MPI_DOUBLE,MPI_INT};

    // Used for configuring the socket (controller communication)
    struct sockaddr_in si_me;
    struct hostent   *he;

    // start limitless script
    system ("limitless.sh start");
            
    //Initialize MPI environment
    err = MPI_Init_thread(argc, argv,MPI_THREAD_FUNNELED,&provided); // The new threads do not perform MPI calls


    if (err == MPI_ERR_OTHER) return MPI_ERR_OTHER;
    if(provided!=MPI_THREAD_FUNNELED){
          fprintf(stderr,"Error in MPI_Init routine: MPI_THREAD_FUNNELED is not supported \n");
          exit(1);        
    }
    
    //Initialize global variables with default values
    EMPI_GLOBAL_tcomp = EMPI_GLOBAL_tcomm = EMPI_GLOBAL_PAPI_rtime_init = EMPI_GLOBAL_PAPI_ptime_init = EMPI_GLOBAL_hpos = 0;
    EMPI_GLOBAL_overhead_rpolicy = EMPI_GLOBAL_overhead_lbalance = EMPI_GLOBAL_overhead_processes = EMPI_GLOBAL_overhead_rdata = EMPI_GLOBAL_overhead_aux = 0;
    EMPI_GLOBAL_tcomm_interval = 0;
    EMPI_GLOBAL_tcomm_interval_lb = 0;
    EMPI_GLOBAL_over_si = 0;
    EMPI_GLOBAL_comm_prev = 0;
    EMPI_GLOBAL_lbalance_disabled = EMPI_FALSE;
    EMPI_GLOBAL_nhclasses = 0;
    EMPI_GLOBAL_initnc = EMPI_TRUE;
    EMPI_GLOBAL_tcomp_ini = MPI_Wtime();
    EMPI_GLOBAL_minprocs = EMPI_GLOBAL_hsteps = 1;
    EMPI_GLOBAL_niter = 10;
    EMPI_GLOBAL_niter_lb = 10;
    EMPI_GLOBAL_wpolicy = EMPI_STATIC;
    EMPI_GLOBAL_spolicy = EMPI_AVAIL_NODE;
    EMPI_GLOBAL_vcounts = EMPI_GLOBAL_displs = NULL;
    
    EMPI_GLOBAL_Data = NULL;
    EMPI_GLOBAL_system_classes = NULL;
    EMPI_GLOBAL_mpolicy = EMPI_NULL;
    EMPI_GLOBAL_lbalance = EMPI_GLOBAL_PAPI_init = EMPI_GLOBAL_self_adaptation = EMPI_FALSE;
    EMPI_GLOBAL_lbpolicy = EMPI_LBMFLOPS;
    EMPI_GLOBAL_concurrency = EMPI_TRUE;
    EMPI_GLOBAL_PAPI_flops = EMPI_GLOBAL_PAPI_rtime = EMPI_GLOBAL_PAPI_ptime = EMPI_GLOBAL_PAPI_hwpc_1 = EMPI_GLOBAL_PAPI_hwpc_2 = 0;
    EMPI_GLOBAL_PAPI_flops_lb = EMPI_GLOBAL_PAPI_rtime_lb = EMPI_GLOBAL_PAPI_ptime_lb = EMPI_GLOBAL_PAPI_hwpc_1_lb = EMPI_GLOBAL_PAPI_hwpc_2_lb = 0;
    EMPI_GLOBAL_PAPI_eventSet = PAPI_NULL;
    EMPI_GLOBAL_threshold = 0.15; //load balancing threshold
    EMPI_GLOBAL_allocation = EMPI_NULL;
    EMPI_GLOBAL_iterative_ini = EMPI_GLOBAL_iterative_end = 0;
    EMPI_GLOBAL_Load_threshold = 0.05; //OS noise threshold
    EMPI_GLOBAL_obj_texec_threshold = 0.15; //execution time threshold
    EMPI_GLOBAL_comms = NULL;
    EMPI_GLOBAL_cum_cost = EMPI_GLOBAL_cum_time = 0;
    EMPI_GLOBAL_iteration = 0;
    EMPI_GLOBAL_track_flops = NULL;
    EMPI_GLOBAL_track_rtime = NULL;
    EMPI_GLOBAL_sampling_time = 0.0;
    EMPI_GLOBAL_nextrm = -1;
    EMPI_GLOBAL_Adaptability_policy = EMPI_ADAPTABILITY_EX;
    EMPI_GLOBAL_percentage = 0.25;
    EMPI_GLOBAL_corebinding=0;
    
    // I/O-related variables
    EMPI_GLOBAL_delayio=0;
    EMPI_GLOBAL_delayiotime=0;
    EMPI_GLOBAL_tio_last = MPI_Wtime();
    EMPI_GLOBAL_tio_ini = EMPI_GLOBAL_tio_fin = 0;
    EMPI_GLOBAL_socket=-1;
    EMPI_GLOBAL_dummyIO=-1;     // When <0 performs MPI I/O; When >=0 performs dummy I/O of EMPI_GLOBAL_dummyIO seconds 

    // Poster thread (initially not active)
    EMPI_GLOBAL_posteractive = 0;

    //EMPI_GLOBAL_spawn_cost = 0.520147; //0.520147085671638 old
    EMPI_GLOBAL_spawn_cost = 0.3248116; //0.520147085671638
    EMPI_GLOBAL_remove_cost = 0.000800; //0.000800976666412954
    //EMPI_GLOBAL_alpha = 0.00005; (old)
    EMPI_GLOBAL_alpha = 0.000034; //seconds
    //EMPI_GLOBAL_bandwidth = 943; //Mbits per second (old)
    EMPI_GLOBAL_bandwidth = 2834; //Mbits per second
    EMPI_GLOBAL_beta = 1 / (EMPI_GLOBAL_bandwidth * 1.0E6) ;
    EMPI_GLOBAL_gamma = 2.0E-9;
    
    EMPI_GLOBAL_PAPI_numevents            = 0;
    
    EMPI_GLOBAL_PAPI_real_flops_iteration = 0;
    EMPI_GLOBAL_debug_comms                  = 0;
    EMPI_GLOBAL_flag_dynamic              = 1;
    EMPI_GLOBAL_energy_eficiency_op_mode  =-1;
    EMPI_GLOBAL_flag_enter                = 1;

    //EMPI_GLOBAL_ENERGY_overflow_counter_1 = 0;
    //EMPI_GLOBAL_ENERGY_overflow_counter_2 = 0;

    EMPI_GLOBAL_non_exclusive              = 1;

    flag = 0;

    EMPI_GLOBAL_perform_load_balance      = 0;
    EMPI_GLOBAL_spawn_data.dirty          = 0;
    EMPI_GLOBAL_spawn_data.hostid         = NULL;
    EMPI_GLOBAL_spawn_data.nprocs         = NULL;
    //EMPI_GLOBAL_spawn_data.name         = NULL;
    
    EMPI_array_alloc                      = 0;

    //ALBERTO
        //ALBERTO
    ADM_GLOBAL_checkpointing = 0;
    ADM_GLOBAL_waiting_decision = 0; 
    ADM_GLOBAL_shrink = 0;
    ADM_GLOBAL_nnodes = 0;
    ADM_APP_TYPE=0; // Flexmpi

    
   
    // We need to protect it because the server access to EMPI_GLOBAL_PAPI_nhwpc_* when option 7 is used
    pthread_mutex_lock(&EMPI_GLOBAL_server_lock); // Only the server has an attached thread
    // Default values for hwpc_1 and hwpc_2 hardware counters
    sprintf(EMPI_GLOBAL_PAPI_nhwpc_1,"PAPI_SP_OPS");
    sprintf(EMPI_GLOBAL_PAPI_nhwpc_2,"PAPI_TOT_CYC");    
    strncpy(EMPI_GLOBAL_monitoring_data.nhwpc_1,EMPI_GLOBAL_PAPI_nhwpc_1,EMPI_Monitor_string_size);
    strncpy(EMPI_GLOBAL_monitoring_data.nhwpc_2,EMPI_GLOBAL_PAPI_nhwpc_2,EMPI_Monitor_string_size);
    EMPI_GLOBAL_monitoring_data.termination=0;                // 0 means no termination
    EMPI_GLOBAL_corebinding=0;                                // Sets no core binding as default

    sprintf(EMPI_GLOBAL_controller,"NULL");                   // Default name of the external server
    strcpy(EMPI_GLOBAL_application,*argv[0]);                 // Name of the application
    
    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);

    
    // No process-core binding
    EMPI_GLOBAL_monitor.corebinding=0;


    // Generates a Datatype for the monitor struct
    //displacements
    MPI_Get_address (&EMPI_GLOBAL_monitor, &start_address);
    MPI_Get_address (&(EMPI_GLOBAL_monitor.count), &address);
    displs[0] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.rtime), &address);
    displs[1] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.ptime), &address);
    displs[2] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.ctime), &address);
    displs[3] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.flops), &address);
    displs[4] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.hostid), &address);
    displs[5] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.flops_iteration), &address);
    displs[6] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.it_time), &address);
    displs[7] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.hwpc_1), &address);
    displs[8] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.hwpc_2), &address);
    displs[9] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.nhwpc_1), &address);
    displs[10] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.nhwpc_2), &address);
    displs[11] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.corebinding), &address);
    displs[12] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.termination), &address);
    displs[13] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.lbalance), &address);
    displs[14] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.iotime), &address);
    displs[15] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.EMPI_array_alloc), &address);
    displs[16] = address - start_address;
    
    //MPI datatype
    MPI_Type_create_struct (17, blocklen, displs, EMPI_monitor_struct_type, &EMPI_Monitor_Datatype); //MPI_Type_struct
    MPI_Type_commit (&EMPI_Monitor_Datatype);

    // MPI_Type_create_struct (2, blocklen, displs, EMPI_monitor_struct_type, &EMPI_Monitor_Datatype);
    // MPI_Type_commit (&EMPI_Monitor_Datatype);

    //Init PAPI library
    int retval;
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if(retval != PAPI_VER_CURRENT){
        fprintf(stderr,"Error initializing library\n");
    }
    if (retval != PAPI_VER_CURRENT && retval > 0) {
          fprintf(stderr,"PAPI library version mismatch!\n");
          exit(1);
    }

    if (retval < 0) {
          fprintf(stderr, "Initialization error!\n");
          exit(1);
    }

    //set process status
    EMPI_GLOBAL_status = EMPI_ACTIVE;

    // CHANGE TEMP
    // init malleability reconfig data
    reconfig_data_t aux_reconfig_data = RECONFIG_DATA_DEFAULT;
    EMPI_GLOBAL_reconfig_data = aux_reconfig_data;
    // END CHANGE TEMP

    //debug
    #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
        fprintf (stderr, "\n*** DEBUG_MSG::call::EMPI_Parse_options in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
    #endif
    
    // Manages the input argument:
    for(i=0;i<*argc;i++){
        if(strcmp((*argv)[i],"-alloc:1")==0){
            EMPI_alloc();
        }
    }

    //Parse EMPI global options
    //PTR_EMPI_COMM_WORLD = (MPI_Comm *)malloc(sizeof(MPI_Comm));
    //MPI_Comm_dup (MPI_COMM_WORLD, PTR_EMPI_COMM_WORLD);
    EMPI_Parse_options (*argc, *argv);
    //MPI_Comm_disconnect (PTR_EMPI_COMM_WORLD);
    //EMPI_COMM_WORLD = MPI_COMM_NULL;


    // CHANGE: START_NEWVER
    int spawnedProcess;
    if  (EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY_TRIG) {
        fprintf (stderr, "EMPI_Init(%d): EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY_TRIG\n",getpid());
        //spawnedProcess = EMPI_Spawn_Init_Orig ();
        spawnedProcess = EMPI_Spawn_Init ((*argv), &EMPI_GLOBAL_reconfig_data);
    } else {
        spawnedProcess = EMPI_Spawn_Init_Orig ();
    }
    // CHANGE: END_NEWVER

    // Spawned process
    if (spawnedProcess == 1) {
        
        int *countdispl = NULL;

        //Process type
        EMPI_GLOBAL_type = EMPI_SPAWNED;


        // CHANGE
        //debug
        //#if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
        //    fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Intercomm_merge in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        //#endif
        //Merge intercommunicator
        //err = MPI_Intercomm_merge (parentcomm, 1, PTR_EMPI_COMM_WORLD);
        //if (err) fprintf (stderr, "Error in MPI_Intercomm_merge\n");

        //debug
        //#if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
        //    fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Comm_disconnect in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        //#endif

        //Disconnect aux communicator
        //err = MPI_Comm_disconnect (&parentcomm);
        //if (err) fprintf (stderr, "Error in MPI_Comm_disconnect\n");
        // END CHANGE

        //Recv rprocs, minprocs, iteration and hostid. The MPI_Send command is executed in EMPI_Spawn function.
        fprintf (stderr, "FLEXMPI_Init(%d): BEGIN PMPI_Recv\n",getpid());
        PMPI_Recv (rdata, 4, MPI_INT, EMPI_root, tag, EMPI_COMM_WORLD, &mpi_status);
        fprintf (stderr, "FLEXMPI_Init(%d): END PMPI_Recv\n",getpid());

        //Set rprocs, minprocs, iteration and hostid
        rprocs = rdata[0];
        EMPI_GLOBAL_minprocs = rdata[1];
        EMPI_GLOBAL_iteration = rdata[2];
        EMPI_GLOBAL_hostid = rdata[3];

        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stderr, "\n*** DEBUG_MSG::call::EMPI_Spawn in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif

        MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
        MPI_Comm_size (EMPI_COMM_WORLD, &size);

        fprintf (stderr, "FLEXMPI_Init(%d): rank/size(%d/%d)\n",getpid(),rank,size);

        EMPI_GLOBAL_vcounts = (int*) calloc (size, sizeof (int));
        assert (EMPI_GLOBAL_vcounts);

        EMPI_GLOBAL_displs = (int*) calloc (size, sizeof (int));
        assert (EMPI_GLOBAL_displs);

        if (EMPI_GLOBAL_mpolicy != EMPI_NULL) {

            // CHANGE: JAVI
            int sync_data;
            fprintf(stderr, "FLEXMPI_Init(%d): PMPI_Bcast sync_data\n",getpid());
            PMPI_Bcast (&sync_data, 1, MPI_INT, EMPI_root, EMPI_COMM_WORLD);
            if (sync_data == 1) {
                // END CHANGE: JAVI
                countdispl = (int*) malloc ((size*2) * sizeof(int));
                assert (countdispl);
                
                //Recv vcounts and displs
                PMPI_Bcast (countdispl, (size*2), MPI_INT, EMPI_root, EMPI_COMM_WORLD);
                
                //memcpy
                memcpy (EMPI_GLOBAL_vcounts, countdispl, size*sizeof(int));
                memcpy (EMPI_GLOBAL_displs, countdispl+size, size*sizeof(int));
                
                free (countdispl);
            // CHANGE: JAVI
            }
            // END CHANGE: JAVI
        }

        //set dynamic workload
        EMPI_GLOBAL_wpolicy = EMPI_DYNAMIC;

        fprintf (stderr, "FLEXMPI_Init(%d): End spawned\n",getpid());

    } else { //Native process

        //Initialize global variables
        EMPI_GLOBAL_nhosts = 0;

        //Process type
        EMPI_GLOBAL_type = EMPI_NATIVE;

        //Set minprocs
        MPI_Comm_rank (MPI_COMM_WORLD, &rank);
        MPI_Comm_size (MPI_COMM_WORLD, &size);

        //Initialize number of procs
        EMPI_GLOBAL_minprocs = size;
        EMPI_GLOBAL_maxprocs = 0;

        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stderr, "\n*** DEBUG_MSG::call::EMPI_Parse_cfile in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif

        //Parse cfile with the node list
        //EMPI_Parse_cfile (*argc, *argv, &init_hostlist);
        //Create system table
        //EMPI_Create_system_classes ();
        
        // fillup hostlist ands init_hostlist
        EMPI_fillup_init_hostlist ();
        get_init_hostlist (&init_hostlist);

        // CHANGE
        //MPI_Comm_dup(MPI_COMM_WORLD, &EMPI_GLOBAL_comm_energy);
        //MPI_Comm_rank (EMPI_GLOBAL_comm_energy, &energy_rank);
        // END CHANGE
        
        //gerhostname(host_name, host_name_length);
        MPI_Get_processor_name (EMPI_GLOBAL_host_name, &host_name_length);
        
    }
    
     // Configures the socket for controller communication
    if (rank == EMPI_root)
    {
        if((EMPI_GLOBAL_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
            fprintf(stderr, "Error creating socket \n");
        }
        
        memset((char *) &si_me, 0, sizeof(si_me));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(EMPI_GLOBAL_recvport+200);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(EMPI_GLOBAL_socket, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
        {
            fprintf(stderr, "Error binding socket \n");
        }
    
        EMPI_GLOBAL_controller_addr.sin_family = AF_INET;
        EMPI_GLOBAL_controller_addr.sin_port = htons(EMPI_GLOBAL_sendport);

        if ((he = gethostbyname(EMPI_GLOBAL_controller) ) == NULL ) {
            fprintf(stderr, "Error resolving host name \n");
        }
                    
        EMPI_GLOBAL_controller_addr.sin_addr=*(struct in_addr *) he->h_addr;
        memset(EMPI_GLOBAL_controller_addr.sin_zero, '\0', sizeof(EMPI_GLOBAL_controller_addr.sin_zero));  
    }   

    if (rank == EMPI_root) {

        //track FLOPS
        EMPI_GLOBAL_track_flops = (long long*) malloc (2 * sizeof(long));
        assert (EMPI_GLOBAL_track_flops);

        //track RTIME
        EMPI_GLOBAL_track_rtime = (long long*) malloc (2 * sizeof(long));
        assert (EMPI_GLOBAL_track_rtime);

        //set initial track
        EMPI_GLOBAL_track_flops[0] = -1;
        EMPI_GLOBAL_track_flops[1] = -1;

        EMPI_GLOBAL_track_rtime[0] = -1;
        EMPI_GLOBAL_track_rtime[1] = -1;
    }

    if ((EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY)||(EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY_COND)) EMPI_Parse_malleability ();

    //FIXME: hay que enviarlo a los spawned procs.
    //Historical monitoring
    EMPI_GLOBAL_hmon = (int*) malloc (EMPI_GLOBAL_hsteps * sizeof(int));
    assert (EMPI_GLOBAL_hmon);

    for (n = 0; n < EMPI_GLOBAL_hsteps; n ++) EMPI_GLOBAL_hmon[n] = EMPI_DEDICATED;


    //EMPI_GLOBAL_power_monitoring_data = calloc(EMPI_GLOBAL_nhosts, sizeof(double));
    // fprintf(stderr, "minprocs = %d,  nhosts = %d\n", EMPI_GLOBAL_minprocs, EMPI_GLOBAL_nhosts);
    EMPI_GLOBAL_power_monitoring_data = calloc(EMPI_GLOBAL_minprocs, sizeof(double));
    
    // CHANGE: JAVI
    // if it is a native process
    if (spawnedProcess == 0) {
        // ADD extra processes
        fprintf (stderr, "FLEXMPI_Init(%d): Begin EMPI_Expand_processes\n",getpid());
        EMPI_Expand_processes (*argc, *argv, &added_hostlist);
        fprintf (stderr, "FLEXMPI_Init(%d): End EMPI_Expand_processes\n",getpid());
    }
    // END CHANGE: JAVI

    if (rank == EMPI_root)
    {
        int rc;  // return code
        //pthread_t thread, rpc_thread; // CHANGE FINI
        pthread_attr_t attr, attr2; // CHANGE FINI

        rpc_thread_flag = 1; // CHANGE FINI

        if (pthread_mutex_init(&EMPI_GLOBAL_server_lock, NULL) != 0)
        {
            fprintf(stderr, "\n mutex init failed\n");
        }

        rc = pthread_attr_init(&attr);
        check_posix_return(rc, "Initializing attribute");

        rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        check_posix_return(rc, "Setting detached state");

        rc = pthread_create(&thread, &attr, (void*)&command_listener, NULL);
        check_posix_return(rc, "Creating socket listener");
        
        // CHANGE BEGIN
        rc = pthread_attr_init(&attr2); // CHANGE FINI
        rc = pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_JOINABLE); // CHANGE FINI

        rc = pthread_create(&rpc_thread, &attr2, (void*)&thread_rpc_release_nodes, NULL); // CHANGE FINI
        check_posix_return(rc, "Creating thread_rpc_release_nodes");
        // JAVIER: 01032024: MERGIN DYNAMIC AND STARTSTOP
        fprintf (stderr, "FLEXMPI_Init(%d): ADM_APP_TYPE = %d\n",getpid(), ADM_APP_TYPE);
        if (ADM_APP_TYPE == 0) {
            signal_thread_rpc_init(init_hostlist, added_hostlist); // CHANGE: JAVI
        } else {
            signal_thread_rpc_init_ss(init_hostlist); // CHANGE: JAVI
        }
        // CHANGE END
    }
    
    // deallocate added_hostlist and init_hostlist
    free(added_hostlist); // CHANGE: JAVI
    free(init_hostlist); // CHANGE: JAVI

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Init in <%s> ***\n", __FILE__);
    #endif
        
    // run ADM_Init
    int world_rank, world_size;
    MPI_Comm_rank(EMPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(EMPI_COMM_WORLD, &world_size);
    fprintf (stderr, "FLEXMPI_Init(%d): Begin ADM_Init\n",getpid());
    ADM_Init (*argc, *argv, world_rank, world_size);
    fprintf (stderr, "FLEXMPI_Init(%d): End ADM_Init\n",getpid());

    
    return MPI_SUCCESS;
}

/**
 * ALBERTO: FlexMPI_Init for start and stop applications
 * Difference in the reconfigure function.
 * Consider ADM_APP_MODE (0) as initial run, and (1) as restart
*/
int FLEXMPI_Init_ss (int *argc, char ***argv) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Init in <%s> ***\n", __FILE__);
    #endif

    fprintf (stderr, "main: getpid: %d\n", getpid());

    char *init_hostlist = NULL;

    int err, rank, size, tag=997, rprocs, rdata[4], n, i;
    int provided;
    
    int blocklen [17] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, EMPI_Monitor_string_size, EMPI_Monitor_string_size,1,1,1,1,1};
    //int blocklen_energy [2] = {1, 1};
    // CHANGE
    //int energy_rank = -1;
    // END CHANGE

    int host_name_length;

    MPI_Aint displs[17], address, start_address;

    MPI_Comm parentcomm;

    MPI_Status mpi_status;

    MPI_Datatype EMPI_monitor_struct_type [17] = {MPI_INT, MPI_LONG_LONG, MPI_LONG_LONG, MPI_DOUBLE, MPI_LONG_LONG, MPI_INT, MPI_LONG_LONG, MPI_LONG_LONG, MPI_LONG_LONG, MPI_LONG_LONG, MPI_CHAR, MPI_CHAR, MPI_INT,MPI_INT, MPI_INT, MPI_DOUBLE,MPI_INT};

    // Used for configuring the socket (controller communication)
    struct sockaddr_in si_me;
    struct hostent   *he;

    //Initialize MPI environment
    err = MPI_Init_thread(argc, argv,MPI_THREAD_FUNNELED,&provided); // The new threads do not perform MPI calls


    if (err == MPI_ERR_OTHER) return MPI_ERR_OTHER;
    if(provided!=MPI_THREAD_FUNNELED){
          fprintf(stderr,"Error in MPI_Init routine: MPI_THREAD_FUNNELED is not supported \n");
          exit(1);        
    }
    
    //Initialize global variables with default values
    EMPI_GLOBAL_tcomp = EMPI_GLOBAL_tcomm = EMPI_GLOBAL_PAPI_rtime_init = EMPI_GLOBAL_PAPI_ptime_init = EMPI_GLOBAL_hpos = 0;
    EMPI_GLOBAL_overhead_rpolicy = EMPI_GLOBAL_overhead_lbalance = EMPI_GLOBAL_overhead_processes = EMPI_GLOBAL_overhead_rdata = EMPI_GLOBAL_overhead_aux = 0;
    EMPI_GLOBAL_tcomm_interval = 0;
    EMPI_GLOBAL_tcomm_interval_lb = 0;
    EMPI_GLOBAL_over_si = 0;
    EMPI_GLOBAL_comm_prev = 0;
    EMPI_GLOBAL_lbalance_disabled = EMPI_FALSE;
    EMPI_GLOBAL_nhclasses = 0;
    EMPI_GLOBAL_initnc = EMPI_TRUE;
    EMPI_GLOBAL_tcomp_ini = MPI_Wtime();
    EMPI_GLOBAL_minprocs = EMPI_GLOBAL_hsteps = 1;
    EMPI_GLOBAL_niter = 10;
    EMPI_GLOBAL_niter_lb = 10;
    EMPI_GLOBAL_wpolicy = EMPI_STATIC;
    EMPI_GLOBAL_spolicy = EMPI_AVAIL_NODE;
    EMPI_GLOBAL_vcounts = EMPI_GLOBAL_displs = NULL;
    
    EMPI_GLOBAL_Data = NULL;
    EMPI_GLOBAL_system_classes = NULL;
    EMPI_GLOBAL_mpolicy = EMPI_NULL;
    EMPI_GLOBAL_lbalance = EMPI_GLOBAL_PAPI_init = EMPI_GLOBAL_self_adaptation = EMPI_FALSE;
    EMPI_GLOBAL_lbpolicy = EMPI_LBMFLOPS;
    EMPI_GLOBAL_concurrency = EMPI_TRUE;
    EMPI_GLOBAL_PAPI_flops = EMPI_GLOBAL_PAPI_rtime = EMPI_GLOBAL_PAPI_ptime = EMPI_GLOBAL_PAPI_hwpc_1 = EMPI_GLOBAL_PAPI_hwpc_2 = 0;
    EMPI_GLOBAL_PAPI_flops_lb = EMPI_GLOBAL_PAPI_rtime_lb = EMPI_GLOBAL_PAPI_ptime_lb = EMPI_GLOBAL_PAPI_hwpc_1_lb = EMPI_GLOBAL_PAPI_hwpc_2_lb = 0;
    EMPI_GLOBAL_PAPI_eventSet = PAPI_NULL;
    EMPI_GLOBAL_threshold = 0.15; //load balancing threshold
    EMPI_GLOBAL_allocation = EMPI_NULL;
    EMPI_GLOBAL_iterative_ini = EMPI_GLOBAL_iterative_end = 0;
    EMPI_GLOBAL_Load_threshold = 0.05; //OS noise threshold
    EMPI_GLOBAL_obj_texec_threshold = 0.15; //execution time threshold
    EMPI_GLOBAL_comms = NULL;
    EMPI_GLOBAL_cum_cost = EMPI_GLOBAL_cum_time = 0;
    EMPI_GLOBAL_iteration = 0;
    EMPI_GLOBAL_track_flops = NULL;
    EMPI_GLOBAL_track_rtime = NULL;
    EMPI_GLOBAL_sampling_time = 0.0;
    EMPI_GLOBAL_nextrm = -1;
    EMPI_GLOBAL_Adaptability_policy = EMPI_ADAPTABILITY_EX;
    EMPI_GLOBAL_percentage = 0.25;
    EMPI_GLOBAL_corebinding=0;
    
    // I/O-related variables
    EMPI_GLOBAL_delayio=0;
    EMPI_GLOBAL_delayiotime=0;
    EMPI_GLOBAL_tio_last = MPI_Wtime();
    EMPI_GLOBAL_tio_ini = EMPI_GLOBAL_tio_fin = 0;
    EMPI_GLOBAL_socket=-1;
    EMPI_GLOBAL_dummyIO=-1;     // When <0 performs MPI I/O; When >=0 performs dummy I/O of EMPI_GLOBAL_dummyIO seconds 

    // Poster thread (initially not active)
    EMPI_GLOBAL_posteractive = 0;

    //EMPI_GLOBAL_spawn_cost = 0.520147; //0.520147085671638 old
    EMPI_GLOBAL_spawn_cost = 0.3248116; //0.520147085671638
    EMPI_GLOBAL_remove_cost = 0.000800; //0.000800976666412954
    //EMPI_GLOBAL_alpha = 0.00005; (old)
    EMPI_GLOBAL_alpha = 0.000034; //seconds
    //EMPI_GLOBAL_bandwidth = 943; //Mbits per second (old)
    EMPI_GLOBAL_bandwidth = 2834; //Mbits per second
    EMPI_GLOBAL_beta = 1 / (EMPI_GLOBAL_bandwidth * 1.0E6) ;
    EMPI_GLOBAL_gamma = 2.0E-9;
    
    EMPI_GLOBAL_PAPI_numevents            = 0;
    
    EMPI_GLOBAL_PAPI_real_flops_iteration = 0;
    EMPI_GLOBAL_debug_comms                  = 0;
    EMPI_GLOBAL_flag_dynamic              = 1;
    EMPI_GLOBAL_energy_eficiency_op_mode  =-1;
    EMPI_GLOBAL_flag_enter                = 1;

    //EMPI_GLOBAL_ENERGY_overflow_counter_1 = 0;
    //EMPI_GLOBAL_ENERGY_overflow_counter_2 = 0;

    EMPI_GLOBAL_non_exclusive              = 1;

    flag = 0;

    EMPI_GLOBAL_perform_load_balance      = 0;
    EMPI_GLOBAL_spawn_data.dirty          = 0;
    EMPI_GLOBAL_spawn_data.hostid         = NULL;
    EMPI_GLOBAL_spawn_data.nprocs         = NULL;
    //EMPI_GLOBAL_spawn_data.name         = NULL;
    
    EMPI_array_alloc                      = 0;

    //ALBERTO
    ADM_GLOBAL_checkpointing = 0;
    ADM_GLOBAL_waiting_decision = 0; 
    ADM_GLOBAL_shrink = 0;
    ADM_GLOBAL_nnodes = 0;
    ADM_APP_TYPE=0; // Flexmpi
    MAX_LENGTH_NODELIST = 128;
    /*ADM_GLOBAL_nodelist = (char*)calloc(MAX_LENGTH_NODELIST, sizeof(char));   UNUSED */
   
    // We need to protect it because the server access to EMPI_GLOBAL_PAPI_nhwpc_* when option 7 is used
    pthread_mutex_lock(&EMPI_GLOBAL_server_lock); // Only the server has an attached thread
    // Default values for hwpc_1 and hwpc_2 hardware counters
    sprintf(EMPI_GLOBAL_PAPI_nhwpc_1,"PAPI_SP_OPS");
    sprintf(EMPI_GLOBAL_PAPI_nhwpc_2,"PAPI_TOT_CYC");    
    strncpy(EMPI_GLOBAL_monitoring_data.nhwpc_1,EMPI_GLOBAL_PAPI_nhwpc_1,EMPI_Monitor_string_size);
    strncpy(EMPI_GLOBAL_monitoring_data.nhwpc_2,EMPI_GLOBAL_PAPI_nhwpc_2,EMPI_Monitor_string_size);
    EMPI_GLOBAL_monitoring_data.termination=0;                // 0 means no termination
    EMPI_GLOBAL_corebinding=0;                                // Sets no core binding as default

    sprintf(EMPI_GLOBAL_controller,"NULL");                   // Default name of the external server
    strcpy(EMPI_GLOBAL_application,*argv[0]);                 // Name of the application
    
    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);

    
    // No process-core binding
    EMPI_GLOBAL_monitor.corebinding=0;


    // Generates a Datatype for the monitor struct
    //displacements
    MPI_Get_address (&EMPI_GLOBAL_monitor, &start_address);
    MPI_Get_address (&(EMPI_GLOBAL_monitor.count), &address);
    displs[0] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.rtime), &address);
    displs[1] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.ptime), &address);
    displs[2] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.ctime), &address);
    displs[3] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.flops), &address);
    displs[4] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.hostid), &address);
    displs[5] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.flops_iteration), &address);
    displs[6] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.it_time), &address);
    displs[7] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.hwpc_1), &address);
    displs[8] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.hwpc_2), &address);
    displs[9] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.nhwpc_1), &address);
    displs[10] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.nhwpc_2), &address);
    displs[11] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.corebinding), &address);
    displs[12] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.termination), &address);
    displs[13] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.lbalance), &address);
    displs[14] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.iotime), &address);
    displs[15] = address - start_address;
    MPI_Get_address (&(EMPI_GLOBAL_monitor.EMPI_array_alloc), &address);
    displs[16] = address - start_address;
    
    //MPI datatype
    MPI_Type_create_struct (17, blocklen, displs, EMPI_monitor_struct_type, &EMPI_Monitor_Datatype); //MPI_Type_struct
    MPI_Type_commit (&EMPI_Monitor_Datatype);

    // MPI_Type_create_struct (2, blocklen, displs, EMPI_monitor_struct_type, &EMPI_Monitor_Datatype);
    // MPI_Type_commit (&EMPI_Monitor_Datatype);

    //Init PAPI library
    int retval;
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if(retval != PAPI_VER_CURRENT){
        fprintf(stderr,"Error initializing library\n");
    }
    if (retval != PAPI_VER_CURRENT && retval > 0) {
          fprintf(stderr,"PAPI library version mismatch!\n");
          exit(1);
    }

    if (retval < 0) {
          fprintf(stderr, "Initialization error!\n");
          exit(1);
    }

    //set process status
    EMPI_GLOBAL_status = EMPI_ACTIVE;

    // CHANGE TEMP
    // init malleability reconfig data
    reconfig_data_t aux_reconfig_data = RECONFIG_DATA_DEFAULT;
    EMPI_GLOBAL_reconfig_data = aux_reconfig_data;
    // END CHANGE TEMP

    //debug
    #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
        fprintf (stderr, "\n*** DEBUG_MSG::call::EMPI_Parse_options in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
    #endif
    
    // Manages the input argument:
    for(i=0;i<*argc;i++){
        if(strcmp((*argv)[i],"-alloc:1")==0){
            EMPI_alloc();
        }
    }

    //Parse EMPI global options
    //PTR_EMPI_COMM_WORLD = (MPI_Comm *)malloc(sizeof(MPI_Comm));
    //MPI_Comm_dup (MPI_COMM_WORLD, PTR_EMPI_COMM_WORLD);
    EMPI_Parse_options (*argc, *argv);
    //MPI_Comm_disconnect (PTR_EMPI_COMM_WORLD);
    //EMPI_COMM_WORLD = MPI_COMM_NULL;


    // CHANGE: START_NEWVER
    int spawnedProcess;
    if  (EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY_TRIG) {
        fprintf (stderr, "EMPI_Init(%d): EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY_TRIG\n",getpid());
        //spawnedProcess = EMPI_Spawn_Init_Orig ();
        spawnedProcess = EMPI_Spawn_Init ((*argv), &EMPI_GLOBAL_reconfig_data);
    } else {
        spawnedProcess = EMPI_Spawn_Init_Orig ();
    }
    // CHANGE: END_NEWVER

    // Spawned process
    if (spawnedProcess == 1) {
        
        int *countdispl = NULL;

        //Process type
        EMPI_GLOBAL_type = EMPI_SPAWNED;


        // CHANGE
        //debug
        //#if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
        //    fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Intercomm_merge in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        //#endif
        //Merge intercommunicator
        //err = MPI_Intercomm_merge (parentcomm, 1, PTR_EMPI_COMM_WORLD);
        //if (err) fprintf (stderr, "Error in MPI_Intercomm_merge\n");

        //debug
        //#if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
        //    fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Comm_disconnect in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        //#endif

        //Disconnect aux communicator
        //err = MPI_Comm_disconnect (&parentcomm);
        //if (err) fprintf (stderr, "Error in MPI_Comm_disconnect\n");
        // END CHANGE

        //Recv rprocs, minprocs, iteration and hostid. The MPI_Send command is executed in EMPI_Spawn function. 
        PMPI_Recv (rdata, 4, MPI_INT, EMPI_root, tag, EMPI_COMM_WORLD, &mpi_status);
        
        //Set rprocs, minprocs, iteration and hostid
        rprocs = rdata[0];
        EMPI_GLOBAL_minprocs = rdata[1];
        EMPI_GLOBAL_iteration = rdata[2];
        EMPI_GLOBAL_hostid = rdata[3];

        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stderr, "\n*** DEBUG_MSG::call::EMPI_Spawn in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif

        MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
        MPI_Comm_size (EMPI_COMM_WORLD, &size);

        EMPI_GLOBAL_vcounts = (int*) calloc (size, sizeof (int));
        assert (EMPI_GLOBAL_vcounts);

        EMPI_GLOBAL_displs = (int*) calloc (size, sizeof (int));
        assert (EMPI_GLOBAL_displs);

        if (EMPI_GLOBAL_mpolicy != EMPI_NULL) {

            // CHANGE: JAVI
            int sync_data;
            fprintf(stderr, "FLEXMPI_INIT_ss(%d): PMPI_Bcast sync_data\n",getpid());
            PMPI_Bcast (&sync_data, 1, MPI_INT, EMPI_root, EMPI_COMM_WORLD);
            if (sync_data == 1) {
                // END CHANGE: JAVI
                
                countdispl = (int*) malloc ((size*2) * sizeof(int));
                assert (countdispl);
                
                //Recv vcounts and displs
                PMPI_Bcast (countdispl, (size*2), MPI_INT, EMPI_root, EMPI_COMM_WORLD);
                
                //memcpy
                memcpy (EMPI_GLOBAL_vcounts, countdispl, size*sizeof(int));
                memcpy (EMPI_GLOBAL_displs, countdispl+size, size*sizeof(int));
                
                free (countdispl);
            // CHANGE: JAVI
            }
            // END CHANGE: JAVI
        }

        //set dynamic workload
        EMPI_GLOBAL_wpolicy = EMPI_DYNAMIC;

    } else { //Native process

        //Initialize global variables
        EMPI_GLOBAL_nhosts = 0;

        //Process type
        EMPI_GLOBAL_type = EMPI_NATIVE;

        //Set minprocs
        MPI_Comm_rank (MPI_COMM_WORLD, &rank);
        MPI_Comm_size (MPI_COMM_WORLD, &size);

        //Initialize number of procs
        EMPI_GLOBAL_minprocs = size;
        EMPI_GLOBAL_maxprocs = 0;

        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stderr, "\n*** DEBUG_MSG::call::EMPI_Parse_cfile in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif

        //Parse cfile with the node list
        //EMPI_Parse_cfile (*argc, *argv, &init_hostlist);
        //Create system table
        //EMPI_Create_system_classes ();

        // fillup hostlist ands init_hostlist
        EMPI_fillup_init_hostlist ();
        get_init_hostlist (&init_hostlist);

        // CHANGE
        //MPI_Comm_dup(MPI_COMM_WORLD, &EMPI_GLOBAL_comm_energy);
        //MPI_Comm_rank (EMPI_GLOBAL_comm_energy, &energy_rank);
        // END CHANGE
        
        //gerhostname(host_name, host_name_length);
        MPI_Get_processor_name (EMPI_GLOBAL_host_name, &host_name_length);

    }
    
     // Configures the socket for controller communication
    if (rank == EMPI_root)
    {
        if((EMPI_GLOBAL_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
            fprintf(stderr, "Error creating socket \n");
        }
        
        memset((char *) &si_me, 0, sizeof(si_me));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(EMPI_GLOBAL_recvport+200);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(EMPI_GLOBAL_socket, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
        {
            fprintf(stderr, "Error binding socket \n");
        }
    
        EMPI_GLOBAL_controller_addr.sin_family = AF_INET;
        EMPI_GLOBAL_controller_addr.sin_port = htons(EMPI_GLOBAL_sendport);

        if ((he = gethostbyname(EMPI_GLOBAL_controller) ) == NULL ) {
            fprintf(stderr, "Error resolving host name \n");
        }
                    
        EMPI_GLOBAL_controller_addr.sin_addr=*(struct in_addr *) he->h_addr;
        memset(EMPI_GLOBAL_controller_addr.sin_zero, '\0', sizeof(EMPI_GLOBAL_controller_addr.sin_zero));  
    }   

    if (rank == EMPI_root) {

        //track FLOPS
        EMPI_GLOBAL_track_flops = (long long*) malloc (2 * sizeof(long));
        assert (EMPI_GLOBAL_track_flops);

        //track RTIME
        EMPI_GLOBAL_track_rtime = (long long*) malloc (2 * sizeof(long));
        assert (EMPI_GLOBAL_track_rtime);

        //set initial track
        EMPI_GLOBAL_track_flops[0] = -1;
        EMPI_GLOBAL_track_flops[1] = -1;

        EMPI_GLOBAL_track_rtime[0] = -1;
        EMPI_GLOBAL_track_rtime[1] = -1;
    }

    if ((EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY)||(EMPI_GLOBAL_mpolicy == EMPI_MALLEABILITY_COND)) EMPI_Parse_malleability ();

    //FIXME: hay que enviarlo a los spawned procs.
    //Historical monitoring
    EMPI_GLOBAL_hmon = (int*) malloc (EMPI_GLOBAL_hsteps * sizeof(int));
    assert (EMPI_GLOBAL_hmon);

    for (n = 0; n < EMPI_GLOBAL_hsteps; n ++) EMPI_GLOBAL_hmon[n] = EMPI_DEDICATED;


    //EMPI_GLOBAL_power_monitoring_data = calloc(EMPI_GLOBAL_nhosts, sizeof(double));
    // fprintf(stderr, "minprocs = %d,  nhosts = %d\n", EMPI_GLOBAL_minprocs, EMPI_GLOBAL_nhosts);
    EMPI_GLOBAL_power_monitoring_data = calloc(EMPI_GLOBAL_minprocs, sizeof(double));
    

    
    if (rank == EMPI_root)
    {
        int rc;  // return code
        //pthread_t thread, rpc_thread; // CHANGE FINI
        pthread_attr_t attr, attr2; // CHANGE FINI
        rpc_thread_flag = 1; // CHANGE FINI


        if (pthread_mutex_init(&EMPI_GLOBAL_server_lock, NULL) != 0)
        {
            fprintf(stderr, "\n mutex init failed\n");
        }

        rc = pthread_attr_init(&attr);
        check_posix_return(rc, "Initializing attribute");

        rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        check_posix_return(rc, "Setting detached state");

        rc = pthread_create(&thread, &attr, (void*)&command_listener, NULL);
        check_posix_return(rc, "Creating socket listener");
        
        // CHANGE BEGIN
        rc = pthread_attr_init(&attr2); // CHANGE FINI
        rc = pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_JOINABLE); // CHANGE FINI

        rc = pthread_create(&rpc_thread, &attr2, (void*)&thread_rpc_release_nodes, NULL); // CHANGE FINI
        check_posix_return(rc, "Creating thread_rpc_release_nodes");
        signal_thread_rpc_init_ss(init_hostlist); // CHANGE: JAVI
        // CHANGE END
    }

    // deallocate added_hostlist and init_hostlist
    free(init_hostlist); // CHANGE: JAVI

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Init in <%s> ***\n", __FILE__);
    #endif
    
    // run ADM_Init
    int world_rank, world_size;
    MPI_Comm_rank(EMPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(EMPI_COMM_WORLD, &world_size);
    ADM_Init (*argc, *argv, world_rank, world_size);

    
    return MPI_SUCCESS;
}

/****************************************************************************************************************************************
*
*    'MPI_Finalize'
*
****************************************************************************************************************************************/
int MPI_Finalize (void) {
    return FLEXMPI_Finalize();
}

/****************************************************************************************************************************************
*
*    'FLEXMPI_Finalize'
*
****************************************************************************************************************************************/
int FLEXMPI_Finalize (void) {
    
    static int done = 0;

    if (done == 1) {
        fprintf (stderr, "MPI_Finalize(%d): nothing to do\n", getpid());
        return MPI_SUCCESS;
    }
    done = 1;
    
    fprintf (stderr, "MPI_Finalize(%d): START\n", getpid());

    // stop limitless script
    system ("limitless.sh stop");

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Finalize in <%s> ***\n", __FILE__);
    #endif

    int err,eventcode_hwpc_1,eventcode_hwpc_2,active;
    long long values[3] = {0, 0, 0};
    char socketcmd[1024]; 

    char nodename[1024],command[2048];
    int nodename_len;
    
    // CHANGE: remove sockert for now
    // Sends the termination command to the controller
//    if(EMPI_GLOBAL_socket!=-1){  // Only rank=0 process has a value != -1
//        fprintf (stderr, "No enter (socket)\n");
//
//        MPI_Get_processor_name(nodename,&nodename_len);
//        sprintf(command,"nping --udp -g 5000 -p %d  -c 1 %s --data-string \"%s\">/dev/null",EMPI_GLOBAL_recvport,nodename,"4:on");
//        fprintf(stderr, "\n sending %s \n",command);
//        system(command);
//        sleep(30);
//        sprintf(command,"nping --udp -g 5000 -p %d  -c 1 %s --data-string \"%s\">/dev/null",EMPI_GLOBAL_recvport,nodename,"5:");
//        fprintf(stderr, "\n sending %s \n",command);
//        system(command);
//        sleep(120);
//        //sprintf(socketcmd,"Application terminated");
//        //sendto(EMPI_GLOBAL_socket,socketcmd,strlen(socketcmd),0,(struct sockaddr *)&EMPI_GLOBAL_controller_addr,sizeof(EMPI_GLOBAL_controller_addr));
//    }
    // CHANGE END: remove sockert for now

    // Detects if monitoring thread is active
    pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
    active=EMPI_GLOBAL_posteractive;
    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
    
    // If there is a monitoring thread (server.c), then activates termination in order to send the termination message
    if(active==1){
        pthread_mutex_lock(&EMPI_GLOBAL_server_lock); // Only the server has an attached thread
        EMPI_GLOBAL_monitoring_data.termination=1; // 1 means program has started the termination
        pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
    }
    
    // If the the monitoring thread is active, do an active wait until it completes the execution 
    while(active==1){
        pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
        active=EMPI_GLOBAL_posteractive; // Detects if monitoring thread is active
        pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
        sleep(1);
    }
        
//    if ((EMPI_GLOBAL_mpolicy == EMPI_LBALANCE) || (EMPI_GLOBAL_lbalance == EMPI_TRUE)) {
//        fprintf (stderr, "MPI_Finalize(%d): Stop PAPI events\n", getpid());
//
//        //PAPI stop
//        PAPI_stop (EMPI_GLOBAL_PAPI_eventSet, values);
//
//        //PAPI remove event
//        PAPI_remove_event (EMPI_GLOBAL_PAPI_eventSet, PAPI_FP_OPS);
//
//        //PAPI remove event
//        PAPI_event_name_to_code(EMPI_GLOBAL_PAPI_nhwpc_2, &eventcode_hwpc_1 );
//        PAPI_remove_event (EMPI_GLOBAL_PAPI_eventSet, eventcode_hwpc_1);
//
//        //PAPI remove event
//        PAPI_event_name_to_code(EMPI_GLOBAL_PAPI_nhwpc_2, &eventcode_hwpc_2 );
//        PAPI_remove_event (EMPI_GLOBAL_PAPI_eventSet, eventcode_hwpc_2);
//
//        fprintf (stderr, "MPI_Finalize(%d): Stop PAPI events DONE\n", getpid());
//    }

    //PAPI shutdown
    PAPI_shutdown();
    
    //memory free
    free (EMPI_GLOBAL_hmon);

    if (EMPI_GLOBAL_vcounts != NULL) free (EMPI_GLOBAL_vcounts);
    if (EMPI_GLOBAL_displs != NULL) free (EMPI_GLOBAL_displs);

    if (EMPI_GLOBAL_track_flops != NULL) free (EMPI_GLOBAL_track_flops);
    
    //destroy data structures
    EMPI_Destroy_data_structure ();

    //destroy comms data structure
    EMPI_Destroy_comms ();

    //destroy system classes data structure
    EMPI_Destroy_system_classes ();

    if (EMPI_COMM_WORLD != MPI_COMM_NULL) {

        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Comm_disconnect in line %d function MPI_Finalize in <%s> *\n", __LINE__, __FILE__);
        #endif

        //Disconnect communicator (NOTE: should not be needed)
        err = MPI_Comm_disconnect (PTR_EMPI_COMM_WORLD);
        if (err) fprintf (stderr, "Error in MPI_Comm_disconnect\n");
    }

    //Global tcomp
    EMPI_GLOBAL_tcomp_fin = MPI_Wtime();

    //debug
    #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
        fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Finalize in line %d function EMPI_Finalize in <%s> ***\n", __LINE__, __FILE__);
    #endif

    //Finalize MPI environment
    err = PMPI_Finalize ();
    if (err != MPI_SUCCESS) return MPI_ERR_OTHER;

    // CHANGE: FINI
    if (rpc_thread_flag == 1){
      // end icc conexion
      signal_thread_rpc_icc_fini();
      //wait for the thread to finish after ending conexion
      fprintf (stderr, "MPI_Finalize(%d): join rpc_thread\n", getpid());
      pthread_join(rpc_thread,NULL);
    }
    // END CHANGE: FINI

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Finalize in <%s> ***\n", __FILE__);
    #endif
    fprintf (stderr, "MPI_Finalize(%d): END\n", getpid());

    return MPI_SUCCESS;
}

/* Finalize for stop and start applications */
int FLEXMPI_Finalize_ss (void) {
    
    static int done = 0;

    if (done == 1) {
        fprintf (stderr, "MPI_Finalize(%d): nothing to do\n", getpid());
        return MPI_SUCCESS;
    }
    done = 1;
    
    fprintf (stderr, "MPI_Finalize(%d): START\n", getpid());

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Finalize in <%s> ***\n", __FILE__);
    #endif

    int err,eventcode_hwpc_1,eventcode_hwpc_2,active;
    long long values[3] = {0, 0, 0};
    char socketcmd[1024]; 

    char nodename[1024],command[2048];
    int nodename_len;
    

    // Detects if monitoring thread is active
    pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
    active=EMPI_GLOBAL_posteractive;
    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
    
    // If there is a monitoring thread (server.c), then activates termination in order to send the termination message
    if(active==1){
        pthread_mutex_lock(&EMPI_GLOBAL_server_lock); // Only the server has an attached thread
        EMPI_GLOBAL_monitoring_data.termination=1; // 1 means program has started the termination
        pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
    }
    
    // If the the monitoring thread is active, do an active wait until it completes the execution 
    while(active==1){
        pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
        active=EMPI_GLOBAL_posteractive; // Detects if monitoring thread is active
        pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
        sleep(1);
    }
  
    //PAPI shutdown
    PAPI_shutdown();
    
    //memory free
    free (EMPI_GLOBAL_hmon);

    if (EMPI_GLOBAL_vcounts != NULL) free (EMPI_GLOBAL_vcounts);
    if (EMPI_GLOBAL_displs != NULL) free (EMPI_GLOBAL_displs);

    if (EMPI_GLOBAL_track_flops != NULL) free (EMPI_GLOBAL_track_flops);
    
    //destroy data structures
    EMPI_Destroy_data_structure ();

    //destroy comms data structure
    EMPI_Destroy_comms ();

    //destroy system classes data structure
    EMPI_Destroy_system_classes ();

    if (EMPI_COMM_WORLD != MPI_COMM_NULL) {

        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Comm_disconnect in line %d function MPI_Finalize in <%s> *\n", __LINE__, __FILE__);
        #endif

        //Disconnect communicator (NOTE: should not be needed)
        err = MPI_Comm_disconnect (PTR_EMPI_COMM_WORLD);
        if (err) fprintf (stderr, "Error in MPI_Comm_disconnect\n");
    }

    //Global tcomp
    EMPI_GLOBAL_tcomp_fin = MPI_Wtime();

    //debug
    #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
        fprintf (stderr, "\n*** DEBUG_MSG::call::MPI_Finalize in line %d function EMPI_Finalize in <%s> ***\n", __LINE__, __FILE__);
    #endif

    //Finalize MPI environment
    err = PMPI_Finalize ();
    if (err != MPI_SUCCESS) return MPI_ERR_OTHER;

    // CHANGE: FINI
    if (rpc_thread_flag == 1){
      // end icc conexion
      signal_thread_rpc_icc_fini();
      //wait for the thread to finish after ending conexion
      fprintf (stderr, "MPI_Finalize(%d): join rpc_thread\n", getpid());
      pthread_join(rpc_thread,NULL);
    }
    // END CHANGE: FINI

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Finalize in <%s> ***\n", __FILE__);
    #endif
    fprintf (stderr, "MPI_Finalize(%d): END\n", getpid());

    /* TODO: Remove IC data from this job instead of doing it in the application */

    return MPI_SUCCESS;
}


/****************************************************************************************************************************************
*
*    'EMPI_Get_wsize'
*
****************************************************************************************************************************************/
void EMPI_Get_wsize    (int rank, int size, int dim, int *desp, int *count, int *vcounts, int *displs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Get_wsize in <%s> ***\n", __FILE__);
    #endif

    int rest = 0, n = 0, aux = 0, status;

    EMPI_Get_status (&status);

    if (EMPI_GLOBAL_wpolicy == EMPI_STATIC) {
        //static workload partitions

        //set desp and count
        if (status == EMPI_REMOVED) {

            *desp = -1;
            *count = -1;

        } else if (size > 1) {

            *count = (dim / size);
            rest = dim - ((*count) * size);

            if ((rest > 0)&&(rank >= (size-rest))) *count = *count + 1;

            if (rank >= (size-rest)) {

                *desp = ((size-rest) * (*count-1)) + ((rank-(size-rest)) * (*count));

            } else     if (rank < (size-rest)) {

                    *desp = (rank * (*count));
            }

        } else if (size == 1) {

            *desp = 0;
            *count = dim;
        }

        //set vcounts and displs arrays
        if ((vcounts != NULL) && (displs != NULL)) {

            aux = (dim / size);
            rest = dim - (aux * size);

            for (n = 0; n < size; n ++) vcounts[n] = aux;

            for (n = (size-rest); n < size; n ++) vcounts[n] = vcounts[n] + 1;

            displs[0] = 0;

            for (n = 1; n < size; n ++) displs[n] = displs[n-1] + vcounts[n-1];
        }

    } else if (EMPI_GLOBAL_wpolicy == EMPI_DYNAMIC) {

        //dynamic workload partitions
        if (status == EMPI_REMOVED) {

            *count = -1;
            *desp = -1;

        } else {

            *count = EMPI_GLOBAL_vcounts[rank];
            *desp = EMPI_GLOBAL_displs[rank];

            //set vcounts and displs arrays
            if ((vcounts != NULL) && (displs != NULL)) {

                memcpy (vcounts, EMPI_GLOBAL_vcounts, size*sizeof(int));
                memcpy (displs, EMPI_GLOBAL_displs, size*sizeof(int));
            }
        }
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Get_wsize in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Get_status'
*
****************************************************************************************************************************************/
void EMPI_Get_status (int *status) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Get_status in <%s> ***\n", __FILE__);
    #endif

    *status = EMPI_GLOBAL_status;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Get_status in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Get_type'
*
****************************************************************************************************************************************/
void EMPI_Get_type (int *type) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Get_type in <%s> ***\n", __FILE__);
    #endif

    *type = EMPI_GLOBAL_type;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Get_type in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Set_type'
*
****************************************************************************************************************************************/
void EMPI_Set_type (int type) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Set_type in <%s> ***\n", __FILE__);
    #endif

    EMPI_GLOBAL_type = type;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Set_type in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Parse_cfile
 '
*
****************************************************************************************************************************************/
// CHANGE: JAVI
//static void EMPI_Parse_cfile (int argc, char **argv) {
static void EMPI_Parse_cfile (int argc, char **argv, char **init_hostlist) {
// END CHANGE: JAVI

    char *saveptr;
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Parse_cfile in <%s> ***\n", __FILE__);
    #endif

    //Core file format
    //hostname:maxprocs:class:cost:mflops

    int n, machine = EMPI_FALSE, length, hostname = EMPI_FALSE, rank, size, tag = 989, class_found = 0;

    char cfile[EMPI_MAX_FILENAME], readline[EMPI_MAX_LINE_LENGTH], token[] = ":", *record = NULL, *chostname = NULL;

    EMPI_host_type *hostlist = NULL, *aux = NULL;

    MPI_Status status;

    FILE *file;

    // CHANGE: JAVI
    //check and init init_hostlist
    assert(init_hostlist != NULL);
    (*init_hostlist) = NULL;
    // END CHANGE: JAVI

    //Get rank and size
    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);

    //Get cfile name
    for (n = 0; n < argc; n ++) {

        if ((strcmp(argv[n], "-cfile") == 0) && ((n+1) < argc)) {

            //get file name
            strcpy (cfile, argv[n+1]);

            //cfile provided
            machine = EMPI_TRUE;

        } if ((strcmp(argv[n], "-cfile") == 0) && ((n+1) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_cfile getting file name\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            MPI_Abort (EMPI_COMM_WORLD, -1);
        }
    }

    if (machine == EMPI_TRUE) {

        //parse cfile
        if ((file = fopen (cfile, "r")) == NULL) {

            fprintf (stderr, "\nError in EMPI_Parse_cfile file opening\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            MPI_Abort (EMPI_COMM_WORLD, -1);
        }

        if (rank == EMPI_root) {

            //read cfile
            while (fscanf (file, "%s\n", readline) != EOF) {

                aux = EMPI_GLOBAL_hostlist;

                hostname = EMPI_FALSE;

                //Get hostname
                record = strtok_r (readline, token, &saveptr);
                //find repeated hostname
                while (aux != NULL) {

                    if (strcmp (aux->hostname, record) == 0) hostname = EMPI_TRUE;

                    aux = aux->next;
                }
                
                if (hostname == EMPI_FALSE) {

                    if (hostlist == NULL) {

                        hostlist = (EMPI_host_type*) malloc (sizeof(EMPI_host_type));
                        assert (hostlist);

                        //set pointer
                        EMPI_GLOBAL_hostlist = hostlist;

                    } else {

                        hostlist->next = (EMPI_host_type*) malloc (sizeof(EMPI_host_type));
                        assert (hostlist->next);

                        hostlist = hostlist->next;
                    }

                    //Set host id
                    hostlist->id = EMPI_GLOBAL_nhosts;

                    //number of hosts
                    EMPI_GLOBAL_nhosts ++;

                    //Set nprocs
                    if (hostlist->id == 0) hostlist->nprocs = 0;
                    else hostlist->nprocs = 0;

                    hostlist->iniprocs = hostlist->nprocs;

                    //Set hostname
                    strcpy (hostlist->hostname, record);

                    //Get maxprocs
                    record = strtok_r (NULL, token, &saveptr);
                    if (record != NULL)    hostlist->maxprocs = atoi(record);
                    else hostlist->maxprocs = EMPI_MAX_NPROCS;

                    fprintf(stderr, "EMPI_Parse_cfile(%d): Host %s maxprocs is %d. ID is %d\n",getpid(), hostlist->hostname, hostlist->maxprocs, hostlist->id);

                    //Get class
                    record = strtok_r (NULL, token, &saveptr);
                    if (record != NULL)    strcpy (hostlist->hclass, record);

                    //Check host class
                    for (class_found = 0, n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

                        if (strcmp (EMPI_GLOBAL_hclasses[n], hostlist->hclass) == 0) {

                            class_found = 1;

                            //set host class id
                            hostlist->idclass = n;
                        }
                    }

                    if (class_found == 0) {

                        //create new class
                        strcpy (EMPI_GLOBAL_hclasses[EMPI_GLOBAL_nhclasses], hostlist->hclass);
                        
                        //set host class id
                        hostlist->idclass = EMPI_GLOBAL_nhclasses;
                        EMPI_GLOBAL_nhclasses ++;
                    }

                    //Get cost
                    record = strtok_r (NULL, token, &saveptr);
                    if (record != NULL)    hostlist->cost = atof(record);
                    else hostlist->cost = 0.0; //default cost

                    //Get mflops
                    record = strtok_r (NULL, token, &saveptr);
                    if (record != NULL)    hostlist->mflops = atoi(record);
                    else hostlist->mflops = -1; //default mflops

                    hostlist->next = NULL;

                    EMPI_GLOBAL_maxprocs += hostlist->maxprocs;
                }
            }

            aux = NULL;

            //set root hostid
            EMPI_GLOBAL_hostid = 0;

            for (n = 1; n < size; n ++) {

                //pointer to global variable
                hostlist = EMPI_GLOBAL_hostlist;

                chostname = (char *) malloc (MPI_MAX_PROCESSOR_NAME * sizeof (char));

                //receive hostname from n
                PMPI_Recv (chostname, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, n, tag, EMPI_COMM_WORLD, &status);

                //find hostname in hostlist
                while (hostlist != NULL) {

                    if (strcmp (hostlist->hostname, chostname) == 0) {

                        hostlist->nprocs = hostlist->nprocs + 1;
                        hostlist->iniprocs = hostlist->iniprocs + 1;

                        //send hostid
                        PMPI_Send (&hostlist->id, 1, MPI_INT, n, tag, EMPI_COMM_WORLD);

                        hostlist = NULL;

                    } else {

                        hostlist = hostlist->next;
                    }
                    //fprintf(stderr, "Host %s maxprocs is %d\n", hostlist->hostname, hostlist->maxprocs);

                }

                free (chostname); chostname = NULL;
            }


            // CHANGE: JAVI
            fprintf(stderr, "EMPI_Parse_cfile(%d): Begin  init_hostlist\n",getpid());
            // allocate init_hostlist memory
            (*init_hostlist) = (char *)calloc(size*100,1);
            char *aux_hostlist = (char *)calloc(size*100,1);
            assert ((*init_hostlist));
            assert (aux_hostlist);

            //pointer to global variable
            hostlist = EMPI_GLOBAL_hostlist;
            //find hostname with processes in hostlist
            while (hostlist != NULL) {
                if (hostlist->iniprocs != 0) {
                    if (strcmp((*init_hostlist),"") == 0) {
                        sprintf(aux_hostlist, "%s:%d",hostlist->hostname, hostlist->iniprocs+1);
                    } else {
                        sprintf(aux_hostlist, "%s,%s:%d", (*init_hostlist), hostlist->hostname, hostlist->iniprocs);
                    }
                    //swap pointers
                    char *aux = (*init_hostlist);
                    (*init_hostlist) = aux_hostlist;
                    aux_hostlist = aux;
                }
                hostlist = hostlist->next;
            }
            free(aux_hostlist);
            fprintf(stderr, "EMPI_Parse_cfile(%d): End  init_hostlist ~ %s\n",getpid(), *init_hostlist);
            // END CHANGE: JAVI
        } else {

            chostname = (char *) malloc (MPI_MAX_PROCESSOR_NAME * sizeof (char));

            //get hostname
            MPI_Get_processor_name (chostname, &length);

            //send hostname to root
            PMPI_Send (chostname, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, EMPI_root, tag, EMPI_COMM_WORLD);

            //receive hostid
            PMPI_Recv (&EMPI_GLOBAL_hostid, 1, MPI_INT, EMPI_root, tag, EMPI_COMM_WORLD, &status);

            free (chostname);
        }

    } else {

        if (rank == EMPI_root) {

            hostlist = (EMPI_host_type*) malloc (sizeof(EMPI_host_type));
            assert (hostlist);

            //set pointer
            EMPI_GLOBAL_hostlist = hostlist;

            EMPI_GLOBAL_nhosts = 1;

            //cfile not provided
            hostlist->id = 0;
            hostlist->nprocs = size;
            hostlist->iniprocs = size;
            MPI_Get_processor_name (hostlist->hostname, &length);
            hostlist->maxprocs = EMPI_MAX_NPROCS;
            hostlist->next = NULL;
            hostlist->cost = 0.0;
            hostlist->idclass = 0;
            strcpy (hostlist->hclass, "classA");
            strcpy (EMPI_GLOBAL_hclasses[hostlist->idclass], "classA");
            //default mflops
            hostlist->mflops = -1;

            EMPI_GLOBAL_nhclasses ++;

            EMPI_GLOBAL_maxprocs += hostlist->maxprocs;

            hostlist = NULL;
        }

        //set hostid
        EMPI_GLOBAL_hostid = 0;
    }

    //Bcast maxprocs
    PMPI_Bcast (&EMPI_GLOBAL_maxprocs, 1, MPI_INT, EMPI_root, EMPI_COMM_WORLD);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Parse_cfile in <%s> ***\n", __FILE__);
    #endif
}

// CHANGE: JAVI

/****************************************************************************************************************************************
*
*    'EMPI_fillup_init_hostlist'
*
****************************************************************************************************************************************/
// CHANGE: JAVI
static void EMPI_fillup_init_hostlist () {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Parse_cfile in <%s> ***\n", __FILE__);
    #endif
    int rank, size;
    int tag = 989;
    MPI_Status status;

    //Get rank and size
    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);

    if (rank == EMPI_root) {
        int length = 0;
        char hostname[MPI_MAX_PROCESSOR_NAME];
    
        //get hostname
        MPI_Get_processor_name (hostname, &length);
        assert (MPI_MAX_PROCESSOR_NAME>length);
        
        // add host and its class
        int hostid = add_host (hostname, 1, 1, 0, hostname);
        assert (hostid == 0);
        //set root hostid
        EMPI_GLOBAL_hostid = 0;
        
        for (int n = 1; n < size; n ++) {
            //receive hostname from n
            PMPI_Recv (hostname, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, n, tag, EMPI_COMM_WORLD, &status);

            // add host and its class
            int hostid = add_host (hostname, 1, 1, 0, hostname);
            //send hostid
            PMPI_Send (&hostid, 1, MPI_INT, n, tag, EMPI_COMM_WORLD);
        }
    } else {
        int length = 0;
        char hostname[MPI_MAX_PROCESSOR_NAME];
        //get hostname
        MPI_Get_processor_name (hostname, &length);
        assert (MPI_MAX_PROCESSOR_NAME>length);
        //send hostname to root
        PMPI_Send (hostname, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, EMPI_root, tag, EMPI_COMM_WORLD);
        //receive hostid
        PMPI_Recv (&EMPI_GLOBAL_hostid, 1, MPI_INT, EMPI_root, tag, EMPI_COMM_WORLD, &status);
    }


    //Bcast maxprocs
    PMPI_Bcast (&EMPI_GLOBAL_maxprocs, 1, MPI_INT, EMPI_root, EMPI_COMM_WORLD);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Parse_cfile in <%s> ***\n", __FILE__);
    #endif
}

// CHANGE: JAVI
/****************************************************************************************************************************************
*
*    'EMPI_Expand_processes'
*
****************************************************************************************************************************************/
static void EMPI_Expand_processes (int argc, char **argv, char **hostlist) {
    
    int numprocs = 0;
    int rank, size;
    
    // init hostlist
    assert(hostlist != NULL);
    (*hostlist) = NULL;
    
    //Get rank and size
    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);
    
    //Get number if processes and nodes name
    for (int n = 0; n < argc; n ++) {
        
        // check params are allright
        if ( (strcmp(argv[n], "-addhostlist") == 0) &&
            ( ((n+1) >= argc) || (argv[n+1][0] == '-') ) ) {
            fprintf (stderr, "\nError in EMPI_Expand_processes getting -addhostlist extra parameter\n");
            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();
            MPI_Abort (EMPI_COMM_WORLD, -1);
        }
        // get list of nodes with cores
        if (strcmp(argv[n], "-addhostlist") == 0) {
            (*hostlist) = (char *)calloc(strlen(argv[n+1])+1, 1);
            assert ((*hostlist));
            strcpy ((*hostlist), argv[n+1]);
        }
    }
    
    fprintf(stderr, "EMPI_Expand_processes(%d): Hostlist %s\n",getpid(), (*hostlist));

    // End if there is no parameter
    if ((*hostlist) == NULL) {
        return;
    }
    
    // count number of colon chars for hostid size
    size_t hostid_size = 0;
    char *aux = (*hostlist);
    while(*aux) if (*aux++ == ':') ++hostid_size;
    hostid_size = hostid_size*100;
    int *hostid = (int*) calloc (hostid_size, sizeof(int));
    assert (hostid);
    
    // get number of processes and fill up hostid
    char *listaux1 = (char *)calloc(strlen((*hostlist))+1,1);
    assert (listaux1);
    char *listaux2 = (char *)calloc(strlen((*hostlist))+1,1);
    assert (listaux2);
    char *hostname = (char *)calloc(strlen((*hostlist))+1,1);
    assert (hostname);
    strcpy (listaux1, (*hostlist));
    int ret = 0;
    int count = 0;
    do {
        int value = 0;
        fprintf (stderr, "listaux1=%s\n",listaux1);
        fprintf(stderr, "EMPI_Expand_processes(%d): listaux1=%s\n",getpid(), listaux1);

        ret = sscanf(listaux1,"%[^:]:%d,%s",hostname,&value,listaux2);
        fprintf(stderr, "EMPI_Expand_processes(%d): ret=%d\n",getpid(), ret);

        if ( (ret != 2) && (ret != 3) ) {
            fprintf (stderr, "\nError in EMPI_Expand_processes: -addhostlist  extra parameter format wrong (ret=%d)(hostname=%s)\n",ret, hostname);
            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();
            MPI_Abort (EMPI_COMM_WORLD, -1);
        } else if (ret == 3) {
            char * aux;
            aux = listaux2;
            listaux2 = listaux1;
            listaux1 = aux;
        }
        numprocs = numprocs + value;
        
        if (rank == EMPI_root) {
            
            // recalculate hostid size
            if (hostid_size < numprocs) {
                hostid_size = hostid_size*2;
                hostid = (int *) realloc(hostid, hostid_size);
                assert (hostid);
            }
            // add host and its class
            fprintf(stderr, "EMPI_Expand_processes(%d): add_host begin\n",getpid());
            int id = add_host (hostname, 0, 0, value, hostname);
            for (int j=0; j<value; j++) {
                hostid[count]=id;
                count = count+1;
            }
            // change nprocs of the class
            EMPI_Class_type *class = EMPI_GLOBAL_system_classes;
            while (class != NULL) {
                if (strcmp (class->name, hostname) == 0) {
                    class->nprocs = class->nprocs + value;
                    break;
                }
                class = class->next;
            }
        }
    } while (ret == 3);
    fprintf(stderr, "EMPI_Expand_processes(%d): END ret=%d\n",getpid(), ret);

    // free variables
    free(hostname);
    free(listaux1);
    free(listaux2);
    
    for (int i=0;i<numprocs;i++) {
        fprintf(stderr, "EMPI_Expand_processes(%d): hostid[%d]=%d\n",getpid(), i, hostid[i]);
    }
    fprintf(stderr, "EMPI_Expand_processes(%d): numprocs=%d\n",getpid(), numprocs);

    // fill up reconfigure global variable
    if (rank == EMPI_root) {
        EMPI_reconfigure(0, numprocs, (*hostlist), &EMPI_GLOBAL_reconfig_data);
    }
    EMPI_broadcast_reconfigure_data(&EMPI_GLOBAL_reconfig_data);

    // spawn new processes
    EMPI_Spawn (numprocs, argv, argv[0], hostid, NULL);
    
    if (EMPI_GLOBAL_mpolicy != EMPI_NULL) {
        int sync_data = 0;
        fprintf(stderr, "EMPI_Expand_processes(%d): PMPI_Bcast sync_data\n",getpid());
        PMPI_Bcast (&sync_data, 1, MPI_INT, EMPI_root, EMPI_COMM_WORLD);
    }
    
    
    // free variables
    free(hostid);
}
// END CHANGE: JAVI

/****************************************************************************************************************************************
*
*    'EMPI_Parse_options'
*
****************************************************************************************************************************************/
static void EMPI_Parse_options (int argc, char **argv) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Parse_options in <%s> ***\n", __FILE__);
    #endif

    int n, rank;
    int policy_default = 1, lpolicy = 1, spolicy = 1, apolicy = 1, niters = 1, in_ports = 1, in_ioaction = 1, in_controller = 1, mode = 1, type = 1; //ALBERTO

    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    for (n = 0; n < argc; n ++) {

        /* ALBERTO - 09022024 - Add new param for restarting. */
        if (strcmp(argv[n], "-mode-restart") == 0) {
            fprintf (stderr, "EMPI_Parse_options: mode-restart ON\n");
            ADM_APP_MODE = 1;
    	    mode = 0;
        }
	
	    //ALBERTO
	    if(mode){
	        ADM_APP_MODE = 0;
	    }

        /* JAVIER - 01032024 - Add new param for startstop. */
        if (strcmp(argv[n], "-startstop") == 0) {
            fprintf (stderr, "EMPI_Parse_options: startstop ON\n");
            ADM_APP_TYPE = 1;
            type = 0;
        }
    
        //JAVIER
        if(type){
            ADM_APP_TYPE = 0;
        }

        //Dynamic policy options

        /*if ((strcmp(argv[n], "-policy-efficiency") == 0)&&((n+2) < argc)) {

            //Set efficiency time policy
            EMPI_Set_policy (EMPI_EFFICIENCY);

            //Set objective
            EMPI_GLOBAL_obj_texec = (atoi(argv[n+1]));

            //Set threshold
            EMPI_GLOBAL_obj_texec_threshold = (atof(argv[n+2]));

            //enable load balancing
            EMPI_Enable_lbalance ();

            n = n + 2;
        }

        if ((strcmp(argv[n], "-policy-efficiency-irregular") == 0)&&((n+2) < argc)) {

            //Set efficiency time policy
            EMPI_Set_policy (EMPI_EFFICIENCY_IRR);

            //Set objective
            EMPI_GLOBAL_percentage = (atof(argv[n+1]));

            //Set threshold
            EMPI_GLOBAL_obj_texec_threshold = (atof(argv[n+2]));

            //enable load balancing
            EMPI_Enable_lbalance ();

            EMPI_GLOBAL_self_adaptation = EMPI_TRUE;

            n = n + 2;
        }

        if ((strcmp(argv[n], "-policy-cost") == 0)&&((n+2) < argc)) {

            //Set cost time policy
            EMPI_Set_policy (EMPI_COST);

            //Set objective
            EMPI_GLOBAL_obj_texec = (atoi(argv[n+1]));

            //Set threshold
            EMPI_GLOBAL_obj_texec_threshold = (atof(argv[n+2]));

            //enable load balancing
            EMPI_Enable_lbalance ();

            n = n + 2;
        }

        if ((strcmp(argv[n], "-policy-cost-irregular") == 0)&&((n+2) < argc)) {

            //Set cost time policy
            EMPI_Set_policy (EMPI_COST_IRR);

            //Set objective
            EMPI_GLOBAL_percentage = (atof(argv[n+1]));

            //Set threshold
            EMPI_GLOBAL_obj_texec_threshold = (atof(argv[n+2]));

            //enable load balancing
            EMPI_Enable_lbalance ();

            EMPI_GLOBAL_self_adaptation = EMPI_TRUE;

            n = n + 2;
        }

        // This is a policy alternative to triggered where the system only performs load balance operations
        if (strcmp(argv[n], "-policy-lbalance") == 0) { 

            //Set load balancing policy
            EMPI_Set_policy (EMPI_LBALANCE);

            //enable load balancing
            EMPI_Enable_lbalance ();
        }

        if (strcmp(argv[n], "-policy-monitordbg") == 0) {

            //Set monitor debug
            EMPI_Set_policy (EMPI_MONITORDBG);
        }

        if (strcmp(argv[n], "-policy-malleability") == 0) {

            //Set malleability policy
            EMPI_Set_policy (EMPI_MALLEABILITY);

            //enable load balancing
            EMPI_Enable_lbalance ();
        }

        if (strcmp(argv[n], "-policy-malleability-conditional") == 0) {

            //Set malleability conditional policy
            EMPI_Set_policy (EMPI_MALLEABILITY_COND);

            //enable load balancing
            EMPI_Enable_lbalance ();
        }*/
        
        if (strcmp(argv[n], "-policy-malleability-triggered") == 0) {

            //Set malleability conditional policy
            EMPI_Set_policy (EMPI_MALLEABILITY_TRIG);

            //enable load balancing
            EMPI_Enable_lbalance ();

	    //ALBERTO
	    policy_default = 0;
        }
	
	//ALBERTO
	if(policy_default){
	     //Set malleability conditional policy
            EMPI_Set_policy (EMPI_MALLEABILITY_TRIG);

            //enable load balancing
            EMPI_Enable_lbalance ();
	}



        //Load balancing policy options
        if (strcmp(argv[n], "-lbpolicy-mflops") == 0) {

            //Set mflops policy
            EMPI_Set_lbpolicy (EMPI_LBMFLOPS);
	    lpolicy=0;
        }

        if (strcmp(argv[n], "-lbpolicy-counts") == 0) {

            //Set execution time policy
            EMPI_Set_lbpolicy (EMPI_LBCOUNTS);
	    lpolicy=0;
        }

        if (strcmp(argv[n], "-lbpolicy-static") == 0) {

            //Set execution time policy
            EMPI_Set_lbpolicy (EMPI_LBSTATIC);
	    lpolicy=0;
        }

        
        if (strcmp(argv[n], "-lbpolicy-disabled") == 0) {

            if (EMPI_GLOBAL_mpolicy != EMPI_LBALANCE) {

                //disable load balancing
                EMPI_Disable_lbalance ();

                EMPI_GLOBAL_lbalance_disabled = EMPI_TRUE;

            } else {

                fprintf (stderr, "\nError in EMPI_Parse_options: disabling lbalance with load balancing policy\n");

                //print EMPI usage
                if (rank == EMPI_root) EMPI_Print_usage ();

                //MPI_Abort (EMPI_COMM_WORLD, -1);
                MPI_Abort (MPI_COMM_WORLD, -1);
            }
	    lpolicy=0;
        }

	//ALBERTO
	if(lpolicy){
	    //Set execution time policy
            EMPI_Set_lbpolicy (EMPI_LBSTATIC);
	}


        //Spawning policy options
        if (strcmp(argv[n], "-spolicy-available") == 0) {

            //Set available nodes policy
            EMPI_GLOBAL_spolicy = EMPI_AVAIL_NODE;
	    spolicy = 0;
        }

        if (strcmp(argv[n], "-spolicy-occupied") == 0) {

            //Set occupied nodes policy
            EMPI_GLOBAL_spolicy = EMPI_OCCUP_NODE;
	    spolicy = 0;
        }

	//ALBERTO
	if(spolicy){
		//no default option
	}



        //Adaptability policy options
        if (strcmp(argv[n], "-apolicy-exhaustive") == 0) {

            EMPI_GLOBAL_Adaptability_policy = EMPI_ADAPTABILITY_EX;
	    apolicy = 0;
        }

        if (strcmp(argv[n], "-apolicy-lp") == 0) {

            EMPI_GLOBAL_Adaptability_policy = EMPI_ADAPTABILITY_LP;
	    apolicy = 0;
        }

	//ALBERTO
        if(apolicy){
                //no default option
        }


        //Global data allocation
        if ((strcmp(argv[n], "-galloc") == 0)&&((n+1) < argc)) {

            if (strcmp(argv[n+1], "rows") == 0)

                EMPI_GLOBAL_allocation = EMPI_ROWS;

            else if (strcmp(argv[n+1], "nnz") == 0)

                EMPI_GLOBAL_allocation = EMPI_NNZ;

            else if (strcmp(argv[n+1], "fcost") == 0)

                EMPI_GLOBAL_allocation = EMPI_FCOST;

            else

                EMPI_GLOBAL_allocation = EMPI_NULL; // Default EMPI_LBalance_dense

            //next option
            n ++;

        }  else if ((strcmp(argv[n], "-galloc") == 0)&&((n+1) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_options: global allocation failed\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            //MPI_Abort (EMPI_COMM_WORLD, -1);
            MPI_Abort (MPI_COMM_WORLD, -1);
        }

        //Load balancing threshold option
        if ((strcmp(argv[n], "-threshold") == 0)&&((n+1) < argc)) {

            //set threshold
            EMPI_GLOBAL_threshold = (atoi(argv[n+1]));

            //next option
            n ++;

        }  else if ((strcmp(argv[n], "-threshold") == 0)&&((n+1) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_options: threshold numerical value not given\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            //MPI_Abort (EMPI_COMM_WORLD, -1);
            MPI_Abort (MPI_COMM_WORLD, -1);
        }

        if (strcmp(argv[n], "-self-adaptation") == 0) {

            //Set self adaptation
            EMPI_GLOBAL_self_adaptation = EMPI_TRUE;
        }

        //Historical steps
        if ((strcmp(argv[n], "-hsteps") == 0)&&((n+1) < argc)) {

            //set steps
            EMPI_GLOBAL_hsteps = (atoi(argv[n+1]));

            //next option
            n ++;

        }  else if ((strcmp(argv[n], "-hsteps") == 0)&&((n+1) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_options: hsteps numerical value not given\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            //MPI_Abort (EMPI_COMM_WORLD, -1);
            MPI_Abort (MPI_COMM_WORLD, -1);
        }

        //Monitoring number of iterations options
        if ((strcmp(argv[n], "-ni") == 0)&&((n+1) < argc)) {

            //set number of iterations for monitoring
            EMPI_Set_niter (atoi(argv[n+1]));

            if (EMPI_GLOBAL_niter_lb == 10) EMPI_GLOBAL_niter_lb = EMPI_GLOBAL_niter;

            //next option
            n ++;

	    niters=0;

        }  else if ((strcmp(argv[n], "-ni") == 0)&&((n+1) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_options: number of iterations not given\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            //MPI_Abort (EMPI_COMM_WORLD, -1);
            MPI_Abort (MPI_COMM_WORLD, -1);
        }
	
	//ALBERTO
	if(niters){
	    //set number of iterations for monitoring
            EMPI_Set_niter (1);

            if (EMPI_GLOBAL_niter_lb == 10) EMPI_GLOBAL_niter_lb = EMPI_GLOBAL_niter;	
	}


        //Monitoring number of iterations options
        if ((strcmp(argv[n], "-nilb") == 0)&&((n+1) < argc)) {

            //set number of iterations for monitoring
            EMPI_GLOBAL_niter_lb = (atoi(argv[n+1]));

            //next option
            n ++;

        }  else if ((strcmp(argv[n], "-nilb") == 0)&&((n+1) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_options: number of iterations LB not given\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            //MPI_Abort (EMPI_COMM_WORLD, -1);
            MPI_Abort (MPI_COMM_WORLD, -1);
        }

        // Setting the port numbers
        if ((strcmp(argv[n], "-ports") == 0)&&((n+2) < argc)) {

            //set number of iterations for monitoring
            EMPI_GLOBAL_recvport = (atoi(argv[n+1]));
            EMPI_GLOBAL_sendport = (atoi(argv[n+2]));

            //next option
            n +=2;

	    in_ports = 0;

        }  else if ((strcmp(argv[n], "-ports") == 0)&&((n+2) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_options: value of communication ports not valid\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            //MPI_Abort (EMPI_COMM_WORLD, -1);
            MPI_Abort (MPI_COMM_WORLD, -1);
        }

	//ALBERTO
	if(in_ports){
	    //set number of iterations for monitoring
            EMPI_GLOBAL_recvport = (5000);
            EMPI_GLOBAL_sendport = (5001);
	}

        
        if ((strcmp(argv[n], "-controller") == 0) && ((n+1) < argc)) {

            //get server name
            strcpy (EMPI_GLOBAL_controller, argv[n+1]);
	    in_controller = 0;
        } 
        
        if ((strcmp(argv[n], "-controller") == 0) && ((n+1) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_options: name of the controller not provided\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            //MPI_Abort (EMPI_COMM_WORLD, -1);
            MPI_Abort (MPI_COMM_WORLD, -1);
        }

	if(in_controller){
		strcpy(EMPI_GLOBAL_controller, "localhost");
	}


        // Sets if dummy I/O is performed
        if ((strcmp(argv[n], "-IOaction") == 0)&&((n+1) < argc)) {

            //set number of iterations for monitoring
            EMPI_GLOBAL_dummyIO = (atof(argv[n+1]));

            //next option
            n +=1;

	    in_ioaction=0;
        }    

	//ALBERTO
	if(in_ioaction){
	    //set number of iterations for monitoring
            EMPI_GLOBAL_dummyIO = (1.0);
	}
    }
    
    if(rank == EMPI_root){
            if(EMPI_GLOBAL_dummyIO == -1 ) fprintf(stderr, " FlexMPI: program configured to perform parallel MPI I/O\n");
            else                           fprintf(stderr, " FlexMPI: program configured to perform dummy I/O of %f seconds\n",EMPI_GLOBAL_dummyIO);
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Parse_options in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Print_usage'
*
****************************************************************************************************************************************/
static void EMPI_Print_usage (void) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Print_usage in <%s> ***\n", __FILE__);
    #endif

    printf ("\n%s ver. %d.%s", EMPI_NAME, EMPI_VER, EMPI_SUBVER);

    printf ("\n%s\n%s", EMPI_COPYRIGHT, EMPI_INSTITUTION);

    printf ("\n\nUsage: ./exectutable [executable args] [EMPI global opts]");

    printf ("\n\nGlobal options (passed to all executables):");

    printf ("\n\n  Communication options:");

    printf ("\n\n    -ports {value1} {value2}         port values for receiving commands (value1) and sending data (value2)");

    printf ("\n\n  Corefile options:");

    printf ("\n\n    -cfile {name}                    file containing host names and cores per host");

    printf ("\n\n  Dynamic processes options (none by default):");

    printf ("\n\n    -policy-efficiency {sec} {thrsld} set efficiency policy and the performance constraint {time in seconds} {threshold in percentage}");

    printf ("\n\n    -policy-efficiency-irregular {perc} {thrsld} set efficiency policy for irregular application and the performance constraint {performance improvement in percentage} {threshold in percentage}");

    printf ("\n\n    -policy-cost {sec} {thrsld}       set cost policy and the performance constraint {time in seconds} {threshold in percentage}");

    printf ("\n\n    -policy-cost-irregular {perc} {thrsld} set cost policy for irregular application and the performance constraint {performance improvement in percentage} {threshold in percentage}");

    printf ("\n\n    -policy-energy {sec} {thrsld}     set energy policy and the performance constraint {time in seconds} {threshold in percentage}");

    printf ("\n\n    -policy-malleability             set malleability policy");

    printf ("\n\n    -policy-malleability-conditional set conditional malleability policy");

    printf ("\n\n    -policy-malleability-triggered   set triggered malleability policy");

    printf ("\n\n    -policy-lbalance                 set load balancing policy");

    printf ("\n\n  Load balancing policy options:");

    printf ("\n\n    -lbpolicy-mflops                 set MFLOP/S policy (default)");

    printf ("\n\n    -lbpolicy-counts                 set COUNT/S policy");
    
    printf ("\n\n    -lbpolicy-static                 set static block-based policy");

    printf ("\n\n    -lbpolicy-disabled               disable load balancing");

    printf ("\n\n  Adaptability policy options:");

    printf ("\n\n    -apolicy-exhaustive               set exhaustive estimation policy (default)");

    printf ("\n\n    -apolicy-lp                       set linear programming policy");

    printf ("\n\n  Spawning policy options:");

    printf ("\n\n    -spolicy-available               set available nodes policy");

    printf ("\n\n    -spolicy-occupied                set occupied nodes policy (default)");

    printf ("\n\n  Global data allocation options:");

    printf ("\n\n    -galloc                       set global data allocation (rows, nnz, or fcost)");

    printf ("\n\n  Load balancing threshold options:");

    printf ("\n\n    -threshold                       set threshold in miliseconds (default {0.15 per cent})");

    printf ("\n\n    -self-adaptation                 set self adaptation when threshold is reached (default {no})");

    printf ("\n\n    -hsteps                          set number of steps for historical monitoring (default {1})");

    printf ("\n\n  Monitor options:");

    printf ("\n\n    -ni {value}                      number of iterations of the sampling interval (default {10})");

    printf ("\n\n    -nilb {value}                    number of iterations of the sampling interval for load balancing in irregular applications (default {10})");

    printf ("\n\n    -controller {value}                  name of the host that runs the external controller (default {NULL})");

    printf ("\n\nExamples:");

    printf ("\n\n  mpiexec -np 10 -machinefile myhostfile ./myprogram -cfile mycorefile -policy-efficiency 3000 -ni 100");

    printf ("\n\n  mpiexec -np 10 -machinefile myhostfile ./myprogram -cfile mycorefile -policy-lbalance -lbpolicy-mflops -ni 300");

    printf ("\n\nSee user guide for more details.\n");

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Print_usage in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Destroy_data_structure'
*
****************************************************************************************************************************************/
static void EMPI_Destroy_data_structure (void) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Destroy_data_structure in <%s> ***\n", __FILE__);
    #endif

    //destroy data structure
    if (EMPI_GLOBAL_Data != NULL) {

        EMPI_Data_type *aux;

        aux = EMPI_GLOBAL_Data;

        while (EMPI_GLOBAL_Data != NULL) {

            aux = EMPI_GLOBAL_Data->next;

            free (EMPI_GLOBAL_Data);

            EMPI_GLOBAL_Data = aux;
        }
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Destroy_data_structure in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Destroy_comms'
*
****************************************************************************************************************************************/
void EMPI_Destroy_comms (void) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Destroy_comms in <%s> ***\n", __FILE__);
    #endif

    //destroy comms data structure
    if (EMPI_GLOBAL_comms != NULL) {

        EMPI_Comm_type *aux;

        aux = EMPI_GLOBAL_comms;

        while (EMPI_GLOBAL_comms != NULL) {

            aux = EMPI_GLOBAL_comms->next;

            free (EMPI_GLOBAL_comms);

            EMPI_GLOBAL_comms = aux;
        }

        EMPI_GLOBAL_comms = NULL;
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Destroy_comms in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Destroy_system_classes'
*
****************************************************************************************************************************************/
static void EMPI_Destroy_system_classes (void) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Destroy_system_classes in <%s> ***\n", __FILE__);
    #endif

    //destroy classes data structure
    if (EMPI_GLOBAL_system_classes != NULL) {

        EMPI_Class_type *aux;

        aux = EMPI_GLOBAL_system_classes;

        while (EMPI_GLOBAL_system_classes != NULL) {

            aux = EMPI_GLOBAL_system_classes->next;

            free (EMPI_GLOBAL_system_classes);

            EMPI_GLOBAL_system_classes = aux;
        }

        EMPI_GLOBAL_system_classes = NULL;
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Destroy_system_classes in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Create_system_classes'
*
****************************************************************************************************************************************/
static void EMPI_Create_system_classes (void) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Create_system_classes in <%s> ***\n", __FILE__);
    #endif

    int rank, n, m;

    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);

    if (rank == EMPI_root) {

        EMPI_Class_type *class = NULL;

        EMPI_host_type *hostlist = NULL;

        for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

            if (class == NULL) {

                //initialization
                class = (EMPI_Class_type*) malloc (sizeof(EMPI_Class_type));
                assert (class);

                //set pointer
                EMPI_GLOBAL_system_classes = class;

            } else {

                class->next = (EMPI_Class_type*) malloc (sizeof(EMPI_Class_type));
                assert (class->next);

                class = class->next;
            }

            hostlist = EMPI_GLOBAL_hostlist;

            //Get comms parameters
            class->idclass = n;
            class->mflops = 0;
            class->nprocs = 0; //runtime update
            class->maxprocs = 0;
            class->iniprocs = 0;

            for (m = 0; m < EMPI_GLOBAL_nhosts; m ++) {

                if (hostlist->idclass == n) {

                    if (strcmp(class->name, hostlist->hclass) != 0) strcpy (class->name, hostlist->hclass);
                    class->iniprocs += hostlist->nprocs;
                    class->maxprocs += (hostlist->maxprocs - hostlist->nprocs); //runtime update
                    class->icost = hostlist->cost;
                    if ((EMPI_GLOBAL_mpolicy == EMPI_COST)||(EMPI_GLOBAL_mpolicy == EMPI_COST_IRR))
                        class->cost = hostlist->cost;
                    else
                        class->cost = 0.0;

                    class->mflops += hostlist->mflops;
                }

                hostlist = hostlist->next;
            }

            if (class->mflops < 0) class->mflops = -1; //runtime update

            class->next = NULL;
        }
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Create_system_classes in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Parse_malleability'
*
****************************************************************************************************************************************/
static void EMPI_Parse_malleability (void) {
    char *saveptr;
    
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Parse_malleability in <%s> ***\n", __FILE__);
    #endif

    int nrm = 0, class_found, nprocs_class = 0, ntok, nprocs, n;

    char readline [EMPI_MAX_LINE_LENGTH], token[] = ":", *record = NULL;

    FILE *file;

    //parse roadmap file
    if ((file = fopen ("malleability", "r")) == NULL) {

        fprintf (stderr, "\nError in EMPI_Parse_malleability file opening\n");

        MPI_Abort (EMPI_COMM_WORLD, -1);
    }

    //read roadmap file
    while (fscanf (file, "%s\n", readline) != EOF) {

        ntok = 0;

        //Get rm
        record = strtok_r (readline, token, &saveptr);

        EMPI_GLOBAL_listrm[nrm] = atoi(record);

        //set nextrm
        if ((EMPI_GLOBAL_nextrm == -1) && (EMPI_GLOBAL_listrm[nrm] > EMPI_GLOBAL_iteration)) EMPI_GLOBAL_nextrm = nrm;

        for (nprocs_class = 0; nprocs_class < 10; nprocs_class++) EMPI_GLOBAL_nprocs_class[nrm][nprocs_class] = 0;

        record = strtok_r (NULL, token, &saveptr);

        while (record != NULL) {

            if ((ntok % 2) == 0) {

                //numero de procesos
                nprocs = atoi(record);

            } else if ((ntok % 2) == 1) {

                //nombre de la clase
                for (class_found = 0, n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
                    if (strcmp (EMPI_GLOBAL_hclasses[n], record) == 0) {

                        class_found = 1;                        
                        EMPI_GLOBAL_nprocs_class[nrm][n] = nprocs; // Delta proc (increment/decrement in the proc number)
                        
                    }
                }
            }
            
            record = strtok_r (NULL, token, &saveptr);
            ntok ++;
        }

        nrm ++;
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Parse_malleability in <%s> ***\n", __FILE__);
    #endif
}



