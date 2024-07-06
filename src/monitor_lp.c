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
 *    File:       empi.c                                                                                                                    *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

/* include */
#include <empi.h>
#include <glpk.h>

/* headers */

/****************************************************************************************************************************************
*
*    'EMPI_lp_min_cost_fixed'
*
****************************************************************************************************************************************/
void EMPI_lp_min_cost_fixed (int rflops, int rprocs, int *mflops, int *newsize, int *cost, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_min_cost_fixed in <%s> ***\n", __FILE__);
    #endif

    //NOTA: minimizar el coste con un techo de texec = incrementar procesos por exceso de tiempo de ejecucion.

    //Quiero sumar RFLOPS que tengan menor coste

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: minimize cost
    glp_set_obj_dir(lp, GLP_MIN);

    glp_add_rows(lp, 2);

    glp_set_row_name(lp, 1, "MFLOPS");
    glp_set_row_bnds(lp, 1, GLP_LO, rflops, 0.0);

    glp_set_row_name(lp, 2, "TOT_PROCS");
    glp_set_row_bnds(lp, 2, GLP_FX, rprocs, rprocs);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        //cost per class
        glp_set_col_name(lp, n+1, colname);
        if ((class->maxprocs-class->nprocs) == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, (class->maxprocs-class->nprocs));
        glp_set_obj_coef(lp, n+1, class->cost);
        glp_set_col_kind(lp, n+1, GLP_IV);

        //mflops per class
        ia[(n*2)+1] = 1, ja[(n*2)+1] = n+1, ar[(n*2)+1] = class->mflops;
        ia[(n*2)+2] = 2, ja[(n*2)+2] = n+1, ar[(n*2)+2] = 1;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses*2, ia, ja, ar);

    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    class = EMPI_GLOBAL_system_classes;
    //printf("There are %d host classes\n", EMPI_GLOBAL_nhclasses);
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }

    //set newsize and mflops
    *newsize = 0;
    *mflops = 0;
    *cost = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set newsize
        *newsize = *newsize + class_procs[n];
        //set mflops
        *mflops = *mflops + (class_procs[n] * class->mflops);
        //set cost
        *cost = *cost + (class_procs[n] * class->cost);
//printf ("class %i - nprocs %lfs\n", n, class_procs[n]);
        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_min_cost_fixed in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_max_cost_fixed'
*
****************************************************************************************************************************************/
void EMPI_lp_max_cost_fixed (int rflops, int rprocs, int *mflops, int *newsize, int *cost, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_max_cost_fixed in <%s> ***\n", __FILE__);
    #endif

    //NOTA: maximizar el coste con un techo de texec = remover procesos porque puedo tardar mas (texec) maximizando el ahorro en coste.

    //Quiero quitar RFLOPS que tengan mayor coste

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: maximize cost
    glp_set_obj_dir(lp, GLP_MAX);

    glp_add_rows(lp, 2);

    glp_set_row_name(lp, 1, "MFLOPS");
    glp_set_row_bnds(lp, 1, GLP_UP, 0.0, rflops);

    glp_set_row_name(lp, 2, "TOT_PROCS");
    glp_set_row_bnds(lp, 2, GLP_FX, rprocs, rprocs);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        //cost per class
        glp_set_col_name(lp, n+1, colname);
        if (class->nprocs == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, class->nprocs);
        glp_set_obj_coef(lp, n+1, class->cost);
        glp_set_col_kind(lp, n+1, GLP_IV);

        //mflops per class
        ia[(n*2)+1] = 1, ja[(n*2)+1] = n+1, ar[(n*2)+1] = class->mflops;
        ia[(n*2)+2] = 2, ja[(n*2)+2] = n+1, ar[(n*2)+2] = 1;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses*2, ia, ja, ar);
    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);

    }
    //set newsize and mflops
    *newsize = 0;
    *mflops = 0;
    *cost = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set newsize
        *newsize = *newsize + class_procs[n];
        //set mflops
        *mflops = *mflops + (class_procs[n] * class->mflops);
        //set cost
        *cost = *cost + (class_procs[n] * class->cost);

        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_max_cost_fixed in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_flops_max'
