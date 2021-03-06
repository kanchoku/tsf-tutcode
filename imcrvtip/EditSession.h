﻿#pragma once

#include "imcrvtip.h"
#include "TextService.h"

class CEditSessionBase : public ITfEditSession
{
public:
	CEditSessionBase(CTextService *pTextService, ITfContext *pContext)
	{
		_cRef = 1;

		_pContext = pContext;
		_pTextService = pTextService;
	}

	virtual ~CEditSessionBase()
	{
		_pContext.Release();
		_pTextService.Release();
	}

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj)
	{
		if (ppvObj == nullptr)
		{
			return E_INVALIDARG;
		}

		*ppvObj = nullptr;

		if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession))
		{
			*ppvObj = static_cast<ITfEditSession *>(this);
		}

		if (*ppvObj)
		{
			AddRef();
			return S_OK;
		}

		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef(void)
	{
		return ++_cRef;
	}

	STDMETHODIMP_(ULONG) Release(void)
	{
		if (--_cRef == 0)
		{
			delete this;
			return 0;
		}

		return _cRef;
	}

	// ITfEditSession
	virtual STDMETHODIMP DoEditSession(TfEditCookie ec) = 0;

protected:
	CComPtr<ITfContext> _pContext;
	CComPtr<CTextService> _pTextService;

private:
	LONG _cRef;
};
