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

#include <cassert>
#include <cstdio>
#include <ctime>
#include <memory>

#define MY_APP_NAME	"BarAnalyzer"

char g_alphabet[] = "ATGCN";
BarNodeManager *g_nodeManager = NULL;
BarNode *g_root = NULL;
BarArray *g_codes = NULL;
int g_codeLen = 0;
char g_separator = 0;
long *g_offsets;
int g_totalFiles, g_curFile;
char **g_fileNames;
int g_errorsPermited;
bool g_checkN, g_checkIndel, g_checkIns;
FILE *g_skipped = NULL, *g_summary = NULL;
int g_totalMPs;
char **g_mpData;
int *g_mpPositions;
int g_mpLength;
char *g_header;


bool addToTree(BarCode *code, int errors)
{
    BarNode *curNode = g_root;
    for (int i = 0; i < code->len(); i++)
    {
        t_code curSymbol = code->symbol(i);
        BarNode *newNode = curNode->getChild(curSymbol);
        if (!newNode) // Make new
            curNode = curNode->newChild(curSymbol);
        else
            curNode = newNode;
    }
    if (curNode->getBarCode())// && (curNode->m_code != code || curNode->m_errors != errors)) // Ups!
    {
        return false;

        assert(false);
    }
    curNode->setBarCode(code);
    curNode->setErrors(errors);
    return true;
}

char *nextLine(char *str)
{
    while (*str != 0xd && *str != 0xa && *str)
        ++str;
    while (*str == 0xd || *str == 0xa)
    {
        *str = 0;
        ++str;
    }
    if (*str)
        return str;
    return NULL;
}

BARCORE_API int checkLibrary(const char *path) // Returns max code lenght or <= 0 if failed
{
    FILE *in;
    int outLen;
    uint8_t *outData;

    if ((in = fopen(path, "rb")) == NULL)
        return -1;

    // Read input
    fseek(in, 0, SEEK_END);
    int inLen = ftell(in);
    fseek(in, 0, SEEK_SET);

    uint8_t *inData = (uint8_t *)malloc(inLen);
    fread(inData, 1, inLen, in);
    fclose(in);

    outData = inData;
    outLen = inLen;

    char *dataLine = nextLine((char *)outData);
    if (!dataLine)
    {
        free(outData);
        return -3;
    }

    char *pos = strchr(dataLine, ',');
    if (!pos)
    {
        free(outData);
        return -3;
    }

    int barLen = (int) (pos - dataLine);
    free(outData);
    return barLen;
}


BARCORE_API int loadLibrary(const char *path, int barStart /*1 is first */, int barLength, int errorsPermited/*, int idsColumn, int descColumn, char separator*/, int totalFiles, int totalMPs, int optimizeMemory)
{
    Logger::Log("Load library called with path: %s", path);

    if (g_nodeManager)
        delete g_nodeManager;
    if (g_codes)
        delete g_codes;
    if (optimizeMemory)
    {
        g_nodeManager = new PagedPointerBarNodeManager();
    } else {
        g_nodeManager = new SimpleBarNodeManager();
    }
    g_root = g_nodeManager->newRoot(); //new BarNode(0);
    g_codes = new BarArray();
    g_codeLen = barLength;
    g_errorsPermited = errorsPermited;
    g_totalFiles = totalFiles;
    g_totalMPs = totalMPs;

    time_t start = time(NULL);
    int ids = 0;
    long added2 = 0, skipped2 = 0;


    FILE *db;
    
    if ((db = fopen(path, "rb")) == NULL)
        return CantRead;

    fseek(db, 0, SEEK_END);
    int fsize = ftell(db);
    fseek(db, 0, SEEK_SET);

    uint8_t *inBuf = (uint8_t *)malloc(fsize+1);
    fread(inBuf, 1, fsize, db);
    fclose(db);

    // Decode
    int outLen;
    char *buf;

    outLen = fsize;
    buf = (char *)inBuf;

    buf[outLen] = 0;

    char *curStr = nextLine(buf);
    // Copy header
    g_header = (char *)malloc((strlen(buf)+1)*sizeof(char));
    strcpy(g_header, buf);

    while (curStr)
    {
        char *nextStr = nextLine(curStr);
        BarCode *code = new BarCode(barLength, g_totalFiles, g_totalMPs);
        if (!code->loadFromString(curStr, 0/*idsColumn*/, barStart, /*descColumn*/18, ','/*separator*/))
        {
            Logger::Log("Can't read code in DB");
            free(buf);
            return BadCodeInDB;
        }
        // Check for N - it should not exist here
        if (code->m_NPosition >= 0)
        {
            Logger::Log("Encountered N in DB codes");
            free(buf);
            return NInDB;
        }
//		code->dump();

        g_codes->put(code);

        // Put original
        addToTree(code, 0);
    
        if (errorsPermited > 0)
        {
            while (code->mutate()) // 1st level of mutations
            {
                if (!addToTree(code, 1)) // Should not happen here
                {
                    Logger::Log("Failed to add 1 error code");
                    free(buf);
                    return BadCodeInDB;
                }
            } 
        }
        ++ids;
        curStr = nextStr;
    }
    free(buf);

    // 2 error codes here
    if (errorsPermited > 1)
        for (int i = 0; i < g_codes->count(); i++)
        {
            BarCode *code = g_codes->get(i);
            code->initMutations();
            while (code->mutate()) // 1st level of mutations
            {
                while (code->mutate2()) // 2nd level
                {
                    // Manually here, we should analyze
                    BarNode *curNode = g_root;
                    for (int j = 0; j < code->len(); j++)
                    {
                        t_code curSymbol = code->symbol(j);
                        BarNode *newNode = curNode->getChild(curSymbol);
                        if (!newNode) // Make new
                            curNode = curNode->newChild(curSymbol);
                        else
                            curNode = newNode;
                    }
                    if (!curNode->getBarCode()) // All ok, new code
                    {
                        curNode->setBarCode(code);
                        curNode->setErrors(2);
                        added2++;
                        continue;
                    }

                    // Already have leaf here 
                    if (curNode->getErrors() < 2) // have "good" code here, so skip current
                    {
                        skipped2++;
                        continue;
                    }
                    if (curNode->getErrors() == 2) // Bad case, double hit one node
                    {
                        curNode->setErrors(3); // Indicate it's bad
                        --added2; // correct counter
                        ++skipped2;
                        continue;
                    }
                    // Already marked as bad, so just increase counter
                    ++skipped2;
                }
            } 
        }


    time_t end = time(NULL);

    Logger::Log("Read %d ids in %d seconds", ids, end-start);
    if (errorsPermited > 1) // Stats
        Logger::Log("%d double-errors was added and %d skipped", added2, skipped2);

    return NoError;
}


