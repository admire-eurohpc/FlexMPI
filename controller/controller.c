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
 *    FlexMPI 3.1                                                                                                                            *
 *                                                                                                                                        *
 *    File:       controller.c                                                                                                            *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/
 
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include "controller.h"

// ElasticSearch
// #include <curl/curl.h>
// #include "connector.h"


void application_monitor(void *);
void execute_command(char *,int);
void diep(char *s);
void init_app(char *, int);

static const char *HOME;

struct timeval initial;

struct applicationset GLOBAL_app[MAX_APPS];
struct nodeset GLOBAL_node;

int GLOBAL_napps, GLOBAL_bachapps, GLOBAL_RECONF, GLOBAL_PREDEFSPEEDUP,GLOBAL_SYNCIOPHASES,GLOBAL_busyIO,GLOBAL_monitoring,GLOBAL_iter[MAX_APPS],GLOBAL_GUI,GLOBAL_GUI_PORT,GLOBAL_GUI_ListenerPort,GLOBAL_TERMINATION,GLOBAL_EARLYTERMINATION,GLOBAL_MONITOR,GLOBAL_MONITOR_PORT;
int GLOBAL_reqIO[MAX_APPS];
double GLOBAL_IOsize[MAX_APPS];
int GLOBAL_EXEC,GLOBAL_ADHOC, GLOBAL_RECORDEDSPEEDUP, GLOBAL_RECORDEDEXECTIMES,GLOBAL_MAXTHROUGHPUT,GLOBAL_FAIRSCHEDULING_NP,GLOBAL_SPEEDUPSCHEDULING_NP;
int GLOBAL_CLARISSEONLY,GLOBAL_FAIRSHEDULING,GLOBAL_SPEEDUPSCHEDULING,GLOBAL_IOSCHEDULING;
int GLOBAL_SCHEDULING_IO,GLOBAL_nhclasses,GLOBAL_SCHEDULING_ALGORITHM;

char GLOBAL_FILE[1024],GLOBAL_CONTROLLER_NODE[1024],GUI_NODE[1024],MONITOR_NODE[1024];
int newPort1, newPort2;

// Data structures for - prediction
double tstamp[NUMSAMPLES][MAX_APPS],tlong[NUMSAMPLES][MAX_APPS],cput[NUMSAMPLES][MAX_APPS],delay_v[NUMSAMPLES][MAX_APPS],delay_t[NUMSAMPLES][MAX_APPS];
double GLOBAL_delayt1[MAX_APPS],GLOBAL_delayt2[MAX_APPS],GLOBAL_IOSCHEDULING_THRESHOLD;  // Time samples 
int  numprocs[NUMSAMPLES][MAX_APPS],niter[NUMSAMPLES][MAX_APPS];
int cnt_app[MAX_APPS],update[MAX_APPS],cnt_delay[MAX_APPS];
double p_tstamp[STAMPSIZE][MAX_APPS],p_tlong[MAX_APPS]; // Predicted values
int napps=0; 
pthread_mutex_t CONTROLLER_GLOBAL_server_lock;
int reset[MAX_APPS];
double maxOverlap=6,uncertain=2;
 
// Performance metrics
double GLOBAL_epoch[NUMSAMPLES];
int cnt_epoch;
int cnt_perf[MAX_APPS];           // cnt_perf[num_apps]
double exectime[MAX_APPS][BUFSIZE]; // exectime[num_apps][num_samples]
int exectorig[MAX_APPS][BUFSIZE]; // exectorig[num_apps][num_samples]
double speedup1[MAX_APPS][BUFSIZE][8],speedup2[MAX_APPS][BUFSIZE][8]; // speedup[num_apps][num_samples][num_values]
int cnt_speedup1[MAX_APPS],cnt_speedup2[MAX_APPS];            // cnt_speedup1[num_apps]
int flag_speedup[MAX_APPS];

// Global values
int Tconf=2; // Global reconfiguration time
int terminated_app,GLOBAL_pending = 1;
int GLOBAL_terminated_app_id[MAX_APPS];
int GLOBAL_totalcores;
int GLOBAL_DEBUG;
int GLOBAL_DYNAMIC_ALLOC;
int GLOBAL_nodealloc=0;
int GLOBAL_VERVOSE=0;
int GLOBAL_disable;
int GLOBAL_suppression_time=180;
int GLOBAL_forwarding=0;

typedef struct EMPI_Spawn_data {
    int  dirty;  //indicates if we have fresh spawn data
    int  hostid; //host id
    int  nprocs; //max running procs
    char name [256]; //host name
} EMPI_Spawn_data;

typedef struct command_flexmpi {
    int    command_n;
    char * options[NUMBER_OPTIONS];
} command_flexmpi;

typedef struct service_arguments {
    int socket;
    struct sockaddr_in address;
} service_arguments;

typedef struct performance_metrics{
    double metric[9];
    char  nhwpc_1[256]; // Hardware counter metric name 1
    char  nhwpc_2[256]; // Hardware counter metric name 1
} performance_metrics;

typedef struct new_performance_metrics{
    double metric[12];
} new_performance_metrics;

struct arg_struct1 {
    int id;
    int port2;
};

struct arg_struct2 {
    int id;
    int *flag_speedup;
};

struct arg_struct3 {
    int id;
    char counter_list[1024];
    struct new_performance_metrics *new_perf_metrics;
    int init;
};

struct performance_metrics perf_metrics[MAX_APPS][NUMSAMPLES];                          // perf_metrics[num_apps][num_samples]
struct new_performance_metrics init_perf_metrics[MAX_APPS],curr_perf_metrics[MAX_APPS]; // init_perf_metrics[num_apps]


// Checks the return values of posix calls
void check_posix_return(int rc, char* cause)
{
    if (rc != 0)
    {
        printf("\nError: %s :[%s]", cause, strerror(rc));
    }
    else
    {
        printf("   %s successfully\n", cause);
    }
}

void clean_nodes(char * node){
    char *token, *saveptr;
    char nodelist[1024];
    int i;
    
    strcpy(nodelist,node);
    
    token = strtok_r(nodelist,":",&saveptr);
    while(token!=NULL){       
        for(i=0;i<NCLASES;i++){   
           pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
           if(strcmp(GLOBAL_node.hclasses[i],token)==0){
               GLOBAL_node.tstamp[i]=0;
               printf(" @@ Cleaning timer of node: %s\n",token);
           }   
           pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
        }
        token = strtok_r(NULL,":",&saveptr);  
    }
}


void set_node(char * node){
    int i;
    struct timeval timestamp1;
    uint64_t delta_t;
    gettimeofday(&timestamp1, NULL);
    delta_t = (timestamp1.tv_sec - initial.tv_sec) * 1000 + (timestamp1.tv_usec - initial.tv_usec) / 1000;
    delta_t=delta_t/1000;
    for(i=0;i<NCLASES;i++){
        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
        if(strcmp(GLOBAL_node.hclasses[i],node)==0){
            GLOBAL_node.tstamp[i]=delta_t;
        }
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
    }
}

int get_node(char * node){
    int i,flag;
    struct timeval timestamp1;
    uint64_t delta_t;
    gettimeofday(&timestamp1, NULL);
    delta_t = (timestamp1.tv_sec - initial.tv_sec) * 1000 + (timestamp1.tv_usec - initial.tv_usec) / 1000;
    delta_t=delta_t/1000;
    flag=1;
    for(i=0;i<NCLASES;i++){
        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
        if(strcmp(GLOBAL_node.hclasses[i],node)==0){
            if(GLOBAL_node.tstamp[i]!=0 && (GLOBAL_node.tstamp[i]+GLOBAL_suppression_time)>delta_t){
                flag=0;
            }
        }
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
    }
    return(flag);
} 



int check_exclusive(char * input){
    int i,exclusive;
    const char *options1[2][512],*options2[2][512];;
    options1[0][0] = "compute-9-1";
    options1[1][0] = "compute-9-2";
    options2[0][0] = "compute-11-8";
    exclusive=0;
    
    for (i=0;i<2;i++){
        if(strcmp(input,options1[i][0])==0){ 
            exclusive=1;
        }
    }
    
    for (i=0;i<1;i++){
        if(strcmp(input,options2[i][0])==0){ 
            exclusive=2;
        }
    }
    return(exclusive);
}

// Channel=0 -> FlexMPI channel
// Channel=1 -> Application channel
void send_message(char *msg, int id, int channel, int delay){
    int port;
    char appnode[512],command1[1024];
    
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);       
    port=GLOBAL_app[id].port1;
    strcpy(appnode,GLOBAL_app[id].node);
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);       
    if(channel==1) port=port+2000;
    
    // Activates application monitoring
    sprintf(command1,"nping --udp -p %d -c 1 %s --data-string \"%s\" > /dev/null",port,appnode,msg);
    execute_command(command1,delay);
    printf(" ** [%d] Sending command:  %s \n",id,msg);     
}
void spawn(int id, char *node, int num){
    char command[1024];
    int i,nodeid;
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);     
    
    // Finds the applications running in node
    for(i=0;i<GLOBAL_app[id].nhclasses;i++){
        if(strcmp(GLOBAL_app[id].hclasses[i],node)==0){
            nodeid=i;
        }
    }
    GLOBAL_app[id].nprocs_class[nodeid]+=num;      // Total number of processes
    GLOBAL_app[id].newprocs_class[nodeid]+=num;    // Spawn processes
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);    
    
    sprintf(command,"6:%s:%d",node,num);
    send_message(command, id, 0, 0);
}


// compares two strings ignoring extra characters of str2
int is_different(char *str1,char *str2){
    int i,flag,l1,l2;
    l1=strlen(str1);
    l2=strlen(str2);
    flag=0;
    if(l2<l1) return(1);
    for(i=0;i<l1;i++){     
        if(i<l2 && str1[i]!=str2[i]) flag=1;
    }
    return(flag);
}

int event_mapper(char *event){
    int val=0;
    if(event!=NULL){
        if(strcmp(event,"PAPI_L3_TCM")==0) val=8;
        if(strcmp(event,"PAPI_RES_STL")==0) val=9;
        if(strcmp(event,"PAPI_LD_INS")==0) val=10;
        if(strcmp(event,"PAPI_SR_INS")==0) val=11;
    }
    return(val);
}
void execute_command(char *icommand,int sleeptime){
    char command[1024];
    
    sleep(sleeptime);  
    strcpy(command,icommand);
    fflush(stdout);
    system(command);
}



// app_id=-1 waits for num_samples for all applications
// app_id= i waits for num_samples for i-th application
void wait_for_samples(int num_samples,int app_id){
    int id,tmp,diff;
    int tmp2[GLOBAL_napps];
    printf(" Waiting for %d samples before configuring \n",num_samples);
    for(id=0;id<GLOBAL_napps;id++){
            tmp2[id]=cnt_app[id];
    }
    do{
        tmp=-1;
        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);       
        // Set id to 0 ********************************** 
        for(id=0;id<GLOBAL_napps;id++){
            if(app_id==-1 || id==app_id){
                diff=(cnt_app[id]-tmp2[id]);
                if(diff<tmp || tmp==-1) tmp=diff;
            }
        }
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);    
        sleep(10);
    } while (tmp<num_samples);
    printf(" Samples taken \n");
    execute_command("date",0);
    execute_command("rsh compute-11-5 \"vmstat -w\"",0);
    execute_command("rsh compute-11-5 \"vmstat -s\"",0);
    execute_command("rsh compute-11-5 \"mpstat -A | head -32\"",0);
    execute_command("rsh compute-11-5 \"mpstat -A | tail -13\"",0);
    execute_command("rsh compute-11-5 \"cat /proc/meminfo\"",0);
    execute_command("rsh compute-11-5 \"cat /proc/vmstat\"",0);
    

}
  
void  printstats(){
   
    int n,i;
    double acum_cpu,acum_tlong,acum_delay;
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);

     printf("\n ************* EXECUTION STATISTICS ************* \n");
     printf("Application \t [Tot. CPUt] \t [Tot. IOt] \t [Tot. Delayt] \t [CPUt per it.] [IOt per it.] \t [Delayt  per it.] \n");
    
    for(n=0;n<GLOBAL_napps;n++){
        acum_cpu=0;
        acum_tlong=0;
        acum_delay=0;
        for(i=0;i<cnt_app[n];i++) acum_cpu+=cput[i][n];
        for(i=0;i<cnt_app[n];i++) acum_tlong+=tlong[i][n];
        for(i=0;i<cnt_delay[n];i++) acum_delay+=delay_v[i][n]; 
        printf("    %d \t\t %8.2f \t %8.2f \t %8.2f \t %8.2f \t %8.2f \t %8.2f \n",n,acum_cpu,acum_tlong,acum_delay,acum_cpu/cnt_app[n],acum_tlong/cnt_app[n],acum_delay/cnt_app[n]);
    }
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
    printf("\n \n");
        
}

void check_node()
{
    FILE *file;
    char command[256],nodename1[256];

    printf("\n Setting the node names..... ");
    
    // Opens the nodename generated by workloadgen
    
    sprintf(command,"uname -a | awk '{print $2}' > controller.dat");
    system(command);
    
    // Opens the actual nodename 
    if ((file = fopen ("controller.dat", "r")) == NULL) {
        fprintf (stderr, "\nError opening controller.dat in check_node %s \n","controller.dat");
        exit(1);
    }
    fscanf (file, "%s\n", nodename1);
    fclose(file);
    
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
    strcpy(GLOBAL_CONTROLLER_NODE,nodename1);
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
    printf(" Ok\n ");        
    

 }
 


