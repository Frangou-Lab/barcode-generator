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

#pragma once

#include <cstdio>

#include "BarClasses.h"

#ifdef __APPLE__
#include <libkern/OSAtomic.h>
#define BARCORE_API
#define strcpy_s(src, size, dest) (strncpy(src, dest, size))
#define strcat_s(src, size, dest) (strncat(src, dest, size))
#define fopen_s(pFile, filename, mode) (NULL == (*(pFile) = fopen(filename, mode)))
#define memcpy_s(dest, numberOfElements, src, count) (memcpy(dest, src, count))
#define InterlockedExchangeAdd(added, value) (OSAtomicAdd32(value, added))
#else
#ifdef BARCORE_EXPORTS
#define BARCORE_API __declspec(dllexport)
#else
#define BARCORE_API __declspec(dllimport)
#endif
#endif

class Logger
{
public:
    static void Log(const char *format, ...);
};

extern "C"
{
    BARCORE_API char *getLogFileName();

    BARCORE_API int checkLibrary(const char *path); // Returns max code lenght or <= 0 if failed
    BARCORE_API int loadLibrary(const char *path, int barStart /*1 is first */, int barLength, int errorsPermited, int totalFiles, int totalMPs, int optimizeMemory);
    BARCORE_API int prepare(const char *outPath, const char *skippedPath, const char *summaryPath, bool checkN, bool checkIndel, bool checkIns, const char **mpData, const int *mpPositions);

    BARCORE_API int processFile(const char *inPath);

    BARCORE_API int dumpResults(const char *outPath);
    BARCORE_API void cleanup();

    BARCORE_API int generate(const char *outPath, int len, int distance, int maxCodes, int generateDesc, int detectDeadLoop);
    BARCORE_API int calcMinDistance(const char *inPath, int barLength, int idsColumn, int *maxDist, double *avgDist);

    BARCORE_API char *generateActivationRequest(char *appName, char *name, char *eMail);
    BARCORE_API int isActivated(char *appName);
    BARCORE_API int activate(char *appName, char *activationCode);
    BARCORE_API char *getLicensingInformation(char *appName);
    BARCORE_API int getTrialsLeft(char *appName);
    BARCORE_API void decreaseNumberOfTrialAttempts(char *appName);
}

extern char g_alphabet[];
extern BarNodeManager *g_nodeManager;
extern BarNode *g_root;
extern BarArray *g_codes;
extern int g_codeLen;
extern char g_separator;
extern long *g_offsets;
extern int g_totalFiles, g_curFile;
extern char **g_fileNames;
extern int g_errorsPermited;
extern bool g_checkN, g_checkIndel, g_checkIns;
extern FILE *g_skipped, *g_summary;
extern volatile long g_totalExact, g_total1Error, g_total2Errors, g_totalN, g_totalIndel, g_totalIns, g_totalSkipped;
extern int g_totalMPs;
extern char **g_mpData;
extern int *g_mpPositions;
extern int g_mpLength;

bool addToTree(BarCode *code, int errors);

enum BarErrors
{
    NoError = 0,
    BadCodeInDB = 1,
    NInDB = 2,
    CantRead = 3,
    CantWrite = 4,
    CantAutoDetect = 5,
    TooLarge = 6,
    BadMPData = 7,
    BadGenParams = 8,
    DeadLoopDetected = 9
};