BARCORE_API int prepare(const char *outPath, const char *skippedPath, const char *summaryPath, bool checkN, bool checkIndel, bool checkIns, const char **mpData, const int *mpPositions)
{
    g_checkN = checkN;
    g_checkIndel = checkIndel;
    g_checkIns = checkIns;

    g_curFile = 0;
    g_fileNames = (char **)malloc(g_totalFiles*sizeof(char *));
    memset(g_fileNames, 0, g_totalFiles*sizeof(char *));

    // Try open/close output
    FILE *o;
    
    if ((o = fopen(outPath, "wt")) == NULL)
        return CantWrite;
    fclose(o);

    // MP support
    if (mpData && mpPositions) // Have mp
    {
        // 1st, parse and copy data
        g_mpData = (char **)malloc(g_totalMPs*sizeof(char *));
        memset(g_mpData, 0, g_totalMPs*sizeof(char *));
        g_mpLength = 0;
        for (int i = 0; i < g_totalMPs; i++)
        {
            int ln = (int)strlen(mpData[i]);
            if (g_mpLength == 0)
                g_mpLength = ln;
            else
                if (g_mpLength != ln)
                    return BadMPData;
            g_mpData[i] = (char *)malloc((ln+1)*sizeof(char));
            strcpy(g_mpData[i], mpData[i]);
        }
        // 2nd, set positions
        g_mpPositions = (int *)malloc((g_mpLength+g_codeLen)*sizeof(int));
        memcpy_s(g_mpPositions, (g_mpLength+g_codeLen)*sizeof(int), mpPositions, (g_mpLength+g_codeLen)*sizeof(int));
    }
    else
    {
        g_mpLength = 1;
        g_mpData = NULL;
        g_mpPositions = NULL;
    }


    g_totalExact = g_total1Error = g_totalN = g_totalIndel = g_totalIns = g_totalSkipped = 0;

    // open skipped
    g_skipped = fopen(skippedPath, "wb");
    // open summary
    g_summary = fopen(summaryPath, "wt");
    if (g_summary) // Write header
    {
        fputs("File,Skipped,Exact", g_summary);
        if (g_errorsPermited > 0)
            fputs(",1 Error", g_summary);
        if (g_errorsPermited > 1)
            fputs(",2 Errors", g_summary);
        if (g_checkN)
            fputs(",N corrected", g_summary);
        if (g_checkIndel)
            fputs(",Indels", g_summary);
        if (g_checkIns)
            fputs(",Insertions", g_summary);
        if (g_checkN || g_checkIndel || g_checkIns || g_errorsPermited > 0)
            fputs(",Processed", g_summary);
        fputs("\n", g_summary);
    }
    return NoError;
}

