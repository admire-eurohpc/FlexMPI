/****************************************************************************************************************************************
 *																																		*
 *	Research Project - Universidad Carlos III de Madrid [2009-2019]																		*
 *                                                                                                                                      *
 *	- EpiGraph: A Scalable Simulation Tool for Epidemiological Studies -																*
 *																																		*
 *	File:       epidemiology.h																											*
 *																																		*
 *	Authors:    Gonzalo Martin Cruz         <gmcruz@arcos.inf.uc3m.es>                                                                  *
 *																																		*
 *              David Exposito Singh        <desingh@arcos.inf.uc3m.es>																	*
 *                                                                                                                                      *
 *				Maria-Cristina Marinescu	<mcristina@arcos.inf.uc3m.es>																*
 *																																		*
 ****************************************************************************************************************************************/

#ifndef _EPIDEMIOLOGY_H_
#define _EPIDEMIOLOGY_H_

#include "../epigraph_mpi.h"

/****************************************************************************************************************************************
*
*	'LoadXmlConfigFile'
*
*	Utility:
*	 null.
*
****************************************************************************************************************************************/
int LoadXmlConfigFile    ( char     **cPathogen
                         , char     **cPathV            
                         , char     **cPathStructure
                         , char     **cPathNFS
                         , char     *cConfigXML
                         , char     *cGraphXML
                         , int*     iCommunicationTime
                         , int*     iUpdatingTime
                         , int*     iStatisticsCollectionTime
                         , int*     iSpreadingTime
                         , int*     iHour
                         , int*     iDay
                         , int*     iWeekDay
                         , int*     iMonth
                         , int*     iVaccinationDay         
                         , int*     iVaccinationPeriodLength
                         , int*     iVaccinationList
                         , int*     iMorningPeriod
                         , int*     iEveningPeriod
                         , int*     iNightPeriod
                         , int*     iSourceGraph
                         , int*     iLoadCheckpoint
                         , int*     iBedTime
                         , int*     iSimDays
                         , long int* liCheckpoint
                         , long int* liPopSize
                         , double** dAvgTempMonth
                         , double** dAvgHumdMonth
                         , double*  dBedStudents
                         , double*  dBedElderly
                         , double*  dBedAdults
                         , double*  dInitialInfectives
                         , double*  dLatentPrimaryToSecondary
                         , double*  dLatentPrimaryToAsymptomatic
                         , double*  dInfectiveToRemoved
                         , double*  dInfectiveToHospitalized
                         , double*  dInfectiveToDead
                         , double*  dAsymptomaticToRemove
                         , double*  dHospitalizedToRemoved
                         , double*  dHospitalizedToDead
                         , double*  dRiskArray
                         , double*  dVaccinationPercentage  
                         , double*  dVaccinationSusceptibleEffectiveness
                         , double*  dVaccinationLower64
                         , double*  dVaccinationGreater64
                         , double*  dClosingSchools
                         , double*  dSocialDistancing
                         , double*  dMuGraph
                         , double*  dAntiviralArray
                         , double*  dImmunityWorkers
                         , double*  dImmunityStudents
                         , double*  dImmunityElderly
                         , double*  dImmunityUnemployed );

/****************************************************************************************************************************************
*
*	'InitializeInfectives'
*
*	Utility:
*	 null.
*
****************************************************************************************************************************************/
int InitializeInfectives	( int		iMyRank
							, int**		iArrayCondition
							, int		iList
							, double	dInitialInfectives
							, char*		cPathV
							, long int	liLimitLow
							, long int	liLimitUp );

/****************************************************************************************************************************************
 *
 *	'InitialImmunes'
 *
 *	Utility: 
 *	 COMPLETE
 *
 ****************************************************************************************************************************************/
int InitialImmunes  	( int       iMyRank
                     	, struct 	tPersonalInfo	*tpPersonalInfo
                     	, int**     iArrayCondition
                     	, double    dImmunityWorkers
                     	, double    dImmunityStudents
                     	, double    dImmunityElderly
                     	, double	dImmunityHousewifes
                     	, long int	liLimitLow
                     	, long int	liLimitUp );

/****************************************************************************************************************************************
*
*	'GenLists'
*
*	Utility: 
*	 COMPLETE
*
****************************************************************************************************************************************/
int GenLists		( long int	liPopSize
					, char*		cPathV
					, char*		cPathNFS
					, double	dPercentageIndividuals
					, int		iList );

/****************************************************************************************************************************************
 *
 *	'WInfectivity'
 *
 *	Utilidad: 
 *	 ponderar la probabilidad de infección en función de las características personales del individuo.
 *
 ****************************************************************************************************************************************/
float WInfectivity  ( int       iAge
                    , int       iOccupation
                    , int       iGender
                    , int       iPrevDiseases
                    , int       iRace
                    , int       iProphylaxis
                    , int       iMonth
                    , double*   dAvgTempMonth
                    , double*   dAvgHumdMonth );

/****************************************************************************************************************************************
 *
 *	'Check'
 *
 *	Utilidad: 
 *	 evaluar la posibilidad de comprobacion de probabilidad de infeccion en funcion del dia, hora, tipo de grafo y tipo de conexion.
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
            , int 	iActiveSocialDistancing );
            
/****************************************************************************************************************************************
 *
 *	'CheckStudent'
 *
 *	Utilidad: 
 *	 check the probability of infection for students.
 *
 ****************************************************************************************************************************************/
int CheckStudent    ( int   iHour
					, int	iWeekDay
				    , int   iConnection
				    , int   iInBedA
				    , int   iOccupationB
				    , int	iAgeA
				    , int 	iAgeB
				    , int 	iActiveClosingSchools
				    , int 	iActiveSocialDistancing );

/****************************************************************************************************************************************
 *
 *	'CheckWorker'
 *
 *	Utilidad: 
 *	 check the probability of infection for workers.
 *
 ****************************************************************************************************************************************/
int CheckWorker		( int   iHour
					, int	iWeekDay
				    , int   iConnection
				    , int   iInBedA
				    , int   iOccupationB
				    , int	iAgeA
				    , int 	iAgeB
				    , int 	iActiveClosingSchools
				    , int 	iActiveSocialDistancing );

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
						, int 	iActiveSocialDistancing );

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
						, int 	iActiveSocialDistancing );
						
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
						, int 	iActiveSocialDistancing );

/****************************************************************************************************************************************
 *
 *  'SetCalendar'
 *
 *  Utility: 
 *   set the calendar of the year.
 *
 ****************************************************************************************************************************************/
int SetCalendar ( int**     iCalendar
                , int       iWeekDay );

/****************************************************************************************************************************************
*
*	'EpiSim'
*
*	Utility:
*	 null.
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
            , char 		*argv[]);

#endif
