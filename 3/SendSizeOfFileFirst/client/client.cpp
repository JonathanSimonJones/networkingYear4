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

#pragma comment(lib, "ws2_32.lib")


// The IP address of the server to connect to
#define SERVERIP "127.0.0.1"

// The TCP port number on the server to connect to
#define SERVERPORT 5555

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

	while (true)
	{
		printf("Type some text (\"quit\" to exit): ");
		fflush(stdout);

		// Read a line of text from the user.
		std::string line;
		std::getline(std::cin, line);
		// Now "line" contains what the user typed. Unlike fgets, it doesn't include the \n.

		// Copy the line into the buffer, filling the rest with dashes.
		//memset(buffer, '-', MESSAGESIZE);
		//memcpy(buffer, line.c_str(), line.size());
		// FIXME: if line.size() is bigger than the buffer it'll overflow (and likely corrupt memory)

		// send size of message
		char *numberOfChars = (char*)malloc(line.size());
		itoa(line.size(), numberOfChars, 10);
		send(sock, numberOfChars, sizeof(int), 0);

		// Clean up
		//? Is this correct way
		free(numberOfChars);
		
		char *buffer = (char *)malloc(sizeof(int));
		// Read a response back from the server.
		// FIXME: Don't use old buffer, use correct size buffer instead
		int count = recv(sock, buffer, sizeof(int), MSG_WAITALL);

		// FIXME: check for error from recv
		//			add check for correct return value from server
		if (count <= 0)
		{
			printf("Server closed connection\n");
			break;
		}

		//printf("Received %d bytes from the server: character count returned : ", count);
		//fwrite(buffer, 1, count, stdout);
		//printf("'\nSending text.\n");

		// free buffer after use
		free(buffer);

		// send the text
		send(sock, line.c_str(), line.size(), 0);

		//printf("Sent text.\n");
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