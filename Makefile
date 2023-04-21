sshell: sshell.c
	gcc -O2 -Wall -Wextra -Werror -o sshell sshell.c

clean:
	rm -f sshell sshell.o
