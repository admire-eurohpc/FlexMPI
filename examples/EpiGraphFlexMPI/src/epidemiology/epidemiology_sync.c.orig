/****************************************************************************************************************************************
 *																																		*
 *	Research Project - Universidad Carlos III de Madrid [2009-2019]																		*
 *                                                                                                                                      *
 *	- EpiGraph: A Scalable Simulation Tool for Epidemiological Studies -																*
 *																																		*
 *	File:       epidemiology_sync.c																										*
 *																																		*
 *	Authors:    Gonzalo Martin Cruz         <gmcruz@arcos.inf.uc3m.es>                                                                  *
 *																																		*
 *              David Exposito Singh        <desingh@arcos.inf.uc3m.es>																	*
 *                                                                                                                                      *
 *				Maria-Cristina Marinescu	<mcristina@arcos.inf.uc3m.es>																*
 *																																		*
 ****************************************************************************************************************************************/

/* #include */
#include "epidemiology.h"
#include "epidemiology_sync.h"

/****************************************************************************************************************************************
*
*	'SpreadSync'
*
*	Utilidad:
*	 simulate the spreading of the epidemic.
*
****************************************************************************************************************************************/
int SpreadSync	( int		iMyRank
				, int*		iArrayCondition
                , int*      iArrayConditionTime
				, int		iSpreadingTime
				, int		iHour
				, int		iMin
				, int 		iDay
				, int 		iMonth
				, int 		iWeekDay
				, int*		iCalendar
				, int		iCommunicationTime
				, int		iNumberProcesses
				, int		iUpdatingTime
				, int		iStatisticsCollectionTime
				, int**		iArrayLatentPrimaryPeriod
				, int**		iArrayLatentSecondaryPeriod
				, int**		iArrayInfectivePrimaryPeriod
				, int**		iArrayInfectiveSecondaryPeriod
				, int**		iArrayAntiviralPeriod
				, int**		iArrayAsymptomaticPeriod
				, int**		iArrayHospitalizationPeriod
            	, int**     iArrayAntiviralDelayPeriod
				, int		iVaccinationDay
				, int		iVaccinationPeriodLength
				, int 		iVaccinationList
            	, int      	iSourceGraph
                , int       iSetTimeStamp
				, int		iBedTime
				, int*		iBedStatus
				, int* 		iArrayInfectedPeople
				, int		iGraphProcs
				, long int	liPopSize
				, long int 	liCheckpoint
				, long int	liLimitLow
				, long int	liLimitUp
				, long int	liSimTime
				, cs_int*	csc_matrix
				, struct 	tPersonalInfo	**tpPersonalInfo
				, double	dTimeini
				, FILE		*flLogfile
				, char*		cPathV
				, char*		cPathStructure
				, char*		cPathNFS
				, char*		cPathXML
				, double*	dAvgTempMonth
				, double*	dAvgHumdMonth
				, double 	dBedStudents
				, double 	dBedElderly
				, double 	dBedAdults
				, double	dVaccinationPercentage
				, double	dVaccinationSusceptibleEffectiveness
				, double	dLatentPrimaryToSecondary
				, double	dLatentPrimaryToAsymptomatic
				, double	dInfectiveToRemoved
				, double	dInfectiveToHospitalized
				, double	dInfectiveToDead
				, double	dAsymptomaticToRemove
				, double	dHospitalizedToRemoved
				, double	dHospitalizedToDead
            	, double*   dAntiviralArray
            	, double*	dRiskArray
				, double	dMuGraph
            	, double    dVaccinationLower64
            	, double    dVaccinationGreater64
            	, double	dClosingSchools
            	, double	dSocialDistancing
            	, char 		*argv[] )
{

	int		iCountDay				= 1
			,count					= 0
			,desp					= 0
			,prevNumberProcesses	= 0
			,data				[6] = {0, 0, 0, 0, 0, 0}
			,ptype 					= -1
			,*iCost					= NULL
			,*displs				= NULL
			,*vcounts				= NULL
			,**iArrayPeriod			= NULL
			,**iArrayNewInfectives	= NULL
			,*iArrayNumNewInfectives= NULL
            ,*iArrayRecvNumInfectives = NULL
            ,*iArraySizeNumInfectives = NULL
            ,*iVaccinatedList		= NULL
			,*iCommMatrix			= NULL
			,iActiveClosingSchools	= FALSE
			,iActiveSocialDistancing= FALSE
			,iFinalStats		[17]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
			,iArrayStatProcess	[17]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
			,iTestSpread		[17]= {0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0}
			,iTestUpdate		[17]= {0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0}
			,iNumberVaccinated		= 0
			,iOffsetInfectives		= 0
			,status					= 0;

	long int liTimeStamp			= 0
			,i						= 0;

	double	dTimeStamp				= 0
			,dInfectedRate			= 0
			,dPartInfectedRate		= 0
			,dTimenow				= 0
			,dLogTimeStamp			= 0
			,tini 					= 0
            ,dDegree        		= 0		/* <k> */
			,dDegreeStd				= 0		/* <mean> */
			,dDegreeSquare			= 0;	/* <k^2> */

	FILE	*flFileDegreeInfo		= NULL	/* degree info file */
			,*flFileStatsProcess	= NULL;	/* stats per process file */

	char	cFileNameDegreeInfo		[128]
			,cFileNameStatsProcess	[128]
			,*bin = argv[0];

	/* GSL random number generator */
	const	gsl_rng_type 	* type 	= gsl_rng_ranlxs2;
			gsl_rng     	* rng	= gsl_rng_alloc(type);

	/* System seed */
    gsl_rng_set (rng, 797);

	/* Opening files */
	sprintf (cFileNameDegreeInfo, 		"%s%s", 	cPathV, 		"v_degreeInfo.dat");
	sprintf (cFileNameStatsProcess,		"%s%s%d%s", cPathV, 		"v_stats[", iMyRank, "].out");

	assert	((flFileDegreeInfo			= fopen (cFileNameDegreeInfo,		"r"))	!= NULL);
	assert	((flFileStatsProcess		= fopen (cFileNameStatsProcess,		"w"))	!= NULL);

	//get type
	EMPI_Get_type (&ptype);

	/* MEMORY REQUESTS AND VARIABLES INITIALIZATION */

	displs = (int*) malloc (iNumberProcesses * sizeof(int));
	vcounts = (int*) malloc (iNumberProcesses * sizeof(int));

	EMPI_Get_wsize (iMyRank, iNumberProcesses, (int)(liPopSize), &desp, &count, vcounts, displs);

	/* Request memory for the array of number of infected individuals from another processes */
	assert ((iArrayNumNewInfectives = (int*) 	calloc (iNumberProcesses, sizeof (int))) != NULL);

	/* Request memory for the array of number of intected individuals to receive from another processes */
	assert ((iArrayRecvNumInfectives= (int*) 	calloc (iNumberProcesses, sizeof (int))) != NULL);

	/* Request memory for the array of number of infected individuals from another processes */
	assert ((iArraySizeNumInfectives= (int*) 	calloc (iNumberProcesses, sizeof (int))) != NULL);

	/* Request memory for the array of infected individuals from another process */
	assert ((iArrayNewInfectives 	= (int**) 	malloc (iNumberProcesses * sizeof (int*))) != NULL);

	iOffsetInfectives = BUFSIZ * 0.3 * ( (liPopSize/iNumberProcesses) % BUFSIZ);

	for (i = 0; i < iNumberProcesses; i ++) {
		/* Initialize the array */
    	iArraySizeNumInfectives[i] = iOffsetInfectives;

		assert ((iArrayNewInfectives[i] = (int*) calloc (iOffsetInfectives, sizeof (int))) != NULL);
	}

	/* Request memory for the pointers to the period arrays */
	assert ((iArrayPeriod	  		= (int**) 	malloc (SGSIZE *sizeof (int*))) != NULL);

	/* Set pointers */
	iArrayPeriod [LATENT_PRIMARY] 				= *iArrayLatentPrimaryPeriod;
	iArrayPeriod [LATENT_PRIMARY_TREATED] 		= *iArrayLatentPrimaryPeriod;
	iArrayPeriod [LATENT_SECONDARY] 			= *iArrayLatentSecondaryPeriod;
	iArrayPeriod [LATENT_SECONDARY_TREATED] 	= *iArrayLatentSecondaryPeriod;
	iArrayPeriod [INFECTIVE_PRIMARY] 			= *iArrayInfectivePrimaryPeriod;
	iArrayPeriod [INFECTIVE_PRIMARY_TREATED] 	= *iArrayInfectivePrimaryPeriod;
	iArrayPeriod [INFECTIVE_SECONDARY] 			= *iArrayInfectiveSecondaryPeriod;
	iArrayPeriod [INFECTIVE_SECONDARY_TREATED] 	= *iArrayInfectiveSecondaryPeriod;
	iArrayPeriod [INFECTIVE_ANTIVIRAL] 			= *iArrayAntiviralPeriod;
	iArrayPeriod [ASYMPTOMATIC] 				= *iArrayAsymptomaticPeriod;
	iArrayPeriod [ASYMPTOMATIC_TREATED] 		= *iArrayAsymptomaticPeriod;
	iArrayPeriod [HOSPITALIZED] 				= *iArrayHospitalizationPeriod;
	iArrayPeriod [HOSPITALIZED_TREATED] 		= *iArrayHospitalizationPeriod;

	/* Array para la funcion de coste */
	assert ((iCost = (int*) malloc (count * sizeof (int))) != NULL);

	/* Vaccination list */
	if (iVaccinationList != NOVACC) {

		iNumberVaccinated = (dVaccinationPercentage * liPopSize);

		assert ((iVaccinatedList = (int*) malloc (iNumberVaccinated * sizeof (int))) != NULL);

		/* Loading list of vaccinated individuals */
		LoadVaccinationList (iNumberVaccinated, &iVaccinatedList, cPathV);
	}

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile,"[%6.3f \t- ID_%i] Set communications matrix\n", dTimeStamp, iMyRank))!=EOF);
	fflush (flLogfile);

	SetCommMatrix (iMyRank, iNumberProcesses, &iCommMatrix);

	/* Get Degree Standard, Degree & Square Degree */
	assert ((fscanf (flFileDegreeInfo, "%lf\n%lf\n%lf\n", &dDegreeStd, &dDegree, &dDegreeSquare)) != EOF);
	fclose (flFileDegreeInfo);

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile,"[%6.3f \t- ID_%i] Degree file - Params: Std %lf, Degree %lf, Square %lf\n", dTimeStamp, iMyRank
	, dDegreeStd, dDegree, dDegreeSquare))!=EOF);
	fflush (flLogfile);

	/* LOGFILE - MEMORY USAGE */
	dLogTimeStamp = MPI_Wtime();
	dTimeStamp = dLogTimeStamp - dTimeini;
	assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] Memory usage at %li secs. is %.3lf MB\n", dTimeStamp, iMyRank, liTimeStamp
	, (double)mem_total()/1024)) != EOF);
	fflush (flLogfile);

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile,"[%6.3f \t- ID_%i] Spread simulation begins...\n",dTimeStamp,iMyRank))!=EOF);
	fflush (flLogfile);

	if (ptype == EMPI_NATIVE) MPI_Barrier (EMPI_COMM_WORLD);

	tini = MPI_Wtime();

	/* TRANSMISSION ALGORITHM */
	for (liTimeStamp = iSetTimeStamp; liTimeStamp <= liSimTime; liTimeStamp ++) {
       
		//monitor init
		EMPI_Monitor_init ();

		/* CONCURRENT COMMUNICATIONS OF INFECTED INDIVIDUALS */
		if ((iCommunicationTime > 0)&&((liTimeStamp % iCommunicationTime) == 0))

			CommInfected (iMyRank, iNumberProcesses, iArrayNumNewInfectives, iArrayRecvNumInfectives, iArrayCondition
			, iArrayNewInfectives, iArrayInfectedPeople, iCommMatrix, liLimitLow, liLimitUp);

		/* SIMULATION OF THE SPREADING */
		if ((iSpreadingTime > 0)&&((liTimeStamp % iSpreadingTime) == 0))

			SimSpreading ( iMyRank, iTestSpread, iArrayCondition, iArrayPeriod, iMonth, iHour, iCalendar[((iMonth-1)*30)+iDay], iSourceGraph, iBedStatus
			, iSpreadingTime, iArraySizeNumInfectives, iArrayNumNewInfectives, iArrayNewInfectives, iArrayInfectedPeople, iNumberProcesses
			, iActiveClosingSchools, iActiveSocialDistancing, vcounts, displs, liPopSize, liLimitLow, liLimitUp, csc_matrix, tpPersonalInfo, rng, dAvgTempMonth, dAvgHumdMonth
			, dRiskArray, dDegree, dDegreeSquare, dTimeini, flLogfile );

		/* UPDATING STATES GRAPH */
		if ((iUpdatingTime > 0) && ((liTimeStamp % iUpdatingTime) == 0))

			UpdatingStates (iTestUpdate, iArrayCondition, iArrayConditionTime, iUpdatingTime, iArrayPeriod, iArrayAntiviralDelayPeriod, iBedTime, iBedStatus
			, liLimitLow, liLimitUp, liPopSize, dBedStudents, dBedElderly, dBedAdults, dLatentPrimaryToSecondary, dHospitalizedToRemoved, dInfectiveToRemoved
			, dInfectiveToHospitalized, dAntiviralArray, rng, tpPersonalInfo);

		/* VACCINATION */
		if ((iVaccinationList != NOVACC)&&((liTimeStamp % MINDAY)==0)&&(iCountDay >= iVaccinationDay)&&(iCountDay < (iVaccinationDay+iVaccinationPeriodLength)))

			Vaccination (iArrayCondition, iVaccinatedList, iNumberVaccinated, iCountDay, iVaccinationDay, iVaccinationList, iVaccinationPeriodLength, liLimitLow
			, liLimitUp, rng, tpPersonalInfo, dVaccinationSusceptibleEffectiveness, dVaccinationLower64, dVaccinationGreater64);

		/* PROCESS STATS */
		if ((iSpreadingTime > 0) && (((liTimeStamp % iSpreadingTime) == 0)||(liTimeStamp == liSimTime)||(liTimeStamp == 1))) {

			/* memeset to zeros */
			memset (iArrayStatProcess, 0, SGSIZE*(sizeof(int)));

			for (i = liLimitLow; i <= liLimitUp; i ++) iArrayStatProcess[iArrayCondition[i-liLimitLow]] ++;

			/* Store process stats */
			assert ((fprintf (flFileStatsProcess, 	"%li\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\n"
			, liTimeStamp, iArrayStatProcess[SUSCEPTIBLE], iArrayStatProcess[LATENT_PRIMARY], iArrayStatProcess[LATENT_SECONDARY]
			, iArrayStatProcess[ASYMPTOMATIC], iArrayStatProcess[INFECTIVE_PRIMARY], iArrayStatProcess[INFECTIVE_SECONDARY]
			, iArrayStatProcess[INFECTIVE_ANTIVIRAL], iArrayStatProcess[HOSPITALIZED], iArrayStatProcess[SUSCEPTIBLE_TREATED]
			, iArrayStatProcess[LATENT_PRIMARY_TREATED], iArrayStatProcess[LATENT_SECONDARY_TREATED], iArrayStatProcess[ASYMPTOMATIC_TREATED]
			, iArrayStatProcess[INFECTIVE_PRIMARY_TREATED], iArrayStatProcess[INFECTIVE_SECONDARY_TREATED]
			, iArrayStatProcess[HOSPITALIZED_TREATED], iArrayStatProcess[REMOVED], iArrayStatProcess[DEAD])) != EOF);

			if ((iMyRank == MASTER) && (iStatisticsCollectionTime > 0) && (((liTimeStamp % iStatisticsCollectionTime) == 0)||(liTimeStamp == liSimTime)||(liTimeStamp == 1))) {

				if ((liTimeStamp == iSetTimeStamp) || ((liTimeStamp % (iUpdatingTime*15)) == 0))
					printf ("\n  Time(min)   Suscp   Lat-P   Lat-S   Asymp   Inf-P   Inf-S   Antiv   Hospz   SuscT   LatPT   LatST   AsymT   InfPT   InfST   HospT   Remvd   Dead\n");

				printf ("   %8li%8i%8i%8i%8i%8i%8i%8i%8i%8i%8i%8i%8i%8i%8i%8i%8i%8i\n", liTimeStamp, iArrayStatProcess[SUSCEPTIBLE]
				, iArrayStatProcess[LATENT_PRIMARY], iArrayStatProcess[LATENT_SECONDARY], iArrayStatProcess[ASYMPTOMATIC]
				, iArrayStatProcess[INFECTIVE_PRIMARY], iArrayStatProcess[INFECTIVE_SECONDARY], iArrayStatProcess[INFECTIVE_ANTIVIRAL]
				, iArrayStatProcess[HOSPITALIZED], iArrayStatProcess[SUSCEPTIBLE_TREATED], iArrayStatProcess[LATENT_PRIMARY_TREATED]
				, iArrayStatProcess[LATENT_SECONDARY_TREATED], iArrayStatProcess[ASYMPTOMATIC_TREATED], iArrayStatProcess[INFECTIVE_PRIMARY_TREATED]
				, iArrayStatProcess[INFECTIVE_SECONDARY_TREATED], iArrayStatProcess[HOSPITALIZED_TREATED], iArrayStatProcess[REMOVED]
				, iArrayStatProcess[DEAD]);
				fflush (stdout);
			}

		}

		/* MITIGATION STRATEGIES */
		/*if ((liTimeStamp % (MINDAY*10)) == 0) {

			// Partial infected rate //
			dPartInfectedRate = ((iArrayStatProcess[2] + iArrayStatProcess[3] + iArrayStatProcess[4] + iArrayStatProcess[5] + iArrayStatProcess[7] +
							 	iArrayStatProcess[10] + iArrayStatProcess[11] + iArrayStatProcess[12] + iArrayStatProcess[14]) / liPopSize);

			MPI_Allreduce (&dPartInfectedRate, &dInfectedRate, 1, MPI_DOUBLE, MPI_SUM, EMPI_COMM_WORLD);

			// Evaluate mitigation strategies //
			if (dInfectedRate > dClosingSchools) iActiveClosingSchools = TRUE;
			if (dInfectedRate > dSocialDistancing) iActiveSocialDistancing = TRUE;
		}*/

		prevNumberProcesses = iNumberProcesses;

		//Funcion de coste = spread + update;
		if ((liTimeStamp > 0)&&((liTimeStamp % EMPI_GLOBAL_niter_lb)==0)) {

			for (i = 0; i < count; i ++) iCost[i] = iTestSpread[iArrayCondition[i]] * (csc_matrix->p[i+1] - csc_matrix->p[i]);
		}

		//monitor end
		EMPI_Monitor_end (&iMyRank, &iNumberProcesses, (int)liTimeStamp, (int)liSimTime, &count, &desp, &vcounts, &displs, iCost, argv+1, bin);

		liLimitLow = desp;
		liLimitUp = (desp+count-1);

		//get new addresses
		EMPI_Get_addr_sparse ("matrix", (void*)&csc_matrix->p, (void*)&csc_matrix->i, (void*)&csc_matrix->x);

		iArrayCondition = EMPI_Get_addr("condition");
		iArrayConditionTime = EMPI_Get_addr("conditionTime");
		iBedStatus = EMPI_Get_addr("bedStatus");
		iArrayInfectedPeople = EMPI_Get_addr("infected");

		EMPI_Get_status (&status);

		if (status == EMPI_REMOVED) {

			iNumberProcesses = prevNumberProcesses;

			printf ("[%i] Process removed at %li\n", iMyRank, liTimeStamp);

			break;

		} else if ((status == EMPI_ACTIVE)&&(prevNumberProcesses != iNumberProcesses)) {

			if (iNumberProcesses > prevNumberProcesses) {

				data[0] = iMin;
				data[1] = iHour;
				data[2] = iDay;
				data[3] = iMonth;
				data[4] = iWeekDay;
				data[5] = iGraphProcs;

				PMPI_Bcast (data, 6, MPI_INT, 0, EMPI_COMM_WORLD);
			}

			//FIXME: encapsular en funcion ResetCommArrays

			/* Set new comm matrix */
			free (iCommMatrix);

			SetCommMatrix (iMyRank, iNumberProcesses, &iCommMatrix);

			for (i = 0; i < prevNumberProcesses; i ++) {
				free (iArrayNewInfectives[i]);
				iArrayNewInfectives[i] = NULL; }
			free (iArrayNewInfectives);
			iArrayNewInfectives = NULL;
			free (iArrayNumNewInfectives);
			iArrayNumNewInfectives = NULL;
			free (iArrayRecvNumInfectives);
			iArrayRecvNumInfectives = NULL;
			free (iArraySizeNumInfectives);
			iArraySizeNumInfectives = NULL;

			/* Request memory for the array of number of infected individuals from another processes */
			assert ((iArrayNumNewInfectives = (int*) 	calloc (iNumberProcesses, sizeof (int))) != NULL);

			/* Request memory for the array of number of intected individuals to receive from another processes */
			assert ((iArrayRecvNumInfectives= (int*) 	calloc (iNumberProcesses, sizeof (int))) != NULL);

			/* Request memory for the array of number of infected individuals from another processes */
			assert ((iArraySizeNumInfectives= (int*) 	calloc (iNumberProcesses, sizeof (int))) != NULL);

			/* Request memory for the array of infected individuals from another process */
			assert ((iArrayNewInfectives 	= (int**) 	malloc (iNumberProcesses * sizeof (int*))) != NULL);

			iOffsetInfectives = BUFSIZ * 0.3 * ( (liPopSize/iNumberProcesses) % BUFSIZ);

			for (i = 0; i < iNumberProcesses; i ++) {
				/* Initialize the array */
    			iArraySizeNumInfectives[i] = iOffsetInfectives;

				assert ((iArrayNewInfectives[i] = (int*) calloc (iOffsetInfectives, sizeof (int))) != NULL);
			}
		}

		if ((liTimeStamp > 0)&&((liTimeStamp % EMPI_GLOBAL_niter_lb)==0)) {

			free (iCost);
			iCost = NULL;

			/* Array para la funcion de coste */
			assert ((iCost = (int*) malloc (count * sizeof (int))) != NULL);
		}

		/* Simulation time */
		iMin ++;
		if (iMin == 60) {
			iHour ++;
			iMin = 0;
			if (iHour == 24) {
                iDay ++;
                iCountDay++;
                if (iCountDay == 361)
                	iCountDay = 1;
                iWeekDay ++;
                if (iWeekDay > SUNDAY)
                	iWeekDay = MONDAY;
                iHour = 0;
                if (iDay == 31) {
                	iMonth++;
                	iDay = 1;
                	if (iMonth > DECEMBER) {
                		iMonth = JANUARY;
                		/* Calendar for the new year */
                		SetCalendar (&iCalendar, iWeekDay);
                	}
                }
            }
		}
	}

	EMPI_Get_status (&status);

	if (status == EMPI_ACTIVE)
		/* Communicate final stats */
		MPI_Reduce (iArrayStatProcess, iFinalStats, SGSIZE, MPI_INT, MPI_SUM, MASTER, EMPI_COMM_WORLD);

	/* Show final stats */
	if (iMyRank == MASTER) {

		printf ("\n\n  ***Simulation End***");

		printf ("\n\n  Susceptible individuals\t  : %4.2lf %c", (double) (iFinalStats[SUSCEPTIBLE]*100)/liPopSize, 37);

		printf ("\n  Latent[P] individuals     	  : %4.2lf %c", (double) (iFinalStats[LATENT_PRIMARY]*100)/liPopSize, 37);

		printf ("\n  Latent[S] individuals     	  : %4.2lf %c", (double) (iFinalStats[LATENT_SECONDARY]*100)/liPopSize, 37);

		printf ("\n  Infective[P] individuals        : %4.2lf %c", (double) (iFinalStats[INFECTIVE_PRIMARY]*100)/liPopSize, 37);

		printf ("\n  Infective[S] individuals        : %4.2lf %c", (double) (iFinalStats[INFECTIVE_SECONDARY]*100)/liPopSize, 37);

		printf ("\n  Asymptomatic individuals	  : %4.2lf %c", (double) (iFinalStats[ASYMPTOMATIC]*100)/liPopSize, 37);

		printf ("\n  Antiviral treated individuals   : %4.2lf %c", (double) (iFinalStats[INFECTIVE_ANTIVIRAL]*100)/liPopSize, 37);

		printf ("\n  Hospitalized individuals	  : %4.2lf %c", (double) (iFinalStats[HOSPITALIZED]*100)/liPopSize, 37);

		printf ("\n  Susceptible treated individuals : %4.2lf %c", (double) (iFinalStats[SUSCEPTIBLE_TREATED]*100)/liPopSize, 37);

		printf ("\n  Latent[P] treated individuals   : %4.2lf %c", (double) (iFinalStats[LATENT_PRIMARY_TREATED]*100)/liPopSize, 37);

		printf ("\n  Latent[S] treated individuals   : %4.2lf %c", (double) (iFinalStats[LATENT_SECONDARY_TREATED]*100)/liPopSize, 37);

		printf ("\n  Infective[P] treated individuals: %4.2lf %c", (double) (iFinalStats[INFECTIVE_PRIMARY_TREATED]*100)/liPopSize, 37);

		printf ("\n  Infective[S] treated individuals: %4.2lf %c", (double) (iFinalStats[INFECTIVE_SECONDARY_TREATED]*100)/liPopSize, 37);

		printf ("\n  Asymptomatic treated individuals: %4.2lf %c", (double) (iFinalStats[ASYMPTOMATIC_TREATED]*100)/liPopSize, 37);

		printf ("\n  Hospitalized treated individuals: %4.2lf %c", (double) (iFinalStats[HOSPITALIZED_TREATED]*100)/liPopSize, 37);

		printf ("\n  Removed individuals       	  : %4.2lf %c", (double) (iFinalStats[REMOVED]*100)/liPopSize, 37);

		printf ("\n  Dead individuals    	          : %4.2lf %c\n", (double) (iFinalStats[DEAD]*100)/liPopSize, 37);

		printf ("\n  [P%d] Spreading time             : %.3f seconds\n\n", iMyRank, MPI_Wtime()-tini);

		fflush (stdout);
	}

	/* Close files */
	fclose (flFileStatsProcess);

	/* Memory release */
	gsl_rng_free (rng);
	free (vcounts);
	vcounts = NULL;
	free (displs);
	displs = NULL;
	free (iCost);
	iCost = NULL;
	if (iVaccinationList != NOVACC)
		free (iVaccinatedList);
	iVaccinatedList = NULL;
	free (iCommMatrix);
	iCommMatrix = NULL;
	for (i = 0; i < iNumberProcesses; i ++) {
		free (iArrayNewInfectives[i]);
		iArrayNewInfectives[i] = NULL; }
	free (iArrayNewInfectives);
	iArrayNewInfectives = NULL;
	free (iArrayNumNewInfectives);
	iArrayNumNewInfectives = NULL;
	free (iArrayRecvNumInfectives);
	iArrayRecvNumInfectives = NULL;
    free (iArraySizeNumInfectives);
	iArraySizeNumInfectives = NULL;
	for (i = 0; i < SGSIZE; i ++) iArrayPeriod[i] = NULL;
	free (iArrayPeriod);
	iArrayPeriod = NULL;

	return (0);
}

