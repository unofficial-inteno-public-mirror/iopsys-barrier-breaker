#include "brcmdaemon.h"

/* BUG: move me to header.*/
void    ledmngr( void );

/**************************************************************************
    Function: Print Usage
 
    Description:
        Output the command-line options for this daemon.
 
    Params:
        @argc - Standard argument count
        @argv - Standard argument array
 
    Returns:
        returns void always
**************************************************************************/
void PrintUsage(int argc, char *argv[]) {
    if (argc >=1) {
        printf("Usage: %s -h -n\n", argv[0]);
        printf("  Options: \n");
        printf("      -n\tDon't fork off as a daemon.\n");
        printf("      -h\tShow this help screen.\n");
        printf("\n");
    }
}
 
/**************************************************************************
    Function: signal_handler
 
    Description:
        This function handles select signals that the daemon may
        receive.  This gives the daemon a chance to properly shut
        down in emergency situations.  This function is installed
        as a signal handler in the 'main()' function.
 
    Params:
        @sig - The signal received
 
    Returns:
        returns void always
**************************************************************************/
void signal_handler(int sig) {
 
    switch(sig) {
        case SIGHUP:
            syslog(LOG_WARNING, "Received SIGHUP signal.");
            return;	    
        case SIGINT:
            syslog(LOG_WARNING, "Received SIGINT signal.");
            break;
        case SIGTERM:
            syslog(LOG_WARNING, "Received SIGTERM signal.");
            break;
        default:
            syslog(LOG_WARNING, "Unhandled signal (%d)", sig);
            break;
    }
    syslog(LOG_INFO, "%s daemon exiting", DAEMON_NAME);
    exit(0);
}
 
/**************************************************************************
    Function: main
 
    Description:
        The c standard 'main' entry point function.
 
    Params:
        @argc - count of command line arguments given on command line
        @argv - array of arguments given on command line
 
    Returns:
        returns integer which is passed back to the parent process
**************************************************************************/

#if defined(DEBUG)
    int daemonize = 0;
#else
    int daemonize = 1;
#endif

int main(int argc, char *argv[]) {
 
    // Setup signal handling before we start
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
 
    int c;
    while( (c = getopt(argc, argv, "nh|help")) != -1) {
        switch(c){
            case 'h':
                PrintUsage(argc, argv);
                exit(0);
                break;
            case 'n':
                daemonize = 0;
                break;
            default:
                PrintUsage(argc, argv);
                exit(1);
                break;
        }
    }
 
    syslog(LOG_INFO, "%s daemon starting up", DAEMON_NAME);
 
    // Setup syslog logging - see SETLOGMASK(3)
#if defined(DEBUG)
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
#else
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(DAEMON_NAME, LOG_CONS, LOG_USER);
#endif
 
    /* Our process ID and Session ID */
    pid_t pid, sid;
 
    if (daemonize) {
        syslog(LOG_INFO, "starting the daemonizing process");
 
        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
 
        /* Change the file mode mask */
        umask(0);
 
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }
 
        /* Change the current working directory */
        if ((chdir("/")) < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }
 
        /* Close out the standard file descriptors */
        //close(STDIN_FILENO);
        //close(STDOUT_FILENO);
        //close(STDERR_FILENO);
    }
 
    ledmngr(); 
    syslog(LOG_INFO, "%s daemon exiting", DAEMON_NAME);
    exit(0);
}