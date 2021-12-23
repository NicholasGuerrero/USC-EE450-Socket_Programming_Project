#include "headers.h"
using namespace std;

#define ClIENT_A_TCP_PORT "25452" //TCP Client A port
#define ClIENT_B_TCP_PORT "26452" //TCP Client B port
#define SERVER_T_UDP_PORT "21452" //UDP Server T port
#define SERVER_S_UDP_PORT "22452" //UDP Server S port
#define SERVER_P_UDP_PORT "23452" //UDP Server P port

#define SERVER_C_UDP_PORT "24452" //UDP Server C port

#define BACKLOG 10            // how many pending connections queue will hold
#define LOCALHOST "127.0.0.1" // Host address
#define MAXDATASIZE 512       // max number of bytes we can get at once
#define MAXBUFLEN 512

string convertToString(char *a, int size)
{
    int i;
    string s = "";
    for (i = 0; i < size; i++)
    {
        if (a[i] == '\0')
        {
            return s;
        }
        s = s + a[i];
    }
    return s;
}

void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/**
 * Step 1: Create TCP socket for clientA & bind socket
 */
int create_client_A_socket_and_bind()
{
    int sockfd_clientA; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints_clientA, *servinfo_clientA, *p_clientA;
    int yes = 1;
    int rv;

    memset(&hints_clientA, 0, sizeof hints_clientA);
    hints_clientA.ai_family = AF_UNSPEC;
    hints_clientA.ai_socktype = SOCK_STREAM;
    hints_clientA.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, ClIENT_A_TCP_PORT, &hints_clientA, &servinfo_clientA)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p_clientA = servinfo_clientA; p_clientA != NULL; p_clientA = p_clientA->ai_next)
    {
        if ((sockfd_clientA = socket(p_clientA->ai_family, p_clientA->ai_socktype,
                                     p_clientA->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd_clientA, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd_clientA, p_clientA->ai_addr, p_clientA->ai_addrlen) == -1)
        {
            close(sockfd_clientA);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo_clientA); // all done with this structure

    if (p_clientA == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    return sockfd_clientA;
}

/**
 * Step 2: Listen for incoming connection from client A
 */
void listen_clientA(int sockfd_clientA)
{
    struct sigaction sa_clientA;

    if (listen(sockfd_clientA, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa_clientA.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa_clientA.sa_mask);
    sa_clientA.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa_clientA, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
}

/**
 * Step 3: Create TCP socket for clientB & bind socket
 */
int create_client_B_socket_and_bind()
{
    int sockfd_clientB;
    int rv_clientB;
    int yes_clientB = 1;
    struct addrinfo hints_clientB, *servinfo_clientB, *p_clientB;

    memset(&hints_clientB, 0, sizeof hints_clientB);
    hints_clientB.ai_family = AF_UNSPEC;
    hints_clientB.ai_socktype = SOCK_STREAM;
    hints_clientB.ai_flags = AI_PASSIVE; // use my IP

    if ((rv_clientB = getaddrinfo(NULL, ClIENT_B_TCP_PORT, &hints_clientB, &servinfo_clientB)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_clientB));
    }

    // loop through all the results and bind to the first we can
    for (p_clientB = servinfo_clientB; p_clientB != NULL; p_clientB = p_clientB->ai_next)
    {
        if ((sockfd_clientB = socket(p_clientB->ai_family, p_clientB->ai_socktype,
                                     p_clientB->ai_protocol)) == -1)
        {
            perror("server: socket");
        }

        if (setsockopt(sockfd_clientB, SOL_SOCKET, SO_REUSEADDR, &yes_clientB,
                       sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd_clientB, p_clientB->ai_addr, p_clientB->ai_addrlen) == -1)
        {
            close(sockfd_clientB);
            perror("server: bind");
        }
        break;
    }
    freeaddrinfo(servinfo_clientB); // all done with this structure

    if (p_clientB == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    return sockfd_clientB;
}

/**
 * Step 4: Listen for incoming connection from client B
 */
void listen_clientB(int sockfd_clientB)
{
    struct sigaction sa_clientB;

    if (listen(sockfd_clientB, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa_clientB.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa_clientB.sa_mask);
    sa_clientB.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa_clientB, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
}

/**
 * Step 5 send clientA name and client B name
 */
void send_client_names(int sockfd_UDP, string &clientA_name, string &clientB_name, struct addrinfo *p_UDP)
{
    int numbytes_UDP;
    /////
    //sending 1st client name to serverT
    if ((numbytes_UDP = sendto(sockfd_UDP, clientA_name.c_str(), strlen(clientA_name.c_str()), 0,
                               p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    /////
    //sending 2nd client name to serverT
    if ((numbytes_UDP = sendto(sockfd_UDP, clientB_name.c_str(), strlen(clientB_name.c_str()), 0,
                               p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }
    /////
}

/**
 * Step 6 send subgraph to serverS
 */
void send_subgraph(int sockfd_UDP, string &subgraph, struct addrinfo *p_UDP)
{
    int numbytes_UDP;
    int bytes_to_send = strlen(subgraph.c_str());
    int bytes_sent = 0;
    stringstream ss;

    ss << bytes_to_send;

    //send amount of bytes to send
    if ((numbytes_UDP = sendto(sockfd_UDP, ss.str().c_str(), MAXBUFLEN - 1, 0,
                               p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }
    /////
    //sending subgraph to serverS
    while (bytes_sent < bytes_to_send)
    {
        int end_slice = MAXBUFLEN - 1;
        string response = subgraph.substr(bytes_sent, MAXBUFLEN - 1).c_str();

        if ((numbytes_UDP = sendto(sockfd_UDP, response.c_str(), MAXBUFLEN - 1, 0,
                                   p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
        {
            perror("talker: sendto");
            exit(1);
        }
        bytes_sent += numbytes_UDP;
        end_slice += end_slice;
    }
}

/**
 * Step 7 send subgraph and scores to serverP
 */
void send_subgraph_and_scores(int sockfd_UDP, string &subgraph, string &scores, struct addrinfo *p_UDP)
{
    int numbytes_UDP;

    ///// scores
    int bytes_to_send_scores = strlen(scores.c_str());
    int bytes_sent_scores = 0;
    stringstream ss_scores;

    ss_scores << bytes_to_send_scores;

    if ((numbytes_UDP = sendto(sockfd_UDP, ss_scores.str().c_str(), MAXBUFLEN - 1, 0,
                               p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    /////
    //sending scores to serverP
    while (bytes_sent_scores < bytes_to_send_scores)
    {
        int end_slice = MAXBUFLEN - 1;
        string response = scores.substr(bytes_sent_scores, MAXBUFLEN - 1).c_str();

        if ((numbytes_UDP = sendto(sockfd_UDP, response.c_str(), MAXBUFLEN - 1, 0,
                                   p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
        {
            perror("talker: sendto");
            exit(1);
        }
        bytes_sent_scores += numbytes_UDP;
        end_slice += end_slice;
    }

    /// subgraph portion

    int bytes_to_send_subgraph = strlen(subgraph.c_str());
    int bytes_sent_subgraph = 0;
    stringstream ss_subgraph;

    ss_subgraph << bytes_to_send_subgraph;

    if ((numbytes_UDP = sendto(sockfd_UDP, ss_subgraph.str().c_str(), MAXBUFLEN - 1, 0,
                               p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    /////
    //sending subgraph to serverP
    while (bytes_sent_subgraph < bytes_to_send_subgraph)
    {
        int end_slice = MAXBUFLEN - 1;
        string response_subgraph = subgraph.substr(bytes_sent_subgraph, MAXBUFLEN - 1).c_str();

        if ((numbytes_UDP = sendto(sockfd_UDP, response_subgraph.c_str(), MAXBUFLEN - 1, 0,
                                   p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
        {
            perror("talker: sendto");
            exit(1);
        }
        bytes_sent_subgraph += numbytes_UDP;
        end_slice += end_slice;
    }
}

/**
 * Step 5, 6, 7: Create UDP socket for Server T, S, P. Server C acts as a client
 */
void create_UDP_socket_and_send_request(const char *service_UDP,
                                        string &clientA_name, string &clientB_name, string &subgraph, string &scores)
{
    int sockfd_UDP;
    struct addrinfo hints_UDP, *servinfo_UDP, *p_UDP;
    int rv_UDP;
    int numbytes_UDP;

    memset(&hints_UDP, 0, sizeof hints_UDP);
    hints_UDP.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints_UDP.ai_socktype = SOCK_DGRAM;

    if ((rv_UDP = getaddrinfo(NULL, service_UDP, &hints_UDP, &servinfo_UDP)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_UDP));
        exit(1);
    }

    // loop through all the results and make a socket
    for (p_UDP = servinfo_UDP; p_UDP != NULL; p_UDP = p_UDP->ai_next)
    {
        if ((sockfd_UDP = socket(p_UDP->ai_family, p_UDP->ai_socktype,
                                 p_UDP->ai_protocol)) == -1)
        {
            perror("talker: socket");
            continue;
        }
        break;
    }

    if (p_UDP == NULL)
    {
        fprintf(stderr, "talker: failed to create socket\n");
        exit(1);
    }

    // if data is sent to serverT
    if (strcmp(service_UDP, SERVER_T_UDP_PORT) == 0)
    {
        send_client_names(sockfd_UDP, clientA_name, clientB_name, p_UDP);
        printf("The Central server sent a request to Backend-Server T.\n");
    }

    // if data is sent to serverS
    if (strcmp(service_UDP, SERVER_S_UDP_PORT) == 0)
    {
        send_subgraph(sockfd_UDP, subgraph, p_UDP);
        printf("The Central server sent a request to Backend-Server S.\n");
    }

    // if data is sent to serverP
    if (strcmp(service_UDP, SERVER_P_UDP_PORT) == 0)
    {
        send_subgraph_and_scores(sockfd_UDP, subgraph, scores, p_UDP);
        printf("The Central server sent a processing request to Backend-Server P.\n");
    }

    freeaddrinfo(servinfo_UDP);

    close(sockfd_UDP);
}

void create_UDP_socket_and_recieve_data(const char *service_UDP, string &subgraph, string &scores,
                                        string &min_gap_pathA, string &min_gap_pathB, double &compatibility_score)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buffer[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, SERVER_C_UDP_PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "listener: failed to bind socket\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    addr_len = sizeof their_addr;

    // if data is recieved from serverT
    if (strcmp(service_UDP, SERVER_T_UDP_PORT) == 0)
    {

        int bytes_to_recieve;
        int bytes_recieved = 0;

        if ((numbytes = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        bytes_to_recieve = atoi(convertToString(buffer, sizeof(buffer)).c_str());

        memset(&buffer, 0, sizeof buffer);

        while (bytes_recieved < bytes_to_recieve)
        {
            //recieve subgraph
            if ((numbytes = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0,
                                     (struct sockaddr *)&their_addr, &addr_len)) == -1)
            {
                perror("recvfrom");
                exit(1);
            }

            bytes_recieved += MAXBUFLEN - 1;
            buffer[numbytes] = '\0';

            subgraph += convertToString(buffer, sizeof(buffer));
            memset(&buffer, 0, sizeof buffer);
        }

        printf("The Central server received information from Backend-Server T using UDP over port %s.\n", SERVER_C_UDP_PORT);

        //clear buffer
        memset(&buffer, 0, sizeof(buffer));
    }

    // if data is recieved from serverS
    if (strcmp(service_UDP, SERVER_S_UDP_PORT) == 0)
    {
        int bytes_to_recieve;
        int bytes_recieved = 0;

        if ((numbytes = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        bytes_to_recieve = atoi(convertToString(buffer, sizeof(buffer)).c_str());

        memset(&buffer, 0, sizeof buffer);

        while (bytes_recieved < bytes_to_recieve)
        {

            //recieve scores
            if ((numbytes = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0,
                                     (struct sockaddr *)&their_addr, &addr_len)) == -1)
            {
                perror("recvfrom");
                exit(1);
            }

            bytes_recieved += MAXBUFLEN - 1;
            buffer[numbytes] = '\0';

            scores += convertToString(buffer, sizeof(buffer));
            memset(&buffer, 0, sizeof buffer);
        }

        printf("The Central server received information from Backend-Server S using UDP over port %s.\n", SERVER_C_UDP_PORT);

        //clear buffer
        memset(&buffer, 0, sizeof(buffer));
    }

    // if data is recieved from serverP
    if (strcmp(service_UDP, SERVER_P_UDP_PORT) == 0)
    {
        //recieve min_gap_pathA
        if ((numbytes = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        buffer[numbytes] = '\0';

        min_gap_pathA = convertToString(buffer, sizeof(buffer));

        //recieve min_gap_pathB
        if ((numbytes = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        buffer[numbytes] = '\0';

        min_gap_pathB = convertToString(buffer, sizeof(buffer));

        //clear buffer
        memset(&buffer, 0, sizeof(buffer));

        //recieve compatibility_score
        if ((numbytes = recvfrom(sockfd, &compatibility_score, sizeof(compatibility_score), 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        printf("The Central server received the results from backend server P\n");
    }

    close(sockfd);
}

int main(void)
{
    struct sockaddr_storage their_addr; // connector's address information
    char s[INET6_ADDRSTRLEN];
    socklen_t sin_size;
    int sockfd_clientA, new_fd_clientA;
    int sockfd_clientB, new_fd_clientB;
    int numbytes_clientA, numbytes_clientB;
    char buf_clientA[MAXDATASIZE], buf_clientB[MAXDATASIZE];
    string subgraph = "";
    string scores = "";
    string clientA_name, clientB_name;
    string min_gap_pathA, min_gap_pathB;
    double compatibility_score;
    string clientA_return_data = "", clientB_return_data = "";
    double max_matching_gap = 1.79769e+308;

    sockfd_clientA = create_client_A_socket_and_bind();
    listen_clientA(sockfd_clientA);
    sockfd_clientB = create_client_B_socket_and_bind();
    listen_clientB(sockfd_clientB);

    printf("The Central server is up and running.\n");

    while (1)
    {
        sin_size = sizeof their_addr;

        /////////// Phase 1A //////////////

        /////////// Client A //////////////

        new_fd_clientA = accept(sockfd_clientA, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd_clientA == -1)
        {
            perror("accept");
            exit(1);
        }

        ////
        // recieving clientA name

        if ((numbytes_clientA = recv(new_fd_clientA, buf_clientA, MAXDATASIZE - 1, 0)) == -1)
        {
            perror("recv");
            exit(1);
        }

        buf_clientA[numbytes_clientA] = '\0';

        printf("The Central server received input=%s from the client using TCP over port %s\n", buf_clientA, ClIENT_A_TCP_PORT);

        // ////

        /////////// Client B //////////////

        new_fd_clientB = accept(sockfd_clientB, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd_clientB == -1)
        {
            perror("accept");
            exit(1);
        }

        ////
        // recieving clientB name

        if ((numbytes_clientB = recv(new_fd_clientB, buf_clientB, MAXDATASIZE - 1, 0)) == -1)
        {
            perror("recv");
            exit(1);
        }

        buf_clientB[numbytes_clientB] = '\0';

        printf("The Central server received input=%s from the client using TCP over port %s\n", buf_clientB, ClIENT_B_TCP_PORT);

        // ////

        /////////// Phase 1A END //////////////

        /////////// Phase 1B-2 send requests to Servers T, S, P and recieve relevent data //////////////
        //creating UDP sockets for Servers T, S, & P
        clientA_name = convertToString(buf_clientA, sizeof(buf_clientA));
        clientB_name = convertToString(buf_clientB, sizeof(buf_clientB));

        create_UDP_socket_and_send_request(SERVER_T_UDP_PORT, clientA_name, clientB_name, subgraph, scores);
        create_UDP_socket_and_recieve_data(SERVER_T_UDP_PORT, subgraph, scores, min_gap_pathA, min_gap_pathB, compatibility_score);
        create_UDP_socket_and_send_request(SERVER_S_UDP_PORT, clientA_name, clientB_name, subgraph, scores);
        create_UDP_socket_and_recieve_data(SERVER_S_UDP_PORT, subgraph, scores, min_gap_pathA, min_gap_pathB, compatibility_score);
        create_UDP_socket_and_send_request(SERVER_P_UDP_PORT, clientA_name, clientB_name, subgraph, scores);
        create_UDP_socket_and_recieve_data(SERVER_P_UDP_PORT, subgraph, scores, min_gap_pathA, min_gap_pathB, compatibility_score);

        /////////// END Phase 1B-2//////////////

        /////////// Phase 3 send relevent data to client A & B //////////////
        // if (compatibility_score.empty())
        if (max_matching_gap == compatibility_score)
        {
            clientA_return_data = subgraph + clientA_name + " and " + clientB_name;
            clientB_return_data = subgraph + clientB_name + " and " + clientA_name;
        }
        else
        {
            clientA_return_data = "Found compatibility for " + clientA_name + " and " + clientB_name + "\n";
            clientA_return_data += min_gap_pathA;

            clientB_return_data = "Found compatibility for " + clientB_name + " and " + clientA_name + "\n";
            clientB_return_data += min_gap_pathB;
        }

        // sending compatibility_score to clientA
        if (send(new_fd_clientA, &compatibility_score, sizeof(compatibility_score), 0) == -1)
        {
            perror("send");
        }

        // sending clientA_return_data to clientA
        if (send(new_fd_clientA, clientA_return_data.c_str(), strlen(clientA_return_data.c_str()), 0) == -1)
        {
            perror("send");
        }

        printf("The Central server sent the results to client A.\n");

        close(new_fd_clientA);

        ////
        // sending compatibility_score to clientB
        if (send(new_fd_clientB, &compatibility_score, sizeof(compatibility_score), 0) == -1)
        {
            perror("send");
        }

        // sending clientB_return_data to clientB
        if (send(new_fd_clientB, clientB_return_data.c_str(), strlen(clientB_return_data.c_str()), 0) == -1)
        {
            perror("send");
        }

        printf("The Central server sent the results to client B.\n");

        close(new_fd_clientB);

        /////////// END Phase 3//////////////
    }
    ////// END close the parent sockets /////
    // close parent sockets for sockfd_clientA & sockfd_clientB
    close(sockfd_clientA);
    close(sockfd_clientB);
}
