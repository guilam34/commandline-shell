// UCLA CS 111 Lab 1 command interface

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
#include <unistd.h>

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;
typedef struct command_node *command_n;
/* Create a command stream from GETBYTE and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */

enum command_type check_for_command(char* buffer,int buf_size);
void add_to_buf(char cur_char, char* buffer,int* buf_size);
int check_for_whileuntil(char* buffer, int buf_size);
int check_for_if(char * buffer, int buf_size);
void add_command(char* buffer, enum command_type cmd_type,command_stream_t cmd_stream, int buf_s);
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);
void get_inner_commands(char* buffer,enum command_type cmd_type,command_t* sub_cmds,int buf_size);
command_t inner_command(enum command_type cmd_type,char*buffer,int buf_size);

int get_input(char* buffer,command_t cmd_t,enum command_type cmd_type,int buf_size);
int get_output(char* buffer,command_t cmd_t,enum command_type cmd_type,int buf_size);
/* Prepare for profiling to the file FILENAME.  If FILENAME is null or
   cannot be written to, set errno and return -1.  Otherwise, return a
   nonnegative integer flag useful as an argument to
   execute_command.  */
int prepare_profiling (char const *filename);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t,int profiling);

/* Execute a command.  Use profiling according to the flag; do not profile
   if the flag is negative.  */
void execute_command (command_t, int);

/* Return the exit status of a command, which must have previously
   been executed.  Wait for the command, if it is not already finished.  */
int command_status (command_t);

void profileLine(int profiling,long int time_arr[][2],command_t c,pid_t pid);
