// ./converter/ConverterManager.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "ConverterManager.h"

#include <io.h>
#include <sys/stat.h>

#include "./reader/ReaderFactory.h"
#include "./process/ConversionProcessor.h"
#include "./process/SceneControlVariables.h"
#include "./writer/F4DWriter.h"

#include "LogWriter.h"


// CConverterManager

CConverterManager CConverterManager::m_ConverterManager;

CConverterManager::CConverterManager()
{
	processor = new ConversionProcessor();

	bCreateIndices = bCliMode = bConversion = false;

	bOcclusionCulling = false;

	unitScaleFactor = 1.0;
}

CConverterManager::~CConverterManager()
{
	delete processor;
}


// CConverterManager 멤버 함수

bool CConverterManager::initialize(HDC hDC, int width, int height)
{
	if(hDC == NULL)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_CREATE_DC), true);
		return false;
	}

	return processor->initialize(hDC, width, height);
}

void CConverterManager::changeGLDimension(int width, int height)
{
	processor->defaultSpaceSetupForVisualization(width, height);
}

bool CConverterManager::processDataFolder()
{
	std::wstring inputFolder = inputFolderPath;
	std::wstring outputFolder = outputFolderPath;
	// test if output folder exist
	bool outputFolderExist = false;
	if (_waccess(outputFolder.c_str(), 0) == 0)
	{
		struct _stat64i32 status;
		_wstat(outputFolder.c_str(), &status);
		if (status.st_mode & S_IFDIR)
			outputFolderExist = true;
	}

	if (!outputFolderExist)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(NO_DATA_OR_INVALID_PATH), true);
		return false;
	}

	bool inputFolderExist = false;
	if (_waccess(inputFolder.c_str(), 0) == 0)
	{
		struct _stat64i32 status;
		_wstat(inputFolder.c_str(), &status);
		if (status.st_mode & S_IFDIR)
			inputFolderExist = true;
	}

	if (!inputFolderExist)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(NO_DATA_OR_INVALID_PATH), true);
		return false;
	}

	processDataFolder(inputFolder);


	return true;
}

bool CConverterManager::processSingleFile(std::wstring& filePath)
{
	std::wstring outputFolder = outputFolderPath;
	bool outputFolderExist = false;
	if (_waccess(outputFolder.c_str(), 0) == 0)
	{
		struct _stat64i32 status;
		_wstat(outputFolder.c_str(), &status);
		if (status.st_mode & S_IFDIR)
			outputFolderExist = true;
	}

	if (!outputFolderExist)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(NO_DATA_OR_INVALID_PATH), true);
		return false;
	}

	bool bRawDataFileExists = false;
	if (_waccess(filePath.c_str(), 0) == 0)
	{
		struct _stat64i32 status;
		_wstat(filePath.c_str(), &status);
		if ((status.st_mode & S_IFDIR) != S_IFDIR)
			bRawDataFileExists = true;
	}

	if (!bRawDataFileExists)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(NO_DATA_OR_INVALID_PATH), true);
		return false;
	}

	aReader* reader = ReaderFactory::makeReader(filePath);
	if (reader == NULL)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(UNSUPPORTED_FORMAT), true);
		return false;
	}

	std::wstring fileName;
	std::wstring::size_type slashPosition = filePath.find_last_of(L"\\/");
	if (slashPosition == std::wstring::npos)
		fileName = filePath;
	else
		fileName = filePath.substr(slashPosition + 1, filePath.length() - slashPosition - 1);

	reader->setUnitScaleFactor(unitScaleFactor);

	if (!processDataFile(filePath, reader))
	{
		LogWriter::getLogWriter()->addContents(filePath, true);
		delete reader;
		processor->clear();
		return false;
	}

	delete reader;

	std::wstring::size_type dotPosition = fileName.rfind(L".");
	std::wstring fullId = fileName.substr(0, dotPosition);
	if (!idPrefix.empty())
		fullId = idPrefix + fullId;

	if (!idSuffix.empty())
		fullId += idSuffix;

	processor->addAttribute(std::wstring(L"id"), fullId);

	F4DWriter writer(processor);
	writer.setWriteFolder(outputFolder);
	if (!writer.write())
	{
		LogWriter::getLogWriter()->addContents(filePath, true);
		processor->clear();
		return false;
	}

	processor->clear();

	return true;
}