// Provides the number of cores in use of a given compute node
// Note: this function has to be called inside a critical section (protected by lock/unlock)
// Iter>0, iteration to apply the reconf. iter=0, reconf. applied immediately
// Policy = 0, reconf in a shared compute node,  policy == 1 reconf. applied immediately when delpa_p>0 in a isolated compute node
// tot_p the actual number of processes to be created. If tot_p=0, then delta_p are created
// Output = -1:: not enough resources   Output = 0:: command completed
int allocnodes(int appid, int delta_p, char* command, int iter, int tot_p, int policy){    
    int n,i,j,k;
    int num_classes,max,min,maxindex,cnt,flag,avail_cpus,cnt_procs; 
    int tmp_delta_p;
    int cnt1,cnt2,max1,max2,maxindex1,maxindex2,exclusive;
    int val1[NCLASES],val2[NCLASES],index[NCLASES],index1[NCLASES],index2[NCLASES],tmpval[NCLASES],tmpval1[NCLASES],tmpval2[NCLASES];
    char tmpcommand[1024],output[1024],tmpstring[1024];
    FILE *file1;
    char token1[] = ":",*record = NULL,readline[1000];
    char *save_ptr1;
    int created_p;
    
    if(tot_p==0){
        created_p=delta_p;
    }
    else{
        created_p=tot_p;
        if(tot_p<delta_p){
            printf(" \n Error in fn allocnodes, tot_p %d should be >= than delta_p %d",tot_p,delta_p);
            return(-1);
        }
    }
    if(policy!=0 && policy!=1  && policy!=2){
        printf(" \n Not valid policy %d \n",policy);
        return(-1);
    }
        
   // Parses the node file if global node structure has not been created yet
    if(GLOBAL_nhclasses==0){
        
        if ((file1 = fopen ("../run/nodefile2.dat", "r")) == NULL) {
            fprintf (stderr, "\nError opening nodefile in allocnodes \n");
            exit(1);
        }   

        // Read and parse node file that contains the node file names
        // Creates the global node structure 
        n=0;
        while (fscanf (file1, "%s\n", readline) != EOF) {
            //Get values
            record = strtok_r (readline, token1, &save_ptr1);  // Node name
            if(record==NULL){fprintf (stderr, "\nError1 parsing allocnodes \n");exit(1); }
            strncpy(GLOBAL_node.hclasses[n],record,strlen(record));
            
            record = strtok_r (NULL, token1, &save_ptr1);  // Num procs. 
            if(record==NULL){fprintf (stderr, "\nError3 parsing allocnodes \n");exit(1); }
            GLOBAL_node.ncores_class[n]=atoi(record);
            GLOBAL_node.used_cores[n]=0;   
            
            record = strtok_r (NULL, token1, &save_ptr1);  // Node alias
            if(record==NULL){fprintf (stderr, "\nError2 parsing allocnodes \n");exit(1); }
            strncpy(GLOBAL_node.rclasses[n],record,strlen(record));
            
            GLOBAL_node.exclusive[n]=check_exclusive(GLOBAL_node.hclasses[n]);
            GLOBAL_node.tstamp[n]=0;
            GLOBAL_node.updated[n]=0;
            
            n++;
        }
        fclose(file1);
        GLOBAL_nhclasses=n;
    }
    
    // New application
    if(appid<0){
        
        // Checks if there are enough resources
        avail_cpus=0;
        for (i=0;i<GLOBAL_nhclasses;i++){
            if(GLOBAL_node.exclusive[i]==policy){
                avail_cpus+=GLOBAL_node.ncores_class[i]-GLOBAL_node.used_cores[i];
            }
        }
        if(created_p>avail_cpus){
            printf(" ### I/O allocnodes: not enough resources (%d required vs %d existing). Reconfiguring action canceled \n",delta_p,avail_cpus);
            return(-1);                    
        }
        
        // Available cores
        for (i=0;i<GLOBAL_nhclasses;i++){
            tmpval[i]=GLOBAL_node.ncores_class[i]-GLOBAL_node.used_cores[i];
        }
        
        // Sorts the existing compute nodes according its use:    
        for (i=0;i<GLOBAL_nhclasses;i++){
            max=-1;
            for (j=0;j<GLOBAL_nhclasses;j++){
                if(tmpval[j]>max || max==-1) {
                    max=tmpval[j];
                    maxindex=j;
                }
            }
            index[i]=maxindex;
            tmpval[maxindex]=-100;
        }

        for (i=0;i<GLOBAL_nhclasses;i++){
            tmpval[i]=GLOBAL_node.ncores_class[i]-GLOBAL_node.used_cores[i]; // Available cores
            tmpval2[i]=0;  // Processes per class
        }
        
        tmp_delta_p=delta_p;
        // Only for applications with more procs that one node. 
        if(tot_p>0){
            // Process in ascending order when tmp_delta_p is larger than the node capacity
            for (j=0;j<GLOBAL_nhclasses;j++){
                i=index[j];
                exclusive=GLOBAL_node.exclusive[i];

                // Uses at least one complete compute node
                //if(tmp_delta_p>=tmpval[i] && policy==exclusive){
                // Uses at least one complete compute node that has to be empty
                if(tmp_delta_p>=tmpval[i] && policy==exclusive && tmpval[i]==GLOBAL_node.ncores_class[i]){
                    // GLOBAL_node.used_cores[i] updated in parse_malleability
                    tmpval2[i]+=tmpval[i];
                    if(tmp_delta_p>=tmpval[i]){
                        tmp_delta_p-=tmpval[i];
                        tmpval[i]=0;
                    }
                    else{
                        tmp_delta_p=0;
                        tmpval[i]=tmpval[i]-tmp_delta_p;
                    }
                    
                }
            }
        }  
             
             
        // OLD: Process in descending order when tmp_delta_p is smaller than the node capacity
        //for (j=GLOBAL_nhclasses-1;j>=0;j--){
        // NEW: Process in descending order when tmp_delta_p is smaller than the node capacity
        // -> Small applications will be allocated in a exclusive compute node
        for (j=0;j<GLOBAL_nhclasses;j++){    
            i=index[j];
            exclusive=GLOBAL_node.exclusive[i];
            if(1 || (tmp_delta_p<=tmpval[i] && policy==exclusive && tmpval[i]==GLOBAL_node.ncores_class[i])){
            //if(tmp_delta_p<=tmpval[i] && policy==exclusive){
                // GLOBAL_node.used_cores[i] updated in parse_malleability
                tmpval2[i]+=tmp_delta_p;
                tmpval[i]=tmpval[i]-tmp_delta_p;
                tmp_delta_p=0;
            }
        }

            
        if(tmp_delta_p>0){
            printf(" ### Alloc nodes: not enough resources. Reconfiguration action canceled %d %d \n",tmp_delta_p,appid);
            return(-1);                    
        }
        
        strcpy(command,"");
        // Creates the allocation list
        for (j=0;j<GLOBAL_nhclasses;j++){
            i=index[j];
            if(tmpval2[i]>0){
                
                if(strlen(command)>1){
                    sprintf(tmpcommand,":%s:%d",GLOBAL_node.hclasses[i],tmpval2[i]);
                }
                else{
                    sprintf(tmpcommand,"%s:%d",GLOBAL_node.hclasses[i],tmpval2[i]);
                }
                strcat(command,tmpcommand);
                
            }
        }  
    }   
    //
    // appid>0, Existing application
    // 
    else{
        
        num_classes=GLOBAL_app[appid].nhclasses;
        
        for(i=0;i<num_classes;i++){
            val1[i]=0;
            val2[i]=0;
        }
        
        // Obtains the cores in use per application
        for (n=0;n<GLOBAL_napps;n++){
            for(i=0;i<num_classes;i++){
                if(n==appid)    val1[i]=val1[i]+GLOBAL_app[n].nprocs_class[i];
                else            val2[i]=val2[i]+GLOBAL_app[n].nprocs_class[i];
            }
        }
        
        // Spawned process 
        if(delta_p>0){
              
            // Sorts the list according the application processes
            for (i=0;i<num_classes;i++) tmpval1[i]=val1[i]; // App processes
            for (i=0;i<num_classes;i++) tmpval2[i]=val1[i]+val2[i]; // Total processes

            // Takes the nodes with maximum values
            cnt1=0;
            cnt2=0;
            for (i=0;i<num_classes;i++){
                max1=-100;
                max2=-100;
                
                for (j=0;j<num_classes;j++){
                    
                    exclusive=check_exclusive(GLOBAL_app[appid].hclasses[j]);
                    // First criterium: application number of processes
                    if(tmpval1[j]>max1 && exclusive==policy && tmpval1[j]>0) {
                        max1=tmpval1[j];
                        maxindex1=j;
                    }
                    
                    // Second criterium: total number of processes
                    if(tmpval2[j]>max2 && exclusive==policy) {
                        max2=tmpval2[j];
                        maxindex2=j;
                    }
                }
                if(max1!=-100){
                    index1[cnt1]=maxindex1;
                    tmpval1[maxindex1]=-100;
                    cnt1++;
                }
                if(max2!=-100){
                    index2[cnt2]=maxindex2;
                    tmpval2[maxindex2]=-100;
                    cnt2++;
                }           
            }
            
            // Uses the nodes with greatest number of local processes

            for (i=0;i<num_classes;i++) tmpval[i]=0;
            
            // Selects according the first criterium
            cnt=0;
            j=0;           
            while(cnt<delta_p && j<cnt1){
                i=index1[j];
                if(val1[i]+val2[i]+tmpval[i]<GLOBAL_app[appid].ncores_class[i]){ // David: check
                    tmpval[i]++;
                    cnt++;
                }
                else{
                    j=j+1;
                }    
            }
            
            // Selects according the second criterium
            j=0;           
            while(cnt<delta_p && j<cnt2){
                i=index2[j];
                if(val1[i]+val2[i]+tmpval[i]<GLOBAL_app[appid].ncores_class[i]){ // David: check
                    tmpval[i]++;
                    cnt++;
                }
                else{
                    j=j+1;
                }    
            }    
            
        }
        
        // Process destruction
        else if(delta_p<0){
            
            for (i=0;i<num_classes;i++) tmpval2[i]=GLOBAL_app[appid].newprocs_class[i]; // Only considers the spawned processes of the current application
        
            // Sorts the list new use the permutation vector index);
            cnt1=0;
            for (i=0;i<num_classes;i++){
                min=1000000;
                for (j=0;j<num_classes;j++){
                    
                   exclusive=check_exclusive(GLOBAL_app[appid].hclasses[j]);
                   if(tmpval2[j]<min && exclusive==policy) {
                    min=tmpval2[j];
                    maxindex=j;
                   }
                }
                if(min!=1000000){
                    index[cnt1]=maxindex;
                    tmpval2[maxindex]=1000000;
                    cnt1++;
                }
            }

            // Uses the nodes with greatest number of local processes
            cnt=0;
            j=0;
            
            for (i=0;i<num_classes;i++){
                tmpval[i]=0;
                tmpval2[i]=GLOBAL_app[appid].newprocs_class[i]; // Only considers the spawned processes of the current application
            } 
            
            while(cnt>delta_p && j<cnt1){
                i=index[j];
                if(tmpval2[i]>0){
                    tmpval2[i]--;  // Absolute (total) values
                    tmpval[i]--;   // Relative (incremented) values
                    cnt--;
                }
                else{
                    j=j+1;
                }    
            }
        }
        
        // delta_p==0;
        else{
            for (i=0;i<NCLASES;i++){
                tmpval[i]=0;
            }
        }
            
        if(j==num_classes){
            //printf("    # I/O Scheduler error1: not enough resources: %d \n",delta_p);
            return(-1);
        }
        
        // Evaluates if the number of procs was obtained
        cnt_procs=0;
        for (i=0;i<num_classes;i++){
            cnt_procs+=tmpval[i];
        }
        if(cnt_procs!=delta_p){
            printf("    # I/O Scheduler error2: not enough resources for application %d: %d vs %d \n",appid,cnt_procs,delta_p);
            return(-1);
            
        }
        
        // Updates output classes
        for (i=0;i<num_classes;i++){
            GLOBAL_app[appid].nprocs_class[i]+=tmpval[i];      // Total number of processes
            GLOBAL_app[appid].newprocs_class[i]+=tmpval[i];    // Spawn processes
            GLOBAL_app[appid].nprocs+=tmpval[i];      // Total number of processes
            // Updates the globalnode table
            for(j=0;j<GLOBAL_nhclasses;j++){
                if(strcmp(GLOBAL_app[appid].hclasses[i],GLOBAL_node.hclasses[j])==0){
                    GLOBAL_node.used_cores[j]+=tmpval[i];
                }
            }
            
        }
        
        printf("         ### Node allocation for application %d: Spawn processes: %d\n",appid,delta_p);
        printf("         ### NodeName \t\t   Tot creat. \t Allocated \t\t Used by other \n");
        for (i=0;i<num_classes;i++){
            strcpy(output,"");
            for(j=0;j<GLOBAL_napps;j++){
                for(k=0;k<GLOBAL_app[j].nhclasses;k++){
                    if(strcmp(GLOBAL_app[appid].hclasses[i],GLOBAL_app[j].hclasses[k])==0 && GLOBAL_app[j].nprocs_class[k]!=0){
                        sprintf(tmpstring,"[%d:%d:%s]",j,GLOBAL_app[j].nprocs_class[k],GLOBAL_app[j].appname);
                        strcat(output,tmpstring);
                    }
                }
            }            
            printf("         ### %s  \t\t %.0d \t\t %.0d \t\t %.0d \t %s \n",GLOBAL_app[appid].hclasses[i],val1[i],GLOBAL_app[appid].newprocs_class[i],val2[i],output);
        }
        printf("\n\n");
        
        /*
        printf("         ### Node allocation for application %d: Spawn processes: %d\n",appid,delta_p);
        printf("         ### Application [%d] :  ",appid);
        for (i=0;i<num_classes;i++) printf("\t%d",val1[i]);
        printf("\n");
        printf("         ### Tot spawmned[%d] :  ",appid);
        for (i=0;i<num_classes;i++) printf("\t%d",GLOBAL_app[appid].newprocs_class[i]);
        printf("\n");
        printf("         ### Used by other   :  ");
        for (i=0;i<num_classes;i++) printf("\t%d",val2[i]);
        printf("\n");
        printf("         ### Allocated       :  ");
        for (i=0;i<num_classes;i++) printf("\t%d",tmpval[i]);
        printf("\n");
        */
        
        //for (i=0;i<num_classes;i++){
        //    printf("         ### Class [%d]:: %s %s\n",i,GLOBAL_app[appid].hclasses[i],GLOBAL_app[appid].rclasses[i]);
        //}
        
        // Generates output string
        if(iter<=0){
            sprintf(command,"6");     
        }
        else{
            sprintf(command,"11:%d",iter);
        }
        
        flag=0;
        for (i=0;i<num_classes;i++){
            if(tmpval[i]!=0){
                flag=1;
                sprintf(tmpcommand,":%s:%d",GLOBAL_app[appid].rclasses[i],tmpval[i]);
                strcat(command,tmpcommand);
            } 
        } 
        
        if(flag==0){ // Reconfiguration is not performed
            printf(" ### Alloc nodes: no change in the number of processes. Reconfiguring action canceled \n");
            return(-1);        
        }
        sprintf(tmpcommand,":");
        strcat(command,tmpcommand);
    }

    //printf("         ### Output string   :  %s \n",command);    
    return(0);
}




// Kills all the active applications
void killall(int id)
{
    int n,err,initialSocket,length;
    char initMsg[10],stringport[1000];
    sprintf(initMsg,"5:"); // This is the kill command
    
    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_ADDRCONFIG;
    struct addrinfo* res=0;

    
    for (n=0;n<GLOBAL_napps;n++)
    {
        if(id==-1 || id==n){
            // Parses the destination application
            sprintf(stringport,"%d",GLOBAL_app[n].port1);
            err=getaddrinfo(GLOBAL_app[n].node,stringport,&hints,&res);
            if (err<0) {
                diep("failed2 to");
            }
            
            // Creates a socket to the destination
            initialSocket=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
            if (initialSocket==-1) {
                diep("Error creating socket.");
            }
            
            // Sends the command
            length = sendto(initialSocket,initMsg,strlen(initMsg),0,res->ai_addr,res->ai_addrlen);
            if (length < 0){
                diep("Error: sendto()");
            }
            
            // Closes the socket
            close(initialSocket);
            
        }
    }
}


void intHandler(int dummy) {
    killall(-1);
    exit(1);
}

void diep(char *s)
{
  perror(s);
  killall(-1);
  exit(1);
}


int EMPI_GLOBAL_perform_load_balance;

// This function reads the workload files and creates the environment (conf files + execution scripts) and runs the applications
int parse_command (char *readline, char *command, int appid) 
{
    char nodelist[1024];
    char *record = NULL;
    char token1[]=":";
    int nprocs=0,error;
    char *save_ptr1 = NULL;
    int corespernode;
    
    // printf(" --> Input  command: %s \n",readline);

        //Get application name
        record = strtok_r (readline, token1, &save_ptr1);
        strcpy(command,record);
        strcat(command,":");

        // Obtains the application size
        record = strtok_r (NULL, token1, &save_ptr1);
        strcat(command,record);
        strcat(command,":");
        
        // Obtains the number of CPU iterations
        record = strtok_r (NULL, token1, &save_ptr1);
        strcat(command,record);
        strcat(command,":");
        
        // Obtains the number of COMMUNICATION iterations
        record = strtok_r (NULL, token1, &save_ptr1);
        strcat(command,record);
        strcat(command,":");

        // Obtains the number of IO iterations
        record = strtok_r (NULL, token1, &save_ptr1);
        strcat(command,record);
        strcat(command,":");
        
        // Obtains the value of IO action
        record = strtok_r (NULL, token1, &save_ptr1);
        strcat(command,record);
        strcat(command,":");

        // Obtains the number of iterations
        record = strtok_r (NULL, token1, &save_ptr1);
        strcat(command,record);
        strcat(command,":");
        
        // Following entry is the initial number of processes of the application
        record = strtok_r (NULL, token1, &save_ptr1);
        nprocs = atoi(record)*1; 
   
    // Uses one compute node and the rest of the processes are spawn
    corespernode=12; // David: 12 ToDo: obtain automatically
    if(nprocs>corespernode){ 
        do{
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);        
            error=allocnodes(-1, corespernode, nodelist, 0, nprocs, 0);
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);  
            if(error!=-1){
                strcat(command,nodelist);
                strcat(command,nodelist);
                init_app(command,nprocs);
                clean_nodes(nodelist);
            }        
            else{
                sleep(60);
            }
        }while(error==-1);
        
    }
    else{
        do{
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);        
            error=allocnodes(-1, nprocs, nodelist, 0, 0, 0);
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock); 
            if(error!=-1){
                strcat(command,nodelist);
                init_app(command,0);
                clean_nodes(nodelist);
            }   
            else{
                sleep(60);
            }
        }while(error==-1);
    }
    
    return(error);
}

