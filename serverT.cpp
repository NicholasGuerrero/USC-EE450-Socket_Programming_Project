#include "headers.h"
using namespace std;

#define SERVER_T_UDP_PORT "21452" //UDP port
#define SERVER_C_UDP_PORT "24452" //UDP Server C port
#define MAXBUFLEN 512
#define LOCALHOST "127.0.0.1" // Host address
#define BACKLOG 10            // how many pending connections queue will hold

map<string, int> create_map_from_file(vector<string> &out, string filename)
{
    string line;
    string word;
    ifstream myfile(filename.c_str());
    int counter = 0;
    const char delim = ' ';

    // empty map container
    map<string, int> m;

    if (myfile.is_open())
    {
        while (getline(myfile, line, delim))
        {
            istringstream iss(line);

            while (iss >> word)
            {
                out.push_back(word);
                if (m.find(word) == m.end())
                {
                    m[word] = counter;
                    counter++;
                }
                else
                {
                    continue;
                }
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

bool names_exist_in_file(string src,
                         string dst, map<string, int> m, string &subgraph)
{
    if (m.find(src) == m.end() || m.find(src) == m.end())
    {
        // if src or dst is not in the map means that names were not in edgelist
        subgraph = "Found no compatibility for ";
        return false;
    }
    return true;
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

map<int, string> reverse_key_value(map<string, int> m)
{
    map<int, string> reverse_m;
    for (map<string, int>::iterator i = m.begin(); i != m.end(); ++i)
        reverse_m[i->second] = i->first;

    return reverse_m;
}

class Graph
{
    int V;

    list<int> *adj;

    void DFS(int v, bool visited[], vector<string> out, map<int, string> m, string &component);

public:
    Graph(int V);
    ~Graph();
    void connectedComponents(vector<string> out, map<int, string> m, vector<string> &connected_components);
    void add_edges(vector<string> out, map<string, int> m);
};

void Graph::connectedComponents(vector<string> out, map<int, string> m, vector<string> &connected_components)
{
    bool *visited = new bool[V];
    for (int v = 0; v < V; v++)
        visited[v] = false;

    for (int v = 0; v < V; v++)
    {
        if (visited[v] == false)
        {
            string component;
            DFS(v, visited, out, m, component);
            component[strlen(component.c_str()) - 1] = '\n';
            connected_components.push_back(component);
        }
    }
    delete[] visited;
}

void Graph::DFS(int v, bool visited[], vector<string> out, map<int, string> m, string &component)
{
    visited[v] = true;
    component.append(m[v] + " ");

    list<int>::iterator i;
    for (i = adj[v].begin(); i != adj[v].end(); ++i)
        if (!visited[*i])
            DFS(*i, visited, out, m, component);
}

Graph::Graph(int V)
{
    this->V = V;
    adj = new list<int>[V];
}

Graph::~Graph() { delete[] adj; }

void Graph::add_edges(vector<string> out, map<string, int> m)
{
    int index_1 = 0;
    int index_2 = 1;

    for (int i = 0; i < out.size() / 2; i++)
    {
        adj[m[out[index_1]]].push_back(m[out[index_2]]);
        adj[m[out[index_2]]].push_back(m[out[index_1]]);

        index_1 += 2;
        index_2 += 2;
    }
}

string find_correct_graph_component(string clientA, string clientB, vector<string> connected_components)
{
    string component = "";
    int found;

    for (int i = 0; i < connected_components.size(); i++)
    {
        component = connected_components[i];
        found = 0;
        istringstream iss(component);
        do
        {
            if (found == 2)
            {
                return component;
            }
            string subs;
            iss >> subs;

            if (clientA == subs || clientB == subs)
            {
                found += 1;
            }

        } while (iss);
    }

    return "Found no compatibility for ";
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

void create_UDP_socket_and_send_data(const char *service_UDP, string &subgraph)
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

    //sending subgraph to central server
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

    freeaddrinfo(servinfo_UDP);

    printf("The ServerT finished sending the topology to Central\n");

    close(sockfd_UDP);
}

string create_edge_graph(string subgraph, vector<string> out, map<string, int> m)
{
    string edgelist = "";
    istringstream iss(subgraph);
    map<string, string> no_dups;

    do
    {
        string node;
        iss >> node;

        int index_1 = 0;
        int index_2 = 1;
        for (int i = 0; i < out.size() / 2; i++)
        {
            string name1 = out[index_1];
            string name2 = out[index_2];

            if (node == name1 || node == name2)
            {

                if (no_dups.find(name1 + " " + name2) == no_dups.end())
                {
                    edgelist.append(name1 + " " + name2 + "\n");
                    no_dups[name1 + " " + name2] = name2 + " " + name1;
                    no_dups[name2 + " " + name1] = name1 + " " + name2;
                }
            }
            index_1 += 2;
            index_2 += 2;
        }
    } while (iss);

    return edgelist;
}

int main(void)
{

    printf("The Server T is up and running using UDP on port %s. \n", SERVER_T_UDP_PORT);

    while (1)
    {
        int sockfd;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        int numbytes;
        struct sockaddr_storage their_addr;
        char buf_clientA[MAXBUFLEN], buf_clientB[MAXBUFLEN];
        socklen_t addr_len;
        char s[INET6_ADDRSTRLEN];
        string clientA_name, clientB_name;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        if ((rv = getaddrinfo(NULL, SERVER_T_UDP_PORT, &hints, &servinfo)) != 0)
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

        ///
        //recieving 1st client name
        if ((numbytes = recvfrom(sockfd, buf_clientA, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        buf_clientA[numbytes] = '\0';

        /////
        //recieving 2nd client name
        if ((numbytes = recvfrom(sockfd, buf_clientB, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        buf_clientB[numbytes] = '\0';
        /////

        printf("The ServerT received a request from Central to get the topology\n");

        close(sockfd);

        /// BUILDING SUBGRAPH ///
        vector<string> out;
        map<string, int> m;
        map<int, string> reverse_m;
        vector<string> connected_components;
        string subgraph;
        int v;

        m = create_map_from_file(out, "./edgelist.txt");
        v = m.size();
        reverse_m = reverse_key_value(m);

        clientA_name = convertToString(buf_clientA, sizeof(buf_clientA));
        clientB_name = convertToString(buf_clientB, sizeof(buf_clientB));

        Graph g(v);

        g.add_edges(out, m);
        g.connectedComponents(out, reverse_m, connected_components);

        subgraph = find_correct_graph_component(clientA_name, clientB_name, connected_components);

        subgraph = create_edge_graph(subgraph, out, m);
        ///

        create_UDP_socket_and_send_data(SERVER_C_UDP_PORT, subgraph);
    }

    return 0;
}