void CConverterManager::processDataFolder(std::wstring inputFolder)
{
	_wfinddata64_t fd;
	long long handle;
	int result = 1;
	std::wstring fileFilter = inputFolder + std::wstring(L"/*.*");
	handle = _wfindfirsti64(fileFilter.c_str(), &fd);

	if (handle == -1)
		return;

	std::vector<std::wstring> dataFiles;
	std::vector<std::wstring> subFolders;
	while (result != -1)
	{
		if ((fd.attrib & _A_SUBDIR) == _A_SUBDIR)
		{
			if (std::wstring(fd.name) != std::wstring(L".") && std::wstring(fd.name) != std::wstring(L".."))
			{
				std::wstring subFolderFullPath = inputFolder + L"/" + std::wstring(fd.name);
				subFolders.push_back(subFolderFullPath);
			}
		}
		else
		{
			dataFiles.push_back(std::wstring(fd.name));
		}

		result = _wfindnexti64(handle, &fd);
	}

	_findclose(handle);

	size_t subFolderCount = subFolders.size();
	for (size_t i = 0; i < subFolderCount; i++)
	{
		processDataFolder(subFolders[i]);
	}

	size_t dataFileCount = dataFiles.size();
	if (dataFileCount == 0)
		return;

	std::wstring outputFolder = outputFolderPath;

	std::wstring fullId;
	for (size_t i = 0; i < dataFileCount; i++)
	{
		// 1. raw data file을 하나씩 변환
		std::wstring dataFileFullPath = inputFolder + std::wstring(L"/") + dataFiles[i];

		aReader* reader = ReaderFactory::makeReader(dataFileFullPath);
		if (reader == NULL)
			continue;

		LogWriter::getLogWriter()->numberOfFilesToBeConverted += 1;
		reader->setUnitScaleFactor(unitScaleFactor);

		if (!processDataFile(dataFileFullPath, reader))
		{
			LogWriter::getLogWriter()->addContents(dataFiles[i], true);
			delete reader;
			processor->clear();
			continue;
		}

		delete reader;

		std::wstring::size_type dotPosition = dataFiles[i].rfind(L".");
		fullId = dataFiles[i].substr(0, dotPosition);
		if (!idPrefix.empty())
			fullId = idPrefix + fullId;

		if (!idSuffix.empty())
			fullId += idSuffix;

		processor->addAttribute(std::wstring(L"id"), fullId);


		// 2. 변환 결과에서 bbox centerpoint를 로컬 원점으로 이동시키는 변환행렬 추출

		// 3. 변환 결과를 저장
		F4DWriter writer(processor);
		writer.setWriteFolder(outputFolder);
		if (!writer.write())
		{
			LogWriter::getLogWriter()->addContents(dataFiles[i], true);
			processor->clear();
			continue;
		}

		// 4. processor clear
		processor->clear();
		LogWriter::getLogWriter()->numberOfFilesConverted += 1;
	}
}

bool CConverterManager::writeIndexFile()
{
	F4DWriter writer(processor);
	writer.setWriteFolder(outputFolderPath);
	writer.writeIndexFile();

	return true;
}

bool CConverterManager::processDataFile(std::wstring& filePath, aReader* reader)
{
	if (!reader->readRawDataFile(filePath))
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_LOAD_FILE), false);
		return false;
	}

	if (reader->getDataContainer().size() == 0)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(NO_DATA_IN_RAW_DATA), false);
		return false;
	}

	if(!processor->proceedConversion(reader->getDataContainer(), reader->getTextureInfoContainer(), true, bOcclusionCulling))
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(CONVERSION_FAILURE), false);
		return false;
	}

	return true;
}

bool CConverterManager::isInitialized()
{
	if(processor->getSceneControlVariables()->m_width == 0 || processor->getSceneControlVariables()->m_height == 0||
		processor->getSceneControlVariables()->m_myhDC == 0 || processor->getSceneControlVariables()->m_hRC == 0)
		return false;

	return true;
}
