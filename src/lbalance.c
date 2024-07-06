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
 *    File:       load_balance.c                                                                                                            *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

/* include */
#include <empi.h>

/* headers */

/****************************************************************************************************************************************
*
*    'EMPI_evaluate_external_load'
*
****************************************************************************************************************************************/
static int EMPI_evaluate_external_load (int *hmon, int K);

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_spawn_dense'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_spawn_dense (int rank, int size, int newsize, int* newcount, int* vcounts, int* displs, int* newdispl, int *stflops);

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_spawn_sparse'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_spawn_sparse (int rank, int size, int newsize, int* newcount, int* vcounts, int* displs, int* newdispl, EMPI_Monitor_type *smonitor, int *stflops);

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_spawn_fcost'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_spawn_fcost (int rank, int size, int newsize, int* newcount, int* vcounts, int* displs, int* newdispl, EMPI_Monitor_type *smonitor, int *stflops, int *fc);

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_remove_dense'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_remove_dense (int rank, int size, int newsize, int* newcount, int* vcounts, int* displs, int* newdispl, EMPI_Monitor_type *smonitor, int* rremvs);

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_remove_sparse'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_remove_sparse (int rank, int size, int newsize, int* newcount, int* vcounts, int* displs, int* newdispl, EMPI_Monitor_type *smonitor, int* rremvs);

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_remove_fcost'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_remove_fcost (int rank, int size, int newsize, int* newcount, int* vcounts, int* displs, int* newdispl, EMPI_Monitor_type *smonitor, int* rremvs, int *fc);

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_dense'
*
****************************************************************************************************************************************/
static int EMPI_LBalance_dense (int *rank, int *size, int count, int disp, EMPI_Monitor_type *smonitor, int BTtype);

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_sparse'
*
****************************************************************************************************************************************/
static int EMPI_LBalance_sparse (int *rank, int *size, int count, int disp, EMPI_Monitor_type *smonitor, int BTtype);

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_fcost'
*
****************************************************************************************************************************************/
static int EMPI_LBalance_fcost (int *rank, int *size, int count, int disp, int *fc, EMPI_Monitor_type *smonitor, int BTtype);

/* implementation */

/****************************************************************************************************************************************
*
*    'EMPI_Set_lbpolicy'
*
****************************************************************************************************************************************/
void EMPI_Set_lbpolicy (int lbpolicy) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Set_lbpolicy in <%s> ***\n", __FILE__);
    #endif

    if ((lbpolicy != EMPI_LBMFLOPS)&&(lbpolicy != EMPI_LBCOUNTS)&&(lbpolicy != EMPI_LBSTATIC)) {

        fprintf (stderr, "Invalid load balancing policy (set to EMPI_LBMFLOPS, EMPI_LBCOUNTS or EMPI_LBSTATIC)\n");

        MPI_Abort (EMPI_COMM_WORLD, -1);
    }

    //Set lbpolicy
    EMPI_GLOBAL_lbpolicy = lbpolicy;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Set_lbpolicy in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Disable_lbalance'
*
****************************************************************************************************************************************/
void EMPI_Disable_lbalance (void) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Disable_lbalance in <%s> ***\n", __FILE__);
    #endif

    EMPI_GLOBAL_lbalance = EMPI_FALSE;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Disable_lbalance in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Enable_lbalance'
*
****************************************************************************************************************************************/
void EMPI_Enable_lbalance (void) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Enable_lbalance in <%s> ***\n", __FILE__);
    #endif

    EMPI_GLOBAL_lbalance = EMPI_TRUE;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Enable_lbalance in <%s> ***\n", __FILE__);
    #endif
}


/****************************************************************************************************************************************
*
*    'EMPI_evaluate_LB'
*
****************************************************************************************************************************************/
int EMPI_evaluate_LB (int size, EMPI_Monitor_type *smonitor, int BTtype) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_evaluate_LB in <%s> ***\n", __FILE__);
    #endif

    int n, lb = EMPI_NULL;

    double xthreshold;

    long long diff_min, diff_max;

    if (BTtype == EMPI_TCPU) {

        //Tcpu

        //initialize diff max and min
        diff_max = diff_min = smonitor[0].ptime;

        //evaluate load imbalance
        for (n = 0; n < size; n ++) {

            //cpu imbalance
            if (smonitor[n].ptime < diff_min) diff_min = smonitor[n].ptime;

            else if (smonitor[n].ptime > diff_max) diff_max = smonitor[n].ptime;
        }

        xthreshold = (double)(diff_max * EMPI_GLOBAL_threshold);

        //load balance: balanced OR unbalanced
        if ((diff_max - diff_min) > xthreshold) lb = EMPI_UNBALANCED;

        else lb = EMPI_BALANCED;

    } else if (BTtype == EMPI_TREAL) {

        //Treal

        //initialize diff max and min
        diff_max = diff_min = smonitor[0].rtime;

        //evaluate load imbalance
        for (n = 0; n < size; n ++) {

            //cpu imbalance
            if (smonitor[n].rtime < diff_min) diff_min = smonitor[n].rtime;

            else if (smonitor[n].rtime > diff_max) diff_max = smonitor[n].rtime;
        }

        xthreshold = (double)(diff_max * EMPI_GLOBAL_threshold);

        //load balance: balanced OR unbalanced
        if ((diff_max - diff_min) > xthreshold) lb = EMPI_UNBALANCED;

        else lb = EMPI_BALANCED;
    }

    return lb;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_evaluate_LB in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_evaluate_system_status'
