/****************************************************************************************************************************************
 *																																		*
 *	Research Project - Universidad Carlos III de Madrid [2009-2019]																		*
 *                                                                                                                                      *
 *	- EpiGraph: A Scalable Simulation Tool for Epidemiological Studies -																*
 *																																		*
 *	File:       auxiliar.c																											*
 *																																		*
 *	Authors:    Gonzalo Martin Cruz         <gmcruz@arcos.inf.uc3m.es>                                                                  *
 *																																		*
 *              David Exposito Singh        <desingh@arcos.inf.uc3m.es>																	*
 *                                                                                                                                      *
 *				Maria-Cristina Marinescu	<mcristina@arcos.inf.uc3m.es>																*
 *																																		*
 ****************************************************************************************************************************************/

/* #include */
#include "../epigraph_mpi.h"
#include "auxiliar.h"

/****************************************************************************************************************************************
 *
 *	'WhichProcess'
 *
 *	Utilidad: 
 *	 returns the process which belongs to a given person.
 *
 ****************************************************************************************************************************************/
int WhichProcess	( int		iContactIndex
					, int 		iNumberProcesses
					, int		*vcounts
					, int		*displs )
{
	int	n = 0
		,iProcess	= -1;

	for (n = 0; n < iNumberProcesses; n ++) {
	
		if ((iContactIndex >= displs[n])&&(iContactIndex < (displs[n]+vcounts[n]))) {
		
			iProcess = n;
		
			return (iProcess);	
		}
	}
	
	return (iProcess);
}

/****************************************************************************************************************************************
 *
 *	'SetProcess'
 *
 *	Utilidad: 
 *	 returns the process which belongs to a given person.
 *
 ****************************************************************************************************************************************/
int SetProcess	( int		iContactIndex
				, int		iNumberProcesses
				, long int	liPopSize )
{
	int	iPartition	= 0
		,iProcess	= 0;

	iPartition = liPopSize / iNumberProcesses;

	if (liPopSize % iNumberProcesses > 0)
		iPartition ++;

	iProcess = (iContactIndex / iPartition);

	if (((liPopSize % iNumberProcesses) > 0)&&(iProcess >= (liPopSize % iNumberProcesses)))
		iProcess = ((iContactIndex - (iPartition * (liPopSize % iNumberProcesses))) / (iPartition - 1)) + (liPopSize % iNumberProcesses);

	return (iProcess);
}


/****************************************************************************************************************************************
*
*	'CalcLimits'
*
*	Utility: 
*	 get the working limits of a process.
*
****************************************************************************************************************************************/
int CalcLimits	( int			iRank
				, int			iNumberProcesses
				, long int		iPopSize
				, long int* 	iLimitLow
				, long int* 	iLimitUp )
{
	double   dPSizeRank = 0;/* rank population size */

	long int liRest = 0;	/* rest */

	/* Calculate rank population (dPSizeRank) */
	dPSizeRank = floor (iPopSize / iNumberProcesses);
	liRest = iPopSize - ( dPSizeRank * iNumberProcesses );

	if (iRank < liRest ) { iPopSize ++; }
	
	/* Calculate lower and upper limits */
	if (iNumberProcesses > 1) {
		if (iRank > (liRest - 1)) {
			*iLimitLow = (liRest * (dPSizeRank + 1)) + ((iRank - liRest) * dPSizeRank);
			*iLimitUp = (*iLimitLow + dPSizeRank) - 1;	
		}
		else 	if (iRank == (liRest - 1)) {
				*iLimitLow = ((liRest-1) * (dPSizeRank+1));
				*iLimitUp = (*iLimitLow + dPSizeRank);
			}
		     	else if (iRank < (liRest - 1)) {
				*iLimitLow = (iRank * (dPSizeRank+1)); 
				*iLimitUp = (*iLimitLow + dPSizeRank);
			}
	}
	else if (iNumberProcesses == 1) {
		*iLimitLow = 0;
		*iLimitUp = dPSizeRank - 1;
	}

	return (0);
}

/****************************************************************************************************************************************
*
*	'Sort'
*
*	Utility: 
*	 Sort array.
*
****************************************************************************************************************************************/
int Sort	( long int**	liArrayUnordered
			, long int		liLeft
			, long int		liRight )
{
	long int i	= 0
			,j	= 0
			,element= 0
			,aux	= 0;

	element = (*liArrayUnordered)[(long int) ((liLeft + liRight)/2)];

	i = liLeft;
	j = liRight;

	do
	{
		while ( (*liArrayUnordered)[i] < element ) { i++; }
		while ( (*liArrayUnordered)[j] > element ) { j--; }

		if (i <= j)
		{
			aux = (*liArrayUnordered)[i];
			(*liArrayUnordered)[i] = (*liArrayUnordered)[j];
			(*liArrayUnordered)[j] = aux;
			i++;
			j--;
		}

	}while (i <= j);

	if (liLeft < j)	{ Sort (&*liArrayUnordered, liLeft, j);}
	if (i < liRight){ Sort (&*liArrayUnordered, i, liRight);}
	
	return (0);	
}

