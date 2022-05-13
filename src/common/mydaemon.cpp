#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include "mydaemon.h"

// close_stdout决定是否输出至屏幕，默认关闭
void my_daemon(int close_stdout)
{
    pid_t pid;
    
    /* fork 父进程 */
    pid = fork();

    /* error occured */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* success: let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* on success: the child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    /* TODO: Implement a working signal handler */
    // signal(SIGCHLD, SIG_IGN);
    // signal(SIGHUP, SIG_IGN);
    
    /* Fork off for the second time*/
    pid = fork();
    
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    
    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    /* Set new file permissions */
    umask(0);
    
    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");
    
    /* Close all open file descriptors */
    if (close_stdout) {
        int x;
        for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
        {
            close (x);
        }
    } 
}
