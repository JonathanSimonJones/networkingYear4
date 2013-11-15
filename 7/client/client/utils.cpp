/* Utility functions for the client and server */

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>
#include <io.h>
#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <Psapi.h>
#include <tchar.h>
#include <cstring>
#include <wchar.h>
#include <vector>
#include <TlHelp32.h>


#include "utils.h"

#pragma comment(lib, "ws2_32.lib")

bool consoleInit = false;


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

	moveConsoleToVisualStudioInstance();

	consoleInit = true;
}

void moveConsoleToCenterOfDestop()
{
	//Repositon the console window
	//?Check with sampson how to move console using below function
	//SMALL_RECT consolePosition;
	//consolePosition.Left = 1980 * 0.5;
	//consolePosition.Top = 1080 * 0.5;
	//consolePosition.Right = consolePosition.Left + coninfo.dwMaximumWindowSize.X - 10;
	//consolePosition.Bottom = consolePosition.Top + coninfo.dwMaximumWindowSize.Y - 10;
	
	//This is what wasn't working, don't know why it just didn't move the window of the console
	//Tried to find a update function to force windows to redraw window but couldn't find one
	//bool check = SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &consolePosition); 
	//DWORD error = GetLastError();

	// Get handle to console so that we can use it to get info about the window enclosing the console
	WINDOWINFO consoleWinInfo;
	consoleWinInfo.cbSize = sizeof(WINDOWINFO);
	//Fill out console info struct	
	GetWindowInfo(GetConsoleWindow(), (PWINDOWINFO) &consoleWinInfo);

	// Get desktop window dimensions info
	WINDOWINFO desktopWin;
	desktopWin.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(GetDesktopWindow(), (PWINDOWINFO) &desktopWin);

	// Get current monitor info
	//MONITORINFO monInfo;
	//monInfo.cbSize = sizeof(MONITORINFO);
	//HMONITOR hMonitor = MonitorFromWindow(GetConsoleWindow(), MONITOR_DEFAULTTONEAREST);
	//GetMonitorInfo((HMONITOR)hMonitor, &monInfo);

	// For multiple screen set up get dimensions of single monitor
	RECT singleScreenRec;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &singleScreenRec, 0);

	int numberOfScreens = GetSystemMetrics(SM_CMONITORS);
	int top = 0;
	int left = 0;
	int width = consoleWinInfo.rcWindow.right - consoleWinInfo.rcWindow.left;
	int height = consoleWinInfo.rcWindow.bottom - consoleWinInfo.rcWindow.top;
	int offsetTop = 0;
	int offsetLeft = singleScreenRec.right * 0.5;

	// Calculate top of console window
	// If console window was started above main desktop screen
	if( consoleWinInfo.rcWindow.top < desktopWin.rcWindow.top)
	{
		top = desktopWin.rcWindow.top - consoleWinInfo.rcWindow.top;
	}
	// If console window was started below desktop main screen
	else if(consoleWinInfo.rcWindow.top > desktopWin.rcWindow.bottom)
	{
		top = desktopWin.rcWindow.bottom + consoleWinInfo.rcWindow.top;
	}
	else
	{
		top = consoleWinInfo.rcWindow.top;
	}

	// Calculate bottom of window
	// If console window was started on the left of main desktop screen
	if( consoleWinInfo.rcWindow.left < desktopWin.rcWindow.left)
	{
		left = desktopWin.rcWindow.left - consoleWinInfo.rcWindow.left;
	}
	// If console window was started right of desktop main screen
	else if( consoleWinInfo.rcWindow.left > desktopWin.rcWindow.right)
	{
		left = desktopWin.rcWindow.right + consoleWinInfo.rcWindow.left;
	}
	else
	{
		left = consoleWinInfo.rcWindow.left;
	}
	
	//Reposition the consoles window
	bool check = MoveWindow(GetConsoleWindow(),
		left + offsetLeft,
		top + offsetTop,
		width,
		height,
		TRUE);
}