BARCORE_API void cleanup()
{
    for (int i = 0; i < g_totalFiles; i++)
    {
        if (g_fileNames[i])
            free(g_fileNames[i]);
    }
    free(g_fileNames);

    if (g_nodeManager)
        delete g_nodeManager;
    g_nodeManager = NULL;
    if (g_codes)
        delete g_codes;
    g_codes = NULL;


    free(g_header);

    if (g_skipped) 
        fclose(g_skipped);
    if (g_summary)
    {
        // Dump totals
        fprintf(g_summary, "Total,%ld,%ld", g_totalSkipped, g_totalExact);
        if (g_errorsPermited > 0)
            fprintf(g_summary, ",%ld", g_total1Error);
        if (g_errorsPermited > 1)
            fprintf(g_summary, ",%ld", g_total2Errors);
        if (g_checkN)
            fprintf(g_summary, ",%ld", g_totalN);
        if (g_checkIndel)
            fprintf(g_summary, ",%ld", g_totalIndel);
        if (g_checkIns)
            fprintf(g_summary, ",%ld", g_totalIns);
        if (g_checkN || g_checkIndel || g_checkIns || g_errorsPermited > 0)
            fprintf(g_summary, ",%ld", g_totalIns+g_totalIndel+g_totalN+g_total1Error+g_total2Errors+g_totalExact);
        fputs("\n", g_summary);
        fclose(g_summary);
    }

    // Free MP data
    if (g_mpPositions)
        free(g_mpPositions);
    if (g_mpData)
    {
        for (int i = 0; i < g_totalMPs; i++)
            if (g_mpData[i])
                free (g_mpData[i]);
        free(g_mpData);
    }

}

BARCORE_API int dumpResults(const char *outPath)
{
    time_t start = time(NULL);
    int ids = 0;

    FILE *summary;
    if ((summary = fopen(outPath, "wt")) == NULL)
    {
        return CantWrite;
    }

    // Dump
    // Header
    fprintf(summary, "%s", g_header);

    char lb[2] = {'(', 0};
    char rb[2] = {')', 0};
    if (!g_mpData)
        lb[0] = rb[0] = 0;

    for (int i = 0; i < g_totalFiles; i++)
    {
        for (int j = 0; j < g_totalMPs; j++)
        {
            fprintf(summary, ",%s%s%s%s Exact", g_fileNames[i], lb, g_mpData?g_mpData[j]:"", rb);
            if (g_errorsPermited > 0)
                fprintf(summary, ",%s%s%s%s 1 Error", g_fileNames[i], lb, g_mpData?g_mpData[j]:"", rb);
            if (g_errorsPermited > 1)
                fprintf(summary, ",%s%s%s%s 2 Errors", g_fileNames[i], lb, g_mpData?g_mpData[j]:"", rb);
            if (g_checkN)
                fprintf(summary, ",%s%s%s%s N corrected", g_fileNames[i], lb, g_mpData?g_mpData[j]:"", rb);
            if (g_checkIndel)
                fprintf(summary, ",%s%s%s%s Indels", g_fileNames[i], lb, g_mpData?g_mpData[j]:"", rb);
            if (g_checkIns)
                fprintf(summary, ",%s%s%s%s Insertions", g_fileNames[i], lb, g_mpData?g_mpData[j]:"", rb);
            if (g_checkN || g_checkIndel || g_checkIns || g_errorsPermited > 0)
                fprintf(summary, ",%s%s%s%s Processed", g_fileNames[i], lb, g_mpData?g_mpData[j]:"", rb);
        }
    }
    fprintf(summary, "\n");
    for (int i = 0; i < g_codes->count(); i++)
    {
        BarCode *code = g_codes->get(i);
        if (code->hasZeroCounts()) // Skip
            continue;
        fprintf(summary, "%s", code->m_strDesc);
        for (int i = 0; i < g_totalFiles; i++)
        {
            for (int j = 0; j < g_totalMPs; j++)
            {
                fprintf(summary,",%ld", code->getExactCount(i, j));
                if (g_errorsPermited > 0)
                    fprintf(summary,",%ld", code->get1ErrorCount(i, j));
                if (g_errorsPermited > 1)
                    fprintf(summary,",%ld", code->get2ErrorCount(i, j));
                if (g_checkN)
                    fprintf(summary,",%ld", code->getNCount(i, j));
                if (g_checkIndel)
                    fprintf(summary,",%ld", code->getIndelCount(i, j));
                if (g_checkIns)
                    fprintf(summary,",%ld", code->getInsCount(i, j));
                if (g_checkN || g_checkIndel || g_checkIns || g_errorsPermited > 0)
                    fprintf(summary,",%ld", code->getTotalCount(i, j));
            }
        }
        ++ids;
        fprintf(summary, "\n");
    }
    fclose(summary);

    time_t end = time(NULL);

    Logger::Log("Dumped %d ids in %d seconds\n", ids, end-start);

    return NoError;
}

#ifdef __APPLE__
void Logger::Log(const char *format, ...)
{
    // Do nothing for now
}
#endif