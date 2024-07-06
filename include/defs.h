/**
* @version		FlexMPI v1.4
* @copyright	Copyright (C) 2017 Universidad Carlos III de Madrid. All rights reserved.
* @license		GNU/GPL, see LICENSE.txt
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
 *																																		*
 *	FLEX-MPI																															*
 *																																		*
 *	File:       defs.h																													*
 *																																		*
 ****************************************************************************************************************************************/

#ifndef _EMPI_DEFS_H_
#define _EMPI_DEFS_H_

/* defines */
#define EMPI_VER				1
#define EMPI_SUBVER				"0"
#define EMPI_DATE				2013
#define EMPI_NAME				"EMPI - Elastic Message Passing Interface"
#define EMPI_COPYRIGHT			"Copyright (c) Gonzalo Martin, David E. Singh, and Maria-Cristina Marinescu, 2012"
#define EMPI_MAIL				"gmcruz@arcos.inf.uc3m.es, desingh@arcos.inf.uc3m.es, mcristina@arcos.inf.uc3m.es"
#define EMPI_INSTITUTION		"Universidad Carlos III de Madrid (UC3M) - Computer Science Department"

#define EMPI_DBG_QUIET          5784392
#define EMPI_DBG_FNINFO         5847328
#define EMPI_DBG_DETAILED       5948239

#ifndef EMPI_DBGMODE
//#define EMPI_DBGMODE            EMPI_DBG_QUIET
#define EMPI_DBGMODE            EMPI_DBG_DETAILED
#endif

#define EMPI_EFFICIENCY			312341
#define EMPI_EFFICIENCY_IRR 	312910
#define EMPI_COST				391021
#define EMPI_COST_IRR 			310494
#define EMPI_LBALANCE			357831
#define EMPI_MALLEABILITY		341219
#define EMPI_MALLEABILITY_COND	391000
#define EMPI_MALLEABILITY_TRIG	391050 
#define EMPI_MONITORDBG			303891


#define	EMPI_SPARSE				434589
#define	EMPI_VECTOR				458910
#define	EMPI_DENSE				464890
#define	EMPI_VAR				457682

//Number of cores per node
#define EMPI_NCLASSC92			16
#define EMPI_NCLASSC913			16
#define EMPI_NCLASSC8			12
#define EMPI_NCLASSC7			12
#define EMPI_NCLASSC6			12
#define EMPI_NCLASSC1			8

//MPI operations
#define MPI_SEND				356598
#define MPI_RECV				363453
#define MPI_REDUCE				354584
#define MPI_BCAST				345875
#define MPI_BARRIER				345770
#define MPI_GATHER				354686
#define MPI_SCATTER				356872
#define MPI_ALLGATHER			354871
#define MPI_ALLGATHERV			301982
#define MPI_REDUCE_SCATTER		315679
#define MPI_ALLTOALL			319576
#define MPI_ALLREDUCE			321579


#define MAX_RAPL_EVENTS  64
#define PAPI_MAX_STR_LEN 128
#define PAPI_MIN_STR_LEN 64

//jacobi = 0, gradient = 1
#define BENCHMARK 0

 // #define OPCODE2

#define BUFLEN 512
#define NPACK 10
#define PORT  6666
#define PORT2 6667
#define NUMBER_OPTIONS 100


/* macros */
#define EMPI_MACRO_MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define EMPI_MACRO_MAX(X,Y) ((X) > (Y) ? (X) : (Y))

#endif
