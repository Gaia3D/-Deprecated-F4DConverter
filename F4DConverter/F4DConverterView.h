
// F4DConverterView.h : CF4DConverterView 클래스의 인터페이스
//

#pragma once


class CF4DConverterView : public CView
{
protected: // serialization에서만 만들어집니다.
	CF4DConverterView();
	DECLARE_DYNCREATE(CF4DConverterView)

// 특성입니다.
public:
	CF4DConverterDoc* GetDocument() const;

// 작업입니다.
public:
	void activateConverter();

// 재정의입니다.
public:
	virtual void OnDraw(CDC* pDC);  // 이 뷰를 그리기 위해 재정의되었습니다.
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

// 구현입니다.
public:
	virtual ~CF4DConverterView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 생성된 메시지 맵 함수
protected:
	DECLARE_MESSAGE_MAP()
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
public:
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

#ifndef _DEBUG  // F4DConverterView.cpp의 디버그 버전
inline CF4DConverterDoc* CF4DConverterView::GetDocument() const
   { return reinterpret_cast<CF4DConverterDoc*>(m_pDocument); }
#endif

