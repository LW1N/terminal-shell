#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <fcntl.h>

// shell assumes that:
// max length of command line <= 512 characters
// program has a maximum of 16 non-null arguments
// max of 4 pipes
// max length of individual tokens never exceed 32 characters
#define CMDLINE_MAX 512
#define PIPE_MAX 4
#define VAR_MAX 26
#define TOKEN_MAX 32
// Output redirection flag, if 0 no redirect - if 1 redirect, 
// -1 if combined stdout + stderr redirection
int outredFlag = 0;
// Piping flag, if 0 no pipe - if 1 pipe
int pipeFlag = 0;
// Number of pipes
int num_pipes = 0;
// Number of program arguments
int num_args = 0;
// Number of programs in a pipeline
int num_programs = 0;
// Linked list of program name(first) and arguments
// Array of environment variables
char envar[VAR_MAX][TOKEN_MAX];

struct prog{
	// Program/argument name
	char* name;
	// Number corresponding to index of list item(starting at 0)
	// In an argument list, corresponds to number of argument
	// In a pipe/program list, corresponds to number of program, PIDs of children
	// as well as combined redirection flags(-1 for combined redirection)
	int argn;
	// Pointer to next item/argument in list
	struct prog* next;
};

// Append found argument to end of linked list
struct prog* appendArg(struct prog** first_ref, char* arg){
	// Create and allocate next argument
	struct prog *new_arg = (struct prog*) malloc(sizeof(struct prog));
	// Used to find end of list
	struct prog *tail = *first_ref;
	// Initialize what the arg is
	new_arg->name = arg;	
	// Since appended to end, points to NULL
	new_arg->next = NULL;
	struct prog *new_first = tail;

	// If linked list is empty new_arg becoms first arg(program name)
	if(*first_ref == NULL){
		// Program name is first, arg number 0
		new_arg->argn = 0;
		// First becomes new_arg
		*first_ref = new_arg;
		return new_arg;
	}
	// Otherwise find last(tail) argument
	while(tail->next != NULL){
		tail = tail->next;
	}

	// Argument number is simply 1 more than previous
	new_arg->argn = tail->argn + 1;	
	// Tail is now one before last(appended argument)
	// tail->next then needs to point to appened argument
	tail->next = new_arg;
	num_args = new_arg->argn;
	return new_first;
}

// Adjust argn of an entry in list to a given number n
// Used to keep track of combined redirection
struct prog* adjustArgn(struct prog** first_ref, char* arg, int n){
	// Used to find end of list
	struct prog *tail = *first_ref;
	// Used to return list pointer
	struct prog *new_first = tail;
	// Otherwise find last(tail) argument
	while(tail->name != arg){
		tail = tail->next;
	}
	tail->argn = n;
	
	return new_first;
}

// TESTING FUNCTION
void printArgn(struct prog** first_ref){
	struct prog* f = *first_ref;
	fprintf(stderr, "\n[");
	while(f != NULL){
		fprintf(stderr, " %d", f->argn);
		f = f->next;
	}
	fprintf(stderr, "]\n");
}

// Delete(free) linked list
void deleteArgs(struct prog** first_ref){
	// Current is pointer to parse list, starting at the beginning
	struct prog* current = *first_ref;
	// Next is pointer to argument after current, kind of a temp variable
	struct prog* next;

	// Parse list from beginning to end(when current is NULL)
	while (current != NULL){
		// Next holds argument after current
		next = current->next;
		// Deallocates memory allocated to current argument in list
		free(current);
		// Move on to next argument
		current = next;
	}

	// Reset pointer to first as NULL
	*first_ref = NULL;
}

