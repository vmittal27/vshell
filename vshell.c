#include "str_utils.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdarg.h>
#include <signal.h>

#define RED   "\x1B[31m"
#define CYN   "\x1B[36m"
#define YEL   "\x1B[33m"
#define RESET "\x1B[0m"

typedef enum {
    FILE_DOES_NOT_EXIST, 
    DIR_DOES_NOT_EXIST, 
    FILEPATH_EXISTS, 
} FILEPATH_STATUS;

typedef enum {
    REDIRECT_STANDARD, 
    REDIRECT_PREPEND, 
    REDIRECT_APPEND
} REDIRECT_TYPE; 

typedef enum {
    NO_ACTIVE_CHILD_PROCESS,
    ACTIVE_CHILD_PROCESS
} CHILD_EXEC_STATUS; 

CHILD_EXEC_STATUS child_status = NO_ACTIVE_CHILD_PROCESS; 
pid_t child_pid = 0;  

/*
Handles SIGINT. If a child process has been forked (i.e., a command is being executed), 
SIGINT is routed to the child process ONLY. 
If there is no active child process, SIGINT terminates vsh. 
*/
void SIGINT_handler(int sig) {
    if (child_status == NO_ACTIVE_CHILD_PROCESS) {
        char msg[] = "\nvsh: Exiting...\n"; 
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(0); 
    } 
    else {
        kill(child_pid, SIGINT); 
    }
}

/*Writes an arbitrary number of `char*` arguments to the file stream
referenced by `STDOUT_FILENO`. Each argument must be null-terminated, and
the last argument MUST be `NULL`.*/
void print_msg(char *msg_first, ...)
{
    va_list args; 
    char *msg_next;

    va_start(args, msg_first); 
    write(STDOUT_FILENO, msg_first, strlen(msg_first));

    while ((msg_next = va_arg(args, char *)) != NULL) {
        write(STDOUT_FILENO, msg_next, strlen(msg_next)); 
    }

    va_end(args); 
}

/*Prints an error message to `STDOUT_FILENO`. 
All messages are prepended with `"vsh ERROR: "` and
appended with `"\n"`.*/
void raise_error(char *err)
{  
    print_msg(RED, "vsh ERROR", RESET, ": ", err, "\n", NULL);
}

/*
Validates a file path. If filepath is valid, return FILEPATH_EXISTS.
If a component of the path prefix is not a directory, return DIR_DOES_NOT_EXIST.
If the prefix is valid, but the file does not exist, return FILE_DOES_NOT_EXIST.
*/
FILEPATH_STATUS validate_file(char *filepath) {
    int fd;
    fd = open(filepath, O_RDONLY); 

    if (fd != -1) {
        close(fd); 
        return FILEPATH_EXISTS; 
    }
    else { // in the case the file does not exist, we check why
        fd = open(filepath, O_RDWR | O_CREAT, 0666);
        // this occurs in the case the directory path is invalid
        if (fd == -1) 
            return DIR_DOES_NOT_EXIST; 
        close(fd); 
        remove(filepath); 
        return FILE_DOES_NOT_EXIST; 
    }

}

/*Given a command `input`, determine the type of redirection 
to use. If no redirection is to occur, this function returns `STANDARD`.*/
REDIRECT_TYPE determine_redirect_type(char *input) {
    char **split_input;
    int split_len;

    split_len = 0; 
    split_input = split_str(input, ">+", &split_len); 
    free_str_array(split_input);

    if (split_len > 1) {
        return REDIRECT_PREPEND; 
    }

    split_len = 0; 
    split_input = split_str(input, ">>", &split_len);
    free_str_array(split_input);
 
    if (split_len > 1) {
        return REDIRECT_APPEND;
    }

    return REDIRECT_STANDARD;
}

