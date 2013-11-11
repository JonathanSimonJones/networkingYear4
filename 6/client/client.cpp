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

#include <WinBase.h>
#include "..\SharedFiles\Message.hpp"

#pragma comment(lib, "ws2_32.lib")


// The IP address of the server
#define SERVERIP "127.0.0.1"

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

	Message player; 
	player.objectID = time(NULL);
	player.x = 0;
	player.y = 0;

	Message recievedMsg;
	recievedMsg.objectID = 0;
	recievedMsg.x = 0;
	recievedMsg.y = 0;

	do {
		player.x += 1;
		player.y += 2;
		// Send the message to the server.
		sendto(sock, (char *)&player, sizeof(Message), 0,
			   (const sockaddr *) &toAddr, sizeof(toAddr));
		// FIXME: check for error from sendto

		// Read a response back from the server (or from anyone, in fact).
		sockaddr_in fromAddr;
		int fromAddrSize = sizeof(fromAddr);
		int count = recvfrom(sock, (char *)&recievedMsg, sizeof(Message), 0,
			                 (sockaddr *) &fromAddr, &fromAddrSize);
		// FIXME: check for error from recvfrom

		/*
		printf("Received %d bytes from address %s port %d: '",
			   count, inet_ntoa(fromAddr.sin_addr), ntohs(fromAddr.sin_port));
		fwrite(buffer, 1, count, stdout);
		printf("'\n");
		*/
		printf("Players dets from server:\nid: %d \nx: %d , \ny: %d \n", recievedMsg.objectID, recievedMsg.x, recievedMsg.y);

		Sleep(1000);	// Hold execution of programme for 1000 milliseconds

		// Keep going unt get a message starting with "quit".
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