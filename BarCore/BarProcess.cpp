/*
* Copyright 2018 Frangou Lab
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <Windows.h>
#include <ctime>

#include "BarCore.h"
#include "BarClasses.h"

CRITICAL_SECTION g_cs;
HANDLE g_hMapFile;
HANDLE g_hFile; 
LPVOID g_lpMapAddress;
char *g_pData;
DWORD g_remainingData;
char g_CR;
int g_nCR;
bool g_counterFirst;

volatile long g_totalExact, g_total1Error, g_total2Errors, g_totalN, g_totalIndel, g_totalIns, g_totalSkipped;
volatile long *g_fileExact, *g_file1Error, *g_file2Errors, *g_fileN, *g_fileIndel, *g_fileIns, g_fileSkipped;

BarNode *getNodeForCode(BarCode *code)
{
    // Find in tree
    BarNode *curNode = g_root;
    for (int i = 0; i < g_codeLen; i++)
    {
        t_code curSymbol = code->symbol(i);
        if (!(curNode = curNode->getChild(curSymbol))) // upfs
            return NULL;
    }
    return curNode;
}



volatile long totalIds;

DWORD WINAPI myProcessingThreadProc(LPVOID lpParameter)
{
    Logger::Log("Thread %d started", GetCurrentThreadId());
    BarCode *code = new BarCode(g_codeLen, 0, g_totalMPs);

    int threadNum = (int) lpParameter;
    int curOffset = g_offsets[threadNum];

    long *fileExact = new long[g_totalMPs];
    long *file1Error = new long[g_totalMPs];
    long *file2errors = new long[g_totalMPs];
    long *fileN = new long[g_totalMPs];
    long *fileIndel = new long[g_totalMPs];
    long *fileIns = new long[g_totalMPs];
    long fileSkipped;
    for (int i = 0; i < g_totalMPs; i++)
        fileExact[i] = file1Error[i] = file2errors[i] = fileN[i] = fileIndel[i] = fileIns[i] = 0;
    fileSkipped = 0;

    do
    {
        if (curOffset >= g_offsets[threadNum+1])
            break;

        char *buf = g_pData+curOffset;

        char *cr = strchr(buf, g_CR);
        if (!cr) // last
            curOffset = g_offsets[threadNum+1];
        else
        {
            curOffset += (int)(cr+g_nCR-buf);
            *cr = 0;
        }

/*		int left = g_pData - (char *)g_lpMapAddress;

        // Find next CR
        for (;g_remainingData;g_pData++,g_remainingData--)
            if (*g_pData == 0xd || *g_pData == 0xa) 
                break;
        // Skip all CRs
        for (;g_remainingData;g_pData++,g_remainingData--)
            if (*g_pData != 0xd && *g_pData != 0xa) 
                break;*/

        int mpIdx = 0;

        if (g_mpData) // Load mp
        {
            mpIdx = code->loadFromStringMP(buf, g_counterFirst?1:0, g_separator, g_mpLength, g_totalMPs, g_mpPositions, g_mpData);
            if (mpIdx < 0)
            {
                fileSkipped++;
                continue;
            }
        }
        else
            if (!code->loadFromString(buf, g_counterFirst?1:0, -1, g_separator))
            {
                fileSkipped++;
                continue;
            }

        int curCount = 1;

        SearchResult r = findColumn(buf, g_counterFirst?0:1, g_separator);
        if (r.position >= 0) // Try to get count
        {
            //char ch = buf[r.position+r.length];
            //buf[r.position+r.length] = 0;
            int cnt = atoi(buf+r.position);
            if (cnt > 0)
                curCount = cnt;
            //buf[r.position+r.length] = ch;
        }

        BarNode *curNode;
        int insOrIndel;

        if (code->m_NPosition >= 0) // Mmmm, it's N, special case
        {
            if (g_checkN)
            {
                BarNode *foundNode = NULL;
                if (g_errorsPermited > 0) // Have to check
                {
                    for (int i = 0; i < CODE_SIZE; i++)
                    {
                        code->setNTo(i);
                        curNode = getNodeForCode(code);
                        if (curNode && curNode->getErrors() == 0) // Found
                        {
                            if (foundNode) // Double, skip
                            {
                                foundNode = NULL;
                                break;
                            }
                            foundNode = curNode;
                        }
                    }
                }
                if (foundNode)
                {
                    foundNode->getBarCode()->incNCount(g_curFile, mpIdx, curCount);
                    fileN[mpIdx] += curCount;
                    continue;
                }
            }
        }
        else if ((insOrIndel = code->hasInsertionOrIndel()) != 0) // Indel & insertion handling
        {
            if ((g_checkIndel && insOrIndel < 0) || (g_checkIns && insOrIndel > 0)) // Have to check
            {
                BarNode *foundNode = NULL;
                code->prepare();
                while (code->nextSuitable()) // Mutate this code to next suitable
                {
                    curNode = getNodeForCode(code);
                    if (curNode && curNode->getErrors() == 0) // Found
                    {
                        if (foundNode) // Double, skip
                        {
                            foundNode = NULL;
                            break;
                        }
                        foundNode = curNode;
                    }
                }
                if (foundNode)
                {
                    if (insOrIndel > 0)
                    {
                        foundNode->getBarCode()->incInsCount(g_curFile, mpIdx, curCount);
                        fileIns[mpIdx] += curCount;
                    }
                    else
                    {
                        foundNode->getBarCode()->incIndelCount(g_curFile, mpIdx, curCount);
                        fileIndel[mpIdx] += curCount;
                    }
                    continue;
                }
            }
        }
        else	// Normal case
        {
            curNode = getNodeForCode(code);
            if (curNode)
            {
                if (curNode->getErrors() == 0)
                {
                    curNode->getBarCode()->incExactCount(g_curFile, mpIdx, curCount);
                    fileExact[mpIdx] += curCount;
                    continue;
                }
                else if (curNode->getErrors() == 1)
                {
                    curNode->getBarCode()->inc1ErrorCount(g_curFile, mpIdx, curCount);
                    file1Error[mpIdx] += curCount;
                    continue;
                }
                else if (curNode->getErrors() == 2)
                {
                    curNode->getBarCode()->inc2ErrorCount(g_curFile, mpIdx, curCount);
                    file2errors[mpIdx] += curCount;
                    continue;
                }
            }
        }
        // Skipped this code some way
        if (g_skipped) // Dump
        {
            EnterCriticalSection(&g_cs);
            fwrite(buf, g_pData+curOffset-buf, 1, g_skipped);
            LeaveCriticalSection(&g_cs);
        }
        fileSkipped++;
    } while (1);

    delete code;

    for (int i = 0; i < g_totalMPs; i++)
    {
        InterlockedExchangeAdd(g_fileExact+i, fileExact[i]);
        InterlockedExchangeAdd(g_file1Error+i, file1Error[i]);
        InterlockedExchangeAdd(g_file2Errors+i, file2errors[i]);
        InterlockedExchangeAdd(g_fileN+i, fileN[i]);
        InterlockedExchangeAdd(g_fileIns+i, fileIns[i]);
        InterlockedExchangeAdd(g_fileIndel+i, fileIndel[i]);
    }
    InterlockedExchangeAdd(&g_fileSkipped, fileSkipped);

    delete[] fileExact;
    delete[] file1Error;
    delete[] file2errors;
    delete[] fileN;
    delete[] fileIndel;
    delete[] fileIns;

    Logger::Log("Thread %d finished", GetCurrentThreadId());
    return 0;
}