/*Takes a raw `input` and parses the redirection, returning
the command without the redirection symbols. 
`redirect` will contain the type of redirection parsed (`STANDARD` or `PREPEND`).
If the redirection is valid, `fd` will contain the file descriptor
of the file to redirect to. 
Otherwise, it will print an error and message contain -1.
*/
char* parse_redirection(char *input, int *fd, REDIRECT_TYPE *redirect) {

    *redirect = determine_redirect_type(input);
    int split_len = 0; 
    char **split_input = NULL; 

    if (*redirect == REDIRECT_PREPEND || *redirect == REDIRECT_APPEND) {
        char *delim = (*redirect == REDIRECT_PREPEND) ? ">+" : ">>";  
        split_input = split_str(input, delim, &split_len); 

        // if a user tries to redirect more than once, 
        // we raise an error. 
        if (split_len > 2) {
            raise_error("Multiple redirection not supported."); 
            *fd = -1; 
        }
        else {
            char *trimmed_filename = trim_str(split_input[1]); 
            int split_filename_len = 0;
            char **split_filename = split_str(trimmed_filename, " ", &split_filename_len); 
            FILEPATH_STATUS status = validate_file(trimmed_filename);

            // If there is more than one file to redirect to, we raise a error.
            // If there is no file to redirect to, we raise an error.
            // If the file we are trying to redirect has an invalid path, we raise an error.
            if (split_filename_len > 1 || strlen(trimmed_filename) == 0) {
                raise_error("Expected one file to redirect to.");
            }
            else if (status == DIR_DOES_NOT_EXIST) {
                raise_error("Directory does not exist.");
                *fd = -1; 
            }
            // In the case that the file we are trying to prepend to doesn't exist, 
            // we should just conduct standard redirection. 
            else if (status == FILE_DOES_NOT_EXIST) {
               *fd = open(trimmed_filename, O_CREAT | O_RDWR, 0666); 
               *redirect = REDIRECT_STANDARD;
            }
            else {
                if (*redirect == REDIRECT_PREPEND)
                    *fd = open(trimmed_filename, O_RDWR); 
                else
                    *fd = open(trimmed_filename, O_WRONLY | O_APPEND); 
            }
    
            free(trimmed_filename); 
            free_str_array(split_filename);
        }
    }

    else if (*redirect == REDIRECT_STANDARD) {

        split_input = split_str(input, ">", &split_len); 

        if (split_len == 1) {
            if (strchr(input, '>') != NULL) { // no file specified
                raise_error("Expected one file to redirect to.");
                *fd = -1;
            }
            else { // no redirection
                *fd = STDOUT_FILENO;
            }
        }
        else if (split_len == 2) {
    
            char *trimmed_filename = trim_str(split_input[1]); 
            int split_filename_len = 0;
            char **split_filename = split_str(trimmed_filename, " ", &split_filename_len); 
            FILEPATH_STATUS status = validate_file(trimmed_filename); 
        
            // If there is more than one file to redirect to, we raise a error.
            // If there is no file to redirect to, we raise an error.
            // If the file we are trying to redirect to already exists 
            // OR cannot be created because it has an invalid path, we raise an error. 
            if (strlen(trimmed_filename) == 0 || split_filename_len > 1) {
                raise_error("Expected one file to redirect to.");
                *fd = -1;
            } 

            else if (status != FILE_DOES_NOT_EXIST) {
                raise_error("Directory does not exist.");
                *fd = -1;
            }
    
            else {
                *fd = open(trimmed_filename, O_CREAT | O_RDWR, 0666); 
            }
    
            free(trimmed_filename); 
            free_str_array(split_filename);
        }
        else { // we can only redirect once, so raise an error
            raise_error("Multiple redirection not supported."); 
            *fd = -1;
        }

    }

    char *cmd = strdup(split_input[0]);
    free_str_array(split_input);

    return cmd; 

}

/*
Runs non-built in commands by creating a forked process and
running executing the program specified in `cmdv`. 
`cmdv` must be null-terminated as this function calls `execvp()`. 
This function also handles file redirection by redirecting stdout 
to the file specified by file descriptor `dest_f` 
and the type of redirection specifed by `redirect` (`STANDARD`, `PREPEND`, or `APPEND`).
Note that to prevent any redirection (i.e., printing to stdout), 
`dest_f` should be specified as `STDOUT_FILENO`, and redirect should be `STANDARD`. 
*/
void exec_cmd(char **cmdv, int dest_f, REDIRECT_TYPE redirect) {
    int status; 

    int output_f; 
    char tmp_fname[] = "/tmp/prepend_tmpXXXXXX"; 
    output_f = (redirect == REDIRECT_PREPEND) ? mkstemp(tmp_fname) : dest_f;

    child_pid = fork();

    if (child_pid == 0) { // child process
        if (dup2(output_f, STDOUT_FILENO) >= 0) {
            execvp(cmdv[0], cmdv); 
        }

        // in the case that the command does not exist, execvp fails
        // and we execute this line of code
        // we then exit out of the child process 
        raise_error("Command does not exist."); 
        exit(1);
    }
    else if (child_pid > 0) { // parent process

        child_status = ACTIVE_CHILD_PROCESS;
        waitpid(child_pid, &status, 0); 

        child_status = NO_ACTIVE_CHILD_PROCESS; 

        // handles prepend redirection
        if (redirect == REDIRECT_PREPEND) {
            // we copy the initial file to the temporary file
            char c; 
            while (read(dest_f, &c, 1) > 0) {
                write(output_f, &c, 1); 
            }

            // now we copy the tmp file into the dest file
            lseek(dest_f, 0, SEEK_SET);
            lseek(output_f, 0, SEEK_SET); 
            while (read(output_f, &c, 1) > 0) {
                write(dest_f, &c, 1); 
            }

            // we can now delete the temporary file
            unlink(tmp_fname); 
            close(output_f); 
        }
    }

    else { // child process couldn't be created
        raise_error("Process could not be forked. Command will not be executed.");
    }
}

