// UCLA CS 111 Lab 1 command reading

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

#include <error.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

int max_buf_size=1024;
int max_stream_size=200;
command_n last_node;
struct command_node{
  command_n prev;
  command_n next;
  command_t cmd;
};

struct command_stream{
  command_n head;
  int stream_size;
};

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  last_node=checked_malloc(sizeof(struct command_node));
  int inif=0;
  int buf_size=0;
  command_stream_t cmd_stream=checked_malloc(sizeof(struct command_stream));
  enum command_type cmd_type;
  cmd_stream->stream_size=0;
  cmd_stream->head=NULL;
  char * buffer=checked_malloc(sizeof(char)*max_buf_size);
  memset(buffer,NULL,sizeof(char)*max_buf_size);
  char cur_char;
  while((cur_char=get_next_byte(get_next_byte_argument))!=EOF)
    {
      if(buf_size==0 && strchr(" \t\n",cur_char)!=NULL)
	continue;

      if(buf_size==0 && strchr("<>;|)",cur_char)!=NULL)
	error(1,0,"invalid starting token\n");

      if(!isalnum(cur_char) && strchr("\n\t #!%+,-.\"\'/:@^_;|()<>",cur_char)==NULL)
	error(1,0,"invalid character in file\n");
      
      if(cur_char=='#' && buf_size==0)
	{
	  while(cur_char!='\n')
	    cur_char=get_next_byte(get_next_byte_argument);		
	}
      if(check_for_whileuntil(buffer,buf_size)==1&&inif==0)
	{
	  int n_st=1;
	  int n_done=0;
	  add_to_buf(cur_char,buffer,&buf_size);
	  while((cur_char=get_next_byte(get_next_byte_argument))!=EOF)
	    {	    
	      if(n_st==n_done)
		break;
	      if(strchr(" \t\n><;",cur_char)!=NULL && buffer[buf_size-1]=='e'
		 && buffer[buf_size-2]=='n' && buffer[buf_size-3]=='o'&&
		 buffer[buf_size-4]=='d'&& strchr(" \t\n><;",buffer[buf_size-5])!=NULL)
		n_done++;
	      if(strchr(" \t\n><;",cur_char)!=NULL && buffer[buf_size-1]=='e'
		 && buffer[buf_size-2]=='l' && buffer[buf_size-3]=='i'&&
		 buffer[buf_size-4]=='h' && buffer[buf_size-5]=='w'
		 && strchr(" \t\n><;",buffer[buf_size-6])!=NULL)
		n_st++;
	      if(strchr(" \t\n><;",cur_char)!=NULL && buffer[buf_size-1]=='l'
		 && buffer[buf_size-2]=='i' && buffer[buf_size-3]=='t'&&
		 buffer[buf_size-4]=='n' && buffer[buf_size-5]=='u'
		 && strchr(" \t\n><;",buffer[buf_size-6])!=NULL)
		n_st++;	      
	      add_to_buf(cur_char,buffer,&buf_size);	      
	    }
	  if(n_st!=n_done)
	    error(1,0,"Incomplete while|until statement\n");
	  if(cur_char==EOF)
	    break;
	  inif=1;
	}
      if(check_for_if(buffer,buf_size)==1&&inif==0)
	{
	  int n_if=1;
	  int n_fi=0;
	  add_to_buf(cur_char,buffer,&buf_size);
	  while((cur_char=get_next_byte(get_next_byte_argument))!=EOF)
	    {	    
	      if(n_if==n_fi)
		break;		
	      if(strchr("\t \n><;",cur_char)!=NULL && buffer[buf_size-1]=='f'
		 &&buffer[buf_size-2]=='i' && strchr("\t \n><;",buffer[buf_size-3])!=NULL)
		n_if++;		
	      if(strchr("\t \n><;",cur_char)!=NULL && buffer[buf_size-1]=='i'
		 &&buffer[buf_size-2]=='f' && strchr("\t \n><;",buffer[buf_size-3])!=NULL)
		n_fi++;	      
	      add_to_buf(cur_char,buffer,&buf_size);
	    }
	  if(n_if!=n_fi)
	    error(1,0,"Incomplete if statement\n");
	  if(cur_char==EOF)
	    break;
	  inif=1;
	}
      if(1)
	{
	  if(buf_size>=1&&cur_char=='\n'&& buffer[buf_size-1]=='\n')
	    {
	      cmd_type=check_for_command(buffer,buf_size);
	      add_command(buffer,cmd_type,cmd_stream,buf_size);
	      memset(buffer,NULL,buf_size);	      
	      buf_size=0;
	      inif=0;
	    }
	  else	      
	    add_to_buf(cur_char,buffer,&buf_size);
	}
      
    }
  if(buf_size!=0)
    {
      cmd_type=check_for_command(buffer,buf_size);
      add_command(buffer,cmd_type,cmd_stream,buf_size);
      memset(buffer,NULL,buf_size);
      buf_size=0;
      inif=0;
    }
  return cmd_stream;
}

int check_for_whileuntil(char* buffer, int buf_size){
  if(buf_size<=5)
    return -1;
  else if(buf_size==6)
    {
      if(buffer[0]=='u' && buffer[1]=='n' && buffer[2]=='t' && buffer[3]=='i'
	 && buffer[4]=='l' && strchr(" \n\t><;",buffer[5])!=NULL)
	{return 1;}
      else if(buffer[0]=='w' && buffer[1]=='h' && buffer[2]=='i' && buffer[3]=='l'
	      && buffer[4]=='e' && strchr(" \n\t><;",buffer[5])!=NULL)
	return 1;
      else
	return -1;
    }
  else
    return -1;
  
}

int check_for_if(char * buffer, int buf_size)
{
  if(buf_size<2)
    return -1;
  else if(buf_size==3)
    {
      if(buffer[0]=='i' && buffer[1]=='f' && strchr(" \n\t><;",buffer[2])!=NULL)
	return 1;
      else
	return -1;
    }
  else
    return -1;
}


void add_to_buf(char cur_char, char* buffer,int* buf_size){
  if((*buf_size)==max_buf_size)
    checked_realloc(buffer,max_buf_size);
  buffer[(*buf_size)]=cur_char;
  (*buf_size)++;
}

