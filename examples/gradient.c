/****************************************************************************************************************************************
*
*	Conjugate_gradient iterative method
*
****************************************************************************************************************************************/

#include <stdio.h>
#include <mpi.h>
#include <empi.h>
#include <math.h>
#include "papi.h"

/* datatypes */
typedef struct spr {
	int		dim;
	int		nnz;
	int		*row;
	int		*col;
	double	*val;
} spr;

//function prototype
void cg (int *rank, int *size, spr *A, double *x, double *b, double *s, double *r, int it, int itmax, double diff_tol, char *argv[]);
void load_matrix (spr *A, int count, int desp, char *inputpath);
void load_vector (double *b, int dim);
double dot (double *x, double *y, int desp, int count);
void matvec (double *y, spr *A, double *x, int desp, int count, int *vcounts, int *displs);
void axpy (double *a, double x, double *p, double *y, int dim, int desp, int count, int *vcounts, int *displs);
void get (char *id, int **addr_row, int **addr_col, double **addr_val);

#ifdef MEASURE_ENERGY
double energy1 = 0;
double energy2 = 0;
double power1 = 0;
double power2 = 0;
#endif


//main
int main (int argc, char *argv[])
{
	int rank, size, itmax, dim, count, desp, type, it = 0;

	double tini, exect, diff_tol, ldata_ini, ldata_fin;

	double *b = NULL, *x = NULL, *s = NULL, *r = NULL;

	char mpi_name[128],inputpath[500];

	int len;

	spr *A = NULL;

	//Init
	MPI_Init (&argc, &argv);

	if (argc < 4) {

		printf ("Conjugate gradient usage: ./gradient <dim> <itmax> <diff_tol>\n");

		MPI_Abort (-1, EMPI_COMM_WORLD);
	}

	//get dim
	dim = atoi (argv[1]);

	//get itmax
	itmax = atoi (argv[2]);

	//get diff tolerance
	diff_tol = atof (argv[3]);

    // Matrix location path
	strcpy(inputpath,argv[4]);

    
	//csr format
	A = (spr*) malloc (sizeof (spr));
	//arrays
	b = (double*) calloc (dim, sizeof (double));
	x = (double*) calloc (dim, sizeof (double));
	s = (double*) calloc (dim, sizeof (double));
	r = (double*) calloc (dim, sizeof (double));

	//Get rank & size
	MPI_Comm_rank (EMPI_COMM_WORLD, &rank);
	MPI_Comm_size (EMPI_COMM_WORLD, &size);
	MPI_Get_processor_name (mpi_name, &len);

	//get worksize
	EMPI_Get_wsize (rank, size, dim, &desp, &count, NULL, NULL);

	//get type
	EMPI_Get_type (&type);

	//load matrix
	if (type == EMPI_NATIVE) {

		ldata_ini = MPI_Wtime ();

		load_matrix (A, count, desp, inputpath);

		ldata_fin = MPI_Wtime ();

		printf ("[%i] Process spawned in %s - Data loaded in %lf\n", rank, mpi_name, ldata_fin-ldata_ini);

		//register sparse matrix
		EMPI_Register_sparse ("matrix", A->row, A->col, A->val, MPI_DOUBLE, dim, A->nnz);

		//Register arrays
		EMPI_Register_vector ("x", x, MPI_DOUBLE, dim, EMPI_SHARED);
		EMPI_Register_vector ("s", s, MPI_DOUBLE, dim, EMPI_SHARED);
		EMPI_Register_vector ("r", r, MPI_DOUBLE, dim, EMPI_SHARED);

		MPI_Barrier (EMPI_COMM_WORLD);

	} else {

		//register sparse matrix
		EMPI_Register_sparse ("matrix", A->row, A->col, A->val, MPI_DOUBLE, dim, 0);

		//Register arrays
		EMPI_Register_vector ("x", x, MPI_DOUBLE, dim, EMPI_SHARED);
		EMPI_Register_vector ("s", s, MPI_DOUBLE, dim, EMPI_SHARED);
		EMPI_Register_vector ("r", r, MPI_DOUBLE, dim, EMPI_SHARED);

		ldata_ini = MPI_Wtime ();

		//get shared data and iteration
		EMPI_Get_shared (&it);

		ldata_fin = MPI_Wtime ();

		//Get addr sparse
		EMPI_Get_addr_sparse ("matrix", (void*)&A->row, (void*)&A->col, (void*)&A->val);

		A->dim = dim;

		printf ("[%i] Process spawned in %s at %i | Data received in %lf secs.\n", rank, mpi_name, it, ldata_fin-ldata_ini);
	}

	//load vector 'b'
	load_vector (b, A->dim);

	//get tinit
	tini = MPI_Wtime();

	printf ("[%i] Gradient started\n", rank);

	//conjugate gradient
	cg (&rank, &size, A, x, b, s, r, it, itmax, diff_tol, argv);

	//get exect
	exect = MPI_Wtime() - tini;

	if (rank == 0) printf ("[%i] Conjugate gradient finished in %.3lf seconds - cost %f\n", rank, exect, EMPI_GLOBAL_cum_cost);

	//Finalize
	MPI_Finalize();

	//free
	EMPI_free (A, "matrix");
	EMPI_free (x, "x");
	EMPI_free (s, "s");
	EMPI_free (r, "r");
	free (b);

	return 0;
}