void moveConsoleToVisualStudioInstance()
{
	// Get parent processId of console
	ULONG_PTR parentProcessId = GetParentProcessId();

	// Create struct to pass parentProcessId to enum window callback while also using struct to get a hWnd for instance of visual studio that created the console
	paramsForEnumWindows winEnumParams;
	winEnumParams.processId = parentProcessId;
	// Iterate over all open windows to get hWnd for visual studio instance
	BOOL enumSuccess = EnumDesktopWindows(GetThreadDesktop(GetCurrentThreadId()),EnumWindowsCallbackGetHwndFromProcessIdOfActiveWindow, (LPARAM)&winEnumParams);

	// Get info of instance of visual studio referenced by hWnd returned through EnumDesktopWindows
	WINDOWINFO visualStudioWin;
	visualStudioWin.cbSize = sizeof(WINDOWINFO);
	// Get window info from handle
	GetWindowInfo(winEnumParams.windowHwnd, (PWINDOWINFO) &visualStudioWin);

	// Get handle to console so that we can use it to get info about the window enclosing the console
	WINDOWINFO consoleWinInfo;
	consoleWinInfo.cbSize = sizeof(WINDOWINFO);
	//Fill out console info struct	
	GetWindowInfo(GetConsoleWindow(), (PWINDOWINFO) &consoleWinInfo);

	// For multiple screen set up get dimensions of single monitor
	RECT singleScreenRec;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &singleScreenRec, 0);

	// Vars for console window
	int top = visualStudioWin.rcWindow.top;
	int left = visualStudioWin.rcWindow.left;
	int width = consoleWinInfo.rcWindow.right - consoleWinInfo.rcWindow.left;
	int height = consoleWinInfo.rcWindow.bottom - consoleWinInfo.rcWindow.top;
	int offsetTop = 100;
	int offsetLeft = 100;
	
	//Reposition the consoles window
	bool check = MoveWindow(GetConsoleWindow(),
		left + offsetLeft,
		top + offsetTop,
		width,
		height,
		TRUE);

	return;
}


void getConsoleWindowRect(RECT &rect)
{
	WINDOWINFO consoleWinInfo;
	consoleWinInfo.cbSize = sizeof(WINDOWINFO);
	//Fill out console info struct	
	GetWindowInfo(GetConsoleWindow(), (PWINDOWINFO) &consoleWinInfo);

	rect = consoleWinInfo.rcWindow;
}

bool getConsoleInit()
{
	return consoleInit;
}


void GetProcessName(DWORD processID)
{
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

	// Get a handle to the process 
	HANDLE hProcess = OpenProcess ( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

	// Get the Process Name
	if (NULL != hProcess)
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded) )
		{
			GetModuleBaseName (hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR) );
		}
	}

	TCHAR compareName[MAX_PATH] = TEXT("devenv.exe");

	if(wcscmp(szProcessName, compareName) == 0)//== L"devenv.exe" )
	{

		printf("Found string finally");
		return;
	}

	_tprintf( TEXT("%s (PID: %u)\n"), szProcessName, processID );

	// Release the handle to the process
	CloseHandle(hProcess);
}

