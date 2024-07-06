/****************************************************************************************************************************************
*
*    Jacobi iterative method
*
****************************************************************************************************************************************/
#include <stdio.h>
#include <mpi.h> 
#include <empi.h>
#include <math.h>
#include "papi.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include "error.h"

MPI_File fh;

void jacobi (int dim, int *rank, int *size, double *A, double *b, double *x, double *x_old, int it, int itmax, double diff_limit, int cpu_i, int com_i, int io_i, int appd_id, char *argv[]);
void load_matrix (int dim, int despl, int count, double *A, double *b);
void generate_matrix (int dim, int count, double *A, double *b);

double tcomm_r = 0, tcomp_r = 0;


void diep(char *s)
{
  perror(s);
  exit(1);
}

//main
int main (int argc, char *argv[])
{
    int dim, rank, size, itmax, type, despl, count, it = 0;
    int cpu_i,com_i,io_i,app_id;

    double *A = NULL, *b = NULL, *x = NULL, *x_old = NULL, comp_ini, comp_fin, ldata_ini, ldata_fin;

    double diff_tol = 0.0;

    char mpi_name[128];

    int len,err;

    
    //MPI init
    err = MPI_Init(&argc, &argv);
       if (err == MPI_ERR_OTHER) return MPI_ERR_OTHER;
    
    MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
    MPI_Comm_size (EMPI_COMM_WORLD, &size);
    MPI_Get_processor_name (mpi_name, &len);
    
    if (argc < 8) {

    printf ("Jacobi usage: ./jacobi <dim> <itmax> <diff_tol> <cpu_intensity> <com_intensity> <IO_intensity> <application ID>\n");
        MPI_Abort (MPI_COMM_WORLD,1);
    }

    //get dim
    dim = atoi (argv[1]);

    //get itmax
    itmax = atoi (argv[2]);

    //get diff tolerance
    diff_tol = atof (argv[3]);

    //get CPU intensity
    cpu_i = atoi(argv[4]);
    if(cpu_i<1) cpu_i=1;
    
    //get communication intensity
    com_i = atoi(argv[5]);
    
    //get IO intensity
    io_i = atoi(argv[6]);

    //application id
    app_id = atoi(argv[7]);

    b = (double*) calloc (dim, sizeof (double));
    x = (double*) calloc (dim, sizeof (double));
    x_old = (double*) calloc (dim, sizeof (double));

    //get worksize
    EMPI_Get_wsize (rank, size, dim, &despl, &count, NULL, NULL);

    A = (double*) calloc (count * dim, sizeof (double));

    //register dense matrix 
    EMPI_Register_dense ("matrix", A, MPI_DOUBLE, dim, EMPI_DISJOINT);
    
    //register vector
    EMPI_Register_vector ("x_old", x_old, MPI_DOUBLE, dim, EMPI_SHARED);

    EMPI_Get_type (&type);

    if (type == EMPI_NATIVE) {

        MPI_Barrier (EMPI_COMM_WORLD);

        ldata_ini = MPI_Wtime ();

        // Generates the matrix
        generate_matrix (dim, count, A, b);
        
        ldata_fin = MPI_Wtime ();

        printf ("[%i] Process spawned in %s | Data loaded in %lf secs.\n", rank, mpi_name, ldata_fin-ldata_ini);

        MPI_Barrier (EMPI_COMM_WORLD);

    } else {

        ldata_ini = MPI_Wtime ();

        //get shared array
        EMPI_Get_shared (&it);

        ldata_fin = MPI_Wtime ();

        printf (" [%i] Process spawned in %s at %i | Data received in %lf secs.\n", rank, mpi_name, it, ldata_fin-ldata_ini);
    }



    comp_ini = MPI_Wtime ();

    printf (" [%d] Jacobi started \n", rank);
    if(rank==0) printf(" [%d] Configuration: \t nprocs: %d \t dim: %d \t itmax: %d \t diff_tol: %f \t cpu_intensity: %d  \t com_intensity: %d \t IO_intensity: %d \n\n",rank,size,dim,itmax,diff_tol,cpu_i,com_i,io_i);

    //jacobi
    jacobi (dim, &rank, &size, A, b, x, x_old, it, itmax, diff_tol, cpu_i, com_i, io_i, app_id, argv);

    comp_fin = MPI_Wtime ();

    if (rank == 0) printf (" [%i] Jacobi finished in %lf seconds - cost %f\n", rank, comp_fin-comp_ini, EMPI_GLOBAL_cum_cost);

    
    //free
    EMPI_free (A, "matrix");
    free (b);
    free (x);
    EMPI_free (x_old, "x_old");



    if (rank == 0) printf (" Terminating MPI \n");    
    
    //MPI finalize
    MPI_Finalize();


    if (rank == 0) printf (" Terminating Application \n");    

    exit(1);
}

