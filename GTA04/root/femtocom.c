/* femtocom.c - gcc -o femtocom femtocom.c */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

main(int argc, char *argv[])
{
	int fd;
	if(!argv[1])
		{
		fprintf(stderr, "usage: %s /dev/ttyHS? [nocrlf]\n", argv[0]);
		return 1;
		}
	fd=open(argv[1], O_RDWR);
	if(fd < 0)
		return 1;
	/* fixme: tcsetattr() for raw input mode with timer */
	while(1)
		{
		fd_set rfd, wfd, efd;
		FD_SET(0, &rfd);
		FD_SET(fd, &rfd);
		FD_SET(1, &wfd);
		FD_SET(fd, &wfd);
		if(select(fd+1, &rfd, &wfd, &efd, NULL) > 0)
			{
			char buf[1];
			if(FD_ISSET(0, &rfd) && FD_ISSET(fd, &wfd))
				{ /* echo stdin -> tty */
				int n=read(0, buf, 1);
				if(n < 0)
					return 2;	/* error */
				if(n == 0)
					return 0;	/* EOF */
				if(argc == 2 && buf[0] == '\n')
					{ /* send CRLF */
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
					return 3;	/* error */
				if(argc == 2 && buf[0] == '\r')
					continue;	/* ignore \r */
				if(n > 0)
					write(1, buf, n);
				}
			}
		}
}
