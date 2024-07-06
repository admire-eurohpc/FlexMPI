/**
* @version		FlexMPI v3.1
* @copyright	Copyright (C) 2018 Universidad Carlos III de Madrid. All rights reserved.
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
 *	File:       lbalance.h																												*
 *																																		*
 ****************************************************************************************************************************************/

#ifndef _EMPI_LBALANCE_H_
#define _EMPI_LBALANCE_H_

/****************************************************************************************************************************************
*
*	'EMPI_evaluate_LB'
*
****************************************************************************************************************************************/
int EMPI_evaluate_LB (int size, EMPI_Monitor_type *smonitor, int BTtype);

/****************************************************************************************************************************************
*
*	'EMPI_Set_lbpolicy'
*
****************************************************************************************************************************************/
void EMPI_Set_lbpolicy (int lbpolicy);

/****************************************************************************************************************************************
*
*	'EMPI_Disable_lbalance'
*
****************************************************************************************************************************************/
void EMPI_Disable_lbalance (void);

/****************************************************************************************************************************************
*
*	'EMPI_Enable_lbalance'
*
****************************************************************************************************************************************/
void EMPI_Enable_lbalance (void);

/****************************************************************************************************************************************
*
*	'EMPI_evaluate_system_status'
*
****************************************************************************************************************************************/
void EMPI_evaluate_system_status (int size, EMPI_Monitor_type *smonitor, int *sampling_status);

/****************************************************************************************************************************************
*
*	'EMPI_LBalance_spawn'
*
****************************************************************************************************************************************/
void EMPI_LBalance_spawn (int rank, int size, int newsize, int* newcount, int* vcounts, int* displs, int* newdispl, EMPI_Monitor_type *smonitor, int *stflops, int* fc);

/****************************************************************************************************************************************
*
*	'EMPI_LBalance_remove'
*
****************************************************************************************************************************************/
void EMPI_LBalance_remove (int rank, int size, int newsize, int* newcount, int* vcounts, int* displs, int* newdispl, EMPI_Monitor_type *smonitor, int *rremvs, int* fc);

/****************************************************************************************************************************************
*
*	'EMPI_LBalance'
*
****************************************************************************************************************************************/
int EMPI_LBalance (int *rank, int *size, int count, int disp, int *fc, EMPI_Monitor_type *smonitor);

#endif
