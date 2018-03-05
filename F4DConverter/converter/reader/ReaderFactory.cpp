
#include "stdafx.h"

#include "ReaderFactory.h"

#include <cctype>
#include <string>
#include <algorithm>
#include "../LogWriter.h"

#ifdef SHIJTFORMAT
#include "JtReader.h"
#endif
#ifdef IFCFORMAT
#include "IfcReader.h"
#endif
#ifdef CLASSICFORMAT
#include "ClassicFormatReader.h"
#endif

ReaderFactory::ReaderFactory()
{
}

ReaderFactory::~ReaderFactory()
{
}

aReader* ReaderFactory::makeReader(std::wstring& filePath)
{
	std::wstring::size_type dotPosition = filePath.rfind(L".");
	if(dotPosition == std::wstring::npos)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(INVALID_TRIANGLE_COUNT), false);
		return NULL;
	}

	std::wstring::size_type fileExtLength = filePath.length() - dotPosition - 1;

	std::wstring fileExt = filePath.substr(dotPosition + 1, fileExtLength);

	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), towlower);

#ifdef SHIJTFORMAT
	if(fileExt.compare(std::wstring(L"jt")) == 0)
	{
		return new JtReader;
	}
#endif

#ifdef IFCFORMAT
	if (fileExt.compare(std::wstring(L"ifc")) == 0)
	{
		return new IfcReader;
	}
#endif

#ifdef CLASSICFORMAT
	if (fileExt.compare(std::wstring(L"obj")) == 0 ||
		fileExt.compare(std::wstring(L"dae")) == 0||
		fileExt.compare(std::wstring(L"3ds")) == 0)
	{
		return new ClassicFormatReader;
	}
#endif

	return NULL;
}