// change this method signature according to your requirements 
BOOL GetParentPID( DWORD dwParentId, DWORD &dwChildProcessId, bool &bChildProcess, std::vector<DWORD> &listParentIds) 
{ 
	OSVERSIONINFO osver ; 
	HINSTANCE hInstLib ;
	HANDLE hSnapShot ; 
	BOOL bContinue ;  

	// ToolHelp Function Pointers. 
	HANDLE (WINAPI *lpfCreateToolhelp32Snapshot)(DWORD,DWORD) ; 
	BOOL (WINAPI *lpfProcess32First)(HANDLE,LPPROCESSENTRY32) ;
	BOOL (WINAPI *lpfProcess32Next)(HANDLE,LPPROCESSENTRY32) ; 

	hInstLib = LoadLibraryA( "Kernel32.DLL" ) ; 
	if( hInstLib == NULL ) { return false; }  

	// Get procedure addresses. 
	lpfCreateToolhelp32Snapshot = (HANDLE(WINAPI *)(DWORD,DWORD)) GetProcAddress( hInstLib, "CreateToolhelp32Snapshot" ) ; 
	lpfProcess32First= (BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32)) GetProcAddress( hInstLib, "Process32First" ) ; 
	lpfProcess32Next= (BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32)) GetProcAddress( hInstLib, "Process32Next" ) ; 

	if( lpfProcess32Next == NULL || lpfProcess32First == NULL || lpfCreateToolhelp32Snapshot == NULL ) 
	{ 
		FreeLibrary( hInstLib ) ; 
		return false ; 
	}  

	// Get a handle to a Toolhelp snapshot of the systems 
	// processes. 
	hSnapShot = lpfCreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 ) ; 
	if( hSnapShot == INVALID_HANDLE_VALUE ) 
	{ 
		FreeLibrary( hInstLib ) ; 
		return false ; 
	}  

	PROCESSENTRY32 procentry;  

	// Get the first process' information. 
	memset((LPVOID)&procentry,0,sizeof(PROCESSENTRY32)); 
	procentry.dwSize = sizeof(PROCESSENTRY32); 
	bContinue = lpfProcess32First( hSnapShot, &procentry );  
	DWORD pid = 0; 

	// While there are processes, keep looping. 
	DWORD crtpid= dwParentId; 

	while( bContinue ) 
	{ 
		if(procentry.th32ParentProcessID == dwParentId) 
		{ 
			// add the ids of the processes with the parent id 
			listParentIds.push_back(procentry.th32ProcessID); 
		}  

		procentry.dwSize = sizeof(PROCESSENTRY32) ; 
		bContinue = lpfProcess32Next( hSnapShot, &procentry );  
	}//while ends   

	// Free the library. 
	FreeLibrary( hInstLib ) ; 
	return true; 
}

ULONG_PTR GetParentProcessId() // By Napalm @ NetCore2K
{
	ULONG_PTR pbi[6];
	ULONG ulSize = 0;
	LONG (WINAPI *NtQueryInformationProcess)(HANDLE ProcessHandle, ULONG ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength); 
	*(FARPROC *)&NtQueryInformationProcess = GetProcAddress(LoadLibraryA("NTDLL.DLL"), "NtQueryInformationProcess");

	if(NtQueryInformationProcess){
		if(NtQueryInformationProcess(GetCurrentProcess(), 0, &pbi, sizeof(pbi), &ulSize) >= 0 && ulSize == sizeof(pbi))
			return pbi[5];
	}
	return (ULONG_PTR)-1;
}

BOOL CALLBACK EnumWindowsCallbackGetHwndFromProcessIdOfActiveWindow(HWND hWnd, LPARAM lParam)
{
	// Get process id of handle to window passed to function
	DWORD processId;
	DWORD errorGetWindowThreadId = GetWindowThreadProcessId(hWnd, &processId);

	// Get process id to compare with from lParam
    paramsForEnumWindows pToCompareProcessIDandReturnHwnd;
	pToCompareProcessIDandReturnHwnd.processId = ((paramsForEnumWindows*)lParam)->processId;
	DWORD compare = (DWORD)pToCompareProcessIDandReturnHwnd.processId;

	// Compare process ids to check for match with current open windows
	if(processId == compare)
	{
		// Get the style of the window
		LONG windowStyle = GetWindowLong(hWnd, GWL_STYLE);

		// Check if the window is visible as this is how we identify the visual studio instance
		if(windowStyle & WS_VISIBLE)
		{
			// Store the handle to the window so that it can be recieved outside of the function
			((paramsForEnumWindows*)lParam)->windowHwnd = hWnd;

			// Let enum windows know that we want to stop enumerating as we have found the correct window handle
			return false;
		}
	}

	// Carry on as we have not found the correct handle to the window
	return true;
}