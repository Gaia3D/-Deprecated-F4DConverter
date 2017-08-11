#pragma once

#include <string>

#define ERROR_FLAG					L"[ERROR]"
#define WARNING_FLAG				L"[WARNING]"
#define NO_BLOCK_ID					L"[No block ID]"
#define NO_OBJECT_ID				L"[No object ID]"
#define NO_TRANSFORM_MATRIX			L"[No Transform Matrix]"
#define NO_VERTEX_COUNT				L"[No vertex count]"
#define NO_VERTEX_ARRAY				L"[No vertex array]"
#define NO_INDEX_COUNT				L"[No index count]"
#define NO_INDEX_ARRAY				L"[No index array]"
#define NO_NORMAL_COUNT				L"[No normal vector count]"
#define NO_NORMAL_ARRAY				L"[No normal vector array]"
#define INVALID_TRIANGLE_COUNT		L"[Invalid triangle count]"
#define UNKNOWN_NODE_TYPE			L"[Unknown node type]"
#define NO_DATA_OR_INVALID_PATH		L"[No raw data or invalid data path]"
#define UNSUPPORTED_FORMAT			L"[Unsupported raw data format]"
#define CANNOT_LOAD_FILE			L"[Unable to read data file]"
#define NO_DATA_IN_RAW_DATA			L"[No data in raw data file]"
#define CONVERSION_FAILURE			L"[Conversion Failed]"
#define CANNOT_CREATE_DC			L"[Unable to create device context]"
#define CANNOT_CHOOSE_PF			L"[Unable to choose appropriate pixel format for device context]"
#define CANNOT_SET_PF				L"[Unable to set up pixel format for device context]"
#define CANNOT_CREATE_GL_CONTEXT	L"[Unable to create OpenGL context for device context]"
#define CANNOT_CONNECT_GLC_TO_DC	L"[Unable to connect OpenGL context to device context]"
#define CANNOT_INITIALIZE_GLEW		L"[Unable to initialize GLEW]"
#define CANNOT_INITIALIZE			L"[Unable to initialize this application]"
#define CANNOT_CREATE_DIRECTORY		L"[Unable to create the conversion result directory]"
#define UNLOADABLE_MESH_EXISTS		L"[At least 1 unloadable mesh exists]"

class LogWriter
{
private:
	LogWriter();

public:
	virtual ~LogWriter();

	unsigned int numberOfFilesToBeConverted;

	unsigned int numberOfFilesConverted;

	

private:
	static LogWriter logWriter;

	std::wstring startTime;

	std::wstring endTime;

	std::wstring logContents;

	std::wstring fullPath;

	bool isSuccess;

public:
	static LogWriter* getLogWriter() {return &logWriter;}

	void setFullPath(std::wstring& path);

	void addContents(std::wstring& contents, bool newLine);

	void clearContents();

	void save();

	void setStatus(bool bSuccess);

	void start();

	void finish();

	bool isStarted();

private:
	std::wstring getCurrentTimeString();
};