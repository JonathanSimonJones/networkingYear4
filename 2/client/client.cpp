/*	AG0907 TCP client example - by Henry Fortuna and Adam Sampson

	A simple client that connects to a server and waits for
	a response. After initial connection is made, the server must send
	the character "!" to initiate a proper connection sequesnce.
	Text typed is then sent to the server which echos it back;
	and the response is printed out.

	There is nothing elegant about this code as you will see.
	Its purpose is to illustrate enough to get started coding
	with WinSock.
*/

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")


// The IP address of the server to connect to
#define SERVERIP "127.0.0.1"

// The TCP port number on the server to connect to
#define SERVERPORT 5555

// The (fixed) size of message that we send between the two programs
#define MESSAGESIZE 40


// Prototypes
void die(const char *message);
int checkValidityOfMessageSize(char *message);
void invalidateMessage(char *message);

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
	SOCKET sock = socket(AF_INET, SOCK_STREAM, -1);
	// FIXME: check for errors from socket
	if (sock == INVALID_SOCKET )
	{
		die("Invalid socket, exiting");		// Hits is sever isn't active at time of connect by client
	}

	// Fill out a sockaddr_in structure with the address that
	// we want to connect to.
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	// htons converts the port number to network byte order (big-endian).
	addr.sin_port = SERVERPORT;
	addr.sin_port = htons(SERVERPORT);
	addr.sin_addr.s_addr = inet_addr(SERVERIP);
	
	printf("IP address to connect to: ");
	printf("%d.", addr.sin_addr.S_un.S_un_b.s_b1);
	printf("%d.", addr.sin_addr.S_un.S_un_b.s_b2);
	printf("%d.", addr.sin_addr.S_un.S_un_b.s_b3);
	printf("%d\n", addr.sin_addr.S_un.S_un_b.s_b4);

	printf("Port number to connect to: ");
	// ntohs does the opposite of htons.
	int check = ntohs(addr.sin_port);
	printf("%d\n\n", ntohs(addr.sin_port));


	// Connect the socket to the server.
	if (connect(sock, (LPSOCKADDR) &addr, sizeof addr) == SOCKET_ERROR)
	{
		die("connect failed");		// Hits is sever isn't active at time of connect by client
	}

	printf("Connected to server\n");

	// We'll use this buffer to hold what we receive from the server.
	char buffer[MESSAGESIZE];

	// We expect the server to send us a welcome message ("hello") when we connect.

	// Receive a message.
	recv(sock, buffer, MESSAGESIZE, 0);
	// FIXME: check for errors, or for unexpected message size

	// Check it's what we expected.
	if (memcmp(buffer, "hello", 5) != 0)
	{
		die("Expected \"hello\" upon connection, but got something else");
	}

	while (true)
	{
		printf("Type some text (\"quit\" to exit): ");
		fflush(stdout);

		// Read a line of text from the user.
		char line[MESSAGESIZE];
		fgets(line, MESSAGESIZE, stdin);
		
		// Copy the line into the buffer, filling the rest with dashes.
		// (Length - 1 because we don't want to copy the \n at the end.)
		memset(buffer, '-', MESSAGESIZE);
		memcpy(buffer, line, strlen(line) - 1);

		// Send the message to the server.
		send(sock, buffer, MESSAGESIZE, 0);
		// FIXME: check for error from send

		// Read a response back from the server.
		int count = recv(sock, buffer, MESSAGESIZE, 0);
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

// Checks the validity of the message being passed by the defined MESSAGESIZE
int checkValidityOfMessageSize(char *message)
{
	// Check to see if message is too big
	if(strlen(message) > MESSAGESIZE)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

// Changes the contents of message
void invalidateMessage(char *message)
{
	memcmp(message, "Invalid message size", 20);
}