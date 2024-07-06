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
#include <fcntl.h>
#include <stdio.h>
// END CHANGE

/*
 * CONSTANTS
 */
#define MAX_NEW_NODES_LIST_SIZE 20

/*
 * TYPES
 */

struct new_nodes_list {
    int    first_empty_pos;
    process_list_t node_info[MAX_NEW_NODES_LIST_SIZE];
};


/*
 * LOCAL PROTOTIPES
 */
int command_rpc_init();
int command_rpc_malleability_enter_region(int procs_hint, int excl_nodes_hint);
int command_rpc_malleability_leave_region();
int command_rpc_release_nodes();
int command_rpc_release_register(char *hostname, int num_procs);
int command_rpc_icc_fini(); // CHANGE FINI

/*
 * STOP AND RESTART LOCAL PROTOTYPES
 */
int command_rpc_check_checkpointing();
int command_rpc_malleability_query();
int command_rpc_init_ss();

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
char *thread_init_hostlist = NULL;
char *thread_added_hostlist = NULL;
char * addr_ic_str = NULL;
char * clid = NULL;
// END CHANGE

process_list_t processNodes[MAX_NEW_NODES_LIST_SIZE];

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
        fprintf(stderr, "\nError: %s :[%s]", cause, strerror(rc));
    }
    else
    {
        fprintf(stderr, "\n[DEBUG] %s successfully\n", cause);
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
void parse_add_command(const char *hostlist, struct new_nodes_list *new_nodes_list, int num_procs, int excl_nodes_hint, process_list_t *processNodes)
{
    /* check parameters */
    if ((new_nodes_list == NULL) || (hostlist == NULL)) {
        diep("Error an input parameters is equal to NULL");
    }
    fprintf(stderr, "parse_add_command(%d): hostlist=(%s)\n", getpid(), hostlist);

    /* set all arrays to 0/NULL */
    memset (processNodes, 0, MAX_NEW_NODES_LIST_SIZE*sizeof(process_list_t));

    /* walk through other tokens */
    int count_procs = 0;
    int count_nodes = 0;
    int ret_parse = 0;
    
    // copy hostlist on aux_st¡r
    int length_hostlist = strlen(hostlist)+1;
    char *aux_str = (char *) calloc(length_hostlist,1);
    char *aux_str2 = (char *) calloc(length_hostlist,1);
    char *token_name_host = (char *) calloc(length_hostlist,1);
    int num_node_procs;
    strcpy (aux_str,hostlist);
    
    // get hostname token and numprocs
    ret_parse = sscanf(aux_str, "%[^:]:%d,%s", token_name_host, &num_node_procs, aux_str2);
    strcpy(aux_str, aux_str2);
    bzero(aux_str2,length_hostlist);

    // loop to tokenize aux_str
    while (ret_parse >= 2) {

        fprintf(stderr, "parse_add_command(%d): token_name_host=%s, num_node_procs=%d\n", getpid(), token_name_host, num_node_procs);

        // if node list is full ERROR
        if (new_nodes_list->first_empty_pos >= MAX_NEW_NODES_LIST_SIZE) {
            diep("Error storing the nodes list. Increase MAX_NEW_NODES_LIST_SIZE value.");
        }
        
        // copy and increment node list index
        int j = new_nodes_list->first_empty_pos;
        new_nodes_list->first_empty_pos++;
        
        // copy node name in node list
        if (strlen(token_name_host) >= MAX_NODE_NAME) {
            diep("Error storing the nodes list. Increase MAX_NODE_NAME value.");
        }
        strcpy (new_nodes_list->node_info[j].name, token_name_host);

        // copy pointer to hostname from node list to computeNodes
        strcpy(processNodes[count_nodes].name, new_nodes_list->node_info[j].name);

        // copy procs per node in nodelist and processList
        if (excl_nodes_hint == 1) {
            new_nodes_list->node_info[j].num_proc = 1;
            signal_thread_rpc_release_register(processNodes[count_nodes].name, num_node_procs-1);
        } else {
            new_nodes_list->node_info[j].num_proc = num_node_procs;
        }
        processNodes[count_nodes].num_proc = new_nodes_list->node_info[j].num_proc;

        //update count_procs
        count_procs = count_procs + processNodes[count_nodes].num_proc;

        //copy booked procs per node in nodelist
        //new_nodes_list->node_info[j].num_booked_proc = num_node_procs;
        new_nodes_list->node_info[j].num_booked_proc = new_nodes_list->node_info[j].num_proc;

        fprintf(stderr, "parse_add_command(%d): iter=%d, computeNode=%s, numProcs=%d, NumBookedProcs=%d\n", getpid(), j, processNodes[count_nodes].name, processNodes[count_nodes].num_proc, new_nodes_list->node_info[j].num_booked_proc);

        // increment nodes index for computeNodes and processList
        count_nodes++;
        
        // get hostname token and numprocs
        ret_parse = sscanf(aux_str, "%[^:]:%d,%s", token_name_host, &num_node_procs, aux_str2);
        strcpy(aux_str, aux_str2);
        bzero(aux_str2,length_hostlist);
    }
    

    /* free tokenized string */
    free(aux_str);
    free(aux_str2);
    free(token_name_host);

    fprintf(stderr, "parse_add_command(%d): procs removed=%d/%d\n", getpid(), count_procs, num_procs);

}

    
/*
 * parse_remove_command
 */
void parse_remove_command(const char *hostlist, struct new_nodes_list *new_nodes_list, int num_procs, process_list_t *processNodes)
{

    /* check parameters */
    if (new_nodes_list == NULL) {
        diep("Error new_nodes_list input parameters is equal to NULL");
    }
    /* set all arrays to 0/NULL */
    memset (processNodes, 0, MAX_NEW_NODES_LIST_SIZE*sizeof(process_list_t));

    /* walk through other tokens */
    int count_procs = 0;
    int count_nodes = 0;

    // go through the list of allocated nodes
    int last_node_index = new_nodes_list->first_empty_pos - 1;
    for (int j=last_node_index; j>=0; j--) {
        fprintf(stderr, "parse_remove_command: new_nodes_list->node_info[%d].num_proc = %d\n", j, new_nodes_list->node_info[j].num_proc);
        
        // check the node has processes
        if (new_nodes_list->node_info[j].num_proc > 0) {
            int rest_proc = num_procs - count_procs;

            fprintf(stderr, "parse_remove_command: new_nodes_list->node_info[%d].num_proc = %d   - Rest_proc = %d\n", j, new_nodes_list->node_info[j].num_proc, rest_proc);
            if (new_nodes_list->node_info[j].num_proc <= rest_proc) {

                // store node procs to erase them
                strcpy(processNodes[count_nodes].name, new_nodes_list->node_info[j].name);
                processNodes[count_nodes].num_proc = new_nodes_list->node_info[j].num_proc;
                processNodes[count_nodes].num_booked_proc = new_nodes_list->node_info[j].num_booked_proc;
                
                fprintf(stderr, "parse_remove_command(%d): iter=%d, computeNode=%s, numProcs=%d, NumBookedProcs=%d\n", getpid(), j, processNodes[count_nodes].name, processNodes[count_nodes].num_proc, processNodes[count_nodes].num_booked_proc);
                
                // erase node form node list
                new_nodes_list->node_info[j].num_proc = 0;
                new_nodes_list->node_info[j].num_booked_proc = 0;
                bzero (&(new_nodes_list->node_info[j].name),MAX_NODE_NAME);
                
                //update node list index
                new_nodes_list->first_empty_pos--;
                
                // update count_procs
                count_procs += processNodes[count_nodes].num_proc;

            } else {
                // end removing processes
                break;
            }
        }
        count_nodes++;
    }
    fprintf(stderr, "parse_remove_command(%d): procs removed=%d/%d\n", getpid(), count_procs, num_procs);
}

/*
 * flexmpi_reconfigure
 */
int flexmpi_reconfigure(int shrink, uint32_t maxprocs, const char *hostlist, void *data) {
  int mod_procs = 0;
  int free_cmd = 0; // boolean value to set free cmd or not
  int res = 0;

    fprintf(stderr, "flexmpi_reconfigure(%d): shrink=%d, maxprocs=%d, hostlist=%s\n", getpid(), shrink, maxprocs, hostlist);

    // if shrink reduce max procs to whole nodes
    if (shrink == 1) {
        int aux_maxprocs = 0;
        int aux_num_iter = 0;
        comm_get_numremovableprocs(&(EMPI_GLOBAL_reconfig_data.comm), maxprocs, &aux_maxprocs, &aux_num_iter);
        maxprocs = aux_maxprocs;
        fprintf(stderr, "flexmpi_reconfigure(%d): comm_get_numremovableprocs: maxprocs=%d\n", getpid(), maxprocs);

        // if not processes to remove, return
        if (maxprocs == 0) return ICC_SUCCESS;
    }

    // call reconfigure function to fillup EMPI_GLOBAL_reconfig_data
    fprintf(stderr, "flexmpi_reconfigure(%d): EMPI_reconfigure\n", getpid());
    EMPI_reconfigure(shrink, maxprocs, hostlist, &EMPI_GLOBAL_reconfig_data);

    if (shrink == 0 && (hostlist == NULL || strlen(hostlist) == 0)) {
    fprintf(stderr, "\nAdd Empty hostlist\n");
    return ICC_FAILURE;
    } else {
        if (shrink == 0) {
            /* add new hosts and procs to nodes list*/
            fprintf(stderr, "flexmpi_reconfigure(%d): parse_add_command\n", getpid());
            parse_add_command(hostlist, &new_nodes_list, maxprocs, EMPI_GLOBAL_reconfig_data.excl_nodes_hint, processNodes);
            fprintf(stderr, "flexmpi_reconfigure(%d): ADM_SpawnProcess\n", getpid());
            res = ADM_SpawnProcess (processNodes);
            if (res != 0) return ICC_FAILURE;
        } else {
            /* add new hosts and procs to nodes list*/
            fprintf(stderr, "flexmpi_reconfigure(%d): parse_remove_command\n", getpid());
            parse_remove_command(hostlist, &new_nodes_list, maxprocs, processNodes);
            fprintf(stderr, "flexmpi_reconfigure(%d): ADM_RemoveProcess\n", getpid());
            res = ADM_RemoveProcess (processNodes);
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
        fprintf(stderr, "\n Name of the external controller not provided. Monitoring thread exiting.... \n \n");
        pthread_exit(&EMPI_GLOBAL_posteractive);
    }
    else
    {
        fprintf(stderr, "\n Establishing connection with controller in %s \n \n",EMPI_GLOBAL_controller);
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
        fprintf(stderr, "Sending > %s \n", buf);

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
    int end = 0;
    fprintf(stderr, "thread_rpc_release_nodes: begin\n");
    while (end == 0) {
        // wait until launch_rpc_release_nodes == 1
        pthread_mutex_lock(&mutex_rpc_call);
        while (launch_rpc_release_nodes == 0) {
            pthread_cond_wait(&cond_rpc_call, &mutex_rpc_call);
        }
        fprintf(stderr, "thread_rpc_release_nodes: stop waiting (launch_rpc_release_nodes=%d)\n",launch_rpc_release_nodes);
        if (launch_rpc_release_nodes == 1) {
            fprintf(stderr, "thread_rpc_release_nodes: command_rpc_init\n");
            command_rpc_init(thread_init_hostlist, thread_added_hostlist, &addr_ic_str, &clid);
        } else if (launch_rpc_release_nodes == 2) {
            fprintf(stderr, "thread_rpc_release_nodes: command_rpc_malleability_enter_region\n");
            command_rpc_malleability_enter_region(thread_procs_hint, thread_excl_nodes_hint);
        } else if (launch_rpc_release_nodes == 3) {
            fprintf(stderr, "thread_rpc_release_nodes: command_rpc_malleability_leave_region\n");
            command_rpc_malleability_leave_region();
        } else if (launch_rpc_release_nodes == 4) {
            fprintf(stderr, "thread_rpc_release_nodes: command_rpc_release_nodes\n");
            sleep(3); //TIMEOUT time
            command_rpc_release_nodes();
        } else if (launch_rpc_release_nodes == 5) {
            fprintf(stderr, "thread_rpc_release_nodes: command_rpc_release_register\n");
            command_rpc_release_register(thread_hostname, thread_num_procs);
        } else if (launch_rpc_release_nodes == 6) {
	        fprintf(stderr, "thread_rpc_release_nodes: command_rpc_init_ss\n");
            command_rpc_init_ss(thread_init_hostlist, &addr_ic_str, &clid);
	    } else if (launch_rpc_release_nodes == 7) {
	        fprintf(stderr, "thread_rpc_release_nodes: command_rpc_check_checkpointing\n");
            command_rpc_check_checkpointing();
	    } else if (launch_rpc_release_nodes == 8) {
	        fprintf(stderr, "thread_rpc_release_nodes: command_rpc_malleability_query\n");
            command_rpc_malleability_query();
	    } else if (launch_rpc_release_nodes == 9) {
	        fprintf(stderr, "thread_rpc_release_nodes: command_rpc_icc_fini\n");
            sleep(1); //TIMEOUT time
            command_rpc_icc_fini();
            end = 1; // end thread
	    }
        fprintf(stderr, "thread_rpc_release_nodes: signal\n");

        launch_rpc_release_nodes = 0;
        pthread_cond_broadcast(&cond_rpc_call_end); /* To wake up all*/
        pthread_mutex_unlock(&mutex_rpc_call);

    }
    return (NULL);
}


int signal_thread_rpc_release_register(char *hostname, int num_procs)
{
    int ret = 0;
    fprintf(stderr, "signal_thread_rpc_release_register: begin %s:%d\n",hostname,num_procs);
    // set launch_rpc_release_nodes == true and signal thread
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    fprintf(stderr, "signal_thread_rpc_release_register: signal thread to remove nodes\n");
    
    // copy hostname
    free(thread_hostname);
    thread_hostname = NULL;
    if (hostname != NULL) {
        thread_hostname = (char *)calloc(strlen(hostname)+1,1);
        assert(thread_hostname);
        strcpy(thread_hostname, hostname);
    }
    thread_num_procs = num_procs;
    launch_rpc_release_nodes = 5;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

int signal_thread_rpc_release_nodes()
{
    int ret = 0;
    fprintf(stderr, "signal_thread_rpc_release_nodes: begin\n");
    // set   == true and signal thread
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    fprintf(stderr, "signal_thread_rpc_release_nodes: signal thread to remove nodes\n");
    launch_rpc_release_nodes = 4;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

int signal_thread_rpc_malleability_leave_region()
{
    int ret = 0;
    fprintf(stderr, "signal_thread_rpc_malleability_leave_region: begin\n");
    // set launch_rpc_release_nodes == true and signal thread
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    fprintf(stderr, "signal_thread_rpc_malleability_leave_region: signal thread to remove nodes\n");
    launch_rpc_release_nodes = 3;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

int signal_thread_rpc_malleability_enter_region(int procs_hint, int excl_nodes_hint)
{
    int ret = 0;
    fprintf(stderr, "signal_thread_rpc_malleability_enter_region: begin\n");
    // set launch_rpc_release_nodes == true and signal thread
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    fprintf(stderr, "signal_thread_rpc_malleability_enter_region: signal thread to remove nodes\n");
    thread_procs_hint = procs_hint;
    thread_excl_nodes_hint = excl_nodes_hint;
    launch_rpc_release_nodes = 2;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

int signal_thread_rpc_init(char *init_hostlist, char *added_hostlist)
{
    int ret = 0;
    fprintf(stderr, "signal_thread_rpc_init\n");
    // set launch_rpc_release_nodes == true and signal thread
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    fprintf(stderr, "signal_thread_rpc_init: performing action\n");
    fprintf(stderr, "signal_thread_rpc_init: init_hostlist=%s\n", init_hostlist);
    if (added_hostlist) fprintf(stderr, "signal_thread_rpc_init: added_hostlist=%s\n", added_hostlist);
    
    // copy init_hostlist
    free(thread_init_hostlist);
    thread_init_hostlist = NULL;
    if (init_hostlist != NULL) {
        thread_init_hostlist = (char *)calloc(strlen(init_hostlist)+1,1);
        assert(thread_init_hostlist);
        strcpy(thread_init_hostlist, init_hostlist);
    }
    // copy added_hostlist
    free(thread_added_hostlist);
    thread_added_hostlist = NULL;
    if (added_hostlist != NULL) {
        thread_added_hostlist = (char *)calloc(strlen(added_hostlist)+1,1);
        assert(thread_added_hostlist);
        strcpy(thread_added_hostlist, added_hostlist);
    }
    launch_rpc_release_nodes = 1;
    pthread_cond_signal(&cond_rpc_call);

    while (addr_ic_str == NULL) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }

    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}


/**
 * Calls icc_mpi_init by means of FlexMPI signal thread.
 */
int signal_thread_rpc_init_ss(char *init_hostlist)
{
    int ret = 0;
    fprintf(stderr, "signal_thread_rpc_init_ss\n");
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    fprintf(stderr, "signal_thread_rpc_init_ss: signal thread to initialize icc from stop and restart\n");
    // copy init_hostlist
    free(thread_init_hostlist);
    thread_init_hostlist = NULL;
    if (init_hostlist != NULL) {
        thread_init_hostlist = (char *)calloc(strlen(init_hostlist)+1,1);
        assert(thread_init_hostlist);
        strcpy(thread_init_hostlist, init_hostlist);
    }
    launch_rpc_release_nodes = 6;
    pthread_cond_signal(&cond_rpc_call);

    while (addr_ic_str == NULL) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }

    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

/* 02022024 - UNUSED with the new version */
int signal_thread_rpc_check_checkpointing()
{
    int ret = 0;
    fprintf(stderr, "signal_thread_rpc_check_checkpointing called\n");
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    fprintf(stderr, "signal_thread_rpc_check_checkpointing: signal thread to check checkpointing\n");
    launch_rpc_release_nodes = 7;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

/* 02022024 - UNUSED with the new version */
int command_rpc_check_checkpointing(){
    int ret = 0;
    int rpcret = 0;
    fprintf(stderr, "command_rpc_check_checkpointing: checks if the IC need stop-restart an app\n");
    pthread_mutex_lock(&mutex_rpc_icc);
    //ret = icc_rpc_checkpointing(icc, &rpcret);
        
    if (icc == NULL)
          fprintf(stderr, "[application] Error connecting to IC\n");

    //fprintf(stderr, "[DEBUG] Checkpointing results: ret = %d, rpcret = %d \n", ret, rpcret);
    ADM_GLOBAL_checkpointing = rpcret;
    pthread_mutex_unlock(&mutex_rpc_icc);
    return ret;
}

/* 02022024 - UNUSED with the new version */
int signal_thread_rpc_malleability_query()
{
    int ret = 0;
    fprintf(stderr, "signal_thread_rpc_malleability_query called\n");
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    fprintf(stderr, "signal_thread_rpc_malleability_query: signal thread to query for malleability operations\n");
    launch_rpc_release_nodes = 8;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

/* 02022024 - UNUSED with the new version */
int command_rpc_malleability_query(){
    int ret = 0;
    int rpcret;
    int malleability = 0, nnodes = 0;
    char * hostlist = NULL;
    fprintf(stderr, "command_rpc_malleability_query: query for malleability operations\n");
    pthread_mutex_lock(&mutex_rpc_icc);
    //ret = icc_rpc_malleability_query(icc, &malleability, &nnodes, &hostlist);
        
    if (icc == NULL)
          fprintf(stderr, "[application] Error connecting to IC\n");

    fprintf(stderr, "[DEBUG] Malleability query answer: malleability = %d, nnodes = %d, hostlist = %s\n", malleability, nnodes, hostlist);
    //global variables for the malleable operations
    pthread_mutex_unlock(&mutex_rpc_icc);
    return ret;
}

/**
 * Reconfigure for stop and restart applications
 * ALBERTO 13072023
*/
int adm_ss_reconfigure(int shrink, uint32_t maxprocs, const char *hostlist, void *data) {
  fprintf(stderr, "adm_ss_reconfigure(%d): shrink=%d, maxprocs=%d, hostlist=%s\n", getpid(), shrink, maxprocs, hostlist);

  int mod_procs = 0;
  int free_cmd = 0; // boolean value to set free cmd or not
  int res = 0;

    fprintf(stderr, "stoprestart_reconfigure(%d): shrink=%d, maxprocs=%d, hostlist=%s\n", getpid(), shrink, maxprocs, hostlist);

    if (shrink == 0 && (hostlist == NULL || strlen(hostlist) == 0)) {
        fprintf(stderr, "\nAdd Empty hostlist\n");
        return ICC_FAILURE;
    } else {
        if (shrink == 0) {
            /* add new hosts and procs to nodes list*/
            fprintf(stderr, "stoprestart_reconfigure(%d): parse_add_command\n", getpid());
            parse_add_command(hostlist, &new_nodes_list, maxprocs, EMPI_GLOBAL_reconfig_data.excl_nodes_hint, processNodes);
            /* stop and restart
            * 1. Create hostfile based on new_nodes_list
            * 2. Finalize */ 
            FILE *fd = fopen("/tmp/nek_malleability.res", "w");
            if (fd == NULL) return 0;
        
            /* Calculate the total length of the concatenated string*/
            int totalLength = 0;
            for (int i = 0; i <  new_nodes_list.first_empty_pos; i++) {
                totalLength += strlen(new_nodes_list.node_info[i].name);
                totalLength += 2; // +1 per ',' and +1 per ':'
                totalLength += snprintf(NULL, 0, "%d", new_nodes_list.node_info[i].num_proc);
                fprintf(stderr, "adm_ss_reconfigure: node=%s:%d(%d)\n",new_nodes_list.node_info[i].name,new_nodes_list.node_info[i].num_proc,totalLength);
            }

            /* Allocate memory for the concatenated string */
            char *concatenatedString = (char *)calloc(totalLength, 1); 
            if (concatenatedString == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                return ICC_FAILURE;
            }

            /* Concatenate the nodes*/
            concatenatedString[0] = '\0'; 
            int counter = 0;
            for (int i = 0; i < new_nodes_list.first_empty_pos; i++) {
                if (i == 0)
                    counter += snprintf(&(concatenatedString[counter]), (totalLength-counter), "%s:%d", new_nodes_list.node_info[i].name, new_nodes_list.node_info[i].num_proc);
                else
                    counter += snprintf(&(concatenatedString[counter]), (totalLength-counter), ",%s:%d", new_nodes_list.node_info[i].name, new_nodes_list.node_info[i].num_proc);
            }
            fprintf(stderr, "adm_ss_reconfigure: noconcatenatedStringde=%s\n",concatenatedString);


            /* write the nodelist to file*/
            ssize_t bytesWritten = fwrite(concatenatedString, sizeof(char), strlen(concatenatedString), fd);

            fclose(fd);      
            fprintf(stderr, "stoprestart_reconfigure(%d): Hostlist in /tmp/nek_malleability.res\n", getpid());
            fprintf(stderr, "stoprestart_reconfigure(%d): Hostlist written: %s\n", getpid(), concatenatedString);
            /* Free the dynamically allocated memory*/
            free(concatenatedString);

            /* Set global vars to its correct value for checkpoint*/
            ADM_GLOBAL_shrink = shrink;
            ADM_GLOBAL_checkpointing = 1;

        } else { 
            /* remove desired hosts and procs from node list*/
            fprintf(stderr, "stoprestart_reconfigure(%d): parse_remove_command\n", getpid());
            parse_remove_command(hostlist, &new_nodes_list, maxprocs, processNodes);
            fprintf(stderr, "stoprestart_reconfigure(%d): ADM_RemoveProcess\n", getpid());
            
            /* stop and restart
            * 1. Create hostfile without desired nodes from new_nodes_list
            * 2. release_register
            * 3. release_nodes
            * 4. Finalize */

            FILE *fd = fopen("/tmp/nek_malleability.res", "w");
            if (fd == NULL) return 0;
        
            /* Calculate the total length of the concatenated string*/
            int totalLength = 0;
            for (int i = 0; i < new_nodes_list.first_empty_pos; i++) {
                fprintf(stderr, "adm_ss_reconfigure: SHRINK - new_nodes_list(%d - %s)\n", i, new_nodes_list.node_info[i].name);
                totalLength += strlen(new_nodes_list.node_info[i].name);
                totalLength += 2; // +1 per ',' and +1 per ':'
                totalLength += snprintf(NULL, 0, "%d", new_nodes_list.node_info[i].num_proc);
                fprintf(stderr, "adm_ss_reconfigure: node=%s:%d(%d)\n",new_nodes_list.node_info[i].name,new_nodes_list.node_info[i].num_proc,totalLength);
            }

            /* Allocate memory for the concatenated string */
            /*char *concatenatedString = (char *)malloc(totalLength + 1 + EMPI_GLOBAL_nhosts); // due to \n per node
            if (concatenatedString == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            return ICC_FAILURE;
            }*/
            char *concatenatedString = (char *)calloc(totalLength, 1); 
            if (concatenatedString == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                return ICC_FAILURE;
            }

            /* Concatenate the nodes*/
            /*concatenatedString[0] = '\0'; 
            for (int i = 0; i < EMPI_GLOBAL_nhosts; i++) {
                strcat(concatenatedString, new_nodes_list.node_info[i].name);
                strcat(concatenatedString, "\n");
            }*/
            concatenatedString[0] = '\0'; 
            int counter = 0;
            for (int i = 0; i < new_nodes_list.first_empty_pos; i++) { // last node released
                if (i == 0)
                    counter += snprintf(&(concatenatedString[counter]), (totalLength-counter), "%s:%d", new_nodes_list.node_info[i].name, new_nodes_list.node_info[i].num_proc);
                else
                    counter += snprintf(&(concatenatedString[counter]), (totalLength-counter), ",%s:%d", new_nodes_list.node_info[i].name, new_nodes_list.node_info[i].num_proc);
            }

            fprintf(stderr, "adm_ss_reconfigure: noconcatenatedStringde=%s\n",concatenatedString);

            /* write the nodelist to file*/
            ssize_t bytesWritten = fwrite(concatenatedString, sizeof(char), strlen(concatenatedString), fd);

            /* Free the dynamically allocated memory*/
            free(concatenatedString);

            fclose(fd);      
            fprintf(stderr, "stoprestart_reconfigure(%d): Hostlist in /tmp/nek_malleability.res\n", getpid());

            res = ADM_ReleaseProcesses (processNodes);
            if (res != 0) return ICC_FAILURE;
            
            /* This should be done during the restart (after load the IC status) 
            signal_thread_rpc_release_nodes();*/

            /* Set global vars to its correct value for checkpoint*/
            ADM_GLOBAL_shrink = shrink;
            ADM_GLOBAL_checkpointing = 1;
        }
    }    

  /* check potential errors in conversions (14/01/2022) */
  return ICC_SUCCESS;
}

/**
 * Calls icc_init_mpi for stop and restart applications
 * Differences: app type and function to handle the communication with the IC.
*/
int command_rpc_init_ss(char *init_hostlist, char ** ip, char ** clid)
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
  fprintf(stderr, "**** JOBID = %d NNODES = %d MODE = %d HOSTLIST = %s\n", jobid, nnodes, ADM_APP_MODE, init_hostlist);

  //set init nodes on new_nodes_list before starting IC
  parse_add_command(init_hostlist, &new_nodes_list, 0, 0, processNodes); // CHANGE: JAVI

  pthread_mutex_lock(&mutex_rpc_icc);
  icc_init_mpi(ICC_LOG_DEBUG, ICC_TYPE_STOPRESTART, nnodes, adm_ss_reconfigure, NULL, ADM_APP_MODE, ip, clid, init_hostlist, &icc);
  if (icc == NULL)
          fprintf(stderr, "[application] Error connecting to IC\n");
  pthread_mutex_unlock(&mutex_rpc_icc);

  return 0;
}

/**
 * ALBERTO 17072023 icc_rpc_fini after MPI_Finalize
*/
int signal_thread_rpc_icc_fini(){
    int ret = 0;
    fprintf(stderr, "signal_thread_rpc_icc_fini called\n");
    pthread_mutex_lock(&mutex_rpc_call);
    while (launch_rpc_release_nodes != 0) {
        pthread_cond_wait(&cond_rpc_call_end, &mutex_rpc_call);
    }
    fprintf(stderr, "signal_thread_rpc_icc_fini: signal thread to remove job data in icc db\n");
    launch_rpc_release_nodes = 9;
    pthread_cond_signal(&cond_rpc_call);
    pthread_mutex_unlock(&mutex_rpc_call);
    return 0;
}

int command_rpc_icc_fini(){
    int ret = 0;
    fprintf(stderr, "command_rpc_icc_fini: removing job data in icc db\n");
    pthread_mutex_lock(&mutex_rpc_icc);
    ret = icc_fini(icc);
    pthread_mutex_unlock(&mutex_rpc_icc);

    if ((ret == ICC_SUCCESS) || (icc == NULL))
          fprintf(stderr, "[application] Error connecting to IC\n");

    fprintf(stderr, "[DEBUG] ICC fini done\n");
    return ret;
}


int command_rpc_release_register(char *hostname, int num_procs)
{
    int ret = 0;
    
    fprintf(stderr, "command_rpc_release_register: register nodes to remove: %s:%d\n",hostname,num_procs);
    pthread_mutex_lock(&mutex_rpc_icc);
    ret = icc_release_register(icc, hostname, num_procs);
    pthread_mutex_unlock(&mutex_rpc_icc);
    assert(ret == ICC_SUCCESS);
    
    return ret;
}

int command_rpc_release_nodes()
{
    int ret = 0;

    fprintf(stderr, "command_rpc_release_nodes: release nodes\n");
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
int command_rpc_init(char *init_hostlist, char *added_hostlist, char ** ip, char ** clid)
// CHANGE END
{
    /*IC*/
  int ret;
  int rpcret;

  uint32_t jobid, nnodes;
  char *slurm_jobid = getenv("SLURM_JOBID");
  char *slurm_nnodes = getenv("SLURM_NNODES");

  fprintf(stderr, "command_rpc_init(%d): init_hostlist=%s\n", getpid(),init_hostlist);
  if (added_hostlist) fprintf(stderr, "command_rpc_init(%d): added_hostlist=%s\n", getpid(),added_hostlist);

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
  fprintf(stderr, "**** JOBID = %d NNODES = %d MODE = %d\n", jobid, nnodes, ADM_APP_MODE);

  //set init nodes on new_nodes_list before starting IC
  fprintf(stderr, "command_rpc_init(%d): Begin parse_add_command\n", getpid());
  if (added_hostlist) parse_add_command(added_hostlist, &new_nodes_list, 0, 0, processNodes); // CHANGE: JAVI
  fprintf(stderr, "command_rpc_init(%d): End parse_add_command\n", getpid());

    
  int length=(init_hostlist?strlen(init_hostlist):0)+(added_hostlist?strlen(added_hostlist):0)+5;
  char *hostlist = (char *)malloc(length);
  assert(hostlist);
  bzero(hostlist,length);
  if ((init_hostlist==NULL) && (added_hostlist==NULL)) {
    free(hostlist);
    hostlist = NULL;
  } else if ((init_hostlist!=NULL) && (added_hostlist!=NULL)){
    snprintf(hostlist,length,"%s,%s",init_hostlist,added_hostlist);
  } else if (init_hostlist!=NULL) {
    strcpy(hostlist,init_hostlist);
  } else if (added_hostlist!=NULL) {
    strcpy(hostlist,added_hostlist);
  }
  pthread_mutex_lock(&mutex_rpc_icc);
  icc_init_mpi(ICC_LOG_DEBUG, ICC_TYPE_FLEXMPI, nnodes, flexmpi_reconfigure, NULL, ADM_APP_MODE, ip, clid, hostlist, &icc);
  //assert(icc != NULL);
  if (icc == NULL)
          fprintf(stderr, "[application] Error connecting to IC\n");
  pthread_mutex_unlock(&mutex_rpc_icc);
  free(hostlist);

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
        fprintf(stderr, "[application] Received packet from %s:%d   Data: %s\n\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);

        //truncate message
        char bufer_cropped [length+2];
        memset(bufer_cropped, 0, length+2);
        strncpy(bufer_cropped, buf, length);
        strcat(bufer_cropped, "\0"); // David: add termination string
        //Parse string
        parse_command(bufer_cropped, &command);
        
        fprintf(stderr, "[application] Command number is %d\n", command.command_n);
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

                    fprintf(stderr, "policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_obj_texec, EMPI_GLOBAL_obj_texec_threshold);
                    // fprintf(stderr, "policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);
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

                    fprintf(stderr, "policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_percentage, EMPI_GLOBAL_obj_texec_threshold);
                    // fprintf(stderr, "policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);
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

                    fprintf(stderr, "policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_obj_texec, EMPI_GLOBAL_obj_texec_threshold);
                    // fprintf(stderr, "policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);
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

                    fprintf(stderr, "policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_percentage, EMPI_GLOBAL_obj_texec_threshold);
                    // fprintf(stderr, "policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);
                    sprintf(buf_res, "policy: %s - obj = %f - threshold = %f\n", command.options[0], EMPI_GLOBAL_percentage, EMPI_GLOBAL_obj_texec_threshold);
                    // sprintf(buf_res, "policy: %s - obj = %d - threshold = %f\n", command.options[0], obj_texec, obj_texec_threshold);

                }

                else if (strcmp(command.options[0], "LBALANCE") == 0)
                {
                    EMPI_Set_policy(EMPI_LBALANCE);
                    //enable load balancing
                    EMPI_Enable_lbalance ();
                    fprintf(stderr, "policy: %s\n", command.options[0]);
                    sprintf(buf_res, "policy: %s\n", command.options[0]);

                }
                else if (strcmp(command.options[0], "MONITORDBG") == 0)
                {
                    EMPI_Set_policy(EMPI_MONITORDBG);
                    //enable load balancing
                    EMPI_Enable_lbalance ();
                    fprintf(stderr, "policy: %s\n", command.options[0]);
                    sprintf(buf_res, "policy: %s\n", command.options[0]);

                }
                else if (strcmp(command.options[0], "MALLEABILITY") == 0)
                {
                    EMPI_Set_policy(EMPI_MALLEABILITY);
                    //enable load balancing
                    EMPI_Enable_lbalance ();
                    fprintf(stderr, "policy: %s\n", command.options[0]);
                    sprintf(buf_res, "policy: %s\n", command.options[0]);

                }
                else if (strcmp(command.options[0], "MALLEABILITY_COND") == 0)
                {
                    EMPI_Set_policy(EMPI_MALLEABILITY_COND);
                    //enable load balancing
                    EMPI_Enable_lbalance ();
                    fprintf(stderr, "policy: %s\n", command.options[0]);
                    sprintf(buf_res, "policy: %s\n", command.options[0]);

                }
                else
                {
                    fprintf(stderr, "ERROR: Malleability policy %s NOT FOUND!", command.options[0]);
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

                fprintf(stderr, "Statistics: \n");
                char line[1024];
                EMPI_host_type* hostlist = EMPI_GLOBAL_hostlist;
                int index;
                for(index = 0; index < EMPI_GLOBAL_nhosts; index++)
                {
                    fprintf(stderr, " ** [%s] rtime is %lld, ptime is %lld, ctime is %f, flops is %lld \n", hostlist->hostname, EMPI_GLOBAL_monitoring_data.rtime, EMPI_GLOBAL_monitoring_data.ptime, EMPI_GLOBAL_monitoring_data.ctime,EMPI_GLOBAL_monitoring_data.flops);
                    fprintf(stderr, "\n   %s",line);
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

                fprintf(stderr, " Starting the termination of all processes \n" );
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock); // Only the server has an attached thread
                EMPI_GLOBAL_monitoring_data.termination=1;
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                break;
                
            case 6:  
                // Command 6: triggered execution 
                                
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                
                if(EMPI_GLOBAL_listrm[0] >0){ 
                  fprintf(stderr, " Command ignored: previous command already being processed. Try again later \n" );
                } 
                else{ // Asigna el incremento de procesos (deltaP) a cada clase    
                     i = 0;
                     do
                     {
                        for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
                            if (strcmp (EMPI_GLOBAL_hclasses[n], command.options[i]) == 0 && atoi(command.options[i+1])!=0) {
                                EMPI_GLOBAL_nprocs_class[0][n] = atoi(command.options[i+1]); // Delta proc (increment/decrement in the proc number)
                                fprintf(stderr, " Command: Create %d processes in compute node: %s \n",EMPI_GLOBAL_nprocs_class[0][n],EMPI_GLOBAL_hclasses[n]);
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
                    fprintf(stderr, " Command ignored: name of the events is too large  \n" );
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
                  fprintf(stderr, " Command ignored: previous command 9 already being processed with a delay of %f secs. Try again later \n",EMPI_GLOBAL_delayiotime );
                } 
                else{
                    EMPI_GLOBAL_delayio=1;
                    if(command.options[0] != NULL) {
                        EMPI_GLOBAL_delayiotime=atof(command.options[0]);
                    }
                    fprintf(stderr, " Delaying I/O phase %f seconds  \n",EMPI_GLOBAL_delayiotime);
                    
                }
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                break;
                
            case 10:  
                // Command 10: unlocks the I/O Phase
                fprintf(stderr, " Releasing the I/O phase   \n");
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                EMPI_GLOBAL_delayio=0;
                EMPI_GLOBAL_delayiotime=0;
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);    
                break;
                
            case 11:  
                // Command 11: triggered execution for a given iteration
                                
                pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
                
                if(EMPI_GLOBAL_listrm[0] >0){ 
                  fprintf(stderr, " Command ignored: previous command already being processed. Try again later \n" );
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
                                fprintf(stderr, " Command: Create %d processes in compute node: %s at iteration %d\n",EMPI_GLOBAL_nprocs_class[0][n],EMPI_GLOBAL_hclasses[n],EMPI_GLOBAL_listrm[0]);
                            }
                        }
                        i+=2;
                     } while (command.options[i] != NULL);
                }
                pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                break;
                
            case 12:  
                // Command 12: displays message from server
                fprintf(stderr, " Message from server received: %s  \n",buf);
                break;                
            default:
                break;
        }
       length = sendto(s, buf_res, strlen(buf_res), 0, (struct sockaddr *)&si_other, (socklen_t)slen);
       fprintf(stderr, "Sent %d bytes as response\n", length);
    }

    free(buf);
    close(s);
    return 0;
}

char * get_addr_ic_str(){
    return addr_ic_str;
}

char * get_client_id(){
    return clid;
}