BARCORE_API int processFile(const char *inPath)
{
    time_t start = time(NULL);

    g_fileNames[g_curFile] = (char *)malloc((strlen(inPath)+1)*sizeof(char));

    // Eat path and extension
    const char *pth = strrchr(inPath, '\\');
    const char *ext = strrchr(inPath, '.');

    if (!pth)
        pth = inPath;
    else
        ++pth;

    if (!ext)
        ext = inPath+strlen(inPath);

    int fLen = (int)(ext-pth);

    memcpy(g_fileNames[g_curFile], pth, fLen*sizeof(char));
    g_fileNames[g_curFile][fLen] = 0;

    g_fileExact = (volatile long *)malloc(g_totalMPs*sizeof(long));
    g_file1Error = (volatile long *)malloc(g_totalMPs*sizeof(long));
    g_file2Errors = (volatile long *)malloc(g_totalMPs*sizeof(long));
    g_fileN = (volatile long *)malloc(g_totalMPs*sizeof(long));
    g_fileIndel = (volatile long *)malloc(g_totalMPs*sizeof(long));
    g_fileIns = (volatile long *)malloc(g_totalMPs*sizeof(long));

    for (int i = 0; i < g_totalMPs; i++)
        g_fileExact[i] = g_file1Error[i] = g_file2Errors[i] = g_fileN[i] = g_fileIndel[i] = g_fileIns[i] = 0;
    g_fileSkipped = 0;

    // Create mapping
    g_hFile = CreateFileA(inPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (g_hFile == INVALID_HANDLE_VALUE)
        return CantRead;
    BY_HANDLE_FILE_INFORMATION fi;
    if (!GetFileInformationByHandle(g_hFile, &fi))
    {
        CloseHandle(g_hFile);
        return CantRead;
    }
    if (fi.nFileSizeHigh)
    {
        CloseHandle(g_hFile);
        return TooLarge;
    }
    g_remainingData = fi.nFileSizeLow;
  
/*	g_hMapFile = CreateFileMapping( g_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!g_hMapFile)
    {
        CloseHandle(g_hFile);
        return CantRead;
    }
    g_lpMapAddress = MapViewOfFile(g_hMapFile, FILE_MAP_READ, 0, 0, 0);
    if (g_lpMapAddress == NULL) 
    {
        CloseHandle(g_hFile);
        CloseHandle(g_hMapFile);
        return CantRead;
    }
    */
    g_lpMapAddress = malloc(g_remainingData+1);
    DWORD read;
    if (!ReadFile(g_hFile, g_lpMapAddress, g_remainingData, &read, NULL))
    {
        CloseHandle(g_hFile);
        return CantRead;
    }
    g_pData = (char *)g_lpMapAddress;
    g_pData[g_remainingData] = 0;
    // Detect crs
    char *cr = strchr(g_pData, 0xd);
    char *lf = strchr(g_pData, 0xa);

    if (cr)
    {
        g_CR = 0xd;
        if (lf && lf-cr == 1)
            g_nCR = 2;
        else
            g_nCR = 1;
    }
    else
    {
        g_CR = 0xa;
        if (cr && cr-lf == 1)
            g_nCR = 2;
        else
            g_nCR = 1;
        cr = lf;
    }

    // Autodetect separator
    g_separator = ',';
    *cr = 0; // Zero

    if (strchr(g_pData, '\t'))
        g_separator = '\t';
    else if (strchr(g_pData, ','))
        g_separator = ',';
    else if (strchr(g_pData, ';'))
        g_separator = ';';
    else
        Logger::Log("Can't detect separator, assuming comma");

    // Detect if counter column is 1st
    if (atoi(g_pData) > 0) // Seems it is
        g_counterFirst = true;
    else
        g_counterFirst = false;


    *cr = g_CR;

    totalIds = 0;
    InitializeCriticalSection(&g_cs);

    // Create a lot of threads
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );

    int threadNum = sysinfo.dwNumberOfProcessors;

    if (threadNum <= 0)
        threadNum = 1;

    // Additional check on short data
    if (g_remainingData < 65536) // No need to many threads
        threadNum = 1;

    g_offsets = new long[threadNum+1];
    g_offsets[threadNum] = g_remainingData;
    long avgByThread = g_remainingData/threadNum;
    for (int thread = 0; thread < threadNum; thread++)
    {
        long offset = thread*avgByThread;
        if (thread != 0) // Back on bound...
        {
            while (g_pData[offset] != g_CR && offset >= 0)
                --offset;
            
            offset += g_nCR;
        }
        g_offsets[thread] = offset;
    }

    Logger::Log("Starting %d processing threads...", threadNum);
    HANDLE *threads = new HANDLE[threadNum];
    for (int thread = 0; thread < threadNum; thread++)
    {
        threads[thread] = CreateThread(NULL, 0, myProcessingThreadProc, (void *)thread, 0, NULL);
    }
    Logger::Log("Waiting for completion of threads");
    // Wait all exit
    WaitForMultipleObjects(threadNum, threads, true, INFINITE);
    Logger::Log("All threads exited");

    for (int thread = 0; thread < threadNum; thread++)
        CloseHandle(threads[thread]);
    delete[] threads;

    DeleteCriticalSection(&g_cs);

    if (g_summary) // Dump string
    {
        for (int i = 0; i < g_totalMPs; i++)
        {
            if (!g_mpData)
                fprintf(g_summary, "%s,%ld,%ld", g_fileNames[g_curFile], g_fileSkipped, g_fileExact[i]);
            else
                fprintf(g_summary, "%s(%s),%ld,%ld", g_fileNames[g_curFile], g_mpData[i], g_fileSkipped, g_fileExact[i]);
            if (g_errorsPermited > 0)
                fprintf(g_summary, ",%ld", g_file1Error[i]);
            if (g_errorsPermited > 1)
                fprintf(g_summary, ",%ld", g_file2Errors[i]);
            if (g_checkN)
                fprintf(g_summary, ",%ld", g_fileN[i]);
            if (g_checkIndel)
                fprintf(g_summary, ",%ld", g_fileIndel[i]);
            if (g_checkIns)
                fprintf(g_summary, ",%ld", g_fileIns[i]);
            if (g_checkN || g_checkIndel || g_checkIns || g_errorsPermited > 0)
                fprintf(g_summary, ",%ld", g_fileIns[i]+g_fileIndel[i]+g_fileN[i]+g_file1Error[i]+g_file2Errors[i]+g_fileExact[i]);
            fputs("\n", g_summary);
        }
    }

    for (int i = 0; i < g_totalMPs; i++)
    {
        g_totalExact += g_fileExact[i];
        g_total1Error += g_file1Error[i];
        g_total2Errors += g_file2Errors[i];
        g_totalN += g_fileN[i];
        g_totalIns += g_fileIns[i];
        g_totalIndel += g_fileIndel[i];
    } 
    g_totalSkipped += g_fileSkipped;


    delete[] g_offsets;
    /*UnmapViewOfFile(g_lpMapAddress);
    CloseHandle(g_hMapFile);*/

    CloseHandle(g_hFile);
    free(g_lpMapAddress);
    ++g_curFile;

    time_t end = time(NULL);

    Logger::Log("Statistics: %d exacts, %d 1 error, %d 2 errors, %d N, %d indels, %d insertions (Total %d) and %d was skipped in %d seconds", 
        g_totalExact, g_total1Error, g_total2Errors, g_totalN, g_totalIndel, g_totalIns, 
        g_totalExact+ g_total1Error+ g_total2Errors+ g_totalN+ g_totalIndel+ g_totalIns, g_totalSkipped, end-start);

    free((void *)g_file1Error);
    free((void *)g_file2Errors);
    free((void *)g_fileExact);
    free((void *)g_fileN);
    free((void *)g_fileIndel);
    free((void *)g_fileIns);

    return NoError;
}
