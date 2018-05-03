// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include <Shlwapi.h>
#include <Shlobj.h>

#include "BarCore.h"
#include "BarClasses.h"


static char log_name[MAX_PATH];
static FILE *log_file;

#define LOG_NAME "BarAnalyzer.log"

static CRITICAL_SECTION g_logCs;

BARCORE_API char *getLogFileName()
{
    return log_name;
}


void Logger::Log(const char *format, ...)
{
    EnterCriticalSection(&g_logCs);

    if (!log_file)
    {
        fopen_s(&log_file, log_name, "a+t");
        if (!log_file)
            return;
    }

    SYSTEMTIME tm;
    GetSystemTime(&tm);

    va_list marker;

    va_start( marker, format );     

    fprintf(log_file, "%02d.%02d.%04d %02d:%02d:%02d ", tm.wDay, tm.wMonth, tm.wYear, tm.wHour, tm.wMinute, tm.wSecond);
    vfprintf(log_file, format, marker);
    fprintf(log_file, "\n");
    fflush(log_file);
    fclose(log_file);
    log_file = NULL;

    va_end(marker);

    LeaveCriticalSection(&g_logCs);
}


BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        {
        log_file = NULL;
        
        if(SUCCEEDED(SHGetFolderPathA(NULL, 
                             CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, 
                             NULL, 
                             0, 
                             log_name))) 
        {
            PathAppendA(log_name, LOG_NAME);
        }

        InitializeCriticalSection(&g_logCs);

/*
        GetModuleFileNameExA(GetCurrentProcess(), (HMODULE)hModule, log_name, 16384);
        char *pos = strrchr(log_name, '\\');
        if (!pos)
            pos = log_name;
        else
            ++pos;

        strcpy(pos, LOG_NAME);*/

        break;
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        DeleteCriticalSection(&g_logCs);
        if (log_file)
            fclose(log_file);
        break;
    }
    return TRUE;
}