/****************************************************************************************************************************************
*
*	'SimSpreading'
*
*	Utilidad:
*	 evaluate the spreading.
*
****************************************************************************************************************************************/
int SimSpreading	( int		iMyRank
					, int		iTestSpread[]
					, int		*iArrayCondition
					, int		**iArrayPeriod
					, int 		iMonth
					, int		iHour
					, int 		iWeekDay
					, int		iSourceGraph
					, int		*iBedStatus
					, int		iSpreadingTime
					, int		*iArraySizeNumInfectives
					, int		*iArrayNumNewInfectives
					, int		**iArrayNewInfectives
					, int		*iArrayInfectedPeople
					, int		iNumberProcesses
					, int 		iActiveClosingSchools
					, int 		iActiveSocialDistancing
					, int 		*vcounts
					, int 		*displs
					, long int	liPopSize
					, long int	liLimitLow
					, long int 	liLimitUp
					, cs_int	*csc_matrix
					, struct 	tPersonalInfo **tpPersonalInfo
					, const 	gsl_rng *rng
					, double*	dAvgTempMonth
					, double*	dAvgHumdMonth
					, double	*dRiskArray
					, double	dDegree
					, double	dDegreeSquare
					, double	dTimeini
					, FILE		*flLogfile )
{

	int		i 				= 0
			,j				= 0
			,iProcessBelong	= 0;

	double	dRiskInf		= 0
			,dRisk			= 0
			,dProbability	= 0
			,dLogTimeStamp 	= 0
			,dTimeStamp		= 0
			,dIndvDegree	= 0;

	long int liPeriod		= 0
			 ,liContact		= 0;

	/* For each individual... */
	for (i = liLimitLow; i <= liLimitUp; i ++) {

		/* If the individual is infectious... */
		if (iTestSpread[iArrayCondition[i-liLimitLow]]) {

			/* Get risk of transmission */
			dRiskInf = dRiskArray [iArrayCondition[i-liLimitLow]];

			/* Get period */
			liPeriod = iArrayPeriod[iArrayCondition[i-liLimitLow]][i];

			/* Individual degree */
			dIndvDegree = (csc_matrix->p[i-liLimitLow+1] - csc_matrix->p[i-liLimitLow]);

			/* For each connection... */
			for (j = csc_matrix->p[i-liLimitLow]; j < csc_matrix->p[i-liLimitLow+1]; j ++) {

				liContact = csc_matrix->i[j];

				/* Check connection type */
				if ((iArrayInfectedPeople[liContact] == 0) && ( Check (iHour, iWeekDay, iSourceGraph, csc_matrix->x[j]
				, iBedStatus[i-liLimitLow], (*tpPersonalInfo)[i].occupation, (*tpPersonalInfo)[liContact].occupation
				, (*tpPersonalInfo)[i].age, (*tpPersonalInfo)[liContact].age, iActiveClosingSchools, iActiveSocialDistancing) == 1)) {

					/* Get probability */
					dProbability = (double) gsl_rng_get(rng)/gsl_rng_max(rng);

					//FIXME: al cambiar el patron de tiempo segun el tipo de individuo, ¿habria que cambiar la aplicacion del dDegree en la formula?

					/* Compute infection probability */
					dRisk = ( (dRiskInf * (dDegree - 1) * iSpreadingTime) / ( dDegreeSquare * liPeriod) )
					//dRisk = ( (dRiskInf * iSpreadingTime) / ( dIndvDegree * liPeriod) )
					* WInfectivity ((*tpPersonalInfo)[liContact].age, (*tpPersonalInfo)[liContact].occupation, (*tpPersonalInfo)[liContact].gender,
					(*tpPersonalInfo)[liContact].prevdiseases, (*tpPersonalInfo)[liContact].race, (*tpPersonalInfo)[liContact].prophylaxis, iMonth,
					dAvgTempMonth, dAvgHumdMonth);

					if (dProbability < dRisk) {

						// Get process to which infected individual belongs //
						iProcessBelong = WhichProcess ((int)liContact, iNumberProcesses, vcounts, displs);

						if (iArrayNumNewInfectives[iProcessBelong] == iArraySizeNumInfectives[iProcessBelong]) {

							// LOGFILE //
							dLogTimeStamp = MPI_Wtime();
							dTimeStamp = dLogTimeStamp - dTimeini;
							assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] *** WARNING: Realloc iArrayNumNewInfectives\n", dTimeStamp, iMyRank))
							 != EOF);
							fflush (flLogfile);

							// Request additional memory //
							assert ((iArrayNewInfectives[iProcessBelong] = (int*) realloc (iArrayNewInfectives[iProcessBelong]
							, (iArraySizeNumInfectives[iProcessBelong]+BUFSIZ) * sizeof (MPI_INT))) != NULL);

							// New offset size //
							iArraySizeNumInfectives[iProcessBelong] += BUFSIZ;
						}

						// New infected individual //
						iArrayNewInfectives[iProcessBelong][iArrayNumNewInfectives[iProcessBelong]] = liContact;

						// Set infected individual //
						iArrayInfectedPeople[liContact] = 1;

						// Number of infected individuals in this round //
						iArrayNumNewInfectives[iProcessBelong] ++;
					}
				}
			}
		}
	}

	return 0;
}

