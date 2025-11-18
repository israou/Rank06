#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}
int fds[700000],max_fd, fds_count = 0;
char *msgs[700000];
char notifMessage[42];
char buffer[1024];
fd_set readfds, writefds,global_fd;

void error(char *msg)
{
    write(2, msg, strlen(msg));
    exit(1);
}

void Notification(int fd, char *msg)
{
    for (int i = 0; i < max_fd + 1; i++)
    {
        if (FD_ISSET(i, &writefds) && i != fd)
            send(i, msg, strlen(msg), 0);
    }
}

void add_client(int fd)
{
    max_fd = fd > max_fd ? fd : max_fd;
    fds[fd] = fds_count++;
    FD_SET(fd, &global_fd);
    msgs[fd] = NULL;
    sprintf(notifMessage, "server: client %d just arriver\n", fds[fd]);
    Notification(fd, notifMessage);
}

void remove_client(int fd)
{
    sprintf(notifMessage, "server: client %d just left\n", fds[fd]);
    Notification(fd, notifMessage);
    FD_CLR(fd, &global_fd);
    close(fd);
    if (msgs[fd] != NULL) free(msgs[fd]);
    msgs[fd] = NULL;
}

void send_message(int fd)
{
    char *msg;
    while (extract_message(&msgs[fd], &msg))
    {
        sprintf(notifMessage, "client %d: ", fds[fd]);
        Notification(fd, notifMessage);
        Notification(fd, msg);
        free(msg);
    }
}

int main() {
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(8081); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n");
	if (listen(sockfd, 10) != 0) {
		printf("cannot listen\n"); 
		exit(0); 
	}
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len);
	if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n");
}