*
****************************************************************************************************************************************/
void EMPI_evaluate_system_status (int size, EMPI_Monitor_type *smonitor, int *sampling_status) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_evaluate_system_status in <%s> ***\n", __FILE__);
    #endif

    int n, pe_status;

    double xload;

    for (n = 0; n < size; n ++) {

        //processor external load
        xload = 1 - ((double)smonitor[n].ptime / (double)smonitor[n].rtime);

        if (xload > EMPI_GLOBAL_Load_threshold)
            pe_status = EMPI_NON_DEDICATED;
        else
            pe_status = EMPI_DEDICATED;

        if (n == 0) *sampling_status = pe_status;

        //system status: dedicated OR non_dedicated
        if ((*sampling_status == EMPI_DEDICATED)&&(pe_status == EMPI_NON_DEDICATED)) *sampling_status = EMPI_NON_DEDICATED;
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_evaluate_system_status in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_evaluate_external_load'
*
****************************************************************************************************************************************/
static int EMPI_evaluate_external_load (int *hmon, int K) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_evaluate_external_load in <%s> ***\n", __FILE__);
    #endif

    int n, xload = EMPI_NULL;

    //discriminar entre dedicado y burst
    for (n = 0; n < K; n ++) {

        if (hmon[n] == EMPI_NON_DEDICATED)

            xload = EMPI_LONG_TERM;
    }

    //discriminar entre burst y no dedicado
    for (n = 0; n < K; n ++) {

        if (hmon[n] == EMPI_DEDICATED)

            xload = EMPI_BURST;
    }

    return xload;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_evaluate_external_load in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_spawn'
*
****************************************************************************************************************************************/
void EMPI_LBalance_spawn (int rank, int size, int newsize, int* newcount, int* newdispl, int* vcounts, int* displs, EMPI_Monitor_type *smonitor, int *stflops, int* fc) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_spawn in <%s> ***\n", __FILE__);
    #endif

    //Load balancing
    /* CHANGE: begin */
    //if (EMPI_GLOBAL_Data->stype == EMPI_SPARSE) {
    if ((EMPI_GLOBAL_Data != NULL) && (EMPI_GLOBAL_Data->stype == EMPI_SPARSE)) {
    /* CHANGE: end */

        if (EMPI_GLOBAL_allocation == EMPI_NNZ)

            //sparse balance
            EMPI_LBalance_spawn_sparse (rank, size, newsize, newcount, newdispl, vcounts, displs, smonitor, stflops);

        else if (EMPI_GLOBAL_allocation == EMPI_FCOST)

            //cost balance
            EMPI_LBalance_spawn_fcost (rank, size, newsize, newcount, newdispl, vcounts, displs, smonitor, stflops, fc);

        else if (EMPI_GLOBAL_allocation == EMPI_ROWS)

            //dense balance
            EMPI_LBalance_spawn_dense (rank, size, newsize, newcount, newdispl, vcounts, displs, stflops);

    /* CHANGE: begin */
    //} else if ((EMPI_GLOBAL_Data->stype == EMPI_DENSE)||(EMPI_GLOBAL_Data->stype == EMPI_VECTOR)) {
    } else if ( (EMPI_GLOBAL_Data != NULL) && ((EMPI_GLOBAL_Data->stype == EMPI_DENSE)||(EMPI_GLOBAL_Data->stype == EMPI_VECTOR)) ) {
    /* CHANGE: end */

        if (EMPI_GLOBAL_allocation == EMPI_ROWS)

            //dense balance
            EMPI_LBalance_spawn_dense (rank, size, newsize, newcount, newdispl, vcounts, displs, stflops);

        else if (EMPI_GLOBAL_allocation == EMPI_FCOST)

            //cost balance
            EMPI_LBalance_spawn_fcost (rank, size, newsize, newcount, newdispl, vcounts, displs, smonitor, stflops, fc);

    } else

        //dense balance
        EMPI_LBalance_spawn_dense (rank, size, newsize, newcount, newdispl, vcounts, displs, stflops);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_LBalance_spawn in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_spawn_dense'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_spawn_dense (int rank, int size, int newsize, int* newcount, int* newdispl, int* vcounts, int* displs, int *stflops) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_spawn_dense in <%s> ***\n", __FILE__);
    #endif

    double *rp = NULL, srp = 0.0;

    int gcount = 0, psum = 0, n;

    //memory allocation
    rp = (double*) calloc (newsize, sizeof (double));
    assert (rp);

    //set gcount, rp and srp
    for (n = 0; n < newsize; n ++) {

        //get mflops
        if (EMPI_GLOBAL_lbpolicy == EMPI_LBSTATIC){         
            rp[n] = 1;         
        }     
        else{         
            rp[n] = stflops[n];         
        }
        srp += rp[n];
    }
  
    
    //set global count
    gcount = EMPI_GLOBAL_Data->size;

    //set rcount and rdispl
    for (n = 0; n < newsize; n ++) {

        vcounts[n] = gcount * (rp[n]/srp);

        psum += vcounts[n];

        if (n > 0) displs[n] = displs[n-1] + vcounts[n-1];
    }
    // Assign the remaider elements
    n = 0;
    while (psum < gcount)
    {
        vcounts[n] += 1;
        psum++;
        n++;
        if(n == newsize) n = 0;
    }
    
    //set rcount and rdispl
    *newcount = vcounts[rank];
    *newdispl = displs[rank];

    //free memory
    free (rp);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_spawn_dense in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_spawn_sparse'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_spawn_sparse (int rank, int size, int newsize, int* newcount, int* newdispl, int* vcounts, int* displs, EMPI_Monitor_type *smonitor, int *stflops) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_spawn_sparse in <%s> ***\n", __FILE__);
    #endif

    double *rp = NULL, srp = 0.0;

    int count, disp, *tvcounts = NULL, *tdispls = NULL, nrows = 0, acumm = 0, nindex = 0;

    int gcount = 0, psum = 0, n, *nnz = NULL, *nnzrecv = NULL;

    int *addr_row = NULL, *addr_col = NULL;

    void *addr_val = NULL;

    //get sparse address
    EMPI_Get_addr_sparse (EMPI_GLOBAL_Data->id, (void*)&addr_row, (void*)&addr_col, (void*)&addr_val);

    //memory allocation
    rp = (double*) calloc (newsize, sizeof (double));
    assert (rp);

    //set rp and srp
    for (n = 0; n < newsize; n ++) {

        //get mflops
        if (EMPI_GLOBAL_lbpolicy == EMPI_LBSTATIC) {
            rp[n] = 1;
        }
        else{
            rp[n] = stflops[n]; 
        }

        if (n < size) gcount += smonitor[n].count;

        srp += rp[n];
    }
    //memory allocation
    tvcounts = (int*) calloc (size, sizeof (int));
    assert (tvcounts);
    tdispls = (int*) calloc (size, sizeof (int));
    assert (tdispls);

    EMPI_Get_wsize (rank, size, EMPI_GLOBAL_Data->size, &disp, &count, tvcounts, tdispls);

    //memory allocation
    nnz = (int*) calloc (count, sizeof(int));
    assert (nnz);
    nnzrecv = (int*) calloc (EMPI_GLOBAL_Data->size, sizeof(int));
    assert (nnzrecv);

    //set nnz array
    for (n = 0; n < count; n ++) nnz[n] = (addr_row[n+1] - addr_row[n]);

    //reduce nnz array
    PMPI_Allgatherv (nnz, count, MPI_INT, nnzrecv, tvcounts, tdispls, MPI_INT, EMPI_COMM_WORLD);

    free (tvcounts);
    free (tdispls);
    free (nnz);

    //set rcount and rdispl
    for (n = 0; n < newsize; n ++) {

        vcounts[n] = gcount * (rp[n]/srp);

        nrows = acumm = 0;

        while ((nindex < EMPI_GLOBAL_Data->size)&&(acumm < vcounts[n])) {

            acumm += nnzrecv[nindex];

            nrows ++;

            nindex ++;
        }

        vcounts[n] = nrows;

        psum += vcounts[n];

        if (n > 0) displs[n] = displs[n-1] + vcounts[n-1];
    }

    // Assign the remaider elements
    n = 0;
    while (psum < EMPI_GLOBAL_Data->size)
    {
        vcounts[n] += 1;
        psum++;
        n++;
        if(n == newsize) n = 0;
    }
    
    //set rcount and rdispl
    *newcount = vcounts[rank];
    *newdispl = displs[rank];

    //free memory
    free (rp);
    free (nnzrecv);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_spawn_sparse in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_spawn_fcost'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_spawn_fcost (int rank, int size, int newsize, int* newcount, int* newdispl, int* vcounts, int* displs, EMPI_Monitor_type *smonitor, int *stflops, int *fc) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_spawn_fcost in <%s> ***\n", __FILE__);
    #endif

    double *rp = NULL, srp = 0.0;

    int count, disp, *tvcounts = NULL, *tdispls = NULL, nrows = 0, acumm = 0, nindex = 0;

    int gcount = 0, psum = 0, n, *fcrecv = NULL;

    //memory allocation
    rp = (double*) calloc (newsize, sizeof (double));
    assert (rp);

    //set rp and srp
    for (n = 0; n < newsize; n ++) {

        //get mflops
        if (EMPI_GLOBAL_lbpolicy == EMPI_LBSTATIC) {
            rp[n] = 1;
        }
        else{
            rp[n] = stflops[n];
        }

        srp += rp[n];
    }

    //memory allocation
    tvcounts = (int*) calloc (size, sizeof (int));
    assert (tvcounts);
    tdispls = (int*) calloc (size, sizeof (int));
    assert (tdispls);

    EMPI_Get_wsize (rank, size, EMPI_GLOBAL_Data->size, &disp, &count, tvcounts, tdispls);

    //memory allocation
    fcrecv = (int*) calloc (EMPI_GLOBAL_Data->size, sizeof(int));
    assert (fcrecv);

    //reduce fcost array
    PMPI_Allgatherv (fc, count, MPI_INT, fcrecv, tvcounts, tdispls, MPI_INT, EMPI_COMM_WORLD);

    for (n = 0; n < EMPI_GLOBAL_Data->size; n ++) gcount += fcrecv[n];

    free (tvcounts);
    free (tdispls);

    //set rcount and rdispl
    for (n = 0; n < newsize; n ++) {

        vcounts[n] = gcount * (rp[n]/srp);

        nrows = acumm = 0;

        while ((nindex < EMPI_GLOBAL_Data->size)&&(acumm < vcounts[n])) {

            acumm += fcrecv[nindex];

            nrows ++;

            nindex ++;
        }

        vcounts[n] = nrows;

        psum += vcounts[n];

        if (n > 0) displs[n] = displs[n-1] + vcounts[n-1];
    }

        // Assign the remaider elements
    n = 0;
    while (psum < EMPI_GLOBAL_Data->size)
    {
        vcounts[n] += 1;
        psum++;
        n++;
        if(n == newsize) n = 0;
    }

    
    //set rcount and rdispl
    *newcount = vcounts[rank];
    *newdispl = displs[rank];

    //free memory
    free (rp);
    free (fcrecv);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_spawn_fcost in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_remove'
*
****************************************************************************************************************************************/
void EMPI_LBalance_remove (int rank, int size, int newsize, int* newcount, int* newdispl, int* vcounts, int* displs, EMPI_Monitor_type *smonitor, int* rremvs, int* fc) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_remove in <%s> ***\n", __FILE__);
    #endif

    //Load balancing
    /* CHANGE: begin */
    //if (EMPI_GLOBAL_Data->stype == EMPI_SPARSE) {
    if ((EMPI_GLOBAL_Data != NULL) && (EMPI_GLOBAL_Data->stype == EMPI_SPARSE)) {
    /* CHANGE: end */

        if (EMPI_GLOBAL_allocation == EMPI_NNZ)

            //sparse balance
            EMPI_LBalance_remove_sparse (rank, size, newsize, newcount, newdispl, vcounts, displs, smonitor, rremvs);

        else if (EMPI_GLOBAL_allocation == EMPI_FCOST)

            //cost balance
            EMPI_LBalance_remove_fcost (rank, size, newsize, newcount, newdispl, vcounts, displs, smonitor, rremvs, fc);

        else if (EMPI_GLOBAL_allocation == EMPI_ROWS)

            //dense balance
            EMPI_LBalance_remove_dense (rank, size, newsize, newcount, newdispl, vcounts, displs, smonitor, rremvs);

    /* CHANGE: begin */
    //} else if ((EMPI_GLOBAL_Data->stype == EMPI_DENSE)||(EMPI_GLOBAL_Data->stype == EMPI_VECTOR)) {
    } else if ( (EMPI_GLOBAL_Data != NULL) && ((EMPI_GLOBAL_Data->stype == EMPI_DENSE)||(EMPI_GLOBAL_Data->stype == EMPI_VECTOR)) ) {
    /* CHANGE: end */

        if (EMPI_GLOBAL_allocation == EMPI_ROWS)

            //dense balance
            EMPI_LBalance_remove_dense (rank, size, newsize, newcount, newdispl, vcounts, displs, smonitor, rremvs);

        else if (EMPI_GLOBAL_allocation == EMPI_FCOST)

            //cost balance
            EMPI_LBalance_remove_fcost (rank, size, newsize, newcount, newdispl, vcounts, displs, smonitor, rremvs, fc);

    } else

        //dense balance
        EMPI_LBalance_remove_dense (rank, size, newsize, newcount, newdispl, vcounts, displs, smonitor, rremvs);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_LBalance_remove in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_remove_dense'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_remove_dense (int rank, int size, int newsize, int* newcount, int* newdispl, int* vcounts, int* displs, EMPI_Monitor_type *smonitor, int* rremvs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_remove_dense in <%s> ***\n", __FILE__);
    #endif

    double *rp = NULL, srp = 0.0;

    int gcount = 0, psum = 0, n, m, pfound, idx;

    int *tvcounts = NULL, *tdispls = NULL, lastrank = (size-1);

    //memory allocation
    rp = (double*) calloc (size, sizeof (double));
    assert (rp);

    //set gcount, rp and srp
    for (n = 0, pfound = EMPI_FALSE; n < size; n ++, pfound = EMPI_FALSE) {

        //buscar si el proceso va a ser removido
        for (m = 0; m < (size-newsize); m ++) {

            if (rremvs[m] == n) {

                pfound = EMPI_TRUE;
            }
        }

        if (pfound == EMPI_FALSE) {

            //set gcount, rp and srp
            if (EMPI_GLOBAL_lbpolicy == EMPI_LBSTATIC) {
                //get partition size
                rp[n] = 1;
            }
            else{    
                //get mflops     
                rp[n] = (int)(smonitor[n].flops/smonitor[n].rtime);
            }
            srp += rp[n];
            
        } else if (pfound == EMPI_TRUE) {
            //removed process
            rp [n] = -1;
        }
    }

    //set global count
    gcount = EMPI_GLOBAL_Data->size;

    //memory allocation
    tvcounts = (int*) calloc (size, sizeof (int));
    assert (tvcounts);
    tdispls = (int*) calloc (size, sizeof (int));
    assert (tdispls);

    //set rcount and rdispl
    for (n = 0; n < size; n ++) {

        if (rp[n] > 0) {

            tvcounts[n] = gcount * (rp[n]/srp);

            psum += tvcounts[n];

            if (n > 0) {

                for (m = 0; m < n; m ++) {

                    if (rp[m] > 0)

                        tdispls[n] = tdispls[m] + tvcounts[m];
                }
            }

        } else {

            //removed process
            tvcounts[n] = -1;
            tdispls[n] = -1;
        }
    }

    // Assign the remaider elements
    n = 0;
    while (psum < gcount)
    {
        if(rp[n] > 0){
            tvcounts[n] += 1;
            psum++;
        }
        n++;
        if(n == size) n = 0;
   }

    for (idx = 0, n = 0; n < size; n ++) {

        if (rp[n] > 0) {

            vcounts[idx] = tvcounts[n];
            displs[idx] = tdispls[n];

            idx ++;
        }
    }

    //set rcount and rdispl
    *newcount = tvcounts[rank];
    *newdispl = tdispls[rank];

    //free memory
    free (rp);
    free (tvcounts);
    free (tdispls);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_LBalance_remove_dense in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_remove_sparse'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_remove_sparse (int rank, int size, int newsize, int* newcount, int* newdispl, int* vcounts, int* displs, EMPI_Monitor_type *smonitor, int* rremvs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_remove_sparse in <%s> ***\n", __FILE__);
    #endif

    double *rp = NULL, srp = 0.0;

    int gcount = 0, psum = 0, n, m, pfound, idx, disp, count, nrows = 0, acumm = 0, nindex = 0;

    int *tvcounts = NULL, *tdispls = NULL, lastrank = (size-1), *nnz = NULL, *nnzrecv = NULL;

    int *addr_row = NULL, *addr_col = NULL;

    void *addr_val = NULL;

    //get sparse address
    EMPI_Get_addr_sparse (EMPI_GLOBAL_Data->id, (void*)&addr_row, (void*)&addr_col, (void*)&addr_val);

    //memory allocation
    rp = (double*) calloc (size, sizeof (double));
    assert (rp);

    //set gcount, rp and srp
    for (n = 0, pfound = EMPI_FALSE; n < size; n ++, pfound = EMPI_FALSE) {

        //buscar si el proceso va a ser removido
        for (m = 0; m < (size-newsize); m ++) {

            if (rremvs[m] == n) {

                pfound = EMPI_TRUE;
            }
        }

        if (pfound == EMPI_FALSE) {

            //get mflops
            if (EMPI_GLOBAL_lbpolicy == EMPI_LBSTATIC) {
                rp[n] = 1;
            }
            else{
                rp[n] = (int)(smonitor[n].flops/smonitor[n].rtime);
            }
            srp += rp[n];

        } else if (pfound == EMPI_TRUE) {

            //removed process
            rp [n] = -1;
        }

        gcount += smonitor[n].count;
    }

    //memory allocation
    tvcounts = (int*) calloc (size, sizeof (int));
    assert (tvcounts);
    tdispls = (int*) calloc (size, sizeof (int));
    assert (tdispls);

    EMPI_Get_wsize (rank, size, EMPI_GLOBAL_Data->size, &disp, &count, tvcounts, tdispls);

    //memory allocation
    nnz = (int*) calloc (count, sizeof(int));
    assert (nnz);
    nnzrecv = (int*) calloc (EMPI_GLOBAL_Data->size, sizeof(int));
    assert (nnzrecv);

    //set nnz array
    for (n = 0; n < count; n ++) nnz[n] = (addr_row[n+1] - addr_row[n]);

    //reduce nnz array
    PMPI_Allgatherv (nnz, count, MPI_INT, nnzrecv, tvcounts, tdispls, MPI_INT, EMPI_COMM_WORLD);

    free (nnz);

    //set rcount and rdispl
    for (n = 0; n < size; n ++) {

        if (rp[n] > 0) {

            tvcounts[n] = gcount * (rp[n]/srp);

            nrows = acumm = 0;

            while ((nindex < EMPI_GLOBAL_Data->size)&&(acumm < tvcounts[n])) {

                acumm += nnzrecv[nindex];

                nrows ++;

                nindex ++;
            }

            tvcounts[n] = nrows;

            psum += tvcounts[n];

            if (n > 0) {

                for (m = 0; m < n; m ++) {

                    if (rp[m] > 0)

                        tdispls[n] = tdispls[m] + tvcounts[m];
                }
            }

        } else {

            //removed process
            tvcounts[n] = -1;
            tdispls[n] = -1;
        }
    }
    
    // Assign the remaider elements
    n = 0;
    while (psum < EMPI_GLOBAL_Data->size)
    {
        if(rp[n] > 0){
            tvcounts[n] += 1;
            psum++;
        }
        n++;
        if(n == size) n = 0;
    }

    for (idx = 0, n = 0; n < size; n ++) {

        if (rp[n] > 0) {

            vcounts[idx] = tvcounts[n];
            displs[idx] = tdispls[n];

            idx ++;
        }
    }

    //set rcount and rdispl
    *newcount = tvcounts[rank];
    *newdispl = tdispls[rank];

    //free memory
    free (rp);
    free (tvcounts);
    free (tdispls);
    free (nnzrecv);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_LBalance_remove_sparse in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_remove_fcost'
*
****************************************************************************************************************************************/
static void EMPI_LBalance_remove_fcost (int rank, int size, int newsize, int* newcount, int* newdispl, int* vcounts, int* displs, EMPI_Monitor_type *smonitor, int* rremvs, int* fc) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_remove_fcost in <%s> ***\n", __FILE__);
    #endif

    double *rp = NULL, srp = 0.0;

    int gcount = 0, psum = 0, n, m, pfound, idx, disp, count, nrows = 0, acumm = 0, nindex = 0;

    int *tvcounts = NULL, *tdispls = NULL, lastrank = (size-1), *fcrecv = NULL;

    //memory allocation
    rp = (double*) calloc (size, sizeof (double));
    assert (rp);

    //set gcount, rp and srp
    for (n = 0, pfound = EMPI_FALSE; n < size; n ++, pfound = EMPI_FALSE) {

        //buscar si el proceso va a ser removido
        for (m = 0; m < (size-newsize); m ++) {

            if (rremvs[m] == n) {

                pfound = EMPI_TRUE;
            }
        }

        if (pfound == EMPI_FALSE) {

            
            if (EMPI_GLOBAL_lbpolicy == EMPI_LBSTATIC) {
                rp[n] = 1;
            }
            else{  
                //get mflops            
                rp[n] = (int)(smonitor[n].flops/smonitor[n].rtime);
            }
            srp += rp[n];

        } else if (pfound == EMPI_TRUE) {

            //removed process
            rp [n] = -1;
        }
    }

    //memory allocation
    tvcounts = (int*) calloc (size, sizeof (int));
    assert (tvcounts);
    tdispls = (int*) calloc (size, sizeof (int));
    assert (tdispls);

    EMPI_Get_wsize (rank, size, EMPI_GLOBAL_Data->size, &disp, &count, tvcounts, tdispls);

    //memory allocation
    fcrecv = (int*) calloc (EMPI_GLOBAL_Data->size, sizeof(int));
    assert (fcrecv);

    //reduce nnz array
    PMPI_Allgatherv (fc, count, MPI_INT, fcrecv, tvcounts, tdispls, MPI_INT, EMPI_COMM_WORLD);

    for (n = 0; n < EMPI_GLOBAL_Data->size; n ++) gcount += fcrecv[n];

    //set rcount and rdispl
    for (n = 0; n < size; n ++) {

        if (rp[n] > 0) {

            tvcounts[n] = gcount * (rp[n]/srp);

            nrows = acumm = 0;

            while ((nindex < EMPI_GLOBAL_Data->size)&&(acumm < tvcounts[n])) {

                acumm += fcrecv[nindex];

                nrows ++;

                nindex ++;
            }

            tvcounts[n] = nrows;

            psum += tvcounts[n];

            if (n > 0) {

                for (m = 0; m < n; m ++) {

                    if (rp[m] > 0)

                        tdispls[n] = tdispls[m] + tvcounts[m];
                }
            }

        } else {

            //removed process
            tvcounts[n] = -1;
            tdispls[n] = -1;
        }
    }

    
    // Assign the remaider elements
    n = 0;
    while (psum < EMPI_GLOBAL_Data->size)
    {
        if(rp[n] > 0){
            tvcounts[n] += 1;
            psum++;
        }
        n++;
        if(n == size) n = 0;
    }

    for (idx = 0, n = 0; n < size; n ++) {

        if (rp[n] > 0) {

            vcounts[idx] = tvcounts[n];
            displs[idx] = tdispls[n];

            idx ++;
        }
    }

    //set rcount and rdispl
    *newcount = tvcounts[rank];
    *newdispl = tdispls[rank];

    //free memory
    free (rp);
    free (tvcounts);
    free (tdispls);
    free (fcrecv);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_LBalance_remove_fcost in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance'
*
****************************************************************************************************************************************/
int EMPI_LBalance (int *rank, int *size, int count, int disp, int *fc, EMPI_Monitor_type *smonitor) {


    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance in <%s> ***\n", __FILE__);
    #endif

    int n, cpu_balance, real_balance, external_load = EMPI_NULL, bttype;

    double tini;

    tini = MPI_Wtime();

    //return if flops are ZERO
    for (n = 0; n < *size; n ++) if (smonitor[n].flops == 0) return EMPI_ERROR;

    //evaluate cpu balance
    cpu_balance = EMPI_evaluate_LB (*size, smonitor, EMPI_TCPU);

    //evaluate real balance
    real_balance = EMPI_evaluate_LB (*size, smonitor, EMPI_TREAL);

    if (real_balance == EMPI_BALANCED) {

        //self_adaptation = false
        if (EMPI_GLOBAL_self_adaptation == EMPI_FALSE) EMPI_GLOBAL_lbalance = EMPI_FALSE;

        return EMPI_BALANCED;
    }

    //evaluate system status
    EMPI_evaluate_system_status (*size, smonitor, &EMPI_GLOBAL_hmon[EMPI_GLOBAL_hpos]);

    //evaluate external load
    if (EMPI_GLOBAL_hmon[EMPI_GLOBAL_hpos] == EMPI_NON_DEDICATED)

        external_load = EMPI_evaluate_external_load (EMPI_GLOBAL_hmon, EMPI_GLOBAL_hsteps);

    if (((EMPI_GLOBAL_hmon[EMPI_GLOBAL_hpos] == EMPI_DEDICATED) && (cpu_balance == EMPI_UNBALANCED))
    || ((external_load == EMPI_BURST) && (cpu_balance == EMPI_UNBALANCED))) {

        //Tcpu load balance
        bttype = EMPI_TCPU;

    } else if ((external_load == EMPI_LONG_TERM) && (real_balance == EMPI_UNBALANCED)) {

        //Treal load balance
        bttype = EMPI_TREAL;

    } else {

        EMPI_GLOBAL_hpos = ((EMPI_GLOBAL_hpos + 1) % EMPI_GLOBAL_hsteps);

        return -1;
    }

    EMPI_GLOBAL_hpos = ((EMPI_GLOBAL_hpos + 1) % EMPI_GLOBAL_hsteps);

    EMPI_GLOBAL_overhead_lbalance += (MPI_Wtime() - tini);

    //Load balancing
    /* CHANGE: begin */
    //if (EMPI_GLOBAL_Data->stype == EMPI_SPARSE) {
    if ((EMPI_GLOBAL_Data != NULL) && (EMPI_GLOBAL_Data->stype == EMPI_SPARSE)) {
    /* CHANGE: end */


        if (EMPI_GLOBAL_allocation == EMPI_NNZ)

            //sparse balance
            EMPI_LBalance_sparse (rank, size, count, disp, smonitor, bttype);

        else if (EMPI_GLOBAL_allocation == EMPI_FCOST)

            //cost balance
            EMPI_LBalance_fcost (rank, size, count, disp, fc, smonitor, bttype);

        else if (EMPI_GLOBAL_allocation == EMPI_ROWS)

            //dense balance
            EMPI_LBalance_dense (rank, size, count, disp, smonitor, bttype);

    /* CHANGE: begin */
    //} else if ((EMPI_GLOBAL_Data->stype == EMPI_DENSE)||(EMPI_GLOBAL_Data->stype == EMPI_VECTOR)) {
    } else if ( (EMPI_GLOBAL_Data != NULL) && ((EMPI_GLOBAL_Data->stype == EMPI_DENSE)||(EMPI_GLOBAL_Data->stype == EMPI_VECTOR)) ) {
    /* CHANGE: end */

        if (EMPI_GLOBAL_allocation == EMPI_ROWS)

            //dense balance
            EMPI_LBalance_dense (rank, size, count, disp, smonitor, bttype);

        else if (EMPI_GLOBAL_allocation == EMPI_FCOST)

            //cost balance
            EMPI_LBalance_fcost (rank, size, count, disp, fc, smonitor, bttype);

    } else

        //dense balance
        EMPI_LBalance_dense (rank, size, count, disp, smonitor, bttype);

    return EMPI_UNBALANCED;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_LBalance in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_dense'
*
****************************************************************************************************************************************/
static int EMPI_LBalance_dense (int *rank, int *size, int count, int disp, EMPI_Monitor_type *smonitor, int BTtype) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_dense in <%s> ***\n", __FILE__);
    #endif

    //counts
    int gcount = 0, psum = 0, n, rdispl, rcount;

    int *vcounts = NULL, *displs = NULL;

    //relative computing power
    double *rp = NULL, srp = 0.0;

    double tini;

    tini = MPI_Wtime();

    //memory allocation
    rp = (double*) calloc (*size, sizeof (double));
    assert (rp);

    // Static policy: fixed block assignment
    if (EMPI_GLOBAL_lbpolicy == EMPI_LBSTATIC) {

        //set gcount, rp and srp
        for (n = 0; n < *size; n ++) {

            //get partition size
            rp[n] = 1;

            gcount += smonitor[n].count;

            srp += rp[n];
        }
        
    }   else if (BTtype == EMPI_TCPU) {

        if (EMPI_GLOBAL_lbpolicy == EMPI_LBMFLOPS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get mflop/time
                rp[n] = (double)(smonitor[n].flops / smonitor[n].ptime);

                if (rp[n] <= 0) return EMPI_ERROR;

                gcount += smonitor[n].count;

                srp += rp[n];
            }

        } else if (EMPI_GLOBAL_lbpolicy == EMPI_LBCOUNTS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get count/time
                rp[n] = (double)(smonitor[n].count / (smonitor[n].ptime * 1.0E-6));

                if (rp[n] <= 0) return EMPI_ERROR;

                gcount += smonitor[n].count;

                srp += rp[n];
            }
        }

    } else if (BTtype == EMPI_TREAL) {

        if (EMPI_GLOBAL_lbpolicy == EMPI_LBMFLOPS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get mflop/time
                rp[n] = (double)(smonitor[n].flops / smonitor[n].rtime);

                if (rp[n] <= 0) return EMPI_ERROR;

                gcount += smonitor[n].count;

                srp += rp[n];
            }

        } else if (EMPI_GLOBAL_lbpolicy == EMPI_LBCOUNTS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get count/time
                rp[n] = (double)(smonitor[n].count / (smonitor[n].rtime * 1.0E-6));

                if (rp[n] <= 0) return EMPI_ERROR;

                gcount += smonitor[n].count;

                srp += rp[n];
            }
        }
    }

    //memory allocation
    vcounts = (int*) calloc (*size, sizeof (int));
    assert (vcounts);
    displs = (int*) calloc (*size, sizeof (int));
    assert (displs);

    //set rcount and rdispl
    for (n = 0; n < *size; n ++) {

        vcounts[n] = gcount * (rp[n]/srp);

        if (vcounts[n] <= 0) {

            //free memory
            free (vcounts);
            free (displs);
            free (rp);

            EMPI_GLOBAL_overhead_lbalance += (MPI_Wtime() - tini);

            return EMPI_ERROR;
        }

        psum += vcounts[n];

        if (n > 0) displs[n] = displs[n-1] + vcounts[n-1];
    }

     // Assign the remaider elements
    n = 0;
    while (psum < gcount)
    {
        vcounts[n] += 1;
        psum++;
        n++;
        if(n == *size) n = 0;
    }
    
    //free memory
    free (rp);

    EMPI_GLOBAL_overhead_lbalance += (MPI_Wtime() - tini);

    //dynamic partitions
    rcount = vcounts[*rank];
    rdispl = displs[*rank];

    tini = MPI_Wtime();

    //redistribute data
    EMPI_Rdata (count, disp, rcount, rdispl);

    EMPI_GLOBAL_overhead_rdata += (MPI_Wtime() - tini);

    //set dynamic workload
    EMPI_GLOBAL_wpolicy = EMPI_DYNAMIC;

    if (EMPI_GLOBAL_vcounts != NULL) {free (EMPI_GLOBAL_vcounts); EMPI_GLOBAL_vcounts = NULL;}
    if (EMPI_GLOBAL_displs != NULL) {free (EMPI_GLOBAL_displs); EMPI_GLOBAL_displs = NULL;}

    EMPI_GLOBAL_vcounts = (int*) calloc (*size, sizeof (int));
    assert (EMPI_GLOBAL_vcounts);

    EMPI_GLOBAL_displs = (int*) calloc (*size, sizeof (int));
    assert (EMPI_GLOBAL_displs);

    //memcpy
    memcpy (EMPI_GLOBAL_vcounts, vcounts, (*size)*sizeof(int));
    memcpy (EMPI_GLOBAL_displs, displs, (*size)*sizeof(int));

    //free memory
    free (vcounts);
    free (displs);

    return EMPI_END;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_LBalance_dense in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_sparse'
