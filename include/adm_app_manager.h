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
 *                                                                                                                                      *
 *  FLEX-MPI                                                                                                                            *
 *                                                                                                                                      *
 *  File:       adm_app_manager.c                                                                                                                *
 *                                                                                                                                      *
 ****************************************************************************************************************************************/
#ifndef _EMPI_ADM_APP_MANAGER_H_
#define _EMPI_ADM_APP_MANAGER_H_

#include <empi.h>
/*
 * CONSTANTS
 */
#define ADM_SERVICE_START 150520
#define ADM_SERVICE_STOP 203459

#define ADM_COMM_WORLD EMPI_COMM_WORLD
#define ADM_ACTIVE EMPI_ACTIVE
#define ADM_REMOVED EMPI_REMOVED
#define ADM_SPAWNED EMPI_SPAWNED
#define ADM_NATIVE  EMPI_NATIVE
#define MAX_NODE_NAME 256

/*
 * type
 */
struct process_info {
    char name[MAX_NODE_NAME];
    int num_proc;
    int num_booked_proc;
};
typedef struct process_info process_list_t;
/**
Calls from the IC to the Application Manager
**/

/****************************************************************************************************************************************
*
*   'ADM_SpawnThread'
* input : int *threadList: list of the number of threads created in each compute node
* input : char **computeNodes: list of the compute nodes where the threads are created
* output: int exitValue: 0 success, -1 failure
*
****************************************************************************************************************************************/
int ADM_SpawnThread (int *threadList, char **computeNodes);

/****************************************************************************************************************************************
*
*   'ADM_RemoveThread'
* input : int *threadList: list of the number of threads removed in each compute node
* input : char **computeNodes: list of the compute nodes where the threads are destroyed
* output: int exitValue: 0 success, -1 failure
*
****************************************************************************************************************************************/
int ADM_RemoveThread (int *threadList, char **computeNodes);

/****************************************************************************************************************************************
*
*   'ADM_SpawnProcess'
* input : int *processList: list of the number of processes created in each compute node
* input : char **computeNodes: list of the compute nodes where the threads are created
* output: int exitValue: 0 success, -1 failure
*
****************************************************************************************************************************************/
int ADM_SpawnProcess (process_list_t *processNodes);

/****************************************************************************************************************************************
*
*   'ADM_RemoveProcess'
* input : int *processList: list of the number of processes removed in each compute node
* input : char **computeNodes: list of the compute nodes where the threads are destroyed
* output: int exitValue: 0 success, -1 failure
*
****************************************************************************************************************************************/
int ADM_RemoveProcess (process_list_t *processNodes);

/****************************************************************************************************************************************
*
*   'ADM_ReleaseProcesses'
* input : int *processList: list of the number of processes removed in each compute node
* output: int exitValue: 0 success, -1 failure
*
****************************************************************************************************************************************/
int ADM_ReleaseProcesses (process_list_t *processNodes);


/**
Init Call from MPI_Init
**/
/****************************************************************************************************************************************
 ADM_Init
input : argc: number of command line parameters
 input : argv: array of command line parameters
 input : world_rank: rank of the process
 input : world_size: total number of processes
 ****************************************************************************************************************************************/
int ADM_Init (int argc, char **argv, int world_rank, int world_size);

/**
Calls from the Application to the Application Manager
**/


/****************************************************************************************************************************************
*
*   'ADM_MonitoringService'
* input : int command: Action to be done on the given target nodes. START/STOP
* output: int exitValue: 0 success, -1 failure
*
****************************************************************************************************************************************/
int ADM_MonitoringService (int command);

/****************************************************************************************************************************************
*
 ADM_MalleableRegion
 input : START /STOP flag identifying the begin (START), and the end (ST OP ) of the code region.
 output: exitValue: Command==ADM_SERVICE_START -> 0 success, -1 failure
                    Command==ADM_SERVICE_STOP -> ADM_ACTIVE process active,
                                                 ADM_REMOVED process removed,
                                                 -1 failure
*
****************************************************************************************************************************************/
int ADM_MalleableRegion (int command);

/**
 * ADM_RegisterSysAttributesInt
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to the integer value
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  output: int exitValue: 0 success, -1 failure
 */

 int ADM_RegisterSysAttributesInt(char* key, int *val);

/**
 * ADM_RegisterSysAttributesIntArr
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to the first integer value if the array
 *       size:   number of elements on the array
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RegisterSysAttributesIntArr(char* key, int *val, int size);

/**
 * ADM_RegisterSysAttributesDouble
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to the double value
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RegisterSysAttributesDouble(char* key, double *val);

/**
 * ADM_RegisterSysAttributesDoubleArr
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to the first integer value if the array
 *       size:   number of elements on the array
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RegisterSysAttributesDoubleArr(char* key, double *val, int size);

/**
 * ADM_RegisterSysAttributesStr
 *  input : key: string that determines the key of the key/value pair
 *       value:  pointer to the first char of the string array
 *  If key already exist then realloc previous buffer and set the new value
 *  if value -> NULL erase key and value (if exist) or do nothing
 *  output: int exitValue: 0 success, -1 failure
 */
