/****************************************************************************************************************************************
 *																																		*
 *	Research Project - Universidad Carlos III de Madrid [2009-2019]																		*
 *                                                                                                                                      *
 *	- EpiGraph: A Scalable Simulation Tool for Epidemiological Studies -																*
 *																																		*
 *	File:       graph_generator.c																										*
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

/****************************************************************************************************************************************
*
*	'LoadXmlGraphFile'
*
*	Utility:
*	 collect and store XML graph configuration file parameters.
*
****************************************************************************************************************************************/
int LoadXmlGraphFile	( int*		iNumGraphs
                        , int*		iIdGraphWorkers
                        , int*		iIdGraphStudents
                        , int*		iIdGraphElderly
                        , int*		iIdGraphUnemployed
                        , int** 	iArrayAges
                        , int**		iArraySizes
                        , int*      iSourceGraph
                        , int*		iMorningPeriod
                        , int*		iEveningPeriod
                        , int*		iNightPeriod
                        , float**	fArrayPercentages
                        , float*	fThreshold
                        , float*	fWeakConnection
                        , float*	fStrongConnection
						, float*	fMuLatentSecondaryPeriod
                        , float*	fSigmaLatentSecondaryPeriod
                        , float*	fMuAntiviralDelayPeriod
                        , float*	fSigmaAntiviralDelayPeriod
                        , float*	fMuLatentPrimaryPeriod
                        , float*	fMuHospPeriod
                        , float*	fMuInfectivePrimaryPeriod
                        , float*	fMuInfectiveSecondaryPeriod
                        , float*	fSigmaInfectiveSecondaryPeriod
                        , float*	fSigmaInfectivePrimaryPeriod
                        , float*	fSigmaAntiviralPeriod
                        , float*	fMuAntiviralPeriod
                        , float*	fSigmaAsymptomaticPeriod
                        , float*	fMuAsymptomaticPeriod
                        , float*	fSigmaLatentPrimaryPeriod
                        , float*	fSigmaHospPeriod
                        , long int*	liPopSize
                        , long int*	liRelationshipAvgn
                        , double**	dHousehold
                        , double*   dMuGraph
                        , double*   dSigmaGraph
                        , double*   dCompaniesOnSaturday
                        , double*	dWorkersMale
                        , double*	dStudentsMale
                        , double*	dElderlyMale
                        , double*	dUnemployedMale
                        , double*	dRaceWhite
                        , double*	dRaceBlack
                        , double*	dRaceAmindian
                        , double*	dRaceAsian
                        , double*	dRacePacific
                        , double*	dRaceOther
                        , double*	dWorkersSigmaAge
                        , double*	dWorkersMuAge
                        , double*	dStudentsSigmaAge
                        , double*	dStudentsMuAge
                        , double*	dElderlySigmaAge
                        , double*	dElderlyMuAge
                        , double*	dUnemployedSigmaAge
                        , double*	dUnemployedMuAge
                        , char**	cPathV
						, char		*cConfigXML
						, char		*cGraphXML
                        , char**	cPathStructure
                        , char**	cPathSource
                        , char**	cPathNFS )
{
	xmlDocPtr 	xXmlDoc;	/* Pointer to the XML config file */
	xmlNodePtr 	xRoot;		/* Pointer to the file root node */
	xmlChar 	*xValue;	/* Parameter values */

	/* Opening the XML file */
	if ((xXmlDoc = xmlParseFile (cGraphXML)) != NULL) {

		/* Root node reference */
		assert((xRoot = xmlDocGetRootElement (xXmlDoc)) != NULL);

		/* Parameter values of the XML configuration file */
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "NumGraphs")) != NULL);
		*iNumGraphs = atoi ((const char *) xValue);

		/* Loading the age ranges */
		assert ((*iArrayAges 		= (int *) 		malloc (4 * 2 	*sizeof(int)))	 !=NULL);
		assert ((*iArraySizes 		= (int *) 		malloc (4 * 2 	*sizeof(int)))	 !=NULL);
		assert ((*fArrayPercentages = (float *) 	malloc (4 * 2 	*sizeof(float))) !=NULL);
		assert ((*dHousehold 		= (double *) 	malloc (5 		*sizeof(double)))!=NULL);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "IdGraphWorkers")) != NULL);
		*iIdGraphWorkers = atoi ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "WorkersMinAge")) != NULL);
		(*iArrayAges)[0] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "WorkersMaxAge")) != NULL);
		(*iArrayAges)[1] = atoi ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "WorkersMale")) != NULL);
		*dWorkersMale = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PercentageWorkers")) != NULL);
		(*fArrayPercentages)[0] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "WorkersMinSize")) != NULL);
		(*iArraySizes)[0] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "WorkersMaxSize")) != NULL);
		(*iArraySizes)[1] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "IdGraphStudents")) != NULL);
		*iIdGraphStudents = atoi ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "StudentsMinAge")) != NULL);
		(*iArrayAges)[2] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "StudentsMaxAge")) != NULL);
		(*iArrayAges)[3] = atoi ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "StudentsMale")) != NULL);
		*dStudentsMale = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PercentageStudents")) != NULL);
		(*fArrayPercentages)[1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "StudentsMinSize")) != NULL);
		(*iArraySizes)[2] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "StudentsMaxSize")) != NULL);
		(*iArraySizes)[3] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "IdGraphElderly")) != NULL);
		*iIdGraphElderly = atoi ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ElderlyMinAge")) != NULL);
		(*iArrayAges)[4] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ElderlyMaxAge")) != NULL);
		(*iArrayAges)[5] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ElderlyMale")) != NULL);
		*dElderlyMale = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PercentageElderly")) != NULL);
		(*fArrayPercentages)[2] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ElderlyMinSize")) != NULL);
		(*iArraySizes)[4] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ElderlyMaxSize")) != NULL);
		(*iArraySizes)[5] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "IdGraphUnemployed")) != NULL);
		*iIdGraphUnemployed = atoi ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "UnemployedMinAge")) != NULL);
		(*iArrayAges)[6] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "UnemployedMaxAge")) != NULL);
		(*iArrayAges)[7] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "UnemployedMale")) != NULL);
		*dUnemployedMale = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PercentageUnemployed")) != NULL);
		(*fArrayPercentages)[3] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "UnemployedMinSize")) != NULL);
		(*iArraySizes)[6] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "UnemployedMaxSize")) != NULL);
		(*iArraySizes)[7] = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Threshold")) != NULL);
		*fThreshold = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "WeakConnection")) != NULL);
		*fWeakConnection = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "StrongConnection")) != NULL);
		*fStrongConnection = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PopSize")) != NULL);
		*liPopSize = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "RelationshipAvgn")) != NULL);
		*liRelationshipAvgn = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Household_1")) != NULL);
		(*dHousehold)[0] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Household_2")) != NULL);
		(*dHousehold)[1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Household_3")) != NULL);
		(*dHousehold)[2] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Household_4")) != NULL);
		(*dHousehold)[3] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Household_5")) != NULL);
		(*dHousehold)[4] = atof ((const char *) xValue);
        
        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "CompaniesOnSaturday")) != NULL);
		(*dCompaniesOnSaturday) = atof ((const char *) xValue);
        
        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SourceGraph")) != NULL);
		*iSourceGraph = atoi ((const char *) xValue);
        
        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MuGraph")) != NULL);
		*dMuGraph = atof ((const char *) xValue);
        
        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SigmaGraph")) != NULL);
		*dSigmaGraph = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "RaceWhite")) != NULL);
		*dRaceWhite = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "RaceBlack")) != NULL);
		*dRaceBlack = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "RaceAmindian")) != NULL);
		*dRaceAmindian = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "RaceAsian")) != NULL);
		*dRaceAsian = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "RacePacific")) != NULL);
		*dRacePacific = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "RaceOther")) != NULL);
		*dRaceOther = atof ((const char *) xValue);
        
        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SourceGraph")) != NULL);
		*iSourceGraph = atoi ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "WorkersSigmaAge")) != NULL);
		*dWorkersSigmaAge = atof ((const char *) xValue);

        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "WorkersMuAge")) != NULL);
		*dWorkersMuAge = atof ((const char *) xValue);

        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "StudentsSigmaAge")) != NULL);
		*dStudentsSigmaAge = atof ((const char *) xValue);

        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "StudentsMuAge")) != NULL);
		*dStudentsMuAge = atof ((const char *) xValue);

        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ElderlySigmaAge")) != NULL);
		*dElderlySigmaAge = atof ((const char *) xValue);

        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ElderlyMuAge")) != NULL);
		*dElderlyMuAge = atof ((const char *) xValue);

        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "UnemployedSigmaAge")) != NULL);
		*dUnemployedSigmaAge = atof ((const char *) xValue);

        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "UnemployedMuAge")) != NULL);
		*dUnemployedMuAge = atof ((const char *) xValue);
		
		xmlFree (xValue);

	} else {
		printf ("\n**ERROR**: opening the file '%s'\n", cGraphXML);
		exit (-1);
	}

	/* Opening the XML file */
	if ((xXmlDoc = xmlParseFile (cConfigXML)) != NULL) {

		/* Root node reference */
		assert((xRoot = xmlDocGetRootElement (xXmlDoc)) != NULL);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PathDest")) != NULL);
		assert ((*cPathV = (char*) malloc ((strlen((const char *) xValue)+1) * sizeof (char))) != NULL);
		strcpy (*cPathV, (const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PathStructure")) != NULL);
		assert ((*cPathStructure = (char*) malloc ((strlen((const char *) xValue)+1) * sizeof (char))) != NULL);
		strcpy (*cPathStructure, (const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PathSource")) != NULL);
		assert ((*cPathSource = (char*) malloc ((strlen((const char *) xValue)+1) * sizeof (char))) != NULL);
		strcpy (*cPathSource, (const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PathNFS")) != NULL);
		assert ((*cPathNFS = (char*) malloc ((strlen((const char *) xValue)+1) * sizeof (char))) != NULL);
		strcpy (*cPathNFS, (const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MorningPeriod")) != NULL);
		*iMorningPeriod = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "EveningPeriod")) != NULL);
		*iEveningPeriod = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "NightPeriod")) != NULL);
		*iNightPeriod = atoi ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MuLatentPrimaryPeriod")) != NULL);
		*fMuLatentPrimaryPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SigmaLatentPrimaryPeriod")) != NULL);
		*fSigmaLatentPrimaryPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MuLatentSecondaryPeriod")) != NULL);
		*fMuLatentSecondaryPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SigmaLatentSecondaryPeriod")) != NULL);
		*fSigmaLatentSecondaryPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MuHospPeriod")) != NULL);
		*fMuHospPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SigmaInfectivePrimaryPeriod")) != NULL);
		*fSigmaInfectivePrimaryPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MuInfectivePrimaryPeriod")) != NULL);
		*fMuInfectivePrimaryPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SigmaInfectiveSecondaryPeriod")) != NULL);
		*fSigmaInfectiveSecondaryPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MuInfectiveSecondaryPeriod")) != NULL);
		*fMuInfectiveSecondaryPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SigmaAntiviralPeriod")) != NULL);
		*fSigmaAntiviralPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MuAntiviralPeriod")) != NULL);
		*fMuAntiviralPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SigmaAntiviralDelayPeriod")) != NULL);
		*fSigmaAntiviralDelayPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MuAntiviralDelayPeriod")) != NULL);
		*fMuAntiviralDelayPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SigmaAsymptomaticPeriod")) != NULL);
		*fSigmaAsymptomaticPeriod = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MuAsymptomaticPeriod")) != NULL);
		*fMuAsymptomaticPeriod = atof ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SigmaHospPeriod")) != NULL);
		*fSigmaHospPeriod = atof ((const char *) xValue);

		xmlFree (xValue);

	} else {
		printf ("\n**ERROR**: opening the file '%s'\n", cConfigXML);
		exit (-1);
	}

	return (0);
}

/****************************************************************************************************************************************
*
*	'GenerateGraphGroups'
*
*	Utility: 
*	 generate a file with graph groups configuration.
*
****************************************************************************************************************************************/
int GenerateGraphGroups	( int		iMyRank
						, int*		iArraySizes
						, int		iNumberProcesses
						, long int	liPopSize
						, float*	fArrayPercentages
            			, double    dCompaniesOnSaturday
						, char*		cPathStructure )
{
    int		j       			= 0
        	,iRank      		= 0
			,iGroupType			= 0
			,iGroupSize			= 0
        	,**iArrayGProcs    	= NULL
        	,*iArrayNumGroups 	= NULL
        	,iMaxGroups     	= 4;
    
    double  dNewSize   			= 0
    		,dProbability		= 0
    		,dProbCounter		= 0;
    		
   	float	fAux               	= 0;

	long int liAccumulatedPop	= 0
         	,liGroupPop		[4] = {0, 0, 0, 0}
         	,liGroupNumber	[4] = {0, 0, 0, 0}
    		,liTeoricSize	[4] = {0, 0, 0, 0}
    		,*liArrayGenPop   	= NULL
    		,*liArrayTeoricPop 	= NULL;

	char    cFileNameStructFile[128];

	FILE 	*flStructFile		= NULL;
    
	/* GSL random number generator */
	gsl_rng *r;
	r = gsl_rng_alloc (gsl_rng_ranlxs2);
    gsl_rng_set (r, getpid());
    
   	/* Encoding Group Types
     	* 1 - Workers
     	* 2 - Students
    	* 3 - Elderly
    	* 4 - Unemployed
     	* 5 - Workers on Saturday
	*/
    
	/* File name */
	sprintf (cFileNameStructFile, "%s%s", cPathStructure, "structure.dat");

    /* Creating "structure.dat" */
	if ( (flStructFile = fopen (cFileNameStructFile, "w")) == NULL) {
		printf ("\n**ERROR**: creating the file '%s'\n", cFileNameStructFile);
		fflush (stdout);

		exit (-1);
	}
    
    /* Request memory for processes groups */
    iArrayGProcs	= (int**) malloc (iNumberProcesses * sizeof (int *));
    
    /* Request memory for generated groups */
    iArrayNumGroups	= (int*) malloc (iNumberProcesses * sizeof  (int));
    
    /* Request memory for generated population */
    liArrayGenPop	= (long int*) malloc (iNumberProcesses * sizeof  (long int));
    
    /* Request memory for teoric population */
    liArrayTeoricPop= (long int*) malloc (iNumberProcesses * sizeof  (long int));
    
    /* Initialization of variables */
    for (iRank = 0; iRank < iNumberProcesses; iRank ++) {
        
        /* Initialization of arrays */
        iArrayNumGroups[iRank] 	= 0;
        liArrayGenPop [iRank] 	= 0;
        
        /* Calculate the population size of every process */
        liArrayTeoricPop[iRank] = liPopSize / iNumberProcesses;
        
        if (((liPopSize % iNumberProcesses) != 0) && (iRank < (liPopSize % iNumberProcesses)))
			liArrayTeoricPop[iRank] ++;
        
        /* Initialization of groups array */
        iArrayGProcs[iRank] = (int*) malloc (sizeof (int));
    }
    
    /* Compute the maximum teoric size for every group */
    for (iRank = 0; iRank < iMaxGroups; iRank ++)
        liTeoricSize[iRank] = ceil (fArrayPercentages[iRank] * liPopSize);
    
    /* Process counter */
    iRank = 0;
    
    do {
        /* Starting the generation of the groups  */
        dProbability = (double) gsl_rng_get (r)/gsl_rng_max(r);
        
        dProbCounter = 0;
        iGroupType = 0;

        /* Evaluar si el proceso ya ha generado toda su poblacion */
        if (liArrayTeoricPop[iRank] > liArrayGenPop[iRank]) {
            
            /* Type group selected in random way */
            while ((dProbability > (dProbCounter + fArrayPercentages[iGroupType])) && (iGroupType < iMaxGroups)){
                dProbCounter += fArrayPercentages[iGroupType];
                iGroupType ++;
            }
        
            /* Type group counter */
            liGroupNumber[iGroupType] ++;

            /* Probability to workers on Saturday */
            dProbability = (double) gsl_rng_get (r)/gsl_rng_max(r);
                
           	/* Group size selection */
            if ((liArrayTeoricPop[iRank] - liArrayGenPop[iRank]) > iArraySizes[(iGroupType*2)+1]) {
            
                //iGroupSize = (rand () % (iArraySizes[(iGroupType*2)+1]-iArraySizes[iGroupType*2])) + iArraySizes[iGroupType*2];
                iGroupSize = 	( ((double) gsl_rng_get (r)/gsl_rng_max(r)) * (iArraySizes[(iGroupType*2)+1]-iArraySizes[iGroupType*2]) ) 
                				+ iArraySizes[iGroupType*2];
            
                if ((liTeoricSize[iGroupType] - liGroupPop[iGroupType]) < iGroupSize)
                    	iGroupSize = liTeoricSize[iGroupType] - liGroupPop[iGroupType];
            
                liArrayGenPop[iRank] += iGroupSize;
               	iArrayNumGroups[iRank] ++;
            
                iArrayGProcs[iRank] = (int*) realloc (iArrayGProcs[iRank], (1 + (iArrayNumGroups[iRank] * 2)) * sizeof (int));           
            
                /* Store group type and size */
                iArrayGProcs[iRank][(iArrayNumGroups[iRank]*2)-1] = iGroupType+1;
                        
                /* Workers on Saturday */
                if ( ((iGroupType+1) == 1) && (dProbability < dCompaniesOnSaturday) )
                	iArrayGProcs[iRank][(iArrayNumGroups[iRank]*2)-1] = 5;
                        
                	iArrayGProcs[iRank][(iArrayNumGroups[iRank]*2)] = iGroupSize;
            
            } else {
            
                iGroupSize = (liArrayTeoricPop[iRank] - liArrayGenPop[iRank]);
                liArrayGenPop[iRank] += iGroupSize;
                iArrayNumGroups[iRank] ++;
            
                iArrayGProcs[iRank] = (int*) realloc (iArrayGProcs[iRank], (1 + (iArrayNumGroups[iRank] * 2)) * sizeof (int));           
            
                /* Store group type and size */
                iArrayGProcs[iRank][(iArrayNumGroups[iRank]*2)-1] = iGroupType+1;
                        
                /* Workers on Saturday */
                if ( ((iGroupType+1) == 1) && (dProbability < dCompaniesOnSaturday) )
                    iArrayGProcs[iRank][(iArrayNumGroups[iRank]*2)-1] = 5;
                        
                	iArrayGProcs[iRank][(iArrayNumGroups[iRank]*2)] = iGroupSize;
            	}
        
            	/* Size group counter */
            	liGroupPop[iGroupType] += iGroupSize;
            
            	/* Checking maximum teoric group size exceed */
            	if (liGroupPop[iGroupType] >= liTeoricSize[iGroupType]) {
                	fArrayPercentages[iGroupType] = 0;
            
                	dNewSize = 0;
            
                	for (j = 0; j < iMaxGroups; j ++)
                		dNewSize += fArrayPercentages[j];
            
                	dNewSize = dNewSize * 100;
            
				for (j = 0; j < iMaxGroups; j ++) {
                    fAux = fArrayPercentages[j] * (1/dNewSize) * 100;
                    fArrayPercentages[j] = fAux;
                }
            }
		}
        
        /* Next process */
        iRank ++;
        
        if (iRank == iNumberProcesses)
			iRank = 0;
        
        /* Generated population */
        liAccumulatedPop = 0;
        
        for (j = 0; j < iNumberProcesses; j ++)
            liAccumulatedPop += liArrayGenPop[j];
        
        
    } while (liPopSize > liAccumulatedPop); 
    
    /* Set the number of groups for the process */
    for (iRank = 0; iRank < iNumberProcesses; iRank ++) {
        iArrayGProcs[iRank][0] = iArrayNumGroups[iRank];
        
        printf ("\n  [%i] Groups: %i\n", iRank, iArrayNumGroups[iRank]);
    }

    /* Storage groups file */

	/* Set the global number of groups */
	assert ((fprintf (flStructFile, "%li\n", liGroupNumber[0] + liGroupNumber[1] +liGroupNumber[2] +liGroupNumber[3])) != EOF);

    for (iRank = 0; iRank < iNumberProcesses; iRank ++)
        for (j = 0; j < ((iArrayNumGroups[iRank]*2) + 1); j++)
           	assert ((fprintf (flStructFile, "%i\n", iArrayGProcs[iRank][j])) != EOF);
    
    /* Print the groups configuration */
    printf ("\n  Workers    Groups: %li, \tPopulation: %li",   liGroupNumber[0], liGroupPop[0]);
    printf ("\n  Students   Groups: %li, \tPopulation: %li",   liGroupNumber[1], liGroupPop[1]);
    printf ("\n  Elderly    Groups: %li, \tPopulation: %li",   liGroupNumber[2], liGroupPop[2]);
    printf ("\n  Unemployed Groups: %li, \tPopulation: %li\n", liGroupNumber[3], liGroupPop[3]);
    
	/* Closing the files */
	fclose (flStructFile);

    /* Release requested memory */
    for (iRank = 0; iRank < iNumberProcesses; iRank ++) {
        free (iArrayGProcs[iRank]);
        iArrayGProcs[iRank] = NULL;
    }

    free (iArrayGProcs);
    iArrayGProcs = NULL;
    
    free (iArrayNumGroups);
    iArrayNumGroups = NULL;
    
    free (liArrayGenPop);
    liArrayGenPop = NULL;
    
    free (liArrayTeoricPop);
    liArrayTeoricPop = NULL;
        
	gsl_rng_free (r);
        
    return (0);
}

/****************************************************************************************************************************************
*
*	'GetGraphGroups'
*
*	Utility: 
*	 generate an array with the type and size of groups in a given process.
*
****************************************************************************************************************************************/
int GetGraphGroups	( int		iRank
					, int*		iNumberGroups
					, int**		iArrayGroups
					, int*		iGlobalGroups
					, char*		cPathStructure )
{
	FILE 	*flFileGraph	= NULL;	/* pointer to the graph file */

	char 	cFileGraphName	[128],	/* graph file name/path */
			cC;						/* character */
	
	int  	iNumberGroupsAux= 0,	/* auxiliar number of groups */
			iProcess		= 0,	/* process rank counter */
			i				= 0,	/* counter */
			iAux			= 0;	/* auxiliary value */

	/* File name */
	sprintf (cFileGraphName, "%s%s", cPathStructure, "structure.dat");

	/* Opening the file */
	if ( (flFileGraph = fopen (cFileGraphName, "r")) != NULL) {

		/* Get the number of graphs stored */
		assert ((fscanf (flFileGraph, "%i%c", iGlobalGroups, &cC)) != EOF);

		for (iProcess = 0; iProcess <= iRank; iProcess ++) {

			/* Get the number of graphs stored */
			assert ((fscanf (flFileGraph, "%i%c", &iNumberGroupsAux, &cC)) != EOF);

			if (iProcess == iRank) {

				/* Memory request */
				assert ((*iArrayGroups = (int*) malloc ((iNumberGroupsAux*2) * sizeof(int)))!=NULL);

				/* Get the (process) number of graphs stored */
				*iNumberGroups = iNumberGroupsAux;

				for (i = 0; i < (iNumberGroupsAux * 2); i ++)
					/* Get the type and size of every graph stored */
					assert ((fscanf (flFileGraph, "%i%c", &(*iArrayGroups)[i], &cC)) != EOF);
			}

			/* Discart information from another processes */
			else
				for (i = 0; i < (iNumberGroupsAux * 2); i ++)
					assert ((fscanf (flFileGraph, "%i%c", &iAux, &cC)) != EOF);
		}
	
		/* Close the file */
		fclose (flFileGraph);	

		return (0);

	} else {

		printf ("\n**ERROR**: opening the file '%s'\n", cFileGraphName);
		exit (-1);
	}
}

/****************************************************************************************************************************************
*
*	'LoadGroupGraph'
*
*	Utility: 
*	 retrieve data from the graph of a particular group or relationships graph.
*
****************************************************************************************************************************************/
int LoadGroupGraph	( int			iIdGraph
					, int			iCType
					, long int**	liArrayNCol
					, long int**	liArrayNRow
					, long int*		liNCol
					, long int*		liNRow
					, long int*		liNNZ
					, char*			cPathSource
					, float**		fArrayNNZ)
{
	FILE   		*flFileNCol = NULL,		/* NCol File */
	     		*flFileNRow = NULL,		/* NRow File */
	     		*flFileNNZ = NULL;		/* NNZ File */

	char 		cFileMatrixName[128]	/* Matrix file name */
	     		,cFileNColName[128]		/* NCol file name */
	    		,cFileNRowName[128] 	/* NRow file name */
	     		,cFileNNNZName[128]		/* NNZ file name */
	     		,cC;					/* character */

	int  		i = 0;					/* counter */

	xmlDocPtr 	xXmlDoc;				/* Pointer to the XML config file */
	xmlNodePtr 	xRoot;					/* Pointer to the file root node */
	xmlChar 	*xValue;				/* Parameter values */

	/* Get the relationships graph */
	if (iIdGraph == -1) {
		if (iCType) {
			/* strong connections graph */
			sprintf (cFileMatrixName,	"%s%s", cPathSource, "data/graphs/acq/strong.xml");
			sprintf (cFileNColName, 	"%s%s", cPathSource, "data/graphs/acq/ncol_strong.dat");
			sprintf (cFileNRowName, 	"%s%s", cPathSource, "data/graphs/acq/nrow_strong.dat");
			sprintf (cFileNNNZName, 	"%s%s", cPathSource, "data/graphs/acq/nnz_strong.dat");
		} else {
			/* weak connections graph */
			sprintf (cFileMatrixName, 	"%s%s", cPathSource, "data/graphs/acq/weak.xml");
			sprintf (cFileNColName, 	"%s%s", cPathSource, "data/graphs/acq/ncol_weak.dat");
			sprintf (cFileNRowName, 	"%s%s", cPathSource, "data/graphs/acq/nrow_weak.dat");
			sprintf (cFileNNNZName, 	"%s%s", cPathSource, "data/graphs/acq/nnz_weak.dat");
		}
	}
	else {			
		/* file_name = 'matrix_iIdGraph.dat' */
		sprintf (cFileMatrixName, 	"%s%s%d%s", cPathSource, "data/graphs/social/matrix_", iIdGraph, ".xml");

		/* file_name = 'ncol_iIdGraph.dat' */
		sprintf (cFileNColName, 	"%s%s%d%s", cPathSource, "data/graphs/social/ncol_", iIdGraph, ".dat");

		/* file_name = 'nrow_iIdGraph.dat' */
		sprintf (cFileNRowName, 	"%s%s%d%s", cPathSource, "data/graphs/social/nrow_", iIdGraph, ".dat");
	}
	
	/* Get matrix configuration file */
	if ((xXmlDoc = xmlParseFile (cFileMatrixName)) != NULL) {
		/* Root node reference */
		assert((xRoot = xmlDocGetRootElement (xXmlDoc)) != NULL);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ncol")) != NULL);
		*liNCol = atol ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "nrow")) != NULL);
		*liNRow = atol ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "nnz")) != NULL);
		*liNNZ = atol ((const char *) xValue);
		
		xmlFree (xValue);
	}
	else {
		printf ("\n**ERROR**: opening the file '%s'\n", cFileMatrixName);
		exit (-1);
	}

	/* Get data from the NNZ file */
	if (iIdGraph == -1) {
		if ((flFileNNZ = fopen (cFileNNNZName, "r")) != NULL) {
			/* Memory request */
			assert ((*fArrayNNZ = (float*) malloc ((*liNNZ) * sizeof (float))) != NULL);

			/* Get data */
			for (i = 0; i < *liNNZ; i ++)
				assert ((fscanf (flFileNNZ, "%f%c", &(*fArrayNNZ)[i], &cC)) != EOF);
		
			/* Closing file */
			fclose (flFileNNZ);
		}
		else {
			printf ("\n**ERROR**: opening the file '%s'\n", cFileNNNZName);
			exit (-1);
		}
	}

	/* Get data from the NCOL file */
	if ((flFileNCol = fopen (cFileNColName, "r")) != NULL) {
		/* Memory request */
		assert ((*liArrayNCol = (long int*) malloc ((*liNCol+1) * sizeof (long int))) != NULL);

		/* Get data */
		for (i = 0; i <= *liNCol; i ++)
			assert ((fscanf (flFileNCol, "%li%c", &(*liArrayNCol)[i], &cC)) != EOF);
		
		/* Closing file */
		fclose (flFileNCol);
	}
	else {
		printf ("\n**ERROR**: opening the file '%s'\n", cFileNColName);
		exit (-1);
	}

	/* Get data from the NROW file */
	if ((flFileNRow = fopen (cFileNRowName, "r")) != NULL) {
		/* Memory request */
		assert ((*liArrayNRow = (long int*) malloc ((*liNNZ) * sizeof (long int))) != NULL);

		/* Get data */
		for (i = 0; i < *liNNZ; i ++)
			assert ((fscanf (flFileNRow, "%li%c", &(*liArrayNRow)[i], &cC)) != EOF);

		/* Closing file */
		fclose (flFileNRow);
	}
	else {
		printf ("\n**ERROR**: opening the file '%s'\n", cFileNRowName);
		exit (-1);
	}

	return (0);
}

