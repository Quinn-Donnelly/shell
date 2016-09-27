
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <unistd.h>
 #include <stdlib.h>  
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <string.h>
 #include <stdio.h>


#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"


char *read_line(void)
{
  char *line = NULL;
  ssize_t bufsize = 0;
  getline(&line, &bufsize, stdin);
  return line;
}


//   split_line will seprate the arguments 
//   and return them to the caller

char **split_line(char *line)
{
  int bufsize = TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  // Check to ensure the array was made
  if (!tokens) {
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }
  
  // Splits the string up using the tokens defined in the macro
  token = strtok(line, TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    // If more space is needed for the tokens array allocate more
    if (position >= bufsize) {
      bufsize += TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));

      // Handle the allocation error, if occurs
      if (!tokens) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}



//  Function Declarations for supported commands:
int cd(char **args);
int help(char **args);
int shell_exit(char **args);
int ls(char **args);

// List of commands
char *command_str[] = {
  "cd",
  "help",
  "exit",
  "ls"
};

// List of the functions that implement the commands
int (*command_func[]) (char **) = {
  &cd,
  &help,
  &shell_exit,
  &ls
};

// Return the number of commands supported
int num_commands() {
  return sizeof(command_str) / sizeof(char *);
}


// Lists the files in the curent directory
int ls(char **args)
{
  int out;

  if(args[1] != NULL && !strcmp(args[1], ">"))
  {
    if(args[2] == NULL)
    {
      fprintf(stderr, "shell: expected argument after ls > \n");
    }
    else
    {
      out = open(args[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
      dup2(out, 1);
      close(out);
    }
  }
  execv("bin/ls", args);
}

// Changes the current directory
int cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "shell: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("shell");
    }
  }
  return 1;
}


// Prints the shell information including supported commands
int help(char **args)
{
  int i;
  printf("Group 0's shell\n");
  printf("The following are built in:\n");

  for (i = 0; i < num_commands(); i++) {
    printf("  %s\n", command_str[i]);
  }

  return 1;
}


int shell_exit(char **args)
{
  return 0;
}


// launch handles the forking for the child processess
int launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("shell");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("shell");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}


// This loop controls the user side of the shell
// (Reading input and calling execute on it)
void loop(void)
{
  char *line;
  char **args;
  int status;

  int saved_in = dup2(0);
  int saved_out = dup2(1);

  do {
    printf("> ");
    line = read_line();
    args = split_line(line);
    status = execute(args);

    // Restore standard in and out in case they were changed
    dup2(saved_in, 0);
    close(saved_in);
    dup2(saved_out, 1);
    close(saved_out);

    free(line);
    free(args);
  } while (status);
}


// Will take the commands and run the coresponding 
// command from the function list declared
int execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < num_commands(); i++) {
    if (strcmp(args[0], command_str[i]) == 0) {
      return (*command_func[i])(args);
    }
  }

  // This will launch any commands not found in the 
  // shell and see if they execute in the terminal
  return launch(args);
}


int main(int argc, char **argv)
{
  loop();


  return EXIT_SUCCESS;
}





