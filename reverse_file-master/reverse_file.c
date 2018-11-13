/****************************************************
 * Bradley Rawson Jr
 * Assignment 1
 * 9/13/2017
 ****************************************************/

 #include <fcntl.h>
 #include <stdio.h>
 #include <sys/types.h>
 #include <unistd.h>
 #include <stdlib.h>

int main ( int argc, char *argv[]) {
	int i;
	int rr;
	int infile, outfile, filesize;
	char buffer[1];

	if( argc !=3 ) {
		fprintf(stderr, "USAGE: %s inputFile outputFile.\n", argv[0]);
		exit(-1);
	}

	if( (infile = open(argv[1], O_RDONLY)) == -1) {
		fprintf(stderr, "Could not open %s.\n", argv[1]);
		return(-1);
	}

	if((outfile = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644))==-1) {
		close(infile);
		fprintf(stderr, "Could not open %s.\n", argv[2]);
		return (-2);
	}

	filesize=lseek(infile, (off_t)0, SEEK_END); //Pointer on null char
	lseek(infile, (off_t)-1, SEEK_END); //Pointer on newline char

	for(i=filesize-1; i>=0; i--) {

		rr = read(infile, buffer, 1);	/* read one byte, ponter moved forward 1 */

		//lseek(infile, (off_t)-2, SEEK_CUR); //Pointer moved back 2
		//lseek(infile, (off_t)i-1, SEEK_SET); //Pointer moved forward i-1 from start
		lseek(infile, (off_t)0-(filesize-(i-1)), SEEK_END); //Pointer moved back filesize-(i-1) from end

		if( rr != 1 ) {
			fprintf(stderr, "Couldn't read 1 byte [%d]\n", rr);
			exit(-1);
		}

		rr = write(outfile, buffer, 1); /* write the byte to the file*/

		if( rr != 1 ) {
			fprintf(stderr, "Couldn't write 1 byte [%d]\n", rr);
			exit(-1);
		}
	}

	close(infile);
	close(outfile);

	return(0);
}
