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
 *    File:       rdata.c                                                                                                                    *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

/* include */
#include <empi.h>



/* headers */

/****************************************************************************************************************************************
*
*    'EMPI_Rdata_sparse'
*
****************************************************************************************************************************************/
static void EMPI_Rdata_sparse (EMPI_Data_type *data, MPI_Datatype datatype, int scount, int sdispl, int rcount, int rdispl);

/****************************************************************************************************************************************
*
*    'EMPI_Rdata_dense_disjoint'
*
****************************************************************************************************************************************/
static void EMPI_Rdata_dense_disjoint (EMPI_Data_type *data, int scount, int sdispl, int rcount, int rdispl);

/****************************************************************************************************************************************
*
*    'EMPI_Rdata_dense_shared'
*
****************************************************************************************************************************************/
static void EMPI_Rdata_dense_shared (EMPI_Data_type *data, int scount, int sdispl, int rcount, int rdispl);

/* implementation */

/****************************************************************************************************************************************
*
*    'EMPI_DAlltoallv'
*
****************************************************************************************************************************************/
void EMPI_DAlltoallv (void *sendbuf, int sendcnt, int sdispl, MPI_Datatype sendtype, void *recvbuf, int recvcnt, int rdispl, MPI_Datatype recvtype, MPI_Comm comm) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_DAlltoallv in <%s> ***\n", __FILE__);
    #endif

    //NOTE: sendcnt y sdispl se dan en numero de elementos (filas * dim)

    int rank, size, nrank;

    int *scounts = NULL, *sdispls = NULL, *rcounts = NULL, *rdispls = NULL;

    int sdata[4], *rdata;

    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &size);

    rdata = (int*) malloc (4 * size * sizeof (int));
    assert (rdata);

    sdata[0] = sendcnt;
    sdata[1] = sdispl;
    sdata[2] = recvcnt;
    sdata[3] = rdispl;

    //MPI_Allgather
    PMPI_Allgather (sdata, 4, MPI_INT, rdata, 4, MPI_INT, comm);

    scounts = (int*) calloc (size, sizeof(int));
    sdispls = (int*) calloc (size, sizeof(int));
    rcounts = (int*) calloc (size, sizeof(int));
    rdispls = (int*) calloc (size, sizeof(int));
    

    for (nrank = 0; nrank < size; nrank ++) {

        //Send data
        //MIN (rdispl + rcount, sdispl + scount) - MAX (rdispl, sdispl)
        scounts[nrank] = EMPI_MACRO_MIN (rdata[(nrank*4)+3] + rdata[(nrank*4)+2], rdata[(rank*4)+1] + rdata[(rank*4)])
                       - EMPI_MACRO_MAX (rdata[(nrank*4)+3], rdata[(rank*4)+1]);
        sdispls[nrank] = EMPI_MACRO_MAX (rdata[(nrank*4)+3], rdata[(rank*4)+1]) - sdispl;

        if (scounts[nrank] < 0) { scounts[nrank] = 0; sdispls[nrank] = 0; }

        //Recv data
        //MIN (rdispl + rcount, sdispl + scount) - MAX (rdispl, sdispl)
        rcounts[nrank] = EMPI_MACRO_MIN (rdata[(rank*4)+3] + rdata[(rank*4)+2], rdata[(nrank*4)+1] + rdata[(nrank*4)])
                       - EMPI_MACRO_MAX (rdata[(rank*4)+3], rdata[(nrank*4)+1]);
        rdispls[nrank] = EMPI_MACRO_MAX (rdata[(rank*4)+3], rdata[(nrank*4)+1]) - rdispl;

        if (rcounts[nrank] < 0) { rcounts[nrank] = 0; rdispls[nrank] = 0; }
    }


       
    PMPI_Alltoallv (sendbuf, scounts, sdispls, sendtype, recvbuf, rcounts, rdispls, recvtype, comm);

    //release memory
    free (scounts);
    free (sdispls);
    free (rcounts);
    free (rdispls);

    free (rdata);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_DAlltoallv in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_SAlltoallv'
