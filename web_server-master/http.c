/////////////////////////////////////////////////////////////////////////
//
// To compile: 			gcc -o my_httpd my_httpd.c -lnsl -lsocket
//
// To start your server:	./my_httpd 2000 .www
//
// To Kill your server:		kill_my_httpd
//
/////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include <stdlib.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <signal.h>

#define	ERROR_FILE	0
#define REG_FILE  	1
#define DIRECTORY 	2

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void GetMyHomeDir(char *myhome, char **environ) {
        int i=0;
	while( environ[i] != NULL ) {
        	if( ! strncmp(environ[i], "HOME=", 5) ){
			strcpy(myhome, &environ[i][5]);
			return;
		}
		i++;
        }
	fprintf(stderr, "[FATAL] SHOULD NEVER COME HERE\n");
	fflush(stderr);
	exit(-1);
}

////////////////////////////////////////////////////////////////////
// Tells us if the request is a directory or a regular file
////////////////////////////////////////////////////////////////////
int TypeOfFile(char *fullPathToFile) {
	struct stat buf;	/* to stat the file/dir */

        if( stat(fullPathToFile, &buf) != 0 ) {
		perror("stat()");
		fprintf(stderr, "[ERROR] stat() on file: |%s|\n",
						fullPathToFile);
		fflush(stderr);
                exit(-1);
        }


        if( S_ISREG(buf.st_mode) )
		return(REG_FILE);
        else if( S_ISDIR(buf.st_mode) )
		return(DIRECTORY);

	return(ERROR_FILE);
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void SendDataBin(char *fileToSend, int sock, char *home, char *content) {
        int f;
	char fullPathToFile[256];
	char Header[1024];
        int s;
        char buffer[10];
	int fret;		/* return value of TypeOfFile() */

/*
	 * Build the full path to the file
	 */
	sprintf(fullPathToFile, "%s/%s%s", home, content, fileToSend);

// find content length
  int file = open(fullPathToFile, O_RDONLY);
  struct stat st;
  stat(fullPathToFile,&st);
  int length = st.st_size;

	/*
	 * Build the header
	 */

	sprintf(Header, "HTTP/1.0 200 OK\nContent-length: %d\nContent-type: text/html\n\n",length);







	/*
	 * - If the requested file is a directory, append the 'index.html'
    	 *   file to the end of the fullPathToFile
	 *   (Use TypeOfFile(fullPathToFile))
	 * - If the requested file is a regular file, do nothing and proceed
	 * - else your client requested something other than a directory
	 *   or a reqular file
	 */
	/* TODO 5 */



	/*
	 * 1. Send the header (use write())
	 * 2. open the requested file (use open())
	 * 3. now send the requested file (use write())
	 * 4. close the file (use close())
	 */

	 /* TODO 6 */
   write(sock, Header, strlen(Header));
   int r = 1;
   char buff[256];
   while(r > 0){
     r = read(file,buff,256);
     write(sock,buff,r);
   }
   close(file);

}


////////////////////////////////////////////////////////////////////
// Extract the file request from the request lines the client sent
// to us.  Make sure you NULL terminate your result.
////////////////////////////////////////////////////////////////////
void ExtractFileRequest(char *req, char *buff) {
	/* TODO 4  */
  int start = 0;
  int j = 0;
  for(int i =0; i<1024;i++){
    if(buff[i] == ' '){
      j++;
      start = i+1;
    }
    if(j == 1){
      req[i-start] = buff[i];
    }
  }
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
int main(int argc, char **argv, char **environ) {
  	pid_t pid;		/* pid of child */
	int sockid;		/* our initial socket */
	int PORT;		/* Port number, used by 'bind' */
	char content[128];	/* Your directory that contains your web
				   content such as .www in
				   your home directory */
	char myhome[128];	/* Your home directory */
				/* (gets filled in by GetMyHomeDir() */
	/*
	 * structs used for bind, accept..
	 */
  	struct sockaddr_in server_addr, client_addr;

	char file_request[256];	/* where we store the requested file name */
        int one=1;		/* used to set socket options */


	/*
	 * Get my home directory from the environment
	 */
	GetMyHomeDir(myhome, environ);

	if( argc != 3 ) {
		fprintf(stderr, "USAGE: %s <port number> <content directory>\n",
								argv[0]);
		exit(-1);
	}

	PORT = atoi(argv[1]);		/* Get the port number */
	strcpy(content, argv[2]);	/* Get the content directory */


	if ( (pid = fork()) < 0) {
		perror("Cannot fork (for deamon)");
		exit(0);
  	}
	else if (pid != 0) {
		/*
	  	 * I am the parent
		 */
		char t[128];
		sprintf(t, "echo %d > %s.pid\n", pid, argv[0]);
		system(t);
    		exit(0);
  	}

  	// setsid();
  	// chdir("/");
  	// umask(0);

	/*
	 * Create our socket, bind it, listen
	 */

	/* TODO 1 */

  if( (sockid = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    		perror("socket");
		exit(-1);
	}

  	/*
	 * Set up structures to bind the address to the socket.
	 */
  	bzero(&server_addr, sizeof(server_addr));
  	server_addr.sin_family = AF_INET;

        /*
         * INADDR_ANY says that the operating system may choose to
	 * which local IP address to attach the application.
	 * For most machines, which only have one address,
         * this simply chooses that address.  The htonl() function converts
	 * a four-byte integer into the network byte order so that other
         * hosts can interpret the integer even if they internally store
	 * integers using a different byte order.
         */
  	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	server_addr.sin_port = htons(PORT);

	/*
	 * bind the socket to a well-known port
	 */
  	if (bind(sockid, (struct sockaddr_in *) &server_addr, sizeof(server_addr)) != 0) {
		perror("can't bind to socket");
		exit(-1);
	}

	if (listen(sockid, 5) < 0) {
    		perror("listen");
		exit(-1);
	}

	signal(SIGCHLD, SIG_IGN);


	/*
	 * - accept a new connection and fork.
	 * - If you are the child process,  process the request and exit.
	 * - If you are the parent close the socket and come back to
         *   accept another connection
	 */
  	while (1) {
		/*
		 * socket that will be used for communication
		 * between the client and this server (the child)
		 */
		int newsock;

		/*
		 * Get the size of this structure, could pass NULL if we
		 * don't care about who the client is.
		 */
   		int client_len = sizeof(client_addr);

		/*
		 * Accept a connection from a client (a web browser)
		 * accept the new connection. newsock will be used for the
		 * child to communicate to the client (browser)
		 */
		 /* TODO 2 */
     newsock = accept(sockid, (struct sockaddr_in *) &client_addr,
     &client_len);
    		if (newsock < 0) {
			perror("accept");
			exit(-1);
		}

		if ( (pid = fork()) < 0) {
			perror("Cannot fork");
			exit(0);
  		}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
		else if( pid == 0 ) {
			/*
			 * I am the Child
			 */
			int r;
      			char buff[1024];
			int read_so_far = 0;
			char ref[1024], rowRef[1024];

			close(sockid);

			memset(buff, 0, 1024);

			/*
			 * Read client request into buff
			 * 'use a while loop'
			 */
			/* TODO 3 */


      read_so_far = read(newsock, buff, 1023);


//
// What you may get from the client:
//			GET / HTTP/1.0
//			Connection: Keep-Alive
//			User-Agent: Mozilla/3.0 (X11; I; SunOS 5.5 sun4m)
//			Host: spiff:6789
//			Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, * /*
//
//
// Write to client
//
//			You should write to the client an HTTP response and then the
//			requested file, if appropriate. A response may look like:
//
//			HTTP/1.0 200 OK
//			Content-length: 2032
//			Content-type: text/html
//			[single blank line necessary here]
//			[document follows]
//



			ExtractFileRequest(file_request, buff);

			printf("** File Requested: |%s|\n", file_request);
			fflush(stdout);

			SendDataBin(file_request, newsock, myhome, content);
			shutdown(newsock, 1);
			close(newsock);
			exit(0);
    		}
		/*
		 * I am the Parent
		 */
		close(newsock);	/* Parent handed off this connection to its child,
			           doesn't care about this socket */
  	}
}