/****************************************************************************************************************************************
*
*	'CommInfected'
*
*	Utilidad:
*	 communicate infected individualscl
*
****************************************************************************************************************************************/
int CommInfected	( int		iMyRank
					, int		iNumberProcesses
					, int		*iArrayNumNewInfectives
					, int		*iArrayRecvNumInfectives
					, int		*iArrayCondition
					, int		**iArrayNewInfectives
					, int		*iArrayInfectedPeople
					, int		*iCommMatrix
					, long int 	liLimitLow
					, long int 	liLimitUp )
{

	int		n				= 0
			,i				= 0
			,*iBuffer		= NULL
			,iNumComms		= 0
			,mpi_tag		= 678
			,iSendProcess 	= 0
			,iCommRank		= 0
			,iToken			= 0;

	MPI_Status status;

    if ((iNumberProcesses % 2) == 0)
        iNumComms = iNumberProcesses-1;
    else
        iNumComms = iNumberProcesses;

	/* Numbers of communications */
	for (n = 0; n < iNumComms; n ++) {

		/* Get iCommRank */
		for (iSendProcess = n * (iNumComms + 1); iSendProcess < (n * (iNumComms + 1)) + (iNumComms + 1); iSendProcess ++) {

			if (iCommMatrix[iSendProcess] == iMyRank) {

				iCommRank = iCommMatrix[(iNumComms - (iSendProcess % (iNumComms + 1))) + (n * (iNumComms + 1))];

				if (iCommRank < 0)

					iToken = TOKEN_NOACT;

				else {

					if ((iSendProcess % (iNumComms+1)) < CEIL (iNumberProcesses, 2))
						iToken = TOKEN_SEND;
					else
						iToken = TOKEN_RECEIVE;
				}
			}
		}

		if (iToken == TOKEN_SEND) {

			/* SEND DATA */

			/* Send the number of elements of the buffer */
			MPI_Send(&iArrayNumNewInfectives[iCommRank], 1, MPI_INT, iCommRank, mpi_tag, EMPI_COMM_WORLD);

			/* Sending just in case the number of new infectives > 0 */
			if (iArrayNumNewInfectives[iCommRank] > 0) {

				/* Memory request */
				assert ((iBuffer = (int*) malloc (iArrayNumNewInfectives[iCommRank] * sizeof (int))) != NULL);

				/* Copy data into the buffer */
				for (i = 0; i < iArrayNumNewInfectives[iCommRank]; i ++)
					iBuffer[i] = iArrayNewInfectives[iCommRank][i];

				/* Send the external relationships data */
				MPI_Send(iBuffer, iArrayNumNewInfectives[iCommRank], MPI_INT, iCommRank, mpi_tag, EMPI_COMM_WORLD);

				iArrayNumNewInfectives[iCommRank] = 0;

				/* Release memory */
				free (iBuffer);
				iBuffer = NULL;
			}

			/* RECEIVE DATA */

			/* Receive the number of elements of the buffer */
			MPI_Recv(&iArrayRecvNumInfectives[iCommRank], 1, MPI_INT, iCommRank, mpi_tag, EMPI_COMM_WORLD, &status);

			/* Waiting for receive just in case the number of infected individuals > 0 */
			if (iArrayRecvNumInfectives[iCommRank] > 0) {

				/* Memory request */
				assert ((iBuffer = (int*) malloc (iArrayRecvNumInfectives[iCommRank] * sizeof (int))) != NULL);

				/* Receive the external relationships data */
				MPI_Recv(iBuffer, iArrayRecvNumInfectives[iCommRank], MPI_INT, iCommRank, mpi_tag, EMPI_COMM_WORLD, &status);

				/* New infectives */
				for (i = 0; i < iArrayRecvNumInfectives[iCommRank]; i ++)
					if (((iBuffer[i] >= liLimitLow)&&(iBuffer[i] <= liLimitUp))&&(iArrayCondition[iBuffer[i]-liLimitLow] == SUSCEPTIBLE)) {
						iArrayCondition[iBuffer[i]-liLimitLow] = LATENT_PRIMARY;
						iArrayInfectedPeople[iBuffer[i]] = 1;
					} else if (((iBuffer[i] >= liLimitLow)&&(iBuffer[i] <= liLimitUp))&&(iArrayCondition[iBuffer[i]-liLimitLow] == SUSCEPTIBLE_TREATED)) {
						iArrayCondition[iBuffer[i]-liLimitLow] = LATENT_PRIMARY_TREATED;
						iArrayInfectedPeople[iBuffer[i]] = 1;
					}

				/* Reset number of infected individuals to receive */
				iArrayRecvNumInfectives[iCommRank] = 0;

				/* Release memory */
				free (iBuffer);
				iBuffer = NULL;
			}

		} else if (iToken == TOKEN_RECEIVE) {

			/* RECEIVE DATA */

			/* Receive the number of elements of the buffer */
			MPI_Recv(&iArrayRecvNumInfectives[iCommRank], 1, MPI_INT, iCommRank, mpi_tag, EMPI_COMM_WORLD, &status);

			/* Waiting for receive just in case the number of infected individuals > 0 */
			if (iArrayRecvNumInfectives[iCommRank] > 0) {

				/* Memory request */
				assert ((iBuffer = (int*) malloc (iArrayRecvNumInfectives[iCommRank] * sizeof (int))) != NULL);

				/* Receive the external relationships data */
				MPI_Recv(iBuffer, iArrayRecvNumInfectives[iCommRank], MPI_INT, iCommRank, mpi_tag, EMPI_COMM_WORLD, &status);

				/* New infectives */
				for (i = 0; i < iArrayRecvNumInfectives[iCommRank]; i ++)
					if (((iBuffer[i] >= liLimitLow)&&(iBuffer[i] <= liLimitUp))&&(iArrayCondition[iBuffer[i]-liLimitLow] == SUSCEPTIBLE)) {
						iArrayCondition[iBuffer[i]-liLimitLow] = LATENT_PRIMARY;
						iArrayInfectedPeople[iBuffer[i]] = 1;
					} else if (((iBuffer[i] >= liLimitLow)&&(iBuffer[i] <= liLimitUp))&&(iArrayCondition[iBuffer[i]-liLimitLow] == SUSCEPTIBLE_TREATED)) {
						iArrayCondition[iBuffer[i]-liLimitLow] = LATENT_PRIMARY_TREATED;
						iArrayInfectedPeople[iBuffer[i]] = 1;
					}

				/* Reset number of infected individuals to receive */
				iArrayRecvNumInfectives[iCommRank] = 0;

				/* Release memory */
				free (iBuffer);
				iBuffer = NULL;
			}

			/* SEND DATA */

			/* Send the number of elements of the buffer */
			MPI_Send(&iArrayNumNewInfectives[iCommRank], 1, MPI_INT, iCommRank, mpi_tag, EMPI_COMM_WORLD);

			/* Sending just in case the number of new infectives > 0 */
			if (iArrayNumNewInfectives[iCommRank] > 0) {

				/* Memory request */
				assert ((iBuffer = (int*) malloc (iArrayNumNewInfectives[iCommRank] * sizeof (int))) != NULL);

				/* Copy data into the buffer */
				for (i = 0; i < iArrayNumNewInfectives[iCommRank]; i ++)
					iBuffer[i] = iArrayNewInfectives[iCommRank][i];

				/* Send the external relationships data */
				MPI_Send(iBuffer, iArrayNumNewInfectives[iCommRank], MPI_INT, iCommRank, mpi_tag, EMPI_COMM_WORLD);

				iArrayNumNewInfectives[iCommRank] = 0;

				/* Release memory */
				free (iBuffer);
				iBuffer = NULL;
			}
		}
	}

	/* Set my infected individuals */
	for (n = 0; n < iArrayNumNewInfectives[iMyRank]; n ++) {

		if (((iArrayNewInfectives[iMyRank][n] >= liLimitLow)&&(iArrayNewInfectives[iMyRank][n] <= liLimitUp))&&(iArrayCondition[iArrayNewInfectives[iMyRank][n]-liLimitLow] == SUSCEPTIBLE))

			iArrayCondition[iArrayNewInfectives[iMyRank][n]-liLimitLow] = LATENT_PRIMARY;

		else if (((iArrayNewInfectives[iMyRank][n] >= liLimitLow)&&(iArrayNewInfectives[iMyRank][n] <= liLimitUp))&&(iArrayCondition[iArrayNewInfectives[iMyRank][n]-liLimitLow] == SUSCEPTIBLE_TREATED))

			iArrayCondition[iArrayNewInfectives[iMyRank][n]-liLimitLow] = LATENT_PRIMARY_TREATED;
	}

	/* Set number of new infectives */
	iArrayNumNewInfectives[iMyRank] = 0;

	return 0;
}

