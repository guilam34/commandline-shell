// UCLA CS 111 Lab 1 command printing, for debugging

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void
command_indented_print (int indent, command_t c,int profiling)
{
  if(profiling!=-1)
    dup2(profiling,1);
  switch (c->type)
    {
    case IF_COMMAND:
    case UNTIL_COMMAND:
    case WHILE_COMMAND:
      if(profiling!=-1)
	{
	  printf ("%s ",
		  (c->type == IF_COMMAND ? "if"
		   : c->type == UNTIL_COMMAND ? "until" : "while"));
	  command_indented_print (indent + 2, c->u.command[0],profiling);
	  printf ("%s ",  c->type == IF_COMMAND ? "then" : "do");
	  command_indented_print (indent + 2, c->u.command[1],profiling);
	  if (c->type == IF_COMMAND && c->u.command[2])
	    {
	      printf ("else ");
	      command_indented_print (indent + 2, c->u.command[2],profiling);
	    }
	  printf ("%s ",c->type == IF_COMMAND ? "fi" : "done");
	  break;
	}
      else{
	printf ("%*s%s\n", indent, "",
 		(c->type == IF_COMMAND ? "if"
		 : c->type == UNTIL_COMMAND ? "until" : "while"));
	command_indented_print (indent + 2, c->u.command[0],profiling);
	printf ("\n%*s%s\n", indent, "", c->type == IF_COMMAND ? "then" : "do");
	command_indented_print (indent + 2, c->u.command[1],profiling);
	if (c->type == IF_COMMAND && c->u.command[2])
	  {
	    printf ("\n%*selse\n", indent, "");
	    command_indented_print (indent + 2, c->u.command[2],profiling);
	  }
	printf ("\n%*s%s", indent, "", c->type == IF_COMMAND ? "fi" : "done");
	break;
      }

    case SEQUENCE_COMMAND:
    case PIPE_COMMAND:
      {
	if(profiling!=-1)
	  {
	    command_indented_print (indent + 2 * (c->u.command[0]->type != c->type),
				    c->u.command[0],profiling);
	    char separator = c->type == SEQUENCE_COMMAND ? ';' : '|';	  
	    printf ("%c ", separator);
	    command_indented_print (indent + 2 * (c->u.command[1]->type != c->type),
				    c->u.command[1],profiling);
	    break;
	  }
	else{
	  command_indented_print (indent + 2 * (c->u.command[0]->type != c->type),
				  c->u.command[0],profiling);
	  char separator = c->type == SEQUENCE_COMMAND ? ';' : '|';
	  printf (" \\\n%*s%c\n", indent, "", separator);
	  command_indented_print (indent + 2 * (c->u.command[1]->type != c->type),
				  c->u.command[1],profiling);
	  break;
	}
      }

    case SIMPLE_COMMAND:
      {	   
	char **w = c->u.word;
	if(profiling!=-1)
	  {
	    printf ("%s ",*w);
	    while (*++w)
	      {
		printf ("%s ", *w);	   
	      }
	    break;
	  }
	else{
	  printf ("%*s%s", indent, "", *w);
	  while (*++w)
	    printf (" %s", *w);
	  break;
	}
      }

    case SUBSHELL_COMMAND:
      if(profiling!=-1)
	{
	  printf ("( ");
	  command_indented_print (indent + 1, (c->u.command[0]),profiling);
	  printf (") ");
	  break;
	}
      else{
	printf ("%*s(\n", indent, "");
	command_indented_print (indent + 1, (c->u.command[0]),profiling);
	printf ("\n%*s)", indent, "");
	break;
      }

    default:
      abort ();
    }

  if (c->input)
    printf ("<%s", c->input);
  if (c->output)
  printf (">%s", c->output);
}

void
print_command (command_t c,int profiling)
{
  command_indented_print (2, c,profiling);
  putchar('\n');
}