/****************************************************************************************************************************************
*
*	'GenerateInternalConnections'
*
*	Utility: 
*	 generate and load simulation groups in the matrix.
*
****************************************************************************************************************************************/
int GenerateInternalConnections	( int		iMyRank
								, int**		iArrayCol
								, int**		iArrayRow
								, int		iNumberGraphs
								, int		iNumberGroups
								, int		iIdGraphWorkers
								, int		iIdGraphStudents
								, int		iIdGraphElderly
								, int		iIdGraphUnemployed
								, long int* liOffset
								, long int*	liOffsetLimit
								, int*		iArrayGroups
								, long int	liLimitLow
								, long int	liLimitUp
								, int**     iArrayVal
								, float		fThreshold
								, char*		cPathSource
								, double 	dTimeini
		 						, FILE		*flLogfile )
{
	long int liNumberIndividuals	= 0		/* number of individuals in the group */
			,liRandCol				= 0 	/* random column */
			,liNumberRelations		= 0		/* number of individual relationships */
			,liNumberIndividualsRank= 0		/* number of individuals of the process */
			,liLimitLowGroup		= 0;	/* lower limit of the process */

	int 	iIdGraph				= 0		/* group type */
			,iGenRow				= 0
			,iNGroup				= 0 	/* group number */
			,iNIndividual			= 0		/* individual number */
			,iNRelation				= 0		/* relationship number */
			,iArrayIdGraph[5]		= {iIdGraphWorkers, iIdGraphStudents, iIdGraphElderly, iIdGraphUnemployed, iIdGraphWorkers} /* source graph Id */
			,i						= 0;	/* counter */

	double	dProbability 			= 0
			,dTimenow				= 0
			,dTimeStamp				= 0;

	/* type_graph structure */
	struct	type_graph {
		long int	*liArrayNCol;
		long int	*liArrayNRow;
		float		*fArrayNNZ;
		long int	liNCol;
		long int	liNRow;
		long int	liNNZ;
	};

	struct type_graph *tgArrayGraphs = NULL;
	
	/* GSL random number generator */
	gsl_rng *r;
	r = gsl_rng_alloc (gsl_rng_ranlxs2);
    gsl_rng_set (r, getpid());
	
	/* Calculate the number of individuals of the process */
	liNumberIndividualsRank = (liLimitUp - liLimitLow) + 1;

	/* Memory request */
	assert ((tgArrayGraphs = (struct type_graph *) malloc (iNumberGraphs * sizeof (struct type_graph))) != NULL);

	/* Loading graph data */
	for (i = 0; i < iNumberGraphs; i ++)
		LoadGroupGraph (i+1, 0, &tgArrayGraphs[i].liArrayNCol, &tgArrayGraphs[i].liArrayNRow, 
		&tgArrayGraphs[i].liNCol, &tgArrayGraphs[i].liNRow, &tgArrayGraphs[i].liNNZ, cPathSource, &tgArrayGraphs[i].fArrayNNZ);

	/* Generate the groups */
	for (iNGroup = 0; iNGroup < iNumberGroups; iNGroup ++) {

		/* Get the group type */
		iIdGraph = iArrayGroups[2*iNGroup];
        
		/* Get the group size */
		liNumberIndividuals = iArrayGroups[1+(2*iNGroup)];

		/* Get the lower limit of the group */
		liLimitLowGroup = 0;
		
		for (i = 1; i < (iNGroup * 2); i = i+2)
			liLimitLowGroup += iArrayGroups[i];

		/* Generate individuals */
		for (iNIndividual = 0; iNIndividual < liNumberIndividuals; iNIndividual ++) {

			do {
				/* Get random column */
				dProbability = (double) gsl_rng_get(r)/gsl_rng_max(r);
				
				liRandCol = dProbability * tgArrayGraphs[iArrayIdGraph[iIdGraph-1]-1].liNCol;

				/* Get number of relationships from the source graph */
				liNumberRelations = tgArrayGraphs[iArrayIdGraph[iIdGraph-1]-1].liArrayNCol[liRandCol+1] - 
				tgArrayGraphs[iArrayIdGraph[iIdGraph-1]-1].liArrayNCol[liRandCol];
                
               	/* Patch if group.size == 1 */
                if (liNumberIndividuals == 1)
                    liNumberRelations = 0;

			} while ( (liNumberIndividuals * fThreshold) < liNumberRelations );

			/* Generate the relationships */
			for (iNRelation = 0; iNRelation < liNumberRelations; iNRelation ++) {

                /* Check free space in the arrays */
                if (*liOffset == *liOffsetLimit) {
                                  
                    /* LOGFILE data */
					dTimenow = MPI_Wtime();
					dTimeStamp = dTimenow - dTimeini;
					assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] ALERT::: Offset limit reached!!!\n", dTimeStamp, iMyRank)) != EOF);
					fflush (flLogfile);
                                  
                   	/* Resize the arrays */
                    assert (*iArrayRow = (int*) realloc (*iArrayRow, (10000 + *liOffsetLimit) * sizeof (int)));
                    assert (*iArrayCol = (int*) realloc (*iArrayCol, (10000 + *liOffsetLimit) * sizeof (int)));
                    assert (*iArrayVal = (int*) realloc (*iArrayVal, (10000 + *liOffsetLimit) * sizeof (int)));
                    
                    *liOffsetLimit += 10000;
                }

				iGenRow = (int)(( floor ((liNumberIndividuals * 
				tgArrayGraphs[iArrayIdGraph[iIdGraph-1]-1].liArrayNRow[liRandCol+iNRelation]) / 
				tgArrayGraphs[iArrayIdGraph[iIdGraph-1]-1].liNRow) ) + liLimitLowGroup + liLimitLow);
				
				if (iGenRow != (liLimitLow + liLimitLowGroup + iNIndividual)) {
				
					/* Storage the ROW value */
					(*iArrayRow)[*liOffset] = iGenRow;

					/* Storage the COL value */
					(*iArrayCol)[*liOffset] = liLimitLow + liLimitLowGroup + iNIndividual;

					/* Storage the VAL value */
		            (*iArrayVal)[*liOffset] = WORK;

					*liOffset += 1;

					/* Storage the ROW value */
					(*iArrayRow)[*liOffset] = (*iArrayCol)[*liOffset-1];

					/* Storage the COL value */
					(*iArrayCol)[*liOffset] = (*iArrayRow)[*liOffset-1];

					/* Storage the VAL value */
		            (*iArrayVal)[*liOffset] = WORK;
				
					*liOffset += 1;
				}
			}
		}
	}

	/* Release memory */
	for (i = 0; i < iNumberGraphs; i ++) {
		free(tgArrayGraphs[i].liArrayNCol);
		free(tgArrayGraphs[i].liArrayNRow);
	}
	
	gsl_rng_free (r);

	return (0);
}