/****************************************************************************************************************************************
*
*	'UpdatingStates'
*
*	Utilidad:
*	 updates the graph states of the population.
*
****************************************************************************************************************************************/
int UpdatingStates	( int		iTestUpdate[]
					, int		*iArrayCondition
					, int		*iArrayConditionTime
					, int		iUpdatingTime
					, int		**iArrayPeriod
					, int		**iArrayAntiviralDelayPeriod
					, int		iBedTime
					, int		*iBedStatus
					, long int	liLimitLow
					, long int 	liLimitUp
					, long int	liPopSize
					, double 	dBedStudents
					, double 	dBedElderly
					, double 	dBedAdults
					, double	dLatentPrimaryToSecondary
					, double 	dHospitalizedToRemoved
					, double 	dInfectiveToRemoved
					, double 	dInfectiveToHospitalized
					, double	*dAntiviralArray
					, const 	gsl_rng *rng
					, struct 	tPersonalInfo **tpPersonalInfo )
{

	int			i 						= 0;

	long int	liPeriod				= 0;

	double		dProbability			= 0
				,dAntiviralProbability	= 0;

	for (i = liLimitLow; i <= liLimitUp; i ++) {

		if (iTestUpdate[iArrayCondition[i-liLimitLow]]) {

			/* Possible transition depending on the condition of the individual */
			iArrayConditionTime[i-liLimitLow] += iUpdatingTime;

			/* Get period */
			liPeriod = iArrayPeriod[iArrayCondition[i-liLimitLow]][i];

			if (iArrayConditionTime[i-liLimitLow] >= liPeriod) {

				iArrayConditionTime[i-liLimitLow] = 0;

				switch (iArrayCondition[i-liLimitLow]) {

					case HOSPITALIZED:
					case HOSPITALIZED_TREATED:

						dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

						if (dProbability < dHospitalizedToRemoved)
							iArrayCondition[i-liLimitLow] = REMOVED;
						else
							iArrayCondition[i-liLimitLow] = DEAD;

		                break;

					case INFECTIVE_ANTIVIRAL:
					case INFECTIVE_SECONDARY:
					case INFECTIVE_SECONDARY_TREATED:

						dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

						if (dProbability < dInfectiveToRemoved)
							iArrayCondition[i-liLimitLow] = REMOVED;
						else 	if ((dProbability >= dInfectiveToRemoved) && (dProbability < (dInfectiveToRemoved+dInfectiveToHospitalized))) {
									if (iArrayCondition[i-liLimitLow] == INFECTIVE_SECONDARY_TREATED)
										iArrayCondition[i-liLimitLow] = HOSPITALIZED_TREATED;
									else
										iArrayCondition[i-liLimitLow] = HOSPITALIZED;
								} else
									iArrayCondition[i-liLimitLow] = DEAD;

						break;

					case INFECTIVE_PRIMARY:

						iArrayCondition[i-liLimitLow] = INFECTIVE_SECONDARY;

						break;

					case INFECTIVE_PRIMARY_TREATED:

						iArrayCondition[i-liLimitLow] = INFECTIVE_SECONDARY_TREATED;

						break;

					case ASYMPTOMATIC:

						iArrayCondition[i-liLimitLow] = REMOVED;

						break;

					case ASYMPTOMATIC_TREATED:

						iArrayCondition[i-liLimitLow] = REMOVED;

						break;

					case LATENT_SECONDARY:

						iArrayCondition[i-liLimitLow] = INFECTIVE_PRIMARY;

						break;

					case LATENT_SECONDARY_TREATED:

						iArrayCondition[i-liLimitLow] = INFECTIVE_PRIMARY_TREATED;

						break;

					case LATENT_PRIMARY:

						dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

						if (dProbability < dLatentPrimaryToSecondary)
							iArrayCondition[i-liLimitLow] = LATENT_SECONDARY;
						else
							iArrayCondition[i-liLimitLow] = ASYMPTOMATIC;

						break;

					case LATENT_PRIMARY_TREATED:

						dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

						if (dProbability < dLatentPrimaryToSecondary)
							iArrayCondition[i-liLimitLow] = LATENT_SECONDARY_TREATED;
						else
							iArrayCondition[i-liLimitLow] = ASYMPTOMATIC_TREATED;

						break;
				}

			} else 	if (iArrayCondition[i-liLimitLow] == INFECTIVE_PRIMARY) {

				if	((iArrayConditionTime[i-liLimitLow]/iUpdatingTime) ==
					(((*iArrayAntiviralDelayPeriod)[i-liLimitLow])/iUpdatingTime)){

					dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

					/* Get antiviral probability */
					dAntiviralProbability = dAntiviralArray[(*tpPersonalInfo)[i].occupation-1];

					if ((dProbability < dAntiviralProbability) && (iArrayConditionTime[i-liLimitLow] <
						(*iArrayAntiviralDelayPeriod)[i-liLimitLow])) {

						/* Effective probability of the antiviral treatment (interpolation) */
						dAntiviralProbability = 1 + ((*iArrayAntiviralDelayPeriod)[i-liLimitLow]-
						(0.25*MINDAY)) * ((0-1)/(iArrayConditionTime[i-liLimitLow]-
						(*iArrayAntiviralDelayPeriod)[i-liLimitLow]));

						dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

						if (dProbability < dAntiviralProbability) {

							iArrayConditionTime[i-liLimitLow] = 0;

							iArrayCondition[i-liLimitLow] = INFECTIVE_ANTIVIRAL;
						}
					}
				}

				if ((iBedTime/iUpdatingTime) == (iArrayConditionTime[i-liLimitLow]/iUpdatingTime)) {

					dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

					/* Bed rest */
					if ((*tpPersonalInfo)[i].occupation == 2) {

						/* < 18 individuals probability */
						if (dProbability < dBedStudents) iBedStatus[i-liLimitLow] = 1;

		            } else if ((*tpPersonalInfo)[i].occupation == 3) {

		            	/* > 64 individuals probability */
						if (dProbability < dBedElderly) iBedStatus[i-liLimitLow] = 1;

		            } else	{
						/* > 18 and < 64 individuals probability */
						if (dProbability < dBedAdults) iBedStatus[i-liLimitLow] = 1;
					}
				}
			}
		}
	}

	return 0;
}

