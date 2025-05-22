// The MIT License (MIT)
//
// Copyright (c) 2023 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define WHITESPACE " \t\n" // We want to split our command line up into tokens
                           // so we need to define what delimits our tokens.
                           // In this case  white space
                           // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 128 // The maximum command-line size

#define MAX_NUM_ARGUMENTS 10 // Mav shell currently only supports 10 argument

#define MAX_HISTORY 50 //history could be of 50 commands only

char *history[MAX_HISTORY];
int historySize = 0;  //tracks the number of commands in history

//fn to add a new command to history
void addToHistory(char *command_string)
{
    //skip empty commands
    if (strlen(command_string) == 0)
        return;

    //remove the first command if history gets full
    if (historySize == MAX_HISTORY)
    {
        free(history[0]);
        for (int i = 0; i < MAX_HISTORY - 1; i++)
        {
            history[i] = history[i + 1];  
        }
        historySize--;  //decrease history size after removal
    }

    //store the new command in history
    history[historySize] = strdup(command_string);
    historySize++;  // 
}

//handles executing two commands connected by a pipe
void runPipeCommands(char **firstCmd, char **secondCmd) {
  int pipeFd[2];
  pid_t child1, child2;
  
  //create pipe
  if (pipe(pipeFd) == -1) {
      perror("pipe error");
      return;
  }
  
  
  child1 = fork();
  if (child1 < 0) {
      perror("first fork failed");
      return;
  }
  
  // in the first child process - will write to pipe
  if (child1 == 0) {
      close(pipeFd[0]);  
      //connect child's output to write end of pipe
      if (dup2(pipeFd[1], 1) < 0) {
          perror("dup2 failed");
          exit(1);
      }
      close(pipeFd[1]); 
      
      //run the first command
      execvp(firstCmd[0], firstCmd);
      perror("first command failed");
      exit(1);
  }
  
  
  child2 = fork();
  if (child2 < 0) {
      perror("second fork failed");
      return;
  }
  
  //in the second child process - will read from pipe
  if (child2 == 0) {
      close(pipeFd[1]);
      // connect pipe's read end to child's input
      if (dup2(pipeFd[0], 0) < 0) {
          perror("dup2 failed");
          exit(1);
      }
      close(pipeFd[0]);  
      
      
      execvp(secondCmd[0], secondCmd);
      perror("second command failed");
      exit(1);
  }
  
  //close both ends and wait for children
  close(pipeFd[0]);
  close(pipeFd[1]);
  
  wait(NULL);
  wait(NULL);
}

//fn to print the history
void printHistory()
{
    for (int i = 1; i <= historySize; i++)
    {
        printf("[%d] %s\n", i, history[i - 1]);
    }
}

//fn to block ctrl c and ctrl z signals
void blockSignals() {

    sigset_t newmask;//declare a new segset mask .
  
    sigemptyset(&newmask);//empty out the sigset.
  
    sigaddset(&newmask, SIGINT);   //block Ctrl+C
    sigaddset(&newmask, SIGTSTP);  //block Ctrl+Z

    if (sigprocmask(SIG_BLOCK, &newmask, NULL) < 0) {
        perror("sigprocmask"); 
    }//add the new signal set to our process' control block signal mask element
  
}

