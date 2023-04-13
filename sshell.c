#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

// Linked list of program(head) and arguments
struct prog{
        char* arg; // program argument
	int argn; // number corresponding to number of argument(starting at 0)
	struct prog* next;
};

// Insert found argument into linked list
void appendArg(struct prog** first_ref, char* arg){//, int argn){
	// Create and allocate next argument
	struct prog *new_arg = (struct prog*) malloc(sizeof(struct prog));
	// Used to find end of list
	struct prog *tail = *first_ref;
	
	// Initialize what the arg is
	new_arg->arg = arg;	
	// Since appended to end, points to NULL
	new_arg->next = NULL;

	// If linked list is empty or arg doesn't start with -
	// new_prog becoms first prog(program name)
	if(*first_ref == NULL || arg[0] != '-'){
		new_arg->argn = 0;
		*first_ref = new_arg;
		return;
	}



	// Otherwise find last(tail) argument
	while(tail->next != NULL){
		tail = tail->next;
	}

	// Argument number is simply 1 more than previous
	new_arg->argn = tail->argn + 1;	
	
// TESTING
	printf("Arg: %s, Argn: %d\n", new_arg->arg, new_arg->argn);
	
	// Last tail points to next argument
	tail->next = new_arg;
	return;
}


int main(void){
        char cmd[CMDLINE_MAX];
        char cmdCopy[CMDLINE_MAX];
	struct prog *first = NULL; // first argument is name of program being executed
//	struct prog *current = NULL; // current argument being accessed

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


		// Copy and parse retrieved command line
                strcpy(cmdCopy,cmd);
                // Tokenize command line by whitespace to retrieve arguments
		char* token = strtok(cmdCopy, " ");
		// First token is name of progam to be executed
		appendArg(&first, token);
		// Contine after first token
                token = strtok(NULL, " ");
                while(token){
			// If the token starts with -, then it is an argument
                        if (token[0] == '-'){
				appendArg(&first, token);
                        }
			// Grab next token
                        token = strtok(NULL, " ");
                }


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
			// Check if child process completed(has exited)
			if(WIFEXITED(status)){
				// Grab exit status
				exit_status = WEXITSTATUS(status);
				// If exit_status is 0, print completion msg to stderr
				if(!exit_status){
					fprintf(stderr, "+ completed \'%s\' [%d]\n", first->arg, WEXITSTATUS(status));
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