/****************************************************************************************************************************************
*
*	'SortPop'
*
*	Utility: 
*	 Sort array with index (max-min).
*
****************************************************************************************************************************************/
int SortPop	( int*		iArrayUnordered
			, int*		iIndex
			, long int	liLeft
			, long int	liRight )
{
	long int i	= 0
			,j	= 0
			,element= 0
			,aux	= 0;

	element = (iArrayUnordered)[(int) ((liLeft + liRight)/2)];

	i = liLeft;
	j = liRight;

	do
	{
		while ( (iArrayUnordered)[i] > element ) { i++; }
		while ( (iArrayUnordered)[j] < element ) { j--; }

		if (i <= j)
		{
			aux = (iArrayUnordered)[i];
			(iArrayUnordered)[i] = (iArrayUnordered)[j];
			(iArrayUnordered)[j] = aux;

			aux = (iIndex)[i];
			(iIndex)[i] = (iIndex)[j];
			(iIndex)[j] = aux;

			i++;
			j--;
		}

	} while (i <= j);

	if (liLeft < j)	{ SortPop (iArrayUnordered, iIndex, liLeft, j); }
	if (i < liRight){ SortPop (iArrayUnordered, iIndex, i, liRight); }
	
	return (0);	
}

/****************************************************************************************************************************************
*
*	'SortPopMinMax'
*
*	Utility: 
*	 Sort array with index (min-max).
*
****************************************************************************************************************************************/
int SortPopMinMax	( int*		iArrayUnordered
					, int*		iIndex
					, long int	liLeft
					, long int	liRight )
{
	long int i	= 0
			,j	= 0
			,element= 0
			,aux	= 0;

	element = (iArrayUnordered)[((liLeft + liRight)/2)];

	i = liLeft;
	j = liRight;

	do
	{
		while (((iArrayUnordered)[i] < element)&&(i < liRight)) { i++; }
		while (((iArrayUnordered)[j] > element)&&(j > liLeft)) 	{ j--; }

		if (i <= j)
		{
			aux = (iArrayUnordered)[i];
			(iArrayUnordered)[i] = (iArrayUnordered)[j];
			(iArrayUnordered)[j] = aux;

			aux = (iIndex)[i];
			(iIndex)[i] = (iIndex)[j];
			(iIndex)[j] = aux;

			i++;
			j--;
		}

	} while (i <= j);

	if (liLeft < j)	{ SortPopMinMax (iArrayUnordered, iIndex, liLeft, j); }
	if (i < liRight){ SortPopMinMax (iArrayUnordered, iIndex, i, liRight); }
	
	return (0);	
}

/****************************************************************************************************************************************
 *
 *	'StorePeriodsArrays'
 *
 *	Utilidad: 
 *	 store periods arrays.
 *
 ****************************************************************************************************************************************/
int StorePeriodsArrays	 ( int       iMyRank
                         , int       *iArrayLatentPrimaryPeriod
                         , int       *iArrayLatentSecondaryPeriod
                         , int       *iArrayInfectivePrimaryPeriod
                         , int       *iArrayInfectiveSecondaryPeriod
                         , int       *iArrayAntiviralPeriod
                         , int       *iArrayAsymptomaticPeriod
                         , int       *iArrayHospitalizationPeriod
                         , int       *iArrayAntiviralDelayPeriod
                         , long int  liLimitLow
                         , long int  liLimitUp
                         , char*     cPathV )
{    
	FILE 	*flFilePersonalInfo	= NULL;
    
	char	 cFileNamePersonalInfo	[128];
    
	long int i = 0;
    
	sprintf (cFileNamePersonalInfo, "%s%s%i%s", cPathV, "v_periods[", iMyRank, "].out");
    
	if ( (flFilePersonalInfo = fopen (cFileNamePersonalInfo, "w")) != NULL) {
        
		for (i = 0; i < (liLimitUp-liLimitLow+1); i ++)
			assert ((fprintf (flFilePersonalInfo, "%i %i %i %i %i %i %i %i\n", iArrayLatentPrimaryPeriod[i], iArrayLatentSecondaryPeriod[i],
                              iArrayInfectivePrimaryPeriod[i], iArrayInfectiveSecondaryPeriod[i], iArrayAntiviralPeriod[i], 
                              iArrayAsymptomaticPeriod[i], iArrayHospitalizationPeriod[i], iArrayAntiviralDelayPeriod[i])) != EOF);
        
		/* Closing file */
		fclose (flFilePersonalInfo);
        
	} else {
		printf ("\n**ERROR**: creating the file '%s'\n", cFileNamePersonalInfo);
		fflush (stdout);
		return (-1);	
	}
    
	return (0);
}

/****************************************************************************************************************************************
 *
 *	'LoadPeriodsArrays'
 *
 *	Utilidad: 
 *	 load periods arrays.
 *
 ****************************************************************************************************************************************/
