/*
 * Program totroll.c
 *
 * Testing program to test the "troll" (q.v.)
 * Sends messages via the troll to a another process and prints whatever
 * messages come back.  The other process is supposed to be fromtroll.c,
 * which just echos what it gets.
 */
#ifndef lint
static char *rcsid = "$Header: /var/home/solomon/640/troll/RCS/totroll.c,v 3.2 1991/04/13 22:38:01 solomon Distrib solomon $";
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>

extern int errno;

typedef struct MyMessage {
	struct sockaddr_in msg_header;
	long contents;
} MyMessage;

/* interval between message sends */
struct timeval timeout = {
	0L, /* seconds */
	10000L, /* microseconds */
};

struct timeval timeout_original;

int qflag=0;  /* quiet */

/* for lint */
void bzero(), bcopy(), exit(), perror();
double atof();
#define Printf if (!qflag) (void)printf
#define Fprintf (void)fprintf

main(argc,argv)
int argc;
char *argv[];
{

	int sock;	/* a socket for sending messages and receiving responses */
	MyMessage message;
	struct hostent *host;
	u_short port;
	struct sockaddr_in trolladdr, destaddr, localaddr, fromaddr;
	fd_set selectmask;
	int counter, n;
	int arg;

	/* process arguments */

	for (arg=1; arg<argc && argv[arg][0]=='-'; arg++) {
		char *p;
		for (p=argv[arg]+1; *p; p++) switch (*p) {
			case 'i': {
				double fsecs = 1.0;

				if (isdigit(p[1])) {
					fsecs = atof(p+1);
					p += strlen(p)-1;
				}
				else if (arg < argc-1 && isdigit(argv[arg+1][0])) {
					fsecs = atof(argv[++arg]);
				}
				else usage(argv[0]);
				timeout.tv_sec = fsecs;
				fsecs -= timeout.tv_sec;
				timeout.tv_usec = 1000000*fsecs;
				break;
			}
			case 'q': qflag++;
				break;
			default: usage(argv[0]);
		}
	}
					
	if (argc-arg != 4) usage(argv[0]);

	/* troll ... */

	if ((host = gethostbyname(argv[arg])) == NULL) {
		Fprintf(stderr, "%s: Unknown troll host '%s'\n",argv[0],argv[arg]);
		exit(1);
	}  

	port = atoi(argv[arg+1]);
	if (port < 1024 || port > 0xffff) {
		Fprintf(stderr, "%s: Bad troll port %d (must be between 1024 and %d)\n",
			argv[0], port, 0xffff);
		exit(1);
	}

	bzero ((char *)&trolladdr, sizeof trolladdr);
	trolladdr.sin_family = AF_INET;
	bcopy(host->h_addr, (char*)&trolladdr.sin_addr, host->h_length);
	trolladdr.sin_port = htons(port);

	/* destination ... */

	if ((host = gethostbyname(argv[arg+2])) == NULL) {
		Fprintf(stderr, "%s: Unknown troll host '%s'\n",argv[0],argv[arg+2]);
		exit(1);
	}  

	port = atoi(argv[arg+3]);
	if (port < 1024 || port > 0xffff) {
		Fprintf(stderr, "%s: Bad troll port %d (must be between 1024 and %d)\n",
			argv[0], port, 0xffff);
		exit(1);
	}

	bzero ((char *)&destaddr, sizeof destaddr);
	destaddr.sin_family = htons(AF_INET);
	bcopy(host->h_addr, (char*)&destaddr.sin_addr, host->h_length);
	destaddr.sin_port = htons(port);

	/* create a socket... */

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("totroll socket");
		exit(1);
	}
	FD_ZERO(&selectmask);
	FD_SET(sock, &selectmask);

	/* ... and bind its local address */
	bzero((char *)&localaddr, sizeof localaddr);
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr.s_addr = INADDR_ANY; /* let the kernel fill this in */
	localaddr.sin_port = 0;					/* let the kernel choose a port */
	if (bind(sock, (struct sockaddr *)&localaddr, sizeof localaddr) < 0) {
		perror("client bind");
		exit(1);
	}

	/* Main loop */

	counter = 0;
	message.contents = counter++;
	message.msg_header = destaddr; /* structure copy */
	errno = 0;
	n = sendto(sock, (char *)&message, sizeof message, 0,
					(struct sockaddr *)&trolladdr, sizeof trolladdr);
	if (n!=sizeof message) {
		perror("totroll sendto");
		exit(1);
	}

	timeout_original = timeout;
	for(;;) {
		int len = sizeof fromaddr;
		fd_set rmask;

		//printf("Before timeout: %ld %ld\n", timeout.tv_sec, timeout.tv_usec);

		rmask = selectmask;
		n = select(sock+1, &rmask, 0, 0, &timeout);
		if (n < 0) {
			perror("totroll select");
			exit(1);
		}
		if (FD_ISSET(sock, &rmask)) {
			/* read in one message from the troll */
			n = recvfrom(sock, (char *)&message, sizeof message, 0,
					(struct sockaddr *)&fromaddr, &len);
			if (n<0) {
				perror("totroll recvfrom");
				exit(1);
			}
			Printf("<<< %d %s\n",
				message.contents,
				message.contents==counter-1 ? "" : "???");
			continue;
		}
		/* timeout: send off another message to the troll */
		if (timeout.tv_sec == 0 &&  timeout.tv_usec==0){
			timeout = timeout_original;
			message.contents = counter++;
			message.msg_header = destaddr;
			errno = 0;
			Printf(">>> %d\n",message.contents);
			n = sendto(sock, (char *)&message, sizeof message, 0,
							(struct sockaddr *)&trolladdr, sizeof trolladdr);
			if (n!=sizeof message) {
				perror("totroll sendto");
				exit(1);
			}
		}
	}
} /* main */

usage(prog)
char *prog;
{
	Fprintf(stderr, "usage: %s [-i <seconds> ]", prog);
	Fprintf(stderr, " <trollhost> <trollport> <desthost> <destport>\n");
	exit(1);
}


