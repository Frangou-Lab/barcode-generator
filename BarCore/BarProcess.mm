#include "stdafx.h"
#include "BarCore.h"
#include "BarClasses.h"

long g_remainingData;
char *g_pData;
char g_CR;
int g_nCR;
bool g_counterFirst;
NSLock* g_cs;
volatile long g_totalExact, g_total1Error, g_total2Errors, g_totalN, g_totalIndel, g_totalIns, g_totalSkipped;
volatile long_t *g_fileExact, *g_file1Error, *g_file2Errors, *g_fileN, *g_fileIndel, *g_fileIns, g_fileSkipped;
volatile long totalIds;

BarNode *getNodeForCode(BarCode *code)
{
	// Find in tree
	BarNode *curNode = g_root;
	for (int i = 0; i < g_codeLen; i++)
	{
		t_code curSymbol = code->symbol(i);
		if (!(curNode = curNode->getChild(curSymbol))) // ups
			return NULL;
	}
	return curNode;
};

@interface TaskFounder : NSObject
{
	NSConditionLock* m_condition;
}
@property(retain, readonly) NSConditionLock* m_condition;
-(id) initWithNumberOfThreads: (int) numberOfThreads;
-(void) myProcessingThreadProc: (NSNumber*) param;
@end

@implementation TaskFounder
@synthesize m_condition;
-(id) initWithNumberOfThreads: (int) numberOfThreads;
{
	if (self = [super init])
	{
		m_condition = [[NSConditionLock alloc] initWithCondition: numberOfThreads];		
	}
	return self;
}
-(void) dealloc
{
	[m_condition release];
	[super dealloc];
}
-(void) myProcessingThreadProc: (NSNumber*) param
{
	int threadNum = [param intValue];
//	Logger::Log("Thread %d started", threadNum);
	BarCode *code = new BarCode(g_codeLen, 0, g_totalMPs);
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
			int cnt = atoi(buf+r.position);
			if (cnt > 0)
				curCount = cnt;
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
							_ASSERTE(curNode->m_code);
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
						_ASSERTE(curNode->m_code);
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
			if (curNode) // Aha!
			{
				_ASSERTE(curNode->m_code);
				
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
			[g_cs lock];
			fwrite(buf, g_pData+curOffset-buf, 1, g_skipped);
			[g_cs unlock];
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
//	Logger::Log("Thread %d finished", threadNum/*GetCurrentThreadId()*/);
	[m_condition lock];
	NSInteger newNumberOfRunningThreads = [m_condition condition] - 1;
	[m_condition unlockWithCondition: newNumberOfRunningThreads];
}
@end

const char* getFileNameWithoutExt(const char* path)
{
	NSString* pathAsString = [NSString stringWithCString: path encoding: NSUTF8StringEncoding];
	NSString* fileNameWithoutExt = [[pathAsString lastPathComponent] stringByDeletingPathExtension];
	return [fileNameWithoutExt cStringUsingEncoding: NSUTF8StringEncoding];
}

BARCORE_API int processFile(const char *inPath)
{
	NSLog(@"Size of %lu, page size %u", sizeof(g_root), g_nodeManager->getPageSize());
	time_t start = time(NULL);
	
	const char* fileNameWithoutExt = getFileNameWithoutExt(inPath);
	//for capability with windows version
	g_fileNames[g_curFile] = (char *) malloc((strlen(fileNameWithoutExt) + 1) * sizeof(char));
	strcpy(g_fileNames[g_curFile], fileNameWithoutExt);

	g_fileExact = (volatile long_t *)calloc(g_totalMPs, sizeof(long_t));
	g_file1Error = (volatile long_t *)calloc(g_totalMPs, sizeof(long_t));
	g_file2Errors = (volatile long_t *)calloc(g_totalMPs, sizeof(long_t));
	g_fileN = (volatile long_t *)calloc(g_totalMPs, sizeof(long_t));
	g_fileIndel = (volatile long_t *)calloc(g_totalMPs, sizeof(long_t));
	g_fileIns = (volatile long_t *)calloc(g_totalMPs, sizeof(long_t));
	g_fileSkipped = 0;
	
	// Create mapping
	FILE* inputFile = fopen(inPath, "rt");
	if (NULL == inputFile)
	{
		return CantRead;
	}
	fseek(inputFile, 0, SEEK_END);
	long length = ftell(inputFile);
	rewind(inputFile);
	
	g_remainingData = length;
	g_pData = (char* )malloc((g_remainingData+1) * sizeof(char));
	fread(g_pData, 1, g_remainingData, inputFile);
	if (0 != ferror(inputFile))
	{
		fclose(inputFile);
		return CantRead;
	}
	fclose(inputFile);
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
	g_cs = [[NSLock alloc] init];

	NSProcessInfo* pi = [NSProcessInfo processInfo];
	int threadNum = [pi activeProcessorCount] ;
	
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
			_ASSERTE(offset);
			offset += g_nCR;
			_ASSERTE(offset > g_offsets[thread-1]);
		}
		g_offsets[thread] = offset;
	}
	
	Logger::Log("Starting %d processing threads...", threadNum);
	TaskFounder* taskFounder = [[TaskFounder alloc] initWithNumberOfThreads: threadNum];
	for (int currentThread = 0; currentThread < threadNum; currentThread++)
	{
		[taskFounder performSelectorInBackground:@selector(myProcessingThreadProc:) withObject: [NSNumber numberWithInt: currentThread]];
	}
	[[taskFounder m_condition] lockWhenCondition: 0];
	[[taskFounder m_condition] unlock];
	[taskFounder release];
	
	Logger::Log("Waiting for completion of threads");
	// Wait all exit
	Logger::Log("All threads exited");
	
	[g_cs release];

	if (g_summary) // Dump string
	{
		for (int i = 0; i < g_totalMPs; i++)
		{
			if (!g_mpData)
				fprintf(g_summary, "%s,%d,%d", g_fileNames[g_curFile], g_fileSkipped, g_fileExact[i]);
			else
				fprintf(g_summary, "%s(%s),%d,%d", g_fileNames[g_curFile], g_mpData[i], g_fileSkipped, g_fileExact[i]);
			if (g_errorsPermited > 0)
				fprintf(g_summary, ",%d", g_file1Error[i]);
			if (g_errorsPermited > 1)
				fprintf(g_summary, ",%d", g_file2Errors[i]);
			if (g_checkN)
				fprintf(g_summary, ",%d", g_fileN[i]);
			if (g_checkIndel)
				fprintf(g_summary, ",%d", g_fileIndel[i]);
			if (g_checkIns)
				fprintf(g_summary, ",%d", g_fileIns[i]);
			if (g_checkN || g_checkIndel || g_checkIns || g_errorsPermited > 0)
				fprintf(g_summary, ",%d", g_fileIns[i]+g_fileIndel[i]+g_fileN[i]+g_file1Error[i]+g_file2Errors[i]+g_fileExact[i]);
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
