/****************************************************************************************************************************************
 *																																		*
 *	Research Project - Universidad Carlos III de Madrid [2009-2019]																		*
 *                                                                                                                                      *
 *	- EpiGraph: A Scalable Simulation Tool for Epidemiological Studies -																*
 *																																		*
 *	File:       auxiliar.h																											*
 *																																		*
 *	Authors:    Gonzalo Martin Cruz         <gmcruz@arcos.inf.uc3m.es>                                                                  *
 *																																		*
 *              David Exposito Singh        <desingh@arcos.inf.uc3m.es>																	*
 *                                                                                                                                      *
 *				Maria-Cristina Marinescu	<mcristina@arcos.inf.uc3m.es>																*
 *																																		*
 ****************************************************************************************************************************************/

#ifndef _AUXILIAR_H_
#define _AUXILIAR_H_

/* tPersonalInfo structure */
struct	tPersonalInfo {
	int	age;
	int	occupation;		
	int	gender;
	int	prevdiseases;
	int	race;
	int	prophylaxis;
};

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
					, int		*displs );
					
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
				, long int	liPopSize );

/****************************************************************************************************************************************
*
*	'CalcLimits'
*
*	Utility: 
*	 get the working limits of a process.
*
****************************************************************************************************************************************/
int CalcLimits	( int		iRank
				, int		iNumberProcesses
				, long int	iPopSize
				, long int* iLimitLow
				, long int* iLimitUp );

/****************************************************************************************************************************************
*
*	'Sort'
*
*	Utility:
*	 null.
*
****************************************************************************************************************************************/
int Sort	( long int**	liArrayUnordered
			, long int		liLeft
			, long int		liRight );

/****************************************************************************************************************************************
*
*	'SortPop'
*
*	Utility: 
*	 COMPLETE
*
****************************************************************************************************************************************/
int SortPop	( int*		iArrayUnordered
			, int*		iIndex
			, long int	liLeft
			, long int	liRight );
			
/****************************************************************************************************************************************
*
*	'SortPopMinMax'
*
*	Utility: 
*	 COMPLETE
*
****************************************************************************************************************************************/
int SortPopMinMax	( int*		iArrayUnordered
					, int*		iIndex
					, long int	liLeft
					, long int	liRight );
			
/****************************************************************************************************************************************
 *
 *	'StorePeriodsArrays'
 *
 *	Utilidad: 
 *	 null.
 *
 ****************************************************************************************************************************************/
int StorePeriodsArrays	( int       iMyRank
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
                        , char*     cPathV );

/****************************************************************************************************************************************
 *
 *	'LoadPeriodsArrays'
 *
 *	Utilidad: 
 *	 null.
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
                        , char*		cPathV );				

/****************************************************************************************************************************************
*
*	'StorePeopleArrays'
*
*	Utility:
*	 null.
*
****************************************************************************************************************************************/
int StorePeopleArrays	( int		iMyRank
						, struct 	tPersonalInfo *tpPersonalInfo
						, long int	liLimitLow
						, long int	liLimitUp
						, char*		cPathV );

/****************************************************************************************************************************************
 *
 *	'LoadPeopleArrays'
 *
 *	Utility:
 *	 null.
 *
 ****************************************************************************************************************************************/
int LoadPeopleArrays	( int		iNumberProcesses
						, int		iMyRank
						, struct 	tPersonalInfo	**tpPersonalInfo
                        , long int	liPopSize
                        , char*		cPathV );

/****************************************************************************************************************************************
 *
 *	'StoreCondition'
 *
 *	Utilidad: 
 *	 store CONDITION data.
 *
 ****************************************************************************************************************************************/
int StoreCondition	( int		**iArrayCondition
					, int		iMyRank
                    , int       iTimeMode
					, char*		cPathV
					, long int	liTime
					, long int	liLimitLow
					, long int 	liLimitUp
					, long int	liPopSize );

/****************************************************************************************************************************************
 *
 *	'LoadCondition'
 *
 *	Utilidad: 
 *	 null.
 *
 ****************************************************************************************************************************************/
int LoadCondition	( int		**iArrayCondition
                    , int		iMyRank
                    , int       iTimeMode
                    , char*		cPathV
                    , long int	liTime
                    , long int	liLimitLow
                    , long int	liLimitUp );

/****************************************************************************************************************************************
 *
 *   'StoreBedStatus'
 *
 *   Utilidad: 
 *    store individual's bed status.
 *
 ****************************************************************************************************************************************/
int StoreBedStatus	( int		iMyRank
					, int**		iBedStatus
					, char*		cPathV
					, long int	liTime
					, long int	liLimitLow
					, long int	liLimitUp );
                    
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
					, long int	liLimitUp );

/****************************************************************************************************************************************
*
*	'LoadMatrix'
*
*	Utility:
*	 null.
*
****************************************************************************************************************************************/
int LoadMatrix 	( int 		iMyRank
				, int*		iGraphProcs
				, long int	liCount
				, long int	liDispl
				, cs_int** 	csc_matrix
				, char*		cPathV
				, char*		cPathNFS
				, long int*	liPopSize );

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
				, FILE		*flLogfile );

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
					, char*		cPathNFS );

/****************************************************************************************************************************************
*
*	'StoreCSCMatrix'
*
*	Utility: 
*	 storage matrix in CSC format.
*
****************************************************************************************************************************************/
int StoreCSCMatrix	( int 		iMyRank
					, int		iNumberProcesses
					, char*		cPathV
					, cs_int** 	csc_matrix );

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
						, char*		cPathV );

#endif
