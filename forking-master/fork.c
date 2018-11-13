#include <stdio.h>

#include <unistd.h>     // execlp (for freeBSD)
#include <stdlib.h>     // exit() (for freeBSD)

// ps -ef | grep rawsonb | ws
// pipe[0] read
// pipe[1] write
int main() {
	int pipe1[2];
  int pipe2[2];
	int pid;

  // open pipe1
	if (pipe(pipe1) == -1) {
		perror("pipe failed");
		exit(-1);
	}

  // fork
	if ((pid = fork()) < 0) {
		perror("fork failed");
		exit(-2);
	}

  // child grep
	if (pid == 0) {
		close(pipe1[0]); // close read
		dup2(pipe1[1], 1); // make write stdout
		close(pipe1[1]); // close extra write

    ////////////////////////////////////////////////////////////
    // open pipe2
    if (pipe(pipe2) == -1) {
      perror("pipe failed");
      exit(-1);
    }

    // fork
  	if ((pid = fork()) < 0) {
  		perror("fork failed");
  		exit(-2);
  	}

    if (pid == 0) {
      close(pipe2[0]); // close read
      dup2(pipe2[1], 1); // make write stdout
      close(pipe2[1]); // close extra write
      execlp("ps", "ps", "-ef", (char *) 0);
      perror("ps failed");
      exit(-4);
    }
    /////////////////////////////////////////////////////////////
    else {
      close(pipe2[1]); // close write
      dup2(pipe2[0], 0); // make read stdin
      close(pipe2[0]); // close extra read
      execlp("grep", "grep", "rawsonb", (char *) 0); // grep for username
		  perror("grep failed");
		  exit(-3);
    }
	}

  // parent wc
	else {
		close(pipe1[1]); // close write
		dup2(pipe1[0], 0); // make read stdin
		close(pipe1[0]); // close extra read
		execlp("wc", "wc", (char *) 0);
		perror("wc failed");
		exit(-4);
	}

	exit(0);
}
