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
 *    File:       wrapper.h                                                                                                                *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

#ifndef _EMPI_WRAPPER_H_
#define _EMPI_WRAPPER_H_


/****************************************************************************************************************************************
*
*    'MPI_File_write_all'
*
****************************************************************************************************************************************/
int MPI_File_write_all(MPI_File fh, const void *buf, int count, MPI_Datatype datatype,
                       MPI_Status *status);
                                     
                  
/****************************************************************************************************************************************
*
*    'MPI_Allgather'
*
****************************************************************************************************************************************/
//int MPI_Allgather (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                  ;

/****************************************************************************************************************************************
*
*    'MPI_Allgatherv'
*
****************************************************************************************************************************************/
//int MPI_Allgatherv (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm)
                   ;

/****************************************************************************************************************************************
*
*    'MPI_Allreduce'
*
****************************************************************************************************************************************/
//int MPI_Allreduce (void *sendbuf,  void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                  MPI_Op op, MPI_Comm comm)
                  ;

/****************************************************************************************************************************************
*
*    'MPI_Alltoall'
*
****************************************************************************************************************************************/
//int MPI_Alltoall (void *sendbuf,  int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                 ;

/****************************************************************************************************************************************
*
*    'MPI_Alltoallv'
*
****************************************************************************************************************************************/
//int MPI_Alltoallv (void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype sendtype, void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                  MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                  const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
                  ;

/****************************************************************************************************************************************
*
*    'MPI_Alltoallw'
*
****************************************************************************************************************************************/
//int MPI_Alltoallw (void *sendbuf, int *sendcnts, int *sdispls, MPI_Datatype *sendtypes, void *recvbuf, int *recvcnts, int *rdispls, MPI_Datatype *recvtypes, MPI_Comm comm);
int MPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                  const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                  const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);

/****************************************************************************************************************************************
*
*    'MPI_Barrier'
*
****************************************************************************************************************************************/
int MPI_Barrier (MPI_Comm comm);

/****************************************************************************************************************************************
*
*    'MPI_Bcast'
*
****************************************************************************************************************************************/
//int MPI_Bcast (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
              ;

/****************************************************************************************************************************************
*
*    'MPI_Bsend'
*
****************************************************************************************************************************************/
//int MPI_Bsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) ;

/****************************************************************************************************************************************
*
*    'MPI_Bsend'
*
****************************************************************************************************************************************/
//int MPI_Bsend_init (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request) ;

/****************************************************************************************************************************************
*
*    'MPI_Gather'
*
****************************************************************************************************************************************/
//int MPI_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
               int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
               ;

/****************************************************************************************************************************************
*
*    'MPI_Gatherv'
*
****************************************************************************************************************************************/
//int MPI_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf, int *recvcnts, int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root,
                MPI_Comm comm)
                ;

/****************************************************************************************************************************************
*
*    'MPI_Ibsend'
*
****************************************************************************************************************************************/
//int MPI_Ibsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request) ;

/****************************************************************************************************************************************
*
*    'MPI_Iprobe'
*
****************************************************************************************************************************************/
int MPI_Iprobe (int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);

/****************************************************************************************************************************************
*
*    'MPI_Irecv'
*
****************************************************************************************************************************************/
//int MPI_Irecv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request *request) ;

/****************************************************************************************************************************************
*
*    'MPI_Irsend'
*
****************************************************************************************************************************************/
//int MPI_Irsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request) ;

/****************************************************************************************************************************************
*
*    'MPI_Isend'
*
****************************************************************************************************************************************/
//int MPI_Isend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm, MPI_Request *request) ;

/****************************************************************************************************************************************
*
*    'MPI_Issend'
*
****************************************************************************************************************************************/
//int MPI_Issend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request) ;

/****************************************************************************************************************************************
*
*    'MPI_Probe'
*
****************************************************************************************************************************************/
int MPI_Probe (int source, int tag, MPI_Comm comm, MPI_Status *status);

/****************************************************************************************************************************************
*
*    'MPI_Put'
*
****************************************************************************************************************************************/
//int MPI_Put (void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
int MPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
            int target_rank, MPI_Aint target_disp, int target_count,
            MPI_Datatype target_datatype, MPI_Win win) ;

/****************************************************************************************************************************************
*
*    'MPI_Recv'
*
****************************************************************************************************************************************/
//int MPI_Recv (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status) ;

/****************************************************************************************************************************************
*
*    'MPI_Reduce'
*
****************************************************************************************************************************************/
//int MPI_Reduce (void *sendbuf,  void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, int root, MPI_Comm comm)
               ;

/****************************************************************************************************************************************
*
*    'MPI_Reduce_scatter'
*
****************************************************************************************************************************************/
//int MPI_Reduce_scatter (void *sendbuf, void *recvbuf, int *recvcnts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
                       ;

/****************************************************************************************************************************************
*
*    'MPI_Recv_init'
*
****************************************************************************************************************************************/
//int MPI_Recv_init (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                  MPI_Comm comm, MPI_Request *request) ;

/****************************************************************************************************************************************
*
*    'MPI_Rsend'
*
****************************************************************************************************************************************/
//int MPI_Rsend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) ;

/****************************************************************************************************************************************
*
*    'MPI_Rsend_init'
*
****************************************************************************************************************************************/
//int MPI_Rsend_init (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request) ;

/****************************************************************************************************************************************
*
*    'MPI_Scan'
*
****************************************************************************************************************************************/
//int MPI_Scan (void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
             MPI_Comm comm)
             ;

/****************************************************************************************************************************************
*
*    'MPI_Scatter'
*
****************************************************************************************************************************************/
//int MPI_Scatter (void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
                ;

/****************************************************************************************************************************************
*
*    'MPI_Scatterv'
*
****************************************************************************************************************************************/
//int MPI_Scatterv (void *sendbuf, int *sendcnts, int *displs, MPI_Datatype sendtype, void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int root, MPI_Comm comm)
                 ;

/****************************************************************************************************************************************
*
*    'MPI_Send'
*
****************************************************************************************************************************************/
//int MPI_Send (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm) ;

/****************************************************************************************************************************************
*
*    'MPI_Send_init'
*
****************************************************************************************************************************************/
//int MPI_Send_init (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                  MPI_Comm comm, MPI_Request *request) ;

/****************************************************************************************************************************************
*
*    'MPI_Sendrecv'
*
****************************************************************************************************************************************/
//int MPI_Sendrecv (void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest,
                 int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int source, int recvtag, MPI_Comm comm, MPI_Status *status)
                 ;

/****************************************************************************************************************************************
*
*    'MPI_Sendrecv_replace'
*
****************************************************************************************************************************************/
//int MPI_Sendrecv_replace (void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest,
                         int sendtag, int source, int recvtag, MPI_Comm comm,
                         MPI_Status *status) ;

/****************************************************************************************************************************************
*
*    'MPI_Ssend'
*
****************************************************************************************************************************************/
//int MPI_Ssend (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) ;

/****************************************************************************************************************************************
*
*    'MPI_Ssend_init'
*
****************************************************************************************************************************************/
//int MPI_Ssend_init (void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request) ;

/****************************************************************************************************************************************
*
*    'MPI_Wait'
*
****************************************************************************************************************************************/
int MPI_Wait (MPI_Request *request, MPI_Status *status);

/****************************************************************************************************************************************
*
*    'MPI_Waitall'
*
****************************************************************************************************************************************/
int MPI_Waitall (int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]);

#endif
