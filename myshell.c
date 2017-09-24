#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
       
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#define MAXLINE 200  /* This is how we declare constants in C */
#define MAXARGS 20
#define OPERATORSNUM 4

pid_t childPID; // will be used by the signal handler\
                // will be changed by myshell process in "execute()" 

/* In C, "static" means not visible outside of file.  This is different
 * from the usage of "static" in Java.
 * Note that end_ptr is an output parameter.
 */
static char * getword(char * begin, char **end_ptr) {
    char * end = begin;

    while ( *begin == ' ' )
        begin++;  /* Get rid of leading spaces. */
    end = begin;
    while ( *end != '\0' && *end != '\n' && *end != ' ' && *end != '#')
        end++;  /* Keep going. */
    if ( end == begin )
        return NULL;  /* if no more words, return NULL */
    
    if (*end == '#')
        return NULL; /* beginning of a comment, hence no more words */

    *end = '\0';  /* else put string terminator at end of this word. */
    *end_ptr = end;
    if (begin[0] == '$') { /* if this is a variable to be expanded */
        begin = getenv(begin+1); /* begin+1, to skip past '$' */
	if (begin == NULL) {
	    perror("getenv");
	    begin = "UNDEFINED";
        }
    }
    return begin; /* This word is now a null-terminated string.  return it. */
}

/* In C, "int" is used instead of "bool", and "0" means false, any
 * non-zero number (traditionally "1") means true.
 */
/* argc is _count_ of args (*argcp == argc); argv is array of arg _values_*/
static void getargs(char cmd[], int *argcp, char *argv[])
{
    char *cmdp = cmd;
    char *end;
    int i = 0;

    /* fgets creates null-terminated string. stdin is pre-defined C constant
     *   for standard intput.  feof(stdin) tests for file:end-of-file.
     */
    if (fgets(cmd, MAXLINE, stdin) == NULL && feof(stdin)) {
        printf("Couldn't read from standard input. End of file? Exiting ...\n");
        exit(1);  /* any non-zero value for exit means failure. */
    }
    while ( (cmdp = getword(cmdp, &end)) != NULL ) { /* end is output param */
        /* getword converts word into null-terminated string */
        argv[i++] = cmdp;
        /* "end" brings us only to the '\0' at end of string */
	cmdp = end + 1;
    }
    argv[i] = NULL; /* Create additional null word at end for safety. */
    *argcp = i;
}

char* which_operator(int argc, char *argv[], int *index)
{
  int i;
  for (i=0; i<argc; ++i)
  {
    if (strcmp(argv[i], ">") == 0)
    {
      *index = i;
      return ">";
    }    

    else if (strcmp(argv[i], "<") == 0)
    {
      *index = i;
      return "<";
    }

    else if (strcmp(argv[i], "|") == 0)
    {
      *index = i;
      return "|";
    }
   
    else if (strcmp(argv[i], "&") == 0)
    {
      *index = i;
      return "&";
    }
  }
  
  return "none";
}

void execute_pipe(int argc, char *argv[], int index)
{
  int pipefd[2];
  if (pipe(pipefd) == -1)
  {
    perror("pipe()");
    exit(EXIT_FAILURE);
  }     

  pid_t child1pid, child2pid;
  child1pid = fork();
  
  if (child1pid == -1)
  {
    perror("fork()");
    exit(EXIT_FAILURE);
  }

  else if (child1pid == 0) // first child, first command
  {
    // first command outputs for the second command
    // hence it should only own the writing part of the pipe
    // close the reading part of the pipe it inherited
    if (close(pipefd[0]) == -1)
    {
      perror("close()");
      exit(EXIT_FAILURE);
    }
    
    // make pipe[1] the stdout
    if (dup2(pipefd[1], STDOUT_FILENO) == -1)
    {
      perror("dup2()");
      exit(EXIT_FAILURE);
    }
     
    // 
    if (close(pipefd[1]) == -1)
    {
      perror("close()");
      exit(EXIT_FAILURE);
    }

    // 
    char *child1_argv[index+1];
    int i;
    for (i=0; i<index; ++i)
    {
      child1_argv[i] = strdup(argv[i]);
    }
    child1_argv[i] = NULL;
    
    // 
    if (execvp(child1_argv[0], child1_argv) == -1)
    {
      perror("execvp()");
      exit(EXIT_FAILURE);
    }

  }

  else // the parent will create the second child
  {
    // create the second child
    child2pid = fork();
    if (child2pid == -1)
    {
      perror("fork()");
      exit(EXIT_FAILURE);
    }

    else if(child2pid == 0)
    {
      // the second child receives input from the first child.
      // hence it should only own the reading part of the pipe
      // and close the writing part of the pipe it inherited.
      if (close(pipefd[1]) == -1)
      {
        perror("close()");
        exit(EXIT_FAILURE);
      }
      
      // make pipe[0] the stdin
      if (dup2(pipefd[0], STDIN_FILENO) == -1)
      {
        perror("dup2()");
        exit(EXIT_FAILURE);
      }

      if (close(pipefd[0]) == -1)
      {
        perror("close()");
        exit(EXIT_FAILURE);
      }

      // 
      char *child2_argv[argc-index];
      int i;
      for (i=0; i<(argc-index-1); ++i)
      {
        child2_argv[i] = strdup(argv[index+1+i]);
      }
      child2_argv[i] = NULL;

      //
      if (execvp(child2_argv[0], child2_argv) == -1)
      {
        perror("execvp()");
        exit(EXIT_FAILURE);
      }

    }

    else // the parent waits for both child processes to terminate
    {
      // close pipe
      if (close(pipefd[0]) == -1)
      {
        perror("close()");
        exit(EXIT_FAILURE);
      }

      if (close(pipefd[1]) == -1)
      {
        perror("close()");
        exit(EXIT_FAILURE);
      }

      waitpid(child1pid, NULL, 0);
      waitpid(child2pid, NULL, 0);
    }
  }  
  return;
}

