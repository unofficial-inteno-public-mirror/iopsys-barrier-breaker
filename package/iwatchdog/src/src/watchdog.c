#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

const char *watchdog_file = "/proc/watchdog";
const char *init_string   = "1 5000000 1 4";
const char *kicker        = "OK";

int main(int argc, char **argv)
{
        pid_t pid, sid;
        int fd = open(watchdog_file, O_WRONLY);
        if (fd < 0) {
                perror("Open watchdog file");
                exit(1);
        }

        /* init */
        int res = write (fd, init_string, strlen(init_string) + 1);

        if (res < 0 ) {
                perror("Error init watchdog");
                exit(1);
        }

	/* daemonize */
	if (1) {

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
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}

        while (1) {
                sleep(1);
                res = write (fd, kicker, strlen(init_string) + 1);

                if (res < 0 ){
                        perror("Error kicking watchdog");
                }
        }
        return 0;
}
