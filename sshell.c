#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define PIPE_MAX 4
// Output redirection flag, if 0 no redirect - if 1 redirect
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

struct prog{
	// Program/argument name
	char* name;
	// Number corresponding to number of argument(starting at 0)
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
//TESTING
	//printf("Arg: %s, Argn: %d\n", new_arg->arg, new_arg->argn);
	// Tail is now one before last(appended argument)
	// tail->next then needs to point to appened argument
	tail->next = new_arg;
	num_args = new_arg->argn;
	return new_first;
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

// Transform linked list into a string to pass to execlp
char* argstoString(struct prog** first_ref, int pipeF){
	// Allocate memory accordingly
	// str could be entire length of cmd line
	char* str = (char*) malloc(sizeof(char) * CMDLINE_MAX);
	// Pointer parses linked list from beginning to end
	struct prog *ptr = *first_ref;
	// i increments length of str based on length of argument added to string 
	int i = 0;

	// Parse linked list from first to end
	while(ptr != NULL){
		// If last argument, no need for trailing space
		if(ptr->next == NULL){
			sprintf(str + i, "%s", ptr->name);
			i += strlen(str + i);
			break;
		}
		// If no piping append argument to string
		if(pipeF == 0){
			sprintf(str + i, "%s ", ptr->name);
		}
		// If piping, need to add | in between 
		if(pipeF == 1){
			sprintf(str + i, "%s | ", ptr->name);
		}
		// i equals length of (string + i)
		i += strlen(str + i);
		// ptr becomes next argument in list
		ptr = ptr->next;
	}

	return str;
}

// Close a certain number of pipes
void close_pipes(int array[][2], int nump){
	for(int i = 0; i <= nump; i++){
		close(array[i][0]);
		close(array[i][1]);
	}
}

// Pipe programs into other programs
void pipeline(struct prog** first_ref){//, int numP){//int num_pipes){
	// Number of childern = number of programs = num_pipes + 1
	pid_t pids[num_pipes+1];
	// Number of pipes requested
	//int num_pipes = numP;
	// Pipe variable, 
	// fd[pipenumber][read/write access file descriptor]
	int fd[num_pipes][2];
	// Variable to traverse list
	struct prog *current = *first_ref;
	// String to hold current command(program) to be executed
	char* command;
	//TESTING
	// fprintf(stderr, "Number of pipes: %d\n", num_pipes);
	// Establish pipes depending on how many needed
	for(int i = 0; i < num_pipes; i++){
		pipe(fd[i]);
		//TESTING
		// fprintf(stderr, "Pipe #%d Read:%d Write:%d\n", i, fd[i][0], fd[i][1]);
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
	// Close fd
	close(fd);
	// Upon successful output redirection, outred flag is unchanged
	return 1;
}

// Parse command line and find program(s) to be executed
char* parseCmd(struct prog** first_ref, struct prog** firstpipe_ref, char command[]){
	// Copy of command line to parse
	char cmdCopy[CMDLINE_MAX];
	strcpy(cmdCopy, command);
	// Delimiter to tokenize command line by
	char* delimiter = " ";
	// Pointers to adjust lists
	struct prog *firstarg = *first_ref;
	struct prog *firstpipe = *firstpipe_ref;
	char* save = cmdCopy;
	char* save2;
	// Tokenizer
	char* token = strtok_r(cmdCopy, delimiter, &save);
	// Copy of token in case need to tokenize token further
	char* tokenCopy = token;
	// Return string, contains name of output file or NULL if no output redirection
	char* outfile = "";
	// Strings to check if > or | was included without spaces in command line
	char* pipe_check;
	char* outred_check;

	// Parsing cmd string, loop of tokenizing until end of command line string
	while(token){
		tokenCopy = token;
		save2 = token;
		// Check if > or | was included without spaces
		// If a pipe(|) was included without spaces
		// i.e. program arg|program arg, or arg|program>outfile
		//pipe_check = strchr(token, '|');
		pipe_check = strchr(token, '|');
		outred_check = strchr(token, '>');
		while((pipe_check  && strlen(pipe_check) > 1)){
			// Increment number of programs
			num_programs++;
			num_pipes++;
			// Set pipeFlag
			pipeFlag = 1;
			// Use pipe_check to tokenize token2 by pipe symbol(|)
			tokenCopy = strtok_r(tokenCopy, "|", &save2);

			// If arg |program
			if(pipe_check-token == 0){
				// Append previous program to be piped to program list and adjust num_programs
				firstpipe = appendArg(&firstpipe, argstoString(&firstarg, 0));
				// num_programs++;
				// Reset program arguments list
				deleteArgs(&firstarg);
				// Append program name to arguments list
				firstarg = appendArg(&firstarg, tokenCopy);
				// TESTING
				// fprintf(stderr, "YO YO YO: %s %d\n", argstoString(&firstpipe, 0), num_programs);
				// Grab next whitespace token, if NULL, break out of loop(end of line)
				if(!(token = strtok_r(NULL, delimiter, &save))){
					// Jump out of loops
					goto ParseEnd;
				}
				pipe_check = strchr(token, '|');
				tokenCopy = token;
				break;
			}

			// Append first token to program arguments list
			firstarg = appendArg(&firstarg, tokenCopy);
			// Append program to be piped to program list and adjust num_programs
			firstpipe = appendArg(&firstpipe, argstoString(&firstarg, 0));
			// Reset program arguments list
			deleteArgs(&firstarg);
			// Grab 2nd half of arg|program(>outfile)
			tokenCopy = strtok_r(NULL, "|", &save2);

			// If arg| program or arg |program
			// if(!tokenCopy){

			// }

			// If there is also a output redirection > w/o space
			// i.e. program arg|program>outfile
			if((outred_check && strlen(outred_check) > 1)){
				// Tokenize token string by >
				tokenCopy = strtok_r(tokenCopy, ">", &save2);
				// Add last argument to argumentt list and append to program(piping) list
				firstarg = appendArg(&firstarg, tokenCopy);
				firstpipe = appendArg(&firstpipe, argstoString(&firstarg, 0));
				// Increment number of programs whenever appending piping list
				num_programs++;
				// Grab next token(should be output file)
				tokenCopy = strtok_r(NULL, ">", &save2);
				// Store in token, so that parser will recognize
				token = tokenCopy;
				// Assert output redirection flag
				outredFlag = 1;
				// Go back to parsing by whitespace
				break;
			}

			// Append to program args(first arg/program name)
			// num_programs = appendArg(&firstpipe, tokenCopy);
			firstarg = appendArg(&firstarg, tokenCopy);
			
			// Grab next whitespace token, if NULL, break out of loop(end of line)
			if(!(token = strtok_r(NULL, delimiter, &save))){
				// Jump out of loops
				goto ParseEnd;
			}
			pipe_check = strchr(token, '|');
			tokenCopy = token;
			//continue;
		}
		// If only program>outfile
		if(!pipe_check && outred_check && strlen(outred_check) > 1){
			return outfile;
		}
		// ERROR MANAGEMENT
		// If output redirection(>) or pipe(|) but no arguments
		// Assert output redirection flag, with first as NULL and break to throw error
		if((token[0] == '|' || token[0] == '>') && num_args == 0){
			// Adjust pointers b/c of modifications
			*first_ref = firstarg;
			*firstpipe_ref = firstpipe;
			outredFlag = 1;
			deleteArgs(&firstarg);
			return "";
		}
		// if output redirection needed, grab name of output file
		if(outredFlag == 1){
			*first_ref = firstarg;
			*firstpipe_ref = firstpipe;
			outfile = token;
			// if(!(token = strtok(NULL, delimiter))){
			// 	return outfile;
			// }
			// If next token != NULL output redirection misplaced(not at end of cmd line)
			if((token = strtok_r(NULL, delimiter, &save))){
				outfile = "";
				break;
			}
			// If next token == NULL, at the end of cmd line - STOP PARSING
			break;
		}
		// If user is asking for output redirection,  assert flag
		if(token[0] == '>'){
			outredFlag = 1;
		}
		// If user is asking for piping, convert argument list to string
		// Add string(program + args) to list of pipes
		if(token[0] == '|'){
		//if(cmd[token-cmdCopy+strlen(token)] == '|'){
			num_pipes++;
			// num_programs = appendArg(&firstpipe, argstoString(&firstarg, 0));
			firstpipe = appendArg(&firstpipe, argstoString(&firstarg, 0));
			num_programs++;
			// Empy/reset list of args
			deleteArgs(&firstarg);
			// Assert flag
			pipeFlag = 1;
		}
		// Append next arg to linked list
		// If there is output redirection, linked list represents everything before >
		if(outredFlag == 0 && token[0] != '>' && token[0] != '|'){//cmd[token-cmdCopy+strlen(token)] == '|'){ 
			firstarg = appendArg(&firstarg,token);
		}
		// Grab next token
		//token = strtok(NULL, delimiter);
		token = strtok_r(NULL, delimiter, &save);
	}
	// Label to jump out of nested while loop
ParseEnd:
	// If piping, grab missed last program to be piped
	if(pipeFlag == 1 && firstarg != NULL){
		firstpipe = appendArg(&firstpipe, argstoString(&firstarg, 0));
		// Increment number of programs anytime one is appended to piping list
		num_programs++;
	}
	// Adjust pointers b/c of modifications
	*first_ref = firstarg;
	*firstpipe_ref = firstpipe;
	return outfile;
}

int main(void){
		// Command line storage strings
		char cmd[CMDLINE_MAX];
		// char cmdCopy[CMDLINE_MAX];
		// Delimiter to parse command line by
		// char* delimiter = " ";
		// Variable to hold current working directory(cwd)
		char cwd[PATH_MAX];
		// Initialize first item in argument list(program name) as NULL
		struct prog *first = NULL;
		// Initialize first item in program list(first program to be piped) as NULL
		struct prog *firstPipe = NULL;
		

		while (1){
			char *nl;
//			int retval;
			// If there is redirection, outfile is name of output file
			char* outfile;
			// Empty/reset linked lists of args/progs
			deleteArgs(&first);
			deleteArgs(&firstPipe);
			// Output redirection flag, if 0 no redirect - if 1 redirect
			outredFlag = 0;
			// Piping flag, if 0 no pipe - if 1 pipe
			pipeFlag = 0;
			// Number of pipes
			num_pipes = 0;
			// Number of program arguments
			num_args = 0;
			// Number of programs in a pipeline
			num_programs = 0;


			// Provided Code
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

			outfile = parseCmd(&first, &firstPipe, cmd);
			
			// // TESTING
			// fprintf(stderr, "OUTFILE: %s\n", outfile);
			// fprintf(stderr, "PIPELINE: %s\n", argstoString(&firstPipe, 0));
			// fprintf(stderr, "NUMBER OF PIPES: %d\n", num_pipes);
			// // printf("The number of pipes: %d\n", num_pipes);
			// // printf("The number of arguments: %d\n",num_args);
			// fprintf(stderr, "The number of programs: %d\n",num_programs);
			// fprintf(stderr, "OUTFILE: %s\n", outfile);

			// ERROR MANAGEMENT
			// Error: too many process arguments
			// Print to stderr if more thatn 16 program arguments
			if(num_args >= 16){
				fprintf(stderr, "Error: too many process arguments\n");
				continue;
			}
			// Error: missing command
			// If output redirection(>) requested in shell but no command requested
			if(outredFlag == 1 && first == NULL){
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
			// Error: mislocated output redirection
			// If output redirection(>) requested but not at end of command line
			if(outredFlag == 1 && outfile[0] == '\0'){
				fprintf(stderr, "Error: mislocated output redirection\n");
				continue;
			}
			
			
			/* Builtin commands */ 
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
					continue;
				}
				// Print completion message
				fprintf(stderr, "+ completed '%s' [0]\n", cmd);
				continue;
			}
			
			// Builtin pwd command
			if(!strcmp(first->name, "pwd")){
				// Retrieve current working directory(cwd)
				getcwd(cwd, sizeof(cwd));
				// Print cwd to stdout along with completion message
				printf("%s\n", cwd);
				fprintf(stderr, "+ completed '%s' [0]\n", cmd);
				continue;
			}

			/* Regular command */
			// Child Process
			if(!fork()){
				// If output redirection requested, redirect output to outfile
				if(outredFlag == 1){
					// outred now contains -1 if failed redirection, or 1 if success
					outredFlag = output_Redirect(outfile);
				}
				// If output redirection couldn't access output file, exit with exit status 10
				if(outredFlag == -1){
					exit(10);
				}
				// If piping requested, conduct piping and print completion message
				if(pipeFlag == 1){
					pipeline(&firstPipe);//, num_pipes);
					// Print pipeline and exit status of commands in 
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
				if(exit_status == 10){
					continue;
				}
				// If piping, completion message was already printed in child
				if(pipeFlag == 1){
					continue;
				}
				// Print completion message
				fprintf(stderr, "+ completed \'%s\' [%d]\n", cmd, exit_status);	
			}
			
			// Old return status for skeleton code
			// retval = system(cmd);
			// fprintf(stdout, "Return status value for '%s': %d\n",
			// cmd, retval);
		}
		return EXIT_SUCCESS;
}
