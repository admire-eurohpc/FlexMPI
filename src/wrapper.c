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
 *    File:       wrapper.c                                                                                                                *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

/* include */
#include <empi.h>


/****************************************************************************************************************************************
*
*    'MPI_File_write_all'
*
****************************************************************************************************************************************/
int MPI_File_write_all(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status) {
    int err,rank,size;
    double iot2,iot1,local_delayiotime;
    int local_delayio,termination;
    int datatype_size;
    double datasize;
    char socketcmd[1024]; 
    
    MPI_Comm_rank(EMPI_COMM_WORLD,&rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);
    
    // Obtains the amount of data written. Note. As an approximation we assume that all processes write the same amount of data
    MPI_Type_size(datatype,&datatype_size);
    datasize=(double)datatype_size;
    datasize=datasize*count*size;
    
    //capture io
    //if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_ALLGATHER, &sendcount, sendtype);

   
    pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
    EMPI_GLOBAL_delayiotime=-1;
    EMPI_GLOBAL_delayio=1;
    pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);

    EMPI_GLOBAL_tio_ini = MPI_Wtime();
    
    // Sends controller acquire request
    if(rank==0){
        sprintf(socketcmd,"ACQIO: %f",datasize);
        sendto(EMPI_GLOBAL_socket,socketcmd,strlen(socketcmd),0,(struct sockaddr *)&EMPI_GLOBAL_controller_addr,sizeof(EMPI_GLOBAL_controller_addr));     
    }
            
    if(rank==0){
        pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
        local_delayiotime=EMPI_GLOBAL_delayiotime;
        local_delayio=EMPI_GLOBAL_delayio;
        termination=EMPI_GLOBAL_monitoring_data.termination;
        pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
                
        // Detects whether there is I/O delay (only root proc)
        if(local_delayio==1 && local_delayiotime>0 && termination==0){        
            iot1=MPI_Wtime();
            iot2=MPI_Wtime();
            while((iot2-iot1)<local_delayiotime) iot2=MPI_Wtime(); // Delay only applied to the root process        
        }
        
        // Performs active waiting until receiving the command "9:"
        while(local_delayio==1 && local_delayiotime==-1 && termination==0){
            usleep(10000); // Active waiting of 10ms
            pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
            local_delayiotime=EMPI_GLOBAL_delayiotime;
            local_delayio=EMPI_GLOBAL_delayio;
            termination=EMPI_GLOBAL_monitoring_data.termination;
            pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
        }        
        
        pthread_mutex_lock(&EMPI_GLOBAL_server_lock);
        EMPI_GLOBAL_delayiotime=0;
        EMPI_GLOBAL_delayio=0;
        pthread_mutex_unlock(&EMPI_GLOBAL_server_lock);
    }

    MPI_Barrier(EMPI_COMM_WORLD);                           

    //MPI call
    if(EMPI_GLOBAL_dummyIO<0){
        err =  PMPI_File_write_all(fh, buf, count, datatype, status);
    }
    else{
        iot1=MPI_Wtime();
        iot2=MPI_Wtime();
        while((iot2-iot1)<EMPI_GLOBAL_dummyIO) iot2=MPI_Wtime(); // Delaty EMPI_GLOBAL_dummyIO seconds. Active waiting.
        err=0;
    }
   
            
    MPI_Barrier(EMPI_COMM_WORLD);                           
    EMPI_GLOBAL_tio_fin = MPI_Wtime();
    

    EMPI_GLOBAL_tio += (EMPI_GLOBAL_tio_fin - EMPI_GLOBAL_tio_ini);


    // Sends controller release notification
     if(rank==0){
        sprintf(socketcmd,"RELQIO");
        sendto(EMPI_GLOBAL_socket,socketcmd,strlen(socketcmd),0,(struct sockaddr *)&EMPI_GLOBAL_controller_addr,sizeof(EMPI_GLOBAL_controller_addr));     
    }  
    
    // Sends controller I/O stats
    if(rank==0){
              sprintf(socketcmd,"IO %s %d %f %d %f %f",EMPI_GLOBAL_application,EMPI_GLOBAL_iteration,(EMPI_GLOBAL_tio_fin - EMPI_GLOBAL_tio_ini),size,(EMPI_GLOBAL_tio_fin - EMPI_GLOBAL_tio_ini),EMPI_GLOBAL_tio_ini-EMPI_GLOBAL_tio_last);   
               sendto(EMPI_GLOBAL_socket,socketcmd,strlen(socketcmd),0,(struct sockaddr *)&EMPI_GLOBAL_controller_addr,sizeof(EMPI_GLOBAL_controller_addr));     
   }
    
    EMPI_GLOBAL_tio_last = EMPI_GLOBAL_tio_fin;

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Allgatherv'
*
****************************************************************************************************************************************/
//int MPI_Allgatherv (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm) {
int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
                    {

    int err;

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_ALLGATHERV, (int*)recvcounts, sendtype);    
    
    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Allgatherv (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Allgatherv took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Allreduce'
*
****************************************************************************************************************************************/
//int MPI_Allreduce (void *sendbuf,  void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                  MPI_Op op, MPI_Comm comm)
                  {
    int err;

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_ALLREDUCE, &count, datatype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Allreduce (sendbuf, recvbuf, count, datatype, op, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Allreduce took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Alltoall'
*
****************************************************************************************************************************************/
//int MPI_Alltoall (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                  {
    int err;

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_ALLTOALL, &sendcount, sendtype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Alltoall (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Alltoall took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Alltoallv'
*
****************************************************************************************************************************************/
//int MPI_Alltoallv (void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype sendtype, void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype recvtype, MPI_Comm comm) {
int MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                  MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                  const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
                  {
    int err;

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_ALLTOALL, (int*)recvcounts, sendtype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Alltoallv (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Alltoallv took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Alltoallw'
*
****************************************************************************************************************************************/
//int MPI_Alltoallw (void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype *sendtypes, void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm) {
int MPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                  const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                  const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm) {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Alltoallw (sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Alltoallw took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Barrier'
*
****************************************************************************************************************************************/
int MPI_Barrier (MPI_Comm comm) {

    int err;

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_BARRIER, 0, MPI_INT);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Barrier (comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Barrier took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Bcast'
*
****************************************************************************************************************************************/
//int MPI_Bcast (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
               {
    int err;

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_BCAST, &count, datatype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Bcast (buffer, count, datatype, root, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Bcast took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Bsend'
*
****************************************************************************************************************************************/
//int MPI_Bsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Bsend (buf, count, datatype, dest, tag, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Bsend took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Bsend'
*
****************************************************************************************************************************************/
//int MPI_Bsend_init (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
int MPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request)  {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Bsend_init (buf, count, datatype, dest, tag, comm, request);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Bsend took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Gather'
*
****************************************************************************************************************************************/
//int MPI_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm) {
int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
               int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
                {
    int err;

    //FIXME: aqui el size es sendcnt * nprocs

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_GATHER, &sendcount, sendtype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Gather (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Gather took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Gatherv'
*
****************************************************************************************************************************************/
//int MPI_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf, int *recvcnts, int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm) {
int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root,
                MPI_Comm comm)
                 {
    int err;

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_GATHER, (int*)recvcounts, sendtype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Gatherv (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Gatherv took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Ibsend'
*
****************************************************************************************************************************************/
//int MPI_Ibsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request) {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Ibsend (buf, count, datatype, dest, tag, comm, request);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Ibsend took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Iprobe'
*
****************************************************************************************************************************************/
//int MPI_Iprobe (int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status) {
int MPI_Iprobe (int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status) {

    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Iprobe (source, tag, comm, flag, status);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Iprobe took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Irecv'
*
****************************************************************************************************************************************/
//int MPI_Irecv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request) {
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request *request){
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Irecv (buf, count, datatype, source, tag, comm, request);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Irecv took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Irsend'
*
****************************************************************************************************************************************/
//int MPI_Irsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request)  {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Irsend (buf, count, datatype, dest, tag, comm, request);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Irsend took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Isend'
*
****************************************************************************************************************************************/
//int MPI_Isend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm, MPI_Request *request){
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Isend (buf, count, datatype, dest, tag, comm, request);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Isend took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Issend'
*
****************************************************************************************************************************************/
//int MPI_Issend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request)  {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Issend (buf, count, datatype, dest, tag, comm, request);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Issend took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Probe'
*
****************************************************************************************************************************************/
int MPI_Probe (int source, int tag, MPI_Comm comm, MPI_Status *status) {

    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Probe (source, tag, comm, status);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Probe took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Put'
*
****************************************************************************************************************************************/
//int MPI_Put (void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win) {
int MPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
            int target_rank, MPI_Aint target_disp, int target_count,
            MPI_Datatype target_datatype, MPI_Win win) {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Put (origin_addr, origin_count, origin_datatype, target_rank, target_disp, target_count, target_datatype, win);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Put took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Recv'
*
****************************************************************************************************************************************/
//int MPI_Recv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) {
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status) {
    int err;

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_RECV, &count, datatype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Recv (buf, count, datatype, source, tag, comm, status);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Recv took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Reduce'
*
****************************************************************************************************************************************/
//int MPI_Reduce (void *sendbuf,  void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, int root, MPI_Comm comm)
                {
    int err;

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_REDUCE, &count, datatype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Reduce (sendbuf, recvbuf, count, datatype, op, root, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Reduce took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Reduce_scatter'
*
****************************************************************************************************************************************/
//int MPI_Reduce_scatter (void *sendbuf, void *recvbuf, int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
                        {
    int err;

    //FIXME: pendiente.

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Reduce_scatter (sendbuf, recvbuf, recvcounts, datatype, op, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Reduce took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Recv_init'
*
****************************************************************************************************************************************/
//int MPI_Recv_init (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request) {
int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                  MPI_Comm comm, MPI_Request *request) {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Recv_init (buf, count, datatype, source, tag, comm, request);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Recv took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Rsend'
*
****************************************************************************************************************************************/
//int MPI_Rsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm)  {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Rsend (buf, count, datatype, dest, tag, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Rsend took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Rsend_init'
*
****************************************************************************************************************************************/
//int MPI_Rsend_init (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
int MPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request)  {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Rsend_init (buf, count, datatype, dest, tag, comm, request);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Rsend took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Scan'
*
****************************************************************************************************************************************/
//int MPI_Scan (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
             MPI_Comm comm)
             {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Scan (sendbuf, recvbuf, count, datatype, op, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Scan took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Scatter'
*
****************************************************************************************************************************************/
//int MPI_Scatter (void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm) {
int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
                 {
    int err;

    //FIXME: sendcnt hay que coger el de root.

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_SCATTER, &sendcount, sendtype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Scatter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Scatter took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Scatterv'
*
****************************************************************************************************************************************/
//int MPI_Scatterv (void *sendbuf, int *sendcnts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm) {
int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int root, MPI_Comm comm)
                  {
    int err;

    //FIXME: ver que valor coger de sendcnts.

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_SCATTER, (int*)sendcounts, sendtype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Scatterv (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Scatterv took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Send'
*
****************************************************************************************************************************************/
//int MPI_Send (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm) {
    int err;

    //capture communications
    if (EMPI_GLOBAL_capture_comms) EMPI_Capture_comms (MPI_SEND, &count, datatype);

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Send (buf, count, datatype, dest, tag, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Send took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Send_init'
*
****************************************************************************************************************************************/
//int MPI_Send_init (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
int MPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                  MPI_Comm comm, MPI_Request *request) {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Send_init (buf, count, datatype, dest, tag, comm, request);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Send took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Sendrecv'
*
****************************************************************************************************************************************/
//int MPI_Sendrecv (void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status) {
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest,
                 int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int source, int recvtag, MPI_Comm comm, MPI_Status *status)
                 {
    int err;

    //FIXME: pendiente.

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Sendrecv (sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Sendrecv took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Sendrecv_replace'
*
****************************************************************************************************************************************/
//int MPI_Sendrecv_replace (void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status) {
int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest,
                         int sendtag, int source, int recvtag, MPI_Comm comm,
                         MPI_Status *status){
    int err;

    //FIXME: pendiente.

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Sendrecv_replace (buf, count, datatype, dest, sendtag, source, recvtag, comm, status);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Sendrecv took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Ssend'
*
****************************************************************************************************************************************/
//int MPI_Ssend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm)  {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Ssend (buf, count, datatype, dest, tag, comm);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Ssend took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Ssend_init'
*
****************************************************************************************************************************************/
//int MPI_Ssend_init (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
int MPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request)  {
    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Ssend_init (buf, count, datatype, dest, tag, comm, request);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Ssend took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Wait'
*
****************************************************************************************************************************************/
int MPI_Wait (MPI_Request *request, MPI_Status *status) {

    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Wait (request, status);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Wait took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}

/****************************************************************************************************************************************
*
*    'MPI_Waitall'
*
****************************************************************************************************************************************/
int MPI_Waitall (int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]) {

    int err;

    EMPI_GLOBAL_tcomm_ini = MPI_Wtime();

    //MPI call
    err = PMPI_Waitall (count, array_of_requests, array_of_statuses);

    EMPI_GLOBAL_tcomm_fin = MPI_Wtime();

    // if(EMPI_GLOBAL_debug_comms) printf("Communication function PMPI_Waitall took %f seconds\n", EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);
    EMPI_GLOBAL_tcomm += (EMPI_GLOBAL_tcomm_fin - EMPI_GLOBAL_tcomm_ini);

    return err;
}
