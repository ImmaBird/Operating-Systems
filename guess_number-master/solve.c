#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main() {
  int pid;
  int n;
  char buf[101];
  int max = 101;
  int min = -1;
  int num;
  char guess[100];
  int done = 0;

  // CREATE FIRST PIPE
  int ptc[2];
  if(pipe(ptc) < 0){
    perror("parent to child pipe failed to open");
  }

  // CREATE SECOND PIPE
  int ctp[2];
  if(pipe(ctp) < 0){
    perror("child to parent pipe failed to open");
  }

  // FORK()
  pid = fork();

  if (pid == 0){
    // duplicate file descriptor 0 to point to FIRST pipe
    dup2(ptc[0],0);
    close(ptc[0]);

    // CLOSE ends of FIRST pipe you don't need anymore
    close(ptc[1]);

    // duplicate file descriptor 1 to point to SECOND pipe
    dup2(ctp[1],1);
    close(ctp[1]);

    // CLOSE ends of SECOND pipe you don't need anymore
    close(ctp[0]);

    execlp("./A4_CHILD", "./A4_CHILD", (char *) 0);
    perror("execlp");
    exit(-3);
  } else {

    // PARENT PROCESS COMES HERE

    close(ptc[0]);
    close(ctp[1]);
    while( 1 ) {


      int r=0;

      num = ((max-min)/2)+min;
      printf("Enter a Number: ");
      printf("%d", num);
      printf("\n");
      fflush(stdout);

      // SEND NUM to Child process
      sprintf(guess,"%d",num);
      write(ptc[1],guess,strlen(guess));

      // READ FROM CHILD THE RESPONSE into the variable buf and
      //store the return value from read() into the variable r
      r = read(ctp[0],buf,100);
      for(int i = 0; i < 100; i = i + 1){
        if(buf[i] == 'S'){
          min = num;
        }
        if(buf[i] == 'B'){
          max = num;
        }
        if(buf[i] == 'Y'){
          done = 1;
        }
      }

      if( r > 0 ) {
        buf[r] = '\0';
        printf("%s", buf);
        fflush(stdout);
      }else {
        printf("[PARENT] Reading from child: read() returned %d\n", r);
        break;
      }

      if(done){
        break;
      }
    }


    return(0);
  }
}
