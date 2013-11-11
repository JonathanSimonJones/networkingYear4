/* Utility functions for the client and server */

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <io.h>
#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#pragma comment(lib, "ws2_32.lib")


void die(const char *message) {
	fprintf(stderr, "Error: %s (WSAGetLastError() = %d)\n", message, WSAGetLastError());

#ifdef _DEBUG
	// Debug build -- drop the program into the debugger.
	abort();
#else
	exit(1);
#endif
}

void startWinSock() {
	// We want version 2.2.
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
}

static void handleToConsole(DWORD handle, FILE *stream, const char *mode)
{
	long lStdHandle = (long) GetStdHandle(STD_OUTPUT_HANDLE);
	int hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	*stream = *_fdopen(hConHandle, "w");
	setvbuf(stream, NULL, _IONBF, 0);
}

void openConsoleWindow()
{
	// Allocate a console for this app.
	AllocConsole();

	// Make the screen buffer big enough to let us scroll text.
	const WORD MAX_CONSOLE_LINES = 500;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// Redirect the C IO streams (stdout etc.) to the console.
	handleToConsole(STD_INPUT_HANDLE, stdin, "r");
	handleToConsole(STD_OUTPUT_HANDLE, stdout, "w");
	handleToConsole(STD_ERROR_HANDLE, stderr, "w");

	// Redirect the C++ IO streams (cout etc.) to the console.
	std::ios::sync_with_stdio();


	//Repositon the console window
	//?Check with sampson how to move console using below function
	SMALL_RECT consolePosition;
	consolePosition.Left = 1980 * 0.5;
	consolePosition.Top = 1080 * 0.5;
	consolePosition.Right = consolePosition.Left + coninfo.dwMaximumWindowSize.X - 10;
	consolePosition.Bottom = consolePosition.Top + coninfo.dwMaximumWindowSize.Y - 10;
	
	//This is what wasn't working, don't know why it just didn't move the window of the console
	//Tried to find a update function to force windows to redraw window but couldn't find one
	//bool check = SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &consolePosition); 
	//DWORD error = GetLastError();

	// Get handle to console so that we can use it to get info about the window enclosing the console
	HWND hwnToConsoleWindow = GetConsoleWindow();
	WINDOWINFO consoleWinInfo;
	consoleWinInfo.cbSize = sizeof(WINDOWINFO);
	//Fill out console info struct	
	GetWindowInfo(hwnToConsoleWindow, (PWINDOWINFO) &consoleWinInfo);

	// Get handle to desktop window for dimensions
	HWND hDesktop = GetDesktopWindow();
	WINDOWINFO desktopWin;
	desktopWin.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(hDesktop, (PWINDOWINFO) &desktopWin);
	//Reposition the consoles window
	bool check = MoveWindow(hwnToConsoleWindow,
		desktopWin.rcWindow.top + 100,
		0,//(desktopWin.rcWindow.right - desktopWin.rcWindow.left) * 0.5,
		consoleWinInfo.rcWindow.right - consoleWinInfo.rcWindow.left,
		consoleWinInfo.rcWindow.bottom - consoleWinInfo.rcWindow.top,
		TRUE);

	GetWindowInfo(hwnToConsoleWindow, (PWINDOWINFO) &consoleWinInfo);
	int foobar = 0;
}