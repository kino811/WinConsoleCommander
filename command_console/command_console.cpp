// command_console.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include "strsafe.h"

#define BUFSIZE 4096

void ErrorExit(PTSTR lpszFunction) 

    // Format a readable error message, display a message box, 
    // and exit from the application.
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(1);
}

int _tmain(int argc, _TCHAR* argv[])
{
    CHAR chBuf[BUFSIZE]; 
    DWORD dwRead, dwWritten; 
    HANDLE hStdin, hOut; 
    BOOL bSuccess; 

    hOut = GetStdHandle(STD_OUTPUT_HANDLE); 
    if (argc > 1) {
        hOut = (HANDLE)(_tstoi(argv[1]));
    }
    
    hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    if ((hOut == INVALID_HANDLE_VALUE) || 
        (hStdin == INVALID_HANDLE_VALUE)) 
    {
        ErrorExit(TEXT("INVALID_HANDLE_VALUE")); 
    }

    // set menu
    {
        HWND consoleWnd = GetConsoleWindow();
        HMENU menu = GetSystemMenu(consoleWnd, FALSE);

        DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_MINIMIZE, MF_BYCOMMAND);
        DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);

        DrawMenuBar(consoleWnd);
    }    

    // Send something to this process's stdout using printf.
    printf("\n ** please input command and push <Enter> ** \n ** If you want cancel command, Just push <Enter> **\n\n");
    {
        /*printf("-----------------------------------------------\n");
        printf("* cmd list\n");
        printf("  cls() - console screen clear\n");
        printf("  st() - same as status(), and plus detail player count by player state\n");
        printf("  conn(\'acctnm\', n, [channel]) - server to connect\n");
        printf("     param acctnm : account name\n");
        printf("     param n : bot max number\n");
        printf("     param channel: optional. channel for login. if you omit, auto channel login.\n");
        printf("  discon() - all bot connection is close with game server\n");
        printf("  set_logtype(log_type) - log_type : 1=error, 2=warring, 3=debug\n");
        printf("  set_logfile(log_file_name) - log_file_name is string type. logfile in the log dir. ex) input test -> \"test.txt\". auto add .txt\n");
        printf("  logtimeon(switch) - logtime print option. switch is true or false\n");
        printf("  log_net() : show network log\n");
        printf("  log_player(playername)\n");
        printf("  log_player_file(playername)\n");
        printf("-----------------------------------------------\n");*/
    }
    printf("cmd:>");

    // This simple algorithm uses the existence of the pipes to control execution.
    // It relies on the pipe buffers to ensure that no data is lost.
    // Larger applications would use more advanced process control.

    for (;;) 
    { 
        // Read from standard input and stop on error or no data.
        bSuccess = ReadFile(hStdin, chBuf, BUFSIZE, &dwRead, NULL); 

        if (! bSuccess || dwRead == 0) {
            ErrorExit(TEXT("failed to ReadFile."));
            break;
        }

        // Write to standard output and stop on error.
        bSuccess = WriteFile(hOut, chBuf, dwRead, &dwWritten, NULL); 

        if (! bSuccess) {
            ErrorExit(TEXT("failed to WriteFile."));
            break;
        }
    } 

    return 0;
}