*
****************************************************************************************************************************************/
void EMPI_SAlltoallv (int *sendrow, int *sendcol, void *sendval, int sendcnt, int sdispl, int **recvrow, int **recvcol, void **recvval, int recvcnt, int rdispl, MPI_Datatype datatype, MPI_Comm comm) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_SAlltoallv in <%s> ***\n", __FILE__);
    #endif

    //NOTE: sendcnt y sdispl se dan en filas/columnas

    int n, rank, size, nrcount = 0, nrdispl = 0, nscount = 0, nsdispl = 0;

    int *nnz, *nnzrecv;

    int status;

    int fcount, fdisp, *vcounts = NULL, *displs = NULL;

    int btype;

    MPI_Type_size (datatype, &btype);

    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &size);

    EMPI_Get_status (&status);

    //new processes
    if (sendcnt < 0) sendcnt = sdispl = 0;

    nnz = (int*) calloc (sendcnt, sizeof(int));
    assert (nnz);
    nnzrecv = (int*) calloc (EMPI_GLOBAL_Data->size, sizeof(int));
    assert (nnzrecv);

    vcounts = (int*) calloc (size, sizeof (int));
    assert (vcounts);
    displs = (int*) calloc (size, sizeof (int));
    assert (displs);

    EMPI_Get_wsize (rank, size, EMPI_GLOBAL_Data->size, &fdisp, &fcount, vcounts, displs);

    //set nnz array
    for (n = 0; n < sendcnt; n ++) nnz[n] = (sendrow[n+1] - sendrow[n]);

    //gather nnz array
    EMPI_DAlltoallv (nnz, sendcnt, sdispl, MPI_INT, nnzrecv, EMPI_GLOBAL_Data->size, 0, MPI_INT, EMPI_COMM_WORLD);

    //memory free
    free (nnz);
    free (vcounts);
    free (displs);

    if (status == EMPI_ACTIVE) {

        *recvrow = (int*) calloc (recvcnt + 1, sizeof (int));
        assert (*recvrow);

        //recvcol and recvval sizes
        for (n = 0, nrcount = 0; n < recvcnt; n ++) {

            nrcount += nnzrecv[n+rdispl];
            (*recvrow)[n+1] = ((*recvrow)[n] + nnzrecv[n+rdispl]);
        }

        for (n = 0, nrdispl = 0; n < rdispl; n ++) {

            nrdispl += nnzrecv[n];
        }

        //memory allocation
        *recvcol = (int*) malloc (nrcount * sizeof(int));
        assert (*recvcol);

        *recvval = malloc (nrcount * btype);
        assert (*recvval);
    }

    //nscount and nsdispl
    for (n = 0, nscount = 0; n < sendcnt; n ++) {

        nscount += nnzrecv[n+sdispl];
    }

    for (n = 0, nsdispl = 0; n < sdispl; n ++) {

        nsdispl += nnzrecv[n];
    }

    free (nnzrecv);

    //DAlltoallv for recvcol
    EMPI_DAlltoallv (sendcol, nscount, nsdispl, MPI_INT, *recvcol, nrcount, nrdispl, MPI_INT, comm);

    //DAlltoallv for recvval
    EMPI_DAlltoallv (sendval, nscount, nsdispl, datatype, *recvval, nrcount, nrdispl, datatype, comm);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_SAlltoallv in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Rdata_spawn_cost'