// This function reads the workload files and creates the environment (conf files + execution scripts) and runs the applications
void Parse_malleability (char *filename1, char  *filename2, char *command, int benchmark_mode, int deploy) 
{
    int  nprocs=0,i,n,m,work;
    char readline[1000],*readcommand, output[2000],line[2000],token1[] = ":",token2[] = "\n", *record = NULL;
    char *save_ptr1, *save_ptr2;
    int port1,port2,nfile;
    FILE *file1,*file2;
    int index[NCLASES],tmpval[NCLASES];
    char LOCAL_hclasses[NCLASES][256]; //host classes
    char LOCAL_rclasses[NCLASES][256]; //host classes
    int  LOCAL_nprocs_class[NCLASES];
    int  LOCAL_ncores[NCLASES];
    int LOCAL_nhclasses;
    int Nsize,NIO,NCPU,NCOM,NumIter;
    double IOaction;
    uint64_t delta_t;
    struct timeval timestamp1;
    int appclass,profile;
        
    // Parses the node file
    if ((file1 = fopen (filename2, "r")) == NULL) {
        fprintf (stderr, "\nError opening nodefile in Parse_malleability %s \n",filename2);
        exit(1);
    }   

    for(n=0;n<NCLASES;n++){
        LOCAL_ncores[n]=0;
        memset(LOCAL_hclasses[n],0,256);
        memset(LOCAL_rclasses[n],0,256);        
    }
    
    
    
    // Read and parse node file that contains the node file names
    // Creates the global node structure if it was not created before
    LOCAL_nhclasses=0;
    while (fscanf (file1, "%s\n", readline) != EOF) {
        //Get values
        record = strtok_r (readline, token1, &save_ptr1);  // Node name
        if(record==NULL){fprintf (stderr, "\nError1 parsing Parse_malleability %s\n",filename2);exit(1); }
        strncpy(LOCAL_hclasses[LOCAL_nhclasses],record,strlen(record));
        record = strtok_r (NULL, token1, &save_ptr1);  // Num procs. 
        if(record==NULL){fprintf (stderr, "\nError3 parsing Parse_malleability %s\n",filename2);exit(1); }
        LOCAL_ncores[LOCAL_nhclasses]=atoi(record);
        record = strtok_r (NULL, token1, &save_ptr1);  // Node alias
        if(record==NULL){fprintf (stderr, "\nError2 parsing Parse_malleability %s\n",filename2);exit(1); }
        strncpy(LOCAL_rclasses[LOCAL_nhclasses],record,strlen(record));
        
        // Checks whether global node structure has been created
        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock); 
        if(GLOBAL_nhclasses==0){
            strncpy(GLOBAL_node.hclasses[LOCAL_nhclasses],LOCAL_hclasses[LOCAL_nhclasses],strlen(LOCAL_hclasses[LOCAL_nhclasses])); 
            strncpy(GLOBAL_node.rclasses[LOCAL_nhclasses],LOCAL_rclasses[LOCAL_nhclasses],strlen(LOCAL_rclasses[LOCAL_nhclasses])); 
            GLOBAL_node.ncores_class[LOCAL_nhclasses]=LOCAL_ncores[LOCAL_nhclasses];
            GLOBAL_node.used_cores[LOCAL_nhclasses]=0;   
            GLOBAL_node.exclusive[LOCAL_nhclasses]=check_exclusive(GLOBAL_node.hclasses[LOCAL_nhclasses]);
            GLOBAL_node.tstamp[LOCAL_nhclasses]=0;
            GLOBAL_node.updated[LOCAL_nhclasses]=0;
        }
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
        LOCAL_nhclasses++;
    }
    fclose(file1);
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock); 
    GLOBAL_nhclasses=LOCAL_nhclasses;
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);

    
    // Computes the total number of existing cores
    GLOBAL_totalcores=0;
    for(n=0;n<NCLASES;n++){
        GLOBAL_totalcores=GLOBAL_totalcores+LOCAL_ncores[n];
    }
    
    // Initial value of the communication ports
    port1=7668+2*GLOBAL_napps; // Listener
    port2=7669+2*GLOBAL_napps; // Sender
    
    
    // Opens workload file
    if(filename1!=NULL){
        if ((file1 = fopen (filename1, "r")) == NULL) {
            fprintf (stderr, "\nError in EMPI_Parse_malleability file1 opening\n");
            exit(1);
        }
    }
    else if (command==NULL){
        printf(" Parse malleability error: no input file nor command... exiting \n");
        return;
    }

    // Read and parse workload file
    nfile=GLOBAL_napps;
    
    work=0;
    if(filename1!=NULL){
        if(fscanf (file1, "%s\n", readline) != EOF) work=1;
    }
    else{
        readcommand=strtok_r (command, token2, &save_ptr2);
        if(readcommand!=NULL) work=1;
        strcpy(readline,readcommand);
    }
    
    while (work) {
        memset(output, 0, 2000);
        
        // Sets the local structure
        for (n = 0; n < LOCAL_nhclasses; n ++) {
            LOCAL_nprocs_class[n] = 0; 
        }    
        
        //Get values
        record = strtok_r (readline, token1, &save_ptr1);
        nprocs=0;
        appclass=0;
        profile=0;

        
        // First entry is the application name
        if (strcmp (record, "jacobi") == 0) appclass=1;  // Jacobi cpu
        if (strcmp (record, "cg1") == 0) appclass=2; // Congugate gradient low locality
        if (strcmp (record, "cg2") == 0) appclass=6; // Congugate gradient high locality
        if (strcmp (record, "epigraph") == 0) appclass=3; // Epigraph mem
        if (strcmp (record, "irreg") == 0) appclass=4; // Jacobi mem
        if (strcmp (record, "ioserver") == 0) appclass=7; // Jacobi mem
        if (strcmp (record, "dynamic") == 0) { // Dynamic loader
            profile=0;
            appclass=8; 
        }
        if (strcmp (record, "mixed") == 0) {
            profile=0;
            appclass=5; // Mixed workload
        }
        if (strcmp (record, "mixed6") == 0){ // Mixed workload, CPU-profile
            appclass=5;
            profile=6;
        }  
        if (strcmp (record, "mixed7") == 0){ // Mixed workload, irreg-mem-profile
            appclass=5;
            profile=7;
        }  
        if (strcmp (record, "mixed8") == 0){ // Mixed workload, CPU&COMM-profile
            appclass=5;
            profile=8;
        }  
        if (strcmp (record, "mixed9") == 0){ // Mixed workload, reg-mem-profile
            appclass=5;
            profile=9;
        }  
        if (strcmp (record, "container") == 0){ // C++ container
            appclass=9;
        }

        
        if (appclass==0) diep("\n Error format workload file: no valid application\n");

        // Obtains the application name
        strcpy(GLOBAL_app[nfile].appname,record);
        
        // Obtains the application size
        record = strtok_r (NULL, token1, &save_ptr1);
        Nsize=atoi(record);
        
        // Obtains the number of CPU iterations
        record = strtok_r (NULL, token1, &save_ptr1);
        NCPU=atoi(record);
        GLOBAL_app[nfile].cpu_intensity=NCPU;
        
        // Obtains the number of COMMUNICATION iterations
        record = strtok_r (NULL, token1, &save_ptr1);
        NCOM=atoi(record);
        GLOBAL_app[nfile].com_intensity=NCOM;

        // Obtains the number of IO iterations
        record = strtok_r (NULL, token1, &save_ptr1);
        NIO=atoi(record);
        GLOBAL_app[nfile].IO_intensity=NIO;
        
        // Obtains the value of IO action
        record = strtok_r (NULL, token1, &save_ptr1);
        IOaction=atof(record);

        // Obtains the number of iterations
        record = strtok_r (NULL, token1, &save_ptr1);
        NumIter=atoi(record);
        
        // Following entries are the compute nodes and max initial processes per nodes
        while (record != NULL) {
            for (n = 0; n < LOCAL_nhclasses; n ++) {
                if (strcmp (LOCAL_hclasses[n], record) == 0) {
                    record = strtok_r (NULL, token1, &save_ptr1);
                    LOCAL_nprocs_class[n] = atoi(record)*1; 
                    nprocs+=LOCAL_nprocs_class[n];
                }
            }
            record = strtok_r (NULL, token1, &save_ptr1);
        }
        
        // Updates the global node structure
        
    
        // Sorts the list (use the permutation vector index)
        for (n = 0; n < LOCAL_nhclasses; n ++) tmpval[n]=LOCAL_nprocs_class[n];

        // Orders the entries by process number -> produces wrong process-node mapping in bebop
        /*
        for (m = 0; m < LOCAL_nhclasses; m ++){
            
            max=-1;
            for (n = 0; n < LOCAL_nhclasses; n ++){
               if(tmpval[n]>=max) {
                max=tmpval[n];
                maxindex=n;
               }
            }
            
            index[m]=maxindex;
            tmpval[maxindex]=-100;
        }
        */
        
        
        m=0;
        while(m < LOCAL_nhclasses){
            
            // Takes the nodes with >0 procs. Nodes are in original order (requisite for bebop)
            for (n = 0; n < LOCAL_nhclasses; n ++){
                if(tmpval[n]>0) {
                    index[m]=n;
                    m++;
                }
            }
  
            // Takes the rest of them
             for (n = 0; n < LOCAL_nhclasses; n ++){
                if(tmpval[n]==0) {
                    index[m]=n;
                    m++;
                }
            }                    
        }
        
        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);

        // Backups the list to the global structure
        GLOBAL_app[nfile].nhclasses=LOCAL_nhclasses;
        GLOBAL_app[nfile].nsize=Nsize;
        
        // Sets arrival timestamps
        gettimeofday(&timestamp1, NULL);
        delta_t = (timestamp1.tv_sec - initial.tv_sec) * 1000 + (timestamp1.tv_usec - initial.tv_usec) / 1000;
        GLOBAL_app[nfile].nprocs=0;        
        for (n = 0; n < LOCAL_nhclasses; n ++) {
            //m=index[n];
            m=n;
            strncpy(GLOBAL_app[nfile].hclasses[n],LOCAL_hclasses[m],strlen(LOCAL_hclasses[m]));
            strncpy(GLOBAL_app[nfile].rclasses[n],LOCAL_rclasses[m],strlen(LOCAL_rclasses[m]));
            GLOBAL_app[nfile].ncores_class[n]=LOCAL_ncores[m];
            GLOBAL_app[nfile].nprocs_class[n]=LOCAL_nprocs_class[m];
            GLOBAL_app[nfile].port1=port1;
            GLOBAL_app[nfile].port2=port2;
            if(benchmark_mode) GLOBAL_app[nfile].port3=-1;
            if(benchmark_mode){
                GLOBAL_app[nfile].monitor=0;
            }
            else{
                GLOBAL_app[nfile].monitor=1;
            }
            GLOBAL_app[nfile].newprocs_class[n]=0; 
            GLOBAL_app[nfile].app_profile=profile; 
            GLOBAL_app[nfile].app_class=appclass; 
            GLOBAL_app[nfile].timestamp=delta_t;
            GLOBAL_app[nfile].initiated=0;
            GLOBAL_app[nfile].terminated=0;
            GLOBAL_app[nfile].nprocs+=GLOBAL_app[nfile].nprocs_class[n];
            GLOBAL_app[nfile].numiter=NumIter; 
            if(filename1==NULL) strncpy(GLOBAL_app[nfile].input,command,strlen(command));    
            else sprintf(GLOBAL_app[nfile].input," -void- ");
            // Updates the global node table
            for(i=0;i<GLOBAL_nhclasses;i++){
                if(strcmp(GLOBAL_app[nfile].hclasses[n],GLOBAL_node.hclasses[i])==0 &&  benchmark_mode!=1){
                    GLOBAL_node.used_cores[i]+=GLOBAL_app[nfile].nprocs_class[n];
                }
            }
        }
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
        
        // Increases 0 values in nproc_class (crashes MPI app)
        for (m = 0; m < LOCAL_nhclasses; m ++){
            if(LOCAL_nprocs_class[m]==0) LOCAL_nprocs_class[m]=1;
        }
         pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
       
        // Creates the output as a string
        for (m = 0; m< LOCAL_nhclasses; m ++) {
            n=index[m];
            sprintf(line,"%s:%d\n",LOCAL_hclasses[n],LOCAL_nprocs_class[n]);
            strcat(output,line);
        }
        
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
        // Stores the root node name
        strncpy(GLOBAL_app[nfile].node,LOCAL_hclasses[index[0]],sizeof(LOCAL_hclasses[index[0]]));
        if((GLOBAL_EXEC==1 &&  benchmark_mode!=1 ) || GLOBAL_ADHOC>0){      
            printf("      App [%d], root node: %s \n",nfile,GLOBAL_app[nfile].node);
        }
        
        // Create rankfile
        if(GLOBAL_VERVOSE) printf("  Creating the rankfile %d ...\n",nfile);
        sprintf(line,"./rankfiles/rankfile%d",nfile+1);
        if ((file2 = fopen (line, "w")) == NULL) {
            fprintf (stderr, "\nError in EMPI_Parse_malleability file2 opening\n");
            exit(1);
        }
        fprintf(file2,"%s",output);
        fclose(file2);
        
        sprintf(output,"cd %s/FlexMPI/scripts\n",HOME); // For Jacobi
        sprintf(line,"export LD_LIBRARY_PATH=%s/LIBS/glpk/lib/:%s/FlexMPI/lib/:%s/LIBS/mpich/lib/:%s/LIBS/papi/lib/:$LD_LIBRARY_PATH\n",HOME,HOME,HOME,HOME);
        if(appclass==3) sprintf(line,"export LD_LIBRARY_PATH=%s/LIBS/glpk/lib/:%s/FlexMPI/lib/:%s/LIBS/mpich/lib/:%s/LIBS/papi/lib/:%s/LIBS/gsl/gsl-1.16/.libs:%s/LIBS/gsl/gsl-1.16/cblas/.libs:$LD_LIBRARY_PATH\n",HOME,HOME,HOME,HOME,HOME,HOME);
        strcat(output,line);
        if(appclass==6) { // Clarisse
            sprintf(line,"export CLARISSE_COUPLENESS=dynamic\n");
            strcat(output,line);    
            sprintf(line,"export CLARISSE_PORT_PATH=%s/FlexMPI/examples/examples1\n",HOME);
            strcat(output,line);        
        }
       
        
        if (appclass==1) sprintf(line,"./Lanza_Jacobi_IO.sh %d %d %d %d %d %d %d %d %f %d\n",nprocs,port1,port2,nfile+1,Nsize,NIO,NCPU,NCOM,IOaction,NumIter); // Jacobi
        if (appclass==2) sprintf(line,"./Lanza_CG1.sh %d %d %d %d %d %d %d %d %f %d\n",nprocs,port1,port2,nfile+1,Nsize,NIO,NCPU,NCOM,IOaction,NumIter); // Conjugate gradient
        if (appclass==3) sprintf(line,"./Lanza_Epigraph.sh %d %d %d %d %f\n",nprocs,port1,port2,nfile+1,IOaction); // Epigraph
        if (appclass==4) sprintf(line,"./Lanza_Irreg_IO.sh %d %d %d %d %d %d %d %d %f %d\n",nprocs,port1,port2,nfile+1,Nsize,NIO,NCPU,NCOM,IOaction,NumIter); // Jacobi
        if (appclass==5) sprintf(line,"./Lanza_mixed.sh %d %d %d %d %d %d %d %d %f %d %d %d\n",nprocs,port1,port2,nfile+1,Nsize,NIO,NCPU,NCOM,IOaction,NumIter,profile,deploy); // Jacobi
        if (appclass==6) sprintf(line,"./Lanza_CG2.sh %d %d %d %d %d %d %d %d %f %d\n",nprocs,port1,port2,nfile+1,Nsize,NIO,NCPU,NCOM,IOaction,NumIter); // Conjugate gradient
        if (appclass==7) sprintf(line,"./Lanza_ioserver.sh %d %d %d %d %d %d %d %d %f %d\n",nprocs,port1,port2,nfile+1,Nsize,NIO,NCPU,NCOM,IOaction,NumIter); // Jacobi
        if (appclass==8) sprintf(line,"./Lanza_dynamic.sh %d %d %d %d %d %d %d %d %f %d %d %d\n",nprocs,port1,port2,nfile+1,Nsize,NIO,NCPU,NCOM,IOaction,NumIter,profile,deploy); // Dynamic loader    
        if (appclass==9) sprintf(line,"./Lanza_container.sh %d %d \n",port1,port2); // Dynamic loader
        port1+=2;
        port2+=2;
        
        strcat(output,line);

        // Create execution script
        if((GLOBAL_EXEC==1 &&  benchmark_mode!=1 ) || GLOBAL_ADHOC>0){      
            printf("      Creating the execution script %d ...\n",nfile);
        }
        
        sprintf(line,"./execscripts/exec%d",nfile+1);
        if ((file2 = fopen (line, "w")) == NULL) {
            fprintf (stderr, "\nError in EMPI_Parse_malleability file2 opening\n");
            exit(1);
        }
        fprintf(file2,"%s",output);
        fclose(file2);

        sprintf(output,"chmod 755 ./execscripts/exec%d",nfile+1);
        system(output);
        
        // Executes the application
        if((GLOBAL_EXEC==1 &&  benchmark_mode!=1 ) || GLOBAL_ADHOC>0){
            printf("      Executing the application %d ...\n",nfile);
            sprintf(output,"%s/FlexMPI/controller/execscripts/exec%d > ./logs/output%d 2>&1 &",HOME,nfile+1,nfile+1);  
            system(output); // This is the system call to execute
        }
        nfile++;
        
        // Updates the input string
        work=0;
        if(filename1!=NULL){
            if(fscanf (file1, "%s\n", readline) != EOF) work=1;
        }
        else{  
            readcommand=strtok_r (NULL, token2, &save_ptr2);
            if(readcommand!=NULL) work=1;
            if(readcommand!=NULL) strcpy(readline,readcommand);
        }
    
    }
    if(filename1!=NULL) fclose(file1);
    GLOBAL_napps=nfile;
 
}

