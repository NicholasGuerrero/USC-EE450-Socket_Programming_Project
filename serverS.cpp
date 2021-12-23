#include "headers.h"
using namespace std;

#define SERVER_S_UDP_PORT "22452" //UDP port
#define SERVER_C_UDP_PORT "24452" //UDP Server C port
#define MAXBUFLEN 512
#define LOCALHOST "127.0.0.1" // Host address
#define BACKLOG 10            // how many pending connections queue will hold

vector<string> split(string str, char delimiter)
{
    vector<string> paths;
    stringstream ss(str);
    string path;

    while (getline(ss, path, delimiter))
    {
        paths.push_back(path);
    }

    return paths;
}

map<string, string> create_scoremap_from_file(string filename)
{
    string line;
    string name;
    string score;
    ifstream myfile(filename.c_str());
    vector<string> sep;

    // empty map container
    map<string, string> m;

    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            istringstream iss(line);

            sep = split(line, ' ');
            if (m.find(name) == m.end())
            {
                m[sep[0]] = sep[1];
            }
            else
            {
                continue;
            }
        }
        myfile.close();
    }
    else
    {
        cout << "Unable to open file";
        exit(1);
    }

    return m;
}

map<string, string> create_subgraph_map(string subgraph, map<string, string> score_map)
{
    vector<string> paths;
    vector<string> nodes;
    map<string, string> subgraph_map;

    if (subgraph.substr(0, 26) == "Found no compatibility for")
    {
        return subgraph_map;
    }

    paths = split(subgraph, '\n');

    for (int i = 0; i < paths.size(); i++)
    {
        nodes = split(paths[i], ' ');
        for (int j = 0; j < nodes.size(); j++)
        {
            if (subgraph_map.find(nodes[j]) == subgraph_map.end())
            {
                subgraph_map[nodes[j]] = score_map[nodes[j]];
            }
            else
            {
                continue;
            }
        }
    }

    return subgraph_map;
}

string create_scores(map<string, string> subgraph_map)
{
    string scores = "";
    for (map<string, string>::iterator it = subgraph_map.begin(); it != subgraph_map.end(); it++)
    {
        scores.append(it->first + " " + it->second + "\n");
    }
    return scores;
}

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

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void create_UDP_socket_and_send_data(const char *service_UDP, string &scores)
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

    //sending scores to central server
    int bytes_to_send = strlen(scores.c_str());
    int bytes_sent = 0;
    stringstream ss;

    ss << bytes_to_send;

    if ((numbytes_UDP = sendto(sockfd_UDP, ss.str().c_str(), MAXBUFLEN - 1, 0,
                               p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    while (bytes_sent < bytes_to_send)
    {
        int end_slice = MAXBUFLEN - 1;
        string response = scores.substr(bytes_sent, MAXBUFLEN - 1).c_str();

        if ((numbytes_UDP = sendto(sockfd_UDP, response.c_str(), MAXBUFLEN - 1, 0,
                                   p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
        {
            perror("talker: sendto");
            exit(1);
        }
        bytes_sent += numbytes_UDP;
        end_slice += end_slice;
    }

    freeaddrinfo(servinfo_UDP);

    printf("The ServerS finished sending the scores to Central.\n");

    close(sockfd_UDP);
}

int main(void)
{
    printf("The Server S is up and running using UDP on port %s. \n", SERVER_S_UDP_PORT);

    while (1)
    {
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int numbytes;
        struct sockaddr_storage their_addr;
        char buf[MAXBUFLEN];
        socklen_t addr_len;
        char s[INET6_ADDRSTRLEN];
        string subgraph;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        if ((rv = getaddrinfo(NULL, SERVER_S_UDP_PORT, &hints, &servinfo)) != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
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
            return 2;
        }

        freeaddrinfo(servinfo);

        addr_len = sizeof their_addr;

        int bytes_to_recieve;
        int bytes_recieved = 0;

        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        bytes_to_recieve = atoi(convertToString(buf, sizeof(buf)).c_str());

        memset(&buf, 0, sizeof buf);

        while (bytes_recieved < bytes_to_recieve)
        {
            //recieve subgraph
            if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                     (struct sockaddr *)&their_addr, &addr_len)) == -1)
            {
                perror("recvfrom");
                exit(1);
            }

            bytes_recieved += MAXBUFLEN - 1;
            buf[numbytes] = '\0';

            subgraph += convertToString(buf, sizeof(buf));
            memset(&buf, 0, sizeof buf);
        }

        printf("The ServerS received a request from Central to get the scores\n");

        close(sockfd);

        /// BUILDING SUB SCORES ///

        map<string, string> score_map;
        map<string, string> subgraph_map;
        string scores;

        score_map = create_scoremap_from_file("./scores.txt");

        subgraph_map = create_subgraph_map(subgraph, score_map);

        scores = create_scores(subgraph_map);

        create_UDP_socket_and_send_data(SERVER_C_UDP_PORT, scores);
    }

    return 0;
}