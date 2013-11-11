/*	AG0907 TCP client example - by Henry Fortuna and Adam Sampson

	A simple client that connects to a server and waits for
	a response. Text typed is sent to the server which echos it back,
	and the response is printed out.
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")


// The IP address of the server to connect to
#define SERVERIP "127.0.0.1"

// The TCP port number on the server to connect to
#define SERVERPORT 5555

// The (fixed) size of message that we send between the two programs
#define MESSAGESIZE 40

// Debug output define 
#define _DEBUG 0

// Setup debug output
#if _DEBUG == 1
#define DEBUG_PRINT(string,...) printf(string, __VA_ARGS__)
#else
#define DEBUG_PRINT(string,...) 0
#endif

// Prototypes
void die(const char *message);


int main()
{
	printf("Client Program\n");

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

	// Create a TCP socket that we'll connect to the server
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == SOCKET_ERROR)
	{
		die("socket failed");
	}

	// Fill out a sockaddr_in structure with the address that
	// we want to connect to.
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	// htons converts the port number to network byte order (big-endian).
	addr.sin_port = htons(SERVERPORT);
	addr.sin_addr.s_addr = inet_addr(SERVERIP);

	// Connect the socket to the server.
	if (connect(sock, (LPSOCKADDR) &addr, sizeof addr) == SOCKET_ERROR)
	{
		die("connect failed");
	}

	printf("Connected to server\n");

	// We'll use this buffer to hold what we receive from the server.
	char buffer[MESSAGESIZE];
	bool firstIteration = true;

	while (true)
	{
		int count = 0;		// Used to check for error code from send and receive

		if(!firstIteration)
		{
			printf("Type some text (\"quit\" to exit): ");
			fflush(stdout);

			// Read a line of text from the user.
			std::string line;
			std::getline(std::cin, line);
			// Now "line" contains what the user typed (without the trailing \n).

			// Copy the line into the buffer, filling the rest with dashes.
			memset(buffer, '-', MESSAGESIZE);
			int to_copy = min(MESSAGESIZE, line.size());
			memcpy(buffer, line.c_str(), to_copy);

			// Send the message to the server.
			count = send(sock, buffer, MESSAGESIZE, 0);
			if (count != MESSAGESIZE)
			{
				die("send failed");
			}
		}

		firstIteration = false;

		// Read a response back from the server.
		count = recv(sock, buffer, MESSAGESIZE, MSG_WAITALL);
		if (count <= 0)
		{
			printf("Server closed connection\n");
			break;
		}

		DEBUG_PRINT("Received %d bytes from the server.\n", count);
		fwrite(buffer, 1, count, stdout);
		printf("\n");
	}

	printf("Quitting\n");

	// Close the socket and clean up the sockets library.
	closesocket(sock);
	WSACleanup();

	return 0;
}


// Print an error message and exit.
void die(const char *message) {
	fprintf(stderr, "Error: %s (WSAGetLastError() = %d)\n", message, WSAGetLastError());
	exit(1);
}