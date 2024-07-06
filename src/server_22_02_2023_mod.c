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
 *  File:       server.c                                                                                                                *
 *                                                                                                                                      *
 ****************************************************************************************************************************************/

/*
 * INCLUDES
 */
#include <empi.h>
#include "icc.h"
//#include "icdb.h"
#include <string.h>
#include <unistd.h>
// CHANGE
#include <pthread.h>
// END CHANGE

/*
 * CONSTANTS
 */
#define MAX_NODE_NAME 256
#define MAX_NEW_NODES_LIST_SIZE 20

/*
 * TYPES
 */
struct node_info {
    char name[MAX_NODE_NAME];
    int num_proc;
    int num_booked_proc;
};

struct new_nodes_list {
    int    first_empty_pos;
    struct node_info node_info[MAX_NEW_NODES_LIST_SIZE];
};


/*
 * LOCAL PROTOTIPES
 */
int command_rpc_init();
int command_rpc_malleability_enter_region(int procs_hint, int excl_nodes_hint);
int command_rpc_malleability_leave_region();
int command_rpc_release_nodes();
int command_rpc_release_register(char *hostname, int num_procs);
/*
 * GLOBAL VARS.
 */
// CHANGE
pthread_mutex_t mutex_rpc_icc = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_rpc_call = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_rpc_call = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_rpc_call_end = PTHREAD_COND_INITIALIZER;
int launch_rpc_release_nodes = 0;
char *thread_hostname = NULL;
int thread_num_procs = 0;
int thread_procs_hint = 0;
int thread_excl_nodes_hint = 0;
// END CHANGE

int processList[MAX_NEW_NODES_LIST_SIZE];
int bookedList[MAX_NEW_NODES_LIST_SIZE];
char *computeNodes[MAX_NEW_NODES_LIST_SIZE];

struct new_nodes_list new_nodes_list;
struct icc_context *icc = NULL;
int proc_increment=0, n_classes=0;

/*
 * diep
 */
void diep(char *s)
{
  perror(s);
  exit(1);
}

/*
 * check_posix_return
 */
void check_posix_return(int rc, char* cause)
{
    if (rc != 0)
    {
        printf("\nError: %s :[%s]", cause, strerror(rc));
    }
    else
    {
        printf("\n[DEBUG] %s successfully\n", cause);
    }
}

/*
 * parse_command
 */
void parse_command(char * raw_command,struct command_flexmpi *command )
{
    char *saveptr, *token;
    const char s = ':';
    int i = 0;

    /* get the first token */
    token = strtok_r(raw_command, &s, &saveptr);
    command->command_n = atoi(token);
    /* walk through other tokens */
    do
    {
        if(i>=NUMBER_OPTIONS){
            diep("Error parsing the message. Increase NUMBER_OPTIONS value.");
        }
        
        token = strtok_r(NULL, &s, &saveptr);
                
        if(token!=NULL){
            if(command->options[i]!=NULL) free(command->options[i]);
            command->options[i]=malloc(strlen(token)+2);
            strcpy(command->options[i],token);
        }
        else command->options[i]=NULL;
        i++;
        

    } while (token != NULL);

}

/*
 * parse_add_command
 */
