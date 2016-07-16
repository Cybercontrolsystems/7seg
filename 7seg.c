/* 7-segment driver for trailer */

#include <stdio.h>      // for FILE
#include <stdlib.h>     // for timeval
#include <string.h>     // for strlen etc
#include <time.h>       // for ctime
#include <sys/types.h>  // for fd_set
#include <netdb.h>      // for sockaddr_in 
#include <fcntl.h>      // for O_RDWR
#include <termios.h>    // for termios
#include <getopt.h>             // for getopt
#ifdef linux
#include <errno.h>              // for Linux
#endif

/* Version 0.0 24/09/2008 Created by copying from Froniustest */

#define VERSION "0.0"

#define SERIALNAME "/dev/ttyAM0"        /* although it can be supplied on command line */
// Serial retry params
#define SERIALNUMRETRIES 10
#define SERIALRETRYDELAY 1000000 /*microseconds */
#define WAITTIME 3      /*seconds*/
// Set to if(0) to disable debugging
#define DEBUG if(debug)
#define DEBUG2 if(debug > 1)
#define DEBUGFP stderr   /* set to stderr or logfp as required */

// This allows use of stdio instead of the serial device, and you can type in the hex value
// #define DEBUGCOMMS

#ifndef linux
extern
#endif
int errno; 

// Procedures used
int openSerial(const char * name, int baud, int parity, int databits, int stopbits);  // return fd
void closeSerial(int fd);  // restore terminal settings
int sendSerial(int fd, unsigned char data);     // Send a byte
void usage(void);                                               // Standard usage message
void init_table(char *t);

// Globals
int debug = 0;

/********/
/* MAIN */
/********/
int main(int argc, char *argv[])
// arg1: serial device file
// arg2: optional timeout in seconds, default 60
// arg3: optional 'nolog' to carry on when filesystem full
{
    int commfd = 0;
	int option;                             // command line processing
	int baud = 9600;
	char * cp;
	char table[128];
	int flags = CS8 | CLOCAL | CREAD;
	
    char * serialName = SERIALNAME;
	
	memset(table, 0, 128);
	// Command line arguments
	
	init_table(table);
	opterr = 0;
	while ((option = getopt(argc, argv, "b:dVcs:?")) != -1) {
		DEBUG fprintf(stderr, "Option %c ", option);
		switch (option) {
			case 'b': baud = atoi(optarg); break;
			case 'c': flags |= CRTSCTS; break;
			case '?': usage(); exit(1);
			case 's': serialName = optarg; break;
			case 'd': debug++; break;
			case 'V': printf("Version: %s\n", VERSION); exit(0);
		}
	}
	
	// Open serial port
#ifdef DEBUGCOMMS
	commfd = 0;
#else
	if ((commfd = openSerial(serialName, baud, 0, flags, 1)) < 0) {
		fprintf(stderr, "ERROR 7seg Failed to open %s at %d: %s", serialName, baud, strerror(errno));
		exit(1);
	}
	
#endif
	DEBUG2 fprintf(stderr, "Serial port %s open ...", serialName);
	
	// Send data
	while (optind < argc) {		// remainder of command line must be bytes
		for (cp = argv[optind]; *cp; cp++) {
				DEBUG fprintf(stderr, "'%c' [%02x] ", *cp, table[*cp]);
			sendSerial(commfd, table[*cp]);
		}
		optind++;
	}
	DEBUG fprintf(stderr, "\n");
	
	closeSerial(commfd);
	
	return 0;
}

/*********/
/* USAGE */
/*********/
void usage(void) {
        printf("Usage: 7seg [-b 2400|9600|19200] [-s serialdev] [-fcV] hexdata \n");
        printf(" -d: debug -c use CTS/RTS -V version\n");
        return;
}

/**************/
/* SENDSERIAL */
/**************/
int sendSerial(int fd, unsigned char data) {
	// Send a single byte.  Return 1 for a logged failure
	int retries = SERIALNUMRETRIES;
	int written;
#ifdef DEBUGCOMMS
	fprintf(stderr, "Comm 0x%02x(%d) ", data, data);
	return 0;
#endif
	
	DEBUG fprintf(stderr, "[%02x]>> ", data);
	while ((written = write(fd, &data, 1)) < 1) {
        fprintf(DEBUGFP, "Serial wrote %d bytes errno = %d", written, errno);
        perror("");
		if (--retries == 0) {
			fprintf(stderr, "WARN 7seg timed out writing to serial port");
			return 1;
		}
		DEBUG fprintf(DEBUGFP, "Pausing %d ... ", SERIALRETRYDELAY);
		usleep(SERIALRETRYDELAY);
	}
	return 0;       // ok
}

/**************/
/* OPENSERIAL */
/**************/
int openSerial(const char * name, int baud, int parity, int cflag, int stopbits) {
	/* open serial device; return file descriptor or -1 for error (see errno) */
	int fd, res, speed;
	struct termios newSettings;
	
	if ((fd = open(name, O_RDWR | O_NOCTTY)) < 0) return fd;        // an error code
	
	bzero(&newSettings, sizeof(newSettings));
	// Control Modes
	newSettings.c_cflag = cflag; // Shouldn't really set baud like this
	
	if (stopbits == 2) 
		newSettings.c_cflag |= CSTOPB;
	// input modes
	newSettings.c_iflag = IGNPAR;   //input modes
	newSettings.c_oflag = 0;                // output modes
	newSettings.c_lflag = 0;                // local flag
	newSettings.c_cc[VTIME] = 0; // intercharacter timer */
    newSettings.c_cc[VMIN] = 0;     // non-blocking read */
	
	tcflush(fd, TCIFLUSH);          // discard pending data
	if (baud)	{
		switch(baud) {
			case 2400: speed = B2400; break;
			case 9600: speed = B9600; break;
			case 19200: speed = B19200; break;
			default: fprintf(stderr, "Error - Baud must be 2400, 9600 or 19200 - %d supplied\n", baud);
		}
 	if (cfsetspeed(&newSettings, speed))
		perror("Setting serial port");
	}
	//      cfsetospeed(&newSettings, baud);
	if((res = tcsetattr(fd, TCSANOW, &newSettings)) < 0) {
		close(fd);      // if there's an error setting values, return the error code
		return res;
	}
	return fd;
}

/***************/
/* CLOSESERIAL */
/***************/
void closeSerial(int fd) {
	close(fd);
}

void init_table(char *t) {
// Initialise the 7-segment lookup table
	t['0'] = 0x3F;
	t['1'] = 0x06;
	t['2'] = 0x5B;
	t['3'] = 0x4F;
	t['4'] = 0x66;
	t['5'] = 0x6D;
	t['6'] = 0x7d;
	t['7'] = 0x07;
	t['8'] = 0x7F;
	t['9'] = 0x6F;
	t[' '] = 0;
	t['-'] = 0x40;
	t['A'] = 0x77;
	t['C'] = 0x39;
	t['d'] = 0x5E;
	
}


	