*
****************************************************************************************************************************************/
void EMPI_Rdata_spawn_cost (int size, int nprocs, double *cost) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Rdata_spawn_cost in <%s> ***\n", __FILE__);
    #endif

    EMPI_Data_type *data = NULL;

    data = EMPI_GLOBAL_Data;

    unsigned long long int datasize;

    int btype, nsteps, nmess, int_btype;

    long double datasent;

    *cost = 0;

    while (data != NULL) {

        /* CHANGE: begin */
        if (data->stype != EMPI_NULL) {
        /* CHANGE: end */
            datasize = 0;
            nsteps = 0;
            nmess = 0;

            MPI_Type_size (data->datatype, &btype);
            MPI_Type_size (MPI_INT, &int_btype);

            nsteps = ceil(log2(size + nprocs)); //se intercambian datos todos con todos
            nmess = ceil(log2(size + nprocs)); //numero de mensajes simultaneos en un paso
       /* CHANGE: begin */
       }
       /* CHANGE: end */
        
       if ((data->stype == EMPI_DENSE)||(data->stype == EMPI_VECTOR)) {

            //datasize is the number of non-zero elements multiplied by the data type and the number of bits
            datasize = (data->nnz * btype * 8);

            datasent = (datasize / (size+nprocs)) * nprocs;

            *cost += (nsteps * (EMPI_GLOBAL_alpha + ((datasent/nmess) * EMPI_GLOBAL_beta))); //* comm_factor

        } else if (data->stype == EMPI_SPARSE) {

            //ALLTOALLV ARRAY ROW

            datasize = (data->size * int_btype * 8);

            datasent = (datasize / (size+nprocs)) * nprocs;

            *cost += (nsteps * (EMPI_GLOBAL_alpha + ((datasent/nmess) * EMPI_GLOBAL_beta))); //* comm_factor

            //ALLTOALLV ARRAY COL

            //datasize is the number of non-zero elements multiplied by the data type and the number of bits
            datasize = (data->nnz * int_btype * 8);

            datasent = (datasize / (size+nprocs)) * nprocs;

            *cost += (nsteps * (EMPI_GLOBAL_alpha + ((datasent/nmess) * EMPI_GLOBAL_beta))); //* comm_factor

            //ALLTOALLV ARRAY VAL

            //datasize is the number of non-zero elements multiplied by the data type and the number of bits
            datasize = (data->nnz * btype * 8);

            datasent = (datasize / (size+nprocs)) * nprocs;

            *cost += (nsteps * (EMPI_GLOBAL_alpha + ((datasent/nmess) * EMPI_GLOBAL_beta))); //* comm_factor

        }

        //next data structure
        data = data->next;
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Rdata_spawn_cost in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Rdata_remove_cost'
*
****************************************************************************************************************************************/
void EMPI_Rdata_remove_cost (int size, int nprocs, double *cost) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Rdata_remove_cost in <%s> ***\n", __FILE__);
    #endif

    EMPI_Data_type *data = NULL;

    data = EMPI_GLOBAL_Data;

    unsigned long long int datasize;

    int btype, nsteps, nmess, int_btype;

    long double datasent;

    *cost = 0;

    while (data != NULL) {

        /* CHANGE: begin */
        if (data->stype != EMPI_NULL) {
        /* CHANGE: end */
            datasize = 0;
            nsteps = 0;
            nmess = 0;

            MPI_Type_size (data->datatype, &btype);
            MPI_Type_size (MPI_INT, &int_btype);

            nsteps = ceil(log2(size + nprocs)); //se intercambian datos todos con todos
            nmess = ceil(log2(size + nprocs)); //numero de mensajes simultaneos en un paso
        /* CHANGE: begin */
        }
        /* CHANGE: end */

        if ((data->stype == EMPI_DENSE)||(data->stype == EMPI_VECTOR)) {

            //datasize is the number of non-zero elements multiplied by the data type and the number of bits
            datasize = (data->nnz * btype * 8);

            datasent = (datasize / size) * nprocs;

            *cost += (nsteps * (EMPI_GLOBAL_alpha + ((datasent/nmess) * EMPI_GLOBAL_beta))); //* comm_factor

        } else if (data->stype == EMPI_SPARSE) {

            //ALLTOALLV ARRAY ROW

            datasize = (data->size * int_btype * 8);

            datasent = (datasize / size) * nprocs;

            *cost += (nsteps * (EMPI_GLOBAL_alpha + ((datasent/nmess) * EMPI_GLOBAL_beta))); //* comm_factor

            //ALLTOALLV ARRAY COL

            //datasize is the number of non-zero elements multiplied by the data type and the number of bits
            datasize = (data->nnz * int_btype * 8);

            datasent = (datasize / size) * nprocs;

            *cost += (nsteps * (EMPI_GLOBAL_alpha + ((datasent/nmess) * EMPI_GLOBAL_beta))); //* comm_factor

            //ALLTOALLV ARRAY VAL

            //datasize is the number of non-zero elements multiplied by the data type and the number of bits
            datasize = (data->nnz * btype * 8);

            datasent = (datasize / size) * nprocs;

            *cost += (nsteps * (EMPI_GLOBAL_alpha + ((datasent/nmess) * EMPI_GLOBAL_beta))); //* comm_factor

        }

        //next data structure
        data = data->next;
    }
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Rdata_remove_cost in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Rdata'
*
****************************************************************************************************************************************/
void EMPI_Rdata (int scount, int sdispl, int rcount, int rdispl) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Rdata in <%s> ***\n", __FILE__);
    #endif

    EMPI_Data_type *data = NULL;

    data = EMPI_GLOBAL_Data;

    while (data != NULL) {

        switch (data->stype) {

            case EMPI_SPARSE:

                //redistribute sparse matrix
                EMPI_Rdata_sparse (data, data->datatype, scount, sdispl, rcount, rdispl);

                break;

            case EMPI_VECTOR:
            case EMPI_DENSE:

                if (data->mapping == EMPI_DISJOINT)
                    //redistribute disjoing dense matrix & vector
                    EMPI_Rdata_dense_disjoint (data, scount, sdispl, rcount, rdispl);

                else if (data->mapping == EMPI_SHARED)
                    //redistribute shared dense matrix & vector
                    EMPI_Rdata_dense_shared (data, scount, sdispl, rcount, rdispl);

                break;
        }

        //next data structure
        data = data->next;

    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Rdata in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Rdata_sparse'
*
****************************************************************************************************************************************/
static void EMPI_Rdata_sparse (EMPI_Data_type *data, MPI_Datatype datatype, int scount, int sdispl, int rcount, int rdispl) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Rdata_sparse in <%s> ***\n", __FILE__);
    #endif

    //recv buffers
    int *recvbuf_row = NULL;
    int *recvbuf_col = NULL;
    void *recvbuf_val = NULL;

    //send buffers
    int *sendbuf_row = NULL;
    int *sendbuf_col = NULL;
    void *sendbuf_val = NULL;

    //get addresses
    EMPI_Get_addr_sparse (data->id, (void*)&sendbuf_row, (void*)&sendbuf_col, (void*)&sendbuf_val);

    //SAlltoallv sparse data structure
    EMPI_SAlltoallv (sendbuf_row, sendbuf_col, sendbuf_val, scount, sdispl, &recvbuf_row, &recvbuf_col, &recvbuf_val, rcount, rdispl, data->datatype, EMPI_COMM_WORLD);

    //register sparse structure
    EMPI_Register_sparse (data->id, recvbuf_row, recvbuf_col, recvbuf_val, data->datatype, data->size, data->nnz);

    free (sendbuf_row);
    free (sendbuf_col);
    free (sendbuf_val);

    sendbuf_row = sendbuf_col = recvbuf_row = recvbuf_col = NULL;

    sendbuf_val = recvbuf_val = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Rdata_sparse in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Rdata_dense_disjoint'
*
****************************************************************************************************************************************/
static void EMPI_Rdata_dense_disjoint (EMPI_Data_type *data, int scount, int sdispl, int rcount, int rdispl) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Rdata_dense_disjoint in <%s> ***\n", __FILE__);
    #endif

    //send and recv buffers
    void *sendbuf = NULL;
    void *recvbuf = NULL;

    int btype;

    // Datatype for large matrices
    MPI_Datatype MPI_BLOCK;

    MPI_Type_size (data->datatype, &btype);

    sendbuf = EMPI_Get_addr(data->id);

    if (rcount > 0) {

        recvbuf = malloc (rcount * data->dim * btype);
        assert (recvbuf);
    }

    //Alltoallv dense data structure
     if(data->dim<20000){
        EMPI_DAlltoallv (sendbuf, scount*data->dim, sdispl*data->dim, data->datatype, recvbuf, rcount*data->dim, rdispl*data->dim, data->datatype, EMPI_COMM_WORLD);
    }
    else{

        fflush(stderr);
        MPI_Type_contiguous(data->dim, data->datatype, &MPI_BLOCK);
        MPI_Type_commit(&MPI_BLOCK);
        EMPI_DAlltoallv (sendbuf, scount, sdispl, MPI_BLOCK, recvbuf, rcount, rdispl, MPI_BLOCK, EMPI_COMM_WORLD);
        MPI_Type_free(&MPI_BLOCK);
        fflush(stderr);

    }

    if (data->stype == EMPI_DENSE)
        //register new shared data structure
        EMPI_Register_dense (data->id, recvbuf, data->datatype, data->size, data->mapping);

    else if (data->stype == EMPI_VECTOR)
        //register new shared data structure
        EMPI_Register_vector (data->id, recvbuf, data->datatype, data->size, data->mapping);

    free (sendbuf);

    sendbuf = NULL;

    recvbuf = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Rdata_dense_disjoint in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Rdata_dense_shared'
*
****************************************************************************************************************************************/
static void EMPI_Rdata_dense_shared (EMPI_Data_type *data, int scount, int sdispl, int rcount, int rdispl) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Rdata_dense_shared in <%s> ***\n", __FILE__);
    #endif

    //send and recv buffers
    void *sendbuf = NULL;
    void *recvbuf = NULL;

    int btype;

    MPI_Type_size (data->datatype, &btype);

    sendbuf = EMPI_Get_addr(data->id);

    recvbuf = malloc (data->size * btype);
    assert (recvbuf);

    //DAlltoallv dense data structure
    EMPI_DAlltoallv (sendbuf, scount*data->dim, sdispl*data->dim, data->datatype, recvbuf, data->size*data->dim, 0, data->datatype, EMPI_COMM_WORLD);

    if (data->stype == EMPI_DENSE)
        //register new shared data structure
        EMPI_Register_dense (data->id, recvbuf, data->datatype, data->size, data->mapping);

    else if (data->stype == EMPI_VECTOR)
        //register new shared data structure
        EMPI_Register_vector (data->id, recvbuf, data->datatype, data->size, data->mapping);

    free (sendbuf);

    sendbuf = NULL;

    recvbuf = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Rdata_dense_shared in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Register_size'
*
****************************************************************************************************************************************/
void EMPI_Register_size (int size) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Register_size in <%s> ***\n", __FILE__);
    #endif

    EMPI_Data_type *data = NULL;

    EMPI_GLOBAL_Data = (EMPI_Data_type *) malloc (sizeof (EMPI_Data_type));
    assert (EMPI_GLOBAL_Data);

    data = EMPI_GLOBAL_Data;

    //set data->next
    data->next = NULL;

    //register data size
    data->size = size;
    /* CHANGE: begin */
    data->id = NULL;
    /* CHANGE: end */
    data->stype = EMPI_NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Register_size in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Register_sparse'
*
****************************************************************************************************************************************/
void EMPI_Register_sparse (char *id, void *addr_row, void *addr_col, void *addr_val, MPI_Datatype datatype, int size, int nnz) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Register_sparse in <%s> ***\n", __FILE__);
    #endif

    EMPI_Data_type *data = NULL;

    int len;

    if (EMPI_GLOBAL_Data == NULL) {

        EMPI_GLOBAL_Data = (EMPI_Data_type *) malloc (sizeof (EMPI_Data_type));
        assert (EMPI_GLOBAL_Data);

        data = EMPI_GLOBAL_Data;

        //set data->next
        data->next = NULL;

    } else {

        data = EMPI_GLOBAL_Data;

        /* CHANGE: begin */
        //while ((data->next != NULL)&&(strcmp(data->id, id)!=0)) data = data->next;
        while ((data->next != NULL)&&(data->id!=NULL)&&(strcmp(data->id, id)!=0)) data = data->next;
        /* CHANGE: end */

        /* CHANGE: begin */
        //if ((data->next == NULL)&&(strcmp(data->id, id)!=0)) {
        if ((data->next == NULL)&&(data->id!=NULL)&&(strcmp(data->id, id)!=0)) {
        /* CHANGE: end */

            data->next = (EMPI_Data_type *) malloc (sizeof (EMPI_Data_type));
            assert (data->next);

            data = data->next;

            //set data->next
            data->next = NULL;
        }
    }

    //register sparse data structure
    len = strlen (id);
    data->id = (char*) malloc ((len+1) * sizeof (id));
    strcpy (data->id, id);
    data->addr_row = addr_row;
    data->addr_col = addr_col;
    data->addr_val = addr_val;
    data->datatype = datatype;
    data->size = size;
    data->dim = size;
    data->nnz = nnz;
    data->stype = EMPI_SPARSE;
    data->mapping = EMPI_DISJOINT;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Register_sparse in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Register_vector'
*
****************************************************************************************************************************************/
void EMPI_Register_vector (char *id, void *addr, MPI_Datatype datatype, int size, int mapping) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Register_vector in <%s> ***\n", __FILE__);
    #endif

    EMPI_Data_type *data = NULL;

    int len;

    if (EMPI_GLOBAL_Data == NULL) {

        EMPI_GLOBAL_Data = (EMPI_Data_type *) malloc (sizeof (EMPI_Data_type));
        assert (EMPI_GLOBAL_Data);

        data = EMPI_GLOBAL_Data;

        //set data->next
        data->next = NULL;

    } else {

        data = EMPI_GLOBAL_Data;

        /* CHANGE: begin */
        //while ((data->next != NULL)&&(strcmp(data->id, id)!=0)) data = data->next;
        while ((data->next != NULL)&&(data->id!=NULL)&&(strcmp(data->id, id)!=0)) data = data->next;
        /* CHANGE: end */

        /* CHANGE: begin */
        //if ((data->next == NULL)&&(strcmp(data->id, id)!=0)) {
        if ((data->next == NULL)&&(data->id!=NULL)&&(strcmp(data->id, id)!=0)) {
        /* CHANGE: end */
            
            data->next = (EMPI_Data_type *) malloc (sizeof (EMPI_Data_type));
            assert (data->next);

            data = data->next;

            //set data->next
            data->next = NULL;
        }
    }

    //register vector data structure
    len = strlen (id);
    data->id = (char*) malloc ((len+1) * sizeof (id));
    strcpy (data->id, id);
    data->addr = addr;
    data->datatype = datatype;
    data->size = size;
    data->dim = 1;
    data->nnz = size;
    data->stype = EMPI_VECTOR;
    data->mapping = mapping;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Register_vector in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Register_dense'
*
****************************************************************************************************************************************/
void EMPI_Register_dense (char *id, void *addr, MPI_Datatype datatype, int size, int mapping) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Register_dense in <%s> ***\n", __FILE__);
    #endif

    EMPI_Data_type *data = NULL;

    if (EMPI_GLOBAL_Data == NULL) {

        //allocate new data structure register
        EMPI_GLOBAL_Data = (EMPI_Data_type *) malloc (sizeof (EMPI_Data_type));
        assert (EMPI_GLOBAL_Data);

        data = EMPI_GLOBAL_Data;

        //set data->next
        data->next = NULL;

    } else {

        data = EMPI_GLOBAL_Data;

        /* CHANGE: begin */
        //while ((data->next != NULL)&&(strcmp(data->id, id)!=0)) data = data->next;
        while ((data->next != NULL)&&(data->id!=NULL)&&(strcmp(data->id, id)!=0)) data = data->next;
        /* CHANGE: end */

        /* CHANGE: begin */
        //if ((data->next == NULL)&&(strcmp(data->id, id)!=0)) {
        if ((data->next == NULL)&&(data->id!=NULL)&&(strcmp(data->id, id)!=0)) {
        /* CHANGE: end */

            data->next = (EMPI_Data_type *) malloc (sizeof (EMPI_Data_type));
            assert (data->next);

            data = data->next;

            //set data->next
            data->next = NULL;
        }
    }

    int len;

    //register dense data structure
    len = strlen (id);
    data->id = (char*) malloc ((len+1) * sizeof (id));
    strcpy (data->id, id);
    data->addr = addr;
    data->datatype = datatype;
    data->size = size;
    data->dim = size;
    data->nnz = ((unsigned long long int)size) * ((unsigned long long int)size);
    data->stype = EMPI_DENSE;
    data->mapping = mapping;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Register_dense in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Deregister_shared'
*
****************************************************************************************************************************************/
void EMPI_Deregister_shared (char *id) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Deregister_shared in <%s> ***\n", __FILE__);
    #endif

    //FIXME: se puede optimizar eliminando la estructura de la lista.

    EMPI_Data_type *data = NULL;

    data = EMPI_GLOBAL_Data;

    while (data != NULL) {

        /* CHANGE: begin */
        //if (strcmp(data->id, id)==0) {
        if ((data->id!=NULL)&&(strcmp(data->id, id)==0)) {
        /* CHANGE: end */

            data->id = NULL;

            switch (data->stype) {

                case EMPI_SPARSE:

                    //deregister sparse data structure
                    data->addr_row = NULL;
                    data->addr_col = NULL;
                    data->addr_val = NULL;

                    break;

                case EMPI_VECTOR:

                case EMPI_DENSE:

                    //deregister vector or dense data structure
                    data->addr = NULL;

                    break;
            }

            data->stype = EMPI_NULL;
        }

        data = data->next;
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Deregister_shared in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Get_addr'
*
****************************************************************************************************************************************/
void *EMPI_Get_addr (char *id) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Get_addr in <%s> ***\n", __FILE__);
    #endif

    EMPI_Data_type *data = NULL;

    data = EMPI_GLOBAL_Data;

    while (data != NULL) {

        /* CHANGE: begin */
        //if (strcmp(data->id, id)==0) {
        if ((data->id!=NULL)&&(strcmp(data->id, id)==0)) {
        /* CHANGE: end */
            return data->addr;
        }

        data = data->next;
    }
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Get_addr in <%s> ***\n", __FILE__);
    #endif

    return NULL;
}

/****************************************************************************************************************************************
*
*    'EMPI_Get_addr_sparse'
*
****************************************************************************************************************************************/
void EMPI_Get_addr_sparse (char *id, void **addr_row, void **addr_col, void **addr_val) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Get_addr_sparse in <%s> ***\n", __FILE__);
    #endif

    EMPI_Data_type *data = NULL;

    data = EMPI_GLOBAL_Data;

    while (data != NULL) {

        /* CHANGE: begin */
        //if (strcmp(data->id, id)==0) {
        if ((data->id!=NULL)&&(strcmp(data->id, id)==0)) {
        /* CHANGE: end */

            //return pointer addresses
            *addr_row = data->addr_row;
            *addr_col = data->addr_col;
            *addr_val = data->addr_val;
        }

        data = data->next;
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Get_addr_sparse in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Get_shared'
*
****************************************************************************************************************************************/
void EMPI_Get_shared (int *it) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Get_shared in <%s> ***\n", __FILE__);
    #endif

    int type;

    // Datatype for large matrices
    MPI_Datatype MPI_BLOCK;

    //Get process type
    EMPI_Get_type (&type);

    if (type == EMPI_SPAWNED) {

        int rank, size, displ, count;

        int *recvbuf_row = NULL;
        int *recvbuf_col = NULL;
        void *recvbuf = NULL;

        //Get rank and size
        MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
        MPI_Comm_size (EMPI_COMM_WORLD, &size);

        EMPI_Data_type *data = NULL;
        
        
        
        data = EMPI_GLOBAL_Data;

        //get size
        EMPI_Get_wsize (rank, size, data->size, &displ, &count, NULL, NULL);

        while (data != NULL) {

            switch (data->stype) {

                case EMPI_SPARSE:

                    //SAlltoallv data structure (sparse matrix)
                    EMPI_SAlltoallv (NULL, NULL, NULL, -1, -1, &recvbuf_row, &recvbuf_col, (void*)&recvbuf, count, displ, data->datatype, EMPI_COMM_WORLD);

                    EMPI_Register_sparse (data->id, recvbuf_row, recvbuf_col, recvbuf, data->datatype, data->size, data->nnz);

                    break;

                case EMPI_VECTOR:
                case EMPI_DENSE:

                    recvbuf = EMPI_Get_addr(data->id);

                    if (data->mapping == EMPI_DISJOINT){
                        fflush(stderr);
                        //Alltoallv shared data (dense matrix or vector)
                        
                        if(data->dim<20000){
                            EMPI_DAlltoallv (NULL, -1, -1, data->datatype, recvbuf, count*data->dim, displ*data->dim, data->datatype, EMPI_COMM_WORLD);
                        }
                        else{

                            MPI_Type_contiguous(data->dim, data->datatype, &MPI_BLOCK);
                            int err;
                            err=MPI_Type_commit(&MPI_BLOCK);;
                            EMPI_DAlltoallv (NULL, -1, -1, MPI_BLOCK, recvbuf, count, displ, MPI_BLOCK, EMPI_COMM_WORLD);   

                            MPI_Type_free(&MPI_BLOCK);
                            fflush(stderr);
                       
                        }
                    }
                    else if (data->mapping == EMPI_SHARED)

                        //Alltoallv dense data structure
                        EMPI_DAlltoallv (NULL, -1, -1, data->datatype, recvbuf, data->size*data->dim, 0, data->datatype, EMPI_COMM_WORLD);

                    break;
            }

            data = data->next;
        }
              
        
    }

    //Get iteration number
    *it = EMPI_GLOBAL_iteration;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Get_shared in <%s> ***\n", __FILE__);
    #endif
}


/****************************************************************************************************************************************
*
*    'EMPI_alloc'
*
****************************************************************************************************************************************/

void EMPI_alloc()
{
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG:enter:EMPI_alloc in <%s> ***\n", __FILE__);
    #endif    
    
    if (EMPI_array_alloc == 0 ){
        EMPI_array_alloc = 1;
        return;
    }
    if (EMPI_array_alloc == 1 ){
        EMPI_array_alloc = 2;
    }
    
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG:exit:EMPI_alloc in <%s> ***\n", __FILE__);
    #endif    
    
    return;
}

/****************************************************************************************************************************************
*
*    'EMPI_status_alloc'
*
****************************************************************************************************************************************/

int EMPI_status_alloc()
{   
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG:enter:EMPI_status_alloc in <%s> ***\n", __FILE__);
    #endif    
    
     //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG:exit:EMPI_status_alloc in <%s> ***\n", __FILE__);
    #endif     
    return(EMPI_array_alloc);

}



