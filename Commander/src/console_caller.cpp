#include <iostream>
#include <windows.h>
#include <strsafe.h>
#include <stdio.h>

void Exit(int errorCode);

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
    
    Exit(1);
}

#define BUFSIZE 4096

HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

void ReadFromPipe(char buff[], int buffSize)    
{    
    DWORD dwRead;
    BOOL bSuccess = FALSE;

    //printf("> ReadFromPipe waiting ...\n");

    { 
        bSuccess = ReadFile(g_hChildStd_OUT_Rd, buff, buffSize, &dwRead, NULL);        
        if (! bSuccess) {
            printf("ReadFile failed.\n");
            return; 
        }
        else if (dwRead == 0) {
            printf("ReadFile wrong. read size is 0\n");
            return; 
        }
        else {
            printf("cmd:>%s", buff);
        }
    }
} 

HANDLE g_hCommanderProcess = INVALID_HANDLE_VALUE;
HANDLE g_hCommanderThread = INVALID_HANDLE_VALUE;
DWORD g_hCommanderProcessID = 0;
HWND g_hCommanderWnd = NULL;

BOOL CALLBACK EnumWindowsProcMy(HWND hwnd, LPARAM lParam)
{
    DWORD lpdwProcessId;
    GetWindowThreadProcessId(hwnd, &lpdwProcessId);
    if(lpdwProcessId == lParam)
    {
        g_hCommanderWnd = hwnd;
        return FALSE;
    }

    return TRUE;
}

void OpenCommander()
{    
    // create child process
    //if (g_hCommanderProcess == INVALID_HANDLE_VALUE)
    {
        TCHAR szCmdline[255] = TEXT("");
        BOOL bSuccess = FALSE;                

        // create pipe
        {
            SECURITY_ATTRIBUTES saAttr = {0,};
            {
                saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
                saAttr.bInheritHandle = TRUE; 
                saAttr.lpSecurityDescriptor = NULL;
            }            

            // Create a pipe for the child process's STDOUT.

            if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) )
                ErrorExit(TEXT("StdoutRd CreatePipe"));

            if ( ! SetHandleInformation(g_hChildStd_OUT_Wr, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT) )
                ErrorExit(TEXT("Stdout SetHandleInformation"));                     
        }

        wsprintf(szCmdline, TEXT("command_console.exe %d"), (HANDLE)g_hChildStd_OUT_Wr);

        // Set up members of the STARTUPINFO structure. 
        // This structure specifies the STDIN and STDOUT handles for redirection.
        STARTUPINFO siStartInfo = {0,};
        {
            siStartInfo.cb = sizeof(STARTUPINFO);

            siStartInfo.lpTitle = L"Commander";

            // set pos
            {
                RECT rect;
                GetWindowRect(GetConsoleWindow(), &rect);

                siStartInfo.dwX = rect.left;
                siStartInfo.dwY = rect.top;
            }
            siStartInfo.dwFlags |= STARTF_USEPOSITION;
           
            siStartInfo.dwXSize = 750;
            siStartInfo.dwYSize = 100;
            siStartInfo.dwFlags |= STARTF_USESIZE;

            siStartInfo.dwXCountChars = 75;
            siStartInfo.dwFlags |= STARTF_USECOUNTCHARS;

            siStartInfo.dwFillAttribute |= BACKGROUND_GREEN;
            siStartInfo.dwFlags |= STARTF_USEFILLATTRIBUTE;

            siStartInfo.wShowWindow = SW_SHOW;
            siStartInfo.dwFlags |= STARTF_USESHOWWINDOW;
        }

        {
            // Set up members of the PROCESS_INFORMATION structure.     
            PROCESS_INFORMATION piProcInfo;
            ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

            SECURITY_ATTRIBUTES saAttr = {0,};
            {
                saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
                saAttr.bInheritHandle = TRUE; 
                saAttr.lpSecurityDescriptor = NULL;
            }

            // Create the child process.                 
            bSuccess = CreateProcess(NULL, 
                szCmdline,     // command line 
                NULL,//&saAttr,          // process security attributes 
                NULL,          // primary thread security attributes 
                TRUE,          // handles are inherited 
                CREATE_NEW_CONSOLE,             // creation flags 
                NULL,          // use parent's environment 
                NULL,          // use parent's current directory 
                &siStartInfo,  // STARTUPINFO pointer 
                &piProcInfo);  // receives PROCESS_INFORMATION                     

            // If an error occurs, exit the application. 
            if ( ! bSuccess ) {
                ErrorExit(TEXT("CreateProcess."));
            }
            else {
                g_hCommanderProcess = piProcInfo.hProcess;
                g_hCommanderThread = piProcInfo.hThread;

                g_hCommanderProcessID = piProcInfo.dwProcessId;
            }            
        }                
    }
    //else 
    //{
    //    // reopen window
    //    EnumWindows(EnumWindowsProcMy, g_hCommanderProcessID);

    //    if (g_hCommanderWnd != NULL) {
    //        ::ShowWindow(g_hCommanderWnd, SW_SHOW);
    //    }
    //    else {
    //        printf("failed to show commander by null commander-window. processID=%d", g_hCommanderProcessID);
    //    }
    //}

    EnableWindow(GetConsoleWindow(), FALSE);
}

