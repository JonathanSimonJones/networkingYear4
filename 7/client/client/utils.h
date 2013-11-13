/* Utility functions for the client and server */

#ifndef UTILS_H
#define UTILS_H

// Print an error message and die.
void die(const char *message);

// Initialise the WinSock library.
void startWinSock();

// In a Win32 application, open a console window and redirect the
// standard input, output and error streams to it.
void openConsoleWindow();

//! Currently forces specified in the function
void moveConsoleToCenterOfDestop();

void getConsoleWindowRect(RECT &rect);

// See if the console has been init
bool getConsoleInit();

// 
void GetProcessName(DWORD processID);

#endif