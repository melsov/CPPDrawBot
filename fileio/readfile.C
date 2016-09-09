/*
** File FILE_3.C
**
** Illustrates how to read from a file.
**
** The file is opened for reading.  Each line is successively fetched
** using fgets command.  The string is then converted to a long integer.
**
** Note that fgets returns NULL when there are no more lines in the file.
**
** In this example file ELAPSED.DTA consists of various elapsed times in
** seconds.  This may have been the result of logging the time  of events
** using an elapsed time counter which increments each second from the
** time the data logger was placed in service.
**
** Typical data in elapsed.dta might be;
**
** 65
** 142
** 1045
** 60493
** 124567
**
**
** Peter H. Anderson, 4 April, '97
*/



#include <stdio.h>   /* required for file operations */
//#include <conio.h>  /* for clrscr */
//#include <dos.h>  /* for delay */
#include "readfile.h"

FILE *ftoread;            /* declare the file pointer */

char line[80];

void initFileToRead(char path[]){
  ftoread = fopen (path, "rt");  /* open the file for reading */
  //ftoread = fopen ("fileio/readthis.txt", "rt");
}

const char * nextLine()
{
  int n;
  long elapsed_seconds;


  if(fgets(line, 80, ftoread)!=NULL){
    //    const char ** result = malloc(sizeof(line));
    //*result = strdup(line);
    return line;
  }
  return NULL;

  /* readfile.txt is the name of the file */
  /* "rt" means open the file for reading text */

  while(fgets(line, 80, ftoread) != NULL)
    {
      /* get a line, up to 80 chars from fr.  done if NULL */
      sscanf (line, "%ld", &elapsed_seconds);
      /* convert the string to a long int */
      printf ("%ld\n", elapsed_seconds);
    }
  fclose(ftoread);  /* close the file prior to exiting the routine */
} /*of main*/