*
****************************************************************************************************************************************/
void EMPI_lp_flops_max (int rprocs, int *mflops, int *cost, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_flops_max in <%s> ***\n", __FILE__);
    #endif

    //Devuelve max mflops con rprocs

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar, z;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: maximize mflops
    glp_set_obj_dir(lp, GLP_MAX);

    glp_add_rows(lp, 1);

    glp_set_row_name(lp, 1, "TOT_PROCS");
    glp_set_row_bnds(lp, 1, GLP_FX, rprocs, rprocs);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        //cost per class
        glp_set_col_name(lp, n+1, colname);
        if ((class->maxprocs-class->nprocs) == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, (class->maxprocs-class->nprocs));
        glp_set_obj_coef(lp, n+1, class->mflops);
        glp_set_col_kind(lp, n+1, GLP_IV);

        //nprocs per class
        ia[n+1] = 1, ja[n+1] = n+1, ar[n+1] = 1;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses, ia, ja, ar);
    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    z = glp_mip_obj_val(lp);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }
    //set newsize and mflops
    *mflops = z;
    *cost = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        //set cost
        *cost = *cost + (class_procs[n] * class->cost);

        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_flops_max in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_flops_min'
*
****************************************************************************************************************************************/
void EMPI_lp_flops_min (int rprocs, int *mflops, int *cost, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_flops_min in <%s> ***\n", __FILE__);
    #endif

    //Devuelve min mflops con rprocs

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar, z;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: minimize mflops
    glp_set_obj_dir(lp, GLP_MIN);

    glp_add_rows(lp, 1);

    glp_set_row_name(lp, 1, "TOT_PROCS");
    glp_set_row_bnds(lp, 1, GLP_FX, rprocs, rprocs);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        //cost per class
        glp_set_col_name(lp, n+1, colname);
        if (class->nprocs == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, class->nprocs);
        glp_set_obj_coef(lp, n+1, class->mflops);
        glp_set_col_kind(lp, n+1, GLP_IV);

        //nprocs per class
        ia[n+1] = 1, ja[n+1] = n+1, ar[n+1] = 1;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses, ia, ja, ar);
    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    z = glp_mip_obj_val(lp);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }
    //set newsize and mflops
    *mflops = z;
    *cost = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        //set cost
        *cost = *cost + (class_procs[n] * class->cost);

        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_flops_min in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_min_procs'
*
****************************************************************************************************************************************/
void EMPI_lp_min_procs (int rflops, int *newsize, int *mflops, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_min_procs in <%s> ***\n", __FILE__);
    #endif

    //NOTA: minimo numero de procs (newsize) que proporciona, como minimo, rflops

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: minimize nprocs
    glp_set_obj_dir(lp, GLP_MIN);

    glp_add_rows(lp, 1);

    glp_set_row_name(lp, 1, "FLOPS");
    glp_set_row_bnds(lp, 1, GLP_LO, rflops, 0.0);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        glp_set_col_name(lp, n+1, colname);
        if ((class->maxprocs-class->nprocs) == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, (class->maxprocs-class->nprocs));
        glp_set_obj_coef(lp, n+1, 1);
        glp_set_col_kind(lp, n+1, GLP_IV);

        ia[n+1] = 1, ja[n+1] = n+1, ar[n+1] = class->mflops;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses, ia, ja, ar);
    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }
    //set newsize and mflops
    *newsize = 0;
    *mflops = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set newsize
        *newsize = *newsize + class_procs[n];

        //set mflops
        *mflops = *mflops + (class_procs[n] * class->mflops);

        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_min_procs in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_max_procs'
*
****************************************************************************************************************************************/
void EMPI_lp_max_procs (int rflops, int *newsize, int *mflops, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_max_procs in <%s> ***\n", __FILE__);
    #endif

    //NOTA: maximo numero de procs (newsize) que proporciona, como maximo, rflops

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: maximize nprocs
    glp_set_obj_dir(lp, GLP_MAX);

    glp_add_rows(lp, 1);

    glp_set_row_name(lp, 1, "FLOPS");
    glp_set_row_bnds(lp, 1, GLP_UP, 0.0, rflops);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        glp_set_col_name(lp, n+1, colname);
        if (class->nprocs == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, class->nprocs);
        glp_set_obj_coef(lp, n+1, 1);
        glp_set_col_kind(lp, n+1, GLP_IV);

        ia[n+1] = 1, ja[n+1] = n+1, ar[n+1] = class->mflops;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses, ia, ja, ar);
    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }

    //set newsize and mflops
    *newsize = 0;
    *mflops = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set newsize
        *newsize = *newsize + class_procs[n];

        //set mflops
        *mflops = *mflops + (class_procs[n] * class->mflops);

        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_max_procs in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_min_cost'
