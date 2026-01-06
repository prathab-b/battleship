#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define MAX_EVENTS 1024
#define MAX_BUFFER 1024
#define MAX_NAME_LENGTH 20
#define MAX_CLIENTS 1024
#define MAX_BROADCAST_LENGTH 1024

typedef struct damaged_parts
{
    int x, y;
    int damaged;
} damaged_parts;

typedef struct client
{
    int sfd;
    char name[MAX_NAME_LENGTH];
    int registered;
    int ship_location[10][10];
    damaged_parts damaged[5];
} client;

client *clients[MAX_CLIENTS];
int client_count = 0;

void ignore_sigpipe(void)
{
  struct sigaction myaction;

  myaction.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &myaction, NULL);
}

int non_blocking_mode(int sfd)
{
    int flags;

    if ((flags = fcntl(sfd, F_GETFL)) == -1)
    {
        perror("fcntl");
        return -1;
    }

    return fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
}

void new_client(int cfd)
{
    if (client_count < MAX_CLIENTS)
    {
        clients[client_count] = (client *)calloc(1, sizeof(client));
        clients[client_count]->sfd = cfd;
        client_count++;
    }

    else
    {
        perror("new_client, too many clients");
    }
}

int find_client(int cfd)
{
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i]->sfd == cfd)
        {
            return i;
        }
    }

    return -1;
}

void disconnect_client(int cfd, int e);

void broadcast(char message[MAX_BROADCAST_LENGTH], int e)
{
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i]->registered == 1)
        {
            if (write(clients[i]->sfd, message, strlen(message)) == -1)
            {
                disconnect_client(clients[i]->sfd, e);
            }
        }
    }
}

void disconnect_client(int cfd, int e)
{
    int client_num = find_client(cfd);

    if (client_num == -1)
    {
        return;
    }

    if (clients[client_num]->registered == 1)
    {
        char message[MAX_BROADCAST_LENGTH];
        snprintf(message, sizeof(message), "GG %s\n", clients[client_num]->name);
        broadcast(message, e);
    }

    clients[client_num]->registered = 0;
    free(clients[client_num]);

    if (client_num != client_count - 1)
    {
        clients[client_num] = clients[client_count - 1];
    }

    client_count--;

    epoll_ctl(e, EPOLL_CTL_DEL, cfd, NULL);

    close(cfd);
}

void ship_preset(int client_num, int x, int y, char d)
{
    if (d == '-')
    {
        clients[client_num]->ship_location[x - 2][y] = 1;
        clients[client_num]->ship_location[x - 1][y] = 1;
        clients[client_num]->ship_location[x][y] = 1;
        clients[client_num]->ship_location[x + 1][y] = 1;
        clients[client_num]->ship_location[x + 2][y] = 1;

        clients[client_num]->damaged[0].x = x - 2;
        clients[client_num]->damaged[0].y = y;
        clients[client_num]->damaged[1].x = x - 1;
        clients[client_num]->damaged[1].y = y;
        clients[client_num]->damaged[2].x = x;
        clients[client_num]->damaged[2].y = y;
        clients[client_num]->damaged[3].x = x + 1;
        clients[client_num]->damaged[3].y = y;
        clients[client_num]->damaged[4].x = x + 2;
        clients[client_num]->damaged[4].y = y;
    }

    else
    {
        clients[client_num]->ship_location[x][y - 2] = 1;
        clients[client_num]->ship_location[x][y - 1] = 1;
        clients[client_num]->ship_location[x][y] = 1;
        clients[client_num]->ship_location[x][y + 1] = 1;
        clients[client_num]->ship_location[x][y + 2] = 1;

        clients[client_num]->damaged[0].x = x;
        clients[client_num]->damaged[0].y = y - 2;
        clients[client_num]->damaged[1].x = x;
        clients[client_num]->damaged[1].y = y - 1;
        clients[client_num]->damaged[2].x = x;
        clients[client_num]->damaged[2].y = y;
        clients[client_num]->damaged[3].x = x;
        clients[client_num]->damaged[3].y = y + 1;
        clients[client_num]->damaged[4].x = x;
        clients[client_num]->damaged[4].y = y + 2;
    }
}

