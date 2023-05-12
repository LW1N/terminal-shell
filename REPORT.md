# <b>Project Report | Lucas Nguyen, Youssef Qteishat</b>

## <u>Parsing Command Line:</u>
Initially, we suggested storing the program name and parameters in an array, but later we realized that it would be easier to parse through the command line if it were stored in a linked list.

Given that tokenizing modifies the string, the shell takes a copy of the command line and then parse through it and check for names of programs. It would also check for symbols for output redirection, piping, etc.

if no piping is requested, it the command line is tokenized by spaces between and checks for the programs and its arguments. But if it is requested, then the command line is tokenized by the "|" symbol, and the shell would parse through the code after the delimiter.

## <u>Output Redirection:</u>

Accounts for redirecting the output of process and pipes to other files. This is mostly implemented using dup2 function.

## <u>Pipping:</u>

Pipping is repeated based on the number of "|" symbols. At each iteration of the for loop, the process is forked. If it is the first iteration it uses dup2 to change the file descriptor for STDOUT to the write access of it's pipe.

If it's a middle iteration/process (there's more than 2 programs, more than 1 pipe) then the previous process' pipe is accessed in order to grab it's read access to change the current process' STDIN, and it's changes its STDOUT to its own pipe's write access.

It it's the last iteration/process/last program, then the previous process' pipe's read access is used to change the current process' STDIN.

And at each iteration, close_pipes is called in order to make sure that there are no hanging(unclosed) pipes left in a process.

## <u>Error Management:</u>

For error management, the error messages are outputted based on the signal flags sent out from all the other functions. 

## <u>Extra Features:</u>

In order to implement the combined redirection, the & symbol is kept track of by a string that is NULL if there is no combined redirection and has a value otherwise. This string would change how we parse for regular output redirection. During regular output redirection the flag outredFlag is set to 1 and the current token of the command line is parsed by a further token of the output redirection symbol, but during combined redirection the flag is set to 2 and the token is tokenized by > and &.

As for the environment variables, there is a global variable which is an array of 26 variables represented by strings of length 32(the maximum length of individual tokens) all initially empty. The builtin command set iterates through the array of variables and assigns it a given value if a correct variable name was given. Because of the global nature of this array, it is available for any functions to use and it is specifically utilized in the parsing function. Anytime a leading $ is included in a token of the command line and the character directly following it is between  a and z, the environmental variable array is searched through and the value of the variable asked takes the place of the token. The value of the variable is then the one that could be appended to one of the lists.