void analyse_node(char *node, int conflict, int *result, char *resultnode){
    int i,j,k,n,id,cnt=0,flag;
    double ratio1,ratio2,ratio3,ratio4,dtmp;
    int res,selectid,selectclass;
    int apps[1024],app_class[1024];
    struct arg_struct3 args3[MAX_APPS];
    pthread_t thread3[MAX_APPS];
    pthread_attr_t attr3[MAX_APPS];
    int rc;  // return code
    char counters[1024];
    
    
    //if(conflict==1) strcpy(counters,"PAPI_L3_TCM"); // cache
    if(conflict==1) strcpy(counters,""); // cache
    else if(conflict==2) strcpy(counters,"");
    //else if(conflict==3) strcpy(counters,"PAPI_LD_INS:PAPI_SR_INS");
    else if(conflict==3) strcpy(counters,"");
    else {
        printf("\n Error: conflict no defined. Terminating the program");
        killall(-1);
    }
        
    printf("\n --> Analyzing compute node %s looking for conflict %d \n",node,conflict);
    
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);     
    
    // Finds the applications running in node
    cnt=0;
    for(id=0;id<GLOBAL_napps;id++){
        for(i=0;i<GLOBAL_app[id].nhclasses;i++){
            if(GLOBAL_terminated_app_id[id]!=1 && strcmp(GLOBAL_app[id].hclasses[i],node)==0 && GLOBAL_app[id].nprocs_class[i]>0){
                printf("    * Application %d in node %s with %d processes created at %d secs.\n",id,node,GLOBAL_app[id].nprocs_class[i],GLOBAL_app[id].timestamp/1000);
                apps[cnt]=id;
                app_class[cnt]=i;
                cnt++;
            }
        }
    }
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);  
    if(cnt==0){
        result[0]=-1;
        result[1]=-1;
        strcpy(resultnode,"void"); 
        printf("    * No applications running in this node \n");
    }
    printf("\n");
    
    // Only analyses performance when there are more than one application
    if(cnt>1){
        if(conflict==1 || conflict==3){
            for(i=0;i<cnt;i++){
                n=apps[i];
                
                // Waits for the application monitor to get the initial metrics
                printf("\n  [%d] Waiting for initiating the application \n",n);
                dtmp=0;
                while(dtmp==0){
                    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);      
                    dtmp=GLOBAL_app[n].initiated;
                    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                    sleep(5);
                }
                printf("\n  [%d] Application initiated \n",n);
                
                args3[n].id = n;
                strcpy(args3[n].counter_list,counters);
                args3[n].new_perf_metrics = curr_perf_metrics;
                args3[n].init=0;
                rc = pthread_attr_init(&attr3[n]);
                check_posix_return(rc, "Initializing attribute");
                rc = pthread_create(&thread3[n], &attr3[n], (void*)&application_monitor,(void *)(&args3[n]));
                check_posix_return(rc, "Creating application-monitor thread ");
                sleep(5); 
            }        
            
            for(i=0;i<cnt;i++){
                n=apps[i];
                if(GLOBAL_VERVOSE) printf("    Waiting for thread %d.... \n",n);
                (void) pthread_join(thread3[n], NULL);
                if(GLOBAL_VERVOSE) printf("    Waiting thread %d completed \n",n);
            }
            
            // Prints the difference
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
            
            for(j=0;j<cnt;j++){
                id=apps[j];
                printf("\n      Node %s - Application %d init: ",node,id);
                for(i=0;i<12;i++){
                    printf(" %.1e ",init_perf_metrics[id].metric[i]);
                }
                printf("\n");
                printf("      Node %s - Application %d end : ",node,id);
                for(i=0;i<12;i++){
                    printf(" %.1e ",curr_perf_metrics[id].metric[i]);
                }
                printf("\n\n");

            }
             

            flag=0;
            for(j=0;j<cnt;j++){
                id=apps[j];
                if(init_perf_metrics[id].metric[0]!=0) ratio1=curr_perf_metrics[id].metric[0]/init_perf_metrics[id].metric[0];
                else ratio1=0;
                if(init_perf_metrics[id].metric[1]!=0) ratio2=curr_perf_metrics[id].metric[1]/init_perf_metrics[id].metric[1];
                else ratio2=0;
                if(init_perf_metrics[id].metric[8]!=0) ratio3=curr_perf_metrics[id].metric[8]/init_perf_metrics[id].metric[8];
                else ratio3=0; 
                if(init_perf_metrics[id].metric[8]!=0) ratio4=curr_perf_metrics[id].metric[8]/init_perf_metrics[id].metric[8];
                else ratio4=0; 
                printf("     # Ratios for %d :: \t %.2f\t %.2f\t %.2f\t %.2f\t \n",id,ratio1,ratio2,ratio3,ratio4);
                
                if((ratio1>1.1 || ratio2>1.1 || (ratio3>1.1 && ratio3<1000) || (ratio4>1.1 && ratio4<1000))) flag=1;
            }
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);      
        }
        // Conflic=2 
        else{
            system("top -n 1");
            for(i=0;i<cnt;i++){
                n=apps[i];
                
                // Waits for the application monitor to get the initial metrics
                printf("\n  [%d] Waiting for initiating the application \n",n);
                dtmp=0;
                while(dtmp==0){
                    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);      
                    dtmp=GLOBAL_app[n].initiated;
                    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                    sleep(5);
                }
                printf("\n  [%d] Application initiated \n",n);
            }
            flag=1;
        }
        
        result[0]=-1;
        result[1]=-1;
        
        // Conflict solving
        if(flag==1){
            res=-1;
            selectid=-1;
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);      
            for(j=0;j<cnt;j++){
                id=apps[j];
                k=app_class[j];
                //if(res==-1 || GLOBAL_app[id].nprocs_class[k]<res){
                    //res=GLOBAL_app[id].nprocs_class[k];
                //if((res==-1 || GLOBAL_app[id].timestamp>res) && GLOBAL_app[id].nprocs>=12){
                if((res==-1 || GLOBAL_app[id].timestamp>res) && strcmp(GLOBAL_app[id].node,node)!=0){
                    res=(int)GLOBAL_app[id].timestamp;
                    selectid=id;
                    selectclass=k;
                }
            }
            
            printf("     # Application selected: %d  \n\n",selectid);
            result[0]=selectid;
            result[1]=-GLOBAL_app[selectid].nprocs_class[selectclass];
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
            strcpy(resultnode,GLOBAL_app[selectid].hclasses[selectclass]);
            if(selectid==-1){
                result[0]=-1;
                result[1]=-1;
                strcpy(resultnode,"void");        
            } 
        }
        else{
            printf("     #No performance degradation. No actions taken \n");
            result[0]=-1;
            result[1]=-1;
            strcpy(resultnode,"void");        
        }
    }
    else{
        result[0]=-1;
        result[1]=-1;
        strcpy(resultnode,"void");        
    }
}


// Listener thread
void monitor_listener()
{
    int i,s;
    struct sockaddr_in si_other;
    struct sockaddr_in serverAddr;
    struct hostent   *he;
    socklen_t addr_size; 
    int slen = sizeof(si_other);
    char *token; 
    char *saveptr;    
    GLOBAL_MONITOR_PORT=5002;
    strcpy(MONITOR_NODE,"compute-9-1"); 
    char buf[EMPI_COMMBUFFSIZE];
    int flag,mem,cpu,net,cache;
    char node[512];
    
    

    struct sockaddr_in si_me;
    
    // Configures the socket reception
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        diep("socket1");
    }
    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(7050);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
    {
        diep("bind1 monitor_listener");
    }

    sleep(3);
    // Configures the connection with Monitor
        
    /*Configure settings in address struct*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(GLOBAL_MONITOR_PORT);
        
    if ( (he = gethostbyname(MONITOR_NODE) ) == NULL ) {
        diep("Error resolving host name");
        exit(1); /* error */
    }
                
    serverAddr.sin_addr=*(struct in_addr *) he->h_addr;
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));  
    addr_size = sizeof(serverAddr); 

    // Sends the connection command to the monitor
    printf(buf,"1:1");
    sendto(s,buf,strlen(buf),0,(struct sockaddr *)&serverAddr,addr_size);  
    
    while(1){
        // Receives from the monitor
        recvfrom(s, buf, EMPI_COMMBUFFSIZE, 0, (struct sockaddr *)&si_other, (socklen_t *)&slen);
        
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");            
        printf("@@@@@ Message from monitor: %s \n",buf);    
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");      
          
        flag=0;
        token = strtok_r(buf,":",&saveptr);
        while(token!=NULL){
            strcpy(node,token);
            token = strtok_r(NULL,":",&saveptr);
            if(token==NULL) break;
            mem=atoi(token); // it  is numprocs, but its value is overwritten next
            token = strtok_r(NULL,":",&saveptr);
            if(token==NULL) break;
            mem=atoi(token);
            token = strtok_r(NULL,":",&saveptr);
            if(token==NULL) break;
            cpu=atoi(token);
            token = strtok_r(NULL,":",&saveptr);
            if(token==NULL) break;
            net=atoi(token);
            token = strtok_r(NULL,":",&saveptr);
            if(token==NULL) break;
            cache=atoi(token);
            flag=1;
            break;
        }
        
        // Valid metrics
        if(flag==1){
            for(i=0;i<NCLASES;i++){
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                if(strcmp(GLOBAL_node.hclasses[i],node)==0){
                    GLOBAL_node.cpu[i]=cpu;
                    GLOBAL_node.mem[i]=mem;
                    GLOBAL_node.net[i]=net;
                    GLOBAL_node.cache[i]=cache;
                    GLOBAL_node.updated[i]=1;
                }
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
            }
        }  
    }
}
    
    
void monitor_analyzer() 
{
    int i,j,newevent;
    int mem, net;
    char node[512],resultnode[512];
    int deltap,err,nump,currp,scenario;
    int result[2];

    while(1){
        
        newevent=0;
        for(i=0;i<NCLASES;i++){
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
            if(GLOBAL_node.updated[i]==1){
                strcpy(node,GLOBAL_node.hclasses[i]);
                mem=GLOBAL_node.mem[i];
                net=GLOBAL_node.net[i];
                GLOBAL_node.updated[i]=0;
                newevent=1;
            }
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
            if(newevent) break;
        }
        
        if(newevent && get_node(node)){
            if(mem>90) scenario=2;
            else if(net>30) scenario=3;
            else scenario=1;

            analyse_node(node,scenario,result,resultnode);
            if(result[0]>=0){
                j=result[0];

                deltap=result[1];
                printf("  *** Applying malleability to application %d :: DeltaP = %d at node %s\n",j,deltap,resultnode);
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                err=allocnodes(j,deltap, resultnode, 0, 0, 0);
                nump=GLOBAL_app[j].nprocs;
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                
                // Creates the specified number of processes
                if(err==-1){
                    printf(" *** Error batch_scheduler: not enough processes for %d \n",j); 
                }
                else{
                    send_message(resultnode,j,0,0);     
                    clean_nodes(resultnode);
                }
                
                // Waits until nump processes have been created.
                do{
                    send_message("4:on",j,0,30);     
                    currp=0;
                    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock); 
                    if(cnt_perf[j]>1) currp=perf_metrics[j][cnt_perf[j]-1].metric[7];
                    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
                    printf("   App [%d] Waiting in main until the remaining processes are dynamically created. Current: %d \n",j,currp);               
                }
                while(currp!=nump && GLOBAL_terminated_app_id[j]==0);  
                send_message("4:off",j,0,30);     

                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);     
                err=allocnodes(j,-deltap, resultnode, 0, 0, 1);
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
                sleep(60);
                
                // Creates the specified number of processes
                if(err==-1){
                    printf(" *** Error batch_scheduler: not enough processes for %d \n",j); 
                }
                else{
                    send_message(resultnode,j,0,0);     
                }                        
            }
            else{
                if(get_node(node)){
                    printf (" @@ Node %s suppressed \n",node);
                    set_node(node);
                }
            }
        }
        else{
            sleep(2);
        }
    }   
}

