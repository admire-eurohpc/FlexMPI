# README #

FLEX-MPI is a runtime system that extends the functionalities of the MPI library by providing dynamic load balancing and performance-aware malleability capabilities to MPI applications. Dynamic load balancing allows FLEX-MPI to adapt the workload assignments at runtime to the performance of the computing elements that execute the parallel application. Performance-aware malleability improves the performance of applications by changing the number of processes at runtime.

The project goals are:
 
* To provide MPI applications of malleable capabilities that permit to specify a given application execution time. FLEX-MPI will automatically create or remove new processes to adjust the execution time to the desired objective.

* We provide an external controller that is able to execute different applications and dynamically monitor the application performance using hardware counters. 

* FLEX-MPI external controller includes a I/O scheduling algorithm as well as 

* Flex-MPI performs all the optimizations during the application execution, in a transparent way, without user intervention.
 

## Requirements ##

The following libraries and compiler are required for compilation:

* GCC (>= 4.9)
* PAPI (>= 5.5.1.0)
* GLPK (GNU Linear Programming Kit) (>= 4.47)
* MPI MPICH distribution (>3.2)

## How do I get set up and execute the program? ##

Instructions are included in the FlexMPI User Manual.


## Acknowledgements ##


This work has been partially supported by the Spanish Ministry of Economy and Competitiveness 
under the project TIN2013-41350-P (Scalable Data Management Techniques for High-End Computing Systems)



## References ##

* David E. Singh and Jesus Carretero. Combining malleability and I/O control mechanisms to enhance the execution of multiple applications. Journal of Systems and Software. 2019.

* Gonzalo Martin, David E. Singh, Maria-Cristina Marinescu and Jesus Carretero. Enhancing the performance of malleable MPI applications by using performance-aware dynamic reconfiguration. Parallel Computing. Vol. 46, No. 0. Pages: 60-77. 2015.

* Manuel RodrÌguez-Gonzalo, David E. Singh, Javier GarcÌa Blas and Jes˙s Carretero. Improving the energy efficiency of MPI applications by means of malleability. 24th Euromicro International Conference on Parallel, Distributed and Network-based Processing (PDP). Heraklion, Greece, February, 2016.

* Gonzalo MartÌn, David E. Singh, Maria-Cristina Marinescu and Jes˙s Carretero. FLEX-MPI: an MPI extension for supporting dynamic load balancing on heterogeneous non-dedicated systems. European Conference on Parallel Computing (EUROPAR). Aachen, Germany. 2013.

* Gonzalo MartÌn, David E. Singh, Maria-Cristina Marinescu and Jes˙s Carretero. Runtime support for adaptive resource provisioning in MPI applications. The 19th European MPI Usersí Group Meeting ñ EuroMPI. Vienna, Austria. 2012.
