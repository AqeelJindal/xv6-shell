#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

/* Read a line of characters from stdin. */
int getcmd(char *buf, int nbuf) {
  memset(buf, 0, nbuf); // to clear the buf
  
  printf(">>> "); // Prompt the user
  
  if(read(0, buf, nbuf) < 0)
  {
    fprintf(2, "Error: Unable to read the command");
  }

  return 0;
}

/*
  A recursive function which parses the command
  at *buf and executes it.
*/
__attribute__((noreturn))
void run_command(char *buf, int nbuf, int *pcp) {

  /* Useful data structures and flags. */
  char *arguments[10] = { 0 };
  int numargs = 0;
  /* Word start/end */
  int ws = 1;
  int we = 1;

  int redirection_left = 0;
  int redirection_right = 0;
  char *file_name_l = 0;
  char *file_name_r = 0;

  int p[2];
  int q; // to take the place of stdin/stdout for redirection
  int pipe_cmd = 0;
  int sequence_cmd = 0;
  
  int i = 0;

  /* Parse the command character by character. */
  for (; i < nbuf; i++)
  {

    /* Parse the current character and set-up various flags:
       sequence_cmd, redirection, pipe_cmd and similar. */

 
    if (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\0')
    {
      if((ws && buf[i] == ' ')|| (ws && buf[i] == '\t') || (ws && buf[i] == '\0')) // catching the stress test for the space  and the tab case
      {
        buf[i] = '\0';
      }
      else if (we) // A word has ended
      {
        ws = 1;
        we = 0;
        buf[i] = '\0';
        numargs++;
      }
    }
    else if (buf[i] == '\n')
    {
      buf[i] = '\0';
      break;
    }
    else
    {
       if (buf[i] == ';')
       {
         buf[i] = '\0';
         sequence_cmd = 1;
         break;
       }
       else if (buf[i] == '>')
       {
         buf[i] = '\0';
         ws = 0;  // for the file to be captured
         redirection_right = 1;
       }
       else if (buf[i] == '<')
       {
         buf[i] = '\0';
         ws = 0;
         redirection_left = 1;
       }
       else if (buf[i] == '|')
       {
         buf[i] = '\0';
         pipe_cmd = 1;
         break;
       }

       if (ws) // the address of the start of the word is stored in arguments
      {
        arguments[numargs] = &buf[i];
        ws = 0;
        we = 1;
      }
    }
   
    if (redirection_left != 0|| redirection_right != 0)
    {
      /* Redirection command. Capture the file names. */
      for (int n = i + 1; n < nbuf - i; n++)
      {
        if (buf[n] == 0 || buf[n] == ';')
        {
          buf[n] = '\0';
          break;
        }
        else if (buf[n] == ' '|| buf[n] == '\t' || buf[n] == '\n')
        {
          buf[n] = '\0';
          // to catch the number of times the above characters appear in the filename before the null terminator and after the redirection character
          if (redirection_right != 0)
          {
            redirection_right++; 
          }
          else if (redirection_left != 0)
          {
            redirection_left++;
          }
        }
        
        if (!ws && buf[n] != 0)
        {
          ws = 1;
          file_name_r = &buf[n];
          file_name_l = &buf[n];
        }
      }
      break;
    }
  }
  
  /*
    Sequence command. Continue this command in a new process.
    Wait for it to complete and execute the command following ';'.
  */
  if (sequence_cmd)
  {
    sequence_cmd = 0;
    if (fork() == 0)
    {
      exec(arguments[0], arguments);
    }
    else
    {
      wait(0);
      run_command(&buf[i + 1], nbuf - i, pcp);
    }
  }

  /*
    If this is a redirection command,
    tie the specified files to std in/out.
  */
  
  if (redirection_right != 0)
  {
    if(fork() == 0)
    {
      // child process
      if((q = open(file_name_r, O_WRONLY | O_TRUNC| O_CREATE)) < 0)
      {
        fprintf(2, "Error: cannot open '%s'\n", file_name_r);
      }

      close(1); // closes the current stdout
      dup(q); // duplicates q to be the new stdout
 
      exec(arguments[0], arguments);
    }
    
    else
    {
      // parent process
      wait(0);
      run_command(&buf[i + strlen(file_name_r) + redirection_right], nbuf - i - strlen(file_name_r) - redirection_right + 1, pcp);
    }
  }
  else if (redirection_left != 0)
  {
    if(fork() == 0)
    {
      // child process
      if((q = open(file_name_l, O_RDONLY)) < 0)
      {
        fprintf(2, "Error: cannot open '%s'\n", file_name_l);
      }
    
      close(0); // closes the current stdin
      dup(q); // duplicates q to be the new stdin

      exec(arguments[0], arguments); // executes the command from the stdin
    }
    else
    {
      // parent process
      wait(0);
      run_command(&buf[i + strlen(file_name_l) + redirection_left], nbuf - i - strlen(file_name_l) - redirection_left + 1, pcp);
    }
  }

  /* Parsing done. Execute the command. */

  /*
    If this command is a CD command, write the arguments to the pcp pipe
    and exit with '2' to tell the parent process about this.
  */
  if (strcmp(arguments[0], "cd") == 0)
  {
    write(pcp[1], arguments[1], nbuf);
    exit(2);
  }
  else
  {
    /*
      Pipe command: fork twice. Execute the left hand side directly.
      Call run_command recursion for the right side of the pipe.
    */
    
    if (pipe_cmd)
    {
      pipe(p);
 
      // left hand side of the pipe
      if (fork() == 0)
      {
        // child process
        // close all the pipe ends for no memory leaks
        close(1); 
        dup(p[1]);
        close(p[0]);
        close(p[1]);

        if(exec(arguments[0], arguments) < 0)
        {
          fprintf(2, "Error");
        }
      }
      
      // Right hand side of the pipe
      if (fork() == 0)
      {
        // child process
        
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        
        run_command(&buf[i + 1], nbuf - i, pcp);
      }
      else
      {
        // Parent process
        close(p[1]);
        close(p[0]);
        // wait for the two fork processes to be finished
        wait(0);  
        wait(0);
      }
    }
    else
    {
      if (strcmp(arguments[0], "exit") == 0) // exits the shell
      {
        exit(1);
      }
      else if (redirection_right != 0)
      {
      }
      else if (redirection_left != 0)
      {
      }
      else
      {
        if ((exec(arguments[0], arguments)) && (arguments[0] != 0))
        {
          if (arguments[10] != 0)
          {
            fprintf(2, "Error: too many arguments\n");
          }
          else
          {
            fprintf(2, "Error: Invalid command '%s'\n", arguments[0]);
          }
        }
      }
    }
  }
  exit(0);
}

int main(void) {

  static char buf[100];

  int pcp[2];
  pipe(pcp);

  /* Read and run input commands. */
  while(getcmd(buf, sizeof(buf)) >= 0)
  {
    if(fork() == 0)
    {
      run_command(buf, 100, pcp);
    }
    
    /*
      Check if run_command found this is
      a CD command and run it if required.
    */
    int child_status;
    wait(&child_status);
    if (child_status == 2)
    {
      char directory[100];
      read(pcp[0], directory, 100);
      if (chdir(directory) != 0)
      {
        fprintf(2, "cd: cannot change directory to '%s'\n", directory);
      }
    }
    else if (child_status == 1)
    {
      exit(0);
    }
  }
  exit(0);
}