enum command_type check_for_command(char* buffer, int buf_size){
  int x;
  int last_paren=-1;
  /*  for(x=0;x<buf_size;x++)
    printf("%c",buffer[x]);
    printf("\n");*/
  while(strchr(" \t\n;",buffer[0])!=NULL)
    {
      buffer++;
      buf_size--;
    }
  int n_start=0;
  int n_paren=0;
  for(x=0;x<buf_size;x++)
    {
      if(x>=4 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e' && buffer[x-2]=='n'&& buffer[x-3]=='o'&& buffer[x-4]=='d' && (x==4 || strchr(" \t\n><;",buffer[x-5])!=NULL))
	n_start--;
      else
	if(x==buf_size-1 && buffer[x]=='e'&& buffer[x-1]=='n' && buffer[x-2]=='o'&& buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
	  n_start--;
      if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	 && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	 buffer[x-4]=='h' && buffer[x-5]=='w'
	 && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	n_start++;
      else
	if(x==buf_size-1 && buffer[x]=='e'&& buffer[x-1]=='l' && buffer[x-2]=='i'&& buffer[x-3]=='h'&& buffer[x-4]=='w'&&strchr(" \t\n><;",buffer[x-5])!=NULL)
	  n_start++;
      if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	 && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	 buffer[x-4]=='n' && buffer[x-5]=='u'
	 && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	n_start++;
      else
	if(x==buf_size-1 && buffer[x]=='l'&& buffer[x-1]=='i' && buffer[x-2]=='t'&& buffer[x-3]=='n'&& buffer[x-4]=='u'&&strchr(" \t\n><;",buffer[x-5])!=NULL)
	  n_start++;
      if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='o' && buffer[x-2]=='d'&& strchr(" \t\n><;",buffer[x-3])!=NULL)	
	if(n_start==0)
	  error(1,0,"do keyword invalidly used\n");
      if(x==buf_size-1 && buffer[x]=='o'&& buffer[x-1]=='d' && strchr(" \t\n><;",buffer[x-2])!=NULL)
	if(n_start==0)
	  error(1,0,"do keyword invalidly used\n");

      if(buffer[x]=='(')
	n_paren++;
      if(buffer[x]==')')
	n_paren--;
    }
  if(n_start!=0)
    error(1,0,"imbalanced while/until statement\n");
  if(n_paren!=0)
    error(1,0,"parentheses imbalance\n");

  int n_then=0;
  int n_if=0;
  for(x=0;x<buf_size;x++)
    {
      if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='i' && buffer[x-2]=='f'&& (x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))
	{
	  n_if--;
	  n_then--;
	}
      else
	if(x==buf_size-1 && buffer[x]=='i'&& buffer[x-1]=='f' &&strchr(" \t\n><;",buffer[x-2])!=NULL)
	  {
	    n_if--;
	    n_then--;
	  }
      if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='f' && buffer[x-2]=='i' &&(x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))
	n_if++;
      else
	if(x==buf_size-1 && buffer[x]=='f'&& buffer[x-1]=='i'&& strchr(" \t\n><;",buffer[x-2])!=NULL)
	  n_if++;
      if(x>=4 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='n' && buffer[x-2]=='e'&& buffer[x-3]=='h'&& buffer[x-4]=='t' && (x==4 || strchr(" \t\n><;",buffer[x-5])!=NULL))
	n_then++;
      else
	if(x==buf_size-1 && buffer[x]=='n'&& buffer[x-1]=='e' && buffer[x-2]=='h'&& buffer[x-3]=='t'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
	  n_then++;
      if(x>=4 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e' && buffer[x-2]=='s'&& buffer[x-3]=='l'&& buffer[x-4]=='e' && (x==4 || strchr(" \t\n><;",buffer[x-5])!=NULL))
	if(n_if==0)
	  error(1,0,"invalid if statement\n");
      else
	if(x==buf_size-1 && buffer[x]=='e'&& buffer[x-1]=='s' && buffer[x-2]=='l'&& buffer[x-3]=='e'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
	  if(n_if==0)
	    error(1,0,"invalid if statement\n");
   
    }
  if(n_if!=n_then)
    error(1,0,"imbalanced if statement");
  if(n_if!=0||n_then!=0)
    error(1,0,"imbalanced if statement");
  if(buf_size >=3)
    {
      for(x=0;x<buf_size;x++)
	{
	  if(strchr("\t \n",buffer[x])==NULL)
	    break;
	}
      if(x+2<buf_size)
	{
	  if(buffer[x]=='i' && buffer[x+1]=='f' && strchr(" \n\t><;",buffer[2])!=NULL)
	    return IF_COMMAND;
	}
    }

  if(buf_size >=6)
    {
      int last_done=-1;
      if(buffer[0]=='w' && buffer[1]=='h' && buffer[2]=='i' && buffer[3]=='l'
	 && buffer[4]=='e' && strchr(" \n\t><;",buffer[5])!=NULL)
	{
	  int st=0;
	  for(x=0;x<buf_size;x++)
	    {
	      if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e' && buffer[x-2]=='n'&& buffer[x-3]=='o'&& buffer[x-4]=='d' && strchr(" \t\n><;",buffer[x-5])!=NULL)
		{
		  st--;
		  if(st==0)
		    last_done=x-1;
		}
	      if(x==buf_size-1 && buffer[x]=='e'&& buffer[x-1]=='n' && buffer[x-2]=='o'&& buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
		{
		  st--;
		  if(st==0)
		    last_done=x;
		}
	      if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
		 && buffer[x-2]=='l' && buffer[x-3]=='i'&&
		 buffer[x-4]=='h' && buffer[x-5]=='w'
		 && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
		st++;
	      if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	       && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	       buffer[x-4]=='n' && buffer[x-5]=='u'
	       && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
		st++;		
	    }

	   for(x=0;x<buf_size-1;x++)
	     {
	       if(buffer[x]==';' || buffer[x]=='\n')
		 {	  
		   if(x>last_done)		    
		     return SEQUENCE_COMMAND;
		 }
	     }  
	   for(x=0;x<buf_size;x++)
	     {
	       if(buffer[x]=='|')
		 {
		   if(x>last_done)
		     return PIPE_COMMAND;
		 }
	     }	   
	    
	  return WHILE_COMMAND;
	}
      if(buffer[0]=='u' && buffer[1]=='n' && buffer[2]=='t' && buffer[3]=='i'
	 && buffer[4]=='l' && strchr(" \n\t><;",buffer[5])!=NULL)
	{
	  int st=0;
	  for(x=0;x<buf_size;x++)
	    {
	      if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e' && buffer[x-2]=='n'&& buffer[x-3]=='o'&& buffer[x-4]=='d' && strchr(" \t\n><;",buffer[x-5])!=NULL)
	      {
		st--;
		if(st==0)
		  last_done=x-1;
	      }
	    if(x==buf_size-1 && buffer[x]=='e'&& buffer[x-1]=='n' && buffer[x-2]=='o'&& buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
	      {
		st--;
		if(st==0)
		  last_done=x;
	      }
	    if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	       && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	       buffer[x-4]=='h' && buffer[x-5]=='w'
	       && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	      st++;		
	    if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	       && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	       buffer[x-4]=='n' && buffer[x-5]=='u'
	       && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	      st++;		
	    }
	   for(x=0;x<buf_size-1;x++)
	     {
	       if(buffer[x]==';' || buffer[x]=='\n')
		 {
		   if(x>last_done)		    
		     return SEQUENCE_COMMAND;
		 }
	     }  
	   for(x=0;x<buf_size;x++)
	     {
	       if(buffer[x]=='|')
		 {
		   if(x>last_done)
		     return PIPE_COMMAND;
		 }
	     }
	    
	  return UNTIL_COMMAND;
	}
    }
  if(buffer[0]=='(')
    {
      int open=1;
      int closed=0;
      for(x=1;x<buf_size;x++)
	{
	  if(buffer[x]==')')
	    {
	      last_paren=x;
	      closed++;
	    }
	  if(buffer[x]=='(')	    {
	    {
	      open++;
	    }	    
	  }
	}
      if(open!=closed)
	error(1,0,"Parentheses don't match!");
	  
      for(x=0;x<buf_size-1;x++)
	{
	  if(buffer[x]==';' || buffer[x]=='\n')
	    {
	      if(last_paren!=-1 && last_paren<x)
		return SEQUENCE_COMMAND;
	      if(last_paren==-1)
		return SEQUENCE_COMMAND;
	    }
	}  
      for(x=0;x<buf_size;x++)
	{
	  if(buffer[x]=='|')
	    {
	      if(last_paren!=-1 && last_paren<x)
		return PIPE_COMMAND;
	      if(last_paren==-1)
		return PIPE_COMMAND;
	    }
	}
  
      if(last_paren!=-1)
	{
	  for(x=last_paren+1;x<buf_size;x++)
	    {
	      if(buffer[x]==' '||buffer[x]=='\t')
		continue;
	      else if(buffer[x]=='<'||buffer[x]=='>'||buffer[x]=='\n')
		break;
	      else
		error(1,0,"Error: invalid subshell");
	    }
	  return SUBSHELL_COMMAND;
	}
    }
  for(x=0;x<buf_size-1;x++)
    {
      if(buffer[x]==';' || buffer[x]=='\n')
	{
	  return SEQUENCE_COMMAND;
	}
    }  

  for(x=0;x<buf_size;x++)
    {
      if(buffer[x]=='|')
	{
	  return PIPE_COMMAND;
	}
    }
       
  return SIMPLE_COMMAND;
}

void add_command(char* buffer, enum command_type cmd_type,command_stream_t cmd_stream,
		 int buf_s)
{
  int x;
  int input=0;
  int output=0;
  char* temp_buf=checked_malloc(sizeof(char)*buf_s);
  int temp_bufsize=0;
  command_t cmd_t=checked_malloc(sizeof(struct command));
  (cmd_t)->type=cmd_type;
  (cmd_t)->status=-1;
  (cmd_t)->simple_size=0;
  input=get_input(buffer,cmd_t,cmd_type,buf_s);
  output=get_output(buffer,cmd_t,cmd_type,buf_s);
  int last_spot;
  if(input==-1)
    (cmd_t)->input=NULL;
  if(output==-1)
    (cmd_t)->output=NULL;

  if(input!=-1&&output!=-1)
    {
      if(input<output)
	last_spot=input;
      else
	last_spot=output;
    }
  else if(input!=-1)
    last_spot=input;
  else if(output!=-1)
    last_spot=output;
  else
    last_spot=buf_s;

  for(x=0;x<last_spot;x++)
    {
      temp_buf[temp_bufsize]=buffer[x];
      temp_bufsize++;
    }
 
  if(cmd_type==SIMPLE_COMMAND)
    {
      int start=0;
      int end=temp_bufsize-1;
      for(x=0;x<temp_bufsize;x++)
	{
	  if(strchr(" \t\n",temp_buf[x])==NULL)
	    {
	      start=x;
	      break;
	    }
	}
       for(x=temp_bufsize-1;x>=0;x--)
	{
	  if(strchr(" \t\n",temp_buf[x])==NULL)
	    {
	      end=x;
	      break;
	    }
	}
      (cmd_t)->u.word=checked_malloc(sizeof(char*)*buf_s);
      *((cmd_t)->u.word)=checked_malloc(sizeof(char)*buf_s);
      for(x=0;x<=end-start;x++)
	{
	  if(temp_buf[start+x]=='\n')
	    continue;	  
	  else
	    (*((cmd_t)->u.word))[x]=temp_buf[start+x];
	}
    }
  else{
    ((cmd_t)->u.command)=checked_malloc(sizeof(struct command*)*3);
    *((cmd_t)->u.command)=checked_malloc(sizeof(struct command)*3);
    get_inner_commands(temp_buf,cmd_type,(cmd_t)->u.command,temp_bufsize);
  }

  
  command_n cmd_n=checked_malloc(sizeof(struct command_node));
  if(cmd_stream->stream_size==0)
    {
      (cmd_n)->prev=NULL;
      cmd_stream->head=cmd_n;
    }
  else
    {
      (cmd_n)->prev=last_node;
      (last_node)->next=cmd_n;
    }
  (cmd_n)->next=NULL;
  last_node=cmd_n;
  (cmd_n)->cmd=cmd_t;
  (cmd_stream->stream_size)++;

}

void get_inner_commands(char * buffer, enum command_type cmd_type, command_t* sub_cmds, int buf_size)
{

  int x;
  int y;
  enum command_type c_type;
  char* temp_buf=checked_malloc(sizeof(char)*buf_size);
  int temp_bufsize=0;
///////////////////////////////////////////////////////////////
  if(cmd_type==IF_COMMAND)
    {
      int n_st=0;
      int st_pos=0;
      int fi_pos=0;
      int then_pos=-1;
      int else_pos=-1;
      for(x=0;x<buf_size;x++)
	{
	  if(x>=3 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='i' && buffer[x-2]=='f' && strchr(" \t\n><;",buffer[x-3])!=NULL)
	    {	     
	      if(n_st==1)
		fi_pos=x;
	      n_st--;
	    }
	  if(x==buf_size-1 && buffer[x]=='i'&& buffer[x-1]=='f' && strchr(" \t\n><;",buffer[x-2])!=NULL)
	    {	     
	      if(n_st==1)
		fi_pos=x;
	      n_st--;
	    }
	  if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='f'
	     && buffer[x-2]=='i' && (x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))        
	    {
	      if(n_st==0)
		st_pos=x;
	      n_st++;
	    }
	  if(x>=5 && strchr("\t \n><;",buffer[x])!=NULL && buffer[x-1]=='n'
	     &&buffer[x-2]=='e' && buffer[x-3]=='h'&& buffer[x-4]=='t' &&
	     strchr("\t \n><;",buffer[x-5])!=NULL)
	    if(n_st==1)
	      {	
		then_pos=x;
	      }
	  if(x>=5 && strchr("\t \n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     &&buffer[x-2]=='s' && buffer[x-3]=='l'&& buffer[x-4]=='e' &&
	     strchr("\t \n><;",buffer[x-5])!=NULL)
	    if(n_st==1 && then_pos!=-1)
	      {
		else_pos=x;
	      }
	}
      int st_endpos=then_pos-5;
      int then_endpos=fi_pos-3;
      int else_endpos=fi_pos-3;
      while(st_endpos!=st_pos)
	{
	  if(isalnum(buffer[st_endpos])!=0||strchr(":()!",buffer[st_endpos])!=NULL)
	    break;
	  else
	    st_endpos--;
	}
      if(else_pos!=-1)
	{
	  then_endpos=else_pos-5;
	  while(then_endpos!=then_pos)
	    {
	      if(isalnum(buffer[then_endpos])!=0||strchr(":()!",buffer[then_endpos])!=NULL)
		break;
	      else
		then_endpos--;
	    }
	  while(else_endpos!=else_pos)
	    {
	      if(isalnum(buffer[else_endpos])!=0||strchr(":()!",buffer[else_endpos])!=NULL)
		break;
	      else
		else_endpos--;
	    }
	}
      else
	{	 
	  while(then_endpos!=then_pos)
	    {
	      if(isalnum(buffer[then_endpos])!=0 ||strchr(":()!",buffer[then_endpos])!=NULL)
		break;
	      else
		then_endpos--;
	    }
      }
      
      for(y=st_pos;y<=st_endpos;y++)
	{
	  if(temp_bufsize==0 && strchr(" \t\n",buffer[y])!=NULL)
	    continue;	      
	  if(temp_bufsize==0&& strchr("|<>);",buffer[y])!=NULL)
	    error(1,0,"invalid if\n");
	  else
	    {
	      temp_buf[temp_bufsize]=buffer[y];
	      temp_bufsize++;	 
	    }	  		    	  
	}
      if(temp_bufsize==0)
	error(1,0,"invalid expression");      
      c_type=check_for_command(temp_buf,temp_bufsize);
      sub_cmds[0]=inner_command(c_type,temp_buf,temp_bufsize);
      temp_bufsize=0;
      for(y=then_pos;y<=then_endpos;y++)
	{
	  if(temp_bufsize==0 && strchr(" \t\n",buffer[y])!=NULL)
	    continue;	      
	  if(temp_bufsize==0&&strchr(";|<>)",buffer[y])!=NULL)
	    error(1,0,"invalid if\n");
	  else
	    {
	      temp_buf[temp_bufsize]=buffer[y];
	      temp_bufsize++;
	    }	 
	}
      if(temp_bufsize==0)
	error(1,0,"invalid expression");      
      c_type=check_for_command(temp_buf,temp_bufsize);
      sub_cmds[1]=inner_command(c_type,temp_buf,temp_bufsize);
      temp_bufsize=0;      
      
      if(else_pos!=-1)
	{
	  for(y=else_pos;y<=else_endpos;y++)
	    {
	      if(temp_bufsize==0 && strchr(" \t\n",buffer[y])!=NULL)
		continue;	      
	      if(temp_bufsize==0&&strchr("|<>;)",buffer[y])!=NULL)
		error(1,0,"invalid if\n");
	      else
		{
		  temp_buf[temp_bufsize]=buffer[y];
		  temp_bufsize++;
		}
	    }
	  if(temp_bufsize==0)
	    error(1,0,"invalid expression");
	  c_type=check_for_command(temp_buf,temp_bufsize);	  
	  sub_cmds[2]=inner_command(c_type,temp_buf,temp_bufsize);
	  temp_bufsize=0;
	}
    }
  ///////////////////////////////////////////////////////////////
  if(cmd_type==UNTIL_COMMAND||cmd_type==WHILE_COMMAND)
    {
      int n_st=0;
      int st_pos=0;
      int done_pos=0;
      int do_pos=-1;
      for(x=0;x<buf_size;x++)
	{
	  if(x>=4 && strchr(" \t\n><;!()",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='n' && buffer[x-3]=='o'&&
	     buffer[x-4]=='d'&& strchr(" \t\n><;!()",buffer[x-5])!=NULL)
	    {	      
	      if(n_st==1)
		done_pos=x-1;
	      n_st--;
	    }	  
	  if(x==buf_size-1 && buffer[x]=='e'
	     && buffer[x-1]=='n' && buffer[x-2]=='o'&&
	     buffer[x-3]=='d'&& strchr(" \t\n><;!()",buffer[x-4])!=NULL)
	    {	      
	      if(n_st==1)
		done_pos=x;
	      n_st--;
	    }
	  if(x>=5 && strchr(" \t\n><;!()",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	     buffer[x-4]=='h' && buffer[x-5]=='w'
	     && (x==5 || strchr(" \t\n><;!()",buffer[x-6])!=NULL))
	    {
	      if(n_st==0)
		st_pos=x; 
	      n_st++;
	    }
	  if(x>=5 && strchr(" \t\n><;()!",buffer[x])!=NULL && buffer[x-1]=='l'
	     && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	     buffer[x-4]=='n' && buffer[x-5]=='u'
	     && (x==5 || strchr(" \t\n><;()",buffer[x-6])!=NULL))
	    {
	      if(n_st==0)
		st_pos=x;
	      n_st++;
	    }
	  if(x>=3 && strchr("\t \n><;()!",buffer[x])!=NULL && buffer[x-1]=='o'
	     &&buffer[x-2]=='d' && strchr("\t \n><;()!",buffer[x-3])!=NULL)
	    if(n_st==1)
	      {
		do_pos=x;	
	      }
	}
      int st_endpos=done_pos-4;
      int do_endpos=done_pos-4;
      if(do_pos!=-1)
	{
	  st_endpos=do_pos-3;
	  while(st_endpos!=st_pos)
	    {
	      if(isalnum(buffer[st_endpos])!=0 ||strchr(":()!",buffer[st_endpos])!=NULL)
		break;
	      else
		st_endpos--;
	    }
	  while(do_endpos!=do_pos)
	    {
	      if(isalnum(buffer[do_endpos])!=0||strchr(":()!",buffer[do_endpos])!=NULL)
		break;
	      else
		do_endpos--;
	    }
	}
      else
	{
	  while(st_endpos!=st_pos)
	    {
	      if(isalnum(buffer[st_endpos])!=0 ||strchr(":()!",buffer[st_endpos])!=NULL)
		break;
	      else
		st_endpos--;
	    }
      }
      
      for(y=st_pos;y<=st_endpos;y++)
	{
	  if(temp_bufsize==0 && strchr(" \t\n",buffer[y])!=NULL)
	    continue;	      
	  if(temp_bufsize==0&&strchr(";|<>)",buffer[y])!=NULL)
	    error(1,0,"invalid while/until\n");
	  else
	    {
	      temp_buf[temp_bufsize]=buffer[y];
	      temp_bufsize++;
	    }
	}
      if(temp_bufsize==0)
	error(1,0,"invalid expression");      
      c_type=check_for_command(temp_buf,temp_bufsize);
      sub_cmds[0]=inner_command(c_type,temp_buf,temp_bufsize);
      temp_bufsize=0;
      if(do_pos!=-1)
	{
	  for(y=do_pos;y<=do_endpos;y++)
	    {
	      if(temp_bufsize==0 && strchr(" \t\n",buffer[y])!=NULL)
		continue;	      
	      if(temp_bufsize==0&&strchr(";|<>)",buffer[y])!=NULL)
		error(1,0,"invalid while/until\n");		
	      else
		{
		  temp_buf[temp_bufsize]=buffer[y];
		  temp_bufsize++;
		}
	    }
	  if(temp_bufsize==0)
	    error(1,0,"invalid expression");	  
	  c_type=check_for_command(temp_buf,temp_bufsize);
	  sub_cmds[1]=inner_command(c_type,temp_buf,temp_bufsize);
	  temp_bufsize=0;
	}      
    }
  if(cmd_type==PIPE_COMMAND)
    {
      int is_wu=buf_size+1;
      for(x=0;x<buf_size;x++)
	{
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	     buffer[x-4]=='h' && buffer[x-5]=='w'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	     && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	     buffer[x-4]=='n' && buffer[x-5]=='u'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	}
       int last_done=-1;    
       if(is_wu!=buf_size+1)
	 {
	   for(x=0;x<buf_size;x++)
	     {
	       if(x>=4 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='n' && buffer[x-3]=='o'&&
		  buffer[x-4]=='d'&& strchr(" \t\n><;",buffer[x-5])!=NULL)
		 last_done=x-4;
	       if(x==buf_size-1 && buffer[x]=='e'
		  && buffer[x-1]=='n' && buffer[x-2]=='o'&&
		  buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
		 last_done=x-3;
	     }	     
	 }
       int is_if=buf_size+1;
       int last_fi=-1;
       for(x=0;x<buf_size;x++)
	 {
	   if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='f'
	      && buffer[x-2]=='i'
	      && (x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))
	     is_if=x-2;
	 }
	if(is_if!=buf_size+1)
	  {
	    for(x=0;x<buf_size;x++)
	      {
		if(x>=3 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='i' && buffer[x-2]=='f' && strchr(" \t\n><;",buffer[x-3])!=NULL)
		  last_fi=x-1;
		if(x==buf_size-1 && buffer[x]=='i'&& buffer[x-1]=='f' && strchr(" \t\n><;",buffer[x-2])!=NULL)
		  last_fi=x;
	      }
	  }      
      int in_paren=0;
      for(x=0;x<buf_size;x++)
	{
	  if(buffer[x]=='(')
	    in_paren++;
	  if(buffer[x]==')')
	    in_paren--;
	  if(buffer[x]=='|'&&in_paren==0&&(x<is_wu || x>last_done)&&(x<is_if||x>last_fi))
	    break;
	}
      for(y=0;y<x;y++)
	{
	  if(temp_bufsize==0&&strchr(" \t\n",buffer[y])!=NULL)
	    continue;
	  if(temp_bufsize==0&&strchr(";|<>)",buffer[y])!=NULL)
	    error(1,0,"incomplete pipe command");
	  else
	    {
	      temp_buf[temp_bufsize]=buffer[y];
	      temp_bufsize++;
	    }
	}
      if(temp_bufsize==0)
	error(1,0,"invalid expression");      
      c_type=check_for_command(temp_buf,temp_bufsize);
      sub_cmds[0]=inner_command(c_type,temp_buf,temp_bufsize);
      temp_bufsize=0;
      
      for(y=x+1;y<buf_size;y++)
	{
	  if(temp_bufsize==0&&strchr(" \t\n",buffer[y])!=NULL)
	    continue;
	  if(temp_bufsize==0&&strchr(";|<>)",buffer[y])!=NULL)
	    error(1,0,"pipe\n");
	  else
	    {
	      temp_buf[temp_bufsize]=buffer[y];
	      temp_bufsize++;
	    }
	}

      if(temp_bufsize==0)
	error(1,0,"invalid expression");
      c_type=check_for_command(temp_buf,temp_bufsize);
      sub_cmds[1]=inner_command(c_type,temp_buf,temp_bufsize);
      temp_bufsize=0;
      
    }
  ////////////////////////////////////////////////////////////////
    if(cmd_type==SEQUENCE_COMMAND)
    {
         int is_wu=buf_size+1;
      for(x=0;x<buf_size;x++)
	{
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	     buffer[x-4]=='h' && buffer[x-5]=='w'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	     && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	     buffer[x-4]=='n' && buffer[x-5]=='u'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	}
       int last_done=-1;    
       if(is_wu!=buf_size+1)
	 {
	   for(x=0;x<buf_size;x++)
	     {
	       if(x>=4 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='n' && buffer[x-3]=='o'&&
		  buffer[x-4]=='d'&& strchr(" \t\n><;",buffer[x-5])!=NULL)
		 last_done=x-4;
	       if(x==buf_size-1 && buffer[x]=='e'
		  && buffer[x-1]=='n' && buffer[x-2]=='o'&&
		  buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
		 last_done=x-3;
	     }	     
	 }

       int is_if=buf_size+1;
       int last_fi=-1;
       for(x=0;x<buf_size;x++)
	 {
	   if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='f'
	      && buffer[x-2]=='i'
	      && (x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))
	     is_if=x-2;
	 }
	if(is_if!=buf_size+1)
	  {
	    for(x=0;x<buf_size;x++)
	      {
		if(x>=3 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='i' && buffer[x-2]=='f' && strchr(" \t\n><;",buffer[x-3])!=NULL)
		  last_fi=x-1;
		if(x==buf_size-1 && buffer[x]=='i'&& buffer[x-1]=='f' && strchr(" \t\n><;",buffer[x-2])!=NULL)
		  last_fi=x;
	      }
	  }
         
      int in_paren=0;
      for(x=0;x<buf_size-1;x++)
	{
	  if(buffer[x]=='(')
	    in_paren++;
	  if(buffer[x]==')')
	    in_paren--;
	  if(buffer[x]==';'&&in_paren==0&&(x<is_wu || x>last_done)&&(x<is_if||x>last_fi))
	    break;
	  if(buffer[x]=='\n'&&in_paren==0&&(x<is_wu || x>last_done)&&(x<is_if||x>last_fi))
	    break;
	}

      for(y=0;y<x;y++)
	{
	  if(temp_bufsize==0&&strchr(" \t\n",buffer[y])!=NULL)
	    continue;
	  if(temp_bufsize==0&&strchr(";|<>)",buffer[y])!=NULL)
	    error(1,0,"incomplete sequence\n");
	  else
	    {
	      temp_buf[temp_bufsize]=buffer[y];
	      temp_bufsize++;
	    }
	}
      if(temp_bufsize==0)
	error(1,0,"invalid expression");
      c_type=check_for_command(temp_buf,temp_bufsize);
      sub_cmds[0]=inner_command(c_type,temp_buf,temp_bufsize);
      temp_bufsize=0;
      
      for(y=x+1;y<buf_size;y++)
	{
	  if(temp_bufsize==0&&strchr(" \t\n",buffer[y])!=NULL)
	    continue;
	  if(temp_bufsize==0&&strchr(" ;|<>)",buffer[y])!=NULL)
	    break;
	  else
	    {
	      temp_buf[temp_bufsize]=buffer[y];
	      temp_bufsize++;
	    }	  
	}
      if(temp_bufsize==0)
	error(1,0,"invalid expression");
      c_type=check_for_command(temp_buf,temp_bufsize);
      sub_cmds[1]=inner_command(c_type,temp_buf,temp_bufsize);
      temp_bufsize=0;
      
    }
  ////////////////////////////////////////////////////////////////
  if(cmd_type==SUBSHELL_COMMAND)
    {
      int last_paren=0;
      for(x=0;x<buf_size;x++)
	{	  
	  if(buffer[x]==')')
	    last_paren=x;
	}
      for(y=1;y<=last_paren-1;y++)
	{
	  if(temp_bufsize==0&&strchr(" \t\n",buffer[y])!=NULL)
	    continue;
	  if(temp_bufsize==0&&strchr(";|<>)",buffer[y])!=NULL)
	    error(1,0,"incomplete simple command");
	  else
	    {
	      temp_buf[temp_bufsize]=buffer[y];
	      temp_bufsize++;
	    }	  
	}
      if(temp_bufsize==0)
	error(1,0,"invalid expression");
      c_type=check_for_command(temp_buf,temp_bufsize);
      sub_cmds[0]=inner_command(c_type,temp_buf,temp_bufsize);
      temp_bufsize=0;
    }
}

command_t inner_command(enum command_type cmd_type, char* buffer, int buf_size)
{
  int x;
  int input=0;
  int output=0;
  char* temp_buf=checked_malloc(sizeof(char)*buf_size);
  int temp_bufsize=0;
  command_t cmd_t=checked_malloc(sizeof(struct command));
  (cmd_t)->type=cmd_type;
  (cmd_t)->status=-1;
  (cmd_t)->simple_size=0;
  input=get_input(buffer,cmd_t,cmd_type,buf_size);
  output=get_output(buffer,cmd_t,cmd_type,buf_size);
  int last_spot;
  if(input==-1)
    (cmd_t)->input=NULL;
  if(output==-1)
    (cmd_t)->output=NULL;

  if(input!=-1&&output!=-1)
    {
      if(input<output)
	last_spot=input;
      else
	last_spot=output;
    }
  else if(input!=-1)
    last_spot=input;
  else if(output!=-1)
    last_spot=output;
  else
    last_spot=buf_size;

  for(x=0;x<last_spot;x++)
    {
      if(buffer[x]==EOF)
	break;
      temp_buf[temp_bufsize]=buffer[x];
      temp_bufsize++;
    }
  if(cmd_type==SIMPLE_COMMAND)
    {
      int start=0;
      int end=temp_bufsize-1;
      for(x=0;x<temp_bufsize;x++)
	{	 
	  if(strchr(" \t\n",temp_buf[x])==NULL)
	    {
	      start=x;
	      break;
	    }
	}
       for(x=temp_bufsize-1;x>=0;x--)
	{
	  if(strchr(" \t\n",temp_buf[x])==NULL)
	    {
	      end=x;
	      break;
	    }
	}
      (cmd_t)->u.word=checked_malloc(sizeof(char*)*buf_size);
      *((cmd_t)->u.word)=checked_malloc(sizeof(char)*buf_size);
      for(x=0;x<=end-start;x++)
	{
	  if(temp_buf[start+x]=='\n')
	    continue;
	  if(temp_buf[start+x]==EOF)
	    break;
	  else
	    (*((cmd_t)->u.word))[x]=temp_buf[start+x];
	}
    }
  else{
    ((cmd_t)->u.command)=checked_malloc(sizeof(struct command*)*3);
    *((cmd_t)->u.command)=checked_malloc(sizeof(struct command)*3);
    get_inner_commands(temp_buf,cmd_type,(cmd_t)->u.command,temp_bufsize);
  }
  return cmd_t;
}

int get_input(char* buffer,command_t cmd_t,enum command_type cmd_type,int buf_size)
{
  int x;
  int last_arrow=-2;
  int in_size=0;
  ((cmd_t)->input)=checked_malloc(sizeof(char)*buf_size+1);
  if(cmd_type==IF_COMMAND)
    {
      int last_fi=-1;
      int n_ifs=0;
      for(x=0;x<buf_size;x++)
	{
	  if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='f'
	     && buffer[x-2]=='i' && (x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))
	    n_ifs++;
	  else
	    if(x==buf_size-1 &&buffer[x]=='f'
	     && buffer[x-1]=='i'&& strchr(" \t\n><;",buffer[x-2])!=NULL)
	      n_ifs++;
	  if(x>=3 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='i' && buffer[x-2]=='f' && strchr(" \t\n><;",buffer[x-3])!=NULL)
	     {
	      n_ifs--;
	      if(n_ifs==0)
		last_fi=x-1;
	    }
	  else
	    if (x==buf_size-1 && buffer[x]=='i'&& buffer[x-1]=='f' && strchr(" \t\n><;",buffer[x-2])!=NULL)
	      {
		n_ifs--;
		if(n_ifs==0)
		  last_fi=x;
	      }
	  if(buffer[x]=='<' && last_fi!=-1 && last_arrow!=-2)
	    error(1,0,"Error: only one input allowed\n");
	  if(buffer[x]=='<' &&last_fi!=-1)
	    last_arrow=x;
	}
      if(last_arrow==-2)
	{
	  for(x=last_fi+1;x<buf_size;x++)
	    {
	      if(buffer[x]=='>')
		break;
	      if(buffer[x]==EOF)
		break;
	      if(strchr("\t ;\n\0",buffer[x])==NULL)
		error (1,0,"invalid text following if statement\n");
	    }
	}
      if(last_arrow<last_fi)
	return -1;      
    }
  
  if(cmd_type==WHILE_COMMAND || cmd_type==UNTIL_COMMAND)
    {
      int st=0;
      int last_done=-1;
      for(x=0;x<buf_size;x++)
	{
	   if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	     buffer[x-4]=='h' && buffer[x-5]=='w'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))	 
	     st++;;
	   if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	      && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	      buffer[x-4]=='n' && buffer[x-5]=='u'
	      && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	     st++;
	    if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e' && buffer[x-2]=='n'&& buffer[x-3]=='o'&& buffer[x-4]=='d' && strchr(" \t\n><;",buffer[x-5])!=NULL)
	      {
	       st--;
	       if(st==0)
		 last_done=x-1;
	     }
	    else
	      if(x==buf_size-1 && buffer[x]=='e'&& buffer[x-1]=='n' && buffer[x-2]=='o'&& buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
		{
		  st--;
		  if(st==0)
		    last_done=x;
		}
	   if(buffer[x]=='<' && last_done!=-1 && last_arrow!=-2)
	    error(1,0,"Error: only one in allowed\n");
	   if(buffer[x]=='<' && last_done!=-1)
	     last_arrow=x;	  
	}
     
      if(last_arrow==-2)
	{
	  for(x=last_done+1;x<buf_size;x++)
	    {
	      if(buffer[x]=='>')
		break;
	      if(buffer[x]==EOF)
		break;
	      if(strchr("\t ;\n\0",buffer[x])==NULL)
		error (1,0,"invalid text following while statement\n");

	    }
	}      
      if(last_arrow<last_done)
	return -1;
    }

  if(cmd_type==PIPE_COMMAND)
    {
      int is_wu=buf_size+1;
      for(x=0;x<buf_size;x++)
	{
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	     buffer[x-4]=='h' && buffer[x-5]=='w'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	     && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	     buffer[x-4]=='n' && buffer[x-5]=='u'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	}
       int last_done=-1;    
       if(is_wu!=buf_size+1)
	 {
	   for(x=0;x<buf_size;x++)
	     {
	       if(x>=4 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='n' && buffer[x-3]=='o'&&
		  buffer[x-4]=='d'&& strchr(" \t\n><;",buffer[x-5])!=NULL)
		 last_done=x-4;
	       if(x==buf_size-1 && buffer[x]=='e'
		  && buffer[x-1]=='n' && buffer[x-2]=='o'&&
		  buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
		 last_done=x-3;
	     }	     
	 }
       int is_if=buf_size+1;
       int last_fi=-1;
       for(x=0;x<buf_size;x++)
	 {
	   if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='f'
	      && buffer[x-2]=='i'
	      && (x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))
	     is_if=x-2;
	 }
	if(is_if!=buf_size+1)
	  {
	    for(x=0;x<buf_size;x++)
	      {
		if(x>=3 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='i' && buffer[x-2]=='f' && strchr(" \t\n><;",buffer[x-3])!=NULL)
		  last_fi=x-1;
		if(x==buf_size-1 && buffer[x]=='i'&& buffer[x-1]=='f' && strchr(" \t\n><;",buffer[x-2])!=NULL)
		  last_fi=x;
	      }
	  }
      
      int last_pipe=-1;
      int in_paren=0;
      for(x=0;x<buf_size;x++)
	{
	  if(buffer[x]=='(')
	    in_paren++;
	  if(buffer[x]==')')
	    in_paren--;
	  if(buffer[x]=='|'&&in_paren==0&&(x<is_wu || x>last_done)&&(x<is_if||x>last_fi))
	    last_pipe=x; 	
	}
      for(x=last_pipe;x<buf_size;x++)
	{
	   if(buffer[x]=='<' &&in_paren==0&& last_arrow!=-2&&(x<is_wu || x>last_done)&&(x<is_if||x>last_fi))
	    error(1,0,"Error: only one input allowed\n");
	  if(buffer[x]=='<' &&in_paren==0 &&(x<is_wu || x>last_done)&&(x<is_if||x>last_fi))
	    last_arrow=x;
	}
      if(last_arrow<last_pipe)
	return -1;
    }

  if(cmd_type==SEQUENCE_COMMAND)
    {
      int is_wu=buf_size+1;
      for(x=0;x<buf_size;x++)
	{
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	     buffer[x-4]=='h' && buffer[x-5]=='w'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	     && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	     buffer[x-4]=='n' && buffer[x-5]=='u'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	}
       int last_done=-1;    
       if(is_wu!=buf_size+1)
	 {
	   for(x=0;x<buf_size;x++)
	     {
	       if(x>=4 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='n' && buffer[x-3]=='o'&&
		  buffer[x-4]=='d'&& strchr(" \t\n><;",buffer[x-5])!=NULL)
		 last_done=x-4;
	       if(x==buf_size-1 && buffer[x]=='e'
		  && buffer[x-1]=='n' && buffer[x-2]=='o'&&
		  buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
		 last_done=x-3;
	     }	     
	 }
       int is_if=buf_size+1;
       int last_fi=-1;
       for(x=0;x<buf_size;x++)
	 {
	   if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='f'
	      && buffer[x-2]=='i'
	      && (x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))
	     is_if=x-2;
	 }
       if(is_if!=buf_size+1)
	 {
	   for(x=0;x<buf_size;x++)
	     {
	       if(x>=3 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='i' && buffer[x-2]=='f' && strchr(" \t\n><;",buffer[x-3])!=NULL)
		 last_fi=x-1;
	       if(x==buf_size-1 && buffer[x]=='i'&& buffer[x-1]=='f' && strchr(" \t\n><;",buffer[x-2])!=NULL)
		 last_fi=x;
	     }
	 }
       int in_paren=0;
       int last_seq=-1;
       for(x=0;x<buf_size;x++)
	 {
	   if(buffer[x]=='(')
	     in_paren++;
	   if(buffer[x]==')')
	     in_paren--;
	   if((buffer[x]==';'||buffer[x]=='\n')&&in_paren==0&&(x<is_wu || x>last_done)&&(x<is_if||x>last_fi))
	     last_seq=x; 
	 }
       for(x=last_seq;x<buf_size;x++)
	 {
	   if(buffer[x]=='<' && last_arrow!=-2
	      &&in_paren==0&&(x<is_wu || x>last_done)&& (x<is_if||x>last_fi))
	     error(1,0,"Error: only one input allowed\n");
	   if(buffer[x]=='<' &&in_paren==0
	      &&(x<is_wu || x>last_done)&&(x<is_if||x>last_fi))
	     last_arrow=x;
	 }
       if(last_arrow<last_seq)
	 return -1;
    }

  if(cmd_type==SUBSHELL_COMMAND)
    {
      int last_p=-1;
      for(x=0;x<buf_size;x++)
	{
	  if(buffer[x]==')')
	    last_p=x; 
	  if(buffer[x]=='<' && last_p!=-1 && last_arrow!=-2)
	    error(1,0,"Error: only one input allowed\n");
	  if(buffer[x]=='<' && last_p!=-1)
	    last_arrow=x;
	}
      if(last_arrow<last_p)
	return -1;
    }

  if(cmd_type==SIMPLE_COMMAND)
    {
      for(x=0;x<buf_size;x++)
	{
	  if(buffer[x]=='<' && last_arrow!=-2)
	    error(1,0,"Error: only one input allowed\n");
	  if(buffer[x]=='<')
	    last_arrow=x;
	}
      if(last_arrow==-2)
	return -1;
    }
  for(x=last_arrow+1;x<buf_size;x++)
    {
      if(in_size==0 &&strchr(" \t\n",buffer[x])!=NULL)
	continue;
      if(in_size==0 &&strchr(";|()",buffer[x])!=NULL)
	error(1,0,"invalid IO file");
      else if(strchr(";|)",buffer[x])!=NULL)       
	return -1;
      else if(strchr(">\n",buffer[x])!=NULL)
	{ 
	  ((cmd_t)->input)[in_size]='\0';	 
	  return last_arrow;
	}
      else if(strchr(" \t",buffer[x])!=NULL && in_size!=0)
	{
	  ((cmd_t)->input)[in_size]='\0';
	  x++;
	  while(x!=buf_size)
	    {
	      if(buffer[x]=='>')
		return last_arrow;
	      if(isalnum(buffer[x])!=0)
		return -1;
	      x++;
	    }
	  return last_arrow;
	}	     
      else
	{
	  ((cmd_t)->input)[in_size]=buffer[x];
	  in_size++;
	}
    }
  if(in_size==0)
    error(1,0,"unspecified IO\n");
  if(last_arrow==-2)
    return -1;
  else
    return last_arrow;
}


int get_output(char* buffer,command_t cmd_t,enum command_type cmd_type,int buf_size)
{
  int x;
  int last_arrow=-2;
  int in_size=0;
  ((cmd_t)->output)=checked_malloc(sizeof(char)*buf_size+1);
  if(cmd_type==IF_COMMAND)
    {
      int last_fi=-1;
      int n_ifs=0;
      for(x=0;x<buf_size;x++)
	{
	  if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='f'
	     && buffer[x-2]=='i' && (x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))
	    n_ifs++;
	  else
	    if(x==buf_size-1 &&buffer[x]=='f'
	       && buffer[x-1]=='i'&& strchr(" \t\n><;",buffer[x-2])!=NULL)
	      n_ifs++;	  
	  if(x>=3 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='i' && buffer[x-2]=='f' && strchr(" \t\n><;",buffer[x-3])!=NULL)
	    {
	      n_ifs--;
	      if(n_ifs==0)
		last_fi=x-1;
	    }
	  else
	    if (x==buf_size-1 && buffer[x]=='i'&& buffer[x-1]=='f' && strchr(" \t\n><;",buffer[x-2])!=NULL)
	      {
		n_ifs--;
		if(n_ifs==0)
		  last_fi=x;
	      }
	  if(buffer[x]=='>' && last_fi!=-1 && last_arrow!=-2)
	    error(1,0,"Error: only one output allowed\n");
	  if(buffer[x]=='>' &&last_fi!=-1)
	    last_arrow=x;
	}
      if(last_arrow==-2)
	{
	  for(x=last_fi+1;x<buf_size;x++)
	    {
	      if(buffer[x]=='<')
		break;
	      if(buffer[x]==EOF)
		break;
	      if(strchr("\t ;\n\0",buffer[x])==NULL)
		error (1,0,"invalid text following if statement\n");

	    }
	}
      if(last_arrow<last_fi)
	return -1;
    }
  
  if(cmd_type==WHILE_COMMAND || cmd_type==UNTIL_COMMAND)
    {
      int st=0;
      int last_done=-1;
      for(x=0;x<buf_size;x++)
	{
	   if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	     buffer[x-4]=='h' && buffer[x-5]=='w'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))	 
	     st++;;
	   if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	      && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	      buffer[x-4]=='n' && buffer[x-5]=='u'
	      && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	     st++;
	    if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e' && buffer[x-2]=='n'&& buffer[x-3]=='o'&& buffer[x-4]=='d' && strchr(" \t\n><;",buffer[x-5])!=NULL)
	      {
	       st--;
	       if(st==0)
		 last_done=x-1;
	     }
	    else
	      if(x==buf_size-1 && buffer[x]=='e'&& buffer[x-1]=='n' && buffer[x-2]=='o'&& buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
		{
		  st--;
		  if(st==0)
		    last_done=x;
		}
	   if(buffer[x]=='>' && last_done!=-1 && last_arrow!=-2)
	    error(1,0,"Error: only one output allowed\n");
	   if(buffer[x]=='>' && last_done!=-1)
	     last_arrow=x;	  
	}
      if(last_arrow==-2)
	{
	  for(x=last_done+1;x<buf_size;x++)
	    {
	      if(buffer[x]=='<')
		break;
	      if(buffer[x]==EOF)
		break;
	      if(strchr("\t ;\n\0",buffer[x])==NULL)
		error (1,0,"invalid text following while statement\n");
	    }
	}
      if(last_arrow<last_done)
	return -1;
    }

  if(cmd_type==PIPE_COMMAND)
    {
 int is_wu=buf_size+1;
      for(x=0;x<buf_size;x++)
	{
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	     buffer[x-4]=='h' && buffer[x-5]=='w'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	     && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	     buffer[x-4]=='n' && buffer[x-5]=='u'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	}
       int last_done=-1;    
       if(is_wu!=buf_size+1)
	 {
	   for(x=0;x<buf_size;x++)
	     {
	       if(x>=4 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='n' && buffer[x-3]=='o'&&
		  buffer[x-4]=='d'&& strchr(" \t\n><;",buffer[x-5])!=NULL)
		 last_done=x-4;
	       if(x==buf_size-1 && buffer[x]=='e'
		  && buffer[x-1]=='n' && buffer[x-2]=='o'&&
		  buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
		 last_done=x-3;
	     }	     
	 }
       int is_if=buf_size+1;
       int last_fi=-1;
       for(x=0;x<buf_size;x++)
	 {
	   if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='f'
	      && buffer[x-2]=='i'
	      && (x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))
	     is_if=x-2;
	 }
	if(is_if!=buf_size+1)
	  {
	    for(x=0;x<buf_size;x++)
	      {
		if(x>=3 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='i' && buffer[x-2]=='f' && strchr(" \t\n><;",buffer[x-3])!=NULL)
		  last_fi=x-1;
		if(x==buf_size-1 && buffer[x]=='i'&& buffer[x-1]=='f' && strchr(" \t\n><;",buffer[x-2])!=NULL)
		  last_fi=x;
	      }
	  }
      
      int last_pipe=-1;
      int in_paren=0;
      for(x=0;x<buf_size;x++)
	{
	  if(buffer[x]=='(')
	    in_paren++;
	  if(buffer[x]==')')
	    in_paren--;
	  if(buffer[x]=='|'&&in_paren==0&&(x<is_wu && x>last_done)&&(x<is_if&&x>last_fi))
	    last_pipe=x; 	 
	}
      for(x=last_pipe;x<buf_size;x++)
	{
	   if(buffer[x]=='>' &&in_paren==0&& last_arrow!=-2&&(x<is_wu && x>last_done)&&(x<is_if&&x>last_fi))
	    error(1,0,"Error: only one output allowed\n");
	  if(buffer[x]=='>' &&in_paren==0&&(x<is_wu && x>last_done)&&(x<is_if&&x>last_fi))
	    last_arrow=x;
	}
      if(last_arrow<last_pipe)
	return -1;    }

  if(cmd_type==SEQUENCE_COMMAND)
    {
      int is_wu=buf_size+1;
      for(x=0;x<buf_size;x++)
	{
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='l' && buffer[x-3]=='i'&&
	     buffer[x-4]=='h' && buffer[x-5]=='w'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	  if(x>=5 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='l'
	     && buffer[x-2]=='i' && buffer[x-3]=='t'&&
	     buffer[x-4]=='n' && buffer[x-5]=='u'
	     && (x==5 || strchr(" \t\n><;",buffer[x-6])!=NULL))
	    is_wu=x-5;
	}
       int last_done=-1;    
       if(is_wu!=buf_size+1)
	 {
	   for(x=0;x<buf_size;x++)
	     {
	       if(x>=4 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='e'
	     && buffer[x-2]=='n' && buffer[x-3]=='o'&&
		  buffer[x-4]=='d'&& strchr(" \t\n><;",buffer[x-5])!=NULL)
		 last_done=x-4;
	       if(x==buf_size-1 && buffer[x]=='e'
		  && buffer[x-1]=='n' && buffer[x-2]=='o'&&
		  buffer[x-3]=='d'&& strchr(" \t\n><;",buffer[x-4])!=NULL)
		 last_done=x-3;
	     }	     
	 }
       int is_if=buf_size+1;
       int last_fi=-1;
       for(x=0;x<buf_size;x++)
	 {
	   if(x>=2 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='f'
	      && buffer[x-2]=='i'
	      && (x==2 || strchr(" \t\n><;",buffer[x-3])!=NULL))
	     is_if=x-2;
	 }
       if(is_if!=buf_size+1)
	 {
	   for(x=0;x<buf_size;x++)
	     {
	       if(x>=3 && strchr(" \t\n><;",buffer[x])!=NULL && buffer[x-1]=='i' && buffer[x-2]=='f' && strchr(" \t\n><;",buffer[x-3])!=NULL)
		 last_fi=x-1;
	       if(x==buf_size-1 && buffer[x]=='i'&& buffer[x-1]=='f' && strchr(" \t\n><;",buffer[x-2])!=NULL)
		 last_fi=x;
	     }
	 }
       int in_paren=0;
       int last_seq=-1;
       for(x=0;x<buf_size;x++)
	 {
	   if(buffer[x]=='(')
	     in_paren++;
	   if(buffer[x]==')')
	     in_paren--;
	   if((buffer[x]==';')&&in_paren==0&&(x<is_wu && x>last_done)&&(x<is_if&&x>last_fi))
	     last_seq=x;
	 }
       for(x=last_seq;x<buf_size;x++)
	 {
	   if(buffer[x]=='>' && last_arrow!=-2
	      &&in_paren==0&&(x<is_wu || x>last_done)&& (x<is_if&&x>last_fi))
	     error(1,0,"Error: only one output allowed\n");
	   if(buffer[x]=='>'&&in_paren==0
	      &&(x<is_wu || x>last_done)&&(x<is_if&&x>last_fi))
	     last_arrow=x;
	 }
       if(last_arrow<last_seq)
	 return -1;
    }

  if(cmd_type==SUBSHELL_COMMAND)
    {
      int last_p=-1;
      for(x=0;x<buf_size;x++)
	{
	  if(buffer[x]==')')
	    last_p=x; 
	  if(buffer[x]=='>' && last_p!=-1 && last_arrow!=-2)
	    error(1,0,"Error: only one output allowed\n");
	  if(buffer[x]=='>' && last_p!=-1)
	    last_arrow=x;
	}
      if(last_arrow<last_p)
	return -1;
    }

  if(cmd_type==SIMPLE_COMMAND)
    {
      for(x=0;x<buf_size;x++)
	{
	  if(buffer[x]=='>' && last_arrow!=-2)
	    error(1,0,"Error: only one output allowed\n");
	  if(buffer[x]=='>')
	    last_arrow=x;
	}
      if(last_arrow==-2)
	return -1;
    }
  for(x=last_arrow+1;x<buf_size;x++)
    {
      if(in_size==0 &&strchr(" \t\n",buffer[x])!=NULL)
	continue;
      if(in_size==0 &&strchr(";|()",buffer[x])!=NULL)
	error(1,0,"invalid IO file");
      else if(strchr(";|)",buffer[x])!=NULL)       
	return -1;
      else if(strchr("<\n",buffer[x])!=NULL)
	{ 
	  ((cmd_t)->output)[in_size]='\0';
	  return last_arrow;
	}
      else if(strchr(" \t",buffer[x])!=NULL && in_size!=0)
	{
	  ((cmd_t)->output)[in_size]='\0';
	  x++;
	  while(x!=buf_size)
	    {
	      if(buffer[x]=='<')
		return last_arrow;
	      if(isalnum(buffer[x])!=0)
		return -1;
	      x++;
	    }	 
	  return last_arrow;
	}	     
      else
	{	
	  ((cmd_t)->output)[in_size]=buffer[x];
	  in_size++;
	}
    }
  if(in_size==0)
    error(1,0,"unspecified IO\n");
  if(last_arrow==-2)
    return -1;
  else
    return last_arrow;
}

command_t
read_command_stream (command_stream_t s)
{
  if(s->head==NULL)
    return NULL;
  command_t cmd_t=checked_malloc(sizeof(struct command));  
  cmd_t=s->head->cmd;
  s->head=s->head->next;
  return cmd_t;
}
