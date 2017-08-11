
// Mago3dConverterView.cpp : CF4DConverterView 클래스의 구현
//

#include "stdafx.h"
// SHARED_HANDLERS는 미리 보기, 축소판 그림 및 검색 필터 처리기를 구현하는 ATL 프로젝트에서 정의할 수 있으며
// 해당 프로젝트와 문서 코드를 공유하도록 해 줍니다.
#ifndef SHARED_HANDLERS
#include "F4DConverter.h"
#endif

#include "F4DConverterDoc.h"
#include "F4DConverterView.h"

#include "./converter/predefinition.h"
#include "./converter/LogWriter.h"
#include "converter/ConverterManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HDC hDC = NULL;

// CF4DConverterView

IMPLEMENT_DYNCREATE(CF4DConverterView, CView)

BEGIN_MESSAGE_MAP(CF4DConverterView, CView)
	ON_WM_DESTROY()
	ON_WM_SIZE()
END_MESSAGE_MAP()

// CF4DConverterView 생성/소멸

CF4DConverterView::CF4DConverterView()
{
	// TODO: 여기에 생성 코드를 추가합니다.

}

CF4DConverterView::~CF4DConverterView()
{
}

BOOL CF4DConverterView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: CREATESTRUCT cs를 수정하여 여기에서
	//  Window 클래스 또는 스타일을 수정합니다.

	return CView::PreCreateWindow(cs);
}

// CF4DConverterView 그리기

void CF4DConverterView::OnDraw(CDC* /*pDC*/)
{
	CF4DConverterDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: 여기에 원시 데이터에 대한 그리기 코드를 추가합니다.
}


// CF4DConverterView 진단

#ifdef _DEBUG
void CF4DConverterView::AssertValid() const
{
	CView::AssertValid();
}

void CF4DConverterView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CF4DConverterDoc* CF4DConverterView::GetDocument() const // 디버그되지 않은 버전은 인라인으로 지정됩니다.
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CF4DConverterDoc)));
	return (CF4DConverterDoc*)m_pDocument;
}
#endif //_DEBUG

void CF4DConverterView::activateConverter()
{
	if (CConverterManager::getConverterManager()->getIsCliMode())
	{
		if (CConverterManager::getConverterManager()->getConversionOn())
		{
			if (!CConverterManager::getConverterManager()->isInitialized())
			{
				hDC = ::GetDC(this->m_hWnd);
				CRect rect;
				this->GetClientRect(rect);
				if (!CConverterManager::getConverterManager()->initialize(hDC, rect.Width(), rect.Height()))
				{
					LogWriter::getLogWriter()->finish();
					LogWriter::getLogWriter()->save();
					return;
				}

				CConverterManager::getConverterManager()->processDataFolder();

				LogWriter::getLogWriter()->finish();
				LogWriter::getLogWriter()->save();
			}
		}

		if (CConverterManager::getConverterManager()->getIndexCreation())
			CConverterManager::getConverterManager()->writeIndexFile();

		::AfxGetMainWnd()->PostMessage(WM_CLOSE);
	}
	else
	{
		CRect rect;
		this->GetClientRect(rect);
		if (CConverterManager::getConverterManager()->isInitialized())
		{
			CConverterManager::getConverterManager()->changeGLDimension(rect.Width(), rect.Height());
		}
		else
		{
			hDC = ::GetDC(this->m_hWnd);
			CConverterManager::getConverterManager()->initialize(hDC, rect.Width(), rect.Height());
		}
	}
}


// CF4DConverterView 메시지 처리기


void CF4DConverterView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);

	if (bActivate == TRUE)
		activateConverter();
}


void CF4DConverterView::OnDestroy()
{
	CView::OnDestroy();

	if (hDC != NULL)
		::ReleaseDC(this->m_hWnd, hDC);
}


void CF4DConverterView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if (cx == 0 || cy == 0)
		return;

	if (CConverterManager::getConverterManager()->getIsCliMode())
		return;

	if (CConverterManager::getConverterManager()->isInitialized())
		CConverterManager::getConverterManager()->changeGLDimension(cx, cy);
}
