﻿
#include "imcrvtip.h"
#include "TextService.h"

// Register
BOOL RegisterProfiles();
void UnregisterProfiles();
BOOL RegisterCategories();
void UnregisterCategories();
BOOL RegisterServer();
void UnregisterServer();

static LONG g_cRefDll = 0;

class CClassFactory : public IClassFactory
{
public:
	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// IClassFactory
	STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
	STDMETHODIMP LockServer(BOOL fLock);
};

STDAPI CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
	if(ppvObj == NULL)
	{
		return E_INVALIDARG;
	}

	*ppvObj = NULL;

	if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
	{
		*ppvObj = this;
		DllAddRef();
	}
	else
	{
		return E_NOINTERFACE;
	}

	return S_OK;
}

STDAPI_(ULONG) CClassFactory::AddRef()
{
	return DllAddRef();
}

STDAPI_(ULONG) CClassFactory::Release()
{
	return DllRelease();
}

STDAPI CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
	CTextService *pTextService;
	HRESULT hr;

	if(ppvObj == NULL)
	{
		return E_INVALIDARG;
	}

	*ppvObj = NULL;

	if(NULL != pUnkOuter)
	{
		return CLASS_E_NOAGGREGATION;
	}

	pTextService = new CTextService();

	if(pTextService == NULL)
	{
		return E_OUTOFMEMORY;
	}

	hr = pTextService->QueryInterface(riid, ppvObj);

	pTextService->Release();

	return hr;
}

STDAPI CClassFactory::LockServer(BOOL fLock)
{
	if(fLock)
	{
		DllAddRef();
	}
	else
	{
		DllRelease();
	}

	return S_OK;
}

STDAPI DllCanUnloadNow(void)
{
	return g_cRefDll <= 0 ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvObj)
{
	static CClassFactory factory;

	if(ppvObj == NULL)
	{
		return E_INVALIDARG;
	}

	*ppvObj = NULL;

	if((IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) &&
		IsEqualGUID(rclsid, c_clsidTextService))
	{
		*ppvObj = &factory;
		DllAddRef();
		return S_OK;
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllRegisterServer(void)
{
	if(!RegisterServer() || !RegisterCategories() || !RegisterProfiles())
	{
		DllUnregisterServer();
		return E_FAIL;
	}

	return S_OK;
}

STDAPI DllUnregisterServer(void)
{
	UnregisterProfiles();
	UnregisterCategories();
	UnregisterServer();

	return S_OK;
}

LONG DllAddRef(void)
{
	return InterlockedIncrement(&g_cRefDll);
}

LONG DllRelease(void)
{
	return InterlockedDecrement(&g_cRefDll);
}