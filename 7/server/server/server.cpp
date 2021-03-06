/*	AG0907 Lab 7 server example - by Henry Fortuna and Adam Sampson

    This works the same way as Lab 5's select-based server (to start with).
	When a client connects, it gets sent a welcome message; otherwise,
	this just prints out any messages it receives.
*/

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <list>
#include <stdio.h>

#include "utils.h"
#include "protocol.h"

#pragma comment(lib, "ws2_32.lib")


// The IP address for the server to listen on
#define SERVERIP "127.0.0.1"

// The TCP port number for the server to listen on
#define SERVERPORT 5555

// The message ID to use for socket event messages
#define WM_SOCKET (WM_USER + 1)

// Handle for our window.
HWND window;

// The list of clients currently connected to the server.
class Client; //Forward decleration
std::list<Client *> clients;

bool addClientToList(UINT_PTR socket);

// Prototype functions
void registerWindowClass(HINSTANCE hInstance);
void openWindow(HINSTANCE hInstance, int nCmdShow);
void drawWindow();
LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
Client* getClientViaSocketAddress(UINT_PTR socket, std::list<Client *> &clientList);
bool addClientToList(UINT_PTR socket, std::list<Client*> &clientList);
bool removeClient(Client *client_, std::list<Client*> &clientList);

// Each instance of this class represents a connected client.
class Client {
public:
	// Constructor.
	// sock: the socket that we've accepted the client connection on.
	Client(SOCKET sock)
		: sock_(sock)
		, recv_count_(0)
		, send_count_(0)
		, clientWantsQuit(false)
	{
		printf("New connection\n");
	}

	// Destructor.
	~Client()
	{
		printf("Closing connection\n");
		closesocket(sock_);
	}

	// Process an incoming message.
	void processMessage(const NetMessage *message)
	{
		printf("Got network message: type %d, data %d\n", message->type, message->data);
	}

	// Add an outgoing message to the send buffer.
	void sendMessage(const NetMessage *message)
	{
		if (send_count_ + sizeof(NetMessage) > sizeof(send_buf_))
		{
			die("send_buf_ full");
		}

		memcpy(&send_buf_[send_count_], message, sizeof(NetMessage));
		send_count_ += sizeof(NetMessage);
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
		// At present, we always do.
		return true;
	}

	// Return whether this connection is in a state where we want to try
	// writing to the socket.
	bool wantWrite()
	{
		// Only if we've got data to send.
		return send_count_ > 0;
	}

	// Call this when the socket is ready to read.
	// Returns true if the socket should be closed.
	bool doRead()
	{
		// Receive as much data from the client as will fit in the buffer.
		int count = recv(sock_, &recv_buf_[recv_count_], (sizeof recv_buf_) - recv_count_, 0);
		if (count <= 0)
		{
			printf("Client connection closed or broken\n");
			return true;
		}

		printf("Received %d bytes from the client (total %d)\n", count, recv_count_);
		recv_count_ += count;

		// Have we received a complete message?
		if (recv_count_ == sizeof NetMessage)
		{
			processMessage((const NetMessage *) recv_buf_);
			recv_count_ = 0;
		}

		return false;
	}

	// Call this when the socket is ready to write.
	// Returns true if the socket should be closed.
	bool doWrite()
	{
		int count = send(sock_, send_buf_, send_count_, 0);
		if (count <= 0)
		{
			printf("Client connection closed or broken\n");
			return true;
		}

		send_count_ -= count;
		printf("Sent %d bytes to the client (%d left)\n", count, send_count_);

		// Remove the sent data from the start of the buffer.
		memmove(send_buf_, &send_buf_[count], send_count_);

		return false;
	}

	bool setClose(void)
	{
		clientWantsQuit = true;
		return true;
	}

	bool getClose(void)
	{
		return clientWantsQuit;
	}

private:
	SOCKET sock_;

	// Buffer for incoming messages.
	char recv_buf_[sizeof NetMessage];
	int recv_count_;

	// Buffer for outgoing messages.
	char send_buf_[100 * sizeof NetMessage];
	int send_count_;

	bool clientWantsQuit;
};

// Entry point for the program.
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow)
{
	openConsoleWindow();
	startWinSock();

	printf("Server starting\n");

	registerWindowClass(hInstance);
	openWindow(hInstance, nCmdShow);

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
	serverAddr.sin_port = htons(SERVERPORT);

	// Bind the server socket to that address.
	if (bind(serverSocket, (const sockaddr *) &serverAddr, sizeof(serverAddr)) != 0)
	{
		die("bind failed");
	}

	printf("Server socket bound to port %d\n", ntohs(serverAddr.sin_port));

	// Make the socket listen for connections.
	if (listen(serverSocket, 1) != 0)
	{
		die("listen failed");
	}

	printf("Server socket listening\n");



	// Call async select to handle message through windows message loop
	if (WSAAsyncSelect(serverSocket, window, WM_SOCKET, FD_CLOSE | FD_ACCEPT | FD_READ | FD_WRITE | FD_CONNECT ) == SOCKET_ERROR)
	{
		die("WSAAsyncSelect failed");
	}

	// Used in Windows message loop
	MSG msg;

	// The server's main loop, where we'll wait for new connections to
	// come in, or for new data to appear on our existing connections.
	while (GetMessage(&msg, NULL, 0, 0))
	{
		// Windows message loop
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// We won't actually get here, but if we did then we'd want to clean up...
	printf("Quitting\n");
	closesocket(serverSocket);
	WSACleanup();

	return 0;
}

// Register a window class for our window.
void registerWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

	wcex.cbSize = sizeof (wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	// Use our window procedure to handle messages for this kind of window.
	wcex.lpfnWndProc = windowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = 0;
	wcex.hCursor = LoadCursor (NULL, IDC_ARROW);

	wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WindowClass";
	wcex.hIconSm = 0;

	RegisterClassEx(&wcex);
}

// Create and open our window.
void openWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Get console window for position of new Window
	RECT consoleWindowRect;
	if(getConsoleInit())
	{
		getConsoleWindowRect(consoleWindowRect);
	}
	else
	{
		consoleWindowRect.left = 0;
		consoleWindowRect.bottom = 500;
	}

	window = CreateWindow (	L"WindowClass",
							L"Client",
							WS_OVERLAPPEDWINDOW,
							consoleWindowRect.left, consoleWindowRect.bottom,
							400, 200,
							NULL,
							NULL,
							hInstance,
							NULL);
	if (!window)
	{
		die("CreateWindow failed");
	}
	ShowWindow(window, nCmdShow);
	UpdateWindow(window);
}