/****************************************************************************************************************************************
*
*	'ResetCommArrays'
*
*	Utilidad:
*	 reset arrays.
*
****************************************************************************************************************************************/
int ResetCommArrays	( int		iMyRank
					, int		iNumberProcesses
					, int		iNewProcesses
					, int**		iArrayNumNewInfectives
					, int**		iArrayRecvNumInfectives
					, int** 	iArraySizeNumInfectives
					, int***	iArrayNewInfectives
					, long int	liPopSize )
{

	int		n 					= 0
			,iOffsetInfectives 	= 0;

	/* Release memory */
	free ((*iArrayNumNewInfectives));
	(*iArrayNumNewInfectives) = NULL;
	free ((*iArrayRecvNumInfectives));
	(*iArrayRecvNumInfectives) = NULL;
	free ((*iArraySizeNumInfectives));
	(*iArraySizeNumInfectives) = NULL;

	for (n = 0; n < iNumberProcesses; n ++) {

		free ((*iArrayNewInfectives)[n]);
		(*iArrayNewInfectives)[n] = NULL;
	}

	free ((*iArrayNewInfectives));
	(*iArrayNewInfectives) = NULL;

	/* Request memory for the array of number of infected individuals from another processes */
	assert (((*iArrayNumNewInfectives) 	= (int*) 	calloc (iNumberProcesses, sizeof (int))) != NULL);

	/* Request memory for the array of number of intected individuals to receive from another processes */
	assert (((*iArrayRecvNumInfectives)	= (int*) 	calloc (iNumberProcesses, sizeof (int))) != NULL);

	/* Request memory for the array of number of infected individuals from another processes */
	assert (((*iArraySizeNumInfectives)	= (int*) 	calloc (iNumberProcesses, sizeof (int))) != NULL);

	/* Request memory for the array of infected individuals from another process */
	assert (((*iArrayNewInfectives) 	= (int**) 	malloc (iNumberProcesses * sizeof (int*))) != NULL);

	iOffsetInfectives = BUFSIZ * 0.3 * ( (liPopSize/(iNewProcesses)) % BUFSIZ);

	for (n = 0; n < iNumberProcesses; n ++) {

		/* Initialize the array */
		(*iArraySizeNumInfectives)[n] = iOffsetInfectives;

		assert (((*iArrayNewInfectives)[n] = (int*) calloc (iOffsetInfectives, sizeof (int))) != NULL);
	}

	return 0;
}