*
****************************************************************************************************************************************/
void EMPI_lp_min_cost (int rflops, int *newsize, int *mflops, int *cost, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_min_cost in <%s> ***\n", __FILE__);
    #endif

    //NOTA: obtener conjunto de procesos que proporciona, como minimo, rflops, al menor coste

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: minimize cost
    glp_set_obj_dir(lp, GLP_MIN);

    glp_add_rows(lp, 1);

    glp_set_row_name(lp, 1, "FLOPS");
    glp_set_row_bnds(lp, 1, GLP_LO, rflops, 0.0);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        glp_set_col_name(lp, n+1, colname);
        if ((class->maxprocs-class->nprocs) == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, (class->maxprocs-class->nprocs));
        glp_set_obj_coef(lp, n+1, class->cost);
        glp_set_col_kind(lp, n+1, GLP_IV);

        ia[n+1] = 1, ja[n+1] = n+1, ar[n+1] = class->mflops;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses, ia, ja, ar);
    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }
    //set newsize and mflops
    *newsize = 0;
    *mflops = 0;
    *cost = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set newsize
        *newsize = *newsize + class_procs[n];
        //set mflops
        *mflops = *mflops + (class_procs[n] * class->mflops);
        //set cost
        *cost = *cost + (class_procs[n] * class->cost);

        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_min_cost in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_max_cost'
*
****************************************************************************************************************************************/
void EMPI_lp_max_cost (int rflops, int *newsize, int *mflops, int *cost, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_max_cost in <%s> ***\n", __FILE__);
    #endif

    //NOTA: obtener conjunto de procesos que proporciona, como maximo, rflops, al maximo coste

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: maximize cost
    glp_set_obj_dir(lp, GLP_MAX);

    glp_add_rows(lp, 1);

    glp_set_row_name(lp, 1, "FLOPS");
    glp_set_row_bnds(lp, 1, GLP_UP, 0.0, rflops);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        glp_set_col_name(lp, n+1, colname);
        if ((class->maxprocs-class->nprocs) == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, class->nprocs);
        glp_set_obj_coef(lp, n+1, class->cost);
        glp_set_col_kind(lp, n+1, GLP_IV);

        ia[n+1] = 1, ja[n+1] = n+1, ar[n+1] = class->mflops;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses, ia, ja, ar);
    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }

    //set newsize and mflops
    *newsize = 0;
    *mflops = 0;
    *cost = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set newsize
        *newsize = *newsize + class_procs[n];
        //set mflops
        *mflops = *mflops + (class_procs[n] * class->mflops);
        //set cost
        *cost = *cost + (class_procs[n] * class->cost);

        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_max_cost in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_get_procs_flops_spawn'
*
****************************************************************************************************************************************/
void EMPI_lp_get_procs_flops_spawn (int rflops, int rprocs, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_get_procs_flops_spawn in <%s> ***\n", __FILE__);
    #endif

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: minimize cost
    glp_set_obj_dir(lp, GLP_MIN);

    glp_add_rows(lp, 2);

    glp_set_row_name(lp, 1, "MFLOPS");
    glp_set_row_bnds(lp, 1, GLP_FX, rflops, rflops);

    glp_set_row_name(lp, 2, "TOT_PROCS");
    glp_set_row_bnds(lp, 2, GLP_FX, rprocs, rprocs);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        //cost per class
        glp_set_col_name(lp, n+1, colname);
        if ((class->maxprocs-class->nprocs) == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, (class->maxprocs-class->nprocs));
        glp_set_obj_coef(lp, n+1, class->cost);
        glp_set_col_kind(lp, n+1, GLP_IV);

        //mflops per class
        ia[(n*2)+1] = 1, ja[(n*2)+1] = n+1, ar[(n*2)+1] = class->mflops;
        ia[(n*2)+2] = 2, ja[(n*2)+2] = n+1, ar[(n*2)+2] = 1;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses*2, ia, ja, ar);

    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);

    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_get_procs_flops_spawn in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_get_procs_flops_remove'