/****************************************************************************************************************************************
*
*	'GenerateExternalConnections'
*
*	Utility: 
*	 generate external connections between individuals from different groups.
*
****************************************************************************************************************************************/
int GenerateExternalConnections	( int		iConnectionType
								, int**		iArrayCol
								, int**		iArrayRow
								, int		iMyRank
								, int		iNumberProcesses
								, int		iNumberGroupsA
								, long int	liPopSize
								, long int* liOffset
								, long int*	liOffsetLimit
								, int* 		iArrayGroupsA
								, long int	liLimitLowA
								, long int	liLimitUpA
								, int**     iArrayVal
								, double*	dHousehold
								, float		fProbabilityGroup
								, char*		cPathStructure
								, char*		cPathSource
								, double 	dTimeini
		 						, FILE		*flLogfile )
{

	long int 	*liArrayNCol		= NULL	/* dynamic array of NCOL graph data */
				,*liArrayNRow		= NULL	/* dynamic array of NROW graph data */
				,liLimitLowB		= 0		/* lower limit */
				,liLimitUpB			= 0		/* upper limit */
				,liGroupLimitA		= 0		/* lower group limit */
				,liGroupLimitB		= 0		/* lower group limit */
				,liNCol				= 0		/* number of columns */
				,liNRow				= 0		/* number of rows */
				,liNNZ				= 0		/* number of non zero elements */
				,liRandCol			= 0		/* column index */
				,liRandRow			= 0		/* row index */
				,liNumberIndividualsA= 0	/* number of individuals of the group */
				,liNumberIndividualsB= 0	/* number of individuals of the group */
				,liMinSize			= 0;	/* minimal size */

	int 		iNumberGroupsB 		= 0		/* number of group */
				,*iArrayGroupsB		= NULL 	/* dynamic array of group data */
				,iIniGroupA			= 0		/* initial group A */
				,iIniGroupB			= 0		/* initial group B */
				,iTopGroupA			= 0		/* top group A */
				,iTopGroupB			= 0		/* top group B */
				,iGroupTypeA		= 0		/* size of group */
				,iGroupTypeB 		= 0		/* size of group */
				,iGlobalGroups		= 0
				,iNumberProcess		= 0		/* counter */
				,i					= 0		/* counter */
				,j					= 0		/* counter */
				,k					= 0;	/* counter */

	float		*fArrayNNZ	 		= NULL;	/* dynamic array of NNZ graph data */

	double 		dNumberRelations	= 0		/* number of relationships */
				,dProbability		= 0		/* probability */
				,dTimenow			= 0
				,dTimeStamp			= 0;

	/* GSL random number generator */
	gsl_rng *r;
	r = gsl_rng_alloc (gsl_rng_ranlxs2);
    gsl_rng_set (r, getpid());

	if (iConnectionType == 0)
		/* Load weak connection relationships graph */
		LoadGroupGraph (-1, 0, &liArrayNCol, &liArrayNRow, &liNCol, &liNRow, &liNNZ, cPathSource, &fArrayNNZ);
	else
		/* Load strong connection relationships graph */
		LoadGroupGraph (-1, 1, &liArrayNCol, &liArrayNRow, &liNCol, &liNRow, &liNNZ, cPathSource, &fArrayNNZ);

    if (fProbabilityGroup > 0) {
    
        for (iNumberProcess = 0; iNumberProcess < iNumberProcesses; iNumberProcess ++) {
            
			if (iMyRank != iNumberProcess) {
			
				/* Relationships between groups within distinct processes */

 				/* LOGFILE data */
				dTimenow = MPI_Wtime();
				dTimeStamp = dTimenow - dTimeini;
				assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Generating connections with process %i\n", dTimeStamp, iMyRank, iNumberProcess)) 
				 != EOF);
				fflush (flLogfile);

				/* Load group data from process B */ 
				GetGraphGroups (iNumberProcess, &iNumberGroupsB, &iArrayGroupsB, &iGlobalGroups, cPathStructure);

				/* Caculate the limits of the process B */
				CalcLimits (iNumberProcess, iNumberProcesses, liPopSize, &liLimitLowB, &liLimitUpB);

				liGroupLimitA = 0;

				/* Each process computes the half number of the groups */
				if (iMyRank < iNumberProcess) {
					iIniGroupA = 0;
					iTopGroupA = (iNumberGroupsA / 2);

					iIniGroupB = 0;
					iTopGroupB = iNumberGroupsB;

				} else {
					iIniGroupA = 0;
					iTopGroupA = iNumberGroupsA;

					iIniGroupB = (iNumberGroupsB / 2);
					iTopGroupB = iNumberGroupsB;

				}

				for (i = iIniGroupA; i < iTopGroupA; i ++) {
					/* Get the group type */
					iGroupTypeA = iArrayGroupsA[i*2];
				
					/* Get the number of individuals of the group */
					liNumberIndividualsA = iArrayGroupsA[(i*2)+1];
				
					liGroupLimitB = 0;
				
					for (j = iIniGroupB; j < iTopGroupB; j ++) {

						/* Calculate the probability of connections */
						dProbability = (double) gsl_rng_get (r)/gsl_rng_max(r);

						/* Get the group type */
						iGroupTypeB = iArrayGroupsB[j*2];

						/* Get the number of individuals of the group */
						liNumberIndividualsB = iArrayGroupsB[(j*2)+1];

						if (dProbability <= fProbabilityGroup) {

							/* Calculate the minimal size between the groups */
							if (liNumberIndividualsA <= liNumberIndividualsB)
								liMinSize = liNumberIndividualsA;
							else
								liMinSize = liNumberIndividualsB;

							/* Get the relationships percentage */
							for (k=liArrayNCol[iGroupTypeA-1]; k < liArrayNCol[iGroupTypeA]; k ++)
								if (liArrayNRow[k] == (iGroupTypeB-1)) {
									dNumberRelations = fArrayNNZ[k];
									k = liArrayNCol[iGroupTypeA];
								}

							/* Calculate the number of relationships and insert it on the matrix in a random way */
							dNumberRelations = ceil (dNumberRelations * liMinSize);

							for (k = 0; k < dNumberRelations; k ++) {

								liRandCol = (((double) gsl_rng_get (r)/gsl_rng_max(r)) * (liNumberIndividualsA-1)) + liLimitLowA + liGroupLimitA;
								
								liRandRow = (((double) gsl_rng_get (r)/gsl_rng_max(r)) * (liNumberIndividualsB-1)) + liLimitLowB + liGroupLimitB;
                            
                            	/* Check free space in the arrays */
                            	if (*liOffset == *liOffsetLimit) {
                            	                 
                            	    /* LOGFILE data */
									dTimenow = MPI_Wtime();
									dTimeStamp = dTimenow - dTimeini;
									assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] ALERT::: Offset limit reached!!!\n", dTimeStamp, iMyRank)) != EOF);
									fflush (flLogfile);
                            	                 
                                	/* Resize the arrays */
                                	assert (*iArrayRow = (int*) realloc (*iArrayRow, (10000 + *liOffsetLimit) * sizeof (int)));
                                	assert (*iArrayCol = (int*) realloc (*iArrayCol, (10000 + *liOffsetLimit) * sizeof (int)));
                                	assert (*iArrayVal = (int*) realloc (*iArrayVal, (10000 + *liOffsetLimit) * sizeof (int)));
                                
                                	*liOffsetLimit += 10000;
                            	}

								/* Storage */
								(*iArrayCol)[*liOffset] = liRandCol;
								(*iArrayRow)[*liOffset] = liRandRow;
								(*iArrayVal)[*liOffset] = LEISURE;
				
								*liOffset += 1;
							}
						}
						liGroupLimitB += liNumberIndividualsB;
					}
					liGroupLimitA += liNumberIndividualsA;
				}

				/* Memory release */
				free (iArrayGroupsB);
				iArrayGroupsB = NULL;
				
			} else {
			
				/* Relationships between groups within the same process */

 				/* LOGFILE data */
				dTimenow = MPI_Wtime();
				dTimeStamp = dTimenow - dTimeini;
				assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Generating connections in my process\n", dTimeStamp, iMyRank)) != EOF);
				fflush (flLogfile);

				liGroupLimitA = 0;
			
				for (i = 0; i < iNumberGroupsA; i ++) {

					/* Get the group type */
					iGroupTypeA = iArrayGroupsA[i*2];
					
					/* Get the number of individuals of the group */
					liNumberIndividualsA = iArrayGroupsA[(i*2)+1];
				
					liGroupLimitB = 0;
				
					for (j = 0; j < iNumberGroupsA; j ++) {

						if ((i != j) && (j > i)) {

							/* Calculate the probability of connections */
							dProbability = (double) gsl_rng_get (r)/gsl_rng_max(r);

							/* Get the group type */
							iGroupTypeB = iArrayGroupsA[j*2];
					
							/* Get the number of individuals of the group */
							liNumberIndividualsB = iArrayGroupsA[(j*2)+1];

							if (dProbability <= fProbabilityGroup) {

								/* Calculate the minimal size between the groups */
								if (liNumberIndividualsA <= liNumberIndividualsB)
									liMinSize = liNumberIndividualsA;
								else
									liMinSize = liNumberIndividualsB;

								/* Get the relationships percentage */
								for (k=liArrayNCol[iGroupTypeA-1]; k < liArrayNCol[iGroupTypeA]; k ++)
									if (liArrayNRow[k] == (iGroupTypeB-1)) {
										dNumberRelations = fArrayNNZ[k];
										k = liArrayNCol[iGroupTypeA];
									}

								/* Calculate the number of relationships and insert it on the matrix in a random way */ 
								dNumberRelations = ceil (dNumberRelations * liMinSize);

								for (k = 0; k < dNumberRelations; k ++) {

									liRandCol = (((double) gsl_rng_get (r)/gsl_rng_max(r)) * (liNumberIndividualsA-1)) + liLimitLowA + liGroupLimitA;
									
									liRandRow = (((double) gsl_rng_get (r)/gsl_rng_max(r)) * (liNumberIndividualsB-1)) + liLimitLowA + liGroupLimitB;

                                	/* Check free space in the arrays */
                                	if (*liOffset == *liOffsetLimit) {  
                                	                  
                                	    /* LOGFILE data */
										dTimenow = MPI_Wtime();
										dTimeStamp = dTimenow - dTimeini;
										assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] ALERT::: Offset limit reached!!!\n", dTimeStamp, iMyRank)) 
										 != EOF);
										fflush (flLogfile);
                                	                  
                                    	/* Resize the arrays */
                                   		assert (*iArrayRow = (int*) realloc (*iArrayRow, (10000 + *liOffsetLimit) * sizeof (int)));
                                   		assert (*iArrayCol = (int*) realloc (*iArrayCol, (10000 + *liOffsetLimit) * sizeof (int)));
                                    	assert (*iArrayVal = (int*) realloc (*iArrayVal, (10000 + *liOffsetLimit) * sizeof (int)));
                                    
                                    	*liOffsetLimit += 10000;
                               		}

									/* Storage */
									(*iArrayCol)[*liOffset] = liRandCol;
									(*iArrayRow)[*liOffset] = liRandRow;
									(*iArrayVal)[*liOffset] = LEISURE;

									*liOffset += 1;

									/* Storage */
									(*iArrayCol)[*liOffset] = liRandRow;
									(*iArrayRow)[*liOffset] = liRandCol;
									(*iArrayVal)[*liOffset] = LEISURE;

									*liOffset += 1;
								}
							}
						
							liGroupLimitB += liNumberIndividualsB;
						}
					}
				
					liGroupLimitA += liNumberIndividualsA;
				}		
			}
		}
    }
        
	/* Memory release */
	free (liArrayNCol);
	liArrayNCol = NULL;
	free (liArrayNRow);
	liArrayNRow = NULL;
	free (fArrayNNZ);
	fArrayNNZ = NULL;

	gsl_rng_free (r);

	return (0);
}