/****************************************************************************************************************************************
*
*	'SetCommMatrix'
*
*	Utilidad:
*	 generate the communications matrix for the active processes.
*
****************************************************************************************************************************************/
int SetCommMatrix	( int		iMyRank
					, int		iNumberProcesses
					, int		**iCommMatrix)
{

	int		iStartRank			= 0
			,iLastValue			= 0
			,iNumComms			= 0
			,aux				= 0
			,n					= 0
			,i					= 0;

	/* Declare and generate the communications matrix */
   	/* Set the number of communications */
    if ((iNumberProcesses % 2) == 0) {

        iNumComms = iNumberProcesses-1;
        iStartRank = iNumberProcesses-2;
        iLastValue = iNumberProcesses-1;

    } else {

        iNumComms = iNumberProcesses;
		iStartRank = iNumberProcesses-1;
        iLastValue = -1;
    }

    /* Request memory for the communications matrix */
	assert ((*iCommMatrix  = (int*) calloc (iNumComms * (iNumComms + 1), sizeof (int))) != NULL);

    /* Set first row */
    for (n = 0; n <= iStartRank; n ++)
        (*iCommMatrix)[n] = n;

    /* Set last value */
	if ((iNumberProcesses % 2) == 0)
    	for (n = 0; n < iNumComms; n ++)
        	(*iCommMatrix)[((n+1)*(iNumComms))+n] = iLastValue;
    else
    	for (n = 0; n < iNumComms; n ++)
        	(*iCommMatrix)[((n+1)*(iNumComms))+n] = -1;

    /* Complete the matrix */
    for (n = 1; n < iNumComms; n ++) {

        aux = iStartRank;

        for (i = 0; i < iNumComms; i ++) {

            (*iCommMatrix)[(n*(iNumComms+1))+i] = aux;

            aux ++;

			if ((iNumberProcesses % 2) == 0) {
            	if (aux == iNumberProcesses - 1)
                	aux = 0;
			} else 	if ((iNumberProcesses % 2) != 0) {
            			if (aux == iNumberProcesses)
                			aux = 0;
			}
        }

        iStartRank --;
    }

	return 0;
}