*
****************************************************************************************************************************************/
void EMPI_lp_get_procs_flops_remove (int rflops, int rprocs, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_get_procs_flops_remove in <%s> ***\n", __FILE__);
    #endif

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: minimize cost
    glp_set_obj_dir(lp, GLP_MIN);

    glp_add_rows(lp, 2);

    glp_set_row_name(lp, 1, "MFLOPS");
    glp_set_row_bnds(lp, 1, GLP_FX, rflops, rflops);

    glp_set_row_name(lp, 2, "TOT_PROCS");
    glp_set_row_bnds(lp, 2, GLP_FX, rprocs, rprocs);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        //cost per class
        glp_set_col_name(lp, n+1, colname);
        if (class->nprocs == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, class->nprocs);
        glp_set_obj_coef(lp, n+1, class->cost);
        glp_set_col_kind(lp, n+1, GLP_IV);

        //mflops per class
        ia[(n*2)+1] = 1, ja[(n*2)+1] = n+1, ar[(n*2)+1] = class->mflops;
        ia[(n*2)+2] = 2, ja[(n*2)+2] = n+1, ar[(n*2)+2] = 1;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses*2, ia, ja, ar);

    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }
    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_get_procs_flops_remove in <%s> ***\n", __FILE__);
    #endif
}


/****************************************************************************************************************************************
*
*    'EMPI_lp_min_cost_fixed_eff'
*
****************************************************************************************************************************************/
void EMPI_lp_min_cost_fixed_eff (int rflops, int rprocs, int *mflops, int *newsize, int *cost, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_min_cost_fixed_eff in <%s> ***\n", __FILE__);
    #endif

    //NOTA: minimizar el coste con un techo de texec = incrementar procesos por exceso de tiempo de ejecucion.

    //Quiero sumar RFLOPS que tengan menor coste

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: minimize cost
    glp_set_obj_dir(lp, GLP_MIN);

    glp_add_rows(lp, 2);

    glp_set_row_name(lp, 1, "MFLOPS");
    glp_set_row_bnds(lp, 1, GLP_LO, rflops, 0.0);

    glp_set_row_name(lp, 2, "TOT_PROCS");
    glp_set_row_bnds(lp, 2, GLP_FX, rprocs, rprocs);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        //cost per class
        glp_set_col_name(lp, n+1, colname);
        if ((class->maxprocs-class->nprocs) == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, (class->nprocs));
        glp_set_obj_coef(lp, n+1, class->cost);
        glp_set_col_kind(lp, n+1, GLP_IV);

        //mflops per class
        ia[(n*2)+1] = 1, ja[(n*2)+1] = n+1, ar[(n*2)+1] = class->mflops;
        ia[(n*2)+2] = 2, ja[(n*2)+2] = n+1, ar[(n*2)+2] = 1;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses*2, ia, ja, ar);

    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    class = EMPI_GLOBAL_system_classes;
    //printf("There are %d host classes\n", EMPI_GLOBAL_nhclasses);
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }

    //set newsize and mflops
    *newsize = 0;
    *mflops = 0;
    *cost = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set newsize
        *newsize = *newsize + class_procs[n];
        //set mflops
        *mflops = *mflops + (class_procs[n] * class->mflops);
        //set cost
        *cost = *cost + (class_procs[n] * class->cost);
//printf ("class %i - nprocs %lfs\n", n, class_procs[n]);
        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_min_cost_fixed_eff in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_max_cost_fixed_eff'
*
****************************************************************************************************************************************/
void EMPI_lp_max_cost_fixed_eff (int rflops, int rprocs, int *mflops, int *newsize, int *cost, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_max_cost_fixed_eff in <%s> ***\n", __FILE__);
    #endif

    //NOTA: maximizar el coste con un techo de texec = remover procesos porque puedo tardar mas (texec) maximizando el ahorro en coste.

    //Quiero quitar RFLOPS que tengan mayor coste

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc (((EMPI_GLOBAL_nhclasses*2)+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: maximize cost
    glp_set_obj_dir(lp, GLP_MAX);

    glp_add_rows(lp, 2);

    glp_set_row_name(lp, 1, "MFLOPS");
    glp_set_row_bnds(lp, 1, GLP_UP, 0.0, rflops);

    glp_set_row_name(lp, 2, "TOT_PROCS");
    glp_set_row_bnds(lp, 2, GLP_FX, rprocs, rprocs);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        //cost per class
        glp_set_col_name(lp, n+1, colname);
        if (class->nprocs == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, class->maxprocs-class->nprocs);
        glp_set_obj_coef(lp, n+1, class->cost);
        glp_set_col_kind(lp, n+1, GLP_IV);

        //mflops per class
        ia[(n*2)+1] = 1, ja[(n*2)+1] = n+1, ar[(n*2)+1] = class->mflops;
        ia[(n*2)+2] = 2, ja[(n*2)+2] = n+1, ar[(n*2)+2] = 1;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses*2, ia, ja, ar);
    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }
    //set newsize and mflops
    *newsize = 0;
    *mflops = 0;
    *cost = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set newsize
        *newsize = *newsize + class_procs[n];
        //set mflops
        *mflops = *mflops + (class_procs[n] * class->mflops);
        //set cost
        *cost = *cost + (class_procs[n] * class->cost);

        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_max_cost_fixed_eff in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_mips_w_max'
*
****************************************************************************************************************************************/
void EMPI_lp_mips_w_max (int rprocs, int *mflops, int *cost, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_mips_w_max in <%s> ***\n", __FILE__);
    #endif

    //Devuelve max mflops con rprocs

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar, z;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: maximize mflops
    glp_set_obj_dir(lp, GLP_MAX);

    glp_add_rows(lp, 1);

    glp_set_row_name(lp, 1, "TOT_PROCS");
    glp_set_row_bnds(lp, 1, GLP_FX, rprocs, rprocs);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        //cost per class
        glp_set_col_name(lp, n+1, colname);
        if ((class->maxprocs-class->nprocs) == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, (class->maxprocs-class->nprocs));
        glp_set_obj_coef(lp, n+1, class->cost);
        glp_set_col_kind(lp, n+1, GLP_IV);

        //nprocs per class
        ia[n+1] = 1, ja[n+1] = n+1, ar[n+1] = 1;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses, ia, ja, ar);
    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    z = glp_mip_obj_val(lp);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }
    //set newsize and mflops
    *mflops = z;
    *cost = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        //set cost
        *cost = *cost + (class_procs[n] * class->cost);

        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_mips_w_max in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_lp_mips_w_min'