// Monitor thread. One per application
void batch_scheduler(void *arguments)
{
    int i,j,cnt,id,skip,control;
    int flag[MAX_APPS],sched_class[MAX_APPS];
    FILE *file;
    int tmp_time,err,cnt2;
    uint64_t delta_t,timestamp[MAX_APPS],starttime[MAX_APPS];
    char command[MAX_APPS][1024],nodes[1024],tmpcommand[1024];
    char type;
    char output[1024];
    struct timeval initial,timestamp1;
    
    gettimeofday(&initial, NULL);
    
    for(i=0;i<MAX_APPS;i++){
        flag[i]=0;
        sched_class[i]=0;
        starttime[i]=0;
    }
    // Opens workload file
    printf("\n   Reading the batch files ... \n");
    if ((file = fopen ("batchfile.dat", "r")) == NULL) {
        printf ("  file not present. Batch scheduler exiting. \n");
        pthread_exit(NULL);
    }
    
    cnt=0;
    while (fscanf (file, "%c %d %s\n", &type, &tmp_time,command[cnt]) != EOF) {
        // Scheduling methods:
        
        // T means triggered by time. The second argument is the starting execution time (in seconds)
        // The execution is triggered when the time counted since the controller start is reached. 
        // T 0  cg2:2000:1:1:0:0.02:2000:4     -> CG2 executed at time 0 sec.
        // T 60 jacobi:3000:1:1:0:0.02:3000:4  -> Jacobi executed (if enough resources) after 60 seconds
        
        // B means triggered by termination. The second argument is application id 
        // The execution is triggered when the application terminates. 
        // T 0 jacobi:3000:1:1:0:0.02:2000:4   -> Jacobi executed at time 0 sec.
        // B 0 jacobi:3000:1:1:0:0.300:3000:4  -> Jacobi executed when application 0 terminates 

        // M means malleable action triggered by time
        // The controller sends a malleable command to the previous application in the batchfile
        // T 10 jacobi:30000:1:1:0:0.02:2000:4 -> Jacobi executed at time 10 secs.
        // M 70 4                              -> At time 10+70 secs. the previous application creates 4 procs. 
        
        // I means triggered by initialization. The second argument is application id that triggers the execution
        // The execution is triggered when the application stats. 
        // T 60 jacobi:3000:1:1:0:0.02:2000:4  -> Jacobi executed at time 60 secs.
        // I 0  jacobi:3000:1:1:0:0.300:3000:4 -> Jacobi executed when the first one starts (time 60)  
        
        
        // Sets global pending applications
        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
        GLOBAL_pending=1;
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
        
        skip=0;
        if(type=='T' || type=='t') 
            sched_class[cnt]=0; // Triggered by time
        else if(type=='B' || type=='b') 
            sched_class[cnt]=1; // Triggered by termination
        else if(type=='M' || type=='m') 
            sched_class[cnt]=2; // Malleable by time
        else if(type=='I' || type=='i') 
            sched_class[cnt]=3; // Triggered by initialization
        else if(type=='#') 
            skip=1;
        else{
            printf("\n\n  Error: batchfile format not recognized... exiting  \n\n");
            killall(-1);
            exit(0);
        }
        if(skip==0){
            timestamp[cnt]=(uint64_t)tmp_time;        
            if(sched_class[cnt]==0) printf("    Application %d \t triggered by time             %s \n",cnt,command[cnt]);
            if(sched_class[cnt]==1) printf("    Application %d \t triggered by termination      %s \n",cnt,command[cnt]);
            if(sched_class[cnt]==2) printf("    Application %d \t malleable by time             %s \n",cnt,command[cnt]);
            if(sched_class[cnt]==3) printf("    Application %d \t triggered by initialization   %s \n",cnt,command[cnt]);
        }
        cnt++;
    }
    printf("\n\n");
    fclose(file);
    
    // Total number of jobs
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
    GLOBAL_bachapps=cnt;
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
    
    j=0;
    while(1){
        gettimeofday(&timestamp1, NULL);
        
        delta_t = (timestamp1.tv_sec - initial.tv_sec) * 1000 + (timestamp1.tv_usec - initial.tv_usec) / 1000;
        delta_t = delta_t/1000;
        //printf("-> %d \n",(int)delta_t);
        cnt2=0;
        for(i=0;i<cnt;i++){
            
            // Execute time-triggered applications
            if(flag[i]==0 && delta_t>=timestamp[i] && sched_class[i]==0){
                // The number of processes is provided
                if(!GLOBAL_nodealloc){
                    strcpy(tmpcommand,command[i]);
                    err=parse_command (tmpcommand, output, cnt2);  
                }
                // Compute nodes are explicitly specified 
                else{
                    strcpy(tmpcommand,command[i]);
                    init_app(tmpcommand,0);  // Application start
                    err=0;
                }
                if(err==0){
                    // Schedule new application
                    starttime[i]=(int)delta_t;
                    printf("  *** Application %d started at time %d:: %s \n",cnt2,(int)delta_t,command[i]);
                    flag[i]=1;
                    j++;
                }
                else{
                    printf("  *** Application %d waiting.... \n",cnt2);
                }
                
            }
            
            // Execute triggered scheduling
            if(flag[i]==0 && (sched_class[i]==1 || sched_class[i]==3))
            {
                id=timestamp[i]; // Timestamp here contains the application ID, NOT the time
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                if(sched_class[i]==1) control=GLOBAL_terminated_app_id[id];
                if(sched_class[i]==3) control=GLOBAL_app[id].initiated;
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                if(control==1){      
                    // The number of processes is provided
                    if(!GLOBAL_nodealloc){
                        strcpy(tmpcommand,command[i]);
                        err=parse_command (tmpcommand, output, cnt2);
                    }
                    // Compute nodes are explicitly specified 
                    else{
                        strcpy(tmpcommand,command[i]);
                        init_app(tmpcommand,0);  // Application start
                        err=1;
                    }
                    if(err==0){
                        // Schedule new application
                        printf(" ** Starting application %d at time %d:: %s \n",cnt2,(int)delta_t,command[i]);
                        starttime[i]=(int)delta_t;
                        flag[i]=1;
                        j++;
                    }
                    else{
                        printf("  *** Application %d waiting.... \n",cnt2);
                    }
                }
            }
            
            // Execute malleable operations 
            if(flag[i]==0 && flag[i-1]==1 && delta_t>=timestamp[i]+starttime[i-1] && sched_class[i]==2){
                // Malleable action 
                printf("  *** Applying malleability to application %d (from %d) at time %d:: DeltaP = %d \n",cnt2-1,i,(int)delta_t,atoi(command[i]));
                do{
                    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                    strcpy(tmpcommand,command[i]);
                    err=allocnodes(cnt2-1,atoi(tmpcommand), nodes, 0, 0, 0);
                    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                    if(err==0) flag[i]=1;
                    // Creates the specified number of processes
                    if(err==-1){
                        printf(" *** Error batch_scheduler: not enough processes for %d \n",cnt2-1);
                        sleep(60);
                    }
                    else{
                        send_message(nodes,cnt2-1,0,0);     
                        clean_nodes(nodes);
                    }
                }while(err==-1);
            }
            
            // All the applications have started the execution
            if(j==cnt){
                // Sets global pending applications
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                GLOBAL_pending=0;
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);            
            }
            if(sched_class[i]==0 || sched_class[i]==1){
                cnt2++;
            }
        }
        sleep(5);
    }

}


// Monitor thread. One per application
void application_monitor(void *arguments)
{
    int i,id,last_cnt,new_cnt,local_cnt,num_samples;
    int flag;
    char counter_list[1024];
    struct arg_struct3 *args = (struct arg_struct3 *) arguments;
    char *token; 
    char *saveptr;
    struct new_performance_metrics *lperf_metrics;

    double localt;
    
    char command1[1024],localname[256];
    
    id=args->id;
    //init=args->init; // init==1, application initial monitorization.
    strcpy(counter_list,args->counter_list);
    lperf_metrics=args->new_perf_metrics;

    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);       
    for(i=0;i<12;i++){
        lperf_metrics[id].metric[i]=0;
    }
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);       
        
    
    strcpy(counter_list,args->counter_list);
    
    // Sleeps a certain amount, to let the interference to stabilizez
    sleep(30);
    
    // Collects performance metrics of the ad-hoc performance counters
   
    token = strtok_r(counter_list,":",&saveptr);
    
    flag=0;
    // Obtains metrics for each counter
    while(token!=NULL){
        printf("    [%d] Processing counter %s\n",id,token);
         
        if(token!=NULL) {          
            // Activates application monitoring
            send_message("4:on",id,0,0);     
            flag=1;
            sprintf(command1,"7:%s:NULL",token);
            send_message(command1,id,0,0);     
        }
        
        // Wait for final size
        printf("    [%d] Waiting for nnz values \n",id);
        do{
            send_message("4:on",id,0,15);     
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);       
            if(cnt_perf[id]>3) localt=perf_metrics[id][cnt_perf[id]-2].metric[1];
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);             
        }while(localt<0.01);
        
        
        printf("    [%d] Waiting for the counter class \n",id);
        strcpy(localname,"void");
        local_cnt=0;
        while(token!=NULL && is_different(token,localname)){
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);       
            if(cnt_perf[id]>1) strcpy(localname,perf_metrics[id][cnt_perf[id]-1].nhwpc_1);
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);             
            sleep(5);
            local_cnt++;
            if(local_cnt%10==0) send_message(command1,id,0,0);     
        }
        

        if(cnt_perf[id]>1 && perf_metrics[id][cnt_perf[id]-1].metric[1]<20) num_samples=6;
        else num_samples=5;
        //if(cnt_perf[id]>1 && perf_metrics[id][cnt_perf[id]-1].metric[1]<20) num_samples=1;
        //else num_samples=1;
        
        printf("    [%d] Waiting for %d samples \n",id,num_samples);
        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);       
        last_cnt=cnt_perf[id];
        //local_cnt=0;
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);  
        do{
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);       
            new_cnt=cnt_perf[id];
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);             
            sleep(5);
            //local_cnt++;
            //if(local_cnt%10==0) send_message(command1,id,0,0);     
        }while(new_cnt<last_cnt+num_samples);
        
        
        printf("    [%d] Metrics collected \n",id);
        i=event_mapper(token);
        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);       
        lperf_metrics[id].metric[i]=perf_metrics[id][cnt_perf[id]-1].metric[5];       
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
              
        token = strtok_r(NULL,":",&saveptr);
    }
    
    // Backs-up the remaining metrics
    
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);       
    for(i=0;i<8;i++){
        lperf_metrics[id].metric[i]=perf_metrics[id][cnt_perf[id]-1].metric[i];
    }
    //if(init==1) GLOBAL_app[id].initiated=1;    
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);         
    
    // Prints the metrics
    /*
    printf("\n Application %d: ",id);
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);       
    for(i=0;i<12;i++){
        printf(" %.2f \t ",lperf_metrics[id].metric[i]);
    }
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
    printf("\n"); 
    */
    // Deactivates application monitoring
    if (flag==1) send_message("4:off",id,0,0);        
    pthread_exit(NULL);
}

// Forwarder thread. One thread

void forwarder(void *arguments)
{
    int s,n;
    struct sockaddr_in si_me;
    struct sockaddr_in si_other;
    struct sockaddr_in serverAddr;
    struct hostent   *he;
    socklen_t addr_size;
    char *token; 
    char *saveptr;
    int slen = sizeof(si_other);
    int channel;
    int flag,currp;
    
    char buf[EMPI_COMMBUFFSIZE],nodename[512];

    FILE *file;
    // Opens the actual nodename 
    if ((file = fopen ("controller2.dat", "r")) == NULL) {
        fprintf (stderr, "\nError opening controller2.dat in check_node %s \n","controller2.dat");
        exit(1);
    }
    fscanf (file, "%s\n", nodename);
    fclose(file);
    
    
    // Configures the socket reception
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        diep("socket1");
    }
    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(8900);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
    {
        diep(" Error in socket in forwarder thread");
    }
    
    // Configures the connection with the external domain master
    
    /*Configure settings in address struct*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9912);
        
    if ( (he = gethostbyname(nodename) ) == NULL ) {
        diep("Error resolving host name");
        exit(1); /* error */
    }
                
    serverAddr.sin_addr=*(struct in_addr *) he->h_addr;
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));  
    addr_size = sizeof(serverAddr);  
    
    
    while(1){
        memset(buf, 0, EMPI_COMMBUFFSIZE);
        
        // Reception request
        recvfrom(s, buf, EMPI_COMMBUFFSIZE, 0, (struct sockaddr *)&si_other, (socklen_t *)&slen);  
        printf("  Forwarder message received:  %s \n",buf);
           
        // Extracts the header
        //    header==-1 -> create new application  with details in payload 
        //    header>=0  -> send application header payload using channel
        // Channel == 2   -> send the application number of proc to the domain master
        token = strtok_r(buf, " ",&saveptr);
        
        flag=0;
        n=atoi(token);
        if(n<-1 ||n>GLOBAL_napps){
            printf("Error in the message format: application id incorrect \n");
            flag=1;
        }
        
        // Extracts the channel
        if(n>=0 && flag==0){
            token = strtok_r(NULL, " ",&saveptr);
            channel=atoi(token);
            if(channel!=0 && channel!=1 && channel!=2){
                printf("Error in the message format: application channel incorrect \n");
                flag=1;
            }
            // Extracts the command
            // Sends the command to the application
            if(channel==0 ||channel==1){
                if(flag==0) token = strtok_r(NULL, " ",&saveptr);  
                send_message(token, n, channel, 0); 
            }
            // Sends the info to the domain master
            else{
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock); 
                if(cnt_perf[n]>1) currp=perf_metrics[n][cnt_perf[n]-1].metric[7];
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
                sprintf(buf,"NumProcs %d",currp);
                printf("  Sending to the domain master: %s",buf);    
        
                // Sends the connection 
                 sendto(s,buf,strlen(buf),0,(struct sockaddr *)&serverAddr,addr_size);                     
            }
        }       
        // Run a new application
        else if(n==-1 && flag==0){
            // Extracts the command
            token = strtok_r(NULL, " ",&saveptr);
            init_app(token,0);  // Application start
        }
        
    } 
    
}

