#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h> //Add
#include <stdlib.h>//add
#include <sys/select.h>//add

typedef struct s_client
{
    int id;
    char msg[1000000];
} t_client;

t_client clients[1024];
char buffRead[1000000], buffWrite[1000000];
int max_fd = 0, next_id = 0;
fd_set active_fds, read_fds, write_fds;

void exitError(char *str)
{
    write(2, str, strlen(str));
    exit(1);
}

void sendMsg(int sender_fd)
{
    for (int fd = 0; fd <= max_fd; fd++)
        if (FD_ISSET(fd, &write_fds) && fd != sender_fd)
            send(fd, buffWrite, strlen(buffWrite), 0);
}

int main(int ac, char **av)
{
    if (ac != 2)
        exitError("Wrong number of arguments\n");
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        exitError("Fatal error\n");
    max_fd = sockfd;
    FD_ZERO(&active_fds);
    FD_SET(sockfd, &active_fds);
    struct sockaddr_in sadd;
    memset(&sadd, 0, sizeof(sadd));
    sadd.sin_family = AF_INET;
    sadd.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
    sadd.sin_port = htons(atoi(av[1]));
    if (bind(sockfd, (const struct sockaddr *)&sadd, sizeof(sadd)) != 0)
        exitError("Fatal error\n");
    if (listen(sockfd, 10) != 0)
        exitError("Fatal error\n");
    while (1)
    {
        read_fds = write_fds = active_fds;
        if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0)
            continue;
        for (int fd = 0; fd <= max_fd; fd++)
        {
            if (FD_ISSET(fd, &read_fds) && fd == sockfd)
            {
                int cl_sock = accept(sockfd, NULL, NULL);
                if (cl_sock < 0)
                    continue;
                if (cl_sock > max_fd)
                    max_fd = cl_sock;
                clients[cl_sock].id = next_id++;
                bzero(clients[cl_sock].msg, sizeof(clients[cl_sock].msg));
                FD_SET(cl_sock, &active_fds);
                sprintf(buffWrite, "server: client %d just arrived\n", clients[cl_sock].id);
                sendMsg(cl_sock);
                break;
            }
            else if (FD_ISSET(fd, &read_fds))
            {
                int read_bytes = recv(fd, buffRead, sizeof(buffRead) - 1, 0);
                if (read_bytes <= 0)
                {
                    sprintf(buffWrite, "server: client %d just left\n", clients[fd].id);
                    sendMsg(fd);
                    FD_CLR(fd, &active_fds);
                    close(fd);
                    break;
                }
                else
                {
                    buffRead[read_bytes] = '\0';
                    for (int i = 0, j = strlen(clients[fd].msg); i < read_bytes && (size_t)j < sizeof(clients[fd].msg) - 1; i++, j++)
                    {
                        clients[fd].msg[j] = buffRead[i];
                        if (clients[fd].msg[j] == '\n')
                        {
                            clients[fd].msg[j] = '\0';
                            sprintf(buffWrite, "client %d: %.999980s\n", clients[fd].id, clients[fd].msg);
                            sendMsg(fd);
                            bzero(clients[fd].msg, sizeof(clients[fd].msg));
                            j = -1;
                        }
                    }
                }
            }
        }
    }
}
