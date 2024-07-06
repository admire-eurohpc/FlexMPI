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
static void EMPI_Parse_cfile (int argc, char **argv);

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

/* implementation */

/****************************************************************************************************************************************
*
*    'FLEXMPI_Init'
*
****************************************************************************************************************************************/
int MPI_Init (int *argc, char ***argv) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Init in <%s> ***\n", __FILE__);
    #endif

    int err, rank, size, tag=997, rprocs, rdata[4], n, i;
    int provided;
    
    int blocklen [17] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, EMPI_Monitor_string_size, EMPI_Monitor_string_size,1,1,1,1,1};
    //int blocklen_energy [2] = {1, 1};
    int energy_rank = -1;

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
    MPI_Type_create_struct (17, blocklen, displs, EMPI_monitor_struct_type, &EMPI_Monitor_Datatype);
    MPI_Type_commit (&EMPI_Monitor_Datatype);

    // MPI_Type_create_struct (2, blocklen, displs, EMPI_monitor_struct_type, &EMPI_Monitor_Datatype);
    // MPI_Type_commit (&EMPI_Monitor_Datatype);

    //Init PAPI library
    int retval;
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if(retval != PAPI_VER_CURRENT){
        printf("Error initializing library\n");
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

    //Get parent
    err = MPI_Comm_get_parent (&parentcomm);

    if (err) fprintf (stderr, "Error in MPI_Comm_get_parent\n");

       //Dup MPI_COMM_WORLD
       err = MPI_Comm_dup (MPI_COMM_WORLD, &EMPI_COMM_WORLD);

    if (err) fprintf (stderr, "Error in MPI_Comm_dup\n");

    //debug
    #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
        fprintf (stdout, "\n*** DEBUG_MSG::call::EMPI_Parse_options in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
    #endif
    
    
    // Manages the input argument:
    for(i=0;i<*argc;i++){
        if(strcmp((*argv)[i],"-alloc:1")==0){
            EMPI_alloc();
        }
    }

    
    //Parse EMPI global options
    EMPI_Parse_options (*argc, *argv);
    
    // Spawned process
    if (parentcomm != MPI_COMM_NULL) {            
        
        int *countdispl = NULL;

        //Process type
        EMPI_GLOBAL_type = EMPI_SPAWNED;

        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stdout, "\n*** DEBUG_MSG::call::MPI_Intercomm_merge in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif


        
        //Merge intercommunicator
        err = MPI_Intercomm_merge (parentcomm, 1, &EMPI_COMM_WORLD);
        if (err) fprintf (stderr, "Error in MPI_Intercomm_merge\n");

        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stdout, "\n*** DEBUG_MSG::call::MPI_Comm_disconnect in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif

        //Disconnect aux communicator
        err = MPI_Comm_disconnect (&parentcomm);
        if (err) fprintf (stderr, "Error in MPI_Comm_disconnect\n");
            
        //Recv rprocs, minprocs, iteration and hostid. The MPI_Send command is executed in EMPI_Spawn function. 
        PMPI_Recv (rdata, 4, MPI_INT, EMPI_root, tag, EMPI_COMM_WORLD, &mpi_status);
        
        //Set rprocs, minprocs, iteration and hostid
        rprocs = rdata[0];
        EMPI_GLOBAL_minprocs = rdata[1];
        EMPI_GLOBAL_iteration = rdata[2];
        EMPI_GLOBAL_hostid = rdata[3];

        //debug
        #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
            fprintf (stdout, "\n*** DEBUG_MSG::call::EMPI_Spawn in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif
 
        MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
        MPI_Comm_size (EMPI_COMM_WORLD, &size);

        EMPI_GLOBAL_vcounts = (int*) calloc (size, sizeof (int));
        assert (EMPI_GLOBAL_vcounts);

        EMPI_GLOBAL_displs = (int*) calloc (size, sizeof (int));
        assert (EMPI_GLOBAL_displs);

        if (EMPI_GLOBAL_mpolicy != EMPI_NULL) {

            countdispl = (int*) malloc ((size*2) * sizeof(int));
            assert (countdispl);

            //Recv vcounts and displs
            PMPI_Bcast (countdispl, (size*2), MPI_INT, EMPI_root, EMPI_COMM_WORLD);

            //memcpy
            memcpy (EMPI_GLOBAL_vcounts, countdispl, size*sizeof(int));
            memcpy (EMPI_GLOBAL_displs, countdispl+size, size*sizeof(int));

            free (countdispl);
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
            fprintf (stdout, "\n*** DEBUG_MSG::call::EMPI_Parse_cfile in line %d function EMPI_Init in <%s> ***\n", __LINE__, __FILE__);
        #endif

        //Parse cfile with the node list
        EMPI_Parse_cfile (*argc, *argv);
        
        //Create system table
        EMPI_Create_system_classes ();

        MPI_Comm_dup(MPI_COMM_WORLD, &EMPI_GLOBAL_comm_energy);
        MPI_Comm_rank (EMPI_GLOBAL_comm_energy, &energy_rank);

        
        //gerhostname(host_name, host_name_length);
        MPI_Get_processor_name (EMPI_GLOBAL_host_name, &host_name_length);

    }
    
     // Configures the socket for controller communication
    if (rank == EMPI_root)
    {
        if((EMPI_GLOBAL_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
            printf("Error creating socket \n");
        }
        
        memset((char *) &si_me, 0, sizeof(si_me));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(EMPI_GLOBAL_recvport+200);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(EMPI_GLOBAL_socket, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
        {
            printf("Error binding socket \n");
        }
    
        EMPI_GLOBAL_controller_addr.sin_family = AF_INET;
        EMPI_GLOBAL_controller_addr.sin_port = htons(EMPI_GLOBAL_sendport);

        if ((he = gethostbyname(EMPI_GLOBAL_controller) ) == NULL ) {
            printf("Error resolving host name \n");
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
    // printf("minprocs = %d,  nhosts = %d\n", EMPI_GLOBAL_minprocs, EMPI_GLOBAL_nhosts);
    EMPI_GLOBAL_power_monitoring_data = calloc(EMPI_GLOBAL_minprocs, sizeof(double));
    

    
    if (rank == EMPI_root)
    {
        int rc;  // return code
        pthread_t thread;
        pthread_attr_t attr;

        if (pthread_mutex_init(&EMPI_GLOBAL_server_lock, NULL) != 0)
        {
            printf("\n mutex init failed\n");
        }

        rc = pthread_attr_init(&attr);
        check_posix_return(rc, "Initializing attribute");

        rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        check_posix_return(rc, "Setting detached state");

        rc = pthread_create(&thread, &attr, (void*)&command_listener, NULL);
        check_posix_return(rc, "Creating thread");
    }
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Init in <%s> ***\n", __FILE__);
    #endif
    

    return MPI_SUCCESS;
}

/****************************************************************************************************************************************
*
*    'MPI_Finalize'
*
****************************************************************************************************************************************/
int MPI_Finalize (void) {


    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Finalize in <%s> ***\n", __FILE__);
    #endif

    int err,eventcode_hwpc_1,eventcode_hwpc_2,active;
    long long values[3] = {0, 0, 0};
    char socketcmd[1024]; 

    char nodename[1024],command[1024];
    int nodename_len;
    
    
    
    
    
    
    // Sends the termination command to the controller
    if(EMPI_GLOBAL_socket!=-1){  // Only rank=0 process has a value != -1
        MPI_Get_processor_name(nodename,&nodename_len);
        sprintf(command,"nping --udp -g 5000 -p %d  -c 1 %s --data-string \"%s\">/dev/null",EMPI_GLOBAL_recvport,nodename,"4:on");
        printf("\n sending %s \n",command);
        system(command);
        sleep(30);
        sprintf(command,"nping --udp -g 5000 -p %d  -c 1 %s --data-string \"%s\">/dev/null",EMPI_GLOBAL_recvport,nodename,"5:");
        printf("\n sending %s \n",command);
        system(command);
        sleep(120);     
        //sprintf(socketcmd,"Application terminated");
        //sendto(EMPI_GLOBAL_socket,socketcmd,strlen(socketcmd),0,(struct sockaddr *)&EMPI_GLOBAL_controller_addr,sizeof(EMPI_GLOBAL_controller_addr));
    }
    
    
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

    

    if ((EMPI_GLOBAL_mpolicy == EMPI_LBALANCE) || (EMPI_GLOBAL_lbalance == EMPI_TRUE)) {

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
            fprintf (stdout, "\n*** DEBUG_MSG::call::MPI_Comm_disconnect in line %d function MPI_Finalize in <%s> *\n", __LINE__, __FILE__);
        #endif

        //Disconnect communicator
           err = MPI_Comm_disconnect (&EMPI_COMM_WORLD);

        if (err) fprintf (stderr, "Error in MPI_Comm_disconnect\n");
    }

    //Global tcomp
    EMPI_GLOBAL_tcomp_fin = MPI_Wtime();

    //debug
    #if (EMPI_DBGMODE == EMPI_DBG_DETAILED)
        fprintf (stdout, "\n*** DEBUG_MSG::call::MPI_Finalize in line %d function EMPI_Finalize in <%s> ***\n", __LINE__, __FILE__);
    #endif

    //Finalize MPI environment
    err = PMPI_Finalize ();
    if (err != MPI_SUCCESS) return MPI_ERR_OTHER;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Finalize in <%s> ***\n", __FILE__);
    #endif

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
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Get_wsize in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Get_wsize in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Get_status in <%s> ***\n", __FILE__);
    #endif

    *status = EMPI_GLOBAL_status;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Get_status in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Get_type in <%s> ***\n", __FILE__);
    #endif

    *type = EMPI_GLOBAL_type;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Get_type in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Parse_cfile'
*
****************************************************************************************************************************************/
static void EMPI_Parse_cfile (int argc, char **argv) {

    char *saveptr;
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Parse_cfile in <%s> ***\n", __FILE__);
    #endif

    //Core file format
    //hostname:maxprocs:class:cost:mflops

    int n, machine = EMPI_FALSE, length, hostname = EMPI_FALSE, rank, size, tag = 989, class_found = 0;

    char cfile[EMPI_MAX_FILENAME], readline[EMPI_MAX_LINE_LENGTH], token[] = ":", *record = NULL, *chostname = NULL;

    EMPI_host_type *hostlist = NULL, *aux = NULL;

    MPI_Status status;

    FILE *file;

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

                    printf("Host %s maxprocs is %d. ID is %d\n", hostlist->hostname, hostlist->maxprocs, hostlist->id);

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
                    //printf("Host %s maxprocs is %d\n", hostlist->hostname, hostlist->maxprocs);

                }

                free (chostname); chostname = NULL;
            }

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
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Parse_cfile in <%s> ***\n", __FILE__);
    #endif
}

                
/****************************************************************************************************************************************
*
*    'EMPI_Parse_options'
*
****************************************************************************************************************************************/
static void EMPI_Parse_options (int argc, char **argv) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Parse_options in <%s> ***\n", __FILE__);
    #endif

    int n, rank;

    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);

    for (n = 0; n < argc; n ++) {

        //Dynamic policy options

        if ((strcmp(argv[n], "-policy-efficiency") == 0)&&((n+2) < argc)) {

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
        }
        
        if (strcmp(argv[n], "-policy-malleability-triggered") == 0) {

            //Set malleability conditional policy
            EMPI_Set_policy (EMPI_MALLEABILITY_TRIG);

            //enable load balancing
            EMPI_Enable_lbalance ();
        }

        //Load balancing policy options
        if (strcmp(argv[n], "-lbpolicy-mflops") == 0) {

            //Set mflops policy
            EMPI_Set_lbpolicy (EMPI_LBMFLOPS);
        }

        if (strcmp(argv[n], "-lbpolicy-counts") == 0) {

            //Set execution time policy
            EMPI_Set_lbpolicy (EMPI_LBCOUNTS);
        }

        if (strcmp(argv[n], "-lbpolicy-static") == 0) {

            //Set execution time policy
            EMPI_Set_lbpolicy (EMPI_LBSTATIC);
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

                MPI_Abort (EMPI_COMM_WORLD, -1);
            }
        }

        //Spawning policy options
        if (strcmp(argv[n], "-spolicy-available") == 0) {

            //Set available nodes policy
            EMPI_GLOBAL_spolicy = EMPI_AVAIL_NODE;
        }

        if (strcmp(argv[n], "-spolicy-occupied") == 0) {

            //Set occupied nodes policy
            EMPI_GLOBAL_spolicy = EMPI_OCCUP_NODE;
        }

        //Adaptability policy options
        if (strcmp(argv[n], "-apolicy-exhaustive") == 0) {

            EMPI_GLOBAL_Adaptability_policy = EMPI_ADAPTABILITY_EX;
        }

        if (strcmp(argv[n], "-apolicy-lp") == 0) {

            EMPI_GLOBAL_Adaptability_policy = EMPI_ADAPTABILITY_LP;
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

            MPI_Abort (EMPI_COMM_WORLD, -1);
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

            MPI_Abort (EMPI_COMM_WORLD, -1);
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

            MPI_Abort (EMPI_COMM_WORLD, -1);
        }

        //Monitoring number of iterations options
        if ((strcmp(argv[n], "-ni") == 0)&&((n+1) < argc)) {

            //set number of iterations for monitoring
            EMPI_Set_niter (atoi(argv[n+1]));

            if (EMPI_GLOBAL_niter_lb == 10) EMPI_GLOBAL_niter_lb = EMPI_GLOBAL_niter;

            //next option
            n ++;

        }  else if ((strcmp(argv[n], "-ni") == 0)&&((n+1) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_options: number of iterations not given\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            MPI_Abort (EMPI_COMM_WORLD, -1);
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

            MPI_Abort (EMPI_COMM_WORLD, -1);
        }

        // Setting the port numbers
        if ((strcmp(argv[n], "-ports") == 0)&&((n+2) < argc)) {

            //set number of iterations for monitoring
            EMPI_GLOBAL_recvport = (atoi(argv[n+1]));
            EMPI_GLOBAL_sendport = (atoi(argv[n+2]));

            //next option
            n +=2;

        }  else if ((strcmp(argv[n], "-ports") == 0)&&((n+2) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_options: value of communication ports not valid\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            MPI_Abort (EMPI_COMM_WORLD, -1);
        }
        
        if ((strcmp(argv[n], "-controller") == 0) && ((n+1) < argc)) {

            //get server name
            strcpy (EMPI_GLOBAL_controller, argv[n+1]);

        } 
        
        if ((strcmp(argv[n], "-controller") == 0) && ((n+1) >= argc)) {

            fprintf (stderr, "\nError in EMPI_Parse_options: name of the controller not provided\n");

            //print EMPI usage
            if (rank == EMPI_root) EMPI_Print_usage ();

            MPI_Abort (EMPI_COMM_WORLD, -1);
        }

        // Sets if dummy I/O is performed
        if ((strcmp(argv[n], "-IOaction") == 0)&&((n+1) < argc)) {

            //set number of iterations for monitoring
            EMPI_GLOBAL_dummyIO = (atof(argv[n+1]));

            //next option
            n +=1;

        }    
    }
    
    if(rank == EMPI_root){
            if(EMPI_GLOBAL_dummyIO == -1 ) printf(" FlexMPI: program configured to perform parallel MPI I/O\n");
            else                           printf(" FlexMPI: program configured to perform dummy I/O of %f seconds\n",EMPI_GLOBAL_dummyIO);
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Parse_options in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Print_usage in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Print_usage in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Destroy_data_structure in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Destroy_data_structure in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Destroy_comms in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Destroy_comms in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Destroy_system_classes in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Destroy_system_classes in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Create_system_classes in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Create_system_classes in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_Parse_malleability in <%s> ***\n", __FILE__);
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
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_Parse_malleability in <%s> ***\n", __FILE__);
    #endif
}



