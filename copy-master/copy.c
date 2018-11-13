/****************************************************
 * Bradley Rawson Jr
 * Assignment 2
 * 9/27/2017
 ****************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>


// copys the input_file to the output_file
void copy(char input_file_dir[], char output_file_dir[]){
  char buffer[256];
  int bytes_read = 256;
  int input_file, output_file;

	// open input_file
  if((input_file = open(input_file_dir, O_RDONLY)) < 0){
    perror("FAILED TO OPEN INPUT FILE");
    exit(-2);
  }

	// open output_file
  if((output_file = open(output_file_dir, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0){
    perror("FAILED TO OPEN OUTPUT FILE");
    close(input_file);
    exit(-3);
  }

  // loops through bytes in input file and writes them to the output file
  while(1){
    bytes_read = read(input_file, buffer, 256);

    if(bytes_read < 0){
      perror("FAILED TO READ INPUT FILE");
    }

    if(bytes_read != 0){
      write(output_file, buffer, bytes_read);
    }else{
      break;
    }
  }

	// close files
  close(input_file);
  close(output_file);
}

int main(int argc, char *argv[]){
  int i;
  char output_file_dir[100];

  // error if not enough arguments
  if(argc < 3){
    fprintf(stderr, "USAGE: %s inputFile outputFile.\n", argv[0]);
    exit(-1);
  }

  // loop through input files and performs the copy
  for(i=1; i < argc-1; i++){
    sprintf(output_file_dir, "%s/%s_copy", argv[argc-1], argv[i]);
    copy(argv[i], output_file_dir);
  }

	return(0);
}
