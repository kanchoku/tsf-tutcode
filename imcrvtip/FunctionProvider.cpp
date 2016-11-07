﻿
#include "imcrvtip.h"
#include "TextService.h"
#include "FnCandidateList.h"

HRESULT CTextService::GetType(GUID *pguid)
{
	if(pguid == nullptr)
	{
		return E_INVALIDARG;
	}

	*pguid = c_clsidTextService;

	return S_OK;
}

HRESULT CTextService::GetDescription(BSTR *pbstrDesc)
{
	BSTR bstrDesc;

	if(pbstrDesc == nullptr)
	{
		return E_INVALIDARG;
	}

	*pbstrDesc = nullptr;

	bstrDesc = SysAllocString(TextServiceDesc);

	if(bstrDesc == nullptr)
	{
		return E_OUTOFMEMORY;
	}

	*pbstrDesc = bstrDesc;

	return S_OK;
}

HRESULT CTextService::GetFunction(REFGUID rguid, REFIID riid, IUnknown **ppunk)
{
	if(ppunk == nullptr)
	{
		return E_INVALIDARG;
	}

	*ppunk = nullptr;

	//This value can be GUID_NULL.
	if(!IsEqualGUID(rguid, GUID_NULL) && !IsEqualGUID(rguid, c_clsidTextService))
	{
		return E_INVALIDARG;
	}

	if(IsEqualIID(riid, IID_ITfFnConfigure))
	{
		*ppunk = (ITfFnConfigure *)this;
	}
	else if(IsEqualIID(riid, IID_ITfFnShowHelp))
	{
		*ppunk = (ITfFnShowHelp *)this;
	}
	else if(IsEqualIID(riid, IID_ITfFnReconversion))
	{
		*ppunk = (ITfFnReconversion *)this;
	}
	else if(IsEqualIID(riid, IID_ITfFnGetPreferredTouchKeyboardLayout))
	{
		*ppunk = (ITfFnGetPreferredTouchKeyboardLayout *)this;
	}

	if(*ppunk)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

HRESULT CTextService::GetDisplayName(BSTR *pbstrName)
{
	BSTR bstrName;

	if(pbstrName == nullptr)
	{
		return E_INVALIDARG;
	}

	*pbstrName = nullptr;

	bstrName = SysAllocString(TextServiceDesc);

	if(bstrName == nullptr)
	{
		return E_OUTOFMEMORY;
	}

	*pbstrName = bstrName;

	return S_OK;
}

HRESULT CTextService::Show(HWND hwndParent, LANGID langid, REFGUID rguidProfile)
{
	if(!IsEqualGUID(rguidProfile, c_guidProfile))
	{
		return E_INVALIDARG;
	}

	_StartConfigure();

	return S_OK;
}

HRESULT CTextService::Show(HWND hwndParent)
{
	_StartConfigure();

	return S_OK;
}

HRESULT CTextService::QueryRange(ITfRange *pRange, ITfRange **ppNewRange, BOOL *pfConvertable)
{
	if(pRange == nullptr || pfConvertable == nullptr)
	{
		return E_INVALIDARG;
	}

	if(ppNewRange == nullptr)
	{
		*pfConvertable = TRUE;
		return S_OK;
	}

	*ppNewRange = nullptr;
	*pfConvertable = FALSE;

	HRESULT hr = pRange->Clone(ppNewRange);

	if(hr == S_OK)
	{
		*pfConvertable = TRUE;
	}

	return hr;
}

HRESULT CTextService::GetReconversion(ITfRange *pRange, ITfCandidateList **ppCandList)
{
	if(pRange == nullptr || ppCandList == nullptr)
	{
		return E_INVALIDARG;
	}

	*ppCandList = nullptr;

	HRESULT hr = E_FAIL;

	std::wstring text;
	if((_GetRangeText(pRange, text) == S_OK) && !text.empty())
	{
		std::wstring key;
		_ConvKanaToKana(text, im_katakana, key, im_hiragana);

		CTextService *pTextService = nullptr;

		try
		{
			pTextService = new CTextService();
			pTextService->AddRef();

			pTextService->_ResetStatus();
			pTextService->_CreateConfigPath();
			pTextService->inputmode = im_hiragana;
			pTextService->kana = key;

			pTextService->_StartSubConv(REQ_SEARCH);

			*ppCandList = new CFnCandidateList(this, key, pTextService->candidates);

			hr = S_OK;
		}
		catch(...)
		{
			return E_OUTOFMEMORY;
		}

		SafeRelease(&pTextService);
	}

	return hr;
}

HRESULT CTextService::Reconvert(ITfRange *pRange)
{
	if(pRange == nullptr)
	{
		return E_INVALIDARG;
	}

	if(_IsComposing())
	{
		return S_OK;
	}

	HRESULT hr = E_FAIL;

	std::wstring text;
	if((_GetRangeText(pRange, text) == S_OK) && !text.empty())
	{
		ITfDocumentMgr *pDocumentMgr;
		if((_pThreadMgr->GetFocus(&pDocumentMgr) == S_OK) && (pDocumentMgr != nullptr))
		{
			ITfContext *pContext;
			if((pDocumentMgr->GetTop(&pContext) == S_OK) && (pContext != nullptr))
			{
				reconversion = TRUE;
				reconvsrc = text;

				if(!_IsKeyboardOpen())
				{
					inputmode = im_disable;
					_SetKeyboardOpen(TRUE);
				}

				inputmode = im_hiragana;
				inputkey = TRUE;
				_ConvKanaToKana(text, im_katakana, kana, im_hiragana);

				hr = _InvokeKeyHandler(pContext, 0, 0, SKK_NEXT_CAND);

				SafeRelease(&pContext);
			}
			SafeRelease(&pDocumentMgr);
		}
	}

	return hr;
}

HRESULT CTextService::GetLayout(TKBLayoutType *pTKBLayoutType, WORD *pwPreferredLayoutId)
{
	if(pTKBLayoutType == nullptr || pwPreferredLayoutId == nullptr)
	{
		return E_INVALIDARG;
	}

	*pTKBLayoutType = TKBLT_OPTIMIZED;
	*pwPreferredLayoutId = TKBL_OPT_JAPANESE_ABC;

	return S_OK;
}


class CGetRangeTextEditSession : public CEditSessionBase
{
public:
	CGetRangeTextEditSession(CTextService *pTextService, ITfContext *pContext, ITfRange *pRange) : CEditSessionBase(pTextService, pContext)
	{
		_pRange = pRange;
		_pRange->AddRef();
	}

	~CGetRangeTextEditSession()
	{
		SafeRelease(&_pRange);
	}

	// ITfEditSession
	STDMETHODIMP DoEditSession(TfEditCookie ec)
	{
		HRESULT hr;
		WCHAR buf[16];
		ULONG cch = _countof(buf) - 1;

		_Text.clear();

		while(cch == _countof(buf) - 1)
		{
			ZeroMemory(buf, sizeof(buf));
			cch = _countof(buf) - 1;
			hr = _pRange->GetText(ec, TF_TF_MOVESTART, buf, cch, &cch);
			if(hr == S_OK)
			{
				_Text.append(buf);
			}
			else
			{
				_Text.clear();
				return hr;
			}
		}

		return S_OK;
	}

	std::wstring _GetText()
	{
		return _Text;
	}

private:
	ITfRange *_pRange;
	std::wstring _Text;
};

HRESULT CTextService::_GetRangeText(ITfRange *pRange, std::wstring &text)
{
	HRESULT hr = E_FAIL;

	ITfContext *pContext;
	if(pRange->GetContext(&pContext) == S_OK)
	{
		try
		{
			CGetRangeTextEditSession *pEditSession = new CGetRangeTextEditSession(this, pContext, pRange);
			pContext->RequestEditSession(_ClientId, pEditSession, TF_ES_SYNC | TF_ES_READ, &hr);
			if(hr == S_OK)
			{
				text = pEditSession->_GetText();
			}
			SafeRelease(&pEditSession);
		}
		catch(...)
		{
			return E_OUTOFMEMORY;
		}

		SafeRelease(&pContext);
	}

	return hr;
}

HRESULT CTextService::_SetReconvertResult(const std::wstring &fnsearchkey, const CANDIDATES &fncandidates, UINT index)
{
	HRESULT hr = S_OK;

	if(index >= (ULONG)fncandidates.size())
	{
		return E_FAIL;
	}

	ITfDocumentMgr *pDocumentMgr;
	if((_pThreadMgr->GetFocus(&pDocumentMgr) == S_OK) && (pDocumentMgr != nullptr))
	{
		ITfContext *pContext;
		if((pDocumentMgr->GetTop(&pContext) == S_OK) && (pContext != nullptr))
		{
			inputkey = TRUE;
			searchkey = fnsearchkey;
			searchkeyorg = fnsearchkey;
			showentry = TRUE;
			candidates = fncandidates;
			candidx = (size_t)index;
			if(candidx >= cx_untilcandlist - 1)
			{
				showcandlist = TRUE;
			}

			try
			{
				CSetResultEditSession *pEditSession = new CSetResultEditSession(this, pContext);
				pContext->RequestEditSession(_ClientId, pEditSession, TF_ES_SYNC | TF_ES_READWRITE, &hr);
				SafeRelease(&pEditSession);
			}
			catch(...)
			{
				hr = E_FAIL;
			}

			SafeRelease(&pContext);
		}
		SafeRelease(&pDocumentMgr);
	}

	return hr;
}

BOOL CTextService::_InitFunctionProvider()
{
	HRESULT hr = E_FAIL;

	ITfSourceSingle *pSourceSingle;
	if(_pThreadMgr->QueryInterface(IID_PPV_ARGS(&pSourceSingle)) == S_OK)
	{
		hr = pSourceSingle->AdviseSingleSink(_ClientId, IID_IUNK_ARGS((ITfFunctionProvider *)this));
		SafeRelease(&pSourceSingle);
	}

	return (hr == S_OK);
}

void CTextService::_UninitFunctionProvider()
{
	ITfSourceSingle *pSourceSingle;
	if(_pThreadMgr->QueryInterface(IID_PPV_ARGS(&pSourceSingle)) == S_OK)
	{
		pSourceSingle->UnadviseSingleSink(_ClientId, IID_ITfFunctionProvider);
		SafeRelease(&pSourceSingle);
	}
}
