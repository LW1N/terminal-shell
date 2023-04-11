#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

struct prog{
        char* pn; // program name
        char argArr[16][2]; // program argument array
};
/*
// populates array with empty strings
void newProg(struct prog *p, char* pp ){
        strcpy(p->pn, pp);

        for (int i=0; i< 16; i++){
                strcpy(p->argArr[i], "");
        }
}
*/




int main(void){
        char cmd[CMDLINE_MAX];
        char cmdCopy[CMDLINE_MAX];

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

                strcpy(cmdCopy,cmd);
                char* token = strtok(cmdCopy, " ");

                struct prog program;
                program.pn = token;
                token = strtok(NULL, " ");
                int count = 0;
                while(token){
                        if (token[0] == '-'){
                                strcpy(program.argArr[count], token);
                                count++;
                        }

                        token = strtok(NULL, " ");
                }

                char args[48];
                for (int i = 0; i < count+1; i ++){
                        if(i == 0){
                                strcpy(args, program.argArr[i]);
                        }
                        else{
                                strcat(args, program.argArr[i]);
                        }
                }
                printf("The args are %s\n", args);


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
					fprintf(stderr, "+ completed \'%s\' [%d]\n", program.pn, WEXITSTATUS(status));
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
