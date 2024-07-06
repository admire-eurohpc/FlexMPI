/****************************************************************************************************************************************
 *																																		*
 *	Research Project - Universidad Carlos III de Madrid [2009-2019]																		*
 *                                                                                                                                      *
 *	- EpiGraph: A Scalable Simulation Tool for Epidemiological Studies -																*
 *																																		*
 *	File:       graph_generator.h																										*
 *																																		*
 *	Authors:    Gonzalo Martin Cruz         <gmcruz@arcos.inf.uc3m.es>                                                                  *
 *																																		*
 *              David Exposito Singh        <desingh@arcos.inf.uc3m.es>																	*
 *                                                                                                                                      *
 *				Maria-Cristina Marinescu	<mcristina@arcos.inf.uc3m.es>																*
 *																																		*
 ****************************************************************************************************************************************/

#ifndef _GRAPH_GENERATOR_
#define _GRAPH_GENERATOR_

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
                        , int*		iIdGraphHousewifes
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
                        , double*	dHousewifesSigmaAge
                        , double*	dHousewifesMuAge
                        , char**	cPathV
						, char		*cConfigXML
						, char		*cGraphXML
                        , char**	cPathStructure
                        , char**	cPathSource
                        , char**	cPathNFS );
/****************************************************************************************************************************************
*
*	'GenerateGraphGroups'
*
*	Utility: 
*	 generate an file with graph groups configuration.
*
****************************************************************************************************************************************/
int GenerateGraphGroups	( int		iMyRank
						, int*		iArraySizes
						, int		iNumberProcesses
						, long int	liPopSize
						, float*	fArrayPercentages
            			, double    dCompaniesOnSaturday
						, char*		cPathStructure );

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
					, char*		cPathStructure );

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
					, float**		fArrayNNZ);

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
								, int		iIdGraphHousewifes
								, long int* liOffset
								, long int*	liOffsetLimit
								, int*		iArrayGroups
								, long int	liLimitLow
								, long int	liLimitUp
								, int**     iArrayVal
								, float		fThreshold
								, char*		cPathSource
								, double 	dTimeini
		 						, FILE		*flLogfile );

/****************************************************************************************************************************************
*
*	'GenerateExternalConnections'
*
*	Utility: 
*	 generate relationships between individuals.
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
		 						, FILE		*flLogfile );

/****************************************************************************************************************************************
*
*	'GenerateFamiliarConnections'
*
*	Utility: 
*	 generate familiar relationships between individuals.
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
									, int**     iArrayVal
									, double*	dHousehold
									, double 	dTimeini
		 							, FILE		*flLogfile );

/****************************************************************************************************************************************
*
*	'RemoveDuplicates'
*
*	Utility: 
*	 remove duplicates with value N from the matrix.
*
****************************************************************************************************************************************/
int RemoveDuplicates	( cs_int**	csMatrix );

/****************************************************************************************************************************************
*
*	'MergeConnections'
*
*	Utility: 
*	 comunica los conocidos fuera del ámbito de la matriz local a sus correspondientes procesadores.
*
****************************************************************************************************************************************/
int MergeConnections		( int		iNumberProcesses
							, int**		iArrayCol
							, int**		iArrayRow
							, int		iMyRank
							, double 	dTimeini
		 					, FILE		*flLogfile
							, long int* liOffset
							, long int	liPopSize
							, long int*	liOffsetLimit
							, int**     iArrayVal );

/****************************************************************************************************************************************
*
*	'GenPopulationInfo'
*
*	Utility:
*	 null.
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
						, double	dHousewifesSigmaAge
						, double	dHousewifesMuAge
						, FILE		*flLogfile
						, struct 	tPersonalInfo	**tpPersonalInfo );

/****************************************************************************************************************************************
 *
 *	'GenDistributionMatrix'
 *
 *	Utility: 
 *	 generate a matrix from exponential/poisson/gaussian distribution.
 *
 ****************************************************************************************************************************************/
int GenDistributionMatrix   	( int           **iArrayCol
                             	, int           **iArrayRow
                             	, int           **iArrayVal
                             	, int           iSourceGraph
                             	, double        dMuGraph
                             	, double        dSigmaGraph
                             	, int           iMyRank
                             	, long int*		liOffset
                             	, long int*     liOffsetLimit
                             	, long int      liLimitLow
                             	, long int      liLimitUp
                             	, long int      liPopSize );

#endif