*
****************************************************************************************************************************************/
void EMPI_lp_mips_w_min (int rprocs, int *mflops, int *cost, double *class_procs) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::enter::EMPI_lp_mips_w_min in <%s> ***\n", __FILE__);
    #endif

    //Devuelve min mflops con rprocs

    glp_prob *lp;

    int *ia, *ja, n;

    double *ar, z;

    char colname[12];

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    lp = glp_create_prob();

    //memory allocation
    ia = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ia);
    ja = (int*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(int));
    assert (ja);
    ar = (double*) calloc ((EMPI_GLOBAL_nhclasses+1), sizeof(double));
    assert (ar);

    //glp params
    glp_smcp paramsm;
    glp_iocp paramio;

    glp_init_smcp (&paramsm);
    glp_init_iocp (&paramio);

    paramsm.msg_lev = GLP_MSG_OFF;
    paramio.msg_lev = GLP_MSG_OFF;

    glp_set_prob_name(lp, "sample");

    //Objective function: minimize mflops
    glp_set_obj_dir(lp, GLP_MIN);

    glp_add_rows(lp, 1);

    glp_set_row_name(lp, 1, "TOT_PROCS");
    glp_set_row_bnds(lp, 1, GLP_FX, rprocs, rprocs);

    glp_add_cols(lp, EMPI_GLOBAL_nhclasses);

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {

        //set colname
        sprintf (colname, "%s%i", "NPROCS_C", class->idclass);

        //cost per class
        glp_set_col_name(lp, n+1, colname);
        if (class->nprocs == 0)
            glp_set_col_bnds(lp, n+1, GLP_FX, 0.0, 0.0);
        else
            glp_set_col_bnds(lp, n+1, GLP_DB, 0.0, class->nprocs);
        glp_set_obj_coef(lp, n+1, class->cost);
        glp_set_col_kind(lp, n+1, GLP_IV);

        //nprocs per class
        ia[n+1] = 1, ja[n+1] = n+1, ar[n+1] = 1;

        //next class
        class = class->next;
    }

    glp_load_matrix(lp, EMPI_GLOBAL_nhclasses, ia, ja, ar);
    glp_simplex(lp, &paramsm);
    glp_intopt(lp, &paramio);

    z = glp_mip_obj_val(lp);

    //for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) class_procs[n] = glp_mip_col_val(lp, n+1);
    class = EMPI_GLOBAL_system_classes;
    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        class_procs[n] = glp_mip_col_val(lp, n+1);
    }
    //set newsize and mflops
    *mflops = z;
    *cost = 0;

    class = EMPI_GLOBAL_system_classes;

    for (n = 0; n < EMPI_GLOBAL_nhclasses; n ++) {
        //set cost
        *cost = *cost + (class_procs[n] * class->cost);

        //next class
        class = class->next;
    }

    glp_delete_prob(lp);

    free (ia);
    free (ja);
    free (ar);

    ia = ja = NULL;
    ar = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stdout, "\n*** DEBUG_MSG::exit::EMPI_lp_mips_w_min in <%s> ***\n", __FILE__);
    #endif
}