/****************************************************************************************************************************************
*
*	'Vaccination'
*
*	Utilidad:
*	 evaluating vaccination of individuals.
*
****************************************************************************************************************************************/
int Vaccination	( int*		iArrayCondition
				, int*		iVaccinatedList
				, int 		iVaccinatedIndividuals
				, int 		iCountDay
				, int		iVaccinationDay
				, int		iVaccinationList
				, int		iVaccinationPeriodLength
				, long int	liLimitLow
				, long int 	liLimitUp
				, const 	gsl_rng *rng
				, struct 	tPersonalInfo **tpPersonalInfo
				, double	dVaccinationSusceptibleEffectiveness
				, double	dVaccinationLower64
				, double	dVaccinationGreater64 )
{
	int 	i 		= 0;

	long int iStart = 0
			,iEnd 	= 0;

	double 	dProbability = 0
			,dVaccinationEffectiveness = 0;

	/* Calculating the vaccinated individuals */
	CalcLimits ((iCountDay-iVaccinationDay), iVaccinationPeriodLength, iVaccinatedIndividuals, &iStart, &iEnd);

	for (i = iStart; i <= iEnd; i ++) {

		if ((iVaccinatedList[i] >= liLimitLow)&&(iVaccinatedList[i] <= liLimitUp)&&(iArrayCondition[iVaccinatedList[i]-liLimitLow] == SUSCEPTIBLE)) {

			dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

			/* Vaccination treatment effect depends on the age of the individual */
		    if ( (*tpPersonalInfo)[i].age < 64 )
		        dVaccinationEffectiveness = dVaccinationSusceptibleEffectiveness * dVaccinationLower64;
		    else
		        dVaccinationEffectiveness = dVaccinationSusceptibleEffectiveness * dVaccinationGreater64;

			if (dProbability <= dVaccinationEffectiveness)
				iArrayCondition[iVaccinatedList[i]-liLimitLow] = SUSCEPTIBLE_TREATED;
		}
	}

	return 0;
}
