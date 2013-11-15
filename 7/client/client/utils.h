/* Utility functions for the client and server */

#ifndef UTILS_H
#define UTILS_H

#include <vector>

// Print an error message and die.
void die(const char *message);

// Initialise the WinSock library.
void startWinSock();

// In a Win32 application, open a console window and redirect the
// standard input, output and error streams to it.
void openConsoleWindow();

void moveConsoleToCenterOfDestop();

void moveConsoleToVisualStudioInstance();

void getConsoleWindowRect(RECT &rect);

// See if the console has been init
bool getConsoleInit();

// 
void GetProcessName(DWORD processID);

BOOL GetParentPID( DWORD dwParentId, DWORD &dwChildProcessId, bool &bChildProcess, std::vector<DWORD> &listParentIds);

ULONG_PTR GetParentProcessId(); // By Napalm @ NetCore2K

struct paramsForEnumWindows
{
	ULONG_PTR processId;
	HWND windowHwnd;
};

BOOL CALLBACK EnumWindowsCallbackGetHwndFromProcessIdOfActiveWindow(HWND hWnd, LPARAM lParam);

#endif