/****************************************************************************************************************************************
*
*	'GenerateFamiliarConnections'
*
*	Utility: 
*	 generate familiar connections between individuals.
*
****************************************************************************************************************************************/
int GenerateFamiliarConnections		( int**		iArrayCol
									, int**		iArrayRow
									, int		iMyRank
									, int		iNumberProcesses
									, long int	liPopSize
									, long int* liOffset
									, long int*	liOffsetLimit
									, long int	liLimitLow
									, long int	liLimitUp
									, int**		iArrayVal
									, double*	dHousehold
									, double 	dTimeini
		 							, FILE		*flLogfile )
{
	int		*iArrayFamiliar 		= NULL
			,*iArrayFamiliarAux 	= NULL
			,iNumberFamiliar[4]    	= {0, 0, 0, 0}
			,iNumberFamiliarAux[4]	= {0, 0, 0, 0}
			,*iArrayOne				= NULL
			,*iArrayTwo				= NULL
			,*iArrayThree			= NULL
			,*iArrayFour			= NULL
			,iIndexOne				= 0
			,iIndexTwo				= 0
			,iIndexThree			= 0
			,iIndexFour				= 0
           	,iCalcCode     			= 0
			,iRound					= 0;

	long int i						= 0
			,j						= 0
			,k						= 0
            ,liArrayAux[5]  		= {0, 0, 0, 0, 0}
			,liIndex				= 0;
	
	double	dProbability			= 0
			,dTimenow				= 0
			,dTimeStamp				= 0;
    
	/* GSL random number generator */
	gsl_rng *r;
	r = gsl_rng_alloc (gsl_rng_ranlxs2);
    gsl_rng_set (r, getpid());

    assert ((iArrayFamiliar     = 	(int*) malloc (liPopSize*sizeof(int))) != NULL);
    assert ((iArrayFamiliarAux  = 	(int*) malloc (liPopSize*sizeof(int))) != NULL);
    
	/* Set the size of the familiar unit for each individual */
	for (i = 0; i < liPopSize; i ++) {
	
		if ((i >= liLimitLow) && (i <= liLimitUp)) {
		
			dProbability = (double) gsl_rng_get (r)/gsl_rng_max(r);

			if (dProbability < dHousehold[0])
				iArrayFamiliarAux[i] = -1;
			else	if ((dProbability >= dHousehold[0]) && 
				(dProbability < dHousehold[0]+dHousehold[1]))
					iArrayFamiliarAux[i] = 1;
				else	if ((dProbability >= dHousehold[0]+dHousehold[1])&& 
				   	(dProbability < dHousehold[0]+dHousehold[1]+dHousehold[2]))
						iArrayFamiliarAux[i] = 2;
					else	if ((dProbability >= dHousehold[0]+dHousehold[1]+dHousehold[2])&& 
				   		(dProbability < dHousehold[0]+dHousehold[1]+dHousehold[2]+dHousehold[3]))
							iArrayFamiliarAux[i] = 3;
						else	if (dProbability >= dHousehold[0]+dHousehold[1]+dHousehold[2]+dHousehold[3])
								iArrayFamiliarAux[i] = 4;
	
			/* Number of familiar relationships for every type {1, 2, 3, 4} */
			if (iArrayFamiliarAux[i] > 0)
			
				iNumberFamiliarAux[iArrayFamiliarAux[i]-1] ++;
		} else
			/* Individual from another process */
			iArrayFamiliarAux[i] = -1;
	}

	/* Communicate the number of familiar relationships */
	MPI_Allreduce (iNumberFamiliarAux, iNumberFamiliar, 4, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

	/* Communicate familiar relationships */
	MPI_Allreduce (iArrayFamiliarAux, iArrayFamiliar, liPopSize, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    /* Binary encoding: processes tasks */
    // 0000(0): processing of two people familiar units
    // 0001(1): processing of two people familiar units
    // 0010(2): processing of three people familiar units
    // 0011(3): processing of two and three people familiar units
    // 0100(4): processing of four people familiar units
    // 1000(8): processing of five people familiar units
    // 1100(12):processing of four and five people familiar units
    // 1111(15):processing of two, three, four and five people familiar units
    
    if (iNumberProcesses == 1)
       	iCalcCode = 15;
    else 	if ((iNumberProcesses == 2)||(iNumberProcesses == 3 )) {
            	if (iMyRank == MASTER)
               		iCalcCode = 3;
            	if (iMyRank == 1)
                	iCalcCode = 12;  
        	} else 	if (iNumberProcesses > 3) {
            	if (iMyRank == MASTER)
                	iCalcCode = 1;
            	if (iMyRank == 1)
                	iCalcCode = 2;
            	if (iMyRank == 2)
                	iCalcCode = 4;
            	if (iMyRank == 3)
               	 	iCalcCode = 8;
    }
    
	/* Familiar units processing */
	if (iCalcCode  != 0 ) {
	
		/* Memory request */
		assert ((iArrayOne = 	(int*) malloc (iNumberFamiliar[0]*sizeof(int))) != NULL);
		assert ((iArrayTwo = 	(int*) malloc (iNumberFamiliar[1]*sizeof(int))) != NULL);
		assert ((iArrayThree = 	(int*) malloc (iNumberFamiliar[2]*sizeof(int))) != NULL);
		assert ((iArrayFour = 	(int*) malloc (iNumberFamiliar[3]*sizeof(int))) != NULL);

        /* Categorize the individuals according to the familiar unit size */
       	for (i = 0; i < liPopSize; i ++) {
            switch (iArrayFamiliar[i]) {
				case 1:
					iArrayOne[iIndexOne] = i;
					iArrayFamiliar[i] = -1;
					iIndexOne++;
					break;
				case 2:
					iArrayTwo[iIndexTwo] = i;
					iArrayFamiliar[i] = -1;
					iIndexTwo++;
					break;
				case 3:
					iArrayThree[iIndexThree] = i;
					iArrayFamiliar[i] = -1;
					iIndexThree++;
					break;
				case 4:
					iArrayFour[iIndexFour] = i;
					iArrayFamiliar[i] = -1;
					iIndexFour++;
					break;
			}				
		}

        /* Familiar units computation */

        /* Two people familiar unit */
       	if ((iCalcCode == 1)||(iCalcCode == 3)||(iCalcCode == 15)) {

			/* iRound is the number of groups based on the unit size */
			iRound = iNumberFamiliar[0]/2;

         	/* Formar nucleos familiares */
			for (i = 0; i < iRound; i ++) {
                
                for (j = 0; j < 2; j++) {
					/* Select random individual */
                    liIndex = ((double) gsl_rng_get(r)/gsl_rng_max(r)) * (iIndexOne-1);
					/* Store individual */
                    liArrayAux[j] = iArrayOne[liIndex];
					/* If the selected individual is not in the last position of the array we have to move the last one to this position */
                    if (liIndex < iIndexOne-1)
                       	iArrayOne[liIndex] = iArrayOne[iIndexOne-1];
					/* Drop the last one individual */
                    iArrayOne[iIndexOne-1] = -1;
                    iIndexOne--;
                }
                
                iArrayFamiliar[liArrayAux[0]] = liArrayAux[1];
                iArrayFamiliar[liArrayAux[1]] = liArrayAux[0];
                 
                for (j = 0; j < 5; j ++)
                    liArrayAux[j] = -1;
			}

			/* Individuos extra quedan sin formar nucleo familiar */
			if (iNumberFamiliar[0] % 2 != 0) {
				iArrayFamiliar[iArrayOne[iIndexOne-1]] = -1;
				iIndexOne--;
			}
        }

        /* Three people familiar unit */
        if ((iCalcCode == 2)||(iCalcCode == 3)||(iCalcCode == 15)) {

			iRound = iNumberFamiliar[1]/3;

        	/* Formar nucleos familiares */
			for (i = 0; i < iRound; i ++) {
                
        		for (j = 0; j < 3; j++) {
            		liIndex = ((double) gsl_rng_get (r)/gsl_rng_max(r)) * (iIndexTwo-1);
                	liArrayAux[j] = iArrayTwo[liIndex];
                	if (liIndex < iIndexTwo-1)
                		iArrayTwo[liIndex] = iArrayTwo[iIndexTwo-1];
                   	iArrayTwo[iIndexTwo-1] = -1;
                    iIndexTwo--;
                }
                 
                for (j = 0; j < 2; j ++)
                   	iArrayFamiliar[liArrayAux[j]] = liArrayAux[j+1];

                iArrayFamiliar[liArrayAux[2]] = liArrayAux[0];
                 
                for (j = 0; j < 5; j ++)
                    liArrayAux[j] = -1;
			}

			/* Individuos extra quedan sin formar nucleo familiar */
			if (iNumberFamiliar[1] % 3 != 0)
				for (i = 0; i < iNumberFamiliar[1] % 3; i ++) {
					iArrayFamiliar[iArrayTwo[iIndexTwo-1]] = -1;
					iIndexTwo--;
				}
        }

        /* Four people familiar unit */
       	if ((iCalcCode == 4)||(iCalcCode == 12)||(iCalcCode == 15)) {

			iRound = iNumberFamiliar[2]/4;

            /* Formar nucleos familiares */
			for (i = 0; i < iRound; i ++) {
                
                for (j = 0; j < 4; j++) {
            		liIndex = ((double) gsl_rng_get (r)/gsl_rng_max(r)) * (iIndexThree-1);
                    liArrayAux[j] = iArrayThree[liIndex];
                    if (liIndex < iIndexThree-1)
                        iArrayThree[liIndex] = iArrayThree[iIndexThree-1];
                   	iArrayThree[iIndexThree-1] = -1;
                    iIndexThree--;
               	}
                 
                for (j = 0; j < 3; j ++)
                     iArrayFamiliar[liArrayAux[j]] = liArrayAux[j+1];
                 
                iArrayFamiliar[liArrayAux[3]] = liArrayAux[0];
                 
                for (j = 0; j < 5; j ++)
                    liArrayAux[j] = -1;
			}

			/* Individuos extra quedan sin formar nucleo familiar */
			if (iNumberFamiliar[2] % 4 != 0)
				for (i = 0; i < iNumberFamiliar[2] % 4; i ++) {
					iArrayFamiliar[iArrayThree[iIndexThree-1]] = -1;
					iIndexThree--;
				}
        }

        /* Five people familiar unit */
        if ((iCalcCode == 8)||(iCalcCode == 12)||(iCalcCode == 15)) {

			iRound = iNumberFamiliar[3]/5;

            /* Formar nucleos familiares */
			for (i = 0; i < iRound; i ++) {
                
                for (j = 0; j < 5; j++) {
            		liIndex = ((double) gsl_rng_get (r)/gsl_rng_max(r)) * (iIndexFour-1);
                    liArrayAux[j] = iArrayFour[liIndex];
                    if (liIndex < iIndexFour-1)
                        iArrayFour[liIndex] = iArrayFour[iIndexFour-1];
                    iArrayFour[iIndexFour-1] = -1;
                    iIndexFour--;
                }
                 
                for (j = 0; j < 4; j ++)
                    iArrayFamiliar[liArrayAux[j]] = liArrayAux[j+1];
                 
                iArrayFamiliar[liArrayAux[4]] = liArrayAux[0];
                 
                for (j = 0; j < 5; j ++)
                    liArrayAux[j] = -1;
			}

			/* Individuos extra quedan sin formar nucleo familiar */
			if (iNumberFamiliar[3] % 5 != 0)
				for (i = 0; i < iNumberFamiliar[3] % 5; i ++) {
					iArrayFamiliar[iArrayFour[iIndexFour-1]] = -1;
					iIndexFour--;
				}
        }

		/* Release requested memory */
		free(iArrayOne);
		iArrayOne = NULL;
		free(iArrayTwo);
		iArrayTwo = NULL;
		free(iArrayThree);
		iArrayThree = NULL;
		free(iArrayFour);
		iArrayFour = NULL;
        
	} else
	
        /* Procesos que no intervienen en el computo de los nucleos familiares */
		for (i = 0; i < liPopSize; i ++)
		
			iArrayFamiliar[i] = -1;

    /* LOGFILE data */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Array has been generated, now MPI_Allreduce\n", dTimeStamp, iMyRank)) != EOF);
	fflush (flLogfile);

	/* Communicate number of familiars */
	MPI_Allreduce (iArrayFamiliar, iArrayFamiliarAux, liPopSize, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    /* LOGFILE data */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] MPI_Allreduce reached\n", dTimeStamp, iMyRank)) != EOF);
	fflush (flLogfile);

	/* Release memory */
	free (iArrayFamiliar);
	iArrayFamiliar = NULL;

	/* Search and insert familiar connections */
	for (i = liLimitLow; i <= liLimitUp; i ++) {

		/* Si >0 el individuo forma nucleo familiar */
		if (iArrayFamiliarAux[i] >= 0) {
            
			j = i;

			do {
				k = iArrayFamiliarAux[j];

				if (k != i) {

                    /* Check free space in the arrays */
                    if (*liOffset == *liOffsetLimit) {
                  
                        /* LOGFILE data */
						dTimenow = MPI_Wtime();
						dTimeStamp = dTimenow - dTimeini;
						assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] ALERT::: Offset limit reached!!!\n", dTimeStamp, iMyRank)) != EOF);
						fflush (flLogfile);
                  
                        /* Resize the arrays */
                        assert (*iArrayRow = (int*) realloc (*iArrayRow, (10000 + *liOffsetLimit) * sizeof (int)));
                        assert (*iArrayCol = (int*) realloc (*iArrayCol, (10000 + *liOffsetLimit) * sizeof (int)));
                        assert (*iArrayVal = (int*) realloc (*iArrayVal, (10000 + *liOffsetLimit) * sizeof (int)));
                        
                        *liOffsetLimit += 10000;
                    }

					/* Storage */
					(*iArrayCol)[*liOffset] = i;
					(*iArrayRow)[*liOffset] = k;
					(*iArrayVal)[*liOffset] = FAMILIAR;

					*liOffset += 1;
				}	

				j = k;

			} while (j != i);
		}
	}

	/* GSL free */
    gsl_rng_free (r);

	/* Release memory */
	free (iArrayFamiliarAux);
	iArrayFamiliarAux = NULL;

	return (0);
}

/****************************************************************************************************************************************
*
*	'RemoveDuplicates'
*
*	Utility: 
*	 remove duplicates with value N from the matrix.
*
****************************************************************************************************************************************/
int RemoveDuplicates	( cs_int**	csMatrix )
{
	int n = 0, m = 0, nz = 0, ncon = 0, *dupcol = NULL, **occurs = NULL;
	
	dupcol = (int*)  malloc (((*csMatrix)->m+1)* sizeof (int));
	occurs = (int**) malloc (4 * sizeof (int*));
	
	occurs[0] = (int*) malloc ((*csMatrix)->m* sizeof (int));
	occurs[1] = (int*) malloc ((*csMatrix)->m* sizeof (int));
	occurs[2] = (int*) malloc ((*csMatrix)->m* sizeof (int));
	occurs[3] = (int*) malloc ((*csMatrix)->m* sizeof (int));
	
	memset (occurs[0], -1, (*csMatrix)->m * sizeof (int));
	memset (occurs[1], -1, (*csMatrix)->m * sizeof (int));
	memset (occurs[2], -1, (*csMatrix)->m * sizeof (int));
	memset (occurs[3], -1, (*csMatrix)->m * sizeof (int));
	
	memcpy (dupcol, (*csMatrix)->p, ((*csMatrix)->m+1)*sizeof (int));
	
	for (n = 0; n < (*csMatrix)->m; n ++) {
		
		for (m = dupcol[n]; m < dupcol[n+1]; m ++) {
			
			if (occurs[(*csMatrix)->x[m]-1][(*csMatrix)->i[m]] < n) {
						
				//primera aparicion del individuo
				occurs[(*csMatrix)->x[m]-1][(*csMatrix)->i[m]] = n;
				
				//sobreescribo el contacto
				(*csMatrix)->i[nz] = (*csMatrix)->i[m];
				(*csMatrix)->x[nz] = (*csMatrix)->x[m];
				
				ncon ++;
					
				nz ++;
			}
		}
		
		(*csMatrix)->p[n+1] = (*csMatrix)->p[n] + ncon;
		
		ncon = 0;
	}
	
	cs_sprealloc_int ((*csMatrix), nz);
	
	free (dupcol);
	dupcol = NULL;
	free (occurs[0]);
	occurs[0] = NULL;
	free (occurs[1]);
	occurs[1] = NULL;
	free (occurs[2]);
	occurs[2] = NULL;
	free (occurs[3]);
	occurs[3] = NULL;
	free (occurs);
	occurs = NULL;

	return 0;
}	

/****************************************************************************************************************************************
 *
 *	'GenDistributionMatrix'
 *
 *	Utility: 
 *	 generate a matrix from exponential/poisson/gaussian distribution.
 *
 ****************************************************************************************************************************************/
int GenDistributionMatrix	( int           **iArrayCol
                          	, int           **iArrayRow
                            , int           **iArrayVal
                            , int           iSourceGraph
                            , double        dMuGraph
                            , double        dSigmaGraph
                            , int           iMyRank
                            , long int* 	liOffset
                            , long int*     liOffsetLimit
                            , long int      liLimitLow
                            , long int      liLimitUp
                            , long int      liPopSize )
{
	long int    	liNumCol  		= 0
                	,liNumRow    	= 0;
    
    int         	iNNZ        	= 0;
    
    double      	dRandRow    	= 0;
    
    const       	gsl_rng_type 	* type 	= gsl_rng_ranlxs2;
					gsl_rng     	* rng	= gsl_rng_alloc(type);
    
    gsl_rng_set (rng, getpid());
    
    /* Generar los conocidos para cada individuo de la poblacion */
    for (liNumCol = liLimitLow; liNumCol <= liLimitUp; liNumCol ++) {
        
        /* Seleccionar el numero de conocidos segun la distribucion */
        switch (iSourceGraph) {
            case GAUSSIAN:
                /* Gaussian Distribution */
                iNNZ = (gsl_ran_gaussian(rng, dSigmaGraph) + dMuGraph);
                break;

            case EXPONENTIAL:
                /* Exponential Distribution */
                iNNZ = gsl_ran_exponential (rng, dMuGraph);
                break;

            case POISSON:
                /* Poisson Distribution */
                iNNZ = gsl_ran_poisson (rng, dMuGraph);
                break;
        }
        
        for (liNumRow = 0; liNumRow < iNNZ; liNumRow ++) {

            dRandRow = ceil(((double) gsl_rng_get (rng)/gsl_rng_max(rng)) * (liPopSize-1));
            
            /* Check free space in the arrays */
            if (*liOffset == *liOffsetLimit) {
                                                            
            	/* Resize the arrays */
				assert (*iArrayRow = (int*) realloc (*iArrayRow, (10000 + *liOffsetLimit) * sizeof (int)));
				assert (*iArrayCol = (int*) realloc (*iArrayCol, (10000 + *liOffsetLimit) * sizeof (int)));
				assert (*iArrayVal = (int*) realloc (*iArrayVal, (10000 + *liOffsetLimit) * sizeof (int)));
                    
				*liOffsetLimit += 10000;
			}
            
            /* Storage */
            (*iArrayCol)[*liOffset] = liNumCol;
            (*iArrayRow)[*liOffset] = dRandRow;
            (*iArrayVal)[*liOffset] = WORK;
            
            *liOffset += 1;
        }
    }

	/* Free */
	gsl_rng_free (rng);
    
    return (0);
}

/****************************************************************************************************************************************
*
*	'MergeConnections'
*
*	Utility: 
*	 data communications between different processes.
*
****************************************************************************************************************************************/
int MergeConnections		( int		iNumberProcesses
							, int**		iArrayCol
							, int**		iArrayRow
							, int		iMyRank
							, double	dTimeini
		 					, FILE		*flLogfile
							, long int* liOffset
							, long int	liPopSize
							, long int*	liOffsetLimit
							, int**     iArrayVal )
{

	int			i 			= 0	/* counter */
				,j 			= 0	/* counter */
				,n			= 0
				,aux		= 0
				,iToken		= 0
				,iNumberExt	= 0	/* number of inter-group relationships */
				,*iBuffer	= NULL  /* data buffer */
				,*iCommMatrix= NULL
				,iStartRank	= 0
				,iLastValue	= 0
				,iNumComms	= 0
				,iCommRank	= 0	/* mpi_source and mpi_dest */
				,mpi_tag	= 0;/* mpi tag */
	
	long int 	liOffsetInitial = *liOffset
				,liLimitLow	= 0
				,liLimitUp	= 0;

	double		dTimeStamp	= 0
				,dTimenow	= 0;

	MPI_Status 	status;	  	/* status */

   	/* Set the number of communications */
    if ((iNumberProcesses % 2) == 0) {
        iNumComms = iNumberProcesses - 1;
        iStartRank = iNumberProcesses - 2;
        iLastValue = iNumberProcesses - 1;
    } else {
        iNumComms = iNumberProcesses;
        iStartRank = iNumberProcesses - 1;
        iLastValue = -1;
    }

	/* Request memory for the communications matrix */
	assert ((iCommMatrix  = (int*) calloc (iNumComms * (iNumComms + 1), sizeof (int))) != NULL);
    
    /* Set first row */
    for (n = 0; n <= iStartRank; n ++)
        iCommMatrix[n] = n;
    
    /* Set last value */
    for (n = 0; n < iNumComms; n ++)
        iCommMatrix[((n+1)*iNumComms)+n] = iLastValue;

    /* Complete the matrix */
    for (n = 1; n < iNumComms; n ++) {
        
        aux = iStartRank;
        
        for (i = 0; i < iNumComms; i ++) {
            
            iCommMatrix[(n*(iNumComms+1))+i] = aux;
            
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

    for (n = 0; n < iNumComms; n ++) {
        
        for (i = n * (iNumComms + 1); i < (n * (iNumComms + 1)) + (iNumComms + 1); i ++) {
            
            if (iCommMatrix[i] == iMyRank) {

                iCommRank = iCommMatrix[(iNumComms - (i % (iNumComms + 1))) + (n * (iNumComms + 1))];
                
                if (iCommRank < 0)
    
                    iToken = TOKEN_NOACT;
                    
                else {
                    
                    if ((i % (iNumComms+1)) < CEIL (iNumberProcesses, 2))
                        iToken = TOKEN_SEND;
                    else
                        iToken = TOKEN_RECEIVE;
                }
            }     
        }
        
		if (iToken == TOKEN_SEND) {
		
			/* Send data */
		
			/* Get the matrix limits */
			CalcLimits (iCommRank, iNumberProcesses, liPopSize, &liLimitLow, &liLimitUp);

			iNumberExt = 0;

			for (j = 0; j < liOffsetInitial; j ++)
				if (((*iArrayRow)[j]>=liLimitLow) && ((*iArrayRow)[j]<=liLimitUp))
					iNumberExt ++;

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Sending data to process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Send the number of elements of the buffer */	
			MPI_Send(&iNumberExt, 1, MPI_INT, iCommRank, mpi_tag, MPI_COMM_WORLD);
						
			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Sent data size to process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Memory requesting */
			assert ((iBuffer = (int*) malloc (iNumberExt * 3 * sizeof (int))) != NULL);
					
			iNumberExt = 0;
					
			for (j = 0; j < liOffsetInitial; j ++)

				if (((*iArrayRow)[j]>=liLimitLow) && ((*iArrayRow)[j]<=liLimitUp)) {
					iBuffer[(iNumberExt*3)+0] = (*iArrayVal)[j];
					iBuffer[(iNumberExt*3)+1] = (*iArrayRow)[j];
					iBuffer[(iNumberExt*3)+2] = (*iArrayCol)[j];
					
					iNumberExt++;
				}
					
			/* Send the external relationships data */
			MPI_Send(iBuffer, iNumberExt*3, MPI_INT, iCommRank, mpi_tag, MPI_COMM_WORLD);

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Sent data to process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Release memory */
			free (iBuffer);
			iBuffer = NULL;	
		
			/* Receive data */

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Waiting for receive data from process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);
				
			/* Receive the number of elements of the buffer */
			MPI_Recv(&iNumberExt, 1, MPI_INT, iCommRank, mpi_tag, MPI_COMM_WORLD, &status);

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Received data size from process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Memory request */
			assert ((iBuffer = (int*) malloc (iNumberExt * 3 * sizeof (int))) != NULL);

			/* Receive the external relationships data */
			MPI_Recv(iBuffer, iNumberExt*3, MPI_INT, iCommRank, mpi_tag, MPI_COMM_WORLD, &status);

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Received data from process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Insert data in local matrix */
			/* Invert row and column */
			for (i = 0; i < (iNumberExt * 3); i += 3) {

                /* Check free space in the arrays */
                if (*liOffset == *liOffsetLimit) {
                
                    /* LOGFILE data */
					dTimenow = MPI_Wtime();
					dTimeStamp = dTimenow - dTimeini;
					assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] ALERT::: Offset limit reached!!!\n", dTimeStamp, iMyRank)) != EOF);
					fflush (flLogfile);
                   
                    /* Resize the arrays */
                    assert (*iArrayRow = (int*) realloc (*iArrayRow, (10000 + *liOffsetLimit) * sizeof (int)));
                    assert (*iArrayCol = (int*) realloc (*iArrayCol, (10000 + *liOffsetLimit) * sizeof (int)));
                    assert (*iArrayVal = (int*) realloc (*iArrayVal, (10000 + *liOffsetLimit) * sizeof (int)));
                    
                    *liOffsetLimit += 10000;
                }

				(*iArrayVal)[*liOffset] = iBuffer[i];
				(*iArrayRow)[*liOffset] = iBuffer[i+2];
				(*iArrayCol)[*liOffset] = iBuffer[i+1];

				*liOffset += 1;
			}

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Stored data from process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Release memory */
			free (iBuffer);
			iBuffer = NULL;		
		
		} else if (iToken == TOKEN_RECEIVE) {
		
			/* Receive data */

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Waiting for receive data from process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);
				
			/* Receive the number of elements of the buffer */
			MPI_Recv(&iNumberExt, 1, MPI_INT, iCommRank, mpi_tag, MPI_COMM_WORLD, &status);

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Received data size from process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Memory request */
			assert ((iBuffer = (int*) malloc (iNumberExt * 3 * sizeof (int))) != NULL);

			/* Receive the external relationships data */
			MPI_Recv(iBuffer, iNumberExt*3, MPI_INT, iCommRank, mpi_tag, MPI_COMM_WORLD, &status);

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Received data from process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Insert data in local matrix */
			/* Invert row and column */
			for (i = 0; i < (iNumberExt * 3); i += 3) {

                /* Check free space in the arrays */
                if (*liOffset == *liOffsetLimit) {
                   
                   	/* LOGFILE data */
					dTimenow = MPI_Wtime();
					dTimeStamp = dTimenow - dTimeini;
					assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] ALERT::: Offset limit reached!!!\n", dTimeStamp, iMyRank)) != EOF);
					fflush (flLogfile);
                   
                    /* Resize the arrays */
                    assert (*iArrayRow = (int*) realloc (*iArrayRow, (10000 + *liOffsetLimit) * sizeof (int)));
                    assert (*iArrayCol = (int*) realloc (*iArrayCol, (10000 + *liOffsetLimit) * sizeof (int)));
                    assert (*iArrayVal = (int*) realloc (*iArrayVal, (10000 + *liOffsetLimit) * sizeof (int)));
                    
                    *liOffsetLimit += 10000;
                }

				(*iArrayVal)[*liOffset] = iBuffer[i];
				(*iArrayRow)[*liOffset] = iBuffer[i+2];
				(*iArrayCol)[*liOffset] = iBuffer[i+1];

				*liOffset += 1;
			}

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Stored data from process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Release memory */
			free (iBuffer);
			iBuffer = NULL;
		
			/* Send data */
		
			/* Get the matrix limits */
			CalcLimits (iCommRank, iNumberProcesses, liPopSize, &liLimitLow, &liLimitUp);

			iNumberExt = 0;

			for (j = 0; j < liOffsetInitial; j ++)
				if (((*iArrayRow)[j]>=liLimitLow) && ((*iArrayRow)[j]<=liLimitUp))
					iNumberExt ++;

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Sending data to process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Send the number of elements of the buffer */	
			MPI_Send(&iNumberExt, 1, MPI_INT, iCommRank, mpi_tag, MPI_COMM_WORLD);
						
			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Sent data size to process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Memory requesting */
			assert ((iBuffer = (int*) malloc (iNumberExt * 3 * sizeof (int))) != NULL);
					
			iNumberExt = 0;

			for (j = 0; j < liOffsetInitial; j ++)

				if (((*iArrayRow)[j]>=liLimitLow) && ((*iArrayRow)[j]<=liLimitUp)) {
					iBuffer[(iNumberExt*3)+0] = (*iArrayVal)[j];
					iBuffer[(iNumberExt*3)+1] = (*iArrayRow)[j];
					iBuffer[(iNumberExt*3)+2] = (*iArrayCol)[j];
					
					iNumberExt ++;
				}
					
			/* Send the external relationships data */
			MPI_Send(iBuffer, iNumberExt*3, MPI_INT, iCommRank, mpi_tag, MPI_COMM_WORLD);

			/* LOGFILE data */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%.3f \t- ID_%i] Sent data to process %i\n", dTimeStamp, iMyRank, iCommRank)) != EOF);
			fflush (flLogfile);

			/* Release memory */
			free (iBuffer);
			iBuffer = NULL;	
		}	
	}
	
	/* Release memory */
    free (iCommMatrix);
    iCommMatrix = NULL;

	return (0);
}