void interpret_message(char buffer[MAX_BUFFER], int cfd, int ep)
{
    int client_num = find_client(cfd);
    char message[MAX_BROADCAST_LENGTH];

    if (client_num == -1)
    {
        return;
    }

    if (strncmp(buffer, "REG", 3) == 0)
    {
        int x, y;
        char d;
        char name[MAX_NAME_LENGTH];
        int success = 0;

        if (sscanf(buffer, "REG %20s %d %d %c\n", name, &x, &y, &d) == 4)
        {
            if (d != '-' && d != '|')
            {
                if (write(clients[client_num]->sfd, "INVALID\n", 8) == -1)
                {
                    disconnect_client(clients[client_num]->sfd, ep);
                }

                return;
            }

            else if (x < 0 || x > 9 || y < 0 || y > 9)
            {
                if (write(clients[client_num]->sfd, "INVALID\n", 8) == -1)
                {
                    disconnect_client(clients[client_num]->sfd, ep);
                }

                return;
            }

            else if (d == '-' && (x < 2 || x > 7))
            {
                if (write(clients[client_num]->sfd, "INVALID\n", 8) == -1)
                {
                    disconnect_client(clients[client_num]->sfd, ep);
                }

                return;
            }

            else if (d == '|' && (y < 2 || y > 7))
            {
                if (write(clients[client_num]->sfd, "INVALID\n", 8) == -1)
                {
                    disconnect_client(clients[client_num]->sfd, ep);
                }

                return;
            }

            else
            {
                for (int i = 0; i < client_count; i++)
                {
                    if (strcmp(name, clients[i]->name) == 0)
                    {
                        if (write(clients[client_num]->sfd, "TAKEN\n", 6) == -1)
                        {
                            disconnect_client(clients[client_num]->sfd, ep);
                        }

                        return;
                    }
                }
            }
        }

        else
        {
            if (write(clients[client_num]->sfd, "INVALID\n", 8) == -1)
            {
                disconnect_client(clients[client_num]->sfd, ep);
            }

            return;
        }

        memset(clients[client_num]->ship_location, 0, sizeof(clients[client_num]->ship_location));
        memset(clients[client_num]->damaged, 0, sizeof(clients[client_num]->damaged));

        strcpy(clients[client_num]->name, name);

        ship_preset(client_num, x, y, d);

        if (write(clients[client_num]->sfd, "WELCOME\n", 8) == -1)
        {
            disconnect_client(clients[client_num]->sfd, ep);
            return;
        }

        clients[client_num]->registered = 1;

        snprintf(message, sizeof(message), "JOIN %s\n", name);
        broadcast(message, ep);
    }

    else if (strncmp(buffer, "BOMB", 4) == 0)
    {
        int x, y;
        int hit_status = 0;
        int ship_destroyed = 0;

        if (sscanf(buffer, "BOMB %d %d\n", &x, &y) != 2)
        {
            if (write(clients[client_num]->sfd, "INVALID\n", 8) == -1)
            {
                disconnect_client(clients[client_num]->sfd, ep);
                return;
            }
        }

        if (clients[client_num]->registered == 0)
        {
            return;
        }

        else
        {
            if (x >= 0 && x < 10 && y >= 0 && y < 10)
            {
                hit_status = 0;

                for (int i = 0 ; i < client_count; i++)
                {
                    ship_destroyed = 1;

                    if (clients[i]->ship_location[x][y] == 1 && strcmp(clients[i]->name, clients[client_num]->name) != 0)
                    {
                        char message[MAX_BROADCAST_LENGTH];
                        hit_status = 1;

                        snprintf(message, sizeof(message), "HIT %s %d %d %s\n", clients[client_num]->name, x, y, clients[i]->name);
                        broadcast(message, ep);

                        for (int j = 0; j < 5; j++)
                        {
                            if (clients[i]->damaged[j].x == x && clients[i]->damaged[j].y == y)
                            {
                                clients[i]->damaged[j].damaged = 1;
                            }
                        }
                    }

                    for (int j = 0; j < 5; j++)
                    {
                        if (clients[i]->damaged[j].damaged != 1)
                        {
                            ship_destroyed = 0;
                            break;
                        }
                    }

                    if (ship_destroyed == 1)
                    {
                        disconnect_client(clients[i]->sfd, ep);
                    }
                }
            }

            if (hit_status == 0)
            {
                snprintf(message, sizeof(message), "MISS %s %d %d\n", clients[client_num]->name, x, y);
                broadcast(message, ep);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int sfd;
    struct sockaddr_in a;

    ignore_sigpipe();

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&a, 0, sizeof(struct sockaddr_in));

    a.sin_family = AF_INET;
    a.sin_port = htons(atoi(argv[1]));
    a.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sfd, (struct sockaddr *)&a, sizeof(struct sockaddr_in)) == -1)
    {
        perror("bind");
        return -1;
    }

    if (listen(sfd, 2) == -1)
    {
        perror("listen");
        return -1;
    }

    if (non_blocking_mode(sfd) == -1)
    {
        perror("non_blocking_mode");
        return -1;
    }

    int ep;
    struct epoll_event e;

    if ((ep = epoll_create1(0)) == -1)
    {
        perror("epoll_create1");
        return -1;
    }

    e.events = EPOLLIN;
    e.data.fd = sfd;

    if (epoll_ctl(ep, EPOLL_CTL_ADD, sfd, &e) == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    for (;;)
    {
        int n;
        struct epoll_event es[MAX_EVENTS];

        perror("calling epoll_wait\n"); // REMOVE, FOR TEST

        n = epoll_wait(ep, es, MAX_EVENTS, -1);

        for (int i = 0; i < n; i++)
        {
            if (es[i].events & EPOLLHUP)
            {
                disconnect_client(es[i].data.fd, ep);
            }

            else
            {
                if (es[i].data.fd == sfd)
                {
                    int cfd;
                    struct sockaddr_in ca;
                    socklen_t sinlen;

                    sinlen = sizeof(struct sockaddr_in);
                    if ((cfd = accept(sfd, (struct sockaddr *)&ca, &sinlen)) != -1)
                    {
                        if (non_blocking_mode(cfd) == -1)
                        {
                            perror("non_blocking_mode, client");
                            return -1;
                        }

                        e.events = EPOLLIN | EPOLLET;
                        e.data.fd = cfd;
                        epoll_ctl(ep, EPOLL_CTL_ADD, cfd, &e);

                        new_client(cfd);
                    }

                    else
                    {
                        perror("accept");
                    }
                }

                else
                {
                    char buffer[MAX_BUFFER];
                    int num_read = read(es[i].data.fd, buffer, MAX_BUFFER);
                    int num_chars = 0;
                    int client_found = 0;

                    for (int j = 0; j < client_count; j++)
                    {
                        if (clients[j]->sfd == es[i].data.fd)
                        {
                            client_found = 1;
                        }
                    }

                    if (client_found == 1)
                    {
                        if (num_read > 0)
                        {
                            while (buffer[num_chars] != '\n' && num_chars < 100)
                            {
                                num_chars++;
                            }

                            if (num_chars < 100 || buffer[99] == '\n')
                            {
                                buffer[num_chars] = '\0';

                                interpret_message(buffer, es[i].data.fd, ep);
                            }

                            else
                            {
                                disconnect_client(es[i].data.fd, ep);
                            }
                        }

                        else
                        {
                            disconnect_client(es[i].data.fd, ep);
                        }
                    }
                }
            }
        }
    }
}