int main()
{
blockSignals();//calling block signal function
  char *command_string = (char *)malloc(MAX_COMMAND_SIZE);

  while (1)
  {
    // Print out the msh prompt
    printf("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while (!fgets(command_string, MAX_COMMAND_SIZE, stdin))
      ;
    addToHistory(command_string);
    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for (int i = 0; i < MAX_NUM_ARGUMENTS; i++)
    {
      token[i] = NULL;
    }

    int token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;

    char *working_string = strdup(command_string);

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while (((argument_ptr = strsep(&working_string, WHITESPACE)) != NULL) &&
           (token_count < MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
      if (strlen(token[token_count]) == 0)
      {
        token[token_count] = NULL;
      }
      token_count++;
    }
    // if-else statements for handling the inputted cmds
    if (token[0] == NULL)
    {

    } // if user enters nothing do nothing

    //to run !# commands from history
    else if (token[0][0] == '!' && token[0][1] != '\0') {
      int idx = atoi(&token[0][1]) - 1; //convert string to index
      if (idx >= 0 && idx < historySize) { //ensure idx >=0 and less than the history size
          pid_t pid = fork(); 
          if (pid == 0) { 
              execlp("/bin/sh", "sh", "-c", history[idx], NULL);
              perror("exec failed");
              exit(EXIT_FAILURE); //exit child process if exec fails
          } else if (pid < 0) {
              perror("fork failed");
          } else {
              wait(NULL); //parent waits for child to finish
          }
      }
  }
  
    else if (strcmp(token[0], "cd") == 0)
    { // if user enters cd change the directory respectively
      if (token[1] == NULL)
      {
        chdir(getenv("HOME")); // changes to home directory
      }
      else if (strcmp(token[1], "..") == 0)
      {
        chdir(".."); // move back one directory
      }
      else
      {
        if (chdir(token[1]) == -1)
        {
          perror("cd failed"); // cd failed
        }
      }
    }
    else if (strcmp(token[0], "history") == 0)
    { // if users enters history call the history function
      printHistory();
      continue;
    }
    else if (strcmp(token[0], "quit") == 0 || strcmp(token[0], "exit") == 0)
    { // exit if user enters quit or exit
      exit(0);
    }
    else
    { int pipeIndex = -1;
      char **cmd1, **cmd2;
      
      //looking for pupe symbol
      for (int i = 0; i < token_count; i++) {
        if (token[i] != NULL && strcmp(token[i], "|") == 0) {
              pipeIndex = i;
              break;
          }
      }
       //if we found a pipe, split and handle the commands
       if (pipeIndex > 0) {
        //make space for the first command's tokens
        cmd1 = (char**)malloc(MAX_NUM_ARGUMENTS * sizeof(char*));
        int i;
        //copy everything before the pipe
        for (i = 0; i < pipeIndex; i++) {
            cmd1[i] = strdup(token[i]);
        }
        cmd1[i] = NULL;  
        
        //mke space for the second command's tokens
        cmd2 = (char**)malloc(MAX_NUM_ARGUMENTS * sizeof(char*));
        int j = 0;
        // Copy everything after the pipe
        for (i = pipeIndex + 1; token[i] != NULL; i++) {
            cmd2[j] = strdup(token[i]);
            j++;
        }
        cmd2[j] = NULL;  //null terminate the array
        
        
        runPipeCommands(cmd1, cmd2);
        
        //Clean up all our allocated memory
        for (i = 0; cmd1[i] != NULL; i++) free(cmd1[i]);
        for (i = 0; cmd2[i] != NULL; i++) free(cmd2[i]);
        free(cmd1);
        free(cmd2);
        
      }
      else{
        pid_t pid = fork(); //fork to execute external commands
        if (pid == 0)
        {
            int redirectIndex = -1;
            for (int i = 0; i < token_count; i++) //handle input output redirection
            {
                if (token[i] && strcmp(token[i], ">") == 0)
                {
                    int fd = open(token[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                    if (fd < 0)
                    {
                        perror("Can't open output file");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);// redirect stdout to file
                    close(fd);
                    redirectIndex = i;
                    break;
                }
                else if (token[i] && strcmp(token[i], "<") == 0)
                {
                    int fd = open(token[i + 1], O_RDONLY);
                    if (fd < 0)
                    {
                        perror("Can't open input file");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDIN_FILENO);// redirect stdin from file
                    close(fd);
                    redirectIndex = i;
                    break;
                }
            }
    
            // remove redirection tokens from the argument list
            if (redirectIndex != -1)
            {
                token[redirectIndex] = NULL;
            }
      
            execvp(token[0], token); //execute the command
            perror("execvp failed");  
            exit(EXIT_FAILURE);
        }
        else if (pid == -1)
        {
            perror("fork failed");
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }
    }
  }
    
    // Cleanup allocated memory
    for (int i = 0; i < MAX_NUM_ARGUMENTS; i++)
    {
      if (token[i] != NULL)
      {
        free(token[i]);
      }
    }

    free(head_ptr);
  }

  free(command_string);

  return 0;
  // e1234ca2-76f3-90d6-0703ac120004
}