/*
Given a raw `input`, parse and run the command specified.
In the case that the command specified is one of the built in commands
(`cd`, `exit`, or `pwd`), the shell does not call `runCmd()` and instead uses internal
implementations of these commands. Otherwise, it calls `runCmd().`
*/
void parse_and_run_cmd(char *input) {
    int fd = STDOUT_FILENO; 
    REDIRECT_TYPE redirect; 
    char *cmd = parse_redirection(input, &fd, &redirect);
    if (fd == -1) { // in the case redirection fails
        free(cmd); 
        return;
    }

    char *trimmed_input = trim_str(cmd);
    int split_len = 0;
    char **split_input = split_str(trimmed_input, " ", &split_len); 
    free(cmd);
    free(trimmed_input); 

    // built in command: exit
    if (split_len >= 1 && strcmp(split_input[0], "exit") == 0) {
        if (split_len > 1) 
            raise_error("exit does not support command line options.");
        else if (fd != STDOUT_FILENO)
            raise_error("exit does not support redirection operations.");
        else {
            free(split_input[0]);
            free(split_input); 
            
            exit(0);
        }
    }

    // built in command: cd
    else if (split_len >= 1 && strcmp(split_input[0], "cd") == 0) {
        if (split_len > 2) 
            raise_error("cd only takes 1 parameter.");
        else if (fd != STDOUT_FILENO)
            raise_error("cd does not support redirection operations.");
        // If no directory is specified, we set it to the home directory.
        else if (split_len == 1) {
            if (chdir(getenv("HOME")) == -1) 
                raise_error("Could not change working directory.");
        }
        else {
            if (chdir(split_input[1]) == -1)
                raise_error("Could not change working directory.");
        }
    }

    // built in command: pwd
    else if (split_len >= 1 && strcmp(split_input[0], "pwd") == 0) {
        if (split_len > 1)
            raise_error("pwd does not support command line options.");
        else if (fd != STDOUT_FILENO)
            raise_error("pwd does not support redirection operations.");
        else {
            char *cwd = getcwd(NULL, 0);
            print_msg(cwd, "\n", NULL);
            free(cwd);
        }

    }

    else if (split_len >= 1) {
        exec_cmd(split_input, fd, redirect); 
    }

    if (fd != STDOUT_FILENO) {
        close(fd);
    }

    free_str_array(split_input);
}

int main(int argc, char *argv[]) 
{
    if (argc > 1) {
        raise_error("vsh does not support parameters. Exiting..."); 
        exit(EXIT_FAILURE); 
    }

    signal(SIGINT, SIGINT_handler); 

    char HOSTNAME[256];
    char *USERNAME = getenv("USER");
    char *HOME = getenv("HOME"); 
    gethostname(HOSTNAME, sizeof(HOSTNAME));

    // We initialize the shell to run from root, 
    // regardless of the actual entry point of the program.
    chdir(HOME); 

    char *line = NULL;
    size_t line_size; 

    while (1) {


        char *cwd = getcwd(NULL, 0);
        print_msg("(vsh) ", CYN, USERNAME, "@", HOSTNAME, RESET, ":" YEL, cwd, RESET, " > ", NULL);
        free(cwd); 

        getline(&line, &line_size, stdin); 

        /*Parses individual commands seperated by `;`, executing one at a time.*/
        int num_cmds = 0;
            char **split_cmds = split_str(line, ";", &num_cmds); 
    
            for (int i = 0; i < num_cmds; i++) {
                parse_and_run_cmd(split_cmds[i]); 
            }
    
            free_str_array(split_cmds);
    }

}
