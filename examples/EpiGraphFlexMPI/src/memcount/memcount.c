#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define SIZE          8192  /* buffer size for reading /proc/<pid>/status */

int mem_total ()
{
	  char 	a[SIZE]
	  		,*p
	  		,*q;
	  		
	  int 	data 	= 0
	  		,stack 	= 0
	  		,n
	  		,v
	  		,fd;
	  		
	  pid_t pid		= getpid();

  	p = a;
  	sprintf (p, "/proc/%d/status", pid);
  	fd = open (p, O_RDONLY);
  
  	if (fd < 0)
    return -1;
    
  	do
		n = read (fd, p, SIZE);
	while ((n < 0) && (errno == EINTR));
	
	if (n < 0)
		return -2;
	
	do
		v = close (fd);
	while ((v < 0) && (errno == EINTR));
	
	if (v < 0)
		return -3;
		
	q = strstr (p, "VmData:");
	
	if (q != NULL) {
		sscanf (q, "%*s %d", &data);
	
		q = strstr (q, "VmStk:");
		
      	if (q != NULL)
    		sscanf (q, "%*s %d\n", &stack);
	}
	
	return (data+stack);
}
