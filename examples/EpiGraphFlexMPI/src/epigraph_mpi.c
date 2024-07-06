/****************************************************************************************************************************************
*																																		*
*	Research Project - Universidad Carlos III de Madrid [2009-2019]																		*
*                                                                                                                                       *
*	- EpiGraph: A Scalable Simulation Tool for Epidemiological Studies -																*
*																																		*
*	File:       epigraph_mpi_parent.c																									*
*																																		*
*	Authors:    Gonzalo Martin Cruz         <gmcruz@arcos.inf.uc3m.es>                                                                  *
*																																		*
*               David Exposito Singh        <desingh@arcos.inf.uc3m.es>																	*
*                                                                                                                                       *
*				Maria-Cristina Marinescu	<mcristina@arcos.inf.uc3m.es>																*
*																																		*
****************************************************************************************************************************************/

/* include */
#include "epigraph_mpi.h"
#include "./epidemiology/epidemiology.h"

/****************************************************************************************************************************************
*
*	'main'
*
*	Utility: 
*	 main function.
*
****************************************************************************************************************************************/
int main	( int	argc
			, char	*argv[] )
{

	int		iMyRank				= 0
			,iList				= 0
			,iLength			= 0
            ,iFileSystem		= 0
            ,iNumberProcesses	= 0;

	double	dTimeStamp			= 0
			,dTimeini			= 0
			,dTimenow			= 0
			,dTimefin			= 0;

	FILE	*flLogfile			= NULL;

	char	cLogfileName		[128]
			,mpi_name			[128]
			,*cPathV			= NULL
			,*cPathXML			= NULL
			,*cFileSystem		= NULL
			,*cGraphXML			= NULL
			,*cConfigXML		= NULL;

	/* MPI Initialization */
   	MPI_Init (&argc, &argv);
	
	/* MPI process rank */
	MPI_Comm_rank (EMPI_COMM_WORLD, &iMyRank);
	
	/* MPI number of processes */
	MPI_Comm_size (EMPI_COMM_WORLD, &iNumberProcesses);
	
	/* Processor name */
	MPI_Get_processor_name (mpi_name, &iLength);

	/* Init execution time */
	dTimeini = MPI_Wtime();

	if (iMyRank == MASTER) {
		puts ("\n   ==============================================================");
		puts ("            EpiGraph - Epidemiological Simulator. 2009-2019.");
		puts ("   ==============================================================");
		fflush (stdout);
	}

    /* Get the XML configuration files path */
	if (argc >= 3) {

		assert ((cPathXML = (char *) malloc ((strlen((const char *) argv[2])+40) * sizeof (char))) != NULL);
		strcpy (cPathXML, (const char *) argv[2]);
        strcpy (workdirpath, (const char *) argv[2]);
        
        strcat (cPathXML,"src/data/");

		assert ((cGraphXML = (char *) malloc ((strlen((const char *) argv[2])+50) * sizeof (char))) != NULL);
		sprintf (cGraphXML, "%s%s", cPathXML, "XMLGraphConfigFile.xml");

		assert ((cConfigXML = (char *) malloc ((strlen((const char *) argv[2])+30) * sizeof (char))) != NULL);
		sprintf (cConfigXML, "%s%s", cPathXML, "XMLConfigFile.xml");

	} else {
	
		if (iMyRank == MASTER) {
			printf ("\n  [P%d] Wrong number of parameters: List(max/maxinternal/maxexternal/mean/random] XMLpath\n\n", iMyRank);
			fflush (stdout);
		}

		/* Finish MPI */ 
  		MPI_Abort (EMPI_COMM_WORLD, -1);

  		exit (-1);
	}

	/* Get the output path */
	xmlDocPtr 	xXmlDoc;	/* Pointer to the XML config file */
	xmlNodePtr 	xRoot;		/* Pointer to the file root node */
	xmlChar 	*xValue;	/* Parameter values */

	/* Opening the XML file */
	if ((xXmlDoc = xmlParseFile (cConfigXML)) != NULL) {

		/* Root node reference */
		assert((xRoot = xmlDocGetRootElement (xXmlDoc)) != NULL);

		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "PathDest")) != NULL);
		assert ((cPathV = (char *) malloc ((strlen((const char *) xValue)+100) * sizeof (char))) != NULL);   
		strcpy (cPathV, workdirpath);
		strcat(cPathV, (const char *) xValue);
		
		assert((xValue = xmlGetProp(xRoot, (const xmlChar *) "FileSystem")) != NULL);
		assert ((cFileSystem = (char *) malloc ((strlen((const char *) xValue)+1) * sizeof (char))) != NULL);
		strcpy (cFileSystem, (const char *) xValue);

	} else {

		printf ("\n**ERROR**: opening the file '%s'\n", cConfigXML);
		fflush (stdout);

		/* Finish MPI */ 
  		MPI_Abort(EMPI_COMM_WORLD, -1);
	}

	/* File system */
	if (strcmp (cFileSystem, "pvfs") == 0) {
	
		iFileSystem = PVFS;

		if (iMyRank == MASTER)
			printf ("\n  [P%d] PVFS File system\n", iMyRank);
		
	} else	if (strcmp (cFileSystem, "nfs") == 0) {
	
				iFileSystem = NFS;
			
				if (iMyRank == MASTER)
					printf ("\n  [P%d] NFS File system\n", iMyRank);
			
			} else {
			
				printf ("\n**ERROR**: file system '%s' not recognized\n", cFileSystem);
				fflush (stdout);

				/* Finish MPI */ 
		  		MPI_Abort(EMPI_COMM_WORLD, -1);
		  		exit (-1);
			}				

	/* logfile_name = 'epigraph_[iMyRank].log' */
	sprintf (cLogfileName, "%s%s%d%s", cPathV, "log/epigraph[", iMyRank, "].log");

	/* Create logfile */
	if ((flLogfile = fopen (cLogfileName, "w")) != NULL) {

		/* LOGFILE */
		dTimenow = MPI_Wtime();
		dTimeStamp = dTimenow - dTimeini;
		assert ((fprintf (flLogfile, "[%6.3f\t- ID_%i] BEGIN SIMULATION\n", dTimeStamp, iMyRank)) != EOF);
        assert ((fprintf (flLogfile, "[%6.3f\t- ID_%i] [P%d] from processor %s - PID %i\n", dTimeStamp, iMyRank, iMyRank, mpi_name, getpid())) != EOF);
        fflush (flLogfile);

	} else {

		printf ("\n**ERROR**: creating the file '%s' in %s\n", cLogfileName, mpi_name);
		fflush (stdout);

		/* Finalizar MPI */ 
  		MPI_Abort(EMPI_COMM_WORLD, -1);
	}
     
	/* Initial infectives */
	if (strcmp (argv[1], "max") == 0)
		iList = MAXDEGREE;

	else if (strcmp (argv[1], "maxinternal") == 0)
		iList = MAXINTERNAL;

	else if (strcmp (argv[1], "maxexternal") == 0)
		iList = MAXEXTERNAL;

	else if (strcmp (argv[1], "mean") == 0)
		iList = MEAN;

	else if (strcmp (argv[1], "random") == 0)
		iList = RANDOM;

	/* Spreading */
	if (iMyRank == MASTER)
		printf ("\n  [P%d] Spreading\n", iMyRank);
	fflush (stdout);

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile, "[%6.3f\t- ID_%i] MODULE-ENTER: Spreading\n", dTimeStamp, iMyRank)) != EOF);
	fflush (flLogfile);

	/* Simulate spreading */
	EpiSim (iMyRank, iNumberProcesses, iFileSystem, iList, cConfigXML, cGraphXML, flLogfile, dTimeini, argv[2], argv);

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile, "[%6.3f\t- ID_%i] MODULE-EXIT: Spreading\n", dTimeStamp, iMyRank)) != EOF);
	fflush (flLogfile);

	/* Get the finish time */
	dTimefin = MPI_Wtime();

	if (iMyRank == MASTER) {

		/* Print final execution time */
		dTimeStamp = dTimefin - dTimeini;
		printf ("  [P%d] Execution time             : %.3f seconds  - cost %lf\n\n", iMyRank, dTimeStamp, EMPI_GLOBAL_cum_cost);
	}

	/*Tcomp y Tcomm*/
	double tcomp = 0, tcomm = 0;

	//get aggregated tcomp
	EMPI_Get_aggregated_tcomp (&tcomp);
	EMPI_Get_aggregated_tcomm (&tcomm);

	printf ("[%i] EpiGraph finished - tcomp %lf tcomm %lf - overhead %lf lbalance %lf processes %lf reconfiguring %lf rdata %lf other %lf\n", iMyRank, tcomp, tcomm, EMPI_GLOBAL_tover, EMPI_GLOBAL_overhead_lbalance, EMPI_GLOBAL_overhead_processes, EMPI_GLOBAL_overhead_rpolicy, EMPI_GLOBAL_overhead_rdata, (EMPI_GLOBAL_tover - EMPI_GLOBAL_overhead_lbalance - EMPI_GLOBAL_overhead_processes - EMPI_GLOBAL_overhead_rpolicy - EMPI_GLOBAL_overhead_rdata));

	/* LOGFILE */
	dTimenow = MPI_Wtime();
	dTimeStamp = dTimenow - dTimeini;
	assert ((fprintf (flLogfile, "[%6.3f\t- ID_%i] END SIMULATION\n", dTimeStamp, iMyRank)) != EOF);

	/* Close logfile */
	fclose (flLogfile);
		
	/* Finish MPI */ 
  	MPI_Finalize();
	
	return (0);
}