void TerminateCommander()
{
    if (g_hCommanderThread != INVALID_HANDLE_VALUE) {        
        TerminateProcess(g_hCommanderThread, THREAD_TERMINATE);
        CloseHandle(g_hCommanderThread);
    }

    if (g_hCommanderProcess != INVALID_HANDLE_VALUE) {
        TerminateProcess(g_hCommanderProcess, PROCESS_TERMINATE);        
        CloseHandle(g_hCommanderProcess);
    }

    EnableWindow(GetConsoleWindow(), TRUE);
}

void CloseCommander()
{
    //if (g_hCommanderProcess != INVALID_HANDLE_VALUE) {
    //    EnumWindows(EnumWindowsProcMy, g_hCommanderProcessID);

    //    if (g_hCommanderWnd != NULL) {
    //        // close window
    //        ::ShowWindow(g_hCommanderWnd, SW_HIDE);
    //    }
    //    else {
    //        printf("failed to hide commander by null commander-window. processID=%d", g_hCommanderProcessID);
    //    }
    //}    

    //::EnableWindow(GetConsoleWindow(), TRUE);

    TerminateCommander();
}

void Exit(int errorCode)
{
    TerminateCommander();
    ExitProcess(errorCode);
}

/**
    @brief Main function
*/
int main(int argc,const char * argv[])
{
    // set console
    {
        //_CRTDBG_CHECK_ALWAYS_DF|
	    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_DELAY_FREE_MEM_DF);
	    int tmpDbgFlag;

        _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
        _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDERR );
        /*
         * Set the debug-heap flag to keep freed blocks in the
         * heap's linked list - This will allow us to catch any
         * inadvertent use of freed memory
         */
        tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	    tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;
        tmpDbgFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
        tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
	    tmpDbgFlag |= _CRTDBG_CHECK_CRT_DF;
        _CrtSetDbgFlag(tmpDbgFlag);

        SetConsoleTitle(L"Bot Test Tool. ver 1.0 (e<Enter>: exit. just <Enter>: open commander.)");

        // set menu
        {
            HWND consoleWnd = GetConsoleWindow();
            HMENU menu = GetSystemMenu(consoleWnd, FALSE);

            DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);            

            DrawMenuBar(consoleWnd);
        }
    }	

	for (;;)
	{
        char buff[BUFSIZE] = {0};

		if (NULL == fgets(buff,sizeof(buff),stdin))
			break;

        if (!strcmp(buff, "e\n")) {
            Exit(0);
            break;
        }        
        else if (!strcmp(buff, "\n")) {
            OpenCommander();
            ReadFromPipe(buff, BUFSIZE);
            CloseCommander();
        }

        if (!strcmp(buff, "\n") || !strcmp(buff, "\r\n")) {
            continue;
        }
	};
};
