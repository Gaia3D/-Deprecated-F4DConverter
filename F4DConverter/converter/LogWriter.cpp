
#include "stdafx.h"

#include "LogWriter.h"

#include <fstream>
#include <time.h>

LogWriter LogWriter::logWriter; 

LogWriter::LogWriter()
{
	numberOfFilesToBeConverted = 0;

	numberOfFilesConverted = 0;
}

LogWriter::~LogWriter()
{
}

void LogWriter::setFullPath(std::wstring& path)
{
	fullPath = path;
}

void LogWriter::addContents(std::wstring& contents, bool newLine)
{
	logContents += contents;
	if(newLine)
		logContents += std::wstring(L"\n");
}

void LogWriter::clearContents()
{
	logContents.clear();
}

void LogWriter::save()
{
	std::wofstream outFile;
	outFile.open(fullPath);

	// 1. result
	wchar_t stringLine[1024];
	memset(stringLine, 0x00, sizeof(wchar_t)* 1024);
	wsprintf(stringLine, L"%u of %u files have been converted.\n", numberOfFilesConverted, numberOfFilesToBeConverted);
	outFile << stringLine;

	outFile << L"-----------------------------------------------------\n";

	// 2. conversion time
	outFile << L"start time : " << startTime << L"\n";
	outFile << L"end time   : " << endTime << L"\n";

	outFile << L"-----------------------------------------------------\n";


	// 3. detailed result

	outFile << logContents;

	outFile.close();
}

void LogWriter::setStatus(bool bSuccess)
{
	isSuccess = bSuccess;
}

void LogWriter::start()
{
	startTime = getCurrentTimeString();
}

void LogWriter::finish()
{
	endTime = getCurrentTimeString();
}

bool LogWriter::isStarted()
{
	return !(startTime.empty());
}

std::wstring LogWriter::getCurrentTimeString()
{
	time_t     now = time(0);
    struct tm*  tstruct = NULL;
    tstruct = localtime(&now);

	wchar_t       buf[80];
	memset(buf, 0x00, sizeof(wchar_t)*80);
	wcsftime(buf, sizeof(buf), L"%Y-%m-%d %X", tstruct);

	return std::wstring(buf);
}