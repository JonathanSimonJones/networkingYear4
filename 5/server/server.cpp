/*	AG0907 select-based TCP server example - by Henry Fortuna and Adam Sampson

	A simple server that waits for connections.
	The server repeats back anything it receives from the client.

	TO DO:
	Add recv for client to get message
*/

#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")


// The IP address for the server to listen on
#define SERVERIP "127.0.0.1"

// The TCP port number for the server to listen on
#define SERVERPORT 5555

// The (fixed) size of message that we send between the two programs
#define MESSAGESIZE 40

// The maximum number of clients who can connect to the server
#define MAX_NUM_CLIENTS 5

// Debug output define 
#define _DEBUG 1

// Setup debug output
#if _DEBUG == 1
#define DEBUG_PRINT(string,...) printf(string, __VA_ARGS__)
#else
#define DEBUG_PRINT(string,...) 0
#endif

// Prototypes
void die(const char *message);

// Each instance of this class represents a connected client.
class Client {
public:
	// Constructor.
	// sock: the socket that we've accepted the client connection on.
	Client(SOCKET sock, sockaddr _addressTargetSocket)
		: sock_(sock)
		, addressTargetSocket(_addressTargetSocket)
		, write(false)
		, read(false)
	{
		printf("New connection\n");
	}

	// Destructor.
	~Client()
	{
		printf("Closing connection\n");
		closesocket(sock_);
	}

	// Return the client's socket.
	SOCKET sock()
	{
		return sock_;
	}

	// Return whether this connection is in a state where we want to try
	// reading from the socket.
	bool wantRead()
	{
		return read;
	}

	// Call this when the socket is ready to read.
	// Returns true if the socket should be closed.
	bool doRead()
	{
		char buffer[MESSAGESIZE];

		// Receive as much data from the client as will fit in the buffer.
		// FIXME: we might not get the whole message at once...
		int count = recv(sock_, buffer, MESSAGESIZE, 0);
		if (count <= 0) {
			printf("Client connection closed or broken\n");
			return true;
		}
		if (count != MESSAGESIZE) {
			printf("Got strange-sized message from client\n");
			return true;
		}
		if (memcmp(buffer, "quit", 4) == 0) {
			printf("Client asked to quit\n");
			return true;
		}

		// (Note that recv will not write a \0 at the end of the message it's
		// received -- so we can't just use it as a C-style string directly
		// without writing the \0 ourself.)

		DEBUG_PRINT("Received %d bytes from the client: '", count);
		fwrite(buffer, 1, count, stdout);
		printf("'\n");

		// Send the same data back to the client.
		// FIXME: the socket might not be ready to send yet -- so this could block!
		count = send(sock_, buffer, MESSAGESIZE, 0);
		if (count != MESSAGESIZE)
		{
			printf("send failed\n");
			return true;
		}

		return false;
	}

	bool wantWrite(void)
	{
		return write;
	}
	
	bool doWrite(std::string message)
	{
		return write;	// Temp value
	}

	void setReadReady()
	{
		// If not read ready 
		if(!read)
		{
			// Set to read ready
			changeState();
		}
	}

	void setWriteReady()
	{
		// If not write ready
		if(!write)
		{
			// Set to write
			changeState();
		}
	}

private:
	SOCKET sock_;
	bool read;
	bool write;

	sockaddr addressTargetSocket;

	void changeState()
	{
		if(read)
		{
			write = true;
			read = false;
		}
		else
		{
			read=true;
			write = false;
		}
	}
};


