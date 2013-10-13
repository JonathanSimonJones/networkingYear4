/*	AG0907 TCP server example - by Henry Fortuna and Adam Sampson

	A simple server that waits for a connection.
	When a connection is made, the server sends "!" to the client.
	It then repeats back anything it receives from the client.
	All the calls are blocking for simplicity.

	There is nothing elegant about this code as you will see.
	Its purpose is to illustrate enough to get started coding
	with WinSock.
*/

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


// Prototypes
void die(const char *message);


int main()
{
	printf("Echo Server\n");

	// Initialise the WinSock library -- we want version 2.2.
	WSADATA w;
	int error = WSAStartup(0x0202, &w);		// WSAStartup(wVersionRequested, whereToStoreData);
	if (error != 0)
	{
		die("WSAStartup failed");
	}
	if (w.wVersion != 0x0202)
	{
		die("Wrong WinSock version");
	}

	// Create a TCP socket that we'll use to listen for connections.
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);			// Creates a socket
																	// socket(address family specification (IPv4 or 6), type specification, protocol);
																	// Return - No errors then returns a desciptor referencing the new socket, else error code
																	// AF_INET		- currently using IPv4 address family  
																	// SOCK_STREAM	- Sequenced/reliable/two-way/connectioned based 
																	//					byte strams with OOV data transmission mechanism/uses 
																	//					transmission control protocol 
																	// 0			- (specific to address family and socket type) we have not specified a protocol and the service provider will choose one for us.
																	//? Why not specify TCP directly?
	// FIXME: we should test for error here

	// Fill out a sockaddr_in structure to describe the address we'll listen on.
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(SERVERIP);				// Converts a string containing an IPv4 dotted-decimal address into a proper address for the IN_ADDR structure
																	// inet_addr(a null termined character string representing a number expressed in the Internet standard "." (dotted) notation); 
	// htons converts the port number to network byte order (big-endian).
	serverAddr.sin_port = htons(SERVERPORT);						// converts a u_short from host to TCP/IP network byte order (which is big-endian)

	// Bind the server socket to that address.
	if (bind(serverSocket, (LPSOCKADDR) &serverAddr, sizeof(serverAddr)) != 0)		// Associates a local address with a socket
	{																				// bind(a socket (SOCKET), pointer to socket address structure with a local address to assign to bound socket, length in bytes of calue pointed to by the name parameter)
		die("bind failed");
	}

	// ntohs does the opposite of htons.
	printf("Server socket bound to port %d\n", ntohs(serverAddr.sin_port));

	// Make the socket listen for connections.
	if (listen(serverSocket, 1) != 0)		// Places a socket in a state in which it is listening for an incoming connection
	{										// listen(a socket, maxium length of a queue of pending connections);
		die("listen failed");				// must bind socket or this is hit
	}

	printf("Server socket listening\n");

	while (true)
	{
		printf("Waiting for a connection...\n");

		// Accept a new connection to the server socket.
		// This gives us back a new socket connected to the client, and
		// also fills in an address structure with the client's address.
		sockaddr clientAddr;
		int addrSize = sizeof(sockaddr);
		SOCKET clientSocket = accept(serverSocket, &clientAddr, &addrSize);				// accept(socket, optional pointer to a buffer that receives the address of the connecting entity, optional pointer to an integer that contains the length of a structure pointed to by the previous parameter);
		if (clientSocket == INVALID_SOCKET)
		{
			die("accept failed");		// Gets hit if listen isn't called
			// FIXME: in a real server, we wouldn't want the server to crash if
			// it failed to accept a connection -- recover more effectively!
		}

		printf("Client has connected!\n");

		// We'll use this array to hold the messages we exchange with the client.
		char buffer[MESSAGESIZE];

		// Fill the buffer with - characters to start with.
		memset(buffer, '-', MESSAGESIZE);

		// Send a greeting message to the client.
		memcpy(buffer, "hello", 5);
		send(clientSocket, buffer, MESSAGESIZE, 0);
		// FIXME: check for errors from send

		while (true)
		{
			// Receive as much data from the client as will fit in the buffer.
			int count = recv(clientSocket, buffer, MESSAGESIZE, 0);
			// FIXME: check for errors from recv

			if (count <= 0) {
				printf("Client closed connection\n");
				break;
			}
			if (count != MESSAGESIZE) {
				die("Got strange-sized message from client");
			}
			if (memcmp(buffer, "quit", 4) == 0) {
				printf("Client asked to quit\n");
				break;
			}

			// (Note that recv will not write a \0 at the end of the message it's
			// received -- so we can't just use it as a C-style string directly
			// without writing the \0 ourself.)

			printf("Received %d bytes from the client: '", count);
			fwrite(buffer, 1, count, stdout);
			printf("'\n");

			// Send the same data back to the client.
			send(clientSocket, buffer, MESSAGESIZE, 0);
			// FIXME: check for errors from send
		}

		printf("Closing connection\n");

		// Close the connection.
		closesocket(clientSocket);
	}

	// We won't actually get here, but if we did then we'd want to clean up...
	printf("Quitting\n");
	closesocket(serverSocket);
	WSACleanup();
	return 0;
}


// Print an error message and exit.
void die(const char *message) {
	fprintf(stderr, "Error: %s\n", message);
	exit(1);
}