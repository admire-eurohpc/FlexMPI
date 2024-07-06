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
 *    File:       vars.h                                                                                                                    *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

#ifndef _EMPI_VARS_H_
#define _EMPI_VARS_H_

    
/* include */
#include <types.h>
#include <comm_data.h>
#include <malleability_tools.h>

/* global constants */
static const int EMPI_MAX_FILENAME         = 1024;
static const int EMPI_MAX_LINE_LENGTH     = 32768;
static const int EMPI_MAX_NPROCS         = 24;

static const int EMPI_root                = 0;

static const int EMPI_DISJOINT            = 674782;
static const int EMPI_SHARED            = 617828;

static const int EMPI_CBAL                = 182919;
static const int EMPI_CUSER                = 192018;

static const int EMPI_TRUE                 = 674321;
static const int EMPI_FALSE                = 684312;

static const int EMPI_ERROR                = 939199;

static const int EMPI_STRUCT_DENSE        = 534321;
static const int EMPI_STRUCT_SPARSE        = 544312;

static const int EMPI_STATIC             = 324542;
static const int EMPI_DYNAMIC             = 325765;

static const int EMPI_NULL                = 949998;

static const int EMPI_END                 = 921028;

static const int EMPI_ACTIVE            = 776765;
static const int EMPI_REMOVED            = 787634;
static const int EMPI_TO_REMOVE            = 792214;

//ALBERTO
static const int EMPI_SPAWNED_SYNC	= 938593;
static const int EMPI_SPAWNED            = 938594;
static const int EMPI_NATIVE            = 941490;

static const int EMPI_LBMFLOPS            = 854392;
static const int EMPI_LBCOUNTS            = 829010;
static const int EMPI_LBSTATIC            = 829099;

static const int EMPI_ROWS                = 748838;
static const int EMPI_NNZ                 = 782891;
static const int EMPI_FCOST             = 728191;

static const int EMPI_BALANCED             = 189312;
static const int EMPI_UNBALANCED        = 135894;

static const int EMPI_BURST             = 225615;
static const int EMPI_LONG_TERM            = 234684;

static const int EMPI_AVAIL_NODE        = 678292;
static const int EMPI_OCCUP_NODE        = 627543;

static const int EMPI_DEDICATED            = 689894;
static const int EMPI_NON_DEDICATED        = 685138;

static const int EMPI_TREAL                = 896646;
static const int EMPI_TCPU                = 877789;

static const int EMPI_ADAPTABILITY_EX    = 743289;
static const int EMPI_ADAPTABILITY_LP    = 718291;
// Communication variables
static const int EMPI_COMMBUFFSIZE        = 2048;
static const int EMPI_COMMNPACK            = 10;
static const int EMPI_COMMNUMOPTIONS     = 10;

// CHANGE
//malleability reconfig data
reconfig_data_t EMPI_GLOBAL_reconfig_data;
// CHANGE END
// Communication variables

int EMPI_GLOBAL_recvport;
int EMPI_GLOBAL_sendport;


/* global variables */
int EMPI_GLOBAL_nhosts;

double EMPI_GLOBAL_percentage;

int EMPI_GLOBAL_lbalance;

int EMPI_GLOBAL_maxprocs;
int EMPI_GLOBAL_minprocs;

double EMPI_GLOBAL_comm_prev;

double EMPI_GLOBAL_over_si;

int EMPI_GLOBAL_niter;
int EMPI_GLOBAL_niter_lb;

int EMPI_GLOBAL_wpolicy; //workload policy

int EMPI_GLOBAL_lbpolicy; //load balancing policy

int *EMPI_GLOBAL_vcounts;
int *EMPI_GLOBAL_displs;

int EMPI_GLOBAL_capture_comms;

int EMPI_GLOBAL_mpolicy; //monitor policy

int EMPI_GLOBAL_concurrency;

int EMPI_GLOBAL_hostid; //host id where the process is being executed

int EMPI_GLOBAL_nhclasses; //number of host classes

char EMPI_GLOBAL_hclasses[1024][128]; //host classes

char EMPI_GLOBAL_controller[512];   // external server name
char EMPI_GLOBAL_application[512];  // application name

int EMPI_GLOBAL_lbalance_disabled;

int EMPI_GLOBAL_perform_load_balance;

//Malleability
int EMPI_GLOBAL_nextrm;
int EMPI_GLOBAL_listrm[100];
int EMPI_GLOBAL_nprocs_class[100][1024];

