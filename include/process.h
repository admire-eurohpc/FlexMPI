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
 *	File:       process.h																												*
 *																																		*
 ****************************************************************************************************************************************/

#ifndef _EMPI_PROCESS_H_
#define _EMPI_PROCESS_H_

/****************************************************************************************************************************************
*
*	'EMPI_Spawn'
*
****************************************************************************************************************************************/
int EMPI_Spawn (int nprocs, char *argv[], char *bin, int *hostid, MPI_Info *info);

/****************************************************************************************************************************************
*
*	'EMPI_Remove'
*
****************************************************************************************************************************************/
int EMPI_Remove (int nprocs, int *rank);

#endif