void parse_add_command(const char *hostlist, struct new_nodes_list *new_nodes_list, int num_procs, int excl_nodes_hint, int *processList, char **computeNodes)
{
    char *saveptr, *token;
    const char s = ':';

    /* check parameters */
    if ((new_nodes_list == NULL) || (hostlist == NULL)) {
        diep("Error an input parameters is equal to NULL");
    }

    /* set all arrays to 0/NULL */
    memset (processList, 0, MAX_NEW_NODES_LIST_SIZE*sizeof(int));
    memset (computeNodes, 0, MAX_NEW_NODES_LIST_SIZE*sizeof(char *));

    /* walk through other tokens */
    int count_procs = 0;
    int count_nodes = 0;
    char *token_name_host = NULL;
    char *aux_str = (char *) malloc(strlen(hostlist)+1);
    strcpy (aux_str,hostlist);
    token_name_host = strtok_r(aux_str, &s, &saveptr);
    while (token_name_host != NULL) {

        char *token_num_procs = NULL;
        token_num_procs = strtok_r(NULL, &s, &saveptr);
        if(token_num_procs==NULL){
            diep("Error parsing the message. Node without number of processes");
        }
        int num_node_procs = atoi(token_num_procs)
        count_procs = count_procs + num_node_procs;
        int found = 0;
        /* check if the node is already on the list */
        for (int j=0; j<new_nodes_list->first_empty_pos; j++) {
            if (0 == strcmp(token_name_host, new_nodes_list->node_info[j].name)) {
                // ESTO NO PUEDE PASAR
                assert (0);
                if (excl_nodes_hint == 0) {
                    new_nodes_list->node_info[j].num_proc += num_node_procs;
                    new_nodes_list->node_info[j].num_booked_proc += num_node_procs;
                    computeNodes[count_nodes] = new_nodes_list->node_info[j].name;
                    processList[count_nodes] = num_node_procs;
                    count_nodes++;
                }
                found = 1;
            }
        }
        if (found == 0) {
            if (new_nodes_list->first_empty_pos >= MAX_NEW_NODES_LIST_SIZE) {
                diep("Error storing the nodes list. Increase MAX_NEW_NODES_LIST_SIZE value.");
            }
            int j = new_nodes_list->first_empty_pos;
            new_nodes_list->first_empty_pos++;
            if (strlen(token_name_host) >= MAX_NODE_NAME) {
                diep("Error storing the nodes list. Increase MAX_NODE_NAME value.");
            }

            strcpy (new_nodes_list->node_info[j].name, token_name_host);
            if (excl_nodes_hint == 1) {
                new_nodes_list->node_info[j].num_proc = 1;
                processList[count_nodes] = 1;
            } else {
                new_nodes_list->node_info[j].num_proc = num_node_procs;
                processList[count_nodes] = num_node_procs;
            }
            new_nodes_list->node_info[j].num_booked_proc = num_node_procs;
            computeNodes[count_nodes] = new_nodes_list->node_info[j].name;
            count_nodes++;
        }
        token_name_host = strtok_r(NULL, &s, &saveptr);
    }
    /* Wrong number of processes */
    if(count_procs < num_procs) {
        diep("Error parsing the message. Wrong number of processes");
    }
    /* free tokenized string */
    free(aux_str);
}

    
/*
 * parse_remove_command
 */
void parse_remove_command(const char *hostlist, struct new_nodes_list *new_nodes_list, int num_procs, int *processList, int *bookedList, char **computeNodes)
{
    char *saveptr, *token;
    const char s = ':';
    char *aux_str = NULL;

    /* check parameters */
    if (new_nodes_list == NULL) {
        diep("Error new_nodes_list input parameters is equal to NULL");
    }
    if (hostlist == NULL) {
        aux_str = (char *) malloc(1);
        aux_str[0] = '\0';
    } else {
        aux_str = (char *) malloc(strlen(hostlist)+1);
        strcpy (aux_str,hostlist);
    }
    /* set all arrays to 0/NULL */
    memset (processList, 0, MAX_NEW_NODES_LIST_SIZE*sizeof(int));
    memset (bookedList, 0, MAX_NEW_NODES_LIST_SIZE*sizeof(int));
    memset (computeNodes, 0, MAX_NEW_NODES_LIST_SIZE*sizeof(char *));

    /* walk through other tokens */
    int count_procs = 0;
    int count_nodes = 0;
    char *token_name_host = NULL;
    token_name_host = strtok_r(aux_str, &s, &saveptr);
    while (token_name_host != NULL) {
        // NUNCA ENTRA EN ESTE WHILE
        assert(0);
        char *token_num_procs = NULL;
        token_num_procs = strtok_r(NULL, &s, &saveptr);
        if(token_num_procs==NULL){
            diep("Error parsing the message. Node without number of processes");
        }
        int num_node_procs = atoi(token_num_procs)
        count_procs = count_procs + num_node_procs;
        int found = 0;
        /* check if the node is already on the list */
        for (int j=0; j<new_nodes_list->first_empty_pos; j++) {
            if (0 == strcmp(token_name_host, new_nodes_list->node_info[j].name)) {
                if (new_nodes_list->node_info[j].num_proc <= num_node_procs) {
                    diep("Error removing processes. removing more processes that those running in the node");
                }
                // ESTO ESTA MAL
                new_nodes_list->node_info[j].num_proc -= num_node_procs;
                new_nodes_list->node_info[j].num_booked_proc -= num_node_procs;//??
                computeNodes[count_nodes] = new_nodes_list->node_info[j].name;
                processList[count_nodes] = num_node_procs;
                bookedList[count_nodes] = num_node_procs;//??
                count_nodes++;
                found = 1;
                // END: ESTO ESTA MAL
            }
        }
        if (found == 0) {
            diep("Error removing nodes. node removed is not and added one.");
        }
        token_name_host = strtok_r(NULL, &s, &saveptr);
    }
    /* Wrong number of processes */
    if(count_procs > num_procs) {
        diep("Error parsing the message. Wrong number of processes");
    }
    for (int j=(new_nodes_list->first_empty_pos - 1); j>=0; j--) {
        if (new_nodes_list->node_info[j].num_proc > 0) {
            computeNodes[count_nodes] = new_nodes_list->node_info[j].name;
            int rest_proc = num_procs - count_procs;
            if (new_nodes_list->node_info[j].num_proc <= rest_proc) {
                processList[count_nodes] = new_nodes_list->node_info[j].num_proc;
                bookedList[count_nodes] = new_nodes_list->node_info[j].num_booked_proc;
                count_procs += new_nodes_list->node_info[j].num_proc;
                new_nodes_list->node_info[j].num_proc = 0;
                new_nodes_list->node_info[j].num_booked_proc = 0;
            } else {
                // NUNCA DEBERIA LLEGAR AQUI
                assert(0);
                processList[count_nodes] = rest_proc;
                count_procs += rest_proc;
                new_nodes_list->node_info[j].num_proc -= rest_proc;
            }
            if (count_procs == num_procs) break;
        }
    }
    /* free tokenized string */
    free(aux_str);
}

