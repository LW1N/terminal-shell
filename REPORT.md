# <b>Project 1 Report| Lucas Nguyen, Youssef Qteishat</b>

## <u>Parsing Command Line:</u>
Initially, we suggested storing the program name and parameters in an array, but later we reliazed that it would be easier to parse through the command line if it were stored in a linked list.

Given that tokenizing modifies the string, the shell takes a copy of the command line and then parse through it and check for names of programs. It would also check for symbols for output redirection, piping, etc.

if no piping is requested, it the command line is tokenized by spaces between and checks for the programs and its arguements. But if it is requested, then the command line is tokenized by the "|" symbol, and the shell would parse through the code after the delimator.

## <u>Output Redirection:</u>

Accounts for redirecting the output of proccess and pipes to other files. This is mostly implemented using dup2 function.

## <u>Piping:</u>

## <u>Error Management:</u>

For error management, the error messages are outputted based on the signal flags sent out from all the other fucntions.

## <u>Extra Features:</u>