//parallel conjugate gradient
void cg (int *rank, int *size, spr *A, double *x, double *b, double *s, double *r, int it, int itmax, double diff_tol, char *argv[]) {

	int desp, count, status, *displs = NULL, *vcounts = NULL, type;

	char *bin = argv[0];

	double rnorm2 = 0, rnorm2_old = 0, rho, alpha, *z = NULL, tcomm = 0, tcomp = 0;

	z = (double*) malloc (A->dim * sizeof (double));

	EMPI_Get_type (&type);

	//malloc vcounts and displs array
	displs = (int*) malloc (*size * sizeof(int));
	vcounts = (int*) malloc (*size * sizeof(int));

	//get worksize
	EMPI_Get_wsize (*rank, *size, A->dim, &desp, &count, vcounts, displs);
	for (; it < itmax; it ++) {

		//monitor init
		EMPI_Monitor_init ();
		if (it == 0) {
			memset (x, 0, (A->dim * sizeof (double)));

			//r = b - Ax
			//s = A * x
			matvec (s, A, x, desp, count, vcounts, displs);

			//r = -1 * s + b
			axpy (r, -1, s, b, A->dim, desp, count, vcounts, displs);

			//s = r
			memcpy (s, r, (A->dim * sizeof (double)));
			//z = zeros
			memset (z, 0, (A->dim * sizeof (double)));

			//r * r
			rnorm2_old = dot (r, r, desp, count);
			
		}
		

		//z = A * s
		matvec (z, A, s, desp, count, vcounts, displs);

		//alpha = rnorm2_old / s * z
		alpha = rnorm2_old / dot (s, z, desp, count);

		//x = alpha * s + x
		axpy (x, alpha, s, x, A->dim, desp, count, vcounts, displs);

		//r = -alpha * z + r
		axpy (r, -alpha, z, r, A->dim, desp, count, vcounts, displs);

		//r * r
		rnorm2 = dot (r, r, desp, count);

		//rho
		rho = rnorm2 / rnorm2_old;

		//s = rho * s + r
		axpy (s, rho, s, r, A->dim, desp, count, vcounts, displs);

		rnorm2_old = rnorm2;

		//printf ("[%i - it %i] Calculation desp %i count %i - ||rnorm2|| %lf\n", *rank, it, desp, count, sqrt(rnorm2));

		if (sqrt(rnorm2) <= diff_tol) break;

		//monitor end
		EMPI_Monitor_end (rank, size, it, itmax, &count, &desp, &vcounts, &displs, NULL, argv+1, bin);

		//get new addresses
		EMPI_Get_addr_sparse ("matrix", (void*)&A->row, (void*)&A->col, (void*)&A->val);

		s = EMPI_Get_addr("s");
		r = EMPI_Get_addr("r");
		x = EMPI_Get_addr("x");

		EMPI_Get_status (&status);

		if (status == EMPI_REMOVED) break;
	}

	//get aggregated tcomp
	EMPI_Get_aggregated_tcomp (&tcomp);
	EMPI_Get_aggregated_tcomm (&tcomm);

	printf ("[%i] Conjugate Gradient finished in %i iterations - tcomp %lf tcomm %lf - overhead %lf lbalance %lf rdata %lf processes %lf reconfiguring %lf other %lf\n", *rank, it, tcomp, tcomm, EMPI_GLOBAL_tover, EMPI_GLOBAL_overhead_lbalance, EMPI_GLOBAL_overhead_rdata, EMPI_GLOBAL_overhead_processes, EMPI_GLOBAL_overhead_rpolicy, (EMPI_GLOBAL_tover - EMPI_GLOBAL_overhead_lbalance - EMPI_GLOBAL_overhead_processes - EMPI_GLOBAL_overhead_rdata - EMPI_GLOBAL_overhead_rpolicy));

	free (displs);
	free (vcounts);
	free (z);
}

