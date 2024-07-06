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
 *    File:      types.h                                                                                                                    *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

#ifndef _EMPI_TYPS_H_
#define _EMPI_TYPS_H_

#define EMPI_Monitor_string_size 32
#define EMPI_max_process 100

/* datatypes */
typedef struct EMPI_host_type {
    int        id; //host id
    int        nprocs; //running procs
    int        maxprocs; //max running procs
    int     iniprocs; //number of native procs
    int        mflops; //mega flops per second
    float    cost; //cost per second
    char     hclass [128]; //host class
    int     idclass; //host class id
    char     hostname [MPI_MAX_INFO_VAL];
    struct     EMPI_host_type *next;
} EMPI_host_type;

typedef struct EMPI_Cost_type {
    double     timestep;
    int     classes[20];
} EMPI_Cost_type;

typedef struct EMPI_Data_type {
    char    *id;
    void    *addr;
    int     *addr_row;
    int     *addr_col;
    void    *addr_val;
    int        size;
    int     dim;
//    unsigned long long int nnz;
    int     nnz;
    int        stype;
    int        mapping;
    MPI_Datatype datatype;
    struct EMPI_Data_type *next;
} EMPI_Data_type;

typedef struct EMPI_Monitor_type {
    int             count;
    long long       rtime;
    long long       ptime;
    double          ctime;
    long long       flops;
    int             hostid;
    long long       flops_iteration;
    long long       it_time;
    long long       hwpc_1;
    long long       hwpc_2;
    char            nhwpc_1[EMPI_Monitor_string_size];
    char            nhwpc_2[EMPI_Monitor_string_size];
    int             corebinding;
    int             termination; // Used to flag the program termination
    int             lbalance; // Used to flag the program termination
    double          iotime;
    int             EMPI_array_alloc;
} EMPI_Monitor_type;

typedef struct EMPI_Comm_type {
    int                mpi_op;        //mpi operation
    int                nprocs;     //number of processes
    int                datasize;     //data size (number of elements)
    MPI_Datatype     datatype;     //data size * datatype = number of bytes
    struct             EMPI_Comm_type *next;
} EMPI_Comm_type;

typedef struct EMPI_Class_type {
    int        idclass;     //class id
    int        nprocs;     //number of running, non initial, procs
    int     iniprocs;    //number of initial procs
    int     maxprocs;    //max running procs
    char     name [128]; //class name
    float    cost;         //cost
    float    icost;        //para hacer calculos de coste
    int     mflops;     //mflops
    struct     EMPI_Class_type *next;
} EMPI_Class_type;

typedef struct EMPI_Scale_factor {
    int        nhosts;     //number of hosts per class
    double  scale_factor; //scale factor
    char     name [128]; //host name
} EMPI_Scale_factor;


typedef struct EMPI_Spawn_data {
    int   dirty;  //indicates if we have fresh spawn data
    int   n_spawns;
    int*  hostid; //host id
    int*  nprocs; //procs
    //char name [128]; //host name
} EMPI_Spawn_data;

typedef struct command_flexmpi {
    int    command_n;
    char  *options[NUMBER_OPTIONS];
} command_flexmpi;

typedef struct service_arguments {
    int socket;
    struct sockaddr_in address;
} service_arguments;

#endif

