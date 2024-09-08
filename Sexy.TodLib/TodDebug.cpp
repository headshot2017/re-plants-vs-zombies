#include <time.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdexcept>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include "TodDebug.h"
#include "TodCommon.h"
#include "misc/Debug.h"
#include "../SexyAppFramework/SexyAppBase.h"

using namespace Sexy;

static char gLogFileName[512];
static char gDebugDataFolder[512];

//0x514EA0
void TodErrorMessageBox(const char* theMessage, const char* theTitle)
{

#ifdef __SWITCH__
	ErrorApplicationConfig c;
	errorApplicationCreate(&c, theTitle, theMessage);
	errorApplicationShow(&c);
#else
	throw std::runtime_error("Error Box\n--" + std::string(theTitle) + "--\n" + theMessage);
#endif

	//probably will break
	FILE* f = fopen(GetAppDataFolder() + "userdata/error.txt", "a");
	if (f == nullptr)
	{
		TodLog(theMessage, __S("Failed to open log file '%s'\n"), GetAppDataFolder() + "userdata/error.txt");
		return;
	}

	if (fwrite(theMsg, strlen(theMsg), 1, f) != 1)
	{
		TodLog(stderr, __S("Failed to write to log file\n"));
	}

	fclose(f);
}

void TodTraceMemory()
{
}

void* TodMalloc(int theSize)
{
	TOD_ASSERT(theSize > 0);
	return malloc(theSize);
}

void TodFree(void* theBlock)
{
	if (theBlock != nullptr)
	{
		free(theBlock);
	}
}

void TodAssertFailed(const char* theCondition, const char* theFile, int theLine, const char* theMsg, ...)
{
	char aFormattedMsg[1024];
	va_list argList;
	va_start(argList, theMsg);
	int aCount = TodVsnprintf(aFormattedMsg, sizeof(aFormattedMsg), theMsg, argList);
	va_end(argList);

	if (aCount != 0) {
		if (aFormattedMsg[aCount - 1] != '\n')
		{
			if (aCount + 1 < 1024)
			{
				aFormattedMsg[aCount] = '\n';
				aFormattedMsg[aCount + 1] = '\0';
			}
			else
			{
				aFormattedMsg[aCount - 1] = '\n';
			}
		}
	}

	char aBuffer[1024];
	if (*theCondition != '\0')
	{
		TodSnprintf(aBuffer, sizeof(aBuffer), "\n%s(%d)\nassertion failed: '%s'\n%s\n", theFile, theLine, theCondition, aFormattedMsg);
	}
	else
	{
		TodSnprintf(aBuffer, sizeof(aBuffer), "\n%s(%d)\nassertion failed: %s\n", theFile, theLine, aFormattedMsg);
	}
	TodTrace("%s", aBuffer);

	TodErrorMessageBox(aBuffer, "Assertion failed");

	exit(0);
}

void TodLog(const char* theFormat, ...)
{
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

	TodLogString(aButter);
}

void TodLogString(const char* theMsg)
{
	FILE* f = fopen(gLogFileName, "a");
	if (f == nullptr)
	{
		fprintf(stderr, __S("Failed to open log file '%s'\n"), gLogFileName);
		return;
	}

	if (fwrite(theMsg, strlen(theMsg), 1, f) != 1)
	{
		fprintf(stderr, __S("Failed to write to log file\n"));
	}

	fclose(f);
}

void TodTrace(const char* theFormat, ...)
{
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

	printf("%s", aButter);
}

void TodHesitationTrace(...)
{
}

void TodTraceAndLog(const char* theFormat, ...)
{
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

	printf("%s", aButter);
	TodLogString(aButter);
}

void TodTraceWithoutSpamming(const char* theFormat, ...)
{
	static uint64_t gLastTraceTime = 0LL;
	uint64_t aTime = time(NULL);
	if (aTime < gLastTraceTime)
	{
		return;
	}

	gLastTraceTime = aTime;
	char aButter[1024];
	va_list argList;
	va_start(argList, theFormat);
	int aCount = TodVsnprintf(aButter, sizeof(aButter), theFormat, argList);
	va_end(argList);

	if (aButter[aCount - 1] != '\n')
	{
		if (aCount + 1 < 1024)
		{
			aButter[aCount] = '\n';
			aButter[aCount + 1] = '\0';
		}
		else
		{
			aButter[aCount - 1] = '\n';
		}
	}

	printf("%s", aButter);
}

void TodAssertInitForApp()
{
	MkDir(GetAppDataFolder());
	MkDir(GetAppDataFolder() + "userdata");
	std::string aRelativeUserPath = GetAppDataFolder() + "userdata/";
	strcpy(gDebugDataFolder, aRelativeUserPath.c_str());
	strcpy(gLogFileName, gDebugDataFolder);
	strcpy(gLogFileName + strlen(gLogFileName), "log.txt");
	TOD_ASSERT(strlen(gLogFileName) < 512);

	TodLog("Started %d\n", (uint64_t)time(NULL));
}
