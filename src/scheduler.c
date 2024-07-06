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
 *    File:       scheduler.c                                                                                                                *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

/* include */
#include <empi.h>

/****************************************************************************************************************************************
*
*    'EMPI_Set_host_perf'
*
****************************************************************************************************************************************/
void EMPI_Set_host_perf (EMPI_Monitor_type *monitor, int size) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Set_host_perf in <%s> ***\n", __FILE__);
    #endif

    //NOTE: el rendimiento del host es el real (medido por monitor), el de la clase es la media o propagado.

    int n, m, rank, nhosts, tmflops, nprocs;

    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);

    EMPI_Class_type *class = NULL;

    class = EMPI_GLOBAL_system_classes;

    EMPI_host_type *hostlist = NULL;

    hostlist = EMPI_GLOBAL_hostlist;

    if (rank == EMPI_root) {

        //Get host runtime performance data
        for (nprocs = 0, tmflops = 0, n = 0; n < EMPI_GLOBAL_nhosts; n ++, nprocs = 0, tmflops = 0) {

            for (m = 0; m < size; m ++) {

                if (monitor[m].hostid == n) {

                    tmflops += (int)(monitor[m].flops/monitor[m].rtime);

                    nprocs ++;
                }
            }

            if (nprocs > 0)
                //set host mflops
                hostlist->mflops = (int)(tmflops/nprocs);
            //else
                //hostlist->mflops = -1;

            //next host
            hostlist = hostlist->next;
        }

        //Get class runtime performance data
        for (nhosts = 0, tmflops = 0, n = 0; n < EMPI_GLOBAL_nhclasses; n ++, nhosts = 0, tmflops = 0) {

            hostlist = EMPI_GLOBAL_hostlist;

            while (hostlist != NULL) {

                if ((hostlist->idclass == n)&&(hostlist->nprocs > 0)) {

                    tmflops += hostlist->mflops;

                    nhosts ++;
                }

                //next host
                hostlist = hostlist->next;
            }

            if (nhosts > 0)
                //set class mflops
                class->mflops = (int)(tmflops/nhosts);
            //else
                //class->mflops = -1;

            //next class
            class = class->next;
        }
    }

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Set_host_perf in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Sched_spawn'
*
****************************************************************************************************************************************/
void EMPI_Sched_spawn (int *nhost) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Sched_spawn in <%s> ***\n", __FILE__);
    #endif

    int n = 0, freeslots = EMPI_FALSE, minprocs = -1;

    int *hnprocs = NULL, *hmaxprocs = NULL;

    //nprocs per host
    hnprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof (int));
    assert (hnprocs);
    //max procs per host
    hmaxprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof (int));
    assert (hmaxprocs);

    EMPI_host_type *hostlist = NULL;

    //pointer to global variable
    hostlist = EMPI_GLOBAL_hostlist;

    for (n = 0; n < EMPI_GLOBAL_nhosts; n ++) {

        //get nprocs, maxprocs and minprocs per host
        hnprocs[n] = hostlist->nprocs;
        hmaxprocs[n] = hostlist->maxprocs;

        //get free slots
        if (hostlist->nprocs < hostlist->maxprocs) freeslots = EMPI_TRUE;

        hostlist = hostlist->next;
    }

    if (freeslots == EMPI_FALSE) {

        //TODO: pensar si se permiten o no procesos una vez superados los maximos
        fprintf (stderr, "\nERROR: Error in EMPI_Sched_spawn no more processes are allowed to spawn - check cfile - default max 50 procs per host\n");

        MPI_Abort (EMPI_COMM_WORLD, -1);
    }

    if (EMPI_GLOBAL_spolicy == EMPI_AVAIL_NODE) {

        //choosing the host which, with free slots, has the minimum number of running processes
        for (n = 0; n < EMPI_GLOBAL_nhosts; n ++) {

            if ((hnprocs[n] < hmaxprocs[n])&&((hnprocs[n] < minprocs)||(minprocs == -1))) {

                *nhost = n;

                minprocs = *nhost;
            }
        }

    } else if (EMPI_GLOBAL_spolicy == EMPI_OCCUP_NODE) {

        //choosing the host which, with free slots and nprocs>0, has the minimum number of running processes
        for (n = 0; n < EMPI_GLOBAL_nhosts; n ++) {

            if ((hnprocs[n] < hmaxprocs[n])&&(hnprocs[n] > 0)&&((hnprocs[n] < minprocs)||(minprocs == -1))) {

                *nhost = n;

                minprocs = *nhost;
            }
        }
    }

    //release memory
    free (hnprocs);
    hnprocs = NULL;

    free (hmaxprocs);
    hmaxprocs = NULL;

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Sched_spawn in <%s> ***\n", __FILE__);
    #endif
}

