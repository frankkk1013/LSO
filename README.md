# LSO

Multithread server in C for the following project

# Topic


## Summary Description

To realise a client-server system that allows clients to control the proximity of other clients by monitoring their location and status. The C language is used on a UNIX platform. Processes must communicate via TCP sockets. The implementation must be accompanied by adequate documentation.
## Detailed description
The server must keep the geographical location of connected clients up to date by providing information on them. In order to access the service, each user must first register on the site by entering a password and nickname. Upon access, the client must also declare one of the following statuses: A (healthy) or B (infected). The client will have to provide its geographic location to the server to access the information services. With this access, the client will be able to ask the server for its current status (A or B), the number of nearby clients (with distance provided by the client) in status A or B, the list of nearby users (with distance provided by the client) in status A or B. The server must also periodically update the state of the clients which varies according to the following rule (contagion): a client will go to state B if it remains in the vicinity (at a distance less than a parameter set by the server) of another client in state B for at least N monitoring cycles (with N set by the server).
There is no a priori limit to the number of users that can connect with the server. The client will allow the user to connect to a communication server, indicating via command line the name or IP address of that server and the port to be used. Once connected to a server, the user will be able to: register as a new user or access the server as a registered user. To access the server's services, the client will have to provide its location. Once logged in, the client will be able to request information on its current status and that of other clients as indicated above.
The server must support all the functionalities described in the section on the client. When the server is started, it will be possible to specify via the command line the TCP port on which to listen. The server will be of the concurrent type, i.e. it will be able to serve several clients simultaneously. During its regular operation, the server will log the main activities in a special file. For example, storing the date and time of connection of the clients and their symbolic name (if available, otherwise the IP address) and the number of infected for each monitoring cycle.