// Transform linked list into a string
char* argstoString(struct prog* first_ref, int pipeF){
	// Allocate memory accordingly
	// str could be entire length of cmd line
	char* str =  malloc(CMDLINE_MAX);
	// Pointer parses linked list from beginning to end
	struct prog *ptr = first_ref;
	// Grab program name, store in str, add a space and move on
	strcpy(str, ptr->name);
	strcat(str, " ");
	ptr = ptr->next;
	while(ptr != NULL){
		// If last argument, don't need a space
		if(ptr->next == NULL){
			strcat(str, ptr->name);
			break;
		}
		// If no piping append argument to string
		if(pipeF == 0){
			strcat(str, ptr->name);
			strcat(str, " ");
		}
		// If piping, need to add | in between 
		if(pipeF == 1){
			strcat(str, ptr->name);
			strcat(str, " | ");
		}
		ptr = ptr->next;
	}

	return str;
}

// Close a certain number of pipes(numo)
void close_pipes(int array[][2], int nump){
	for(int i = 0; i <= nump; i++){
		close(array[i][0]);
		close(array[i][1]);
	}
}

// Redirect of program/pipeline to an output file
int output_Redirect(char* outfile_name){
	// integer for file descriptor
	int fd;
	// Grab file descriptor of file
	fd = open(outfile_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	// If cannot access output file, throw error and change outred flag to -1
	if(fd == -1){
		fprintf(stderr, "Error: cannot open output file\n");
		return -1;
	}
	// Redirect stdout to file fd
	dup2(fd, STDOUT_FILENO);
	// If outredFlag == 2, combined redirection
	// Redirect stderr to file fd
	if(outredFlag == 2){
		dup2(fd, STDERR_FILENO);
	}
	// Close fd
	close(fd);
	// Upon successful output redirection, outred flag is unchanged
	return outredFlag;
}

// Pipe programs into other programs
void pipeline(struct prog** first_ref){//, int numP){//int num_pipes){
	// Number of childern = number of programs = num_pipes + 1
	pid_t pids[num_pipes+1];
	// Number of pipes requested
	// Pipe variable, 
	// fd[pipenumber][read/write access file descriptor]
	int fd[num_pipes][2];
	// Variable to traverse list
	struct prog *current = *first_ref;
	// String to hold current command(program) to be executed
	char* command;
	// Establish pipes depending on how many needed
	for(int i = 0; i < num_pipes; i++){
		if(pipe(fd[i])){
			return;
		}
		
	}
	// Pipe programs from and into other programs
	for(int i = 0; i <= num_pipes; i++){
		// Traverse list of programs to be piped
		command = current->name;
		current = current->next;
		// Clone current process for piping and store PID of child in pids[]
		pids[i] = fork();
		// If parent, move on
		if(pids[i]){
			continue;
		}
		// If more than 2 pipes need to account for and close unneeded pipes
		if(i > 1 || (i == num_pipes && num_pipes > 2)){
			close_pipes(fd, i-2);
		}
		// If first program to be executed(first process)
		if(i == 0){
			// Don't need read access, so close
			close(fd[i][0]);
			// Replace stdout with pipe write access
			dup2(fd[i][1], STDOUT_FILENO);
			// Close pipe write access
			close(fd[i][1]);
			// Execute command to be piped
			execlp("/bin/sh", "sh", "-c", command,(char*)NULL);
			perror("execlp");
			exit(1);
		}
		// If last program to be executed(last process)
		if(i == num_pipes){
			// Don't need previous pipe write access, so close
			close(fd[i-1][1]);
			// Replace stdin with previous pipe read access
			dup2(fd[i-1][0], STDIN_FILENO);
			// Close previous pipe read access
			close(fd[i-1][0]);
			// Execute command that Child #1 gets piped into
			execlp("/bin/sh", "sh", "-c", command,(char*)NULL);			
			perror("execlp");
			exit(1);
		}
		// If a program in between programs(in between process(es))
		else if(i > 0 ){ //&& i != num_pipes){
			// Don't need previous pipe write access, so close
			close(fd[i-1][1]);
			// Replace stdin with previous pipe read access
			dup2(fd[i-1][0], STDIN_FILENO);
			// Close previous pipe read access
			close(fd[i-1][0]);
			// Replace stdout with current pipe write access
			dup2(fd[i][1], STDOUT_FILENO);
			// Close current pipe
			close(fd[i][0]);
			close(fd[i][1]);
			// Execute command that Child #1 gets piped into
			execlp("/bin/sh", "sh", "-c", command,(char*)NULL);			
			perror("execlp");
			exit(1);
		}
	}
		// Status holds status of children
		int status[num_pipes + 1];
		// Reset current to traverse list
		current = *first_ref;

		// Close all opened pipes in parent
		for(int i = 0; i < num_pipes; i++){
			close(fd[i][0]);
			close(fd[i][1]);
		}
		// For loop to wait for all children
		for(int i = 0; i <= num_pipes; i++){
			// Wait for all children
			waitpid(pids[i], &status[i], 0);
			// Store exit status of children in argn
			current->argn = WEXITSTATUS(status[i]);
			// Move onto next program/command
			current = current->next;
		}
}

// Print the exit statuses of piped programs
char* print_Exitstatus(struct prog** first_ref){
	// Allocate up to (PIPE_MAX * 3) worth of space
	// (1 command = '[exit_status]' = 3 characters)
	char* str = (char*) malloc(sizeof(char) * (3*PIPE_MAX));
	// Pointer parses linked list from beginning to end
	struct prog *current = *first_ref;
	// i increments length of str based on length of argument added to string 
	int i = 0;
	while(current != NULL){
		// Add exit status of program to string
		sprintf(str + i, "[%d]", current->argn);
		// i equals length of (string + i)
		i += strlen(str + i);
		// ptr becomes next argument in list
		current = current->next;
	}
	return str;
}



// Parse command line and find program(s) to be executed
char* parseCmd(struct prog** first_ref, struct prog** firstpipe_ref, char command[]){
	// Copy of command line to parse
	char cmdCopy[CMDLINE_MAX];
	strcpy(cmdCopy, command);
	// Delimiter to tokenize command line by
	char* delimiter = " ";
	char* delimiter2 = " ";
	// Pointers to adjust lists
	struct prog *firstarg = *first_ref;
	struct prog *firstpipe = *firstpipe_ref;
	// Pointers to hold reentrants for strtok_r() function
	char* save = cmdCopy;
	char* save2 = save;
	// Return string, contains name of output file or NULL if no output redirection
	char* outfile = "";
	// Strings to check if >, | or >&, |& were included in command line
	char* pipe_check;
	char* outred_check;
	char* combred_check[2];

	// If there are pipes, set pipeFlag and tokenize by pipes(|)
	pipe_check = strchr(cmdCopy, '|');
	if(pipe_check){
		pipeFlag = 1;
		delimiter = "|";
	}

	// Tokenizer
	char* token = strtok_r(cmdCopy, delimiter, &save);
	// Copy of token in case need to tokenize token further
	char* tokenCopy = token;
	char* tokenCopy2 = token;

	combred_check[0] = strstr(token, "&");
	combred_check[1] = strstr(token, ">&");
	// Parsing cmd string, loop of tokenizing until end of command line string
	while(token){
		// Two copies of the token in order for further tokenizing
		tokenCopy = token;
		tokenCopy2 = token;
		// Check if there is output redirection needed
		outred_check = strchr(token, '>');
		// Check if there is combined redirection needed
		combred_check[0] = strstr(token, "&");
		combred_check[1] = strstr(token, ">&");
		
		// Check if environment variable was included w/o piping
		while(pipeFlag == 0 && token[0] == '$'){
			// Keep track of environment variable usage with setting pipeFlag to -1
			pipeFlag = -1;
			// If invalid variable, throw error,  and return
			if(strlen(token) != 2 || token[1] < 'a' || token[1] > 'z'){
				fprintf(stderr, "Error invalid variable name\n");
				return "";
			}
			for(int i = 0; i < VAR_MAX; i++){
				if(i + 97 == token[1]){
					firstarg = appendArg(&firstarg, envar[i]);
				}
			}
			token = strtok_r(NULL, delimiter, &save);
			if(!token){
				goto ParseEnd;
			}
		}

		// If output redirection(>) is included w/o piping
		// i.e. program>out, program> out, or program >out
		if(outred_check && pipeFlag == 0){
			outredFlag = 1;
			delimiter2 = ">";
			// If >&, combined redirection
			if(combred_check[1]){
				outredFlag = 2;
				delimiter2 = ">&";
			}
			// *first_ref = firstarg;
			*firstpipe_ref = firstpipe;

			// If just a lone >(&), need to grab last program arg and outfile name
			if(token[0] == '>' && ((strlen(outred_check) == 1) || strlen(combred_check[1]) == 2)){
				token = strtok_r(NULL, delimiter, &save);
				// If next token is NULL, missing command error
				if(!token){
					outfile = "";
					return "";
				}
				// If next token != NULL, store as output file
				outfile = token;
				// If next token != NULL, mislocated output redirection error
				token = strtok_r(NULL, delimiter, &save);
				if(token){
					deleteArgs(&firstarg);
					*first_ref = firstarg;
					outfile = "";
					return "";
				}

				// If next token == NULL, at the end of cmd line - STOP PARSING
				goto ParseEnd;
			}

			// Grab token
			tokenCopy2 = strtok_r(tokenCopy, delimiter2, &save2);

			// If >outfile, no need to grab program/program argument
			if(token[0] == '>'){
				outfile = tokenCopy2;
				if((token = strtok_r(NULL, delimiter, &save))){
					outfile = "";
				}
				return outfile;
			}

			// Else if prog>out or prog> out, need to grab program argument
			firstarg = appendArg(&firstarg, tokenCopy2);
			fprintf(stderr, "CHeck it: %s\n", argstoString(firstarg, 0));
			*first_ref = firstarg;

			// If next token is NULL, prog> [could be prog> out or prog> NULL]
			tokenCopy2 = strtok_r(NULL, delimiter2, &save2);
			if(!tokenCopy2){
				// If next token is NULL, prog> NULL
				token = strtok_r(NULL, delimiter, &save);
				if(!token){
					outfile = "";
				}
				outfile = token;
				// if not end of line, mislocated output redirection error
				token = strtok_r(NULL, delimiter, &save);
				if(token){
					outfile = "";
				}
				return outfile;
			}
			// This would be if prog>outfile
			outfile = tokenCopy2;
			tokenCopy2 = strtok(NULL, delimiter2);
			token = strtok_r(NULL, delimiter, &save);
			if(token){
				outfile = "";
			}
			return outfile;
		}
		tokenCopy2 = strtok_r(tokenCopy, " ", &save2);
		//outred_check = strchr(token, '>');
		// If there are pipes, tokenize token of command line into program arguments
		while(pipeFlag == 1 && tokenCopy2){			
			// Grab program argument and append to firstarg
			firstarg = appendArg(&firstarg, tokenCopy2);

			// If tokenCopy2 is NULL, end of program arguments
			tokenCopy2 = strtok_r(NULL, " ", &save2);
			if(!tokenCopy2){
				// If next token is NULL, end of line
				if(!(token = strtok_r(NULL, delimiter, &save))){
					goto ParseEnd;
				}
				// If next token isn't null and there's an output redirection request
				// Error: output redirection misplacement
				if(token && (outred_check || combred_check[1])){
					outredFlag = 1;
					// Adjust pointers b/c of modifications
					*first_ref = firstarg;
					*firstpipe_ref = firstpipe;
					outfile = "";
					return "";
				}
				// Append found program+arguments to firstpipe
				firstpipe = appendArg(&firstpipe, argstoString(firstarg, 0));
				// Increment # of programs everytime a program is appended to firstpipe
				num_programs++;
				// Increment # of pipes everytime a program is appended and it is not the end of line
				num_pipes++;
				// Reset/Clear program arguments list for next iteration
				deleteArgs(&firstarg);
			}
		}
		// If no piping and no output redirection seen
		if(pipeFlag == 0 && outredFlag == 0){
			firstarg = appendArg(&firstarg,token);
			// fprintf(stderr, "TESTS: %s\n", argstoString(firstarg, 0));
			token = strtok_r(NULL, delimiter, &save);
		}
	}
	// Label to jump out of nested while loop
ParseEnd:
	// If piping, grab missed last program to be piped
	if(pipeFlag == 1 && firstarg != NULL){
		// If last missed program was output redirection, redirect output
		if(outred_check || combred_check[1]){
			outredFlag = 1;
			delimiter2 = ">";
			if(combred_check[1]){
				outredFlag = 2;
				delimiter2 = ">&";
			}
			// Tokenize by > to separate last program from output file
			token = strtok(argstoString(firstarg, 0), delimiter2);
			// Append last program to program list(firstpipe)
			firstpipe = appendArg(&firstpipe, token);
			num_programs++;
			// Grab next token, which should be output file
			token = strtok(NULL, delimiter2);
			outfile = token;
			*first_ref = firstarg;
			*firstpipe_ref = firstpipe;
			return outfile;
		}
		// If last missed program was not output redirection, append to program list
		firstpipe = appendArg(&firstpipe, argstoString(firstarg, 0));
		// Increment number of programs anytime one is appended to piping list
		num_programs++;
	}
	// Adjust pointers b/c of modifications and return name of outfile(null if no output redirection or error)
	*first_ref = firstarg;
	*firstpipe_ref = firstpipe;
	return outfile;
}

int main(void){
		// Command line storage strings
		char cmd[CMDLINE_MAX];
		// Variable to hold current working directory(cwd)
		char cwd[PATH_MAX];
		// Initialize first item in argument list(program name) as NULL
		struct prog *first = NULL;
		// Initialize first item in program list(first program to be piped) as NULL
		struct prog *firstPipe = NULL;
		// If there is redirection, outfile is name of output file
		char* outfile;
		// Initialize all strings in envar as empty
		for(int i = 0; i < VAR_MAX; i++){
			strcpy(envar[i], "");
		}
		
		while (1){
			char *nl;
			// Empty/reset linked lists of args/progs
			deleteArgs(&first);
			deleteArgs(&firstPipe);
			// With every prompt, reset global variables
			outredFlag = 0;
			pipeFlag = 0;
			num_pipes = 0;
			num_args = 0;
			num_programs = 0;

			// Provided Code
			/* Print prompt */
			printf("sshell@ucd$ ");
			fflush(stdout);
			/* Get command line */
			if(!fgets(cmd, CMDLINE_MAX, stdin)){
				continue;
			}

			/* Print command line if stdin is not provided by terminal */
			if (!isatty(STDIN_FILENO)) {
				printf("%s", cmd);
				fflush(stdout);
			}
			/* Remove trailing newline from command line */
			nl = strchr(cmd, '\n');
			if (nl){
				*nl = '\0';
			}
			/* Builtin command */
			if (!strcmp(cmd, "exit")) {
				fprintf(stderr, "Bye...\n");
				fprintf(stderr, "+ completed \'%s\' [%d]\n", cmd, 0);
				break;
			}
			// If no input, continue printing shell prompt
			if(cmd[0] == '\0'){
				continue;
			}

			// Parse the command line
			outfile = parseCmd(&first, &firstPipe, cmd);
			
			if(pipeFlag == -1){
				char* thing = malloc(CMDLINE_MAX);
				firstPipe = appendArg(&firstPipe, argstoString(first, 0));
				strcpy(thing, firstPipe->name);
				strcpy(cmd, thing);
				free(thing);
			}

			// ERROR MANAGEMENT
			if(outredFlag == -1 && outfile[0] == '\0'){
				continue;
			}
			// Error: too many process arguments
			// Print to stderr if more thatn 16 program arguments
			if(num_args >= 16){
				fprintf(stderr, "Error: too many process arguments\n");
				continue;
			}
			// Error: mislocated output redirection
			// If output redirection(>) requested but not at end of command line
			if(outredFlag > 0 && outfile[0] == '\0'){
				fprintf(stderr, "Error: mislocated output redirection\n");
				continue;
			}
			// Error: missing command
			// If output redirection(>) requested in shell but no command requested
			if(outredFlag > 0 && first == NULL){
				fprintf(stderr, "Error: missing command\n");
				continue;
			}
			// If piping(|) requested but not enough commands to be piped
			if(num_programs != num_pipes+1 && num_pipes > 0){
				fprintf(stderr, "Error: missing command!!!!!\n");
				continue;
			}
			if(pipeFlag == 1 && num_programs == 1){
				fprintf(stderr, "Error: missing command\n");
				continue;
			}

			// Builtin cd command
			if(!strcmp(first->name, "cd")){
				if(!first->next){
					continue;
				}
				// If an argument(directory) is passed with cd
				// Newdir holds directory to be changed to
				const char *newdir = first->next->name;
				// Change to newdir directory
				// If unsuccessful, throw error
				if(chdir(newdir)){
					fprintf(stderr, "Error: cannot cd into directory\n");
					fprintf(stderr, "+ completed \'%s\' [1]\n", cmd);
					continue;
				}
				fprintf(stderr, "+ completed \'%s\' [0]\n", cmd);
				continue;
			}

			// Builtin set command
			if(!strcmp(first->name, "set")){
				// If trying to set nothing or invalid variable, throw error and exit
				if(!first->next || first->next->name[0] < 'a' || first->next->name[0] > 'z' || strlen(first->next->name) > 1){
					fprintf(stderr, "Error: invalid variable name\n");
					continue;
				}
				// Find variable in envar array of strings and set value
				for(int i = 0; i < VAR_MAX; i++){
					if(i+97 == first->next->name[0]){
						strcpy(envar[i], first->next->next->name);
						fprintf(stderr, "+ completed \'%s\' [0]\n", cmd);
						continue;
					}
				}
			}

			/* Regular command */
			// Child Process
			if(!fork()){
				// Builtin pwd command
				if(!strcmp(first->name, "pwd")){
					// Retrieve current working directory(cwd)
					if(!getcwd(cwd, sizeof(cwd))){
						continue;
					}
					// Print cwd to stdout
					printf("%s\n", cwd);
					exit(1);
				}

				// If output redirection requested, redirect output to outfile
				if(outredFlag > 0){
					// outred now contains -1 if failed redirection, or unchanged if success
					outredFlag = output_Redirect(outfile);
				}
				// If redirecting stdout & error
				if(outredFlag == 2 && pipeFlag == 0){
					execlp("/bin/sh", "sh", "-c", argstoString(first, 0),(char*)NULL);
					perror("execlp");
					exit(1);
				}
				// If output redirection couldn't access output file, exit with exit status 10
				if(outredFlag == -1){
					exit(1);
				}
				// If piping requested, conduct piping
				if(pipeFlag == 1){
					pipeline(&firstPipe);
					fprintf(stderr, "+ completed \'%s\' %s\n", cmd, print_Exitstatus(&firstPipe));
					exit(1);
				}

				// If no piping, pass parsed command line string to execlp & exit
				execlp("/bin/sh", "sh", "-c", cmd,(char*)NULL);
				perror("execlp");
				exit(1);
			}
			// Parent Process
			else{
				int status;
				int exit_status;
				// Wait for child process
				waitpid(-1, &status, 0); 
				// Retrieve exit status of child process
				exit_status = WEXITSTATUS(status);

				// If output redirection wanted, but not possible, don't print completion message
				if(outredFlag == -1){
					continue;
				}
				// If piping print pipeline and exit status of commands in 
				if(pipeFlag == 1){
					
					continue;
				}

				// Print completion message
				fprintf(stderr, "+ completed \'%s\' [%d]\n", cmd, exit_status);	
			}
		}
		return EXIT_SUCCESS;
}
