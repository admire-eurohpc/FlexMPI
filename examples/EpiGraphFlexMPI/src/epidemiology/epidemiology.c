/****************************************************************************************************************************************
 *																																		*
 *	Research Project - Universidad Carlos III de Madrid [2009-2019]																		*
 *                                                                                                                                      *
 *	- EpiGraph: A Scalable Simulation Tool for Epidemiological Studies -																*
 *																																		*
 *	File:       epidemiology.c																											*
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
#include "epidemiology.h"
#include "epidemiology_sync.h"

/****************************************************************************************************************************************
*
*	'LoadXmlConfigFile'
*
*	Utility:
*	 collect and store XML configuration file parameters.
*
****************************************************************************************************************************************/
int LoadXmlConfigFile	 ( char		**cPathogen
                         , char		**cPathV			
						 , char		**cPathStructure
						 , char		**cPathNFS
						 , char		*cConfigXML
						 , char		*cGraphXML
                         , int*		iCommunicationTime
                         , int*		iUpdatingTime
                         , int*		iStatisticsCollectionTime
                         , int*		iSpreadingTime
                         , int*		iHour
                         , int*		iDay
                         , int*		iWeekDay
                         , int*		iMonth
                         , int*		iVaccinationDay			
                         , int*		iVaccinationPeriodLength
                         , int* 	iVaccinationList
                         , int*		iMorningPeriod
                         , int*		iEveningPeriod
                         , int*		iNightPeriod
                         , int*     iSourceGraph
                         , int*     iLoadCheckpoint
						 , int*		iBedTime
                         , int* 	iSimDays
                         , long int* liCheckpoint
                         , long int* liPopSize
                         , double** dAvgTempMonth
                         , double** dAvgHumdMonth
						 , double*	dBedStudents
						 , double* 	dBedElderly
						 , double* 	dBedAdults
                         , double*	dInitialInfectives
                         , double*	dLatentPrimaryToSecondary
                         , double*	dLatentPrimaryToAsymptomatic
                         , double*	dInfectiveToRemoved
                         , double*	dInfectiveToHospitalized
                         , double*	dInfectiveToDead
                         , double*	dAsymptomaticToRemove
                         , double*	dHospitalizedToRemoved
                         , double*	dHospitalizedToDead
                         , double*	dRiskArray
                         , double*	dVaccinationPercentage 	
                         , double*	dVaccinationSusceptibleEffectiveness
                         , double*  dVaccinationLower64
                         , double*  dVaccinationGreater64
                         , double*	dClosingSchools
                         , double*	dSocialDistancing
                         , double*	dMuGraph
                         , double*	dAntiviralArray
                         , double*  dImmunityWorkers
                         , double*  dImmunityStudents
                         , double*  dImmunityElderly
                         , double*  dImmunityUnemployed )
{
	xmlDocPtr 	xXmlDoc;	/* Pointer to the XML config file */
	xmlNodePtr 	xRoot;		/* Pointer to the file root node */
	xmlChar 	*xValue;	/* Parameter values */

	/* Opening the XML file */
	if ((xXmlDoc = xmlParseFile (cConfigXML)) != NULL) {
		/* Root node reference */
		assert((xRoot = xmlDocGetRootElement (xXmlDoc)) != NULL);

		/* Parameter values of the XML configuration file */
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Pathogen")) != NULL);
		assert ((*cPathogen = (char*) malloc ((strlen((const char *) xValue)+1) * sizeof (char))) != NULL);
		strcpy (*cPathogen, (const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PathDest")) != NULL);
		assert ((*cPathV = (char*) malloc ((strlen((const char *) xValue)+500) * sizeof (char))) != NULL);
        
		strcpy (*cPathV,workdirpath);
        strcat (*cPathV,(const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PathStructure")) != NULL);
		assert ((*cPathStructure = (char*) malloc ((strlen((const char *) xValue)+500) * sizeof (char))) != NULL);
		strcpy (*cPathStructure, workdirpath);
        strcat (*cPathStructure, (const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PathNFS")) != NULL);
		assert ((*cPathNFS = (char*) malloc ((strlen((const char *) xValue)+500) * sizeof (char))) != NULL);
		strcpy (*cPathNFS, workdirpath);
        strcat (*cPathNFS, (const char *) xValue);
        
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "CommunicationTime")) != NULL);
		*iCommunicationTime = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "UpdatingTime")) != NULL);
		*iUpdatingTime = atoi ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "BedTime")) != NULL);
		*iBedTime = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "StatisticsCollectionTime")) != NULL);
		*iStatisticsCollectionTime = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SpreadingTime")) != NULL);
		*iSpreadingTime = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Hour")) != NULL);
		*iHour = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Day")) != NULL);
		*iDay = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "WeekDay")) != NULL);
		*iWeekDay = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Month")) != NULL);
		*iMonth = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "VaccinationDay")) != NULL);
		*iVaccinationDay = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "VaccinationPeriodDuration")) != NULL);
		*iVaccinationPeriodLength = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "VaccinationList")) != NULL);
		
		if (strcmp (((const char *) xValue), "max") == 0)
			*iVaccinationList = VACMAXDEGREE;

		else if (strcmp (((const char *) xValue), "maxinternal") == 0)
			*iVaccinationList = VACMAXINTERNAL;

		else if (strcmp (((const char *) xValue), "maxexternal") == 0)
			*iVaccinationList = VACMAXEXTERNAL;

		else if (strcmp (((const char *) xValue), "mean") == 0)
			*iVaccinationList = VACMEAN;

		else if (strcmp (((const char *) xValue), "random") == 0)
			*iVaccinationList = VACRANDOM;

		else
			*iVaccinationList = NOVACC;

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SimDays")) != NULL);
		*iSimDays = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ClosingSchools")) != NULL);
		*dClosingSchools = atof ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SocialDistancing")) != NULL);
		*dSocialDistancing = atof ((const char *) xValue);
        
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "LoadCheckpoint")) != NULL);
		*iLoadCheckpoint = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "Checkpoint")) != NULL);
		*liCheckpoint = atol ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "InitialInfectives")) != NULL);
		*dInitialInfectives = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "LatentPrimaryToSecondary")) != NULL);
		*dLatentPrimaryToSecondary = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "LatentPrimaryToAsymptomatic")) != NULL);
		*dLatentPrimaryToAsymptomatic = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "InfectiveToRemoved")) != NULL);
		*dInfectiveToRemoved = atof ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "InfectiveToHospitalized")) != NULL);
		*dInfectiveToHospitalized = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "InfectiveToDead")) != NULL);
		*dInfectiveToDead = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AsymptomaticToRemoved")) != NULL);
		*dAsymptomaticToRemove = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HospitalizedToRemoved")) != NULL);
		*dHospitalizedToRemoved = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HospitalizedToDead")) != NULL);
		*dHospitalizedToDead = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "StudentsAntiviral")) != NULL);
		dAntiviralArray[STUDENT-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "WorkersAntiviral")) != NULL);
		dAntiviralArray[WORKER-1] = atof ((const char *) xValue);
		dAntiviralArray[WORKER_SAT-1] = dAntiviralArray[WORKER];

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ElderlyAntiviral")) != NULL);
		dAntiviralArray[ELDERLY-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "UnemployedAntiviral")) != NULL);
		dAntiviralArray[UNEMPLOYED-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HighRiskInfP")) != NULL);
		dRiskArray[INFECTIVE_PRIMARY] = atof ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HighRiskInfS")) != NULL);
		dRiskArray[INFECTIVE_SECONDARY] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HighRiskAntiviral")) != NULL);
		dRiskArray[INFECTIVE_ANTIVIRAL] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HighRiskAsymp")) != NULL);
		dRiskArray[ASYMPTOMATIC] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HighRiskLatentS")) != NULL);
		dRiskArray[LATENT_SECONDARY] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HighRiskLatentS_T")) != NULL);
		dRiskArray[LATENT_SECONDARY_TREATED] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HighRiskInfP_T")) != NULL);
		dRiskArray[INFECTIVE_PRIMARY_TREATED] = atof ((const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HighRiskInfS_T")) != NULL);
		dRiskArray[INFECTIVE_SECONDARY_TREATED] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "HighRiskAsymp_T")) != NULL);
		dRiskArray[ASYMPTOMATIC_TREATED] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "VaccinationPercentage")) != NULL);
		*dVaccinationPercentage = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "VaccinationSusceptibleEffectiveness")) != NULL);
		*dVaccinationSusceptibleEffectiveness = atof ((const char *) xValue);
        
        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "VaccinationLower64")) != NULL);
		*dVaccinationLower64 = atof ((const char *) xValue);
        
       	assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "VaccinationGreater64")) != NULL);
		*dVaccinationGreater64 = atof ((const char *) xValue);
        
        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ImmunityWorkers")) != NULL);
		*dImmunityWorkers = atof ((const char *) xValue);
        
        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ImmunityStudents")) != NULL);
		*dImmunityStudents = atof ((const char *) xValue);
        
        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ImmunityElderly")) != NULL);
		*dImmunityElderly = atof ((const char *) xValue);
        
        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "ImmunityUnemployed")) != NULL);
		*dImmunityUnemployed = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MorningPeriod")) != NULL);
		*iMorningPeriod = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "EveningPeriod")) != NULL);
		*iEveningPeriod = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "NightPeriod")) != NULL);
		*iNightPeriod = atoi ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "BedStudents")) != NULL);
		*dBedStudents = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "BedElderly")) != NULL);
		*dBedElderly = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "BedAdults")) != NULL);
		*dBedAdults = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureJanuary")) != NULL);
		(*dAvgTempMonth)[JANUARY-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureFebruary")) != NULL);
		(*dAvgTempMonth)[FEBRUARY-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureMarch")) != NULL);
		(*dAvgTempMonth)[MARCH-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureApril")) != NULL);
		(*dAvgTempMonth)[APRIL-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureMay")) != NULL);
		(*dAvgTempMonth)[MAY-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureJune")) != NULL);
		(*dAvgTempMonth)[JUNE-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureJuly")) != NULL);
		(*dAvgTempMonth)[JULY-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureAugust")) != NULL);
		(*dAvgTempMonth)[AUGUST-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureSeptember")) != NULL);
		(*dAvgTempMonth)[SEPTEMBER-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureOctober")) != NULL);
		(*dAvgTempMonth)[OCTOBER-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureNovember")) != NULL);
		(*dAvgTempMonth)[NOVEMBER-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgTemperatureDecember")) != NULL);
		(*dAvgTempMonth)[DECEMBER-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityJanuary")) != NULL);
		(*dAvgHumdMonth)[JANUARY-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityFebruary")) != NULL);
		(*dAvgHumdMonth)[FEBRUARY-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityMarch")) != NULL);
		(*dAvgHumdMonth)[MARCH-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityApril")) != NULL);
		(*dAvgHumdMonth)[APRIL-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityMay")) != NULL);
		(*dAvgHumdMonth)[MAY-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityJune")) != NULL);
		(*dAvgHumdMonth)[JUNE-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityJuly")) != NULL);
		(*dAvgHumdMonth)[JULY-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityAugust")) != NULL);
		(*dAvgHumdMonth)[AUGUST-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumiditySeptember")) != NULL);
		(*dAvgHumdMonth)[SEPTEMBER-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityOctober")) != NULL);
		(*dAvgHumdMonth)[OCTOBER-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityNovember")) != NULL);
		(*dAvgHumdMonth)[NOVEMBER-1] = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "AvgHumidityDecember")) != NULL);
		(*dAvgHumdMonth)[DECEMBER-1] = atof ((const char *) xValue);

		xmlFree (xValue);

	} else {
		printf ("\n**ERROR**: opening the file '%s'\n", cConfigXML);
		exit (-1);
	}

	/* Opening the XML file */
	if ((xXmlDoc = xmlParseFile (cGraphXML)) != NULL) {
		/* Root node reference */
		assert((xRoot = xmlDocGetRootElement (xXmlDoc)) != NULL);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PopSize")) != NULL);
		*liPopSize = atol ((const char *) xValue);

        assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "MuGraph")) != NULL);
		*dMuGraph = atof ((const char *) xValue);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "SourceGraph")) != NULL);
		*iSourceGraph = atoi ((const char *) xValue);

		xmlFree (xValue);

	} else {
		printf ("\n**ERROR**: opening the file '%s'\n", cGraphXML);
		exit (-1);
	}

	return (0);
}

/****************************************************************************************************************************************
*
*	'InitializeInfectives'
*
*	Utility: 
*	 Set the initial infected individuals of the simulation.
*
****************************************************************************************************************************************/
int InitializeInfectives	( int		iMyRank
							, int**		iArrayCondition
							, int		iList
							, double	dInitialInfectives
							, char*		cPathV
							, long int	liLimitLow
							, long int	liLimitUp )
{	
	int	i = 0;
	
	double	dProbability = 0;

    FILE    *flFileListInfected = NULL;
    
    char    cFileNameListInfected [128];
    
	/* GSL random number generator */
	gsl_rng *r;
	r = gsl_rng_alloc (gsl_rng_ranlxs2);
    gsl_rng_set (r, getpid());    

	if (iList == RANDOM) {

		/* Random infections */
                
        /* Infecting random individuals */
        for (i = liLimitLow; i <= liLimitUp; i ++) {
        
            dProbability = (double) gsl_rng_get (r)/gsl_rng_max(r);

            /* Infected */
            if (dProbability <= dInitialInfectives)
                (*iArrayCondition)[i-liLimitLow] = LATENT_PRIMARY;
        }

	} else {
	
		/* Infected individuals from file */

		switch (iList) {
			case MAXDEGREE:
				sprintf (cFileNameListInfected, "%s%s", cPathV, "listMax");
				break;

			case MAXINTERNAL:
				sprintf (cFileNameListInfected, "%s%s", cPathV, "listMaxInternal");
				break;

			case MAXEXTERNAL:
				sprintf (cFileNameListInfected, "%s%s", cPathV, "listMaxExternal");
				break;

			case MEAN:
				sprintf (cFileNameListInfected, "%s%s", cPathV, "listMean");
				break;	
		}		

    	assert ((flFileListInfected = fopen (cFileNameListInfected, "r")) != NULL);
        
        while ((fscanf (flFileListInfected, "%i\n", &i)) != EOF)
            if ((i >= liLimitLow)&&(i <= liLimitUp))
				(*iArrayCondition)[i-liLimitLow] = LATENT_PRIMARY;
        
        fclose (flFileListInfected);       
    }

    gsl_rng_free (r);
    
	return (0);
}

/****************************************************************************************************************************************
 *
 *	'InitialImmunes'
 *
 *	Utility: 
 *	 Set the initial immune individuals of the simulation.
 *
 ****************************************************************************************************************************************/
int InitialImmunes  ( int       iMyRank
                    , struct 	tPersonalInfo	*tpPersonalInfo
                    , int**     iArrayCondition
                    , double    dImmunityWorkers
                    , double    dImmunityStudents
                    , double    dImmunityElderly
                    , double    dImmunityUnemployed
                    , long int	liLimitLow
                    , long int	liLimitUp )
{
    long int    i  = 0;
    
    double      dProbability = 0;
    
	/* GSL random number generator */
	gsl_rng *r;
	r = gsl_rng_alloc (gsl_rng_ranlxs2);
    gsl_rng_set (r, getpid());
    
    for ( i = liLimitLow; i <= liLimitUp; i ++) {
   
        dProbability = (double) gsl_rng_get (r)/gsl_rng_max(r);
        
        switch (tpPersonalInfo[i].occupation) {
                
            case 1: /* WORKERS */
            case 5: /* WORKERS ON SATURDAY */
                if (dProbability <= dImmunityWorkers)
                    (*iArrayCondition)[i-liLimitLow] = REMOVED;
                break;
                
            case 2: /* STUDENTS */
                if (dProbability <= dImmunityStudents)
                   	(*iArrayCondition)[i-liLimitLow] = REMOVED;
                break;
                
            case 3: /* ELDERLY */
                if (dProbability <= dImmunityElderly)
                  	(*iArrayCondition)[i-liLimitLow] = REMOVED;
               	break;
                
            case 4: /* UNEMPLOYED */
                if (dProbability <= dImmunityUnemployed)
                    (*iArrayCondition)[i-liLimitLow] = REMOVED;
                break;
                
           	default:
                break;
        }
    }
 
    return (0);
}

/****************************************************************************************************************************************
*
*	'GenLists'
*
*	Utility: 
*	 generate infected and vaccinated lists of individuals.
*
****************************************************************************************************************************************/
int GenLists		( long int	liPopSize
					, char*		cPathV
					, char*		cPathNFS
					, double	dPercentageIndividuals
					, int		iList )
{
	int			i						= 0
				,j						= 0
				,iIndividualIndex		= 0
				,*iArrayPopIndex		= 0
				,*iArrayDegree			= 0
				,*iArrayDegreeInternal 	= 0
				,iNumberIndividuals		= 0; /* number of infected individuals */

	double		dDegree        			= 0	/* <k> */
				,dDegreeStd				= 0	/* <mean> */
				,dDegreeSquare			= 0;/* <k^2> */

	FILE		*flFileListMax			= NULL
				,*flFileListMaxInternal	= NULL
				,*flFileListMaxExternal	= NULL
				,*flFileListMean		= NULL
				,*flFileListRandom		= NULL
				,*flFileDegreeInfo		= NULL;

	MPI_File	flFileDegree			 	/* file degree */
            	,flFileDegreeInternal; 		/* file internal degree */

	char		cFileNameListMax		[128]
				,cFileNameListMaxInternal[128]
				,cFileNameListMaxExternal[128]
				,cFileNameListMean		[128]
				,cFileNameListRandom	[128]
				,cFileNameDegree		[128]
           		,cFileNameDegreeInternal[128]
           		,cFileNameDegreeInfo	[128];
           		
    MPI_Offset	fileOffset				= 0;
    
    MPI_Status	mpi_status;

	/* GSL random number generator */
	const	gsl_rng_type 	* type 	= gsl_rng_ranlxs2;
			gsl_rng     	* rng	= gsl_rng_alloc(type);

	/* System seed */
    gsl_rng_set (rng, 797);

	/* file names */
	sprintf (cFileNameDegree, 			"%s%s", cPathNFS, 	"v_degree.dat");
	sprintf (cFileNameDegreeInternal, 	"%s%s", cPathNFS, 	"v_degree_internal.dat");
	sprintf (cFileNameDegreeInfo, 		"%s%s",	cPathV, 	"v_degreeInfo.dat");

	/* calculate number of infected or vaccinated individuals */
	iNumberIndividuals = (dPercentageIndividuals * liPopSize);	

	/* memory request */
	assert ((iArrayPopIndex	= (int*) malloc (liPopSize * sizeof (int))) != NULL);

	/* initialize population index */
	for (i = 0; i < liPopSize; i ++) iArrayPopIndex[i] = i;

	/* GENERATE LISTS */
	switch (iList) {
	
		case MAXDEGREE:
        case VACMAXDEGREE:

			/* lists file names */
			sprintf (cFileNameListMax, "%s%s", cPathV, "listMax");

			/* memory request */
			assert ((iArrayDegree = (int*) malloc (liPopSize * sizeof (int))) != NULL);

			MPI_File_open (MPI_COMM_SELF, cFileNameDegree, MPI_MODE_RDONLY, MPI_INFO_NULL, &flFileDegree);
    
			MPI_File_set_view(flFileDegree, fileOffset, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
		
		   	MPI_File_read (flFileDegree, iArrayDegree, liPopSize, MPI_INT, &mpi_status);
		
			MPI_File_close(&flFileDegree);		

			/* Generate list max */
			SortPop (iArrayDegree, iArrayPopIndex, 0, liPopSize-1);

			/* infected individuals selection */
			if ((flFileListMax = fopen (cFileNameListMax, "w")) != NULL)
				for (i = 0; i < iNumberIndividuals; i ++)
					assert ((fprintf (flFileListMax, "%i\n", iArrayPopIndex[i])) != EOF);

			/* fclose */
			fclose(flFileListMax);

			/* release memory */
			free(iArrayDegree);
			iArrayDegree = NULL;

			break;

		case MEAN:
        case VACMEAN:

			/* lists file names */
			sprintf (cFileNameListMean, "%s%s", cPathV, "listMean");

			/* memory request */
			assert ((iArrayDegree = (int*) malloc (liPopSize * sizeof (int))) != NULL);
			
			/* Get Degree & Square Degree */
			assert	((flFileDegreeInfo = fopen (cFileNameDegreeInfo, "r"))	!= NULL);
			assert ((fscanf (flFileDegreeInfo, "%lf\n%lf\n%lf\n", &dDegreeStd, &dDegree, &dDegreeSquare)) != EOF);
			fclose (flFileDegreeInfo);
			
			MPI_File_open (MPI_COMM_SELF, cFileNameDegree, MPI_MODE_RDONLY, MPI_INFO_NULL, &flFileDegree);

			MPI_File_set_view(flFileDegree, fileOffset, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);

		   	MPI_File_read_all (flFileDegree, iArrayDegree, liPopSize, MPI_INT, &mpi_status);
		   	
			MPI_File_close(&flFileDegree);
			
			/* Generate list max */
			SortPop (iArrayDegree, iArrayPopIndex, 0, liPopSize-1);
			
			/* Generate list mean */
			while (iArrayDegree[j] > dDegreeStd)
				j ++;
				
			if ((flFileListMean = fopen (cFileNameListMean, "w")) != NULL)
				for (i = j; i < (iNumberIndividuals + j); i ++)
					assert ((fprintf (flFileListMean, "%i\n", iArrayPopIndex[i])) != EOF);
					
			/* fclose */
			fclose(flFileListMean);
			
			/* release memory */
			free(iArrayDegree);
			iArrayDegree = NULL;

			break;

		case MAXINTERNAL:
        case VACMAXINTERNAL:

			/* lists file names */
			sprintf (cFileNameListMaxInternal, "%s%s", cPathV, "listMaxInternal");

			/* memory request */
			assert ((iArrayDegreeInternal = (int*) malloc (liPopSize * sizeof (int))) != NULL);
			
			MPI_File_open (MPI_COMM_SELF, cFileNameDegreeInternal, MPI_MODE_RDONLY, MPI_INFO_NULL, &flFileDegreeInternal);
    
			MPI_File_set_view(flFileDegreeInternal, fileOffset, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
		
		   	MPI_File_read (flFileDegreeInternal, iArrayDegreeInternal, liPopSize, MPI_INT, &mpi_status);
		
			MPI_File_close(&flFileDegreeInternal);

			/* Generate list max internal */
			SortPop (iArrayDegreeInternal, iArrayPopIndex, 0, liPopSize-1);

			/* infected individuals selection */
			if ((flFileListMaxInternal = fopen (cFileNameListMaxInternal, "w")) != NULL)
				for (i = 0; i < iNumberIndividuals; i ++)
					assert ((fprintf (flFileListMaxInternal, "%i\n", iArrayPopIndex[i])) != EOF);

			/* fclose */
			fclose(flFileListMaxInternal);

			/* release memory */
			free(iArrayDegreeInternal);
			iArrayDegreeInternal = NULL;

			break;

		case MAXEXTERNAL:
        case VACMAXEXTERNAL:

			/* lists file names */
			sprintf (cFileNameListMaxExternal, "%s%s", cPathV, "listMaxExternal");

			/* memory request */
			assert ((iArrayDegree = (int*) malloc (liPopSize * sizeof (int))) != NULL);
			assert ((iArrayDegreeInternal = (int*) malloc (liPopSize * sizeof (int))) != NULL);

			MPI_File_open (MPI_COMM_SELF, cFileNameDegree, MPI_MODE_RDONLY, MPI_INFO_NULL, &flFileDegree);
    
			MPI_File_set_view(flFileDegree, fileOffset, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
		
		   	MPI_File_read (flFileDegree, iArrayDegree, liPopSize, MPI_INT, &mpi_status);
		
			MPI_File_close(&flFileDegree);

			MPI_File_open (MPI_COMM_SELF, cFileNameDegreeInternal, MPI_MODE_RDONLY, MPI_INFO_NULL, &flFileDegreeInternal);
    
			MPI_File_set_view(flFileDegreeInternal, fileOffset, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
		
		   	MPI_File_read (flFileDegreeInternal, iArrayDegreeInternal, liPopSize, MPI_INT, &mpi_status);
		
			MPI_File_close(&flFileDegreeInternal);
			
			for (i = 0; i < liPopSize; i ++)
				iArrayDegree[i] -= iArrayDegreeInternal[i];

			/* Generate list max external */
			SortPop (iArrayDegreeInternal, iArrayPopIndex, 0, liPopSize-1);

			/* infected individuals selection */
			if ((flFileListMaxExternal = fopen (cFileNameListMaxExternal, "w")) != NULL)
				for (i = 0; i < iNumberIndividuals; i ++)
					assert ((fprintf (flFileListMaxExternal, "%i\n", iArrayPopIndex[i])) != EOF);

			/* fclose */
			fclose(flFileListMaxExternal);

			/* release memory */
			free(iArrayDegreeInternal);
			iArrayDegreeInternal = NULL;
			free(iArrayDegree);
			iArrayDegree = NULL;
			
			break;

		case VACRANDOM:

			/* lists file names */
			sprintf (cFileNameListRandom, "%s%s", cPathV, "listRandom");

			assert ((flFileListRandom = fopen (cFileNameListRandom, "w")) != NULL);

			for (i = 0; i < iNumberIndividuals; i ++) {

				iIndividualIndex = ((double) gsl_rng_get (rng)/gsl_rng_max(rng)) * liPopSize;

				if (iIndividualIndex == liPopSize) iIndividualIndex = (liPopSize - 1);

				assert ((fprintf (flFileListRandom, "%i\n", iIndividualIndex)) != EOF);
			}

			/* fclose */
			fclose(flFileListRandom);

		break;
	}

	/* release memory */
	gsl_rng_free (rng);
	free(iArrayPopIndex);
	iArrayPopIndex = NULL;

	return (0);
}

/****************************************************************************************************************************************
 *
 *	'WInfectivity'
 *
 *	Utilidad: 
 *	 weight the probability of infection depending on the personal characteristics of the individual.
 *
 ****************************************************************************************************************************************/
float WInfectivity	( int		iAge
					, int		iOccupation
					, int		iGender
					, int		iPrevDiseases
					, int		iRace
					, int		iProphylaxis
					, int 		iMonth
					, double* 	dAvgTempMonth
					, double*	dAvgHumdMonth )
{
    double  dWeighting  = 1;
    
	/* Age */
    if (iAge < 18)
        dWeighting = dWeighting * 1.25;
    else{
        if (iAge > 64)
        dWeighting = dWeighting * 1.125;
    }
	/* Occupation */
	switch (iOccupation) {
		default:
			break;
	}

	/* Gender */
	switch (iGender) {
		default:
			break;
	}

	/* Previous Diseases */
	switch (iPrevDiseases) {
		default:
			break;
	}

	/* Race */
	switch (iRace) {
		default:
			break;
	}

	/* Prophylaxis */
	switch (iProphylaxis) {
		default:
			break;
	}

	/* Weather */
	switch (iMonth) {

		//FIXME: implementar modelo climatico y de humedad

		default:
			break;
	}

	return (dWeighting);
}

/****************************************************************************************************************************************
 *
 *	'Check'
 *
 *	Utilidad: 
 *	 check the probability of infection depending on the type of connection.
 *
 ****************************************************************************************************************************************/
int Check   ( int   iHour
			, int	iWeekDay
            , int   iSourceGraph
            , int   iConnection
            , int   iInBedA
            , int   iOccupationA
            , int   iOccupationB
			, int	iAgeA
			, int 	iAgeB
            , int 	iActiveClosingSchools
            , int 	iActiveSocialDistancing )
{
    int     iChecked = FALSE;
    
    if (iSourceGraph == SOCIALNETWORK) {
        
        //FIXME: calendario laboral y reflejar que individuos se puedan relacionar con familiares de igual ocupacion en caso de mitigation strategies.
        
    	//FIXME: ojo, si cambio patrones de tiempo cambia la division del degree porque ya no seran 8 horas dia, 2 tarde y 14 noche
        
       	switch (iOccupationA) {
       	
       		case STUDENT:
       		
       			iChecked = CheckStudent (iHour, iWeekDay, iConnection, iInBedA, iOccupationB, iAgeA, iAgeB, iActiveClosingSchools, iActiveSocialDistancing);
       			
       			break;
       			
       		case WORKER:
       		
       			iChecked = CheckWorker (iHour, iWeekDay, iConnection, iInBedA, iOccupationB, iAgeA, iAgeB, iActiveClosingSchools, iActiveSocialDistancing);
       			
       			break;
       			
       		case UNEMPLOYED:
       		
       			iChecked = CheckUnemployed (iHour, iWeekDay, iConnection, iInBedA, iOccupationB, iAgeA, iAgeB, iActiveClosingSchools, iActiveSocialDistancing);
       			
       			break;
       			
       		case ELDERLY:
       		
       			iChecked = CheckElderly (iHour, iWeekDay, iConnection, iInBedA, iOccupationB, iAgeA, iAgeB, iActiveClosingSchools, iActiveSocialDistancing);
       			
       			break;
       			
       		case WORKER_SAT:
       		
       			iChecked = CheckWorkerWeekend (iHour, iWeekDay, iConnection, iInBedA, iOccupationB, iAgeA, iAgeB, iActiveClosingSchools, iActiveSocialDistancing);
       			
       			break;
       	}
        
	} else
		iChecked = TRUE;
    
    return (iChecked);
}

/****************************************************************************************************************************************
 *
 *	'CheckStudent'
 *
 *	Utilidad: 
 *	 check the probability of infection for students.
 *
 ****************************************************************************************************************************************/
int CheckStudent	( int   iHour
					, int	iWeekDay
				    , int   iConnection
				    , int   iInBedA
				    , int   iOccupationB
				    , int	iAgeA
				    , int 	iAgeB
				    , int 	iActiveClosingSchools
				    , int 	iActiveSocialDistancing )
{
	int iChecked = FALSE;

	switch (iWeekDay) {
	
		case LABORAL:
		
			if ((iConnection == WORK) && (iHour >= 9) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveClosingSchools == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveClosingSchools == TRUE) && (iConnection == FAMILIAR) && (iHour >= 9) && (iHour <= 14) && (iOccupationB == STUDENT))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 9) && (iHour <= 14) && ((iOccupationB != WORKER) && (iOccupationB != WORKER_SAT)))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour == 15) && ((iOccupationB != WORKER) && (iOccupationB != WORKER_SAT)))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 16) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB == STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 16) && (iHour <= 17) && ((iOccupationB != WORKER) && (iOccupationB != WORKER_SAT)))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 18) && (iHour <= 19))
			
				iChecked = TRUE;
		
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 8))
			
				iChecked = TRUE;
		
			break;
			
		case WEEKEND:
		
			if ((iConnection == LEISURE) && (iHour >= 12) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 12) && (iHour <= 14) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 15) && (iHour <= 16) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour == 17) && (iInBedA == FALSE) && (iOccupationB == STUDENT) && (iOccupationB != WORKER_SAT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour == 17) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 18) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 18) && (iHour <= 19))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 20) && (iHour <= 23) && (iInBedA == FALSE) && (iAgeA >= 15) && (iAgeB <= 35) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour == 0) && (iInBedA == FALSE) && (iAgeA >= 15) && (iAgeB <= 35) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 11))
			
				iChecked = TRUE;
		
			break;
			
		case HOLIDAY:
		
			if ((iConnection == LEISURE) && (iHour >= 12) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 12) && (iHour <= 14))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 15) && (iHour <= 16))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 17) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB == STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 17) && (iHour <= 19))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 11))
			
				iChecked = TRUE;
		
			break;	
	}

	return iChecked;
}