*
****************************************************************************************************************************************/
static int EMPI_LBalance_sparse (int *rank, int *size, int count, int disp, EMPI_Monitor_type *smonitor, int BTtype) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_sparse in <%s> ***\n", __FILE__);
    #endif

    //counts
    int gcount =  0, psum = 0, n, rdispl, rcount;

    int *vcounts = NULL, *displs = NULL;

    int *nnz, *nnzrecv, fdisp, fcount, nindex = 0, acumm = 0, nrows = 0;

    int *addr_row = NULL, *addr_col = NULL;

    void *addr_val = NULL;

    //relative computing power
    double *rp = NULL, srp = 0.0;

    double tini;

    tini = MPI_Wtime();

    //get sparse address
    EMPI_Get_addr_sparse (EMPI_GLOBAL_Data->id, (void*)&addr_row, (void*)&addr_col, (void*)&addr_val);

    //memory allocation
    rp = (double*) calloc (*size, sizeof (double));
    assert (rp);

    // Static policy: fixed block assignment
    if (EMPI_GLOBAL_lbpolicy == EMPI_LBSTATIC) {

        //set gcount, rp and srp
        for (n = 0; n < *size; n ++) {

            //get partition size
            rp[n] = 1;

            gcount += smonitor[n].count;

            srp += rp[n];
        }
        
    }   else if (BTtype == EMPI_TCPU) {

        if (EMPI_GLOBAL_lbpolicy == EMPI_LBMFLOPS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get mflop/time
                rp[n] = (double)(smonitor[n].flops / smonitor[n].ptime);

                if (rp[n] <= 0) return EMPI_ERROR;

                gcount += smonitor[n].count;

                srp += rp[n];
            }

        } else if (EMPI_GLOBAL_lbpolicy == EMPI_LBCOUNTS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get count/time
                rp[n] = (double)(smonitor[n].count / (smonitor[n].ptime * 1.0E-6));

                if (rp[n] <= 0) return EMPI_ERROR;

                gcount += smonitor[n].count;

                srp += rp[n];
            }
        }

    } else if (BTtype == EMPI_TREAL) {

        if (EMPI_GLOBAL_lbpolicy == EMPI_LBMFLOPS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get mflop/time
                rp[n] = (double)(smonitor[n].flops / smonitor[n].rtime);

                if (rp[n] <= 0) return EMPI_ERROR;

                gcount += smonitor[n].count;

                srp += rp[n];
            }

        } else if (EMPI_GLOBAL_lbpolicy == EMPI_LBCOUNTS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get count/time
                rp[n] = (double)(smonitor[n].count / (smonitor[n].rtime * 1.0E-6));

                if (rp[n] <= 0) return EMPI_ERROR;

                gcount += smonitor[n].count;

                srp += rp[n];
            }
        }
    }

    //memory allocation
    vcounts = (int*) calloc (*size, sizeof (int));
    assert (vcounts);
    displs = (int*) calloc (*size, sizeof (int));
    assert (displs);

    EMPI_Get_wsize (*rank, *size, EMPI_GLOBAL_Data->size, &fdisp, &fcount, vcounts, displs);

    //memory allocation
    nnz = (int*) calloc (count, sizeof(int));
    assert (nnz);
    nnzrecv = (int*) calloc (EMPI_GLOBAL_Data->size, sizeof(int));
    assert (nnzrecv);

    //set nnz array
    for (n = 0; n < count; n ++) nnz[n] = (addr_row[n+1] - addr_row[n]);

    //reduce nnz array
    PMPI_Allgatherv (nnz, count, MPI_INT, nnzrecv, vcounts, displs, MPI_INT, EMPI_COMM_WORLD);

    free (nnz);

    //set rcount and rdispl
    for (n = 0; n < *size; n ++) {

        vcounts[n] = gcount * (rp[n]/srp);

        nrows = acumm = 0;

        while ((nindex < EMPI_GLOBAL_Data->size)&&(acumm < vcounts[n])) {

            acumm += nnzrecv[nindex];

            nrows ++;

            nindex ++;
        }

        vcounts[n] = nrows;

        if (vcounts[n] <= 0) {

            //free memory
            free (vcounts);
            free (displs);
            free (nnzrecv);
            free (rp);

            EMPI_GLOBAL_overhead_lbalance += (MPI_Wtime() - tini);

            return EMPI_ERROR;
        }

        psum += vcounts[n];

        if (n > 0) displs[n] = displs[n-1] + vcounts[n-1];
    }

    // Assign the remaider elements
    n = 0;
    while (psum < EMPI_GLOBAL_Data->size)
    {
        vcounts[n] += 1;
        psum++;
        n++;
        if(n == *size) n = 0;
    }
    
    //free memory
    free (nnzrecv);
    free (rp);

    EMPI_GLOBAL_overhead_lbalance += (MPI_Wtime() - tini);

    //dynamic partitions
    rcount = vcounts[*rank];
    rdispl = displs[*rank];

    tini = MPI_Wtime();

    //redistribute data
    EMPI_Rdata (count, disp, rcount, rdispl);

    EMPI_GLOBAL_overhead_rdata += (MPI_Wtime() - tini);

    //set dynamic workload
    EMPI_GLOBAL_wpolicy = EMPI_DYNAMIC;

    if (EMPI_GLOBAL_vcounts != NULL) {free (EMPI_GLOBAL_vcounts); EMPI_GLOBAL_vcounts = NULL;}
    if (EMPI_GLOBAL_displs != NULL) {free (EMPI_GLOBAL_displs); EMPI_GLOBAL_displs = NULL;}

    EMPI_GLOBAL_vcounts = (int*) calloc (*size, sizeof (int));
    assert (EMPI_GLOBAL_vcounts);

    EMPI_GLOBAL_displs = (int*) calloc (*size, sizeof (int));
    assert (EMPI_GLOBAL_displs);

    //memcpy
    memcpy (EMPI_GLOBAL_vcounts, vcounts, (*size)*sizeof(int));
    memcpy (EMPI_GLOBAL_displs, displs, (*size)*sizeof(int));

    //free memory
    free (vcounts);
    free (displs);

    return EMPI_END;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_LBalance_sparse in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_LBalance_fcost'
