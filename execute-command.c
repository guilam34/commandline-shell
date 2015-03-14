// UCLA CS 111 Lab 1 command execution

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

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <error.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
char* filename;
int
prepare_profiling (char const *name)
{
  int fd=open(name,O_WRONLY|O_APPEND|O_CREAT,0600);  
  return fd;
}

void profileLine(int profiling,long int time_arr[][2],command_t c,pid_t pid)
{
  dup2(profiling,1);
  printf("%.2f ",(double)(time_arr[0][0]+(time_arr[0][1])/1E9));
  printf("%.3f ",(double)(time_arr[1][0]+(time_arr[1][1])/1E9));
  printf("%.3f ",(double)(time_arr[2][0]+(time_arr[2][1])/1E9));
  printf("%.3f ",(double)(time_arr[3][0]+(time_arr[3][1])/1E9));
  if(c->type==SIMPLE_COMMAND)
    print_command(c,profiling);
  else
    printf("[%d]\n",pid);
  fflush(stdout);
}

int
command_status (command_t c)
{
  (void)c;
  return c->status;
}

void
execute_command (command_t c, int profiling)
{
  long int time_arr[4][2];
  struct timespec* temp_time=checked_malloc(sizeof(struct timespec));
  clock_gettime(CLOCK_REALTIME,temp_time);
  time_arr[0][0]=temp_time->tv_sec;
  time_arr[0][1]=temp_time->tv_nsec;
  struct rusage ru;
  pid_t pid;
  int status;
  int x;
  int arg_count=0;
  int fd[2];
  int max_word_size=1024;
  int max_word_count=10;
  char ** argv=checked_malloc(max_word_count*sizeof(char*));  
  if(c->type==PIPE_COMMAND)//DONE
    {
      if(c->input!=NULL)
	{
	  (c->u.command)[0]->input=c->input;
	  if((fd[0]=open(c->input,O_RDONLY))==-1)
	    {
	      int temp=24;
	      int ct=0;
	      while((c->input)[ct])
		ct++;
	      temp=temp+ct;
	      char* err_msg=checked_malloc(sizeof(char)*temp);
	      strcpy(err_msg,c->input);
	      strncat(err_msg," is not an existing file",temp);
	      error(1,0,err_msg);
	      exit(-1);	      
	    }
	}
      if(c->output!=NULL)
	(c->u.command)[1]->output=c->output;
      pipe(fd);
      pid=fork();
      if(pid==0)
	{
	  close(fd[0]);
	  dup2(fd[1],1);
	  execute_command((c->u.command)[0],profiling);
	}
      else if(pid<0){}
      else
	{
	  while(waitpid(pid,&status,0)!=pid)
	    ;
	  close(fd[1]);
	  dup2(fd[0],0);	
	  pid_t temp_pid=fork();
	  int temp_status;
	  if(temp_pid==0)
	    execute_command((c->u.command)[1],profiling);
	  else if(temp_pid<0){}
	  else
	    {
	      while(waitpid(temp_pid,&temp_status,0)!=temp_pid)
		;	     
	      (c->u.command)[1]->status=WEXITSTATUS(temp_status);
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
		  profileLine(profiling,time_arr,c,getpid());
		}
	      _exit(WEXITSTATUS(temp_status));	      
	    }
	}
    }
  
  if(c->type==SUBSHELL_COMMAND) //DONE
    {
      if(c->input!=NULL)
	{
	  (c->u.command)[0]->input=c->input;
	  if((fd[0]=open(c->input,O_RDONLY))==-1)
	    {
	      int temp=24;
	      int ct=0;
	      while((c->input)[ct])
		ct++;
	      temp=temp+ct;
	      char* err_msg=checked_malloc(sizeof(char)*temp);
	      strcpy(err_msg,c->input);
	      strncat(err_msg," is not an existing file",temp);
	      error(1,0,err_msg);
	      exit(-1);	      
	    }
	}
      if(c->output!=NULL)
	(c->u.command)[0]->output=c->output;
      pid=fork();
      if(pid==0)
	{
	  if(c->input!=NULL)	    
	    (c->u.command)[0]->input=c->input;
	  if(c->output!=NULL)
	    (c->u.command)[0]->output=c->output;
	  execute_command((c->u.command)[0],profiling);
	}
      else if(pid<0){}
      else{
	while(waitpid(pid,&status,0)!=pid)
	  ;
	(c->u.command)[0]->status=WEXITSTATUS(status);
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
	    profileLine(profiling,time_arr,c,getpid());
	  }
	_exit(WEXITSTATUS(status));
      }
    }
  if(c->type==IF_COMMAND)//DONE
    {
      if(c->input!=NULL)
	{
	  (c->u.command)[0]->input=c->input;
	  if((fd[0]=open(c->input,O_RDONLY))==-1)
	    {
	      int temp=24;
	      int ct=0;
	      while((c->input)[ct])
		ct++;
	      temp=temp+ct;
	      char* err_msg=checked_malloc(sizeof(char)*temp);
	      strcpy(err_msg,c->input);
	      strncat(err_msg," is not an existing file",temp);
	      error(1,0,err_msg);
	      exit(-1);	      
	    }
	}
      if(c->output!=NULL)
	{
	  (c->u.command)[0]->output=c->output;
	  (c->u.command)[1]->output=c->output;
	  if((c->u.command)[2]!=NULL)
	    (c->u.command)[2]->output=c->output;
	}
      pid=fork();
      if(pid==0)
	{	 
	  execute_command((c->u.command)[0],profiling);
	}
      else if(pid<0){}
      else{
	while(waitpid(pid,&status,0)!=pid)
	  ;
	(c->u.command)[0]->status=WEXITSTATUS(status);
	if(WEXITSTATUS(status)==0)
	  {
	    pid_t temp_pid=fork();
	    int temp_status;
	    if(temp_pid==0)	      
		execute_command((c->u.command)[1],profiling);	      
	    else if(pid<0){}
	    else{
	      while(waitpid(temp_pid,&temp_status,0)!=temp_pid)
		;
	      (c->u.command)[1]->status=WEXITSTATUS(temp_status);
	      if(profiling!=-1)
		{
		  clock_gettime(CLOCK_REALTIME,temp_time);
		  time_arr[1][0]=temp_time->tv_sec-time_arr[0][0];
		  time_arr[1][1]=temp_time->tv_nsec-time_arr[0][1];
		  time_arr[0][0]=temp_time->tv_sec;
		  time_arr[0][1]=temp_time->tv_nsec;
		  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,temp_time);
		  time_arr[2][0]=temp_time->tv_sec;
		  time_arr[2][1]=temp_time->tv_nsec;
		  getrusage(RUSAGE_CHILDREN,&ru);
		  time_arr[3][0]=ru.ru_stime.tv_sec;
		  time_arr[3][1]=ru.ru_stime.tv_usec*1000;
		  getrusage(RUSAGE_SELF,&ru);
		  time_arr[3][0]+=ru.ru_stime.tv_sec;
		  time_arr[3][1]+=ru.ru_stime.tv_usec*1000;
		  profileLine(profiling,time_arr,c,getpid());
		}
	      _exit(WEXITSTATUS(temp_status));
	    }
	  }
	else
	  if((c->u.command[2])!=NULL)	
	    {
	       pid_t temp_pid=fork();
	       int temp_status;
	       if(temp_pid==0)		 
		   execute_command((c->u.command)[2],profiling);		 
	       else if(pid<0){}
	       else{
		 while(waitpid(temp_pid,&temp_status,0)!=temp_pid)
		   ;
		 (c->u.command)[2]->status=WEXITSTATUS(temp_status);
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
		     profileLine(profiling,time_arr,c,getpid());
		   }
		 _exit(WEXITSTATUS(temp_status));
	       }
	    }
	  else
	    {
	      _exit(0);
	    }
      }
    }

  if(c->type==WHILE_COMMAND)
    {
      if(c->input!=NULL)
	{
	  (c->u.command)[0]->input=c->input;
	  if((fd[0]=open(c->input,O_RDONLY))==-1)
	    {
	      int temp=24;
	      int ct=0;
	      while((c->input)[ct])
		ct++;
	      temp=temp+ct;
	      char* err_msg=checked_malloc(sizeof(char)*temp);
	      strcpy(err_msg,c->input);
	      strncat(err_msg," is not an existing file",temp);
	      error(1,0,err_msg);
	      exit(-1);	      
	    }
	}
      if(c->output!=NULL)
	{
	  (c->u.command)[0]->output=c->output;
	  (c->u.command)[1]->output=c->output;
	}     
      while(1){
	pid=fork();
	if(pid==0)
	  {
	    execute_command((c->u.command)[0],profiling);
	  }
	else if(pid<0)
	  break;	   	  
	else{
	  while(waitpid(pid,&status,0)!=pid)
	    ;
	  (c->u.command)[0]->status=WEXITSTATUS(status);
	  if(WEXITSTATUS(status)!=0)
	    {
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
		  profileLine(profiling,time_arr,c,getpid());
		}
	      _exit((c->u.command)[1]->status);
	      break;
	    }	  
	  else{
	    pid_t temp_pid=fork();
	    int temp_status;
	    if(temp_pid==0)
	      {
		execute_command((c->u.command)[1],profiling);
	      }
	    else if(pid<0){}
	    else{
	      while(waitpid(temp_pid,&temp_status,0)!=temp_pid)
		;
	      (c->u.command)[1]->status=WEXITSTATUS(temp_status);
	    }
	  }
	}
      }
    }
  if(c->type==UNTIL_COMMAND)
    {
       if(c->input!=NULL)
	{
	  (c->u.command)[0]->input=c->input;
	  if((fd[0]=open(c->input,O_RDONLY))==-1)
	    {
	      int temp=24;
	      int ct=0;
	      while((c->input)[ct])
		ct++;
	      temp=temp+ct;
	      char* err_msg=checked_malloc(sizeof(char)*temp);
	      strcpy(err_msg,c->input);
	      strncat(err_msg," is not an existing file",temp);
	      error(1,0,err_msg);
	      exit(-1);	      
	    }
	}
       if(c->output!=NULL)
	{
	  (c->u.command)[0]->output=c->output;
	  (c->u.command)[1]->output=c->output;
	}    
      while(1){
	pid=fork();
	if(pid==0)
	  {
	    execute_command((c->u.command)[0],profiling);
	  }
	else if(pid<0){break;}	  	   	  
	else{
	  while(waitpid(pid,&status,0)!=pid)
	    ;
	  (c->u.command)[0]->status=WEXITSTATUS(status);
	  if(WEXITSTATUS(status)==0)
	    {
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
		  profileLine(profiling,time_arr,c,getpid());
		}
	      _exit(WEXITSTATUS((c->u.command)[1]->status));
	      break;
	    }	  
	  else{
	    pid_t temp_pid=fork();
	    int temp_status;
	    if(temp_pid==0)
	      {
		execute_command((c->u.command)[1],profiling);
	      }
	    else if(pid<0){}
	    else{
	      while(waitpid(temp_pid,&temp_status,0)!=temp_pid)
		;
	      (c->u.command)[1]->status=WEXITSTATUS(temp_status);
	    }	    
	  }
	}
      }
    }
  
  if(c->type==SEQUENCE_COMMAND)//DONE
    {
      if(c->input!=NULL)
	{
	  (c->u.command)[0]->input=c->input;
	  if((fd[0]=open(c->input,O_RDONLY))==-1)
	    {
	      int temp=24;
	      int ct=0;
	      while((c->input)[ct])
		ct++;
	      temp=temp+ct;
	      char* err_msg=checked_malloc(sizeof(char)*temp);
	      strcpy(err_msg,c->input);
	      strncat(err_msg," is not an existing file",temp);
	      error(1,0,err_msg);
	      exit(-1);	      
	    }
	}
      if(c->output!=NULL)
	(c->u.command)[1]->output=c->output;
      pid=fork();
      if(pid==0)	
	execute_command((c->u.command)[0],profiling);	
      else if(pid<0){}
      else{
	while(waitpid(pid,&status,0)!=pid)
	    ;
	(c->u.command)[0]->status=WEXITSTATUS(status);
	pid_t temp_pid=fork();
	int temp_status;
	if(temp_pid==0)
	  execute_command((c->u.command)[1],profiling);
	else if(temp_pid<0){}
	else
	  {
	    while(waitpid(temp_pid,&temp_status,0)!=temp_pid)
	      ;
	    (c->u.command)[1]->status=WEXITSTATUS(temp_status);
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
		profileLine(profiling,time_arr,c,getpid());
	      }
	    _exit(WEXITSTATUS(temp_status));
	  }		    
      }
    }
    
  if(c->type==SIMPLE_COMMAND)//DONE
    {
      pid=fork();
      if(pid==0)
	{
	  if(c->input!=NULL)
	    {
	      fd[0]=open(c->input,O_RDONLY);
	      if(fd[0]==-1)
		{
		  int temp=24;
		  int ct=0;
		  while((c->input)[ct])
		    ct++;
		  temp=temp+ct;
		  char* err_msg=checked_malloc(sizeof(char)*temp);
		  strcpy(err_msg,c->input);
		  strncat(err_msg," is not an existing file",temp);
		  error(1,0,err_msg);
		  exit(-1);
		}
	      else
		{
		  dup2(fd[0],0);
		}
	      close(fd[0]);
	    }
	  if(c->output!=NULL)
	    {
	      fd[1]=open(c->output,O_WRONLY|O_APPEND|O_CREAT,0600);	     
	      dup2(fd[1],1);
	      close(fd[1]);	      
	    }
	  char** word=c->u.word;
	  int cmd_length=0;
	  while((*word)[cmd_length]!=NULL)
	    {
	      cmd_length++;
	    }
	  int word_pos=0;
	  argv[0]=checked_malloc(sizeof(char)*max_word_size);
	  for(x=0;x<cmd_length;x++)
	    {
	      if(strchr(" \t",(*word)[x])!=NULL)
		{
		  arg_count++;
		  if(arg_count>max_word_count)
		    {
		      checked_realloc(argv,sizeof(char*)*max_word_count);
		      max_word_count=max_word_count*2;
		    }
		  argv[arg_count]=checked_malloc(sizeof(char)*max_word_size);
		  word_pos=0;
		  max_word_size=1024;
		  continue;
		}
	      if(word_pos==max_word_size)
		{
		  checked_realloc(argv[arg_count],sizeof(char)*max_word_size);
		  max_word_size=max_word_size*2;
		}
	      argv[arg_count][word_pos]=(*word)[x];
	      word_pos++;
	    }	 
	  arg_count++;
	  argv[arg_count]=NULL;
	  c->status=0;
	  execvp(argv[0],argv);
	  _exit(1);
	}
      else if(pid<0){}
      else{
	while(waitpid(pid,&status,0)!=pid) 
	  ;
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
	    profileLine(profiling,time_arr,c,getpid());
	  }
	if(WEXITSTATUS(status)!=0)
	  {
	    int temp=23;
	    int ct=0;
	    while((*(c->u.word))[ct])
	      ct++;
	    temp=temp+ct;
	    char* err_msg=checked_malloc(sizeof(char)*temp);
	    strcpy(err_msg,*(c->u.word));
	    strncat(err_msg," is not a valid command",temp);
	    error(1,0,err_msg);
	  }
	_exit(WEXITSTATUS(status));	  
      }
    }
}