/****************************************************************************************************************************************
 *
 *	'CheckWorker'
 *
 *	Utilidad: 
 *	 check the probability of infection for workers.
 *
 ****************************************************************************************************************************************/
int CheckWorker	    ( int   iHour
					, int	iWeekDay
				    , int   iConnection
				    , int   iInBedA
				    , int   iOccupationB
				    , int	iAgeA
				    , int 	iAgeB
				    , int 	iActiveClosingSchools
				    , int 	iActiveSocialDistancing )
{
	int iChecked = FALSE;

	switch (iWeekDay) {
	
		case LABORAL:
		
			if ((iConnection == WORK) && (iHour >= 9) && (iHour <= 17) && (iInBedA == FALSE))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 18) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
		
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 18) && (iHour <= 19))
			
				iChecked = TRUE;
		
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 8))
			
				iChecked = TRUE;
		
			break;
			
		case WEEKEND:
		
			if ((iConnection == LEISURE) && (iHour >= 12) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 12) && (iHour <= 14) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 15) && (iHour <= 16) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour == 17) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iOccupationB != WORKER_SAT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour == 17) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 18) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 18) && (iHour <= 19))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 20) && (iHour <= 23) && (iInBedA == FALSE) && (iAgeA >= 15) && (iAgeB <= 35) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour == 0) && (iInBedA == FALSE) && (iAgeA >= 15) && (iAgeB <= 35) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 11))
			
				iChecked = TRUE;
		
			break;
			
		case HOLIDAY:
		
			if ((iConnection == LEISURE) && (iHour >= 12) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 12) && (iHour <= 14))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 15) && (iHour <= 16))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 17) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 17) && (iHour <= 19))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 11))
			
				iChecked = TRUE;
		
			break;	
	}

	return iChecked;
}