int main()
{
	printf("Echo Server\n");

	// Initialise the WinSock library -- we want version 2.2.
	WSADATA w;
	int error = WSAStartup(0x0202, &w);
	if (error != 0)
	{
		die("WSAStartup failed");
	}
	if (w.wVersion != 0x0202)
	{
		die("Wrong WinSock version");
	}

	// Create a TCP socket that we'll use to listen for connections.
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == SOCKET_ERROR)
	{
		die("socket failed");
	}

	// Fill out a sockaddr_in structure to describe the address we'll listen on.
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(SERVERIP);
	// htons converts the port number to network byte order (big-endian).
	serverAddr.sin_port = htons(SERVERPORT);

	// Bind the server socket to that address.
	if (bind(serverSocket, (LPSOCKADDR) &serverAddr, sizeof(serverAddr)) != 0)
	{
		die("bind failed");
	}

	// ntohs does the opposite of htons.
	printf("Server socket bound to port %d\n", ntohs(serverAddr.sin_port));

	// Make the socket listen for connections.
	if (listen(serverSocket, 1) != 0)
	{
		die("listen failed");
	}

	printf("Server socket listening\n");

	// The list of clients currently connected to the server.
	std::list<Client *> clients;

	// The server's main loop, where we'll wait for new connections to
	// come in, or for new data to appear on our existing connections.
	while (true)
	{
		// The structure that describes the set of sockets we're interested in.
		fd_set readable;		// Contains 2 variables -	fd_count - The number of the sockets in the set
								//							fd_array - An array of sockets that are in the set 
		FD_ZERO(&readable);		// Initialise set to null set

		// Add the server socket, which will become "readable" if there's a new
		// connection to accept.
		FD_SET(serverSocket, &readable);	// Adds descriptor socket to FD set

		// Add all of the connected clients' sockets.
		for (std::list<Client *>::iterator it = clients.begin(); it != clients.end(); ++it)
		{
			Client *client = *it;

			if (client->wantRead())
			{
				FD_SET(client->sock(), &readable);
			}
		}

		// The structure that describes how long to wait for something to happen.
		timeval timeout;
		// We want a 2.5-second timeout.
		timeout.tv_sec = 2;
		timeout.tv_usec = 500000;

		// Wait for one of the sockets to become readable.
		// (We can only get away with passing 0 for the first argument here because
		// we're on Windows -- other sockets implementations need a proper value there.)
		int count = select(0, &readable, NULL, NULL, &timeout);	// Determines the status of one or more sockets, waiting if necessary, to perform synchronous I/O.
																// select(	ingnored	- included only for compatibility with Berkeley sockets
																//			readfds		- An optional pointer to a set of sockets to be checked for readability
																//			writefds	- An optional pointer to a set of sockets to be checked for writability
																//			exceptfds	- An optional pointer to a set of sockets to be checked for errors
																//			timeout		- The maximum time for select to wait, provided in the form of a TIMEVAL structure, set to null for blocking
																// Returns number of sockt handels that are ready and contained in the fd_set structures, zero if time limit expired, or SOCKET_ERROR on error
		if (count == SOCKET_ERROR)
		{
			die("select failed");
		}
		printf("%d clients; %d sockets are ready\n", clients.size(), count);
		// readable now tells us which sockets are ready.
		// If count == 0 (i.e. no sockets are ready) then the timeout occurred.

		// Is there a new connection to accept?
		if (FD_ISSET(serverSocket, &readable))	// Returns non zero if 1st param is non zero
		{
			// Accept a new connection to the server socket.
			// This gives us back a new socket connected to the client, and
			// also fills in an address structure with the client's address.
			sockaddr clientAddr;
			int addrSize = sizeof(sockaddr);
			SOCKET clientSocket = accept(serverSocket, &clientAddr, &addrSize);
			if (clientSocket == INVALID_SOCKET)
			{
				printf("accept failed\n");
				continue;
			}

			// Create a new Client object, and add it to the collection.
			// Check to see if we have too many clients
			if(clients.size() < MAX_NUM_CLIENTS)	
			{
				// Accept the client 
				printf("Accepted client\n");
				clients.push_back(new Client(clientSocket, clientAddr));
				
				// Send welcome message to client 
				char welcomeMessage[MESSAGESIZE] = "Welcome to the chat server :)\n";
				sendto(clientSocket, welcomeMessage, MESSAGESIZE, 0,  (const sockaddr *)&clientAddr, sizeof(clientAddr));
			}
			else
			{
				// Deny the client 
				printf("Denied client\n");

				// Send message to client 
				char sorryMessage[MESSAGESIZE] = "Sorry busy atm :(\n";
				sendto(clientSocket, sorryMessage, MESSAGESIZE, 0,  (const sockaddr *)&clientAddr, sizeof(clientAddr));
			}
		}

		// Check each of the clients.
		for (std::list<Client *>::iterator it = clients.begin(); it != clients.end(); )  // note no ++it here
		{
			Client *client = *it;
			bool dead = false;

			// Is there data to read from this client's socket?
			if (FD_ISSET(client->sock(), &readable))
			{
				// Set the client to read ready
				client->setReadReady();

				// Check client is ready to read
				if(client->doRead())
				{
					dead |= client->doRead();
				}
			}

			if (dead)
			{
				// The client said it was dead -- so free the object,
				// and remove it from the clients list.
				delete client;
				it = clients.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	// We won't actually get here, but if we did then we'd want to clean up...
	printf("Quitting\n");
	closesocket(serverSocket);
	WSACleanup();
	return 0;
}


// Print an error message and exit.
void die(const char *message) {
	fprintf(stderr, "Error: %s (WSAGetLastError() = %d)\n", message, WSAGetLastError());
	exit(1);
}