// Listener thread. One per application
int command_listener(void *arguments)
{
    struct sockaddr_in si_me;
    struct sockaddr_in si_other;
    struct sockaddr_in serverAddr;
    struct sockaddr_in appAddr;
    
    int i, s, n, m, slen = sizeof(si_other),id,local_terminated_app,tmp,local_reconf,iter;
    int WS,busyIO,local_pending,avail;
    struct arg_struct1 *args2 = arguments;
    struct arg_struct1  args;
    struct timeval timestamp1,timestamp2;
    uint64_t delta_t;
    double delta_long,delta_cpu,tot_IO,tmp_dl,tmp_dt,delayttime;
    double rtime,ptime,ctime,mflops,count1,count2,iotime,size,diter,datasize;
    char *token;
    FILE *fp;
    char path[1035];
    int flag[MAX_APPS],flag2;
    char appcmd[1024],nhwpc_1[256],nhwpc_2[256];
    char *saveptr1,*saveptr2,*saveptr3;
    
    struct hostent   *he;
    socklen_t addr_size;   
    
    char * buf = calloc(EMPI_COMMBUFFSIZE, 1);
    char bashcmd[1024];
    
    // Updates args structure
    args.id    = args2->id;
    args.port2 = args2->port2;
    
    
    // Elastic search variables
    // CURL *curl=NULL;
    
    char ip_es[1024],time_stamp[1024];
    char nodename[512];
    //char counter1[512], counter2[512];
    time_t t = time(NULL);
    sprintf(ip_es,"arpia.arcos.inf.uc3m.es");
    //extern void curl_es_app(CURL *curl, char *ip_es, char *ip_node, int appid, char * timestamp, double rtime, double ptime, double ctime, double mflops, char *counter1_name, double counter1, char *counter2_name, double counter2, double iotime, double size, double diter);
    
    for(i=0;i<MAX_APPS;i++){
        if(GLOBAL_monitoring)     flag[i]=0; // Monitoring activated
        else flag[i]=1;                      // Monitoring not activated
    }
    
    // Configures the socket reception
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        diep("socket1");
    }
    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(args.port2);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
    {
        diep("bind1x");
    }

      
    // Configures the connection with GUI
    if(GLOBAL_GUI){
        
        /*Configure settings in address struct*/
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(GLOBAL_GUI_PORT);
        
        if ( (he = gethostbyname(GUI_NODE) ) == NULL ) {
            diep("Error resolving host name");
            exit(1); /* error */
        }
                
        serverAddr.sin_addr=*(struct in_addr *) he->h_addr;
        memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));  
        addr_size = sizeof(serverAddr); 
        
        id=args.id;
        sprintf(buf,"./app:1000:2018/02/26-11:22@format@%d@%d",id,GLOBAL_GUI_ListenerPort);
        printf("  --- Connection with graphic interface at %s port %d with string %s\n",GUI_NODE,GLOBAL_GUI_PORT,buf);    
        
        // Sends the connection command to the GUI (only once)
        sendto(s,buf,strlen(buf),0,(struct sockaddr *)&serverAddr,addr_size);  
        
    }        
    // Main loop that reads the buffer values 
    while(1)
    {
        memset(buf, 0, EMPI_COMMBUFFSIZE);
        int length = recvfrom(s, buf, EMPI_COMMBUFFSIZE, 0, (struct sockaddr *)&si_other, (socklen_t *)&slen);
        if(GLOBAL_DEBUG) printf("  Message received [%d]: %s %d \n",args.id,buf,length);
        if (length >1)
        {      
            // IO operation
            if(strncmp(buf, "IO", 2)==0)
            {
                gettimeofday(&timestamp1, NULL);
                delta_t = (timestamp1.tv_sec - initial.tv_sec) * 1000 + (timestamp1.tv_usec - initial.tv_usec) / 1000;

                /* get the first token */
                token = strtok_r(buf, " ",&saveptr1);
                if(token!=NULL)  token = strtok_r(NULL, " ",&saveptr1);
                if(token!=NULL)
                {
                    token = strtok_r(NULL, " ",&saveptr1);
                    iter=atoi(token);
                    token = strtok_r(NULL, " ",&saveptr1);
                    delta_long=atof(token);
                    token = strtok_r(NULL, " ",&saveptr1);
                    size=atof(token);
                    token = strtok_r(NULL, " ",&saveptr1);
                    tot_IO=atof(token);
                    token = strtok_r(NULL, " ",&saveptr1);
                    delta_cpu=atof(token);
                    
                    
                    id=args.id;

                    // Activates monitoring the first time
                    if(flag[id]==0){
     
                        appAddr.sin_family = AF_INET;
                        appAddr.sin_port = htons(GLOBAL_app[id].port1);
                        if ( (he = gethostbyname(GLOBAL_app[id].node) ) == NULL ) {
                            diep("Error resolving application name2");
                            exit(1); /* error */
                        }
                    
                        appAddr.sin_addr=*(struct in_addr *) he->h_addr;
                        memset(appAddr.sin_zero, '\0', sizeof(appAddr.sin_zero));  
                        addr_size = sizeof(appAddr); 
            
                        sprintf(appcmd,"4:on");
            
                        sendto(s,appcmd,strlen(appcmd),0,(struct sockaddr *)&appAddr,addr_size);  
                         
                        printf("\n\n ***  command_listener: Activating monitoring of application %d ",id);
                        flag[id]=1;
                    }

                    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                    if(id+1>napps) napps=id+1;         
                    // Store values
                    
                    
                    GLOBAL_iter[id]=iter;
                    
                    // Counts the delayed execution time
                    delayttime=GLOBAL_delayt1[id];
                    if(GLOBAL_delayt1[id]!=0) GLOBAL_delayt1[id]=0;
                    niter[cnt_app[id]][id]=iter;
                    tstamp[cnt_app[id]][id]=(double)delta_t/1000-delta_long+delayttime; // The message is sent after the I/O, thus we have to substract its duration + the delay time of the operation
                    tlong[cnt_app[id]][id]=delta_long;
                    cput[cnt_app[id]][id]=delta_cpu;
                    numprocs[cnt_app[id]][id]=(int)size;
                    cnt_app[id]++;
                    if(cnt_app[id]>=NUMSAMPLES){
                        diep("command_listener error: out-of-memory. Increase NUMSAMPLES \n ");
                    }
                    update[id]++;
                    tmp_dl=0;
                    tmp_dt=0;
                     
                    // Make a prediction
                    tmp=cnt_app[id]-reset[id]-1;             // Existing samples
                    if(tmp<WS1) WS=0;                          // Not enough samples
                    else if(tmp>=WS1 && tmp>WS2) WS=WS2;    // Maximum number of samples (WS2)
                    else if(tmp>=WS1 && tmp<=WS2) WS=tmp;    // Intermediate value between (WS1,WS2]
                     
                         
                     if(WS>0){
                         for(i=0;i<WS;i++){
                             tmp_dl=tmp_dl+tlong[cnt_app[id]-1-i][id];
                             tmp_dt=tmp_dt+(tstamp[cnt_app[id]-1-i][id]-tstamp[cnt_app[id]-2-i][id]);
                         }
                         tmp_dt=tmp_dt/WS;
                         tmp_dl=tmp_dl/WS;
                         
                         // Generates the prediction
                         for(i=0;i<STAMPSIZE;i++){
                             p_tstamp[i][id]=tstamp[cnt_app[id]-1][id]+tmp_dt*(i+1); 
                         }
                         p_tlong[id]=tmp_dl;
                     }
    
                     printf("      IO:: %d [size: %d]  \t T= %.2f \t  dT= %.2f \t cpuT= %.2f\t\t  TotIO= %.2f ",id,(int)size,tstamp[cnt_app[id]-1][id],(tstamp[cnt_app[id]-1][id]-tstamp[cnt_app[id]-2][id]),cput[cnt_app[id]-1][id],tot_IO);
                     //for(i=0;i<4;i++) printf("%f ",p_tstamp[i][id]);
                     printf(" \t\t LONG= %.2f [ %.2f ] \n",tlong[cnt_app[id]-1][id],p_tlong[id]);
                     pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                 }
            }
            else if(strcmp(buf,"Application terminated")==0)
            {
             printf("       %d termination data from %s:%d --> %s \n",args.id, inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);
             pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
             if(GLOBAL_app[args.id].terminated==0) terminated_app++; 
             GLOBAL_terminated_app_id[args.id]=1;
             local_terminated_app=terminated_app;
             printf("\n ** [%d] application terminated %d out of %d \n",args.id,terminated_app,GLOBAL_bachapps);
             pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);    // Creates one thread per application
             
             // Notifies the termination external manager
             if(GLOBAL_forwarding){
                char command[1024];
                sprintf(command,"nping --udp -p %d -c 1 %s --data-string \"%d\" > /dev/null",9911,GLOBAL_CONTROLLER_NODE,args.id);
                printf(" Sending notification %s \n",command);
                execute_command(command,0);   
             }
             
             pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
             local_pending=GLOBAL_pending;
             
             // Updates the allocation tables            
             n=args.id;
             printf("  Terminated application: %s, id: %d, Port1 (listener): %d Port2 (Sender): %d\n",GLOBAL_app[n].appname,n,GLOBAL_app[n].port1,GLOBAL_app[n].port2);

             for(m=0;m<GLOBAL_app[n].nhclasses;m++){
                avail=-1;
                for(i=0;i<GLOBAL_nhclasses;i++){
                    if(strcmp(GLOBAL_app[n].hclasses[m],GLOBAL_node.hclasses[i])==0){
                        GLOBAL_node.used_cores[i]-=GLOBAL_app[n].nprocs_class[m];
                        avail=GLOBAL_node.used_cores[i];
                    }
                }
                
                printf(" \t \t %s \t totcores: \t %d \t usedcores: \t %d \t approcs: \t %d \n",GLOBAL_app[n].hclasses[m],GLOBAL_app[n].ncores_class[m],avail,-GLOBAL_app[n].nprocs_class[m]);
                GLOBAL_app[n].nprocs_class[m]=0;
                GLOBAL_app[n].newprocs_class[m]=0;
                GLOBAL_app[n].terminated=1;
             }             
             
             pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);  
                
             if(GLOBAL_EARLYTERMINATION){
                 // Terminates all applications when the first one terminates
                 printf("\n *** The first application has terminated, terminating the remaining ones. \n\n");
                 killall(-1); // Terminates all running applications
                 sleep(15);
                 printstats();
                 exit(1); 
             }
             else if(local_terminated_app>=GLOBAL_napps && local_pending==0 && !GLOBAL_forwarding)
             {
                 
                  printf("\n *** All applications have terminated.... exiting \n\n");
                  sleep(10);          
                  killall(-1); // Terminates all the applications
                  sleep(5);
                  printstats();

                  if(0){
                      for (n=0;n<GLOBAL_napps;n++){
                            /* Open the command for reading. */
                            printf(" *** Application %d log:: \n",n);
                            sprintf(bashcmd,"cat %s/FlexMPI/controller/logs/output%d | grep \"Jacobi fini\" ",HOME,n+1  );

                            fp = popen(bashcmd, "r");
                            if (fp == NULL) {
                                printf("Failed to run command\n" );
                                exit(1);
                            }

                            /* Read the output a line at a time - output it. */
                            while (fgets(path, sizeof(path)-1, fp) != NULL) {
                                printf("   %s \n", path);
                            }

                            /* close */
                            pclose(fp);
                            sleep(5);
                      }
                  }
                  exit(1);
             }
            } 
            else if(strncmp(buf, "ACQIO", 5)==0)
            {
                id=args.id;
                
                /* gets the amount of written data */
                token = strtok_r(buf, " ",&saveptr2);
                if(token!=NULL)
                {
                    token = strtok_r(NULL, " ",&saveptr2);
                    datasize=atof(token);
                    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);   
                    GLOBAL_IOsize[id]=datasize;
                    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);                

                }
                    
                gettimeofday(&timestamp1, NULL);
                delta_t=(timestamp1.tv_sec - initial.tv_sec) * 1000 + (timestamp1.tv_usec - initial.tv_usec) / 1000;
                
                if(GLOBAL_DEBUG) printf("  [ %f ] IO request from %d with %f bytes of data\n",(double)delta_t/1000,id,datasize);            
                
                // Only active when I/O operations are scheduled
                if(GLOBAL_SCHEDULING_IO){
                    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);                
                    local_reconf=GLOBAL_RECONF;
                    if(GLOBAL_busyIO){
                        GLOBAL_reqIO[id]=1; // Request is queued
                        busyIO=1;
                    }
                    else{  
                        GLOBAL_busyIO=1; // Acquires
                        GLOBAL_reqIO[id]=0;
                        busyIO=0;
                    }
                    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);

                    if(local_reconf){
                        gettimeofday(&timestamp1, NULL);
                        
                        flag2=0;
                        while(busyIO && GLOBAL_TERMINATION==0){
                            if(flag2) usleep(10000);
                            flag2=1;
                            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                            if(GLOBAL_reqIO[id]==2){
                                busyIO=GLOBAL_busyIO;
                                if(busyIO==0) GLOBAL_busyIO=1; // Acquires
                                else GLOBAL_reqIO[id]=1;       // Queues
                            }
                            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                            
                            // IO Timeout for preventing packet loss
                            gettimeofday(&timestamp2, NULL);
                            delta_t=(timestamp2.tv_sec - timestamp1.tv_sec) * 1000 + (timestamp2.tv_usec - timestamp1.tv_usec) / 1000;
                            if(((double)delta_t)/1000>120){
                                printf("  *****************************************************************************\n");                        
                                printf("  *** Warning: maximum IO wait time reached. Releasing IO for application %d \n",id);
                                printf("  *****************************************************************************\n");                        
                                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                                busyIO=0;
                                GLOBAL_reqIO[id]=2;
                                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                            } 
                            
                        }
                        gettimeofday(&timestamp2, NULL);
                        delta_t=(timestamp2.tv_sec - timestamp1.tv_sec) * 1000 + (timestamp2.tv_usec - timestamp1.tv_usec) / 1000;
                        tmp_dt=(double)((timestamp2.tv_sec - initial.tv_sec)*1000  + (timestamp2.tv_usec - initial.tv_usec)/ 1000)/1000;
                        if(delta_t>0) printf("          E1: [ %f ] Delaying IO request of %d in %f secs. \n",tmp_dt,id,(double)delta_t/1000);
                        
                        // Records the delay value
                        delay_v[cnt_delay[id]][id]=(double)delta_t/1000;
                        delay_t[cnt_delay[id]][id]=tmp_dt;
                        cnt_delay[id]++;
                        
                        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                        GLOBAL_delayt1[id]=(double)delta_t/1000;
                        GLOBAL_delayt2[id]=(double)delta_t/1000;
                       
                        GLOBAL_reqIO[id]=3;
                        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                    }
                }
                
        
                if(GLOBAL_DEBUG){
                    gettimeofday(&timestamp1, NULL);
                    delta_t = (timestamp1.tv_sec - initial.tv_sec) * 1000 + (timestamp1.tv_usec - initial.tv_usec) / 1000;
                    printf("  [ %f ] Acquiring IO request from %d \n",(double)delta_t/1000,id);            
                }
                
                // Acknowledges the ACQIO request             
                appAddr.sin_family = AF_INET;
                appAddr.sin_port = htons(GLOBAL_app[id].port1);
                if ( (he = gethostbyname(GLOBAL_app[id].node) ) == NULL ) {
                    diep("Error resolving application name1");
                }
                    
                appAddr.sin_addr=*(struct in_addr *) he->h_addr;
                memset(appAddr.sin_zero, '\0', sizeof(appAddr.sin_zero));  
                addr_size = sizeof(appAddr); 
            
                sprintf(appcmd,"10:1");
            
                // Sends the connection command to the application
                sendto(s,appcmd,strlen(appcmd),0,(struct sockaddr *)&appAddr,addr_size);  
                    
            }
            else if(strcmp(buf,"RELQIO")==0)
            {
                if(GLOBAL_DEBUG){
                    gettimeofday(&timestamp1, NULL);
                    delta_t=(timestamp1.tv_sec - initial.tv_sec) * 1000 + (timestamp1.tv_usec - initial.tv_usec) / 1000;
                    printf("  [ %f ] Releasing IO request from %d \n",(double)delta_t/1000,id);            
                }
                
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                GLOBAL_busyIO=0;
                delta_t=(timestamp1.tv_sec - initial.tv_sec) * 1000 + (timestamp1.tv_usec - initial.tv_usec) / 1000;
                           
                // Calls the I/O scheduler
                IO_scheduler();
                     
                if(GLOBAL_DEBUG){
                    gettimeofday(&timestamp1, NULL);
                    delta_t=(timestamp1.tv_sec - initial.tv_sec) * 1000 + (timestamp1.tv_usec - initial.tv_usec) / 1000;
                    printf("  [ %f ] Released IO request from %d granted : ",(double)delta_t/1000,id);
                    for(n=0;n<GLOBAL_napps;n++) printf(" %d ",GLOBAL_reqIO[n]);
                    printf("\n");
                }
                
                GLOBAL_reqIO[id]=0;
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
        
            }
            else{
                
                id=args.id;
                // Monitor connection
                
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                if(0 && GLOBAL_app[id].monitor==1){
                   serverAddr.sin_port = htons(GLOBAL_app[id].port3);
                   memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));  
                   addr_size = sizeof(serverAddr); 
                   GLOBAL_app[id].monitor=2;
                   
                  // Sends the connection command
                  sendto(s,buf,strlen(buf),0,(struct sockaddr *)&serverAddr,addr_size);
                }
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                

                // ****************
               
                
                //printf("               App idd %d sends data from IP %s:%d --> %s",args->id, inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);
                if(strlen(buf)>12 && buf[0]=='F' && buf[1]=='L' && buf[2]=='X'){
                    token = strtok_r(buf, " ",&saveptr3);
                    if(token!=NULL)  token = strtok_r(NULL, " ",&saveptr3);
                    
                          token = strtok_r(NULL, " ",&saveptr3);
                          token = strtok_r(NULL, " ",&saveptr3);
                          rtime = atof(token);
                          token = strtok_r(NULL, " ",&saveptr3);
                          token = strtok_r(NULL, " ",&saveptr3);
                          ptime = atof(token);
                          token = strtok_r(NULL, " ",&saveptr3);
                          token = strtok_r(NULL, " ",&saveptr3);
                          ctime = atof(token);
                          token = strtok_r(NULL, " ",&saveptr3);
                          token = strtok_r(NULL, " ",&saveptr3);
                          mflops= atof(token);
                          token = strtok_r(NULL, " ",&saveptr3);
                          strcpy(nhwpc_1,token);
                          token = strtok_r(NULL, " ",&saveptr3);
                          count1= atof(token);
                          token = strtok_r(NULL, " ",&saveptr3);
                          strcpy(nhwpc_2,token);                     
                          token = strtok_r(NULL, " ",&saveptr3);
                          count2= atof(token);
                          token = strtok_r(NULL, " ",&saveptr3);
                          token = strtok_r(NULL, " ",&saveptr3);
                          iotime= atof(token);
                          token = strtok_r(NULL, " ",&saveptr3);
                          token = strtok_r(NULL, " ",&saveptr3);
                          size= atof(token);
                          token = strtok_r(NULL, " ",&saveptr3);
                          token = strtok_r(NULL, " ",&saveptr3);
                          diter= atof(token);
                          pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                          
                          if(rtime!=0){  // Only counts if nnz values are obtained (real measurement)
                              perf_metrics[id][cnt_perf[id]].metric[0]=rtime;
                              perf_metrics[id][cnt_perf[id]].metric[1]=ptime;
                              perf_metrics[id][cnt_perf[id]].metric[2]=ctime;
                              perf_metrics[id][cnt_perf[id]].metric[3]=iotime;
                              perf_metrics[id][cnt_perf[id]].metric[4]=mflops;
                              perf_metrics[id][cnt_perf[id]].metric[5]=count1;
                              perf_metrics[id][cnt_perf[id]].metric[6]=count2;
                              perf_metrics[id][cnt_perf[id]].metric[7]=size;
                              perf_metrics[id][cnt_perf[id]].metric[8]=diter;
                              strcpy(perf_metrics[id][cnt_perf[id]].nhwpc_1,nhwpc_1);
                              strcpy(perf_metrics[id][cnt_perf[id]].nhwpc_2,nhwpc_2);
                              cnt_perf[id]++;
                              if(cnt_perf[id]>NUMSAMPLES){ // Maximum number of event
                                   killall(-1); // Terminates all the applications
                                   printf(" Maximum entry of cnt_perf buffer reached... exitting  \n \n");
                                   exit(1);
                              }
                          }
                          pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                          
                        // Sends data to ElasticSearch
                        if(rtime!=0 && 0){  // Only for real measurenments
                            struct tm tm2 = *localtime(&t);
                            sprintf(time_stamp,"%d/%d/%d %d:%d:%d", tm2.tm_year + 1900, tm2.tm_mon + 1,tm2.tm_mday, tm2.tm_hour, tm2.tm_min, tm2.tm_sec);
                            //strcpy(nodename,GLOBAL_app[id].rclasses[0]);
                            //strcpy(nodename,"compute-11-4");

                            strcpy(nodename,"test");
                            printf("\n Sending %s to %s --- %s \n",time_stamp,ip_es,nodename);
                            
                            // curl_es_app(curl, ip_es, nodename, id, time_stamp, rtime, ptime, ctime, mflops, "counter1", count1, "counter2", count2, iotime, size, diter);
                            
                            printf("             App %d metrics:  %.2f %.2f %.2f %.2e %s\t%.2e %s\t%.2e %.2f Iter: %d NP: %d\n",args.id,rtime,ptime,ctime,mflops,nhwpc_1,count1,nhwpc_2,count2,iotime,(int)diter,(int)size);
                        }
                        
                 }    
            }
        }
    }
}