int EMPI_GLOBAL_initnc;

int EMPI_GLOBAL_status; //active or removed status

int EMPI_GLOBAL_type; //spawned or native

int EMPI_GLOBAL_allocation;

double EMPI_GLOBAL_obj_texec;
double EMPI_GLOBAL_cum_cost;
double EMPI_GLOBAL_cum_time;
double EMPI_GLOBAL_obj_texec_threshold;

double EMPI_GLOBAL_spawn_cost;
double EMPI_GLOBAL_remove_cost;

int EMPI_GLOBAL_spolicy; //spawn policy: available nodes or occupied nodes

int EMPI_GLOBAL_iteration;

double EMPI_GLOBAL_tcomp;   //aggregated computation time
double EMPI_GLOBAL_tcomm;   //aggregated communication time
double EMPI_GLOBAL_tover;   //aggregated overhead time of the monitor functionality
double EMPI_GLOBAL_tio;     //aggregated communication time

double EMPI_GLOBAL_tcomm_itinit;
double EMPI_GLOBAL_tcomm_interval;

double EMPI_GLOBAL_tio_itinit;
double EMPI_GLOBAL_tio_interval;

double EMPI_GLOBAL_tcomp_ini;
double EMPI_GLOBAL_tcomp_fin;

double EMPI_GLOBAL_tcomm_ini;
double EMPI_GLOBAL_tcomm_fin;

double EMPI_GLOBAL_tio_last;    // Timestamp of the previous I/O operation
double EMPI_GLOBAL_tio_ini;     // Timestamp before the I/O operation
double EMPI_GLOBAL_tio_fin;     // Timestamp after the I/O operation
int    EMPI_GLOBAL_socket;      // Socket for sending control data
struct sockaddr_in EMPI_GLOBAL_controller_addr;  // Address of the controller 
double EMPI_GLOBAL_dummyIO;     // When <0 performs MPI I/O; When >=0 performs dummy I/O of EMPI_GLOBAL_dummyIO seconds 

double EMPI_GLOBAL_iterative_ini;
double EMPI_GLOBAL_iterative_end;

long long EMPI_GLOBAL_tover_ini;

double EMPI_GLOBAL_threshold;

double EMPI_GLOBAL_Load_threshold;

int EMPI_GLOBAL_self_adaptation;

//FIXME
double EMPI_GLOBAL_alpha;
double EMPI_GLOBAL_beta;
double EMPI_GLOBAL_bandwidth;
double EMPI_GLOBAL_gamma;

float EMPI_GLOBAL_sampling_time;

int EMPI_GLOBAL_hsteps;
int *EMPI_GLOBAL_hmon;
int EMPI_GLOBAL_hpos;

int EMPI_GLOBAL_nc;

int EMPI_GLOBAL_PAPI_init;
long long EMPI_GLOBAL_PAPI_rtime;
long long EMPI_GLOBAL_PAPI_rtime_init;
long long EMPI_GLOBAL_PAPI_ptime;
long long EMPI_GLOBAL_PAPI_ptime_init;
long long EMPI_GLOBAL_PAPI_flops;
long long EMPI_GLOBAL_PAPI_hwpc_1;
long long EMPI_GLOBAL_PAPI_hwpc_2;
char EMPI_GLOBAL_PAPI_nhwpc_1[EMPI_Monitor_string_size];
char EMPI_GLOBAL_PAPI_nhwpc_2[EMPI_Monitor_string_size];

int EMPI_GLOBAL_PAPI_eventSet;

int EMPI_GLOBAL_corebinding;

int EMPI_GLOBAL_delayio;
double EMPI_GLOBAL_delayiotime;

long long EMPI_GLOBAL_PAPI_rtime_lb;
long long EMPI_GLOBAL_PAPI_ptime_lb;
long long EMPI_GLOBAL_PAPI_flops_lb;
long long EMPI_GLOBAL_PAPI_hwpc_1_lb;
long long EMPI_GLOBAL_PAPI_hwpc_2_lb;
double EMPI_GLOBAL_tcomm_interval_lb;

