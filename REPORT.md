# <b>Project 1 Report| Lucas Nguyen, Youssef Qteishat</b>

## <u>Parsing Command Line:</u>

Initially, we suggested storing the program name and parameters in an array, but later we reliazed that it would be easier to parse throught he command line if were stored in a linked list. Becasue then, the head node would be the program name and the next nodes would be the program arguements.

For example, if asked to print the working directory, the shell would look at the head node and compare it to the command name. If they match, then that program would be exceuted.

And If the program has arguements, then the shell would look at the next nodes and excute the command acording to the next arguement(s).

## <u>Output Redirection:</u>

Accounts for redirecting the output of proccess and pipes to other files.

## <u>Piping:</u>

## <u>Error Management:</u>

## <u>Extra Features:</u>