int LoadPeriodsArrays	( int       iMyRank
						, int		iGraphProcs
                        , int       **iArrayLatentPrimaryPeriod
                        , int       **iArrayLatentSecondaryPeriod
                        , int       **iArrayInfectivePrimaryPeriod
                        , int       **iArrayInfectiveSecondaryPeriod
                        , int       **iArrayAntiviralPeriod
                        , int       **iArrayAsymptomaticPeriod
                        , int       **iArrayHospitalizationPeriod
                        , int       **iArrayAntiviralDelayPeriod
                        , long int  liLimitLow
                        , long int  liLimitUp
                        , long int	liPopSize
                        , char*		cPathV )
{
	FILE 	*flFilePersonalInfo	= NULL;
    
	char	 cFileNamePersonalInfo	[128];
	
	int		i			= 0
			,iIdProc	= 0;

	long int liPLimitLow= 0
			,liPLimitUp	= 0;
		
	for (iIdProc = 0; iIdProc < iGraphProcs; iIdProc ++) {
    
    	/* Calculate original process limits */
		CalcLimits (iIdProc, iGraphProcs, liPopSize, &liPLimitLow, &liPLimitUp);
		
		sprintf (cFileNamePersonalInfo, "%s%s%i%s", cPathV, "v_periods[", iIdProc, "].out");
		
		if ( (flFilePersonalInfo = fopen (cFileNamePersonalInfo, "r")) != NULL) {
		    
		    for (i = liPLimitLow; i <= liPLimitUp; i ++)
		    	assert ((fscanf (flFilePersonalInfo, "%i %i %i %i %i %i %i %i\n", &(*iArrayLatentPrimaryPeriod)[i], 
								 &(*iArrayLatentSecondaryPeriod)[i], &(*iArrayInfectivePrimaryPeriod)[i], 
								 &(*iArrayInfectiveSecondaryPeriod)[i], &(*iArrayAntiviralPeriod)[i], &(*iArrayAsymptomaticPeriod)[i], 
								 &(*iArrayHospitalizationPeriod)[i], &(*iArrayAntiviralDelayPeriod)[i])) != EOF);
		    
			/* Closing file */
			fclose (flFilePersonalInfo);
		    
		} else {
		
			printf ("\n**ERROR**: creating the file '%s'\n", cFileNamePersonalInfo);
			fflush (stdout);
			return (-1);	
		}
	}
    
	return (0);
}

/****************************************************************************************************************************************
*
*	'StorePeopleArrays'
*
*	Utilidad: 
*	 storage individuals personal information.
*
****************************************************************************************************************************************/
int StorePeopleArrays	( int		iMyRank
						, struct 	tPersonalInfo	*tpPersonalInfo
						, long int	liLimitLow
						, long int	liLimitUp
						, char*		cPathV )
{    
	FILE 	*flFilePersonalInfo	= NULL;

	char	 cFileNamePersonalInfo	[128];

	long int i = 0;

	sprintf (cFileNamePersonalInfo, "%s%s%i%s", cPathV, "v_personalinfo[", iMyRank, "].dat");

	if ( (flFilePersonalInfo = fopen (cFileNamePersonalInfo, "w")) != NULL) {

		/* Store data */
		for (i = liLimitLow; i <= liLimitUp; i ++)
			assert ((fprintf (flFilePersonalInfo, "%i %i %i %i %i %i\n", tpPersonalInfo[i-liLimitLow].age, tpPersonalInfo[i-liLimitLow].occupation, 
                              tpPersonalInfo[i-liLimitLow].gender, tpPersonalInfo[i-liLimitLow].prevdiseases, tpPersonalInfo[i-liLimitLow].race,
                              tpPersonalInfo[i-liLimitLow].prophylaxis)) != EOF);
        
		/* Closing file */
		fclose (flFilePersonalInfo);

	} else {
		printf ("\n**ERROR**: creating the file '%s'\n", cFileNamePersonalInfo);
		fflush (stdout);
		return (-1);	
	}

	return (0);
}

/****************************************************************************************************************************************
 *
 *	'LoadPeopleArrays'
 *
 *	Utilidad: 
 *	 load individuals personal information.
 *
 ****************************************************************************************************************************************/
int LoadPeopleArrays	( int		iNumberProcesses
						, int		iMyRank
						, struct 	tPersonalInfo	**tpPersonalInfo
                        , long int	liPopSize
                        , char*		cPathV )
{
	FILE 	*flFilePersonalInfo	= NULL;
    
	char	 cFileNamePersonalInfo	[128];
    
	int		i 		= 0
			,n 		= 0
			,iRank 	= 0;
	
	long int	liLimitLow	= 0
				,liLimitUp	= 0;
    
    for (n = 0, iRank = (iMyRank % iNumberProcesses) ; n < iNumberProcesses; n ++) {
    
    	CalcLimits (iRank, iNumberProcesses, liPopSize, &liLimitLow, &liLimitUp);
    
		sprintf (cFileNamePersonalInfo, "%s%s%i%s", cPathV, "v_personalinfo[", iRank, "].dat");
    
		if ( (flFilePersonalInfo = fopen (cFileNamePersonalInfo, "r")) != NULL) {
        
			/* Load data */
			for (i = liLimitLow; i <= liLimitUp; i ++)
				assert ((fscanf (flFilePersonalInfo, "%i %i %i %i %i %i\n", &(*tpPersonalInfo)[i].age, &(*tpPersonalInfo)[i].occupation, 
		                         &(*tpPersonalInfo)[i].gender, &(*tpPersonalInfo)[i].prevdiseases, &(*tpPersonalInfo)[i].race,
								 &(*tpPersonalInfo)[i].prophylaxis)) != EOF);
		    
			/* Closing file */
			fclose (flFilePersonalInfo);
		    
		} else {
		
			printf ("\n**ERROR**: opening the file '%s'\n", cFileNamePersonalInfo);
			fflush (stdout);
			
			return (-1);	
		}
				
		/* Next file */
		iRank ++;

		if (iRank == iNumberProcesses) iRank = MASTER;
	}
    
	return (0);
}

