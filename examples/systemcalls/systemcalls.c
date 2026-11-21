#include "systemcalls.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Wait for the child pid and return true only if it exited normally
 * with status 0. Retries if waitpid() is interrupted by a signal.
 */
static bool wait_for_child(pid_t pid)
{
    int status;

    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return false;
        }
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    if (cmd == NULL)
    {
        return false;
    }
    
    const int retval = system(cmd);
    if (retval == -1)
    {
        perror("system");
        return false;
    }
        
    return WIFEXITED(retval) && WEXITSTATUS(retval) == 0;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);
    fflush(stdout);
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return false;
    }
    else if (pid == 0)
    {
        execv(command[0], command);
        
        exit(-1);
    }
    
    return wait_for_child(pid);
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    va_end(args);

    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (fd < 0) 
    { 
        perror("open"); 
        return false; 
    }
    fflush(stdout);
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        close(fd);
        return false;
    }
    else if (pid == 0)
    {
        if (dup2(fd, STDOUT_FILENO) < 0) 
        { 
            perror("dup2"); 
            exit(-1); 
        }
        close(fd);
        execvp(command[0], command); 
        perror("execvp");         
        exit(-1);
    }
    
    close(fd);
    return wait_for_child(pid);
}

