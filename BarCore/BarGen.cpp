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

#include "BarCore.h"

#include <Windows.h>
#include <ctime>

#include <iostream>
#include <fstream>

BarNode *getNodeForCode(BarCode *code);
char *nextLine(char *str);

#define MAX_CONT_ERRORS 10000000

SRWLOCK g_genLock;

long g_codesGenerated, g_codesSkipped;
FILE *g_genOutput;
char g_buf[1024];

int g_maxCodes;
bool *g_failAdd;
long g_failedAdds;
int g_threadNum;
bool g_generateDesc;
int g_minDistance;
int g_maxDistance;
volatile LONGLONG g_sumDistances;
LONGLONG g_comparisons;
int g_index, g_codesCount;
CRITICAL_SECTION g_verCS;
bool g_detectDeadLoop;
bool g_aborted;

std::ofstream debug_output("C:\\Users\\dmusatov\\Desktop\\DEBUG_INFO.txt", std::ios::out);

bool recurFindMutated(BarCode *code, int level)
{
    if (level >= g_errorsPermited)
        return false;
    bool found = false;
    while (!found && code->mutate(level)) 
    {
        if (getNodeForCode(code))
        {
            found = true;
            break;
        }
        found = recurFindMutated(code, level+1);
    }
    return found;
}

void recurAddMutated(BarCode *code, int level)
{
    if (level >= g_errorsPermited)
        return;
    while (code->mutate(level)) 
    {
        addToTree(code, level+1);
        recurAddMutated(code, level+1);
    }
}


DWORD WINAPI genProcessingThreadProc(LPVOID lpParameter)
{
    debug_output << "Entered genProcessingThreadProc(" << lpParameter << ")" << std::endl;

    int myThreadNum = (int)lpParameter;
    BarCode *code = new BarCode(g_codeLen, 0, 0);
    code->mutate(); // Just allocing original code

    srand((unsigned)time(NULL) + myThreadNum);

    long failedContingously = 0;

    while (g_codesGenerated < g_maxCodes) // Main loop
    {
        debug_output << "do // Main loop " << g_codesGenerated << " " << g_maxCodes; // << std::endl;

        if (g_aborted)
            return 1;

        code->generateRandom();

        bool found = true;

        code->resetToOriginal();
        debug_output << " # code->resetToOriginal(); # ";

        if (!getNodeForCode(code))
        {
            debug_output << " # Entered !getNodeForCode(code) # "; // << std::endl;
            code->initMutations();
            debug_output << " # code->initMutations(); # ";
            found = recurFindMutated(code, 0);
            debug_output << " # recurFindMutated(code, 0); found = " << found;
        }

        if (found)
        {
            g_codesSkipped++;

            if (g_detectDeadLoop) {
                if (++failedContingously >= MAX_CONT_ERRORS) // Ups, have to exit
                {
                    g_aborted = true;
                    return 1;
                }
            }
            debug_output << std::endl;
            continue;
        }

        failedContingously = 0; // Reset

        // Aha. Add to tree and dump
        if (g_failAdd[myThreadNum]) // Fail this add
        {
            g_failedAdds++;
            g_failAdd[myThreadNum] = false;
            debug_output << std::endl;
            continue;
        }
        // Signal other thread to fail next add
        for (int i = 0; i < g_threadNum; i++)
            g_failAdd[i] = (i != myThreadNum);

        code->initMutations();
        code->dump(g_buf);
        if (g_generateDesc)
            fprintf(g_genOutput, "Code #%d,%s\n", g_codesGenerated, g_buf);
        else
            fprintf(g_genOutput, "%s\n", g_buf);

        ++g_codesGenerated;

        debug_output << " addToTree:" << addToTree(code, 0) << std::endl;
        recurAddMutated(code, 0);
        debug_output << "recurAddMutated(code, 0);" << std::endl;

/*		while (code->mutate()) // 1st level of mutations
        {
            addToTree(code, 1);
            if (g_errorsPermited > 3)
                while (code->mutate2()) // 2nd level
                    addToTree(code, 2);
        }*/
    }
    debug_output << "!!! Thread is about to exit " << std::endl;
    return 0;
}