/****************************************************************************************************************************************
 *
 *	'StoreCondition'
 *
 *	Utilidad: 
 *	 store condition data.
 *
 ****************************************************************************************************************************************/
int StoreCondition	( int**		iArrayCondition
					, int		iMyRank
                    , int       iTimeMode
					, char*		cPathV
					, long int	liTime
					, long int	liLimitLow
					, long int	liLimitUp
					, long int	liPopSize )
{
	FILE 	*flFileCondition = NULL;
	
	char	cFileNameCondition[128];
	
	long int    i = 0;
	
    if (iTimeMode == 1)
        /* file_name_condition = 'v_conditionTime[rank]_stime.dat' */
        sprintf (cFileNameCondition, "%s%s%i%s%li%s", cPathV, "v_conditionTime[", iMyRank, "]_", liTime, ".dat");
    else
        /* file_name_condition = 'v_condition[rank]_stime.dat' */
        sprintf (cFileNameCondition, "%s%s%i%s%li%s", cPathV, "v_condition[", iMyRank, "]_", liTime, ".dat");

	/* Store data */
	if ( (flFileCondition = fopen (cFileNameCondition, "w")) != NULL) {
	
		for (i = liLimitLow; i <= liLimitUp; i ++)
			
			assert ((fprintf (flFileCondition, "%i ", (*iArrayCondition)[i-liLimitLow])) != EOF);
		
		/* Closing file */
		fclose (flFileCondition);
	
	} else {
	
		printf ("\n**ERROR**: creating the file '%s'\n", cFileNameCondition);
		exit (-1);	
	}

	return (0);	
}

/****************************************************************************************************************************************
 *
 *	'LoadCondition'
 *
 *	Utilidad: 
 *	 load condition data.
 *
 ****************************************************************************************************************************************/
int LoadCondition	( int**		iArrayCondition
                    , int		iMyRank
                    , int       iTimeMode
                    , char*		cPathV
                    , long int	liTime
                    , long int	liLimitLow
                    , long int	liLimitUp )
{
	FILE 	*flFileCondition = NULL;
	
	char	cFileNameCondition[128];
	
	long int    i       = 0
                ,iStart = 0
                ,iEnd   = 0;
	
    if (iTimeMode == 1)
    
        /* file_name_condition = 'v_conditionTime[rank]_stime.dat' */
        sprintf (cFileNameCondition, "%s%s%i%s%li%s", cPathV, "v_conditionTime[", iMyRank, "]_", liTime, ".dat");
        
    else
        /* file_name_condition = 'v_condition[rank]_stime.dat' */
        sprintf (cFileNameCondition, "%s%s%i%s%li%s", cPathV, "v_condition[", iMyRank, "]_", liTime, ".dat");
    
	iStart = 0;
	iEnd = (liLimitUp-liLimitLow+1);
	
	/* Load condition data */
	if ( (flFileCondition = fopen (cFileNameCondition, "r")) != NULL) {
        
		for (i = iStart; i < iEnd; i ++)
		
			assert ((fscanf (flFileCondition, "%i ", &(*iArrayCondition)[i])) != EOF);
		
		/* Closing file */
		fclose (flFileCondition);
        
	} else {
		printf ("\n**ERROR**: opening the file '%s'\n", cFileNameCondition);
		exit (-1);	
	}
    
	return (0);	
}

/****************************************************************************************************************************************
 *
 *	'StoreBedStatus'
 *
 *	Utilidad: 
 *	 store individual's bed status.
 *
 ****************************************************************************************************************************************/
int StoreBedStatus	( int		iMyRank
					, int**		iBedStatus
					, char*		cPathV
					, long int	liTime
					, long int	liLimitLow
					, long int	liLimitUp )
{
	FILE 	*flFileStatus 	= NULL;
	
	char	cFileNameStatus	[128];
	
	long int    i = 0;
	
    /* file_name_status = 'v_bedStatus[rank]_stime.dat' */
    sprintf (cFileNameStatus, "%s%s%i%s%li%s", cPathV, "v_bedStatus[", iMyRank, "]_", liTime, ".dat");

	/* Store data */
	if ( (flFileStatus = fopen (cFileNameStatus, "w")) != NULL) {

		for (i = 0; i < (liLimitUp-liLimitLow+1); i ++)
			assert ((fprintf (flFileStatus, "%i ", (*iBedStatus)[i])) != EOF);
		
		/* Closing file */
		fclose (flFileStatus);
	
	} else {
		printf ("\n**ERROR**: creating the file '%s'\n", cFileNameStatus);
		exit (-1);	
	}

	return (0);	
}

/****************************************************************************************************************************************
 *
 *	'LoadBedStatus'
 *
 *	Utilidad: 
 *	 load individual's bed status.
 *
 ****************************************************************************************************************************************/