// Draw the contents of the window.
void drawWindow()
{
	std::wstring message = L"Server program\n\n";
	message += L"Press Esc to exit\n";

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(window, &ps);
	RECT rt;
	GetClientRect(window, &rt);
	DrawText(hdc, message.c_str(), message.size(), &rt, DT_LEFT);
	EndPaint(window, &ps);
}

// Process messages for our window.
LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_PAINT:
		drawWindow();
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		// A keypress.
		switch(wParam)
		{
		case VK_ESCAPE:
			PostQuitMessage(0);
			break;
		}
		break;

	// A user defined message
	case WM_SOCKET:
		// A socket event message.
		// We can look at wParam to figure out which socket it was 
		printf("Event on socket %ld\n", (long) wParam);

		long error = WSAGETSELECTERROR(lParam);
		if (error != 0)
		{
			// Something went wrong with one of the asynchronous operations.
			fprintf(stderr, "WM_SOCKET error %ld on socket %ld\n", error, (long) wParam);
			die("asynchronous socket operation failed");
		}
		
		// See if it as a new client 
		//! This should contain both messages
		if(WSAGETSELECTEVENT(lParam) == FD_CONNECT || WSAGETSELECTEVENT(lParam) == FD_ACCEPT)
		{
			// If it is add them
			if(!addClientToList(wParam, clients))
			{
				// If it fails don't add them 
				die("Add clients to list failed.");
			}
		}

		Client *client = getClientViaSocketAddress(wParam, clients);

		if(client != 0)
		{
			// What kind of event was it?
			switch (WSAGETSELECTEVENT(lParam))
			{
			case FD_ACCEPT:
				// connect() completed.
				// THIS IS HANDLED ABOVE
				printf("  FD_ACCEPT\n");
				die("FD_CONNECT is not currently handled, don't think you should have got this message :(!\n");
				/*
				if(!addClientToList(wParam, clients))
				{
					die("Add clients to list failed.");
				}
				*/ 
				break;

			case FD_READ:
				// It may be possible to receive.
				printf("  FD_READ\n");
				//tryToRead();	
				if(client->wantRead())
				{
					client->doRead();
				}
				break;

			case FD_WRITE:
				// It may be possible to send.
				// We will only get this notification if we've already tried to send
				// and been told that it would block (which is different from select's behaviour).
				printf("  FD_WRITE\n");
				//tryToWrite();
				if(client->wantWrite())
				{
					client->doWrite();
				}
				break;

			case FD_CLOSE:
				{
					printf("Got close message from socket\n");

					client->setClose();

					if(client->getClose())
					{
						if(!removeClient(client, clients))
						{
							die("Removed client failed, client doesn't exist within the list.\n");
						}
						else
						{
							printf("Client on sock %ld has been removed from client list.\n", (long) wParam);
						}
					}

				}// END FD_CLOS
				break;
			}// END switch (WSAGETSELECTEVENT(lParam))
		}
		else
		{
			//die("NO CLIENT WAS FOUND BUT WM_SOCKET WAS HIT, SHOULDNT HAPPEN\n");
		}
		// WM_SOCKET break
		break;
	}
	// Pass messages down to the default handler.
	return DefWindowProc(hWnd, message, wParam, lParam);
}

Client* getClientViaSocketAddress(UINT_PTR socket, std::list<Client*> &clientList)
{
	for (std::list<Client *>::iterator it = clientList.begin(); it != clientList.end(); ++it)
	{
		Client *client = *it;

		if(client->sock() == socket)
		{
			return client;
		}
		
		if(clientList.end() == it)
		{
			return 0;
		}
	}
	return 0;
}

// Returns true if accepted socket, else returns false
bool addClientToList(UINT_PTR socket, std::list<Client*> &clientList)
{
	// Accept a new connection to the server socket.
	// This gives us back a new socket connected to the client, and
	// also fills in an address structure with the client's address.
	sockaddr_in clientAddr;
	int addrSize = sizeof(clientAddr);
	SOCKET clientSocket = accept(socket, (sockaddr *) &clientAddr, &addrSize);
	if (clientSocket == INVALID_SOCKET)
	{
		printf("accept failed\n");
		return false;
	}

	// Create a new Client object, and add it to the collection.
	Client *client = new Client(clientSocket);
	clientList.push_back(client);

	// Send the new client a welcome message.
	NetMessage message;
	message.type = MT_WELCOME;
	client->sendMessage(&message);

	return true;
}

bool removeClient(Client *client_, std::list<Client*> &clientList)
{
	for (std::list<Client *>::iterator it = clientList.begin(); it != clientList.end(); ++it)
	{
		Client *client = *it;

		if(client->sock() == client_->sock())
		{
			clientList.remove(*it);
			
			return true;
		}
	}

	return false;
}