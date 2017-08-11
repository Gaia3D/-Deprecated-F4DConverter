
// F4DConverter.cpp : 응용 프로그램에 대한 클래스 동작을 정의합니다.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "F4DConverter.h"
#include "MainFrm.h"

#include "F4DConverterDoc.h"
#include "F4DConverterView.h"

#include <map>
#include <vector>
#include <sstream>
#include <locale>
#include <algorithm>

#include "converter/LogWriter.h"
#include "converter/ConverterManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CF4DConverterApp

BEGIN_MESSAGE_MAP(CF4DConverterApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CF4DConverterApp::OnAppAbout)
	// 표준 파일을 기초로 하는 문서 명령입니다.
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
END_MESSAGE_MAP()

std::map<std::wstring, std::wstring> arguments;

// arguments for conversion configuration
std::wstring inputFolder(L"Input Folder");
std::wstring outputFolder(L"Output Folder");
std::wstring logPath(L"Log File Path");
std::wstring indexCreation(L"Create Indices");
std::wstring idPrefix(L"idPrefix");
std::wstring idSuffix(L"idSuffix");
// arguments for processing parameters
std::wstring occlusionCulling(L"Occlusion Culling");
std::wstring unitScaleFactor(L"Unit Scale Factor");

bool extractArguments(std::wstring& cmdLine)
{
	std::wstring token;
	std::wistringstream iss(cmdLine);
	std::vector<std::wstring> tokens;
	while (getline(iss, token, L' '))
	{
		tokens.push_back(token);
	}

	size_t tokenCount = tokens.size();
	for (size_t i = 0; i < tokenCount; i++)
	{
		if (tokens[i].substr(0, 1) == std::wstring(L"-"))
		{
			if (i == tokenCount - 1 || tokens[i + 1].substr(0, 1) == std::wstring(L"-"))
			{
				return false;
			}

			if (tokens[i] == std::wstring(L"-inputFolder"))
			{
				arguments[inputFolder] = tokens[i + 1];
				i++;
				continue;
			}
			if (tokens[i] == std::wstring(L"-outputFolder"))
			{
				arguments[outputFolder] = tokens[i + 1];
				i++;
				continue;
			}
			if (tokens[i] == std::wstring(L"-log"))
			{
				arguments[logPath] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::wstring(L"-indexing"))
			{
				arguments[indexCreation] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::wstring(L"-idPrefix"))
			{
				arguments[idPrefix] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::wstring(L"-idSuffix"))
			{
				arguments[idSuffix] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::wstring(L"-oc"))
			{
				arguments[occlusionCulling] = tokens[i + 1];
				i++;
				continue;
			}

			if (tokens[i] == std::wstring(L"-usf"))
			{
				arguments[unitScaleFactor] = tokens[i + 1];
				i++;
				continue;
			}

			// TODO(khj 20170305) : LOG invalid arguments
			return false;
		}
		else
		{
			// TODO(khj 20170305) : LOG invalid arguments
			return false;
		}
	}

	if (arguments.find(outputFolder) == arguments.end())
		return false;

	if (arguments.find(inputFolder) == arguments.end() && arguments.find(indexCreation) == arguments.end())
		return false;

	if (arguments.find(inputFolder) != arguments.end())
	{
		if (arguments.find(logPath) == arguments.end())
			return false;
	}

	if (arguments.find(indexCreation) != arguments.end())
	{
		if (arguments[indexCreation] != std::wstring(L"Y") &&
			arguments[indexCreation] != std::wstring(L"y") &&
			arguments[indexCreation] != std::wstring(L"N") &&
			arguments[indexCreation] != std::wstring(L"n"))
			return false;
	}

	if (arguments.find(occlusionCulling) != arguments.end())
	{
		if (arguments[occlusionCulling] != std::wstring(L"Y") &&
			arguments[occlusionCulling] != std::wstring(L"y") &&
			arguments[occlusionCulling] != std::wstring(L"N") &&
			arguments[occlusionCulling] != std::wstring(L"n"))
			return false;
	}

	if (arguments.find(unitScaleFactor) != arguments.end())
	{
		try
		{
			double scaleFactor = std::stod(arguments[unitScaleFactor]);

			if (scaleFactor < 0.001)
				return false;
		}
		catch(const std::invalid_argument& error)
		{
			std::string errorMessage = error.what();
			return false;
		}
		catch(const std::out_of_range& error)
		{
			std::string errorMessage = error.what();
			return false;
		}
	}

	return true;
}
// CF4DConverterApp 생성

CF4DConverterApp::CF4DConverterApp()
{
	// TODO: 아래 응용 프로그램 ID 문자열을 고유 ID 문자열로 바꾸십시오(권장).
	// 문자열에 대한 서식: CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("F4DConverter.AppID.NoVersion"));

	// TODO: 여기에 생성 코드를 추가합니다.
	// InitInstance에 모든 중요한 초기화 작업을 배치합니다.
}

// 유일한 CF4DConverterApp 개체입니다.

CF4DConverterApp theApp;


// CF4DConverterApp 초기화

