#include "headers.h"
using namespace std;

#define SERVER_P_UDP_PORT "23452" //UDP port
#define SERVER_C_UDP_PORT "24452" //UDP Server C port
#define MAXBUFLEN 512
#define LOCALHOST "127.0.0.1" // Host address
#define BACKLOG 10            // how many pending connections queue will hold

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void create_UDP_socket_and_send_data(const char *service_UDP, string &min_gap_pathA, string &min_gap_pathB, double compatibility_score)
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

    //sending min_gap_pathA to central server
    if ((numbytes_UDP = sendto(sockfd_UDP, min_gap_pathA.c_str(), strlen(min_gap_pathA.c_str()), 0,
                               p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    //sending min_gap_pathB to central server
    if ((numbytes_UDP = sendto(sockfd_UDP, min_gap_pathB.c_str(), strlen(min_gap_pathB.c_str()), 0,
                               p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    //sending compatibility score to central server
    if ((numbytes_UDP = sendto(sockfd_UDP, &compatibility_score, sizeof(compatibility_score), 0,
                               p_UDP->ai_addr, p_UDP->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo_UDP);

    printf("The ServerP finished sending the results to the Central.\n");

    close(sockfd_UDP);
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

map<string, double> create_double_score_map(string scores)
{
    vector<string> score_entries;
    vector<string> score_entry;
    map<string, double> double_score_map;

    if (scores.empty())
    {
        return double_score_map;
    }

    score_entries = split(scores, '\n');

    for (int i = 0; i < score_entries.size(); i++)
    {
        score_entry = split(score_entries[i], ' ');
        double_score_map[score_entry[0]] = atof(score_entry[1].c_str());
    }

    return double_score_map;
}

double find_optimal_matching_gap(string subgraph, map<string, double> double_score_map, string &min_gap_pathA, string &min_gap_pathB)
{
    vector<string> subgraph_paths;
    vector<string> subgraph_path;
    double matching_gap;
    double matching_gap_num;
    double matching_gap_den;
    double min_matching_gap = 1.79769e+308; //max value for double in c++
    string path_representation_clientA;
    string path_representation_clientB;

    if (subgraph.substr(0, 26) == "Found no compatibility for")
    {
        return min_matching_gap;
    }

    subgraph_paths = split(subgraph, '\n');

    for (int i = 0; i < subgraph_paths.size(); i++)
    {
        subgraph_path = split(subgraph_paths[i], ' ');
        path_representation_clientA = "";
        path_representation_clientB = "";
        for (int j = 0; j < subgraph_path.size() - 1; j++)
        {
            matching_gap_num = abs(double_score_map[subgraph_path[j]] - double_score_map[subgraph_path[j + 1]]);
            matching_gap_den = double_score_map[subgraph_path[j]] + double_score_map[subgraph_path[j + 1]];
            matching_gap += matching_gap_num / matching_gap_den;

            path_representation_clientA += subgraph_path[j] + " --- ";
            path_representation_clientB += subgraph_path[subgraph_path.size() - 1 - j] + " --- ";
        }
        path_representation_clientA += subgraph_path[subgraph_path.size() - 1] + "\nCompatibility score: ";
        path_representation_clientB += subgraph_path[0] + "\nCompatibility score: ";
        if (matching_gap < min_matching_gap)
        {
            min_matching_gap = matching_gap;
            min_gap_pathA = path_representation_clientA;
            min_gap_pathB = path_representation_clientB;
        }
    }

    return min_matching_gap;
}

int main(void)
{
    printf("The Server P is up and running using UDP on port %s. \n", SERVER_P_UDP_PORT);

    while (1)
    {
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int numbytes;
        struct sockaddr_storage their_addr;
        char subgraph_buf[MAXBUFLEN];
        char scores_buf[MAXBUFLEN];
        socklen_t addr_len;
        char s[INET6_ADDRSTRLEN];
        string subgraph;
        string scores;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        if ((rv = getaddrinfo(NULL, SERVER_P_UDP_PORT, &hints, &servinfo)) != 0)
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

        ///  scores portion
        int bytes_to_recieve = 0;
        int bytes_recieved = 0;

        if ((numbytes = recvfrom(sockfd, scores_buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        bytes_to_recieve = atoi(convertToString(scores_buf, sizeof(scores_buf)).c_str());

        memset(&scores_buf, 0, sizeof scores_buf);

        //recieving scores
        while (bytes_recieved < bytes_to_recieve)
        {
            //recieve subgraph
            if ((numbytes = recvfrom(sockfd, scores_buf, MAXBUFLEN - 1, 0,
                                     (struct sockaddr *)&their_addr, &addr_len)) == -1)
            {
                perror("recvfrom");
                exit(1);
            }

            bytes_recieved += MAXBUFLEN - 1;
            scores_buf[numbytes] = '\0';

            scores += convertToString(scores_buf, sizeof(scores_buf));
            memset(&scores_buf, 0, sizeof scores_buf);
        }
        ///

        /// subgraph portion

        bytes_to_recieve = 0;
        bytes_recieved = 0;

        if ((numbytes = recvfrom(sockfd, subgraph_buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        bytes_to_recieve = atoi(convertToString(subgraph_buf, sizeof(subgraph_buf)).c_str());

        memset(&subgraph_buf, 0, sizeof subgraph_buf);

        ///
        //recieving subgraph
        while (bytes_recieved < bytes_to_recieve)
        {
            //recieve subgraph
            if ((numbytes = recvfrom(sockfd, subgraph_buf, MAXBUFLEN - 1, 0,
                                     (struct sockaddr *)&their_addr, &addr_len)) == -1)
            {
                perror("recvfrom");
                exit(1);
            }

            bytes_recieved += MAXBUFLEN - 1;
            subgraph_buf[numbytes] = '\0';

            subgraph += convertToString(subgraph_buf, sizeof(subgraph_buf));
            memset(&subgraph_buf, 0, sizeof subgraph_buf);
        }

        printf("The ServerP received the topology and score information.\n");

        close(sockfd);

        /// calculated optimal matching gap  ///

        map<string, double> double_score_map;
        string min_gap_pathA = "";
        string min_gap_pathB = "";
        double compatibility_score;

        double_score_map = create_double_score_map(scores);

        compatibility_score = find_optimal_matching_gap(subgraph, double_score_map, min_gap_pathA, min_gap_pathB);

        ///

        create_UDP_socket_and_send_data(SERVER_C_UDP_PORT, min_gap_pathA, min_gap_pathB, compatibility_score);
    }

    return 0;
}