/*
 * flexmpi_reconfigure
 */
int flexmpi_reconfigure(int shrink, uint32_t maxprocs, const char *hostlist, void *data) {
  int mod_procs = 0;
  int free_cmd = 0; // boolean value to set free cmd or not
  int res = 0;

  // if shrink reduce max procs to whole nodes
  if (shrink == 1) {
      int aux_maxprocs = 0;
      int aux_num_iter = 0;
      comm_get_numremovableprocs(&(EMPI_GLOBAL_reconfig_data.comm), maxprocs, &aux_maxprocs, &aux_num_iter);
      maxprocs = aux_maxprocs;
  }
    
  // call reconfigure function to fillup EMPI_GLOBAL_reconfig_data
  EMPI_reconfigure(shrink, maxprocs, hostlist, &EMPI_GLOBAL_reconfig_data);

  if (shrink == 0 && (hostlist == NULL || strlen(hostlist) == 0)) {
    printf("\nAdd Empty hostlist\n");
    return ICC_FAILURE;
  } else {
      if (shrink == 0) {
          /* add new hosts and procs to nodes list*/
          parse_add_command(hostlist, &new_nodes_list, maxprocs, EMPI_GLOBAL_reconfig_data.excl_nodes_hint, processList, computeNodes);
          res = ADM_SpawnProcess (processList, computeNodes);
          if (res != 0) return ICC_FAILURE;
      } else {
          /* add new hosts and procs to nodes list*/
          parse_remove_command(hostlist, &new_nodes_list, maxprocs, processList, bookedList, computeNodes);
          res = ADM_RemoveProcess (processList, bookedList, computeNodes);
          if (res != 0) return ICC_FAILURE;
      }
  }
  /* check potential errors in conversions (14/01/2022) */
  return ICC_SUCCESS;
}

