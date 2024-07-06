/****************************************************************************************************************************************
 *																																		*
 *	Research Project - Universidad Carlos III de Madrid [2009-2019]																		*
 *                                                                                                                                      *
 *	- EpiGraph: A Scalable Simulation Tool for Epidemiological Studies -																*
 *																																		*
 *	File:       epidemiology_sync.h                                                                                                     *
 *																																		*
 *	Authors:    Gonzalo Martin Cruz         <gmcruz@arcos.inf.uc3m.es>                                                                  *
 *																																		*
 *              David Exposito Singh        <desingh@arcos.inf.uc3m.es>																	*
 *                                                                                                                                      *
 *				Maria-Cristina Marinescu	<mcristina@arcos.inf.uc3m.es>																*
 *																																		*
 ****************************************************************************************************************************************/

#ifndef _EPIDEMIOLOGY_SYNC_H_
#define _EPIDEMIOLOGY_SYNC_H_

/****************************************************************************************************************************************
*
*	'SpreadSync'
*
*	Utilidad: 
*	 simulate the spreading of the epidemic with P2P synchronous communications.
*
****************************************************************************************************************************************/
int SpreadSync	( int		iMyRank
				, int*		iArrayCondition
                , int*     	iArrayConditionTime
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
				, int 		iGraphProcs
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
            	, char 		*argv[] );
		        
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
					, FILE		*flLogfile );

/****************************************************************************************************************************************
*
*	'CommInfected'
*
*	Utilidad: 
*	 communicate infected individuals
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
					, long int 	liLimitUp );

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
					, struct 	tPersonalInfo **tpPersonalInfo );

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
					, long int	liPopSize );

/****************************************************************************************************************************************
*
*	'SetCommMatrix'
*
*	Utilidad: 
*	 set the communications matrix.
*
****************************************************************************************************************************************/
int SetCommMatrix	( int		iMyRank
					, int		iNumberProcesses
					, int		**iCommMatrix );
					
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
				, double	dVaccinationGreater64 );					
#endif
