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
 *                                                                                                                                      *
 *  FLEX-MPI                                                                                                                            *
 *                                                                                                                                      *
 *  File:       server.c                                                                                                                *
 *                                                                                                                                      *
 ****************************************************************************************************************************************/

#ifndef _EMPI_SERVER_H_
#define _EMPI_SERVER_H_

/****************************************************************************************************************************************
*
*   'check_posix_return'
*
****************************************************************************************************************************************/
void check_posix_return(int rc, char* cause);
/****************************************************************************************************************************************
*
*   'parse_command'
*
****************************************************************************************************************************************/
void parse_command(char * raw_command, struct command_flexmpi * command);
/****************************************************************************************************************************************
*
*   'service_poster'
*
****************************************************************************************************************************************/
int service_poster(void* args);
/****************************************************************************************************************************************
*
*   'command_listener'
*
****************************************************************************************************************************************/
int command_listener(void);

// CHANGE BEGIN
void* thread_rpc_release_nodes(void* args);
int signal_thread_rpc_release_register(char *hostname, int num_procs);
int signal_thread_rpc_release_nodes();
int signal_thread_rpc_malleability_leave_region();
int signal_thread_rpc_malleability_enter_region(int procs_hint, int excl_nodes_hint);
int signal_thread_rpc_init(char *init_hostlist, char *added_hostlist);
int signal_thread_rpc_check_checkpointing();
int signal_thread_rpc_malleability_query();
int signal_thread_rpc_init_ss(char *init_hostlist);
int signal_thread_rpc_icc_fini();
char * get_addr_ic_str();
char * get_client_id();
// CHANGE END
#endif