int service_poster(void* args)
{
    char line[2048];
    char * buf     = calloc(EMPI_COMMBUFFSIZE, 1);
    int index,length,size,resul=1;
    char hostname[1024];
    int hostnamelen;
        
    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_ADDRCONFIG;
    struct addrinfo* res=0;
    
    // Get rank0 node name
     MPI_Get_processor_name(hostname,&hostnamelen);
     
    pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
    if(strlen(EMPI_GLOBAL_controller)<4) resul=1; // Different from NULL
    else resul=strncmp(EMPI_GLOBAL_controller,"NULL",4);
    
    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
    
    if(resul==0){
        printf("\n Name of the external controller not provided. Monitoring thread exiting.... \n \n");
        pthread_exit(&EMPI_GLOBAL_posteractive);
    }
    else
    {
        printf("\n Establishing connection with controller in %s \n \n",EMPI_GLOBAL_controller);
    }

    pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
    sprintf(line,"%d",EMPI_GLOBAL_sendport);
    int err=getaddrinfo(EMPI_GLOBAL_controller,line,&hints,&res);
    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
    int active=1,termination=0;
    
    if (err<0) {
    diep("failed to");
    }

    int initialSocket=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if (initialSocket==-1) {
    diep("Error creating socket.");
    }
        
    while (active)
    {
        pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
        active=EMPI_GLOBAL_posteractive; // Check for service_poster termination
        termination=EMPI_GLOBAL_monitoring_data.termination; // Check for application termination
        pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);

        memset(buf, 0, EMPI_COMMBUFFSIZE);
        memset(line, 0, 2048);
        EMPI_host_type* hostlist = EMPI_GLOBAL_hostlist;
        
        if(termination==0){
            //for(index = 0; index < EMPI_GLOBAL_nhosts; index++)
            for(index = 0; index < 1; index++)
            {
                char line_aux[128];
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                MPI_Comm_size(EMPI_COMM_WORLD,&size);
                if(EMPI_GLOBAL_monitoring_data.rtime!=0) sprintf(line, "FLX [%s] rtime %.2f\t ptime %.2f\t ctime %.4f\t Mflops %lld\t %s\t %lld\t %s\t %lld\t iotime %.4f\t size %d iteration %d \n", hostname, (double)EMPI_GLOBAL_monitoring_data.rtime/1000000, (double)EMPI_GLOBAL_monitoring_data.ptime/1000000, EMPI_GLOBAL_monitoring_data.ctime,(long long int)(EMPI_GLOBAL_monitoring_data.flops/EMPI_GLOBAL_monitoring_data.rtime),EMPI_GLOBAL_monitoring_data.nhwpc_1,EMPI_GLOBAL_monitoring_data.hwpc_1,EMPI_GLOBAL_monitoring_data.nhwpc_2,EMPI_GLOBAL_monitoring_data.hwpc_2,EMPI_GLOBAL_monitoring_data.iotime,size,EMPI_GLOBAL_iteration);    
                else sprintf(line, "FLX [%s] rtime %.2f\t ptime %.2f\t ctime %.4f\t Mflops %lld\t %s\t %lld\t %s\t %lld\t iotime %.4f\t size %d iteration %d\n", hostname, (double)EMPI_GLOBAL_monitoring_data.rtime/1000000, (double)EMPI_GLOBAL_monitoring_data.ptime/1000000, EMPI_GLOBAL_monitoring_data.ctime,(long long int)1,EMPI_GLOBAL_monitoring_data.nhwpc_1,EMPI_GLOBAL_monitoring_data.hwpc_1,EMPI_GLOBAL_monitoring_data.nhwpc_2,EMPI_GLOBAL_monitoring_data.hwpc_2,EMPI_GLOBAL_monitoring_data.iotime,size,EMPI_GLOBAL_iteration);    
            
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                hostlist=hostlist->next;
                strcat(line, line_aux);
            }
        }
        
        if(termination==1){
            sprintf(line, "Application terminated");
            
            // Shutdown this thread
            pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
            EMPI_GLOBAL_posteractive=0;
            active=0;
            pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
        }
        
        strncpy(buf, line, strlen(line));

        //int length = sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&arguments.address, (socklen_t)sizeof(arguments.address));
        length = sendto(initialSocket,buf,strlen(buf),0,res->ai_addr,res->ai_addrlen);
        if (length == -1)
        {
            diep("sendto()");
        }
        printf("Sending > %s \n", buf);

        if (termination==0) sleep((int)floor((double)EMPI_GLOBAL_monitoring_data.ptime/1000000)+5);
        else {
            sleep(10);
            MPI_Abort(EMPI_COMM_WORLD,-1);
        }
    }
    pthread_exit(&EMPI_GLOBAL_posteractive);
}
// CHANGE BEGIN

void* thread_rpc_release_nodes(void* args)
{
    printf("thread_rpc_release_nodes: begin\n");
    while (1) {
        // wait until launch_rpc_release_nodes == 1
        pthread_mutex_lock(&mutex_rpc_call);
        while (launch_rpc_release_nodes == 0) {
            pthread_cond_wait(&cond_rpc_call, &mutex_rpc_call);
        }
        printf("thread_rpc_release_nodes: stop waiting\n");
        if (launch_rpc_release_nodes == 1) {
            printf("thread_rpc_release_nodes: command_rpc_init\n");
            command_rpc_init();
        } else if (launch_rpc_release_nodes == 2) {
            printf("thread_rpc_release_nodes: command_rpc_malleability_enter_region\n");
            command_rpc_malleability_enter_region(thread_procs_hint, thread_excl_nodes_hint);
        } else if (launch_rpc_release_nodes == 3) {
            printf("thread_rpc_release_nodes: command_rpc_malleability_leave_region\n");
            command_rpc_malleability_leave_region();
        } else if (launch_rpc_release_nodes == 4) {
            printf("thread_rpc_release_nodes: command_rpc_release_nodes\n");
            sleep(5); //TIMEOUT time
            command_rpc_release_nodes();
        } else if (launch_rpc_release_nodes == 5) {
            printf("thread_rpc_release_nodes: command_rpc_release_register\n");
            command_rpc_release_register(thread_hostname, thread_num_procs);
        }
        launch_rpc_release_nodes = 0;
        pthread_cond_signal(&cond_rpc_call_end);
        pthread_mutex_unlock(&mutex_rpc_call);

    }
}

