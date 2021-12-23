USC-EE450-Socket-Programming

a. Nicholas Guerrero

b. ng55585


c. 
I have not completed the optional part of the assignement. The program starts out by spinning up the servers in the following order Central server, ServerT, ServerS, ServerP, running them continuously until they are stopped. Users can prompt clientA and client B continuously until the servers are manually shut down.


d.
This is what the files do:
ClientA : Sends clientA name to Central server and displays the compatibility matching information
ClientB : Sends clientB name to Central server and displays the compatibility matching information
Central : Manages the data of the program. Send request to serverT to get a topology which it sends to ServerS and recieves back a score table. It then sends the topology and score table to ServerP and recieves the compatibility information and shortest path information for the names input by clientA and clientB.
ServerT : Recieves clientA name & clientB name and calculates a topology which it sends to the central server
ServerS : Recieves a topology from the Central server and sends back a score table for the relevent nodes
ServerP : Recieves a topology and scores from the Central Server and finds the shortest path between the nodes containing clientA and clientB if it exists.


e.
ClientA:
The client is up and running 
The client sent <INPUT NAME A> to the Central server 
Found compatibility for <INPUT NAME A>  and <INPUT NAME B> 
<INPUT1> --- <USERY> ---<USERX> --- <INPUT2>
Matching Gap : <VALUE>
or 
“Found no compatibility for <INPUT1> and <INPUT2>”

ClientB:
The client is up and running 
The client sent <INPUT NAME A> to the Central server 
Found compatibility for <INPUT2> and <INPUT1>:
<INPUT2> --- <USERX> ---<USERY> --- <INPUT1>
Matching Gap : <VALUE>
or
Found no compatibility for <INPUT2> and <INPUT1>

ServerT:
The Server T is up and running using UDP on port 21452. 
The ServerT received a request from Central to get the topology
The ServerT finished sending the topology to Central

ServerS:
The Server S is up and running using UDP on port 22452. 
The ServerS received a request from Central to get the scores
The ServerS finished sending the scores to Central.

ServerT:
The Server P is up and running using UDP on port 23452. 
The ServerP received the topology and score information.
The ServerP finished sending the results to the Central.

Central:
The Central server is up and running.
The Central server received input=INPUTA from the client using TCP over port 25452
The Central server received input=INPUTB from the client using TCP over port 26452
The Central server sent a request to Backend-Server T.
The Central server received information from Backend-Server T using UDP over port 24452.
The Central server sent a request to Backend-Server S.
The Central server received information from Backend-Server S using UDP over port 24452.
The Central server sent a processing request to Backend-Server P.
The Central server received the results from backend server P
The Central server sent the results to client A.
The Central server sent the results to client B.

or 
“Found no compatibility for <INPUT1> and <INPUT2>”


g. My project works as expected except for part C of the assignment. It does not calculate the matching gap correctly. I couldn't figure out how to implement a shortest path algorithm such as Dijkstra in time for the deadline. I tried very hard and learn a lot. I tried to account for any large amount of data being streamed over the connections using while loops. So that any size file could be used with the program.

h. Most of the code is my own except pieces from
Beej's Code: http://www.beej.us/guide/bgnet/
        Create sockets (TCP / UDP);
        Bind a socket;
        Send & receive;