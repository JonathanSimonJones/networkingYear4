/*	AG0907 Lab 7 event-based client example - by Henry Fortuna and Adam Sampson

    Open a Win32 window, and send NetMessage structures to the server when
	keys are pressed. Print out any NetMessages received.
*/

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <string>

#include "utils.h"
#include "protocol.h"

#pragma comment(lib, "ws2_32.lib")


// The IP address of the server to connect to
#define SERVERIP "127.0.0.1"

// The TCP port number on the server to connect to
#define SERVERPORT 5555

// The message ID to use for socket event messages.
#define WM_SOCKET (WM_USER + 1)


// Handle for our window.
HWND window;

// The socket that we'll connect to the server.
SOCKET sock;

// Are we connected to the server?
bool connected = false;

// Buffer for received data.
// Since a message might be received in several pieces, we need a buffer
// to store as much as we've received so far.
char recv_buf[sizeof NetMessage];
int recv_count = 0;

// Buffer for data to be sent.
// Since we might not be able to send all of a message immediately,
// we need to store what we've got left to send.
char send_buf[100 * sizeof NetMessage];
int send_count = 0;

// Prototypes
void registerWindowClass(HINSTANCE hInstance);
void openWindow(HINSTANCE hInstance, int nCmdShow);
void drawWindow();
LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void processMessage(const NetMessage *message);
void sendMessage(const NetMessage *message);
void tryToRead();
void tryToWrite();


// Entry point for the program.
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow)
{
	HWND parent = GetAncestor(GetConsoleWindow(), GA_ROOT);
	//LWSTR parentName[20];
	
	MONITORINFO monInfo;
	monInfo.cbSize = sizeof(MONITORINFO);
	HMONITOR hMonitor = MonitorFromWindow(parent, MONITOR_DEFAULTTONEAREST);
	GetMonitorInfo((HMONITOR)hMonitor, &monInfo);

	openConsoleWindow();
	startWinSock();

	printf("Client starting\n");

	registerWindowClass(hInstance);
	openWindow(hInstance, nCmdShow);

	// Create the socket.
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == SOCKET_ERROR)
	{
		die("socket failed");
	}

	printf("Created socket %ld\n", (long) sock);

	// Enable WinSock events for the socket.
	// (This also puts the socket into non-blocking mode.)
	if (WSAAsyncSelect(sock, window, WM_SOCKET, FD_CLOSE | FD_CONNECT | FD_READ) == SOCKET_ERROR)
	{
		die("WSAAsyncSelect failed");
	}

	// The standard Win32 message loop -- fetch messages and dispatch them to the appropriate window procedure.
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
    {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

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
	std::wstring message = L"Client program\n\n";
	message += L"Press C to connect\n";
	message += L"Once connected, press letter keys to send\n";
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

		case 'C':
			if (!connected)
			{
				// Start connecting to the server.
				printf("Connecting...\n");

				// The address to connect to.
				sockaddr_in addr;
				addr.sin_family = AF_INET;
				addr.sin_port = htons(SERVERPORT);
				addr.sin_addr.s_addr = inet_addr(SERVERIP);

				if (connect(sock, (const sockaddr *) &addr, sizeof addr) == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						// This is what we expect -- the call *would* have blocked,
						// so it returns the special error code indicating that.
						// But because we've enabled events for it, it's actually
						// connecting and will send us a message once it's done.
					}
					else
					{
						// Something else went wrong!
						die("connect failed");
					}
				}
			}
			break;
		}

		if (wParam >= 'A' && wParam <= 'Z' && connected)
		{
			// Send a keypress message to the server.
			NetMessage message;
			message.type = MT_KEYPRESS;
			message.data = wParam;

			sendMessage(&message);
		}

		break;

	case WM_SOCKET:
		// A socket event message.
		// We can look at wParam to figure out which socket it was (but we only have one here anyway).
		printf("Event on socket %ld\n", (long) wParam);

		long error = WSAGETSELECTERROR(lParam);
		if (error != 0)
		{
			// Something went wrong with one of the asynchronous operations.
			fprintf(stderr, "WM_SOCKET error %ld on socket %ld\n", error, (long) wParam);
			die("asynchronous socket operation failed");
		}

		// What kind of event was it?
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_CONNECT:
			// connect() completed.
			printf("  FD_CONNECT\n");
			connected = true;
			break;

		case FD_READ:
			// It may be possible to receive.
			printf("  FD_READ\n");
			tryToRead();			
			break;

		case FD_WRITE:
			// It may be possible to send.
			// We will only get this notification if we've already tried to send
			// and been told that it would block (which is different from select's behaviour).
			printf("  FD_WRITE\n");
			tryToWrite();
			break;
		}

		break;
	}

	// Pass messages down to the default handler.
	return DefWindowProc(hWnd, message, wParam, lParam);
}

// Try to read, until we're told that reading will block.
void tryToRead()
{
	while (true)
	{
		// Receive into whatever's left of the receive buffer.
		int count = recv(sock, &recv_buf[recv_count], (sizeof recv_buf) - recv_count, 0);
		if (count == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// That's as much data as we're going to get this time!
				// We need to wait for another FD_READ message.
				break;
			}
			else
			{
				// Something went wrong.
				die("recv failed");
			}
		}
		else if (count == 0)
		{
			// The server closed the connection.
			die("connection closed");
		}
		else
		{
			// We got some data!
			printf("  Received %d bytes (total %d so far)\n", count, recv_count);
			recv_count += count;

			// Have we received a complete message?
			if (recv_count == sizeof(NetMessage))
			{
				processMessage((const NetMessage *) recv_buf);
				recv_count = 0;
			}
		}
	}
}

// Try to send data, until we're told that sending will block or we run out of data.
void tryToWrite()
{
	while (send_count > 0)
	{
		// Send as much data as we can.
		int count = send(sock, send_buf, send_count, 0);
		if (count == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// We've sent as much data as we can.
				// We need to wait for another FD_WRITE message.
				break;
			}
			else
			{
				// Something went wrong.
				die("send failed");
			}
		}
		else if (count == 0)
		{
			// The server closed the connection.
			die("connection closed");
		}
		else
		{
			// We sent some data!
			send_count -= count;
			printf("  Sent %d bytes (%d left)\n", count, send_count);

			// Remove the sent data from the start of the buffer.
			memmove(send_buf, &send_buf[count], send_count);
		}
	}
}

// Process a message received from the network.
void processMessage(const NetMessage *message)
{
	printf("Got network message: type %d, data %d\n", message->type, message->data);
}

// Add an outgoing message to the send buffer.
void sendMessage(const NetMessage *message)
{
	if (send_count + sizeof(NetMessage) > sizeof(send_buf))
	{
		die("send_buf full");
	}

	memcpy(&send_buf[send_count], message, sizeof(NetMessage));
	send_count += sizeof(NetMessage);

	// Try sending it immediately.
	tryToWrite();
}