double EMPI_GLOBAL_overhead_rpolicy;
double EMPI_GLOBAL_overhead_lbalance;
double EMPI_GLOBAL_overhead_processes;         // accumulated overhead of process creation/destruction
double EMPI_GLOBAL_lastoverhead_processes;  // last overhead of the last operation of process creation/destruction
double EMPI_GLOBAL_overhead_rdata;             // accumulated overhead of data redistribution
double EMPI_GLOBAL_lastoverhead_rdata;         // last overhead of data redistribution
double EMPI_GLOBAL_overhead_aux;

long long *EMPI_GLOBAL_track_flops;

long long *EMPI_GLOBAL_track_rtime;

int EMPI_GLOBAL_Adaptability_policy;

int EMPI_GLOBAL_PAPI_numevents;



double EMPI_GLOBAL_ENERGY_aggregated_final;
double EMPI_GLOBAL_ENERGY_aggregated_start;
double EMPI_GLOBAL_ENERGY_aggregated_init;
double EMPI_GLOBAL_ENERGY_aggregated_end;
double EMPI_GLOBAL_ENERGY_aggregated_elapsed;
double EMPI_GLOBAL_ENERGY_aggregated_elapsed_final;
double EMPI_GLOBAL_ENERGY_aggregated_power;
double EMPI_GLOBAL_ENERGY_aggregated_power_final;

double * EMPI_GLOBAL_power_monitoring_data;

double EMPI_GLOBAL_ENERGY_monitoring_temp[4];

int EMPI_GLOBAL_debug_comms;
int EMPI_GLOBAL_energy_rank;
int EMPI_GLOBAL_nhosts_aux;
int EMPI_GLOBAL_energy_eficiency_op_mode;
int EMPI_GLOBAL_non_exclusive;

long long EMPI_GLOBAL_PAPI_flops_iteration;
long long EMPI_GLOBAL_PAPI_it_time;
long long EMPI_GLOBAL_PAPI_real_flops_iteration;

long long t1;
long long t2;
int flag;

int EMPI_GLOBAL_flag_dynamic;

int EMPI_GLOBAL_flag_enter;

int EMPI_GLOBAL_PAPI_eventSet_energy;
char event_names[MAX_RAPL_EVENTS][PAPI_MAX_STR_LEN];
char units[MAX_RAPL_EVENTS][PAPI_MIN_STR_LEN];
MPI_Comm EMPI_GLOBAL_comm_energy;

//pointer to EMPI Global communicator
//MPI_Comm * PTR_EMPI_COMM_WORLD;
#define PTR_EMPI_COMM_WORLD (EMPI_GLOBAL_reconfig_data.comm.last_comm<0 ? NULL : EMPI_GLOBAL_reconfig_data.comm.comm[EMPI_GLOBAL_reconfig_data.comm.last_comm].intracomm)
//EMPI Global communicator
#define EMPI_COMM_WORLD (EMPI_GLOBAL_reconfig_data.comm.last_comm<0 ? MPI_COMM_NULL : *(EMPI_GLOBAL_reconfig_data.comm.comm[EMPI_GLOBAL_reconfig_data.comm.last_comm].intracomm))

EMPI_Cost_type *EMPI_GLOBAL_cost;

EMPI_Data_type *EMPI_GLOBAL_Data; //register shared data structure

EMPI_host_type *EMPI_GLOBAL_hostlist; //physical resources

MPI_Datatype EMPI_Monitor_Datatype;

EMPI_Monitor_type EMPI_GLOBAL_monitor;

EMPI_Comm_type* EMPI_GLOBAL_comms;

EMPI_Class_type* EMPI_GLOBAL_system_classes;

char EMPI_GLOBAL_host_name[MPI_MAX_PROCESSOR_NAME];

EMPI_Spawn_data EMPI_GLOBAL_spawn_data;

EMPI_Monitor_type EMPI_GLOBAL_monitoring_data;

pthread_mutex_t EMPI_GLOBAL_server_lock;

// Core binding
int EMPI_GLOBAL_corebindlist[EMPI_max_process][32];

// Poster thread active
int EMPI_GLOBAL_posteractive;

// Large-array allocation
int EMPI_array_alloc;

//ALBERTO: stop & restart vars
int ADM_GLOBAL_checkpointing;
int ADM_GLOBAL_waiting_decision; //1 means decision in progress. Not sure if I need it 
int ADM_GLOBAL_shrink;
int ADM_GLOBAL_nnodes;
char * ADM_GLOBAL_nodelist;
int MAX_LENGTH_NODELIST;
int ADM_APP_TYPE;
int ADM_APP_MODE;
#endif
