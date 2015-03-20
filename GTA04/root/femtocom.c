/* femtocom.c - gcc -o femtocom femtocom.c */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>

main(int argc, char *argv[])
{
	int fd;
	int sendcrlf=0;
	int passcr=0;
	struct termios tc;
	char *arg0=argv[0];
	while(argv[1] && argv[1][0] == '-') {
		if(strcmp(argv[1], "-s") == 0) sendcrlf=1;
		else if(strcmp(argv[1], "-r") == 0) passcr=1;
		else (argv--)[1]=NULL;
		argv++;
	}
	if(!argv[1]) {
		fprintf(stderr, "usage: %s [-s] [-r] /dev/ttyHS?\n", arg0);
		fprintf(stderr, "  -s send \\n as \\n and not as CRLF\n");
		fprintf(stderr, "  -r pass \\r and don't ignore\n");
		return 1;
	}
	fd=open(argv[1], O_RDWR);
	if(fd < 0) {
		perror("open");		return 1;
	}
	if(tcgetattr(fd, &tc) < 0) {		perror("tcgetattr");		return 1;	}  	tc.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CSIZE);
	tc.c_cflag |= CS8;
	tc.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | INPCK | ISIG);
	tc.c_iflag |= IGNBRK | IGNPAR | ICRNL | INLCR;	tc.c_oflag &= ~OPOST;
	tc.c_cc[VMIN]	= 1;	tc.c_cc[VTIME]	=0;	if(tcsetattr(fd, TCSANOW, &tc) < 0) { /* tried to modify */		perror("tcsetattr");		return 1;	}	while(1) {
		fd_set rfd, wfd, efd;
		FD_SET(0, &rfd);
		FD_SET(fd, &rfd);
		FD_SET(1, &wfd);
		FD_SET(fd, &wfd);
		if(select(fd+1, &rfd, &wfd, &efd, NULL) > 0)
			{ // wait for input from either end and forward to the other
			char buf[1];
			if(FD_ISSET(0, &rfd) && FD_ISSET(fd, &wfd))
				{ /* echo stdin -> tty */
				int n=read(0, buf, 1);
				if(n < 0)
					return 2;	/* read error */
				if(n == 0)
					return 0;	/* EOF */
				if(!sendcrlf && buf[0] == '\n')
					{ /* send \n as CRLF */
					buf[0]='\r';
					write(fd, buf, n);
					buf[0]='\n';
					}
				write(fd, buf, n);
				}
			if(FD_ISSET(fd, &rfd) && FD_ISSET(1, &wfd))
				{ /* echo tty -> stdout */
				int n=read(fd, buf, 1);
				if(n < 0)
					return 3;	/* read error */
				if(!passcr && buf[0] == '\r')
					continue;	/* ignore received \r and take \n only */
				if(n > 0)
					write(1, buf, n);
				}
			}
	}
}
