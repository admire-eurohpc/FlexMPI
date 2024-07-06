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
