/****************************************************************************************************************************************
 *																																		*
*	Research Project - Universidad Carlos III de Madrid [2009-2019]																		*
 *                                                                                                                                      *
 *	- EpiGraph: A Scalable Simulation Tool for Epidemiological Studies -																*
 *																																		*
 *	File:       epigraph_mpi.h																											*
 *																																		*
 *	Authors:    Gonzalo Martin Cruz         <gmcruz@arcos.inf.uc3m.es>                                                                  *
 *																																		*
 *              David Exposito Singh        <desingh@arcos.inf.uc3m.es>																	*
 *                                                                                                                                      *
 *				Maria-Cristina Marinescu	<mcristina@arcos.inf.uc3m.es>																*
 *																																		*
 ****************************************************************************************************************************************/

/* include */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <mpi.h>
#include <empi.h>
#include <malloc.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <csparse.h>
#include <auxiliar.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <memcount.h>

/* #define */
/* Minutes per day */
#define MINDAY			1440

/* Individual States */
#define SUSCEPTIBLE                 0
#define LATENT_PRIMARY              1
#define LATENT_SECONDARY            2
#define INFECTIVE_PRIMARY           3
#define INFECTIVE_SECONDARY         4
#define	INFECTIVE_ANTIVIRAL         5
#define	ASYMPTOMATIC                6
#define HOSPITALIZED                7
#define SUSCEPTIBLE_TREATED         8
#define LATENT_PRIMARY_TREATED		9
#define LATENT_SECONDARY_TREATED	10
#define INFECTIVE_PRIMARY_TREATED	11
#define	INFECTIVE_SECONDARY_TREATED	12
#define	ASYMPTOMATIC_TREATED		13
#define HOSPITALIZED_TREATED		14
#define REMOVED                     15
#define DEAD                        16

#define	SGSIZE			17

/* Gender */
#define MALE			0
#define	FEMALE			1

/* Boolean */
#define	FALSE			0
#define TRUE			1

/* Race */
#define WHITE			0
#define BLACK			1
#define AMINDIAN		2
#define	ASIAN			3
#define PACIFIC			4
#define	OTHER			5

/* Week day */
#define MONDAY          0
#define TUESDAY         1
#define WEDNESDAY       2
#define THURSDAY        3
#define FRIDAY          4
#define SATURDAY        5
#define SUNDAY          6

/* Type day */
#define	LABORAL			1
#define WEEKEND			2
#define HOLIDAY			3

/* Risk Relationships Levels */
#define LOW_RISK		1
#define HIGH_RISK		-1

/* Lists */
#define	MAXDEGREE		0
#define MAXINTERNAL		1
#define	MAXEXTERNAL		2
#define	MEAN			3
#define RANDOM			4

/* Vaccination Lists */
#define	VACMAXDEGREE	5
#define VACMAXINTERNAL	6
#define	VACMAXEXTERNAL	7
#define	VACMEAN         8
#define VACRANDOM		9
#define NOVACC			10

/* Graph type */
#define SOCIALNETWORK   1
#define GAUSSIAN        2
#define EXPONENTIAL     3
#define POISSON         4
#define SN_FLATTENED    5

/* Active processes */
#define	NOT_ACTIVE_NOW	-2
#define	NOT_ACTIVE		-1
#define ACTIVE_NOW		2
#define	ACTIVE			1

/* Occupation */
#define WORKER			1
#define	STUDENT			2
#define ELDERLY			3
#define	UNEMPLOYED		4
#define	WORKER_SAT		5

#define OCSIZE			5

/* Connections */
#define	WORK 			1
#define LEISURE			2
#define FAMILIAR		3

/* Processes */
#define MASTER			0

/* Token */
#define TOKEN_SEND		1
#define TOKEN_RECEIVE	2
#define TOKEN_NOACT     3

/* Filesystem */
#define	NFS				0 //Secuential read/write
#define	PVFS			1 //Parallel read/write

/* Months */
#define JANUARY			1
#define FEBRUARY		2
#define MARCH			3
#define APRIL			4
#define MAY 			5
#define JUNE			6
#define JULY			7
#define AUGUST			8
#define SEPTEMBER		9
#define OCTOBER			10
#define NOVEMBER		11
#define DECEMBER		12

/* Macro CEIL */
#define CEIL(A,B)	((A + B - 1) / B)

/* Min and Max macros */
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

// Working directory path
char workdirpath[500];
