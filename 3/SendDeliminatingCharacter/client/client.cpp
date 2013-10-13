/*	AG0907 TCP client example - by Henry Fortuna and Adam Sampson

	A simple client that connects to a server and waits for
	a response. Text typed is sent to the server which echos it back,
	and the response is printed out.
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <winsock2.h>
#include <cstring>
#include <algorithm>		// std::fill

#pragma comment(lib, "ws2_32.lib")


// The IP address of the server to connect to
#define SERVERIP "127.0.0.1"

// The TCP port number on the server to connect to
#define SERVERPORT 5555

// The (fixed) size of message that we send between the two programs
static const int MESSAGESIZE = 40;


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
	// FIXME: check for errors from socket

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
	char buffer[MESSAGESIZE] = {};
	std::string message = "This is a test message to see if it goes beyond the character count, this should be beyond forty characters soon.";

	while (true)
	{
		printf("Type some text (\"quit\" to exit): ");
		fflush(stdout);

		// Read a line of text from the user.
		std::string line = " ";
		std::getline(std::cin, line);
		line.append("|");
		// Now "line" contains what the user typed. Unlike fgets, it doesn't include the \n.

		std::cout << std::endl;

		std::string tempBuffer = " ";
		int i = 0;

		do//for(int i = 0; i < (line.size() / MESSAGESIZE); i++)
		{
			// Copy the line into the buffer, filling the rest with dashes.
			memset(buffer, '\0', sizeof(char)*MESSAGESIZE);
			memcpy(buffer, &line[ (i*40) - (i) ], MESSAGESIZE-1);
			// FIXME: if line.size() is bigger than the buffer it'll overflow (and likely corrupt memory)

			// Send the message to the server.
			send(sock, buffer, MESSAGESIZE, 0);
			// FIXME: check for error from send
			i++;
			
			tempBuffer = buffer;

			/*
			if( tempBuffer.find(".") != std::string::npos )
			{
				int test = 2;
			}
			*/
		}while(tempBuffer.find("|") == std::string::npos);

		// Read a response back from the server.
		int count = recv(sock, buffer, MESSAGESIZE, MSG_WAITALL);
		// FIXME: check for error from recv
		if (count <= 0)
		{
			printf("Server closed connection\n");
			break;
		}

		printf("Received %d bytes from the server: '", count);
		fwrite(buffer, 1, count, stdout);
		printf("'\n");
	}

	printf("Quitting\n");

	// Close the socket and clean up the sockets library.
	closesocket(sock);
	WSACleanup();

	return 0;
}


// Print an error message and exit.
void die(const char *message) {
	fprintf(stderr, "Error: %s\n", message);
	exit(1);
}