BOOL CF4DConverterApp::InitInstance()
{
	// 응용 프로그램 매니페스트가 ComCtl32.dll 버전 6 이상을 사용하여 비주얼 스타일을
	// 사용하도록 지정하는 경우, Windows XP 상에서 반드시 InitCommonControlsEx()가 필요합니다. 
	// InitCommonControlsEx()를 사용하지 않으면 창을 만들 수 없습니다.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 응용 프로그램에서 사용할 모든 공용 컨트롤 클래스를 포함하도록
	// 이 항목을 설정하십시오.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	EnableTaskbarInteraction(FALSE);

	// RichEdit 컨트롤을 사용하려면  AfxInitRichEdit2()가 있어야 합니다.	
	// AfxInitRichEdit2();

	// 표준 초기화
	// 이들 기능을 사용하지 않고 최종 실행 파일의 크기를 줄이려면
	// 아래에서 필요 없는 특정 초기화
	// 루틴을 제거해야 합니다.
	// 해당 설정이 저장된 레지스트리 키를 변경하십시오.
	// TODO: 이 문자열을 회사 또는 조직의 이름과 같은
	// 적절한 내용으로 수정해야 합니다.
	SetRegistryKey(_T("로컬 응용 프로그램 마법사에서 생성된 응용 프로그램"));
	LoadStdProfileSettings(4);  // MRU를 포함하여 표준 INI 파일 옵션을 로드합니다.


	// 응용 프로그램의 문서 템플릿을 등록합니다.  문서 템플릿은
	//  문서, 프레임 창 및 뷰 사이의 연결 역할을 합니다.
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CF4DConverterDoc),
		RUNTIME_CLASS(CMainFrame),       // 주 SDI 프레임 창입니다.
		RUNTIME_CLASS(CF4DConverterView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	std::wstring cmdLine(this->m_lpCmdLine);

	bool isValidParams = false;
	if (cmdLine.length() != 0)
	{
		if (!(isValidParams = extractArguments(cmdLine)))
			return FALSE;
	}

	if (arguments.find(logPath) != arguments.end())
	{
		LogWriter::getLogWriter()->start();
		LogWriter::getLogWriter()->setFullPath(arguments[logPath]);
	}

	// 표준 셸 명령, DDE, 파일 열기에 대한 명령줄을 구문 분석합니다.
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (isValidParams)
	{
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
		CConverterManager::getConverterManager()->setIsCliMode(true);
		if (arguments.find(inputFolder) != arguments.end())
		{
			CConverterManager::getConverterManager()->setConversionOn(true);
			CConverterManager::getConverterManager()->setInputFolder(arguments[inputFolder]);
			CConverterManager::getConverterManager()->setOutputFolder(arguments[outputFolder]);

			if (arguments.find(occlusionCulling) != arguments.end())
			{
				if (arguments[occlusionCulling] == std::wstring(L"Y") ||
					arguments[occlusionCulling] == std::wstring(L"y"))
					CConverterManager::getConverterManager()->setOcclusionCulling(true);
				else
					CConverterManager::getConverterManager()->setOcclusionCulling(false);
			}
			else
				CConverterManager::getConverterManager()->setOcclusionCulling(false);

			if (arguments.find(unitScaleFactor) != arguments.end())
			{
				CConverterManager::getConverterManager()->setUnitScaleFactor(std::stod(arguments[unitScaleFactor]));
			}
		}
		else
			CConverterManager::getConverterManager()->setConversionOn(false);

		if (arguments.find(indexCreation) != arguments.end())
		{
			if (arguments[indexCreation] == std::wstring(L"Y") ||
				arguments[indexCreation] == std::wstring(L"y"))
			{
				CConverterManager::getConverterManager()->setIndexCreation(true);
				CConverterManager::getConverterManager()->setOutputFolder(arguments[outputFolder]);
			}
			else
			{
				CConverterManager::getConverterManager()->setIndexCreation(false);
			}
		}
		else
			CConverterManager::getConverterManager()->setIndexCreation(false);

		if (arguments.find(idPrefix) != arguments.end())
		{
			CConverterManager::getConverterManager()->setIdPrefix(arguments[idPrefix]);
		}

		if (arguments.find(idSuffix) != arguments.end())
		{
			CConverterManager::getConverterManager()->setIdSuffix(arguments[idSuffix]);
		}
	}
	else
		CConverterManager::getConverterManager()->setIsCliMode(false);


	// 명령줄에 지정된 명령을 디스패치합니다.
	// 응용 프로그램이 /RegServer, /Register, /Unregserver 또는 /Unregister로 시작된 경우 FALSE를 반환합니다.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	if (m_pMainWnd == NULL && isValidParams)
		this->OnCmdMsg(ID_FILE_NEW, 0, NULL, NULL);

	if (m_pMainWnd == NULL)
	{
		if (LogWriter::getLogWriter()->isStarted())
		{
			LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::wstring(CANNOT_INITIALIZE), true);
			LogWriter::getLogWriter()->finish();
			LogWriter::getLogWriter()->save();
		}
		return FALSE;
	}

	// 창 하나만 초기화되었으므로 이를 표시하고 업데이트합니다.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

// CF4DConverterApp 메시지 처리기


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// 대화 상자를 실행하기 위한 응용 프로그램 명령입니다.
void CF4DConverterApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CF4DConverterApp 메시지 처리기



