// UCLA CS 111 Lab 1 main program

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include "command.h"
#include "command-internals.h"
#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-p PROF-FILE | -t] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  long int time_arr[4][2];
  struct timespec* temp_time=checked_malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_REALTIME,temp_time);
  time_arr[0][0]=temp_time->tv_sec;
  time_arr[0][1]=temp_time->tv_nsec;
  struct rusage ru;

  int command_number = 1;
  bool print_tree = false;
  char const *profile_name = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "p:t"))
      {
      case 'p': profile_name = optarg; break;
      case 't': print_tree = true; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);
  int profiling = -1;
  int try_prof=-1;
  if (profile_name)
    {
      try_prof=0;
      profiling = prepare_profiling (profile_name);
      if (profiling < 0)
	error (1, errno, "%s: cannot open", profile_name);
    }

  command_t last_command = NULL;
  command_t cmd;

  while ((cmd = read_command_stream (command_stream)))
    {
      if (print_tree)
	{
	  printf ("# %d\n", command_number++);
	  print_command (cmd,-1);
	}
      else
	{
	  last_command = cmd;
	  pid_t pid=fork();
	  int status;
	  if(pid==0)	    
	    execute_command (cmd, profiling);
	  else if(pid<0){}
	  else{
	    while(waitpid(pid,&status,0)!=pid)
	      ;
	    if(profiling==-1&&try_prof==0)	      
	      last_command->status=1;	      
 	    else
	      last_command->status=WEXITSTATUS(status);	      
	  }
	}
    }
  if(profiling!=-1)
    {
      clock_gettime(CLOCK_REALTIME,temp_time);
      time_arr[1][0]=temp_time->tv_sec-time_arr[0][0];
      time_arr[1][1]=temp_time->tv_nsec-time_arr[0][1];
      time_arr[0][0]=temp_time->tv_sec;
      time_arr[0][1]=temp_time->tv_nsec;
      getrusage(RUSAGE_CHILDREN,&ru);
      time_arr[2][0]=ru.ru_utime.tv_sec;
      time_arr[2][1]=ru.ru_utime.tv_usec*1000;
      time_arr[3][0]=ru.ru_stime.tv_sec;
      time_arr[3][1]=ru.ru_stime.tv_usec*1000;
      getrusage(RUSAGE_SELF,&ru);
      time_arr[2][0]+=ru.ru_utime.tv_sec;
      time_arr[2][1]+=ru.ru_utime.tv_usec*1000;
      time_arr[3][0]+=ru.ru_stime.tv_sec;
      time_arr[3][1]+=ru.ru_stime.tv_usec*1000;
      dup2(profiling,1);
      printf("%.2f ",(double)(time_arr[0][0]+(time_arr[0][1])/1E9));
      printf("%.3f ",(double)(time_arr[1][0]+(time_arr[1][1])/1E9));
      printf("%.3f ",(double)(time_arr[2][0]+(time_arr[2][1])/1E9));
      printf("%.3f ",(double)(time_arr[3][0]+(time_arr[3][1])/1E9));
      printf("[%d]\n",getpid());
      fflush(stdout);	       
    }
    return print_tree || !last_command ? 0 : command_status (last_command);
  return -1;
}