/****************************************************************************************************************************************
 *
 *	'CheckUnemployed'
 *
 *	Utilidad: 
 *	 check the probability of infection for unemployed people.
 *
 ****************************************************************************************************************************************/
int CheckUnemployed	    ( int   iHour
						, int	iWeekDay
						, int   iConnection
						, int   iInBedA
						, int   iOccupationB
						, int	iAgeA
						, int 	iAgeB
						, int 	iActiveClosingSchools
						, int 	iActiveSocialDistancing )
{
	int iChecked = FALSE;

	switch (iWeekDay) {
	
		case LABORAL:
		
			if ((iConnection == WORK) && (iHour >= 9) && (iHour <= 11) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 9) && (iHour <= 11) && ((iOccupationB != WORKER) && (iOccupationB != WORKER_SAT)))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 12) && (iHour <= 14) && (iInBedA == FALSE) && (iOccupationB == UNEMPLOYED) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
		
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 12) && (iHour <= 14) && ((iOccupationB != WORKER) && (iOccupationB != WORKER_SAT)))
			
				iChecked = TRUE;
		
			else if ((iConnection == FAMILIAR) && (iHour == 15) && ((iOccupationB != WORKER) && (iOccupationB != WORKER_SAT)))
			
				iChecked = TRUE;
		
			else if ((iConnection == WORK) && (iHour >= 16) && (iHour <= 17) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 16) && (iHour <= 17) && ((iOccupationB != WORKER) && (iOccupationB != WORKER_SAT)))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 18) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 18) && (iHour <= 19))
			
				iChecked = TRUE;
		
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 8))
			
				iChecked = TRUE;
		
			break;
			
		case WEEKEND:		
		
			if ((iConnection == LEISURE) && (iHour >= 12) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 12) && (iHour <= 14) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 15) && (iHour <= 16) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour == 17) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iOccupationB != WORKER_SAT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;

			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour == 17) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
			
			else if ((iConnection == LEISURE) && (iHour >= 18) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 18) && (iHour <= 19))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 20) && (iHour <= 23) && (iInBedA == FALSE) && (iAgeA >= 15) && (iAgeB <= 35) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour == 0) && (iInBedA == FALSE) && (iAgeA >= 15) && (iAgeB <= 35) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 11))
			
				iChecked = TRUE;
		
		
			break;
			
		case HOLIDAY:
		
			if ((iConnection == LEISURE) && (iHour >= 12) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 12) && (iHour <= 14))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 15) && (iHour <= 16))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 17) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 17) && (iHour <= 19))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 11))
			
				iChecked = TRUE;
		
			break;	
	}

	return iChecked;
}

