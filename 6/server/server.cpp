/*	AG0907 UDP server example - by Henry Fortuna and Adam Sampson

	When the user types a message, the client sends it to the server
	as a UDP packet. The server then sends a packet back to the
	client, and the client prints it out.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#include "..\SharedFiles\Message.hpp"

#pragma comment(lib, "ws2_32.lib")


// The IP address for the server
#define SERVERIP "127.0.0.1"

// The UDP port number for the server
#define SERVERPORT 4444

// The (fixed) size of message that we send between the two programs
#define MESSAGESIZE 40


// Prototypes
void die(const char *message);


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

	// Create a UDP socket.
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	// FIXME: we should test for error here

	// Fill out a sockaddr_in structure to describe the address we'll listen on.
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(SERVERIP);
	// htons converts the port number to network byte order (big-endian).
	serverAddr.sin_port = htons(SERVERPORT);

	// Bind the socket to that address.
	if (bind(sock, (LPSOCKADDR) &serverAddr, sizeof(serverAddr)) != 0)
	{
		die("bind failed");
	}

	// ntohs does the opposite of htons.
	printf("Server socket bound to port %d\n", ntohs(serverAddr.sin_port));

	// We'll use this array to hold the messages we exchange with the client.
	char buffer[MESSAGESIZE];

	Message player; 
	player.objectID = 0;
	player.x = 0;
	player.y = 0;

	while (true)
	{
		printf("Waiting for player position...\n");

		sockaddr_in fromAddr;
		int fromAddrSize = sizeof(fromAddr);
		int count = recvfrom(sock, (char *)&player, sizeof(Message), 0,
			                 (sockaddr *) &fromAddr, &fromAddrSize);
		// FIXME: check for error from recvfrom

		/*
		printf("Received %d bytes from address %s port %d: '",
			   count, inet_ntoa(fromAddr.sin_addr), ntohs(fromAddr.sin_port));
		fwrite(buffer, 1, count, stdout);
		printf("'\n");
		*/
		printf("Player:\nid: %d \nx: %d , \ny: %d \n", player.objectID, player.x, player.y);
		
		// Send the same data back to the address it came from.
		sendto(sock, (char *)&player, sizeof(Message), 0,
			   (const sockaddr *) &fromAddr, sizeof(fromAddr));
		// FIXME: check for errors from sendto
	}

	// We won't actually get here, but if we did then we'd want to clean up...
	printf("Quitting\n");
	closesocket(sock);
	WSACleanup();
	return 0;
}


// Print an error message and exit.
void die(const char *message) {
	fprintf(stderr, "Error: %s\n", message);
	exit(1);
}