// Returns the id of the first created application
// When nump>0 the processes greater than procpernode are created dynamically
// When nump=0 all the processes are created statically
void init_app(char *initMsg, int nump){
    int last_GLOBAL_napps,n,m,i,j,k,avail;
    struct addrinfo hints;
    struct arg_struct1 args1[MAX_APPS];
    struct arg_struct3 args3[MAX_APPS];
    int rc;  // return code
    pthread_attr_t attr[MAX_APPS*2+8],attr3[MAX_APPS];
    pthread_t thread[MAX_APPS*2+8],thread3[MAX_APPS];
    char counters[1024];
    int currp,currp_copy,deltap,exclusive,deploy,err;
    int termi;
    char command[1024],output[1024],tmpstring[512];
          
    if(nump==0){
        deploy=1;
    }
    else{
        deploy=0;
    }
        
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);           
    last_GLOBAL_napps=GLOBAL_napps;
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
    Parse_malleability(NULL,"../run/nodefile2.dat",initMsg,1,deploy);
    
    
    // Initialize shared variables. Threads not created yet.
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
    for(n=last_GLOBAL_napps;n<MAX_APPS;n++){
        cnt_app[n]=0;
        cnt_delay[n]=0;
        update[n]=0;
        p_tlong[n]=0;
        for(m=0;m<STAMPSIZE;m++){
            p_tstamp[m][n]=0;

        }
    }
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
        
    // Configures the socket
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_ADDRCONFIG;
    
    // Creates listener threads
    printf("\n \n --- Creating the listener threads \n");
    for(n=last_GLOBAL_napps;n<GLOBAL_napps;n++){ 
        args1[n].id = n;
        args1[n].port2 = GLOBAL_app[n].port2;
        rc = pthread_attr_init(&attr[n]);
        check_posix_return(rc, "  Initializing attribute");
        rc = pthread_create(&thread[n], &attr[n], (void*)&command_listener,(void *)(&args1[n]));
        check_posix_return(rc, "  Creating listener thread ");
        if(GLOBAL_EXEC!=0) sleep(5); 
    }
               
    // Launches applications 
    GLOBAL_napps=last_GLOBAL_napps;
    Parse_malleability(NULL,"../run/nodefile2.dat",initMsg,0,deploy); 

    int error;
    
    if(nump>0){
        // Waits until the initial processes have been created.
        // Only works when one application is created
        n=last_GLOBAL_napps;
        do{
            send_message("4:on",n,0,30);     
            currp=0;
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock); 
            if(cnt_perf[n]>1) currp=perf_metrics[n][cnt_perf[n]-1].metric[7];
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);    
            printf("      App [%d] Waiting1 until the processes are dynamically created.\n",n);
        }
        while(currp==0 && GLOBAL_terminated_app_id[n]==0);
            
        currp_copy=currp;
        
        // Dynamically creates the remaining processes
        do{
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
            // Exclusive node placement
            error=allocnodes(last_GLOBAL_napps,nump-currp, command, 0, 0, 2);
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
            if(error!=-1){
                send_message(command,last_GLOBAL_napps,0,30);
            }
            else{
                printf(" Error init_app: not enough resources for creating processes to application \n");
                sleep(60);
            }
        }while(error==-1);
        
        // Waits until nump processes have been created.
        do{
            currp=0;
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock); 
            if(cnt_perf[n]>1) currp=perf_metrics[n][cnt_perf[n]-1].metric[7];
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
            printf("      App [%d] Waiting2 until the remaining processes are dynamically created. Current: %d \n",n,currp); 
            sleep(10);
        }
        while(currp<nump && GLOBAL_terminated_app_id[n]==0);    
        
        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock); 
        n=last_GLOBAL_napps;    
        if(last_GLOBAL_napps==GLOBAL_napps-1){
            GLOBAL_app[n].nprocs=nump;
        }
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);    
        
    }
    
    if(nump==0) sleep(3);
        
    // Prints the application execution environment
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);    
    for(n=last_GLOBAL_napps;n<GLOBAL_napps;n++){
        printf("\n ###  Application name: %s, id: %d, Port1 (listener): %d Port2 (Sender): %d  Size: %d\n",GLOBAL_app[n].appname,n,GLOBAL_app[n].port1,GLOBAL_app[n].port2,GLOBAL_app[n].nprocs);
        for(m=0;m<GLOBAL_app[n].nhclasses;m++){
            avail=-1;
            for(i=0;i<GLOBAL_nhclasses;i++){
                if(strcmp(GLOBAL_app[n].hclasses[m],GLOBAL_node.hclasses[i])==0){
                    exclusive=GLOBAL_node.exclusive[i];
                    avail=GLOBAL_node.used_cores[i];
                }
            }
            strcpy(output,"");
            for(i=0;i<GLOBAL_napps;i++){
                for(k=0;k<GLOBAL_app[i].nhclasses;k++){
                    if(strcmp(GLOBAL_app[n].hclasses[m],GLOBAL_app[i].hclasses[k])==0 && GLOBAL_app[i].nprocs_class[k]!=0){
                        sprintf(tmpstring,"[%d:%d:%s]",i,GLOBAL_app[i].nprocs_class[k],GLOBAL_app[i].appname);
                        strcat(output,tmpstring);
                    }
                }
            }
            printf(" ###   \t \t %s \t Exc.: \t %.0d \t Tot: \t %.0d \t Used: \t %.0d \t App \t %.0d \t Name %s\n",GLOBAL_app[n].hclasses[m],exclusive,GLOBAL_app[n].ncores_class[m],avail,GLOBAL_app[n].nprocs_class[m],output);
        }
    }
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);    
    fflush(stdout);
    
    system("date");
    //strcpy(counters,"PAPI_L3_TCM");
    strcpy(counters,"");
    
    
    printf("\n \n --- Creating the monitor threads \n");
    
    if(1){
        for(n=last_GLOBAL_napps;n<GLOBAL_napps;n++){ 
            args3[n].id = n;
            strcpy(args3[n].counter_list,counters);
            args3[n].new_perf_metrics = init_perf_metrics;
            args3[n].init=1;
            rc = pthread_attr_init(&attr3[n]);
            check_posix_return(rc, "Initializing attribute");
            rc = pthread_create(&thread3[n], &attr3[n], (void*)&application_monitor ,(void *)(&args3[n]));
            check_posix_return(rc, "Creating application-monitor thread ");
            sleep(5); 
        }
        
        // Wait for the monitor completion
        if(GLOBAL_EXEC)
        {    
         for(n=last_GLOBAL_napps;n<GLOBAL_napps;n++){   
                (void) pthread_join(thread3[n], NULL);
         }
        }
    }
    
    // Exclusive node placement and removal
    if(nump>0){   
        j=last_GLOBAL_napps;
        
        deltap=nump-currp_copy;
        
        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
        // Removes the processes from the exclusive nodes       
        err=allocnodes(j,deltap, command, 0, 0, 0);
        
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
        printf("   App [%d] in Init_app....  DeltaP = %d at node %s\n",n,deltap,command);
       
        // Creates the specified number of processes
        if(err==-1){
            printf(" *** Error batch_scheduler1: not enough processes for %d \n",j); 
        }
        else{
            send_message(command,j,0,0);     
            clean_nodes(command);
        }
        
        // Waits until nump processes have been created.
        do{
            send_message("4:on",j,0,30);     
            currp=0;
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock); 
            if(cnt_perf[j]>1){
               currp=perf_metrics[j][cnt_perf[j]-1].metric[7];               
            } 
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
            printf("   App [%d] in Init_app2.... waiting in main until the remaining processes are dynamically created. Current: %d %d\n",j,currp,nump+deltap);       
        }
        while(currp!=nump+deltap && GLOBAL_terminated_app_id[j]==0);  

        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);     
        
        // Puts the processes from the shared nodes
        err=allocnodes(j,-deltap, command, 0, 0, 2);
        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
        // Creates the specified number of processes
        if(err==-1){
            printf(" *** Error batch_scheduler2: not enough processes for %d \n",j); 
        }
        else{
            send_message(command,j,0,0);     
            clean_nodes(command);
        }
        
        // Waits until nump processes have been created.
        do{
            send_message("4:on",j,0,30);     
            currp=0;
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock); 
            if(cnt_perf[j]>1){
               currp=perf_metrics[j][cnt_perf[j]-1].metric[7];               
            } 
            termi=GLOBAL_terminated_app_id[j];
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
            printf("   App [%d] in Init_app3.... waiting in main until the remaining processes are dynamically created. Current: %d \n",j,currp); 
                
        }
        while(currp!=nump && termi==0);    
        
        send_message("4:off",j,0,30);                    
    }
    
    j=last_GLOBAL_napps;
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);           
    GLOBAL_app[j].initiated=1;         
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);           
    
}