/****************************************************************************************************************************************
*
*	'GenPopulationInfo'
*
*	Utility: 
*	 generate population info (age, race, etc.)
*
****************************************************************************************************************************************/
int GenPopulationInfo	( int		iMyRank
						, int		iNumberProcesses
						, int**		iArrayLatentPrimaryPeriod
						, int**		iArrayLatentSecondaryPeriod
						, int**		iArrayInfectivePrimaryPeriod
						, int**		iArrayInfectiveSecondaryPeriod
						, int**		iArrayAntiviralPeriod
						, int**		iArrayAsymptomaticPeriod
						, int**		iArrayHospitalizationPeriod
						, int**     iArrayAntiviralDelayPeriod
						, float		fMuLatentPrimaryPeriod
						, float		fMuLatentSecondaryPeriod
						, float		fMuHospitalizationPeriod
						, float		fSigmaInfectivePrimaryPeriod
						, float		fMuInfectivePrimaryPeriod
						, float		fSigmaInfectiveSecondaryPeriod
						, float		fMuInfectiveSecondaryPeriod
						, float		fSigmaAsymptomaticPeriod
						, float		fMuAsymptomaticPeriod
						, float		fSigmaAntiviralPeriod
						, float		fMuAntiviralPeriod
						, float		fSigmaAntiviralDelayPeriod
						, float		fMuAntiviralDelayPeriod
						, int*		iArrayAges
						, double	dWorkersMale
						, double	dStudentsMale
						, double	dElderlyMale
						, double	dUnemployedMale
						, double	dRaceWhite
						, double	dRaceBlack
						, double	dRaceAmindian
						, double	dRaceAsian
						, double	dRacePacific
						, double	dRaceOther
						, double	dTimeini
						, long int	liPopSize
						, int* 		iArrayGroups
						, long int	liLimitLow
						, long int	liLimitUp
						, float		fSigmaLatentPrimaryPeriod
						, float		fSigmaLatentSecondaryPeriod
						, float		fSigmaHospitalizationPeriod
						, double	dWorkersSigmaAge
						, double	dWorkersMuAge
						, double	dStudentsSigmaAge
						, double	dStudentsMuAge
						, double	dElderlySigmaAge
						, double	dElderlyMuAge
						, double	dUnemployedSigmaAge
						, double	dUnemployedMuAge
						, FILE		*flLogfile
						, struct 	tPersonalInfo	**tpPersonalInfo )
{
	int 	iNGroup					= 0	/* group number */
			,iGroupType				= 0;	/* group type */

	long int i 						= 0	/* counter */
			,liLimitGroup			= 0;/* group limit */

	double	dProbability			= 0
			,dTimenow				= 0
			,dTimeStamp				= 0;

			/* Distribution */
	const 	gsl_rng_type 	* type 	= gsl_rng_ranlxs2;
			gsl_rng 		* rng 	= gsl_rng_alloc(type);

	gsl_rng_set (rng, getpid());

	/* Request memory for the latent primary period */
	assert ((*iArrayLatentPrimaryPeriod		= (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);
	/* Request memory for the latent secondary period */
	assert ((*iArrayLatentSecondaryPeriod	= (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);
	/* Request memory for the infective primary period */
	assert ((*iArrayInfectivePrimaryPeriod 	= (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);
	/* Request memory for the infective secondary period */
	assert ((*iArrayInfectiveSecondaryPeriod= (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);
	/* Request memory for the antiviral period */
	assert ((*iArrayAntiviralPeriod 		= (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);
	/* Request memory for the antiviral delay period */
	assert ((*iArrayAntiviralDelayPeriod 	= (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);
	/* Request memory for the asymptomatic period */
	assert ((*iArrayAsymptomaticPeriod		= (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);
	/* Request memory for the hospitalization period */
	assert ((*iArrayHospitalizationPeriod 	= (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);

   	/* LOGFILE data */
    dTimenow = MPI_Wtime();
    dTimeStamp = dTimenow - dTimeini;
    assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] GenPopulationInfo: initialization\n", dTimeStamp, iMyRank)) != EOF);
    fflush (flLogfile);

	/* Get the type and size of the first group */
	iGroupType	= iArrayGroups[iNGroup];
	liLimitGroup= iArrayGroups[iNGroup+1];

	/* Generate personal characteristics */
	for (i = liLimitLow; i <= liLimitUp; i ++) {
		
		/* Next group */	
		if (i >= (liLimitGroup+liLimitLow)) {
			iNGroup = iNGroup + 2;
			iGroupType = iArrayGroups[iNGroup];
			liLimitGroup += iArrayGroups[iNGroup+1];
		}

		/* Generate OCCUPATION */
		(*tpPersonalInfo)[i-liLimitLow].occupation = iGroupType;

		/* Generate RACE */
		dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

		if (dProbability < dRaceWhite)
			(*tpPersonalInfo)[i-liLimitLow].race = WHITE;
		else	if ((dProbability >= dRaceWhite) && 
			(dProbability < dRaceWhite+dRaceBlack))
				(*tpPersonalInfo)[i-liLimitLow].race = BLACK;
			else	if ((dProbability >= dRaceWhite+dRaceBlack)&& 
				   (dProbability < dRaceWhite+dRaceBlack+dRaceAmindian))
					(*tpPersonalInfo)[i-liLimitLow].race = AMINDIAN;
				else	if ((dProbability >= dRaceWhite+dRaceBlack+dRaceAmindian)&& 
				   	   (dProbability < dRaceWhite+dRaceBlack+dRaceAmindian+dRaceAsian))
						(*tpPersonalInfo)[i-liLimitLow].race = ASIAN;
					else	if ((dProbability >= dRaceWhite+dRaceBlack+dRaceAmindian+dRaceAsian)&& 
				   	   	   (dProbability < dRaceWhite+dRaceBlack+dRaceAmindian+dRaceAsian+dRacePacific))
							(*tpPersonalInfo)[i-liLimitLow].race = PACIFIC;
						else	if (dProbability >= dRaceWhite+dRaceBlack+dRaceAmindian+dRaceAsian+dRacePacific)
								(*tpPersonalInfo)[i-liLimitLow].race = OTHER;


		/* Generate PREVIOUS_DISEASES */
		(*tpPersonalInfo)[i-liLimitLow].prevdiseases = 0;

		/* Generate PROPHYLAXIS */
		(*tpPersonalInfo)[i-liLimitLow].prophylaxis = 0;

		/* Generate AGE and GENDER based on the OCCUPATION of the individual */
		switch ((*tpPersonalInfo)[i-liLimitLow].occupation) {

			case 1: /* WORKERS */
       		case 5: /* WORKERS ON SATURDAY */

				/* Generate AGE */
				(*tpPersonalInfo)[i-liLimitLow].age = floor(gsl_ran_poisson(rng, dWorkersSigmaAge) + dWorkersMuAge);

				/* Generate GENDER */
				dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

				if (dProbability > dWorkersMale)
					(*tpPersonalInfo)[i-liLimitLow].gender = FEMALE;
				else
					(*tpPersonalInfo)[i-liLimitLow].gender = MALE;

				break;

			case 2: /* STUDENTS */

				/* Generate AGE */
				(*tpPersonalInfo)[i-liLimitLow].age = floor(gsl_ran_poisson(rng, dStudentsSigmaAge) + dStudentsMuAge);

				/* Generate GENDER */
				dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

				if (dProbability > dStudentsMale)
					(*tpPersonalInfo)[i-liLimitLow].gender = FEMALE;
				else
					(*tpPersonalInfo)[i-liLimitLow].gender = MALE;

				break;

			case 3: /* ELDERLY */

				/* Generate AGE */
				(*tpPersonalInfo)[i-liLimitLow].age = dElderlyMuAge + floor(gsl_ran_exponential (rng, dElderlySigmaAge));

				/* Generate GENDER */
				dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);

				if (dProbability > dElderlyMale)
					(*tpPersonalInfo)[i-liLimitLow].gender = FEMALE;
				else
					(*tpPersonalInfo)[i-liLimitLow].gender = MALE;

				break;

			case 4: /* UNEMPLOYED */

				/* Generate AGE */
				(*tpPersonalInfo)[i-liLimitLow].age = floor(gsl_ran_poisson(rng, dUnemployedSigmaAge) + dUnemployedMuAge);

				/* Generate GENDER */
				dProbability = (double) gsl_rng_get (rng)/gsl_rng_max(rng);				
			
				if (dProbability > dUnemployedMale)
					(*tpPersonalInfo)[i-liLimitLow].gender = FEMALE;
				else
					(*tpPersonalInfo)[i-liLimitLow].gender = MALE;

				break;
		}
        
		/* Generate TINC PRIMARY */
		(*iArrayLatentPrimaryPeriod)[i-liLimitLow] 		= (gsl_ran_gaussian(rng, fSigmaLatentPrimaryPeriod) + fMuLatentPrimaryPeriod) * MINDAY;
		/* Generate TINC SECONDARY */
		(*iArrayLatentSecondaryPeriod)[i-liLimitLow] 	= (gsl_ran_gaussian(rng, fSigmaLatentSecondaryPeriod) + fMuLatentSecondaryPeriod) * MINDAY;
		/* Generate TINF PRIMARY */
		(*iArrayInfectivePrimaryPeriod)[i-liLimitLow]	= (gsl_ran_gaussian(rng, fSigmaInfectivePrimaryPeriod) + fMuInfectivePrimaryPeriod) * MINDAY;
        /* Generate TANTIVIRAL DELAY */
		/* 0.25*MINDAY = 360 min, the mean infectious period of pre-symptomatic infection */
		(*iArrayAntiviralDelayPeriod)[i-liLimitLow]		= (((double) gsl_rng_get (rng)/gsl_rng_max(rng)) * 
														  ((*iArrayInfectivePrimaryPeriod)[i-liLimitLow] - (0.25*MINDAY))) + (0.25*MINDAY);
		/* Generate TINF SECONDARY */
		(*iArrayInfectiveSecondaryPeriod)[i-liLimitLow]	= (gsl_ran_gaussian(rng, fSigmaInfectiveSecondaryPeriod) + fMuInfectiveSecondaryPeriod) 
														* MINDAY;
		/* Generate TASYMP */
		(*iArrayAsymptomaticPeriod)[i-liLimitLow]		= (gsl_ran_gaussian(rng, fSigmaAsymptomaticPeriod) + fMuAsymptomaticPeriod) * MINDAY;
		/* Generate TANTIVIRAL */
		(*iArrayAntiviralPeriod)[i-liLimitLow]			= (gsl_ran_gaussian(rng, fSigmaAntiviralPeriod) + fMuAntiviralPeriod) * MINDAY;
		/* Generate THOS */
		(*iArrayHospitalizationPeriod)[i-liLimitLow] 	= (gsl_ran_gaussian(rng, fSigmaHospitalizationPeriod) + fMuHospitalizationPeriod) * MINDAY;
	}
    
	/* Free */
	gsl_rng_free (rng);

	return (0);
}