void execute_backgroundJob(int argc, char *argv[], int index)
{

  return;
}


static void execute(int argc, char *argv[])
{
  int index;
  char *operator;
  operator = which_operator(argc, argv, &index);   
 
  if (strcmp(operator, "|") == 0)
    execute_pipe(argc, argv, index);
  
  else 
  {
    pid_t childpid; /* child process ID */

    childpid = fork();
    if (childpid == -1) { /* in parent (returned error) */
        perror("fork"); /* perror => print error string of last system call */
        printf("  (failed to execute command)\n");
    }
    if (childpid == 0) { /* child:  in child, childpid was set to 0 */
        /* Executes command in argv[0];  It searches for that file in
	 *  the directories specified by the environment variable PATH.
         */
         if (strcmp(operator, "<") == 0)
         {
           // open the file and "dup2()" it on stdin. 
           int inputFilefd = open(argv[index+1], O_RDONLY);
           if (dup2(inputFilefd, STDIN_FILENO) == -1)
           {
             perror("dup2()");
             exit(EXIT_FAILURE);
           }

           if (close(inputFilefd) == -1)
           {
             perror("close()");
             exit(EXIT_FAILURE);
           }
           argv[index] = NULL;
        }

        else if (strcmp(operator, ">") == 0)
        {
          // open the file and "dup2()" it on stdout
          int outputFilefd = open(argv[index+1], O_CREAT|O_WRONLY, 0777);
          if (dup2(outputFilefd, STDOUT_FILENO) == -1)
          {
            perror("dup2()");
            exit(EXIT_FAILURE);
          }
           
          if (close(outputFilefd) == -1)
          {
            perror("close()");
            exit(EXIT_FAILURE);
          }
          argv[index] = NULL;
        }
    
        else if (strcmp(operator, "&") == 0)
          argv[index] = NULL;
               

        if (-1 == execvp(argv[0], argv)) {
          perror("execvp");
          printf("  (couldn't find command)\n");
        }
	/* NOT REACHED unless error occurred */
        exit(1);
    }
 
    else /* parent:  in parent, childpid was set to pid of child process */
    {
      if (strcmp(operator, "&") != 0) // if it's not a command to be run in the background
        waitpid(childpid, NULL, 0);  /* then wait until child process finishes */
    }
  }

  return;
}

void interrupt_handler(int signum)
{
  printf(" was recognized\n"); 
  return;
}

int main(int argc, char *argv[])
{
    // Install a signal handling function for SIGINT
    if (signal(SIGINT, interrupt_handler) == SIG_ERR)
    {
      perror("signal()");
      exit(EXIT_FAILURE);
    }

    if (argc > 1)
    {
      if (chmod(argv[1], 00555) == -1)
      {
        perror("chmod()");
        exit(EXIT_FAILURE);
      }

      if (freopen(argv[1], "r", stdin) == NULL)
      {
        perror("freopen()");
        exit(EXIT_FAILURE);
      } 
    }    


    char cmd[MAXLINE];
    char *childargv[MAXARGS];
    int childargc;
    while (1) {
        printf("%% "); /* printf uses %d, %s, %x, etc.  See 'man 3 printf' */
        fflush(stdout); /* flush from output buffer to terminal itself */
	getargs(cmd, &childargc, childargv); /* childargc and childargv are
            output args; on input they have garbage, but getargs sets them. */
        /* Check first for built-in commands. */
	if ( childargc > 0 && strcmp(childargv[0], "exit") == 0 )
            exit(0);
	else if ( childargc > 0 && strcmp(childargv[0], "logout") == 0 )
            exit(0);
        else
	    execute(childargc, childargv);
    }
    /* NOT REACHED */
}