int LoadBedStatus	( int		iMyRank
					, long int	liTime
					, int**		iBedStatus
					, char*		cPathV
					, long int	liLimitLow
					, long int	liLimitUp )
{
	FILE 	*flFileStatus 	= NULL;
	
	char	cFileNameStatus	[128];
	
	long int    i  = 0;
		
	/* Load bed status */
	sprintf (cFileNameStatus, "%s%s%i%s%li%s", cPathV, "v_bedStatus[", iMyRank, "]_", liTime, ".dat");
    
	if ( (flFileStatus = fopen (cFileNameStatus, "r")) != NULL) {
				
		/* Load data */
		for (i = 0; i < (liLimitUp-liLimitLow+1); i ++)
			assert ((fscanf (flFileStatus, "%i ", &(*iBedStatus)[i])) != EOF);
				
		/* Closing file */
		fclose (flFileStatus);
				
	} else {
		
		printf ("\n**ERROR**: opening the file '%s'\n", cFileNameStatus);
		fflush (stdout);
			
		return (-1);	
	}

	return (0);	
}

/****************************************************************************************************************************************
*
*	'LoadMatrix'
*
*	Utility: 
*	 Load CSC matrix.
*
****************************************************************************************************************************************/
int LoadMatrix 	( int 		iMyRank
				, int*		iGraphProcs
				, long int	liLimitLow
				, long int	liLimitUp
				, cs_int** 	csc_matrix
				, char*		cPathV
				, char*		cPathNFS
				, long int*	liPopSize )
{
	FILE	*flFileMatrix 	= NULL;	/* file matrix */

	char	cFileMatrixName	[128]	/* file matrix name */
			,cFileNameDegree[128];

	int 	i 				= 0
			,iIdMatrix		= 0
			,null			= 0
			,mindex			= 0
			,rowini			= 0
			,rowfin			= 0
			,rindex			= 0
			,vindex			= 0
			,m				= 0
			,n				= 0
			,nzmax			= 0
			,iniproc		= 0
			,finproc		= 0
			,*iPopDegree = NULL;
			
	long int liPLimitLow	= 0
			,liPLimitUp		= 0
			,offset			= 0;

	MPI_File 	flFileDegree;		/* file degree */

	MPI_Status 	status;
			
	MPI_Offset	fileOffset = liLimitLow * sizeof(int);

	/* Get iGraphProcs from csMatrix[0] */
    sprintf (cFileMatrixName, "%s%s", cPathV, "csMatrix0.dat");

	/* Load data */
	if ( (flFileMatrix = fopen (cFileMatrixName, "r")) != NULL)
	{
		/* Get 'm', 'n' and 'nnz' */
		assert ((fscanf (flFileMatrix, "%i %i %i %i\n", &m, &n, &nzmax, &(*iGraphProcs))) != EOF);
		
		*liPopSize = m;
					
		/* Closing file */
		fclose (flFileMatrix);

	} else {
	
		printf ("\n**ERROR**: opening the file '%s'\n", cFileMatrixName);
		fflush (stdout);
		
		MPI_Abort (MPI_COMM_WORLD, -1);
	}
	
	/* Set initial and finish processes */
	iniproc = SetProcess (liLimitLow, *iGraphProcs, *liPopSize);
	
	finproc = SetProcess (liLimitUp, *iGraphProcs, *liPopSize);
	
	/* Request memory for the array of individuals degree */
	assert ((iPopDegree	= (int*) calloc ((liLimitUp-liLimitLow+1), sizeof (int))) != NULL);
	
	/* Get degree from MPI_File */
	sprintf (cFileNameDegree, "%s%s", cPathV, "v_degree.dat");
	
	MPI_File_open (MPI_COMM_SELF, cFileNameDegree, MPI_MODE_RDONLY, MPI_INFO_NULL, &flFileDegree);
    
	MPI_File_set_view(flFileDegree, fileOffset, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
		
	MPI_File_read (flFileDegree, iPopDegree, (liLimitUp-liLimitLow+1), MPI_INT, &status);
		
	MPI_File_close(&flFileDegree);
	
	nzmax = 0;
	
	/* Get nzmax */
	for (i = 0; i < (liLimitUp-liLimitLow+1); i ++) nzmax += iPopDegree[i];
	
	/* Spalloc csc_matrix */
	//(*csc_matrix)= cs_spalloc_int_col (m, n, nzmax, nzmax, 0, (int)(liLimitUp-liLimitLow+1));
	(*csc_matrix)= cs_spalloc_int (m, n, nzmax, nzmax, 0);

	/* Set 'nz' */
	(*csc_matrix)->nz = -1;
	
	/* Set 'col' */
	for (i = 0; i < (liLimitLow+1); i ++)	((*csc_matrix)->p)[i] = 0;
	
	for (; i <= (liLimitUp+1); i ++) 		((*csc_matrix)->p)[i] = ((*csc_matrix)->p)[i-1] + iPopDegree[i-liLimitLow-1];
	
	for (; i <= *liPopSize; i ++) 	((*csc_matrix)->p)[i] = ((*csc_matrix)->p)[liLimitUp+1];
	
	/* Release memory */
	free (iPopDegree);
	iPopDegree = NULL;
	
	mindex = liLimitLow;

	/* Load data files */
	for (iIdMatrix = iniproc; iIdMatrix <= finproc; iIdMatrix ++) {
	
		/* Calculate original process limits */
		CalcLimits (iIdMatrix, *iGraphProcs, *liPopSize, &liPLimitLow, &liPLimitUp);

		/* Read data from mindex to offset */	
		offset = MIN (liLimitUp, liPLimitUp);

		sprintf (cFileMatrixName, "%s%s%d%s", cPathV, "csMatrix", iIdMatrix, ".dat");

		/* Load data */
		if ( (flFileMatrix = fopen (cFileMatrixName, "r")) != NULL)
		{
			/* Get 'm', 'n' and 'nnz' */
			assert ((fscanf (flFileMatrix, "%i %i %i %i\n", &m, &n, &nzmax, &null)) != EOF);

			/* Get 'col' */
			for (i = 0; i <= n; i ++) {
			
				assert (fscanf (flFileMatrix, "%i ", &null) != EOF);
				
				if (i == mindex) rowini = null;
				
				if (i == (offset+1)) rowfin = null;
			}

			/* Get 'row' */
			for (i = 0; i < rowini; i ++)
				assert (fscanf (flFileMatrix, "%i ", &null) != EOF);

			/* Get 'row' */
			for (i = rowini; i < rowfin; i ++) {
			
				assert ((fscanf (flFileMatrix, "%i ", &((*csc_matrix)->i)[rindex])) != EOF);
				
				rindex ++;
			}
			
			/* Get 'row' */
			for (i = rowfin; i < nzmax; i ++)
				assert (fscanf (flFileMatrix, "%i ", &null) != EOF);

			/* Get 'val' */
			for (i = 0; i < rowini; i ++)
				assert (fscanf (flFileMatrix, "%i ", &null) != EOF);
				
			/* Get 'val' */
			for (i = rowini; i < rowfin; i ++) {
			
				assert ((fscanf (flFileMatrix, "%i ", &((*csc_matrix)->x)[vindex])) != EOF);
				
				vindex ++;
			}
				
			/* Closing file */
			fclose (flFileMatrix);

		} else {
		
			printf ("\n**ERROR**: opening the file '%s'\n", cFileMatrixName);
			fflush (stdout);
		
			MPI_Abort (MPI_COMM_WORLD, -1);	
		}

		mindex = liPLimitUp + 1;	
	}
	
	return (0);
}

/****************************************************************************************************************************************
*
*	'LoadDegree'
*
*	Utility: 
*	 load degree file.
*
****************************************************************************************************************************************/
int LoadDegree	( int 		iMyRank
				, int**		iPopDegree
				, long int	liLimitLow
				, long int	liLimitUp
				, long int	liPopSize
				, char*		cPathNFS
				, cs_int**	csMatrix
				, double	dTimeini
				, FILE		*flLogfile )
{
	int		n = 0;
	
	char	cFileNameDegree [128];
	
	double	dTimenow 	= 0
			,dTimeStamp	= 0;
			
	MPI_Status 	status;

	MPI_File 	flFileDegree;
	
	MPI_Offset	fileOffset = liLimitLow * sizeof(int);
	
	sprintf (cFileNameDegree, "%s%s", cPathNFS, "v_degree.dat");
	
	/* Get degree from MPI_File */
	MPI_File_open (MPI_COMM_SELF, cFileNameDegree, MPI_MODE_RDONLY, MPI_INFO_NULL, &flFileDegree);
    
	MPI_File_set_view(flFileDegree, fileOffset, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
		
	MPI_File_read (flFileDegree, (*iPopDegree)+liLimitLow, (liLimitUp-liLimitLow+1), MPI_INT, &status);
		
	MPI_File_close(&flFileDegree);
	
	/* Check data */
	if ((*iPopDegree)[liLimitLow] != ((*csMatrix)->p[liLimitLow+1]-(*csMatrix)->p[liLimitLow])) {
	
		/* LOGFILE */
		dTimenow = MPI_Wtime();
		dTimeStamp = dTimenow - dTimeini;
		assert ((fprintf (flLogfile,"[%6.3f \t- ID_%i] +++ WARNING: Degree file %s NOT correctly loaded\n", dTimeStamp, iMyRank, cFileNameDegree))
		 !=EOF);
		fflush (flLogfile);
		
		/* memset to zeros */
		memset (*iPopDegree, 0, liPopSize*(sizeof(int)));
		
		for (n = liLimitLow; n <= liLimitUp; n ++)
			
				(*iPopDegree)[n] = ((*csMatrix)->p[n+1]-(*csMatrix)->p[n]);
	}

	return 0;
}

/****************************************************************************************************************************************
*
*	'StoreDegree'
*
*	Utility: 
*	 store degree file.
*
****************************************************************************************************************************************/
int StoreDegree 	( int		iMyRank
					, long int	liLimitLow
					, long int	liLimitUp
					, long int	liPopSize
					, cs_int**	csc_matrix
					, int		iMorningPeriod
					, int		iEveningPeriod
					, int		iNightPeriod
					, int		iSourceGraph
					, char*		cPathV
					, char*		cPathNFS )
{
	int 		iNumberM 			= 0
				,iNumberE 			= 0
				,iNumberN			= 0
				,iNumberElements	= 0
				,ncol				= 0
				,nrow				= 0
				,rc 				= 0
				,*iArrayDegree		= NULL
				,*iArrayDegreeInternal= NULL;

	double		dDegree				= 0
				,dDegreeAux			= 0
				,dDegreeSquare		= 0
				,dData				[3] = {0, 0, 0}
				,dDataAux			[3] = {0, 0, 0}
				,dDegreeStd			= 0;

	FILE		*flFileDegreeInfo		= NULL;

	MPI_File	flFileDegree
				,flFileDegreeInternal;
				
	MPI_Status	mpi_status;

	char		cFileNameDegree			[128]
				,cFileNameDegreeInternal[128]
				,cFileNameDegreeInfo	[128];
				
	MPI_Offset	fileOffset;

	/* Set file names */
	sprintf (cFileNameDegree, "%s%s", cPathNFS, "v_degree.dat");
	sprintf (cFileNameDegreeInternal, "%s%s", cPathNFS, "v_degree_internal.dat");

	/* Request memory for the array of degree */
	assert ((iArrayDegree = (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);

	if (iSourceGraph == SOCIALNETWORK)
		assert ((iArrayDegreeInternal = (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);

	for (ncol = liLimitLow; ncol <= liLimitUp; ncol ++) {

		/* Get the number of connctions */
		iNumberElements = (*csc_matrix)->p[ncol+1] - (*csc_matrix)->p[ncol];

        /* Initialize counters */
        iNumberM = 0;
        iNumberE = 0;
        iNumberN = 0;
        
		/* Set connections depending on the type of connection */
		for (nrow = (*csc_matrix)->p[ncol]; nrow < (*csc_matrix)->p[ncol+1]; nrow++) {
            
            switch ((*csc_matrix)->x[nrow]) {
            	case WORK:
                	iNumberM ++;
                    break;
               	case LEISURE:
                    iNumberE ++;
                   	break;
               	case FAMILIAR:
                    iNumberN ++;
                    break;
            }
		}

		/* <mean> */
		dDegreeStd += iNumberElements;

        /* Compute Mean Square Degree */
        if (iSourceGraph == SOCIALNETWORK) {

            /* <k> */
			dDegreeAux = (iMorningPeriod * iNumberM) + (iEveningPeriod * iNumberE) + (iNightPeriod * iNumberN);
            dDegreeAux = (double) (dDegreeAux / (iMorningPeriod+iEveningPeriod+iNightPeriod));
            dDegree += dDegreeAux;
           
			/* <k^2> */
            dDegreeSquare += pow ( dDegreeAux, 2 );

			/* Store dArrayDegree */
			iArrayDegree[ncol-liLimitLow] = iNumberElements;

			/* Store dArrayDegreeInternal */
			iArrayDegreeInternal[ncol-liLimitLow] = iNumberM;

       	 } else {

            /* <k> */
            dDegree += iNumberElements;

			/* <k^2> */
            dDegreeSquare += pow (iNumberElements, 2);

			/* Store dArrayDegree */
			iArrayDegree[ncol-liLimitLow] = iNumberElements;
        }
	}

	/* Compute collective <k>, <mean> and <k^2> */
	dDataAux[0] = dDegree;
	dDataAux[1] = dDegreeStd;
	dDataAux[2] = dDegreeSquare;
	
	MPI_Reduce (dDataAux, dData, 3, MPI_DOUBLE, MPI_SUM, MASTER, MPI_COMM_WORLD);

	dDegree = (double) (dData[0] / liPopSize);

	dDegreeStd = (double) (dData[1] / liPopSize);

	dDegreeSquare = (double) (dData[2] / liPopSize);

	if (iMyRank == MASTER) {
	
		/* Delete MPI files */
		rc = MPI_File_delete (cFileNameDegree, MPI_INFO_NULL);

		if (rc)
			printf ("\n\n  *** ERROR: Deleting MPI_File %s ***", cFileNameDegree);

		rc = MPI_File_delete (cFileNameDegreeInternal, MPI_INFO_NULL);

		if (rc)
			printf ("\n\n  *** ERROR: Deleting MPI_File %s ***", cFileNameDegreeInternal);

		fflush (stdout);
	
		/* Create Degree Info files */
		sprintf (cFileNameDegreeInfo, "%s%s", cPathV, "v_degreeInfo.dat");
		assert((flFileDegreeInfo = fopen (cFileNameDegreeInfo, "w")) != NULL);
		
		/* Storage Degree file */
		printf ("\n  <mean> = %lf - <k> = %lf - <k^2> = %lf\n", dDegreeStd, dDegree, dDegreeSquare);

		assert ((fprintf (flFileDegreeInfo, "%lf\n%lf\n%lf\n", dDegreeStd, dDegree, dDegreeSquare)) != EOF);
		
		fclose (flFileDegreeInfo);
	}

	/* Set file offset */
	fileOffset = liLimitLow * sizeof(int);

	MPI_Barrier (MPI_COMM_WORLD);

	/* Storage degree */
   	MPI_File_open (MPI_COMM_WORLD, cFileNameDegree, MPI_MODE_WRONLY|MPI_MODE_CREATE, MPI_INFO_NULL, &flFileDegree);
    
    MPI_File_set_view (flFileDegree, fileOffset, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
    
   	MPI_File_write_all (flFileDegree, iArrayDegree, (liLimitUp-liLimitLow+1), MPI_INT, &mpi_status);
   	
   	MPI_File_sync (flFileDegree);
   	
   	MPI_File_close (&flFileDegree); 
   	
    if (iSourceGraph == SOCIALNETWORK) {
    
	   	/* Set file offset */
		fileOffset = liLimitLow * sizeof(int);
	   	
	   	/* Storage degree internal */
		MPI_File_open (MPI_COMM_WORLD, cFileNameDegreeInternal, MPI_MODE_WRONLY|MPI_MODE_CREATE, MPI_INFO_NULL, &flFileDegreeInternal);
		
		MPI_File_set_view (flFileDegreeInternal, fileOffset, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
		
	   	MPI_File_write_all (flFileDegreeInternal, iArrayDegreeInternal, (liLimitUp-liLimitLow+1), MPI_INT, &mpi_status);
	   	
	   	MPI_File_sync (flFileDegreeInternal);
	   	
	   	MPI_File_close (&flFileDegreeInternal);
   	
		/* Release memory */
		free (iArrayDegreeInternal);
		iArrayDegreeInternal = NULL;
	}
	
	/* Release memory */
	free (iArrayDegree);
	iArrayDegree = NULL;
	
	return (0);
}

/****************************************************************************************************************************************
*
*	'StoreCSCMatrix'
*
*	Utility: 
*	 store matrix in CSC format.
*
****************************************************************************************************************************************/
int StoreCSCMatrix	( int 		iMyRank
					, int		iNumberProcesses
					, char*		cPathV
					, cs_int** 	csc_matrix )
{
	FILE 	*flMatrixCSC		= NULL;	/* storage file */

	char 	cMatrixFileName		[70];	/* file name */

	int 	i					= 0;	/* counter */

	/* file_name = 'csMatrix[my_rank].dat' */
	sprintf (cMatrixFileName, "%s%s%d%s", cPathV, "csMatrix", iMyRank, ".dat");

	/* Store data */
	if ( (flMatrixCSC = fopen (cMatrixFileName, "w")) != NULL) {
		/* Store m, n, nz */
		assert ((fprintf (flMatrixCSC, "%i %i %i %i\n", (*csc_matrix)->m, (*csc_matrix)->n, (*csc_matrix)->nzmax, iNumberProcesses)) != EOF);

		/* Store column pointers */
		for (i = 0; i <= (*csc_matrix)->m; i ++) {
			assert ((fprintf (flMatrixCSC, "%i", (*csc_matrix)->p[i])) != EOF);
			
			if (i < (*csc_matrix)->m)
				assert ((fprintf (flMatrixCSC, " ")) != EOF);
			else	if (i == (*csc_matrix)->m)
						assert ((fprintf (flMatrixCSC, "\n")) != EOF);
		}

		/* Store row indexes */
		for (i = 0; i < (*csc_matrix)->nzmax; i ++) {
			assert ((fprintf (flMatrixCSC, "%i", (*csc_matrix)->i[i])) != EOF);
			
			if (i < ((*csc_matrix)->nzmax-1))
				assert ((fprintf (flMatrixCSC, " ")) != EOF);
			else	if (i == ((*csc_matrix)->nzmax-1))
						assert ((fprintf (flMatrixCSC, "\n")) != EOF);
		}

		/* Store values */
		for (i = 0; i < (*csc_matrix)->nzmax; i ++) {
			assert ((fprintf (flMatrixCSC, "%i", (*csc_matrix)->x[i])) != EOF);
			
			if (i < ((*csc_matrix)->nzmax-1))
				assert ((fprintf (flMatrixCSC, " ")) != EOF);
		}

		/* Closing file */
		fclose (flMatrixCSC);

		return (0);
		
	}else {
		printf ("\n**ERROR**: opening the file '%s'\n", cMatrixFileName);
		return (-1);	
	}
}

/****************************************************************************************************************************************
 *
 *	'LoadVaccinationList'
 *
 *	Utilidad: 
 *	 load vaccination list.
 *
 ****************************************************************************************************************************************/
int LoadVaccinationList	( int 		iNumberIndividuals
						, int**		iVaccinatedList
						, char*		cPathV )
{
	FILE 	*flFileVacc 	= NULL;
	
	char	cFileNameVacc	[128];
	
	int    	i  = 0;
		
	/* Load vaccination list */
	sprintf (cFileNameVacc, "%s%s", cPathV, "listRandom");
    
	if ( (flFileVacc = fopen (cFileNameVacc, "r")) != NULL) {
				
		/* Load data */
		for (i = 0; i < iNumberIndividuals; i ++)
			assert ((fscanf (flFileVacc, "%i\n", &(*iVaccinatedList)[i])) != EOF);
				
		/* Closing file */
		fclose (flFileVacc);
				
	} else {
		
		printf ("\n**ERROR**: opening the file '%s'\n", cFileNameVacc);
		fflush (stdout);
			
		return (-1);	
	}

	return (0);	
}
