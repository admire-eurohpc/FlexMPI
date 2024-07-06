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
 *    File:       empi.h                                                                                                                    *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

#ifndef _EMPI_H_
#define _EMPI_H_
#define _GNU_SOURCE

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <malloc.h>
#include <float.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

// Server headers
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

/* empi headers */
#include <defs.h>         //defines
#include <vars.h>         //variables
#include <profile.h>     //profiling data
#include <init.h>         //init
#include <lbalance.h>    //load balance
#include <monitor_lp.h> //linear programming functions
#include <monitor.h>     //monitor
#include <scheduler.h>    //scheduler
#include <rdata.h>         //redistribution data
#include <wrapper.h>     //wrapper
#include <memalloc.h>     //memory allocation
#include <process.h>     //process management
#include <server.h>     //API server management
#include <adm_app_manager.h> //ADM app management
#include <comm_data.h> //communicators data
#include <malleability_tools.h> //malleability aux funcs

// Network headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// Core binding headers
#include <sched.h>


#endif