/****************************************************************************************************************************************
 *
 *	'CheckElderly'
 *
 *	Utilidad: 
 *	 check the probability of infection for elderly people.
 *
 ****************************************************************************************************************************************/
int CheckElderly	    ( int   iHour
						, int	iWeekDay
						, int   iConnection
						, int   iInBedA
						, int   iOccupationB
						, int	iAgeA
						, int 	iAgeB
						, int 	iActiveClosingSchools
						, int 	iActiveSocialDistancing )
{
	int iChecked = FALSE;
	
	switch (iWeekDay) {
	
		case LABORAL:
		
			if ((iConnection == WORK) && (iHour >= 10) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
		
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 10) && (iHour <= 14) && ((iOccupationB != WORKER) && (iOccupationB != WORKER_SAT)))
			
				iChecked = TRUE;
		
			else if ((iConnection == FAMILIAR) && (iHour == 15) && ((iOccupationB != WORKER) && (iOccupationB != WORKER_SAT)))
			
				iChecked = TRUE;
		
			else if ((iConnection == WORK) && (iHour >= 16) && (iHour <= 17) && (iInBedA == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 16) && (iHour <= 17) && ((iOccupationB != WORKER) && (iOccupationB != WORKER_SAT)))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 18) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 18) && (iHour <= 19))
			
				iChecked = TRUE;
		
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 9))
			
				iChecked = TRUE;
		
			break;
			
		case WEEKEND:
		
			if ((iConnection == LEISURE) && (iHour >= 12) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 12) && (iHour <= 14) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 15) && (iHour <= 16) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour == 17) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iOccupationB != WORKER_SAT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour == 17) && (iOccupationB != WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 18) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 18) && (iHour <= 19))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 11))
			
				iChecked = TRUE;
	
			break;
		
		case HOLIDAY:
		
			if ((iConnection == LEISURE) && (iHour >= 12) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 12) && (iHour <= 14))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 15) && (iHour <= 16))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 17) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 17) && (iHour <= 19))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 11))
			
				iChecked = TRUE;
		
			break;
	}

	return iChecked;
}

