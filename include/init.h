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
 *    File:       init.h                                                                                                                    *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

#ifndef _EMPI_INIT_H_
#define _EMPI_INIT_H_

/****************************************************************************************************************************************
*
*    'FLEXMPI_Init'
*
****************************************************************************************************************************************/
int MPI_Init (int *argc, char ***argv);
int FLEXMPI_Init (int *argc, char ***argv);
int FLEXMPI_Init_ss (int *argc, char ***argv);

/****************************************************************************************************************************************
*
*    'FLEXMPI_Finalize'
*
****************************************************************************************************************************************/
int MPI_Finalize (void);
int FLEXMPI_Finalize (void);
int FLEXMPI_Finalize_ss (void);

/****************************************************************************************************************************************
*
*    'EMPI_Get_wsize'
*
****************************************************************************************************************************************/
void EMPI_Get_wsize    (int rank, int size, int dim, int *desp, int *count, int *vcounts, int *displs);

/****************************************************************************************************************************************
*
*    'EMPI_Get_status'
*
****************************************************************************************************************************************/
void EMPI_Get_status (int *status);

/****************************************************************************************************************************************
*
*    'EMPI_Get_type'
*
****************************************************************************************************************************************/
void EMPI_Get_type (int *type);

/****************************************************************************************************************************************
*
*    'EMPI_Set_type'
*
****************************************************************************************************************************************/
void EMPI_Set_type (int type);

/****************************************************************************************************************************************
*
*    'EMPI_Destroy_comms'
*
****************************************************************************************************************************************/
void EMPI_Destroy_comms (void);

/****************************************************************************************************************************************
*
*    'add_host
*
****************************************************************************************************************************************/
int add_host (char *hostname, int numprocs, int is_initial_proc, int maxprocs, char *classname);

#endif
