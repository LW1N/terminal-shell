#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

	/* test comment */
int main(void){
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
//                int retval;

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                /* Regular command */
                if(!fork()){
			execlp("/bin/sh", "sh", "-c", cmd, (char*)NULL);
			perror("execv");
			exit(1);
		}
		else{
			int status;
			int exit_status;
			waitpid(-1, &status, 0);
			if(WIFEXITED(status)){
				exit_status = WEXITSTATUS(status);
				if(!exit_status){
					fprintf(stderr, "+ completed \'%s\' [%d]\n", cmd, WEXITSTATUS(status));
				}
			}
		}
/*		retval = system(cmd);
                fprintf(stdout, "Return status value for '%s': %d\n",
                        cmd, retval);
			*/
        }

        return EXIT_SUCCESS;
	
}