// Main program        
int main (int argc, char** argv)
{
    int i,j,n,m;
    char initMsg[500];
    char stringport[100];
    time_t rawtime;
    time (&rawtime);
    int length;
    char counters[1024],resultnode[512];
    struct addrinfo hints;
    struct addrinfo* res=0;
    int initialSocket,err;
    int result[2];

    // Get the initial time value
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
    gettimeofday(&initial, NULL);
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);

    // Monitoring threads
    int rc;  // return code
    pthread_t thread2,thread3;
    pthread_attr_t attr2,attr3;
    pthread_t mthread1,mthread2;
    //pthread_attr_t mattr;
    
    printf("\n \n **************************************************************** \n");
    printf("      FlexMPI external controller 3.1\n");
    printf("\n \n **************************************************************** \n");
    
    printf("\n \n --- Initializing\n");
    system("date");
    // Home capture
    const char *name = "HOME";
    HOME=getenv(name);
    
    // Checks that the current node is the same as the one stored in controller.dat
    check_node();
    
    // Captures ctrl+c signal and exists killing all the apps
    signal(SIGINT, intHandler);

    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
    
    for(n=0;n<MAX_APPS;n++){
        GLOBAL_reqIO[n]=0;
        GLOBAL_IOsize[n]=0;
        GLOBAL_delayt1[n]=0;
        GLOBAL_delayt2[n]=0;
        GLOBAL_terminated_app_id[n]=0;
    }
    
    GLOBAL_TERMINATION=0;
    GLOBAL_RECONF=1;
    GLOBAL_busyIO=0;
    GLOBAL_napps=0;
    GLOBAL_DYNAMIC_ALLOC=1;
    GLOBAL_nhclasses=0;
    
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-nomalleable") == 0) {
            GLOBAL_RECONF=0;
        }
    }
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
    
    if(GLOBAL_RECONF==1) printf("\n   # Application malleability enabled \n");
    else                 printf("\n   # Application malleability disabled \n\n\n");

    // if flag -noexecute is present: only generates exec scripts (it does not execute the application)
    GLOBAL_EXEC=1;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-noexecute") == 0) {
            GLOBAL_EXEC=0;
        }
    }
    if(GLOBAL_EXEC==0) printf("\n   # Generating the execution scripts but not running the application \n");
    
    // if flag -adhoc is present: only spawns the given number of processes and then exits. 
    GLOBAL_ADHOC=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-adhoc") == 0 && GLOBAL_EXEC==1 && GLOBAL_RECONF==1) {
            GLOBAL_ADHOC=atoi(argv[n+1]);
        }
    }
    if(GLOBAL_ADHOC>0) printf("\n   # Performing an ad-hoc dynamic configuration and exiting.... \n");
    
    GLOBAL_monitoring=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-monitoring") == 0 && GLOBAL_EXEC==1) {
            GLOBAL_monitoring=1;
        }
    }
    if(GLOBAL_monitoring==1) printf("\n   # Activating global monitoring.... \n");
    
    GLOBAL_DEBUG=0;
     for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-debug") == 0) {
            GLOBAL_DEBUG=1;
        }
    }
    if(GLOBAL_DEBUG==1) printf("\n   # Activating debug mode.... \n");
   
    
    GLOBAL_PREDEFSPEEDUP=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-predefinedspeedup") == 0) {            
            GLOBAL_PREDEFSPEEDUP=1;
        }
    }
    if(GLOBAL_PREDEFSPEEDUP==1) printf("\n   # Application becnhmarking not activated \n");

    
    GLOBAL_RECORDEDSPEEDUP=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-recordedspeedup") == 0) {            
            GLOBAL_RECORDEDSPEEDUP=1;
            strcpy(GLOBAL_FILE,argv[n+1]);
        }
    }
    if(GLOBAL_RECORDEDSPEEDUP==1) printf("\n   # Speedup loaded from file %s \n",GLOBAL_FILE);

    GLOBAL_SCHEDULING_ALGORITHM=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-scheduling_algorithm") == 0) {            
            GLOBAL_SCHEDULING_ALGORITHM=atoi(argv[n+1]);;
        }
    }  
    if(GLOBAL_SCHEDULING_ALGORITHM>0) printf("\n   # I/O scheduling algorithm %d selected. \n",GLOBAL_SCHEDULING_ALGORITHM);
    
    GLOBAL_RECORDEDEXECTIMES=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-recordedexectime") == 0) {            
            GLOBAL_RECORDEDEXECTIMES=1;
            strcpy(GLOBAL_FILE,argv[n+1]);
        }
    }
    if(GLOBAL_RECORDEDEXECTIMES==1) printf("\n   # Execution times loaded from file %s \n",GLOBAL_FILE);

    GLOBAL_MAXTHROUGHPUT=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-maxthroughput") == 0) {            
            GLOBAL_MAXTHROUGHPUT=1;
        }
    }
    if(GLOBAL_MAXTHROUGHPUT==1) printf("\n   # Maximum throughput policy activated \n");

    GLOBAL_CLARISSEONLY=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-clarisseonly") == 0) {            
            GLOBAL_CLARISSEONLY=1;
        }
    }
    if(GLOBAL_CLARISSEONLY==1) printf("\n   # Running only with Clarisse's I/O Scheduling. Malleability disabled \n\n\n");

    
    if(GLOBAL_RECORDEDSPEEDUP+GLOBAL_RECORDEDEXECTIMES>1){
        diep("Error: flags -recordedspeedup and -recordedexectime cannot be used at the same time \n");
    } 
        
    GLOBAL_SYNCIOPHASES=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-synciophases") == 0) {            
            GLOBAL_SYNCIOPHASES=1;
        }
    }
    if(GLOBAL_SYNCIOPHASES==1) printf("\n   # I/O period phases adjusted \n");

    GLOBAL_SCHEDULING_IO=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-scheduling_io") == 0) {            
            GLOBAL_SCHEDULING_IO=1;
        }
    }
    
    if(GLOBAL_SCHEDULING_IO==1) printf("\n   # I/O scheduling activated. \n");
    else                        printf("\n   # I/O not scheduled \n");
    
    GLOBAL_FAIRSHEDULING=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-fair_scheduling") == 0) {            
            GLOBAL_FAIRSHEDULING=1;
            GLOBAL_FAIRSCHEDULING_NP=atoi(argv[n+1]);
        }
    }
    
    if(GLOBAL_FAIRSHEDULING==1 && GLOBAL_FAIRSCHEDULING_NP==-1) printf("\n   # Fair scheduler activated using all the available processors \n");
    if(GLOBAL_FAIRSHEDULING==1 && GLOBAL_FAIRSCHEDULING_NP>=0)  printf("\n   # Fair scheduler activated using %d processors \n",GLOBAL_FAIRSCHEDULING_NP);

    GLOBAL_SPEEDUPSCHEDULING=0;
     for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-speedup_scheduling") == 0) {            
            GLOBAL_SPEEDUPSCHEDULING=1;
            GLOBAL_SPEEDUPSCHEDULING_NP=atoi(argv[n+1]);
        }
    }
    if(GLOBAL_SPEEDUPSCHEDULING==1 && GLOBAL_SPEEDUPSCHEDULING_NP==-1) printf("\n   # Speedup-based scheduler activated using all the available processors \n");
    if(GLOBAL_SPEEDUPSCHEDULING==1 && GLOBAL_SPEEDUPSCHEDULING_NP>=0)  printf("\n   # Speedup-based scheduler activated using %d processors \n",GLOBAL_SPEEDUPSCHEDULING_NP);
    
    GLOBAL_IOSCHEDULING=0;
    GLOBAL_IOSCHEDULING_THRESHOLD=-1;
     for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-io_scheduling") == 0) {            
            GLOBAL_IOSCHEDULING=1;
            GLOBAL_IOSCHEDULING_THRESHOLD=atof(argv[n+1]);
            if(GLOBAL_IOSCHEDULING_THRESHOLD<0){
                diep("Error with -io_scheduling val argument. Value should be within the interval [0,1]");
            }
        }
    }
    if(GLOBAL_IOSCHEDULING==1) printf("\n   # IO-based scheduler activated \n");
 
    GLOBAL_EARLYTERMINATION=0;
     for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-earlytermination") == 0) {            
            GLOBAL_EARLYTERMINATION=1;
        }
    }
    if(GLOBAL_EARLYTERMINATION==1) printf("\n   # Early termination activated \n");
    
    GLOBAL_disable=0;
     for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-disable") == 0) {            
            GLOBAL_disable=1;
        }
    }
    if(GLOBAL_disable==1) printf("\n   # No optimization applied \n");
      
    if(GLOBAL_IOSCHEDULING+GLOBAL_SPEEDUPSCHEDULING+GLOBAL_FAIRSHEDULING+GLOBAL_CLARISSEONLY+GLOBAL_MAXTHROUGHPUT+GLOBAL_SYNCIOPHASES>1){
        diep("Error: flags -clarisseonly -maxthroughput and -synciophases cannot be used at the same time \n");
    } 

    GLOBAL_GUI=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-GUI") == 0) {            
            GLOBAL_GUI=1;
            strcpy(GUI_NODE,argv[n+1]);
            GLOBAL_GUI_PORT=atoi(argv[n+2]);
            GLOBAL_GUI_ListenerPort=atoi(argv[n+3]);;
        }
    }
    if(GLOBAL_GUI==1) printf("\n   # Graphic user interface node located in %s with port1: %d \n",GUI_NODE,GLOBAL_GUI_PORT);
    
    // Activates forwarding. The 8500 port is used to receive messages that are forwarded to the applications
    GLOBAL_forwarding=0;
    for (n = 0; n < argc; n ++) {
        if (strcmp(argv[n], "-forwarding") == 0) {            
            GLOBAL_forwarding=1;
        }
    }
    if(GLOBAL_forwarding==1) printf("\n   # Controller running in forwarding mode \n");    
    
    // Variable initialization
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
    
    GLOBAL_epoch[0]=0;
    cnt_epoch=2; // Points to the next-to-used entry
    
    for(n=0;n<MAX_APPS;n++)
    {
     cnt_perf[n]=0;
     cnt_speedup1[n]=0;
     cnt_speedup2[n]=0;
     flag_speedup[n]=0; 
    }     
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
    
    printf("\n Entering execution phase ..... \n\n");
       
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
    terminated_app=0;  // Final value is the number of applications
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);    // Creates one thread per application
    

    printf("\n Waiting to execute the application.... \n");

    
    // Sets the initial time value (for the new application)
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
    gettimeofday(&initial, NULL);
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
       
    // Prints the application execution environment   
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
    printf("\n \n --- Displaying the application workload \n");
    for(n=0;n<GLOBAL_napps;n++){
        printf("  Application name: %s, id: %d, Port1 (listener): %d Port2 (Sender): %d\n",GLOBAL_app[n].appname,n,GLOBAL_app[n].port1,GLOBAL_app[n].port2);
        for(m=0;m<GLOBAL_app[n].nhclasses;m++){
            printf(" \t \t %s \t ncores: \t %d \t nprocs \t %d\n",GLOBAL_app[n].hclasses[m],GLOBAL_app[n].ncores_class[m],GLOBAL_app[n].nprocs_class[m]);
        }
    } 
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
    
    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);    
    for(n=0;n<MAX_APPS;n++)
    {
     cnt_perf[n]=0;
     cnt_app[n]=0;
     update[n]=0;
    }
    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
 
    // Monitor-connection thread
    if(!GLOBAL_disable && GLOBAL_EXEC){
        rc = pthread_create(&mthread1, NULL, (void*)&monitor_listener,NULL);
        check_posix_return(rc, "Creating monitor listener thread ");
        rc = pthread_create(&mthread2, NULL, (void*)&monitor_analyzer,NULL);
        check_posix_return(rc, "Creating monitor analyzer thread ");
    }
    
    // Batch-scheduler thread
    if(GLOBAL_EXEC){
        rc = pthread_attr_init(&attr2);
        check_posix_return(rc, "Initializing attribute");
        rc = pthread_create(&thread2, &attr2, (void*)&batch_scheduler,NULL);
        check_posix_return(rc, "Creating bath-scheduler thread ");
    }
    
    //  Forwarding thread
    if(GLOBAL_forwarding){ 
        rc = pthread_attr_init(&attr3);
        check_posix_return(rc, "  Initializing attribute");
        rc = pthread_create(&thread3, &attr3, (void*)&forwarder,NULL);
        check_posix_return(rc, "  Creating forwarder thread ");
    }  
    
    strcpy(counters,"PAPI_L3_TCM"); 
   
    // Work
    
    double dtmp;
    char command1[3000];
    int test=0,lapp_class1,lapp_class2,lapp_profile1,lapp_profile2,limit;
    int deltap,currp,nump,cnt;
    int last_GLOBAL_napps,new_GLOBAL_napps;
                 
    if(0){    
        sleep(60);      
        for(j=0;j<42;j+=2){
            fflush(stdout);       
            // Waits for the application monitor to get the initial metrics
            dtmp=0;
            while(dtmp==0){
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);      
                dtmp=init_perf_metrics[j].metric[0];
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                sleep(5);
            }
            
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);               
            lapp_class1=GLOBAL_app[j].app_class;
            lapp_class2=GLOBAL_app[j+1].app_class;
            lapp_profile1=GLOBAL_app[j].app_profile;
            lapp_profile2=GLOBAL_app[j+1].app_profile;
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock); 

            // Allocates 8
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
            err=allocnodes(j+1,8, resultnode, 0, 0, 0);
            if(err==0){
                sprintf(command1,"nping --udp -g 5000 -p %d  -c 1 %s --data-string \"%s\">/dev/null",GLOBAL_app[j+1].port1,GLOBAL_app[j+1].node,resultnode);
                printf("\t*** %s \n",command1);
                system(command1);
             }
             else{
                  printf(" *** Error batch_scheduler: not enough processes for %d \n",j+1); 
            }
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);

                    
                    
            if(lapp_class1==5 && lapp_profile1==0) send_message("iocmd:5", j, 1, 60);
            if(lapp_class2==5 && lapp_profile2==0) send_message("iocmd:5", j+1, 1, 60);
            if(lapp_class2==5) limit=9;
            else               limit=6;
            sleep(30);
            
            for(i=6;i<=limit;i++){
                printf("\n\n\n\n   ************************************************ ");
                printf("\n # Running:   test # %d \n",test);
                printf(" # Executing: applications %d and %d with profile %d \n",j,j+1,i);
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);      
                printf(" # App %d ::  %s\n",j,GLOBAL_app[j].input);
                printf(" # App %d ::  %s\n",j+1,GLOBAL_app[j+1].input);            
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                printf("   ************************************************\n\n\n");
                fflush(stdout);      
                
                if(lapp_class2==5){
                    sprintf(command1,"iocmd:%d",i);
                    send_message(command1, j+1, 1, 0);  
                }
                    
                sleep(60);   
                analyse_node("compute-11-4", 1,result,resultnode);
                sprintf(command1,"nping --udp -p %d -c 1 %s --data-string \"X:TESTZ1-%d-%d-%d-%d\" > /dev/null",5002,"compute-9-1",test,j,j+1,i);
                printf(" 1--> %s \n",command1);
                execute_command(command1,0);   
                sleep(30);
                test++;
            }
            int term1,term2;
            
            do{
                send_message("5:", j+1, 0, 0);   
                sleep(120);
                sprintf(command1,"nping --udp -p %d -c 1 %s --data-string \"X:TESTZ2-%d-%d-%d-%d\" > /dev/null",5002,"compute-9-1",test,j,j+1,i);
                printf(" 2--> %s \n",command1);
                execute_command(command1,0);
                send_message("5:", j, 0, 30);   
                sleep(120);
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                term1=GLOBAL_app[j].terminated;
                term2=GLOBAL_app[j+1].terminated;
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
            } while(term1==0 || term2==0);
            
            printf(" \n\n  ############################################ \n\n");
            fflush(stdout);
            system("echo compute-11-4");
            system("rsh compute-11-4 \"ps -elf -u desingh | grep desingh |grep tmp \"");
            system("echo compute-11-5");
            system("rsh compute-11-5 \"ps -elf -u desingh | grep desingh |grep tmp\"");
        }
           
        printf("\n Ending..... \n\n");
    }
    
    if(0){
        while(!GLOBAL_disable){
            
            pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
            last_GLOBAL_napps=GLOBAL_napps;
            new_GLOBAL_napps=GLOBAL_napps;
            pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
            
            cnt=0;
            while(new_GLOBAL_napps==last_GLOBAL_napps && cnt<10){
                sleep(30);
                pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                new_GLOBAL_napps=GLOBAL_napps;
                pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);               
                cnt++;
            }
            sleep(30);
            
            for(n=0;n<GLOBAL_nhclasses;n++){
                analyse_node(GLOBAL_node.hclasses[n], 1,result,resultnode);
                if(result[0]>=0){
                    j=result[0];

                    deltap=result[1];
                    printf("  *** Applying malleability to application %d :: DeltaP = %d at node %s\n",j,deltap,resultnode);
                    pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock);
                    err=allocnodes(j,deltap, resultnode, 0, 0, 0);
                    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                    nump=GLOBAL_app[j].nprocs;
                    
                    // Creates the specified number of processes
                    if(err==-1){
                        printf(" *** Error batch_scheduler: not enough processes for %d \n",j); 
                    }
                    else{
                        send_message(resultnode,j,0,0);     
                    }
                    
                    // Waits until nump processes have been created.
                    do{
                        send_message("4:on",j,0,30);     
                        currp=0;
                        pthread_mutex_lock(&CONTROLLER_GLOBAL_server_lock); 
                        if(cnt_perf[j]>1) currp=perf_metrics[j][cnt_perf[j]-1].metric[7];
                        pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);   
                        printf("   App [%d] Waiting in main until the remaining processes are dynamically created. Current: %d \n",j,currp);                   
                    }
                    while(currp!=nump && GLOBAL_terminated_app_id[j]==0);  
                    send_message("4:off",j,0,30);     

                
                    err=allocnodes(j,-deltap, resultnode, 0, 0, 1);
                    sleep(60);
                    
                    pthread_mutex_unlock(&CONTROLLER_GLOBAL_server_lock);
                    // Creates the specified number of processes
                    if(err==-1){
                        printf(" *** Error batch_scheduler: not enough processes for %d \n",j); 
                    }
                    else{
                        send_message(resultnode,j,0,0);     
                    }                                    
                }
            }
        }
    }
    

    // Input commands
    sleep(3);
    printf("\n \n  --- Waiting for new input commands \n");
    
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_ADDRCONFIG;
    while(1)
    {
        scanf("%d %s",&n,initMsg); // First value is the application id, second value is the string with the command
        if(n==-1 || strcmp(initMsg,"exit") == 0 || strcmp(initMsg,"quit") == 0 ) 
        {
             killall(-1); // Terminates all the applications
             printf(" Exiting \n \n");
             exit(1);
        }
        if(n==-2){
            parse_command (initMsg, command1, -1);
        }
        if(n==-3){
            allocnodes(1,atoi(initMsg), command1, 0, 0, 0);
            printf(" ***> %s \n",command1);
        }
             
        else{
            
            // Parses the destination application
            sprintf(stringport,"%d",GLOBAL_app[n].port1);
            err=getaddrinfo(GLOBAL_app[n].node,stringport,&hints,&res);

            if (err<0) {
                printf("This error appears when you introduce a void command in the controller \n");
                diep("failed1 to");
            }
            
            // Creates a socket to the destination
            initialSocket=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
            if (initialSocket==-1) {
                diep("Error creating socket.");
            }
            
            // Sends the command
            length = sendto(initialSocket,initMsg,strlen(initMsg),0,res->ai_addr,res->ai_addrlen);
            if (length < 0){
                diep("Error: sendto()");
            }
            
            // Closes the socket
            close(initialSocket);

            printf("Message: %s. Size: %d bytes sent to app%d at port %d in node %s \n",initMsg, (int)sizeof(initMsg),n+1,GLOBAL_app[n].port1,GLOBAL_app[n].node);
            
        }
    }
    
    printf("\n Ending..... \n\n");
    killall(-1);
    return 0;
}   
