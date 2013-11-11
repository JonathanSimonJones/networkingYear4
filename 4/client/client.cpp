/*	AG0907 UDP client example - by Henry Fortuna and Adam Sampson

	When the user types a message, the client sends it to the server
	as a UDP packet. The server then sends a packet back to the
	client, and the client prints it out.
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <winsock2.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")


// The IP address of the server
#define SERVERIP "193.60.172.31"

// The UDP port number on the server
#define SERVERPORT 4444

// The (fixed) size of message that we send between the two programs
#define MESSAGESIZE 40


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

	// Create a UDP socket.
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	// FIXME: check for errors from socket

	// Fill out a sockaddr_in structure with the address that
	// we want to send to.
	sockaddr_in toAddr;
	toAddr.sin_family = AF_INET;
	// htons converts the port number to network byte order (big-endian).
	toAddr.sin_port = htons(SERVERPORT);
	toAddr.sin_addr.s_addr = inet_addr(SERVERIP);

	// We'll use this buffer to hold the messages we exchange with the server.
	char buffer[MESSAGESIZE];

	/*
	int maxMessageOutgoingSize = 0;
	int optLen = sizeof(int);
	// Get max message size
	if(getsockopt(sock, SOL_SOCKET, SO_MAX_MSG_SIZE, (char*)&maxMessageOutgoingSize, &optLen) != 0)
	{
		die("fail on message max size allowence.");
	}

	// Check max message size 
	if(MESSAGESIZE > maxMessageOutgoingSize)
	{
		die("MESSAGESIZE has been defined too big fool.");
	}
	*/

	bool isFirstTime = true;
	// Seed the rand
	srand(time(NULL));
	do {
		printf("Type some text (\"quit\" to exit): ");
		fflush(stdout);

		// Read a line of text from the user.
		std::string line;
		std::getline(std::cin, line);
		// Now "line" contains what the user typed.

		// Copy the line into the buffer, filling the rest with dashes.
		memset(buffer, '-', MESSAGESIZE);
		memcpy(buffer, line.c_str(), line.size());
		// FIXME: if line.size() is bigger than the buffer it'll overflow (and likely corrupt memory)

		if(isFirstTime || (rand() % 100) < 90 ) // Fail to send messages 10 % of the time
		{
			// Send the message to the server.
			sendto(sock, buffer, MESSAGESIZE, 0,
				   (const sockaddr *) &toAddr, sizeof(toAddr));
			// FIXME: check for error from sendto
		}
		isFirstTime = false;

		// Read a response back from the server (or from anyone, in fact).
		sockaddr_in fromAddr;
		int fromAddrSize = sizeof(fromAddr);
		int count = recvfrom(sock, buffer, MESSAGESIZE, 0,
			                 (sockaddr *) &fromAddr, &fromAddrSize);
		if (count < 0) {
			die("I got a wacky count from recvfrom");
		}
		// FIXME: check for error from recvfrom

		printf("Received %d bytes from address %s port %d: '",
			   count, inet_ntoa(fromAddr.sin_addr), ntohs(fromAddr.sin_port));
		fwrite(buffer, 1, count, stdout);
		printf("'\n");

		// Keep going until we get a message starting with "quit".
	} while (memcmp(buffer, "quit", 4) != 0);

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