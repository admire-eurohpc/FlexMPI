/* include */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MAX_ARGS 40
#define MAX_ARG_LENGTH 80

char ** argv = NULL;
int argc = 0;

/*********** FORTRAN INTEGRATION ***************/
#ifdef __cplusplus
extern "C" {
#endif
    void fmpi_init_(int *err);
    void fmpi_buildArgv_(char *arg);
    void f_mpi_comm_c2f_(MPI_Fint *com);
#ifdef __cplusplus
}
#endif


void fmpi_init_(int  *err){
        argv[argc] = NULL; // C uses strings null-terminated
        /*fprintf(stderr, "fmpi_init: argc=%d\n", argc);
        int i = 0;
        while(argv[i] != NULL){
                fprintf(stderr, "Argv[%d]: %s\n", i, argv[i]);
                i++;
        }*/

        int res = MPI_Init(&argc, &argv);
        *err = 0;
}


void f_mpi_comm_c2f_(MPI_Fint *com){//MPI_Comm *comm) {
    *com = MPI_Comm_c2f(MPI_COMM_WORLD);
}


void fmpi_buildArgv_(char * arg){
    if (argc == 0){
        /*printf("Initializing interoperability\n");*/
        argv = (char**)malloc(MAX_ARGS * sizeof(char*));
        for (int i = 0; i < MAX_ARGS; i++)
            argv[i] = (char*)malloc(MAX_ARG_LENGTH * sizeof(char));
    }

    sprintf(argv[argc], "%s", arg);
    argc++;
}

//Headers for the new functions