/****************************************************************************************************************************************
 *
 *	'CheckWorkerWeekend'
 *
 *	Utilidad: 
 *	 check the probability of infection for workers on saturdays.
 *
 ****************************************************************************************************************************************/
int CheckWorkerWeekend	( int   iHour
						, int	iWeekDay
						, int   iConnection
						, int   iInBedA
						, int   iOccupationB
						, int	iAgeA
						, int 	iAgeB
						, int 	iActiveClosingSchools
						, int 	iActiveSocialDistancing )
{
	int iChecked = FALSE;

	switch (iWeekDay) {
	
		case LABORAL:
		
			if ((iConnection == WORK) && (iHour >= 9) && (iHour <= 17) && (iInBedA == FALSE))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 18) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
		
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 18) && (iHour <= 19))
			
				iChecked = TRUE;
		
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 8))
			
				iChecked = TRUE;
		
			break;
			
		case WEEKEND:
		
			if ((iConnection == WORK) && (iHour >= 9) && (iHour <= 17) && (iInBedA == FALSE) && (iOccupationB == WORKER_SAT))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 18) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 18) && (iHour <= 19))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 20) && (iHour <= 23) && (iInBedA == FALSE) && (iAgeA >= 15) && (iAgeB <= 35) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour == 0) && (iInBedA == FALSE) && (iAgeA >= 15) && (iAgeB <= 35) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 11))
			
				iChecked = TRUE;
		
			break;
			
		case HOLIDAY:
		
			if ((iConnection == LEISURE) && (iHour >= 12) && (iHour <= 14) && (iInBedA == FALSE) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 12) && (iHour <= 14))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 15) && (iHour <= 16))
			
				iChecked = TRUE;
				
			else if ((iConnection == LEISURE) && (iHour >= 17) && (iHour <= 19) && (iInBedA == FALSE) && (iOccupationB != STUDENT) && (iActiveSocialDistancing == FALSE))
			
				iChecked = TRUE;
				
			else if ((iActiveSocialDistancing == TRUE) && (iConnection == FAMILIAR) && (iHour >= 17) && (iHour <= 19))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 20) && (iHour <= 23))
			
				iChecked = TRUE;
				
			else if ((iConnection == FAMILIAR) && (iHour >= 0) && (iHour <= 11))
			
				iChecked = TRUE;
		
			break;	
	}

	return iChecked;
}