*
****************************************************************************************************************************************/
static int EMPI_LBalance_fcost (int *rank, int *size, int count, int disp, int *fc, EMPI_Monitor_type *smonitor, int BTtype) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_LBalance_fcost in <%s> ***\n", __FILE__);
    #endif

    //counts
    int gcount =  0, psum = 0, n, rdispl, rcount, *fcrecv = NULL;

    int *vcounts = NULL, *displs = NULL;

    int fdisp, fcount, nindex = 0, acumm = 0, nrows = 0;

    //relative computing power
    double *rp = NULL, srp = 0.0;

    double tini;

    tini = MPI_Wtime();

    //memory allocation
    rp = (double*) calloc (*size, sizeof (double));
    assert (rp);

    // Static policy: fixed block assignment
    if (EMPI_GLOBAL_lbpolicy == EMPI_LBSTATIC) {

        //set gcount, rp and srp
        for (n = 0; n < *size; n ++) {

            //get partition size
            rp[n] = 1;

            srp += rp[n];
        }
        
    }   else if (BTtype == EMPI_TCPU) {

        if (EMPI_GLOBAL_lbpolicy == EMPI_LBMFLOPS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get mflop/time
                rp[n] = (double)(smonitor[n].flops / smonitor[n].ptime);

                if (rp[n] <= 0) return EMPI_ERROR;

                srp += rp[n];
            }

        } else if (EMPI_GLOBAL_lbpolicy == EMPI_LBCOUNTS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get count/time
                rp[n] = (double)(smonitor[n].count / (smonitor[n].ptime * 1.0E-6));

                if (rp[n] <= 0) return EMPI_ERROR;

                srp += rp[n];
            }
        }

    } else if (BTtype == EMPI_TREAL) {

        if (EMPI_GLOBAL_lbpolicy == EMPI_LBMFLOPS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get mflop/time
                rp[n] = (double)(smonitor[n].flops / smonitor[n].rtime);

                if (rp[n] <= 0) return EMPI_ERROR;

                srp += rp[n];
            }

        } else if (EMPI_GLOBAL_lbpolicy == EMPI_LBCOUNTS) {

            //set gcount, rp and srp
            for (n = 0; n < *size; n ++) {

                //get count/time
                rp[n] = (double)(smonitor[n].count / (smonitor[n].rtime * 1.0E-6));

                if (rp[n] <= 0) return EMPI_ERROR;

                srp += rp[n];
            }
        }
    }

    //memory allocation
    vcounts = (int*) calloc (*size, sizeof (int));
    assert (vcounts);
    displs = (int*) calloc (*size, sizeof (int));
    assert (displs);

    EMPI_Get_wsize (*rank, *size, EMPI_GLOBAL_Data->size, &fdisp, &fcount, vcounts, displs);

    //memory allocation
    fcrecv = (int*) malloc (EMPI_GLOBAL_Data->size * sizeof(int));
    assert (fcrecv);

    PMPI_Allgatherv (fc, count, MPI_INT, fcrecv, vcounts, displs, MPI_INT, EMPI_COMM_WORLD);

    for (n = 0; n < EMPI_GLOBAL_Data->size; n ++) gcount += fcrecv[n];

    //set rcount and rdispl
    for (n = 0; n < *size; n ++) {

        vcounts[n] = gcount * (rp[n]/srp);

        nrows = acumm = 0;

        while ((nindex < EMPI_GLOBAL_Data->size)&&(acumm < vcounts[n])) {

            acumm += fcrecv[nindex];

            nrows ++;

            nindex ++;
        }

        vcounts[n] = nrows;

        if (vcounts[n] <= 0) {

            //free memory
            free (vcounts);
            free (displs);
            free (fcrecv);
            free (rp);

            EMPI_GLOBAL_overhead_lbalance += (MPI_Wtime() - tini);

            return EMPI_ERROR;
        }

        psum += vcounts[n];

        if (n > 0) displs[n] = displs[n-1] + vcounts[n-1];
    }

    // Assign the remaider elements
    n = 0;
    while (psum < EMPI_GLOBAL_Data->size)
    {
        vcounts[n] += 1;
        psum++;
        n++;
        if(n == *size) n = 0;
    }
    
    //free memory
    free (fcrecv);
    free (rp);

    EMPI_GLOBAL_overhead_lbalance += (MPI_Wtime() - tini);

    //dynamic partitions
    rcount = vcounts[*rank];
    rdispl = displs[*rank];

    tini = MPI_Wtime();

    //redistribute data
    EMPI_Rdata (count, disp, rcount, rdispl);

    EMPI_GLOBAL_overhead_rdata += (MPI_Wtime() - tini);

    //set dynamic workload
    EMPI_GLOBAL_wpolicy = EMPI_DYNAMIC;

    if (EMPI_GLOBAL_vcounts != NULL) {free (EMPI_GLOBAL_vcounts); EMPI_GLOBAL_vcounts = NULL;}
    if (EMPI_GLOBAL_displs != NULL) {free (EMPI_GLOBAL_displs); EMPI_GLOBAL_displs = NULL;}

    EMPI_GLOBAL_vcounts = (int*) calloc (*size, sizeof (int));
    assert (EMPI_GLOBAL_vcounts);

    EMPI_GLOBAL_displs = (int*) calloc (*size, sizeof (int));
    assert (EMPI_GLOBAL_displs);

    //memcpy
    memcpy (EMPI_GLOBAL_vcounts, vcounts, (*size)*sizeof(int));
    memcpy (EMPI_GLOBAL_displs, displs, (*size)*sizeof(int));

    //free memory
    free (vcounts);
    free (displs);

    return EMPI_END;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_LBalance_fcost in <%s> ***\n", __FILE__);
    #endif
}