/****************************************************************************************************************************************
*
*    'EMPI_Sched_remove'
*
****************************************************************************************************************************************/
void EMPI_Sched_remove (int *nrank, int *nhost) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_Sched_remove in <%s> ***\n", __FILE__);
    #endif

    int n = 0, rank, size, *hostid = NULL;

    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);

    if (rank == EMPI_root) {

        int maxprocs = -1;

        int *hnprocs = NULL, *hmaxprocs = NULL, *hiniprocs = NULL, diffprocs = 0;

        //nprocs per host
        hnprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof (int));
        assert (hnprocs);
        //max procs per host
        hmaxprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof (int));
        assert (hmaxprocs);
        //min procs per host
        hiniprocs = (int*) calloc (EMPI_GLOBAL_nhosts, sizeof (int));
        assert (hiniprocs);

        EMPI_host_type *hostlist = NULL;

        //pointer to global variable
        hostlist = EMPI_GLOBAL_hostlist;

        for (n = 0; n < EMPI_GLOBAL_nhosts; n ++) {

            //get nprocs, maxprocs, and iniprocs per host
            hnprocs[n] = hostlist->nprocs;
            hmaxprocs[n] = hostlist->maxprocs;
            hiniprocs[n] = hostlist->iniprocs;

            hostlist = hostlist->next;
        }

        //choosing the host with more processes than maxprocs
        for (n = 0; n < EMPI_GLOBAL_nhosts; n ++) {

            if ((hnprocs[n] > hmaxprocs[n])&&((hnprocs[n]-hmaxprocs[n]) > diffprocs)&&((hnprocs[n] > maxprocs)||(maxprocs == -1))) {

                *nhost = n;

                diffprocs = hnprocs[n]-hmaxprocs[n];

                maxprocs = *nhost;
            }
        }

        //choosing the host which has free slots and has the maximum number of running processes
        if (maxprocs == -1) {

            for (n = 0; n < EMPI_GLOBAL_nhosts; n ++) {

                if ((hnprocs[n] > hiniprocs[n])&&((hnprocs[n] > maxprocs)||(maxprocs == -1))) {

                    *nhost = n;

                    maxprocs = *nhost;
                }
            }
        }

        free (hnprocs);
        hnprocs = NULL;
        free (hmaxprocs);
        hmaxprocs = NULL;
        free (hiniprocs);
        hiniprocs = NULL;
    }

    hostid = (int*) calloc (size, sizeof(int));
    assert (hostid);

    PMPI_Gather (&EMPI_GLOBAL_hostid, 1, MPI_INT, hostid, 1, MPI_INT, EMPI_root, EMPI_COMM_WORLD);

    //choosing the rank of the process in host nhost
    if (rank == EMPI_root) {

        for (n = 0, *nrank = 0; n < size; n ++) {

            if ((hostid[n] == *nhost)&&(n >= EMPI_GLOBAL_minprocs)&&(n > *nrank)) *nrank = n;
        }
    }

    free (hostid);
    hostid = NULL;

    //root shares nrank with the rest of processes
    PMPI_Bcast (nrank, 1, MPI_INT, EMPI_root, EMPI_COMM_WORLD);

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_Sched_remove in <%s> ***\n", __FILE__);
    #endif
}