//parallet dot
double dot (double *x, double *y, int desp, int count) {

	double sum = 0, gSum = 0;

	int n;

	for (n = 0; n < count; n ++) sum += (x[n+desp] * y[n+desp]);

	MPI_Allreduce (&sum, &gSum, 1, MPI_DOUBLE, MPI_SUM, EMPI_COMM_WORLD);

	return gSum;
}

//parallel matrix vector
void matvec (double *y, spr *A, double *x, int desp, int count, int *vcounts, int *displs) {

	int n, m;
	double *xrecv;
	xrecv = (double*) malloc (A->dim * sizeof (double));

	//combinar x
	MPI_Allgatherv (x+desp, count, MPI_DOUBLE, xrecv, vcounts, displs, MPI_DOUBLE, EMPI_COMM_WORLD);

	//y = A * x
	//multiplicacion parcial
	for (n = 0; n < count; n ++) {

		y[n+desp] = 0;

		//recorrer matriz sparse
		for (m = A->row[n]; m < A->row[n+1]; m ++) {

			y[n+desp] += (A->val[m] * xrecv[A->col[m]]);
		}
	}
	free (xrecv);
}

//parallel daxpy
void axpy (double *a, double x, double *p, double *y, int dim, int desp, int count, int *vcounts, int *displs) {

	int n;

	for (n = 0; n < count; n ++) a[n+desp] = (x * p[n+desp]) + y[n+desp];
}

//load matrix
void load_matrix (spr *A, int count, int desp, char *inputpath) {

	int n, cnnz = 0, inull, colstart = 0, colfin, err;
    char path[1000];
	double dnull;

	FILE *flCSR = NULL;
    strcpy(path,inputpath);
    strcat(path,"/matrices/nd6k.dat");
    
	flCSR = fopen (path, "r");
	//load matrix A
	err = fscanf (flCSR, "%i %i\n", &A->dim, &A->nnz);
    if(err == EOF){
         perror("Error fscanf\n");
    }

	A->row = (int*) malloc ((count + 1) * sizeof (int));

	//read row
	for (n = 0; n < desp; n ++) {
		err = fscanf (flCSR, "%d ", &inull);
        if(err == EOF){
            perror("Error fscanf\n");
        }		
	}
	for (; n <= (desp+count); n ++) {

		err = fscanf (flCSR, "%d ", &A->row[n-desp]);
        if(err == EOF){
            perror("Error fscanf\n");
        }

		if (n > desp) cnnz += (A->row[n-desp] - A->row[n-desp-1]);

		else colstart = A->row[n-desp];
	}

	colfin = A->row[count];

	for (n = 1; n <= count; n ++) A->row[n] = (A->row[n] - A->row[0]); 
	A->row[0] = 0;

	for (n = (desp+count+1); n <= A->dim; n ++){
		err = fscanf (flCSR, "%d ", &inull);
        if(err == EOF){
            perror("Error fscanf\n");
        }		
	}
	//malloc
	A->col = (int*) malloc (cnnz * sizeof (int));
	A->val = (double*) malloc (cnnz * sizeof (double));

	//read col
	for (n = 0; n < colstart; n ++){
		err = fscanf (flCSR, "%d ", &inull);
        if(err == EOF){
            perror("Error fscanf\n");
        }			
	}
	for (; n < colfin; n ++){
		err = fscanf (flCSR, "%d ", &A->col[n-colstart]);
        if(err == EOF){
            perror("Error fscanf\n");
        }
	}
	for (; n < A->nnz; n ++){
		err = fscanf (flCSR, "%d ", &inull);
        if(err == EOF){
            perror("Error fscanf\n");
        }
	}
	//read val
	for (n = 0; n < colstart; n ++){
		err = fscanf (flCSR, "%lf ", &dnull);
        if(err == EOF){
            perror("Error fscanf\n");
        }
	}
	for (; n < colfin; n ++){
		err = fscanf (flCSR, "%lf ", &A->val[n-colstart]);
        if(err == EOF){
            perror("Error fscanf\n");
        }
	}
	fclose (flCSR);
}

//load vector
void load_vector (double *b, int dim) {

	int n;

	//load vector b
	for (n = 0; n < dim; n ++) b[n] = (n%14)+1;
}