int signal_thread_rpc_release_register(char *hostname, int num_procs)
{
    int ret = 0;
    printf("signal_thread_rpc_release_nodes: begin\n");
    // set launch_rpc_release_nodes == true and signal thread
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    printf("signal_thread_rpc_release_nodes: signal thread to remove nodes\n");
    thread_hostname = hostname;
    thread_num_procs = num_procs;
    launch_rpc_release_nodes = 5;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

int signal_thread_rpc_release_nodes()
{
    int ret = 0;
    printf("signal_thread_rpc_release_nodes: begin\n");
    // set launch_rpc_release_nodes == true and signal thread
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    printf("signal_thread_rpc_release_nodes: signal thread to remove nodes\n");
    launch_rpc_release_nodes = 4;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

int signal_thread_rpc_malleability_leave_region()
{
    int ret = 0;
    printf("signal_thread_rpc_release_nodes: begin\n");
    // set launch_rpc_release_nodes == true and signal thread
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    printf("signal_thread_rpc_release_nodes: signal thread to remove nodes\n");
    launch_rpc_release_nodes = 3;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

int signal_thread_rpc_malleability_enter_region(int procs_hint, int excl_nodes_hint)
{
    int ret = 0;
    printf("signal_thread_rpc_release_nodes: begin\n");
    // set launch_rpc_release_nodes == true and signal thread
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    printf("signal_thread_rpc_release_nodes: signal thread to remove nodes\n");
    thread_procs_hint = procs_hint;
    thread_excl_nodes_hint = excl_nodes_hint;
    launch_rpc_release_nodes = 2;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

int signal_thread_rpc_init()
{
    int ret = 0;
    printf("signal_thread_rpc_release_nodes: begin\n");
    // set launch_rpc_release_nodes == true and signal thread
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    printf("signal_thread_rpc_release_nodes: signal thread to remove nodes\n");
    launch_rpc_release_nodes = 1;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

int command_rpc_release_register(char *hostname, int num_procs)
{
    int ret = 0;
    
    printf("command_rpc_release_register: register nodes to remove\n");
    pthread_mutex_lock(&mutex_rpc_icc);
    ret = icc_release_register(icc, hostname, num_procs);
    pthread_mutex_unlock(&mutex_rpc_icc);
    assert(ret == ICC_SUCCESS);
    
    return ret;
}

int command_rpc_release_nodes()
{
    int ret = 0;

    printf("command_rpc_release_nodes: release nodes\n");
    pthread_mutex_lock(&mutex_rpc_icc);
    ret = icc_release_nodes(icc);
    pthread_mutex_unlock(&mutex_rpc_icc);
    assert(ret == ICC_SUCCESS);
 
    return ret;
}

int command_rpc_malleability_leave_region()
{
    int ret = 0;
    int rpcret = 0;
    
    pthread_mutex_lock(&mutex_rpc_icc);
    ret = icc_rpc_malleability_region(icc, ICC_MALLEABILITY_REGION_LEAVE, 0, 0, &rpcret);
    pthread_mutex_unlock(&mutex_rpc_icc);
    assert(ret == ICC_SUCCESS && rpcret == ICC_SUCCESS);
 
    return ret;
}

int command_rpc_malleability_enter_region(int procs_hint, int excl_nodes_hint)
{
    int ret = 0;
    int rpcret = 0;
    
    pthread_mutex_lock(&mutex_rpc_icc);
    ret = icc_rpc_malleability_region(icc, ICC_MALLEABILITY_REGION_ENTER, procs_hint, excl_nodes_hint, &rpcret);
    pthread_mutex_unlock(&mutex_rpc_icc);
    assert(ret == ICC_SUCCESS && rpcret == ICC_SUCCESS);
 
    return ret;
}


// CHANGE END


// CHANGE BEGIN
//int command_rpc_listener(void)
int command_rpc_init()
// CHANGE END
{
    /*IC*/
  int ret;
  int rpcret;

  uint32_t jobid, nnodes;
  char *slurm_jobid = getenv("SLURM_JOBID");
  char *slurm_nnodes = getenv("SLURM_NNODES");

  if(slurm_jobid == NULL){
    jobid = 77;
    nnodes = 2;
  } else {
    char *end;
    int errno = 0;
    jobid = strtoul(slurm_jobid, &end, 0);
    if (errno != 0 || end == slurm_jobid || *end != '\0')
      jobid = 0;

    errno = 0;
    nnodes = strtoul(slurm_nnodes, &end, 0);
    if (errno != 0 || end == slurm_nnodes || *end != '\0')
      nnodes = 0;
  }

  if (jobid == 0){
    jobid = 77;
    nnodes = 2;
  }

  pthread_mutex_lock(&mutex_rpc_icc);
  icc_init_mpi(ICC_LOG_DEBUG, ICC_TYPE_FLEXMPI, nnodes, flexmpi_reconfigure, NULL, &icc);
  //assert(icc != NULL);
  if (icc == NULL)
          printf("[application] Error connecting to IC\n");
  pthread_mutex_unlock(&mutex_rpc_icc);

  return 0;
  /* FlexMPI apps need to wait for malleability commands */
  /*sleep(15);
  ret = icc_rpc_malleability_region(icc, ICC_MALLEABILITY_REGION_ENTER, &rpcret);
  assert(ret == ICC_SUCCESS && rpcret == ICC_SUCCESS);

  sleep(10000);

  ret = icc_rpc_malleability_region(icc, ICC_MALLEABILITY_REGION_LEAVE, &rpcret);
  assert(ret == ICC_SUCCESS && rpcret == ICC_SUCCESS);*/

  /* wait for allocation request */
  /*puts("IC connection: Finishing");
  ret = icc_fini(icc);
  assert(ret == 0);*/

  //for debugging
  //while(1){sleep(1);} /* to keep the thread doing something (until test what happens)*/
}

int command_listener(void)
{
    //command_rpc_listener();

  /*Socket*/
    struct sockaddr_in si_me;
    struct sockaddr_in si_other;
    int i, n, s, slen = sizeof(si_other);
    size_t len0,len1;
    int option=1;
    struct command_flexmpi command;

    //int buffer_length = sizeof(int);
    char * buf     = calloc(EMPI_COMMBUFFSIZE, 1);
    char * buf_res = calloc(EMPI_COMMBUFFSIZE, 1);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        diep("socket");
    }
    for(i=0;i<NUMBER_OPTIONS;i++){
        command.options[i]=NULL;
    }
    // Reuse socket
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(EMPI_GLOBAL_recvport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
    {
        diep("bind");
    }

    while(1)
    {
        memset(buf, 0, EMPI_COMMBUFFSIZE);
        memset(buf_res, 0, EMPI_COMMBUFFSIZE);
        int length = recvfrom(s, buf, EMPI_COMMBUFFSIZE, 0, (struct sockaddr *)&si_other, (socklen_t *)&slen);
        if (length == -1)
        {
            diep("recvfrom()");
        }
        printf("[application] Received packet from %s:%d   Data: %s\n\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);

        //truncate message
        char bufer_cropped [length+2];
        memset(bufer_cropped, 0, length+2);
        strncpy(bufer_cropped, buf, length);
        strcat(bufer_cropped, "\0"); // David: add termination string
        //Parse string
        parse_command(bufer_cropped, &command);
        
        printf("[application] Command number is %d\n", command.command_n);
        switch(command.command_n)
        {
            case 1: //command: 0:policy:t_obj:threshold
            {

                if (strcmp(command.options[0], "EFFICIENCY") == 0)
                {
                    EMPI_Set_policy(EMPI_EFFICIENCY);
                    //Set objective
                    EMPI_GLOBAL_obj_texec = (atoi(command.options[1]));

                    //Set threshold
                    EMPI_GLOBAL_obj_texec_threshold = (atof(command.options[2]));

                    //enable load balancing
                    EMPI_Enable_lbalance ();

                    printf("policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_obj_texec, EMPI_GLOBAL_obj_texec_threshold);
                    // printf("policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);
                    sprintf(buf_res, "policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_obj_texec, EMPI_GLOBAL_obj_texec_threshold);
                    // sprintf(buf_res, "policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);

                }
                else if (strcmp(command.options[0], "EFFICIENCY_IRR") == 0)
                {
                    EMPI_Set_policy(EMPI_EFFICIENCY_IRR);
                    //Set objective
                    EMPI_GLOBAL_percentage = (atoi(command.options[1]));

                    //Set threshold
                    EMPI_GLOBAL_obj_texec_threshold = (atof(command.options[2]));

                    //enable load balancing
                    EMPI_Enable_lbalance ();

                    printf("policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_percentage, EMPI_GLOBAL_obj_texec_threshold);
                    // printf("policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);
                    sprintf(buf_res, "policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_percentage, EMPI_GLOBAL_obj_texec_threshold);
                    // sprintf(buf_res, "policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);

                }
                else if (strcmp(command.options[0], "COST") == 0)
                {
                    EMPI_Set_policy(EMPI_COST);
                    //Set objective
                    EMPI_GLOBAL_obj_texec = (atoi(command.options[1]));

                    //Set threshold
                    EMPI_GLOBAL_obj_texec_threshold = (atof(command.options[2]));

                    //enable load balancing
                    EMPI_Enable_lbalance ();

                    printf("policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_obj_texec, EMPI_GLOBAL_obj_texec_threshold);
                    // printf("policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);
                    sprintf(buf_res, "policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_obj_texec, EMPI_GLOBAL_obj_texec_threshold);
                    // sprintf(buf_res, "policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);

                }
                else if (strcmp(command.options[0], "COST_IRR") == 0)
                {
                    EMPI_Set_policy(EMPI_COST_IRR);

                    //Set objective
                    EMPI_GLOBAL_percentage = (atoi(command.options[1]));

                    //Set threshold
                    EMPI_GLOBAL_obj_texec_threshold = (atof(command.options[2]));

                    //enable load balancing
                    EMPI_Enable_lbalance ();

                    printf("policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_percentage, EMPI_GLOBAL_obj_texec_threshold);
                    // printf("policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);
                    sprintf(buf_res, "policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_percentage, EMPI_GLOBAL_obj_texec_threshold);
                    // sprintf(buf_res, "policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);

                }

                else if (strcmp(command.options[0], "LBALANCE") == 0)
                {
                    EMPI_Set_policy(EMPI_LBALANCE);
                    //enable load balancing
                    EMPI_Enable_lbalance ();
                    printf("policy: %s\n", command.options[0]);
                    sprintf(buf_res, "policy: %s\n", command.options[0]);

                }
                else if (strcmp(command.options[0], "MONITORDBG") == 0)
                {
                    EMPI_Set_policy(EMPI_MONITORDBG);
                    //enable load balancing
                    EMPI_Enable_lbalance ();
                    printf("policy: %s\n", command.options[0]);
                    sprintf(buf_res, "policy: %s\n", command.options[0]);

                }
                else if (strcmp(command.options[0], "MALLEABILITY") == 0)
                {
                    EMPI_Set_policy(EMPI_MALLEABILITY);
                    //enable load balancing
                    EMPI_Enable_lbalance ();
                    printf("policy: %s\n", command.options[0]);
                    sprintf(buf_res, "policy: %s\n", command.options[0]);

                }
                else if (strcmp(command.options[0], "MALLEABILITY_COND") == 0)
                {
                    EMPI_Set_policy(EMPI_MALLEABILITY_COND);
                    //enable load balancing
                    EMPI_Enable_lbalance ();
                    printf("policy: %s\n", command.options[0]);
                    sprintf(buf_res, "policy: %s\n", command.options[0]);

                }
                else
                {
                    printf("ERROR: Malleability policy %s NOT FOUND!", command.options[0]);
                    sprintf(buf_res, "ERROR: Malleability policy %s NOT FOUND!", command.options[0]);
                }
                break;

            }
            case 2:
                //command: 2:
                //Establish a flag to perform a load balancing at the end of the given sampling interval
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                EMPI_GLOBAL_monitoring_data.lbalance=1;
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);

                break;

            case 3:
                //command: 3:
                //Values for the last sampling interval
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                memset(buf, 0, EMPI_COMMBUFFSIZE);

                printf("Statistics: \n");
                char line[1024];
                EMPI_host_type* hostlist = EMPI_GLOBAL_hostlist;
                int index;
                for(index = 0; index < EMPI_GLOBAL_nhosts; index++)
                {
                    printf(" ** [%s] rtime is %lld, ptime is %lld, ctime is %f, flops is %lld \n", hostlist->hostname, EMPI_GLOBAL_monitoring_data.rtime, EMPI_GLOBAL_monitoring_data.ptime, EMPI_GLOBAL_monitoring_data.ctime,EMPI_GLOBAL_monitoring_data.flops);                    
                    printf("\n   %s",line);
                    hostlist=hostlist->next;
                   
                }

                //strncpy(buf_res, line, strlen(line));

                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                break;

            case 4: {
                //command: 4:
                //Statistics subscribe to service
                int active;
                // Poster active
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                active=EMPI_GLOBAL_posteractive;
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                
                if (strcmp (command.options[0],"on") == 0 && active==0){

                    pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                    EMPI_GLOBAL_posteractive=1;
                    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);

                  
                    int rc;  // return code
                    pthread_t thread;
                    pthread_attr_t attr;

                    service_arguments args;
                    args.socket = s;
                    args.address = si_other;

                    rc = pthread_attr_init(&attr);
                    check_posix_return(rc, "Initializing attribute");

                    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                    check_posix_return(rc, "Setting detached state");

                    rc = pthread_create(&thread, &attr, (void*)&service_poster, (void*)&args);
                    check_posix_return(rc, "Creating thread");
                    

                }
                else if (strcmp (command.options[0],"on") != 0){
                    pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                    EMPI_GLOBAL_posteractive=0;
                    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);                    
                }

                break;
            }
            case 5:

                printf(" Starting the termination of all processes \n" );
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock); // Only the server has an attached thread
                EMPI_GLOBAL_monitoring_data.termination=1;
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                break;
                
            case 6:  
                // Command 6: triggered execution 
                                
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                
                if(EMPI_GLOBAL_listrm[0] >0){ 
                  printf(" Command ignored: previous command already being processed. Try again later \n" );
                } 
                else{ // Asigna el incremento de procesos (deltaP) a cada clase    
                     i = 0;
                     do
                     {
                        for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
                            if (strcmp (EMPI_GLOBAL_hclasses[n], command.options[i]) == 0 && atoi(command.options[i+1])!=0) {
                                EMPI_GLOBAL_nprocs_class[0][n] = atoi(command.options[i+1]); // Delta proc (increment/decrement in the proc number)
                                printf(" Command: Create %d processes in compute node: %s \n",EMPI_GLOBAL_nprocs_class[0][n],EMPI_GLOBAL_hclasses[n]);
                                EMPI_GLOBAL_listrm[0] = 1;
                            }
                        }
                        i+=2;
                     } while (command.options[i] != NULL);
                }
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                break;

            case 7:  
                // Command 7: change the monitoring metrics
                                
                len0=strlen(command.options[0])+1;
                len1=strlen(command.options[1])+1;
                if(len0>EMPI_Monitor_string_size-2 || len1>EMPI_Monitor_string_size-2){
                    printf(" Command ignored: name of the events is too large  \n" );
                }
                else{                  
                  pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                  memcpy(EMPI_GLOBAL_PAPI_nhwpc_1,command.options[0],len0);
                  memcpy(EMPI_GLOBAL_PAPI_nhwpc_2,command.options[1],len1);
                  pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                }

                break;
                
            case 8:  
                // Command 8: core binding
                                
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                
                // Sets the trigger to do the core binding
                EMPI_GLOBAL_corebinding=1;
                i = 0;
                
                // Creates the binding list per process (n). 
                for (n=0;n<EMPI_max_process;n++) EMPI_GLOBAL_corebindlist[n][0]=0; // First column is the count number
                while (command.options[i] != NULL)
                {
                    n=atoi(command.options[i]);
                    EMPI_GLOBAL_corebindlist[n][EMPI_GLOBAL_corebindlist[n][0]+1]=atoi(command.options[i+1]);  // Creates the list
                    fflush(NULL);
                    EMPI_GLOBAL_corebindlist[n][0]++;
                    i+=2;
                } 
                
                 pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                

                break;
                
            case 9:  
                // Command 9: delay the I/O Phase
                                
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                
                // Sets the trigger to do the core binding
                if(EMPI_GLOBAL_delayio ==1){ 
                  printf(" Command ignored: previous command 9 already being processed with a delay of %f secs. Try again later \n",EMPI_GLOBAL_delayiotime );
                } 
                else{
                    EMPI_GLOBAL_delayio=1;
                    if(command.options[0] != NULL) {
                        EMPI_GLOBAL_delayiotime=atof(command.options[0]);
                    }
                    printf(" Delaying I/O phase %f seconds  \n",EMPI_GLOBAL_delayiotime);
                    
                }
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                break;
                
            case 10:  
                // Command 10: unlocks the I/O Phase
                printf(" Releasing the I/O phase   \n");
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                EMPI_GLOBAL_delayio=0;
                EMPI_GLOBAL_delayiotime=0;
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);    
                break;
                
            case 11:  
                // Command 11: triggered execution for a given iteration
                                
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                
                if(EMPI_GLOBAL_listrm[0] >0){ 
                  printf(" Command ignored: previous command already being processed. Try again later \n" );
                } 
                else{ // Asigna el incremento de procesos (deltaP) a cada clase    
                     i = 0;
                     EMPI_GLOBAL_listrm[0]=atoi(command.options[i]); // Iteration number for the reconfiguring action
                     i=1;
                     do
                     {
                        for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
                            if (strcmp (EMPI_GLOBAL_hclasses[n], command.options[i]) == 0 && atoi(command.options[i+1])!=0) {
                                EMPI_GLOBAL_nprocs_class[0][n] = atoi(command.options[i+1]); // Delta proc (increment/decrement in the proc number)
                                printf(" Command: Create %d processes in compute node: %s at iteration %d\n",EMPI_GLOBAL_nprocs_class[0][n],EMPI_GLOBAL_hclasses[n],EMPI_GLOBAL_listrm[0]);
                            }
                        }
                        i+=2;
                     } while (command.options[i] != NULL);
                }
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                break;
                
            case 12:  
                // Command 12: displays message from server
                printf(" Message from server received: %s  \n",buf);
                break;                
            default:
                break;
        }
       length = sendto(s, buf_res, strlen(buf_res), 0, (struct sockaddr *)&si_other, (socklen_t)slen);
       printf("Sent %d bytes as response\n", length);
    }

    free(buf);
    close(s);
    return 0;
}