/****************************************************************************************************************************************
 *
 *	'SetCalendar'
 *
 *	Utility: 
 *	 set the calendar of the year.
 *
 ****************************************************************************************************************************************/
int SetCalendar	( int**		iCalendar
				, int 		iWeekDay )
{
	int m = 0
		,d = 0;

	for (m = 0; m < 12; m ++)

		for (d = 0; d < 30; d ++) {

			switch (iWeekDay) {

				case MONDAY:
				case TUESDAY:
            	case WEDNESDAY:
            	case THURSDAY:
            	case FRIDAY:

            		(*iCalendar)[(m*30)+d] = LABORAL;

            		break; 

            	case SATURDAY:

            		(*iCalendar)[(m*30)+d] = WEEKEND;

            		break;

            	case SUNDAY:

            		(*iCalendar)[(m*30)+d] = HOLIDAY;

            		break;
			}

			iWeekDay ++;

			if (iWeekDay > SUNDAY) iWeekDay = MONDAY;
		}

	//FIXME: Vacaciones de navidad, verano y festivos

	return 0;
}

/****************************************************************************************************************************************
*
*	'EpiSim'
*
*	Utility: 
*	 main function of the simulation of the spreading.
*
****************************************************************************************************************************************/
int EpiSim	( int 		iMyRank
            , int		iNumberProcesses
            , int		iFileSystem
            , int		iList
            , char		*cConfigXML
            , char		*cGraphXML
            , FILE		*flLogfile
            , double 	dTimeini
            , char*		cPathXML
            , char*		argv[] )
{
	int		n								= 0
			, iLength 						= 0
			, it 							= 0
			, data						[6] = {0, 0, 0, 0, 0, 0}
			, desp							= 0
			, count 						= 0
			, type 							= 0
			, iGlobalNNZ					= 0
			, iCommunicationTime			= 0
			, iGraphProcs					= 0
			, iUpdatingTime					= 0
			, iStatisticsCollectionTime		= 0
			, iNoValue						= 0
			, mpi_tag						= 991
			, iSpreadingTime				= 0
			, iHour							= 0
			, iDay							= 1
			, iWeekDay						= MONDAY
			, iMonth						= JANUARY
			, iMin							= 0
			, iSimDays						= 0
			, iBedTime						= 0
			, iVaccinationDay				= 0
			, iVaccinationPeriodLength		= 0
			, iVaccinationList				= VACRANDOM
			, iMorningPeriod				= 0
            , iSourceGraph          		= 0
			, iEveningPeriod				= 0
			, iNightPeriod					= 0
            , iLoadCheckpoint               = 0
            , iSetTimeStamp                 = 1
			, *iArrayInfectedPeople			= NULL
            , *iCalendar					= NULL
            , *iBedStatus					= NULL
			, *iArrayCondition				= NULL
            , *iArrayConditionTime          = NULL
			, *iArrayLatentPrimaryPeriod	= NULL
			, *iArrayLatentSecondaryPeriod	= NULL
			, *iArrayInfectivePrimaryPeriod	= NULL
			, *iArrayInfectiveSecondaryPeriod=NULL
			, *iArrayAntiviralPeriod		= NULL
            , *iArrayAntiviralDelayPeriod 	= NULL
			, *iArrayAsymptomaticPeriod		= NULL
			, *iArrayHospitalizationPeriod	= NULL;

	long int  liPopSize 					= 0
			, liSimTime						= 0
			, liCheckpoint					= 0
			, liLimitLow					= 0
			, liLimitUp						= 0;

	double	dTimeStamp						= 0
			, dBedStudents					= 0
			, dBedElderly 					= 0
			, dBedAdults 					= 0
			, dClosingSchools				= 0
			, dSocialDistancing				= 0
			, dSpreadTimeStamp				= 0
			, dInitialInfectives			= 0
			, dLatentPrimaryToSecondary		= 0
			, dLatentPrimaryToAsymptomatic	= 0
			, dInfectiveToRemoved			= 0
			, dInfectiveToHospitalized		= 0
			, dInfectiveToDead				= 0
			, dAsymptomaticToRemove			= 0
			, dHospitalizedToRemoved		= 0
			, dHospitalizedToDead			= 0
			, dVaccinationPercentage		= 0
			, dVaccinationSusceptibleEffectiveness= 0
            , dVaccinationLower64   		= 0
            , dVaccinationGreater64 		= 0
            , dImmunityWorkers  			= 0
            , dImmunityStudents 			= 0
            , dImmunityElderly  			= 0
            , dImmunityUnemployed   		= 0
			, dMuGraph						= 0
			, dTimenow						= 0
			, dTimespread_ini				= 0
			, dTimespread_end				= 0
            , *dAvgTempMonth				= NULL
            , *dAvgHumdMonth				= NULL
			,*dAntiviralArray				= NULL
			,*dRiskArray					= NULL;

	cs_int	*csMatrix						= NULL;

	MPI_Status	status;

	char	*cPathogen						= NULL
			, *cPathV						= NULL
			, *cPathStructure				= NULL
			, *cPathNFS						= NULL
			, mpi_name 						[128];

	struct tPersonalInfo *tpPersonalInfo = NULL;

	EMPI_Get_type (&type);

	/* Request memory */
	assert ((dRiskArray 		= (double*) calloc (SGSIZE, sizeof (double))) != NULL);
	
	/* Request memory */
	assert ((dAntiviralArray 	= (double*) calloc (OCSIZE, sizeof (double))) != NULL);

	/* Request memory */
	assert ((dAvgTempMonth		= (double*) 	calloc (12, sizeof (double))) != NULL);

	/* Request memory */
	assert ((dAvgHumdMonth		= (double*) 	calloc (12, sizeof (double))) != NULL);

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] FUNCTION: LoadXmlConfigFile\n", dTimeStamp, iMyRank)) != EOF);
    fflush (flLogfile);

	/* LoadXmlConfigFile */
	LoadXmlConfigFile ( &cPathogen, &cPathV, &cPathStructure, &cPathNFS, cConfigXML, cGraphXML, &iCommunicationTime, &iUpdatingTime
	, &iStatisticsCollectionTime, &iSpreadingTime, &iHour, &iDay, &iWeekDay, &iMonth, &iVaccinationDay, &iVaccinationPeriodLength
	, &iVaccinationList , &iMorningPeriod, &iEveningPeriod, &iNightPeriod, &iSourceGraph, &iLoadCheckpoint, &iBedTime
	, &iSimDays, &liCheckpoint, &liPopSize, &dAvgTempMonth, &dAvgHumdMonth, &dBedStudents, &dBedElderly, &dBedAdults, &dInitialInfectives
	, &dLatentPrimaryToSecondary, &dLatentPrimaryToAsymptomatic, &dInfectiveToRemoved, &dInfectiveToHospitalized, &dInfectiveToDead
	, &dAsymptomaticToRemove, &dHospitalizedToRemoved, &dHospitalizedToDead, dRiskArray, &dVaccinationPercentage
	, &dVaccinationSusceptibleEffectiveness, &dVaccinationLower64, &dVaccinationGreater64, &dClosingSchools, &dSocialDistancing
	, &dMuGraph, dAntiviralArray, &dImmunityWorkers, &dImmunityStudents, &dImmunityElderly, &dImmunityUnemployed);
	
	/* Calculate limits */
	EMPI_Get_wsize (iMyRank, iNumberProcesses, (int)(liPopSize), &desp, &count, NULL, NULL);
	liLimitLow = desp;
	liLimitUp = (desp+count-1);

	/* Request memory */
	assert ((iArrayCondition        = (int*) calloc ((liLimitUp-liLimitLow+1), sizeof (int))) != NULL);

	/* Request memory */
	assert ((iArrayConditionTime    = (int*) calloc ((liLimitUp-liLimitLow+1), sizeof (int))) != NULL);

	/* Request memory for the array of infected individuals */
	assert ((iArrayInfectedPeople	= (int*) calloc (liPopSize, sizeof (int))) != NULL);

	/* Request memory for the bed status */
	assert ((iBedStatus				= (int*) malloc ((liLimitUp-liLimitLow+1) * sizeof (int))) != NULL);

	if (type == EMPI_NATIVE) {

		MPI_Get_processor_name (mpi_name, &iLength);
		printf ("[%i] Process spawned in processor %s at %i\n", iMyRank, mpi_name, iSetTimeStamp);

		/* LOGFILE */
		dTimenow = MPI_Wtime();
		dTimeStamp = dTimenow - dTimeini;
		assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] FUNCTION: LoadMatrix\n", dTimeStamp, iMyRank)) != EOF);
	   	fflush (flLogfile);
		
		/* Serial Data Loading */
		if ( (iMyRank < (iNumberProcesses-1)) && ( iFileSystem == NFS ))
			MPI_Recv (&iNoValue, 1, MPI_INT, iMyRank+1, mpi_tag, EMPI_COMM_WORLD, &status);
		    
		/* LoadMatrix */
		LoadMatrix (iMyRank, &iGraphProcs, liLimitLow, liLimitUp, &csMatrix, cPathV, cPathNFS, &liPopSize);
	
		/* Serial Data Loading */
		if ( (iMyRank > MASTER) && ( iFileSystem == NFS ))
			MPI_Send (&iMyRank, 1, MPI_INT, iMyRank-1, mpi_tag, EMPI_COMM_WORLD);

		MPI_Allreduce (&csMatrix->nzmax, &iGlobalNNZ, 1, MPI_INT, MPI_SUM, EMPI_COMM_WORLD);
		
		//Register sparse matrix
		EMPI_Register_sparse ("matrix", csMatrix->p, csMatrix->i, csMatrix->x, MPI_INT, liPopSize, iGlobalNNZ);

		//Register arrays
		EMPI_Register_vector ("condition", iArrayCondition, MPI_INT, (liLimitUp-liLimitLow+1), EMPI_DISJOINT);
		EMPI_Register_vector ("conditionTime", iArrayConditionTime, MPI_INT, (liLimitUp-liLimitLow+1), EMPI_DISJOINT);
		EMPI_Register_vector ("bedStatus", iBedStatus, MPI_INT, (liLimitUp-liLimitLow+1), EMPI_DISJOINT);
		EMPI_Register_vector ("infected", iArrayInfectedPeople, MPI_INT, liPopSize, EMPI_SHARED);
			
		if ((iList != RANDOM)||(iVaccinationList != NOVACC)) {
            
			/* LOGFILE */
			dTimenow = MPI_Wtime();
			dTimeStamp = dTimenow - dTimeini;
			assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] FUNCTION: GenLists\n", dTimeStamp, iMyRank)) != EOF);
			fflush (flLogfile);

			if (iMyRank == MASTER) {

				if (iList != RANDOM)
				/* Generate list of infected people */
				GenLists (liPopSize, cPathV, cPathNFS, dInitialInfectives, iList);

				if (((iList != (iVaccinationList-5))&&(iList != RANDOM))||(iVaccinationList == VACRANDOM))
					/* Generate list of vaccinated people */
					GenLists (liPopSize, cPathV, cPathNFS, dVaccinationPercentage, iVaccinationList);
			}

			/* MPI_Barrier */
			MPI_Barrier (EMPI_COMM_WORLD);
		}

		/* Initialize individual's bed status */
		for (n = liLimitLow; n <= liLimitUp; n ++) iBedStatus[n-liLimitLow] = 0;

		/* LOGFILE */
		dTimenow = MPI_Wtime();
		dTimeStamp = dTimenow - dTimeini;
		assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] FUNCTION: InitialImmunes\n", dTimeStamp, iMyRank)) != EOF);
	 	fflush (flLogfile);
		
		//FIXME: problema: tiene que estar cargado antes tPersonalInfo
		/* Initial Immunes */
		//InitialImmunes (iMyRank, tpPersonalInfo, &iArrayCondition, dImmunityWorkers, dImmunityStudents, dImmunityElderly, dImmunityUnemployed
		//, liLimitLow, liLimitUp);

		/* LOGFILE */
		dTimenow = MPI_Wtime();
		dTimeStamp = dTimenow - dTimeini;
		assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] FUNCTION: InitializeInfectives\n", dTimeStamp, iMyRank)) != EOF);
		fflush (flLogfile);

		/* Initial Infectives */
		InitializeInfectives (iMyRank, &iArrayCondition, iList, dInitialInfectives, cPathV, liLimitLow, liLimitUp);
					
	} else {
		
		/* LOGFILE */
		dTimenow = MPI_Wtime();
		dTimeStamp = dTimenow - dTimeini;
		assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] FUNCTION: LoadMatrix (new process)\n", dTimeStamp, iMyRank)) != EOF);
	   	fflush (flLogfile);
		
		csMatrix = (cs_int*) malloc (sizeof (cs_int));
		
		//register sparse matrix
		EMPI_Register_sparse ("matrix", csMatrix->p, csMatrix->i, csMatrix->x, MPI_INT, liPopSize, 0);

		//Register arrays
		EMPI_Register_vector ("condition", iArrayCondition, MPI_INT, (liLimitUp-liLimitLow+1), EMPI_DISJOINT);
		EMPI_Register_vector ("conditionTime", iArrayConditionTime, MPI_INT, (liLimitUp-liLimitLow+1), EMPI_DISJOINT);
		EMPI_Register_vector ("bedStatus", iBedStatus, MPI_INT, (liLimitUp-liLimitLow+1), EMPI_DISJOINT);
		EMPI_Register_vector ("infected", iArrayInfectedPeople, MPI_INT, liPopSize, EMPI_SHARED);
		
		//get shared array
		EMPI_Get_shared (&it);

		iSetTimeStamp = it;

		MPI_Get_processor_name (mpi_name, &iLength);
		printf ("[%i] Process spawned in processor %s at %i\n", iMyRank, mpi_name, iSetTimeStamp);
		
		PMPI_Bcast (data, 6, MPI_INT, 0, EMPI_COMM_WORLD);
		
		iMin = data[0];
		iHour = data[1];
		iDay = data[2];
		iMonth = data[3];
		iWeekDay = data[4];
		iGraphProcs = data[5];
		
		//Get addr sparse
		EMPI_Get_addr_sparse ("matrix", (void*)&csMatrix->p, (void*)&csMatrix->i, (void*)&csMatrix->x);

		//FIXME: establecer nzmax, nz, etc. para matrix csMatrix
	}

	/* Set SimTime */
	liSimTime = iSimDays * MINDAY;

	/* Request memory */
	assert ((tpPersonalInfo = (struct tPersonalInfo *) malloc (liPopSize * sizeof (struct tPersonalInfo))) != NULL);

	/* Request memory */
	assert ((iCalendar = (int*) calloc (360, sizeof (int))) != NULL);

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] FUNCTION: SetCalendar\n", dTimeStamp, iMyRank)) != EOF);
	fflush (flLogfile);
	
	/* Set Calendar */	    
	SetCalendar (&iCalendar, iWeekDay);

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] FUNCTION: LoadPeopleArrays\n", dTimeStamp, iMyRank)) != EOF);
	fflush (flLogfile);

	/* LoadPeopleArrays */
	LoadPeopleArrays (iGraphProcs, iMyRank, &tpPersonalInfo, liPopSize, cPathV);

	/* Request memory for the latent primary period */
	assert ((iArrayLatentPrimaryPeriod		= (int*) malloc (liPopSize * sizeof (int))) != NULL);
	/* Request memory for the latent secondary period */
	assert ((iArrayLatentSecondaryPeriod	= (int*) malloc (liPopSize * sizeof (int))) != NULL);
	/* Request memory for the infective primary period */
	assert ((iArrayInfectivePrimaryPeriod 	= (int*) malloc (liPopSize * sizeof (int))) != NULL);
	/* Request memory for the infective secondary period */
	assert ((iArrayInfectiveSecondaryPeriod	= (int*) malloc (liPopSize * sizeof (int))) != NULL);
	/* Request memory for the antiviral period */
	assert ((iArrayAntiviralPeriod 			= (int*) malloc (liPopSize * sizeof (int))) != NULL);
	/* Request memory for the antiviral delay period */
	assert ((iArrayAntiviralDelayPeriod 	= (int*) malloc (liPopSize * sizeof (int))) != NULL);
	/* Request memory for the asymptomatic period */
	assert ((iArrayAsymptomaticPeriod		= (int*) malloc (liPopSize * sizeof (int))) != NULL);
	/* Request memory for the hospitalization period */
	assert ((iArrayHospitalizationPeriod 	= (int*) malloc (liPopSize * sizeof (int))) != NULL);

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] FUNCTION: LoadPeriodsArrays\n", dTimeStamp, iMyRank)) != EOF);
	fflush (flLogfile);
	
	/* Load periods arrays */	    
	LoadPeriodsArrays (iMyRank, iGraphProcs, &iArrayLatentPrimaryPeriod, &iArrayLatentSecondaryPeriod, &iArrayInfectivePrimaryPeriod
	, &iArrayInfectiveSecondaryPeriod, &iArrayAntiviralPeriod, &iArrayAsymptomaticPeriod, &iArrayHospitalizationPeriod, &iArrayAntiviralDelayPeriod
	, liLimitLow, liLimitUp, liPopSize, cPathV);

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] FUNCTION: Spread\n", dTimeStamp, iMyRank)) != EOF);
    fflush (flLogfile);

	dTimespread_ini = MPI_Wtime();

    /* Simulate the spreading of the disease with SYNCHRONOUS communication pattern */
    SpreadSync (iMyRank, iArrayCondition, iArrayConditionTime, iSpreadingTime, iHour, iMin, iDay, iMonth, iWeekDay, iCalendar
    , iCommunicationTime, iNumberProcesses, iUpdatingTime, iStatisticsCollectionTime, &iArrayLatentPrimaryPeriod, &iArrayLatentSecondaryPeriod
    , &iArrayInfectivePrimaryPeriod, &iArrayInfectiveSecondaryPeriod, &iArrayAntiviralPeriod, &iArrayAsymptomaticPeriod
    , &iArrayHospitalizationPeriod, &iArrayAntiviralDelayPeriod, iVaccinationDay, iVaccinationPeriodLength, iVaccinationList, iSourceGraph
    , iSetTimeStamp, iBedTime, iBedStatus, iArrayInfectedPeople, iGraphProcs, liPopSize, liCheckpoint, liLimitLow, liLimitUp, liSimTime
    , csMatrix, &tpPersonalInfo, dTimeini, flLogfile, cPathV, cPathStructure, cPathNFS, cPathXML, dAvgTempMonth, dAvgHumdMonth, dBedStudents
    , dBedElderly, dBedAdults, dVaccinationPercentage, dVaccinationSusceptibleEffectiveness, dLatentPrimaryToSecondary, dLatentPrimaryToAsymptomatic
    , dInfectiveToRemoved, dInfectiveToHospitalized, dInfectiveToDead, dAsymptomaticToRemove, dHospitalizedToRemoved, dHospitalizedToDead
    , dAntiviralArray, dRiskArray, dMuGraph, dVaccinationLower64, dVaccinationGreater64, dClosingSchools, dSocialDistancing, argv);

    dTimespread_end = MPI_Wtime();

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	dSpreadTimeStamp = dTimespread_end - dTimespread_ini;
	assert ((fprintf (flLogfile, "[%6.3f \t- ID_%i] Spread-ExecutionTime:         %.3f secs.\n", dTimeStamp, iMyRank, dSpreadTimeStamp)) != EOF);
    fflush (flLogfile);

	/* Release memory for the CSC matrix */
	//cs_spfree_int (csMatrix);

	/* Release memory */
	free (iArrayLatentPrimaryPeriod);
	iArrayLatentPrimaryPeriod = NULL;
	free (iArrayLatentSecondaryPeriod);
	iArrayLatentSecondaryPeriod = NULL;
	free (iArrayInfectivePrimaryPeriod);
	iArrayInfectivePrimaryPeriod = NULL;
	free (iArrayInfectiveSecondaryPeriod);
	iArrayInfectiveSecondaryPeriod = NULL;
	free (iArrayAsymptomaticPeriod);
	iArrayAsymptomaticPeriod = NULL;
	free (iArrayAntiviralPeriod);
	iArrayAntiviralPeriod = NULL;
	free (iArrayAntiviralDelayPeriod);
	iArrayAntiviralDelayPeriod = NULL;
	free (iArrayHospitalizationPeriod);
	iArrayHospitalizationPeriod = NULL;
	//free (iArrayInfectedPeople);
	iArrayInfectedPeople = NULL;
	free (iCalendar);
	iCalendar = NULL;
	free (dAvgTempMonth);
	dAvgTempMonth = NULL;
	free (dAvgHumdMonth);
	dAvgHumdMonth = NULL;
	//free (iBedStatus);
	iBedStatus = NULL;
	//free (iArrayCondition);
	iArrayCondition = NULL;
	//free (iArrayConditionTime);
	iArrayConditionTime = NULL;
	free (dAntiviralArray);
	dAntiviralArray = NULL;
	free (dRiskArray);
	dRiskArray = NULL;
	free (tpPersonalInfo);
	tpPersonalInfo = NULL;
    
	return (0);
}