BARCORE_API int generate(const char *outPath, int len, int distance, int maxCodes, int generateDesc, int detectDeadLoop)
{
    // Only odd distances beginning from 3 are supported
    if (distance < 3 || (distance&0x1) == 0 || len <= distance)
        return BadGenParams;

    if (fopen_s(&g_genOutput, outPath, "wt"))
        return BadGenParams;

    if (generateDesc)
        fprintf(g_genOutput, "Desc,Code\n");
    else
        fprintf(g_genOutput, "Code\n");

    g_maxCodes = maxCodes;

    g_codesGenerated = g_codesSkipped = 0;
    g_generateDesc = (generateDesc != 0);
    g_detectDeadLoop = (detectDeadLoop != 0);
    g_aborted = false;

//	BarCode *code = new BarCode(len, 0, 0);
//	code->mutate(); // Just allocing original code

    time_t start = time(NULL);

    if (g_nodeManager)
        delete g_nodeManager;
    
    g_nodeManager = new SimpleBarNodeManager();
    g_root = g_nodeManager->newRoot(); //new BarNode(0);
    g_codeLen = len;
    g_errorsPermited = (distance-1)/2;

    // Create a lot of threads
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    const int threadNum = 1;

    g_threadNum = threadNum;
    g_failedAdds = 0;
    g_failAdd = new bool[threadNum];
    memset(g_failAdd, 0, threadNum*sizeof(bool));

    Logger::Log("Starting %d processing threads...", threadNum);
    HANDLE *threads = new HANDLE[threadNum];

    for (int thread = 0; thread < threadNum; thread++)
    {
        threads[thread] = CreateThread(NULL, 0, genProcessingThreadProc, (void *)thread, 0, NULL);
    }
    Logger::Log("Waiting for completion of threads");
    // Wait all exit
    WaitForMultipleObjects(threadNum, threads, true, INFINITE);
    Logger::Log("All threads exited");

    for (int thread = 0; thread < threadNum; thread++)
        CloseHandle(threads[thread]);
    delete[] threads;
    delete[] g_failAdd;

//	recurDumpCode(code, 0);

    fclose(g_genOutput);

    delete g_nodeManager;
    g_nodeManager = NULL;

    time_t end = time(NULL);

    Logger::Log("Generated %d codes (and %d was skipped) in %d seconds. %d additions failed.", g_codesGenerated, g_codesSkipped, end-start, g_failedAdds);
    return g_aborted?DeadLoopDetected:NoError;
}

DWORD WINAPI genVerifyingThreadProc(LPVOID lpParameter)
{
    for (;;) {
        // Gen next index
        EnterCriticalSection(&g_verCS);
        if (g_index >= g_codesCount)
        {
            LeaveCriticalSection(&g_verCS);
            break;
        }
        int i = g_index;
        g_index++;
        LeaveCriticalSection(&g_verCS);

        for (int j = i+1; j < g_codesCount; j++)
        {
            int curLen = g_codes->get(i)->hammingDistance(g_codes->get(j));
            if (curLen < g_minDistance)
                g_minDistance = (int)curLen; // No sync here, seems it is normal
            if (curLen > g_maxDistance)
                g_maxDistance = (int)curLen;
            InterlockedExchangeAdd64(&g_sumDistances, (LONGLONG)curLen);
        }
    }

    return 0;
}

BARCORE_API int calcMinDistance(const char *inPath, int barLength, int idsColumn, int *maxDist, double *avgDist)
{
    time_t start = time(NULL);


    char buf[MAX_COUNT+1];

    g_codes = new BarArray();
    buf[MAX_COUNT] = 0;
    FILE *db;
    

    if (fopen_s(&db, inPath, "rb") != 0)
        return -1;

    // Read input
    fseek(db, 0, SEEK_END);
    int inLen = ftell(db);
    fseek(db, 0, SEEK_SET);

    char *data = (char *)malloc(inLen+1);
    fread(data, 1, inLen, db);
    fclose(db);
    data[inLen] = 0;

    char *curLine = nextLine(data); // Skip header

    while (curLine)
    {
        char *newLine = nextLine(curLine); // Zero...
        if (barLength == 0) // Autodetect
        {
            barLength = BarCode::detectCodeLen(curLine, idsColumn, ',');
            if (barLength <= 0)
            {
                Logger::Log("Can't autodetect code length");
                return -1;
            }
        }

        BarCode *code = new BarCode(barLength, 0, 0);
        if (!code->loadFromString(curLine, idsColumn, 1, -1, ','))
        {
            Logger::Log("Can't read code in file");
            return -1;
        }
        g_minDistance = code->len();
        g_codes->put(code);
        curLine = newLine;
    }


    g_maxDistance = 0;
    g_sumDistances = 0;
    g_comparisons = 0;

    free(data);

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    int threadNum = sysinfo.dwNumberOfProcessors;

    if (threadNum <= 0)
        threadNum = 1;

    g_threadNum = threadNum;

    InitializeCriticalSection(&g_verCS);
    g_codesCount = g_codes->count();
    g_index = 0;

    Logger::Log("Starting %d verifying threads...", threadNum);
    HANDLE *threads = new HANDLE[threadNum];

    for (int thread = 0; thread < threadNum; thread++)
    {
        threads[thread] = CreateThread(NULL, 0, genVerifyingThreadProc, (void *)thread, 0, NULL);
    }
    Logger::Log("Waiting for completion of threads");
    // Wait all exit
    WaitForMultipleObjects(threadNum, threads, true, INFINITE);
    Logger::Log("All threads exited");

    for (int thread = 0; thread < threadNum; thread++)
        CloseHandle(threads[thread]);
    delete[] threads;
    DeleteCriticalSection(&g_verCS);


    delete g_codes;

    time_t end = time(NULL);

    g_comparisons = (g_codesCount*(LONGLONG)g_codesCount - g_codesCount)/2;

    Logger::Log("Verified %d ids in %d seconds: min Hamming distance = %d, max = %d, average = %.2f", g_codesCount, end-start, g_minDistance, g_maxDistance, g_sumDistances/(double)g_comparisons);
    if (maxDist)
        *maxDist = g_maxDistance;
    if (avgDist)
        *avgDist = g_sumDistances/(double)g_comparisons;

    return g_minDistance;
}
