#include <stdio.h>
#include <stdlib.h>
#include "controller.h"

void IO_scheduler(){
     int n, cnt;
     
     
     /*
     // This code prints several performance metrics
     printf("          E2:  Application CPU time (secs.): \t");
     for(n=0;n<GLOBAL_napps;n++) printf(" \t %5.2f ",cput[cnt_app[n]-1][n]);
     printf("\n          E2:  Application I/O time (secs.): \t");
     for(n=0;n<GLOBAL_napps;n++) printf(" \t %5.2f ",tlong[cnt_app[n]-1][n]);
     printf("\n          E2:  Application I/O data size (Bytes): ");
     for(n=0;n<GLOBAL_napps;n++) printf(" \t %.2E ",GLOBAL_IOsize[n]);
     */
     
     // Displays the current applications with the I/O blocked
     cnt=0;
     for(n=0;n<GLOBAL_napps;n++){
         if(GLOBAL_reqIO[n]==1)  cnt++;
     }
     
     if(GLOBAL_SCHEDULING_IO) printf("          E2:  I/O blocked applications: %d - ",cnt);
             
     for(n=0;n<GLOBAL_napps;n++){
         if(GLOBAL_reqIO[n]==1)      printf(" [ rank= %d ]",n);    
     }
     //printf("\n");

     // Alternative A: releases all the blocked applications and they will compete for the I/O access.
     if(0){
         for(n=0;n<GLOBAL_napps;n++){
            if(GLOBAL_reqIO[n]==1)      GLOBAL_reqIO[n]=2;
         }
     }

     // Alternative B: releases with the lowest ID.
     if(1){
         for(n=0;n<GLOBAL_napps;n++){
             if(GLOBAL_reqIO[n]==1){
                 GLOBAL_reqIO[n]=2;    
                 break;
             }
         }    
     }
     
 }
 