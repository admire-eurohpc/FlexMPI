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
 *    File:       memalloc.c                                                                                                                *
 *                                                                                                                                        *
 ****************************************************************************************************************************************/

/* include */
#include <empi.h>

/****************************************************************************************************************************************
*
*    'EMPI_malloc'
*
****************************************************************************************************************************************/
void *EMPI_malloc (size_t size) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_malloc [NOT EXIT] in <%s> ***\n", __FILE__);
    #endif

    return malloc (size);
}

/****************************************************************************************************************************************
*
*    'EMPI_calloc'
*
****************************************************************************************************************************************/
void *EMPI_calloc (size_t nmemb, size_t size) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_calloc [NOT EXIT] in <%s> ***\n", __FILE__);
    #endif

    return calloc (nmemb, size);
}

/****************************************************************************************************************************************
*
*    'EMPI_free'
*
****************************************************************************************************************************************/
void EMPI_free (void *addr, char *id) {

    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::enter::EMPI_free in <%s> ***\n", __FILE__);
    #endif

    EMPI_Data_type *data = NULL;
    
    data = EMPI_GLOBAL_Data;
    
    while (data != NULL) {

        /* CHANGE: begin */
        //if (strcmp(data->id, id)==0) {
        if ((data->id!=NULL)&&(strcmp(data->id, id)==0)) {
        /* CHANGE: end */

            if ((data->stype == EMPI_DENSE)||(data->stype == EMPI_VECTOR)) {
    
                //get addr
                addr = data->addr;
    
                if (addr != NULL) free (addr);
        
            } else if (data->stype == EMPI_SPARSE){
    
                if (data->addr_row != NULL) free (data->addr_row);
                if (data->addr_col != NULL) free (data->addr_col);
                if (data->addr_val != NULL) free (data->addr_val);
            }
        }
    
        data = data->next;        
    }
    
    //debug
    #if (EMPI_DBGMODE > EMPI_DBG_QUIET)
        fprintf (stderr, "\n*** DEBUG_MSG::exit::EMPI_free in <%s> ***\n", __FILE__);
    #endif
}