int ADM_RegisterSysAttributesStr(char* key, char *val);

/**
 * ADM_GetSysAttributesInt
 *  input : key: string that determines the key of the key/value pair
 *  input : value: pointer to an existing integer were to a copy the value,
 *  output: value: pointer to an copy of the value,,
 *        exitValue:    0 success;
 *                -1 not found;
 *                -2 wrong size
 */
int ADM_GetSysAttributesInt(char* key, int *val);

/**
 * ADM_GetSysAttributesIntArr
 *  input : key: string that determines the key of the key/value pair
 *  input : value: pointer to an existing integer array were to a copy the value,
 *       size: size of memory buffer in integer elements
 *  output: value: pointer to an copy of the value,,
 *        exitValue:  >0 success (number of array elements)
 *                -1 not found;
 *                -2 wrong size
 */
int ADM_GetSysAttributesIntArr(char* key, int *val, int size);

/**
 * ADM_GetSysAttributesInt
 *  input : key: string that determines the key of the key/value pair
 *  input : value: pointer to an existing double were to a copy the value,
 *  output: value: pointer to an copy of the value,,
 *        exitValue:    0 success;
 *                -1 not found;
 *                -2 wrong size
 */
int ADM_GetSysAttributesDouble(char* key, double *val);

/**
 * ADM_GetSysAttributesDoubleArr
 *  input : key: string that determines the key of the key/value pair
 *  input : value: pointer to an existing double array were to a copy the value,
 *       size: size of memory buffer in double elements
 *  output: value: pointer to a copy of the value,,
 *        exitValue:  >0 success (number of array elements)
 *                -1 not found;
 *                -2 wrong size
 */
int ADM_GetSysAttributesDoubleArr(char* key, double *val, int size);

/**
 * ADM_GetSysAttributesStr_
 *  input : key: string that determines the key of the key/value pair
 *  input : value: pointer to an existing double array were to a copy the value,
 *       size: size of memory buffer in integer elements
 *  output: value: pointer to a copy of the value,,
 *        exitValue:  >0 success (number of array elements)
 *                -1 not found;
 *                -2 wrong size
 */
int ADM_GetSysAttributesStr(char* key, char *val, int size);


/****************************************************************************************************************************************
*
*   'ADM_RegisterSysAttributes'
 input : key: string that determines the key of the key/value pair
         value:  pointer to a memory buffer to store the value
         size:  size of the memory buffer that store the value
  If key already exist then erase previous value and set the new one
  if value -> NULL erase key and value (if exist) or do nothing
  Example of keys:
 • IO_BANDWIDTH. Aggregate I/O system bandwidth currently available expressed in Gb/s.
 • APP_MANAGER_STATUS. Status of the application manager.
 • SLURM_MALLEABLE_CAPABILITIES. Reports whether the SLURM component has malleable features enabled.
 output: int exitValue: 0 success, -1 failure
*
****************************************************************************************************************************************/
int ADM_RegisterSysAttributes (char *key, void *value, int size);

/****************************************************************************************************************************************
*
 ADM_GetSysAttributes
  input : key: string that determines the key of the key/value pair
  input : value: pointer to an existing buffer were to a copy the value,
                 if =NULL then value wont be copied
          size:  size of the memory buffer set to store the value
  output: value: pointer to an copy of the value,,
          size:  required size of the memory buffer to store the value
          exitValue: >0 (and value == NULL) value size; 0 success;
                     -1 not found; -2 wrong size
*
****************************************************************************************************************************************/
int ADM_GetSysAttributes (char *key, void *value, int size);

/****************************************************************************************************************************************
*
 ADM_ListSysAttributes
  input  key_list: previous returned list to be erased (or empty buffer->NULL)
  output  key_list:  new allocated buffer array of stored keys
          list_size: number of keys stored
*
****************************************************************************************************************************************/
void ADM_ListSysAttributes (char ***key_list, int *list_size);

/****************************************************************************************************************************************
*
 ADM_EraseListSysAttributes
  intput  key_list:  array of stored keys
  list_size: number of keys stored
  intput  key_list:  array of stored keys
*
****************************************************************************************************************************************/
void ADM_EraseListSysAttributes (char ***key_list);

/****************************************************************************************************************************************
*
 ADM_Malleability
  intput decision:  if malleability is required by the IC
  input nnodes: number of nodes to add(+) or remove(-)
  intput  hostlist:  list of hostnames to add/remove (separated by comma)
*
****************************************************************************************************************************************/
void ADM_Malleability (int *decision, int *nnodes, char* hostlist);
#endif /*_EMPI_ADM_APP_MANAGER_H_*/