//parallel jacobi
void jacobi (int dim, int *rank, int *size, double *A, double *b, double *x, double *x_old, int it, int itmax, double diff_limit, int cpu_i, int com_i, int io_i, int app_id, char *argv[])
{
    double tcomm = 0, tcomp = 0;
    
    double diff = 0, axi = 0, *x_new = NULL;
    double t1,t2;
    int i,n, m, k, desp, count, status, *displs = NULL, *vcounts = NULL, type;
    int err;
    
    char bin[1024]; 
    sprintf(bin,"%s",argv[0]);
    
    // ROMIO Variables
    MPI_Datatype MPI_BLOCK,MPI_INTERVAL;
    MPI_Offset disp;
    MPI_File     fh;
    char file[500],filename[500];
    
    // Path of the output file. 
    sprintf(filename,"/datafileN1_%d.out",app_id);
    strcpy(file, getenv("HOME")); // Write here the path to the parallel file system 
    strcat(file, filename);

        
    EMPI_Get_type (&type);
    
    //malloc vcounts and displs array
    displs = (int*) malloc (*size * sizeof(int));
    vcounts = (int*) malloc (*size * sizeof(int));
    
    x_new = (double*) calloc (dim, sizeof (double));

    //get worksize
    EMPI_Get_wsize (*rank, *size, dim, &desp, &count, vcounts, displs);
    
    t1=MPI_Wtime();
    for (; it < itmax; it ++) {
        
        //monitor init
        EMPI_Monitor_init ();
        
        // Matrix-vector product
        for(k=0;k<cpu_i;k++){
            for (n = 0; n < count; n ++) {
                axi = 0;
                for (m = 0; m < dim; m ++) { 
                    if ((n+desp) != m) axi += (A[(n*dim)+m] * x_old[m]);
                }
                x[n+desp] = (( b[n+desp] - axi ) / A[(n*dim)+(n+desp)]);
                // Detects out-of-range values
                if(isnan(x[n+desp]) || isinf(x[n+desp]) || fabs(x[n+desp])>1.0e+50) x[n+desp]=0;
            }
        }
        
        //Gathers x vector
        for(i=0;i<com_i;i++)     MPI_Allgatherv (x+desp, count, MPI_DOUBLE, x_new, vcounts, displs, MPI_DOUBLE, EMPI_COMM_WORLD);

        for (diff = 0, n = 0; n < dim; n ++) diff += fabs (x_new[n] - x_old[n]);
        //if (diff <= diff_limit) break;

        // Updates the vector
        memcpy (x_old, x_new, (dim*sizeof(double)));

        /// Performs I/O
        if(it%100==0 && it>0 && io_i>0){
                        
            if(*rank==0) printf(" Performing I/O at iteration %d out of %d\n",it,itmax);    
            if(*rank==0) MPI_File_delete(file, MPI_INFO_NULL);
     
            MPI_Type_contiguous(dim, MPI_DOUBLE,&MPI_BLOCK);
            MPI_Type_commit(&MPI_BLOCK);
            MPI_Type_contiguous(count*io_i, MPI_BLOCK,&MPI_INTERVAL);
            MPI_Type_commit(&MPI_INTERVAL);
                    
            err = MPI_File_open(EMPI_COMM_WORLD,file,MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &fh); 
            if (err != MPI_SUCCESS) diep("MPI_File_open");
            
            disp=(MPI_Offset)(((MPI_Offset)desp)*((MPI_Offset)dim)*((MPI_Offset)io_i)*sizeof(double));

            MPI_File_set_view(fh, disp,MPI_DOUBLE,MPI_INTERVAL,"native",MPI_INFO_NULL);     
            for (m=0;m<io_i;m++){        
                MPI_File_write_all(fh, A, count*dim, MPI_DOUBLE, MPI_STATUS_IGNORE);
            }
            
            MPI_File_close(&fh);
            MPI_Type_free(&MPI_INTERVAL);            
            MPI_Type_free(&MPI_BLOCK);
                        
        }
        
        
        //monitor end
        EMPI_Monitor_end (rank, size, it, itmax, &count, &desp, &vcounts, &displs, NULL, argv+1, bin);

        //get new array address
        A = EMPI_Get_addr("matrix");
        x_old = EMPI_Get_addr("x_old");

        EMPI_Get_status (&status);

        if (status == EMPI_REMOVED) break;

        
    }

    t2=MPI_Wtime();

    //get aggregated tcomp
    EMPI_Get_aggregated_tcomp (&tcomp);
    EMPI_Get_aggregated_tcomm (&tcomm);
    tcomp_r = tcomp;
    tcomm_r = tcomm;
    
    printf ("[%i] Jacobi finished in %i iterations, %f diff value - tcomp %lf tcomm %lf - overhead %lf lbalance %lf rdata %lf processes %lf reconfiguring %lf other %lf\n", *rank, it,diff, tcomp, tcomm, EMPI_GLOBAL_tover, EMPI_GLOBAL_overhead_lbalance, EMPI_GLOBAL_overhead_rdata, EMPI_GLOBAL_overhead_processes, EMPI_GLOBAL_overhead_rpolicy, (EMPI_GLOBAL_tover - EMPI_GLOBAL_overhead_lbalance - EMPI_GLOBAL_overhead_processes - EMPI_GLOBAL_overhead_rdata - EMPI_GLOBAL_overhead_rpolicy));
    printf("\n [%i] Total execution time: %f \n",*rank,t2-t1);
    
    sleep(5);

    free (displs);
    free (vcounts);

    free (x_new);
}

//load matrix
void load_matrix (int dim, int despl, int count, double *A, double *b) {

    int n = 0, m = 0, size_double = 4, size_char = 1;
    int err;
    FILE *flDense = NULL;

    flDense = fopen ("./matrices/densematrix.dat", "r");

    for (n = 0; n < dim; n ++) b[n] = (n % 5) + 1;

        fseek (flDense, (despl * dim * (size_double + size_char)), SEEK_SET);

    for (n = 0; n < count; n ++){
    
        for (m = 0; m < dim; m ++){
        
            err = fscanf (flDense, "%lf\n", &A[(n*dim)+m]);
            if(err == EOF){
                perror("Error fscanf\n");
            }
        }
        fclose (flDense);
    }
}


// Generate matrix's values
void generate_matrix (int dim, int count, double *A, double *b) {

    int n = 0, m = 0;

    for (n = 0; n < dim; n ++) b[n] = (n % 5) + 1;

    for (n = 0; n < count; n ++){
    
        for (m = 0; m < dim; m ++){
        
            A[(n*dim)+m]= 30-60*(rand() / RAND_MAX );  // Original random distribution
            //A[(n*dim)+m]=(double)((n*dim)+m); // For I/O debugging purposes
        }
    }
}




