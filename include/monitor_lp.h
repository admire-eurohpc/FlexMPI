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
 *	File:       monitor.h																												*
 *																																		*
 ****************************************************************************************************************************************/

#ifndef _EMPI_MONITOR_LP_H_
#define _EMPI_MONITOR_LP_H_

/****************************************************************************************************************************************
*
*	'EMPI_lp_min_cost_fixed'
*
****************************************************************************************************************************************/
void EMPI_lp_min_cost_fixed (int rflops, int rprocs, int *mflops, int *newsize, int *cost, double *class_procs);

/****************************************************************************************************************************************
*
*	'EMPI_lp_max_cost_fixed'
*
****************************************************************************************************************************************/
void EMPI_lp_max_cost_fixed (int rflops, int rprocs, int *mflops, int *newsize, int *cost, double *class_procs);

/****************************************************************************************************************************************
*
*	'EMPI_lp_flops_max'
*
****************************************************************************************************************************************/
void EMPI_lp_flops_max (int rprocs, int *mflops, int *cost, double *class_procs);

/****************************************************************************************************************************************
*
*	'EMPI_lp_flops_min'
*
****************************************************************************************************************************************/
void EMPI_lp_flops_min (int rprocs, int *mflops, int *cost, double *class_procs);

/****************************************************************************************************************************************
*
*	'EMPI_lp_min_procs'
*
****************************************************************************************************************************************/
void EMPI_lp_min_procs (int rflops, int *newsize, int *mflops, double *class_procs);

/****************************************************************************************************************************************
*
*	'EMPI_lp_max_procs'
*
****************************************************************************************************************************************/
void EMPI_lp_max_procs (int rflops, int *newsize, int *mflops, double *class_procs);


/****************************************************************************************************************************************
*
*	'EMPI_lp_min_cost'
*
****************************************************************************************************************************************/
void EMPI_lp_min_cost (int rflops, int *newsize, int *mflops, int *cost, double *class_procs);


/****************************************************************************************************************************************
*
*	'EMPI_lp_max_cost'
*
****************************************************************************************************************************************/
void EMPI_lp_max_cost (int rflops, int *newsize, int *mflops, int *cost, double *class_procs);

/****************************************************************************************************************************************
*
*	'EMPI_lp_get_procs_flops_spawn'
*
****************************************************************************************************************************************/
void EMPI_lp_get_procs_flops_spawn (int rflops, int rprocs, double *class_procs);

/****************************************************************************************************************************************
*
*	'EMPI_lp_get_procs_flops_remove'
*
****************************************************************************************************************************************/
void EMPI_lp_get_procs_flops_remove (int rflops, int rprocs, double *class_procs);


/****************************************************************************************************************************************
*
*	'EMPI_lp_min_cost_fixed_eff'
*
****************************************************************************************************************************************/
void EMPI_lp_min_cost_fixed_eff (int rflops, int rprocs, int *mflops, int *newsize, int *cost, double *class_procs);

/****************************************************************************************************************************************
*
*	'EMPI_lp_max_cost_fixed_eff'
*
****************************************************************************************************************************************/
void EMPI_lp_max_cost_fixed_eff (int rflops, int rprocs, int *mflops, int *newsize, int *cost, double *class_procs);

/****************************************************************************************************************************************
*
*	'EMPI_lp_mips_w_max'
*
****************************************************************************************************************************************/
void EMPI_lp_mips_w_max (int rprocs, int *mflops, int *cost, double *class_procs);

/****************************************************************************************************************************************
*
*	'EMPI_lp_mips_w_min'
*
****************************************************************************************************************************************/
void EMPI_lp_mips_w_min (int rprocs, int *mflops, int *cost, double *class_procs);





#endif
