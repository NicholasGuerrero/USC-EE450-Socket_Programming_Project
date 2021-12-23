#include "headers.h"
using namespace std;

#define Client_A_TCP_PORT "25452" //TCP Client A port
#define LOCALHOST "127.0.0.1"

#define MAXDATASIZE 512 // max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    double max_matching_gap = 1.79769e+308;
    double matching_gap;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(NULL, Client_A_TCP_PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    // If connection succeeded, display boot up message
    printf("The client is up and running \n");

    freeaddrinfo(servinfo); // all done with this structure

    ////
    // sending name to central server C
    if (send(sockfd, argv[1], strlen(argv[1]), 0) == -1)
    {
        perror("send");
    }
    printf("The client sent %s to the Central server \n", argv[1]);

    ////
    // recieve matching gap data from central server C
    if ((numbytes = recv(sockfd, &matching_gap, sizeof(matching_gap), 0)) == -1)
    {
        perror("recv");
        exit(1);
    }

    ///
    // recieve path data from central server C

    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }

    printf("%s", buf);

    if (max_matching_gap != matching_gap)
    {
        cout << ceil(matching_gap * 100.0) / 100.0 << endl;
    }

    printf("\n");

    close(sockfd);

    return 0;
}