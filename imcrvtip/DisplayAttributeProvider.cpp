﻿
#include "imcrvtip.h"
#include "TextService.h"
#include "DisplayAttributeInfo.h"
#include "EnumDisplayAttributeInfo.h"

STDAPI CTextService::EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo **ppEnum)
{
	IEnumTfDisplayAttributeInfo *pEnumDisplayAttributeInfo = nullptr;

	if (ppEnum == nullptr)
	{
		return E_INVALIDARG;
	}

	*ppEnum = nullptr;

	try
	{
		pEnumDisplayAttributeInfo = new CEnumDisplayAttributeInfo();
	}
	catch (...)
	{
		return E_OUTOFMEMORY;
	}

	*ppEnum = pEnumDisplayAttributeInfo;

	return S_OK;
}

STDAPI CTextService::GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo **ppInfo)
{
	ITfDisplayAttributeInfo *pDisplayAttributeInfo = nullptr;

	if (ppInfo == nullptr)
	{
		return E_INVALIDARG;
	}

	*ppInfo = nullptr;

	for (int i = 0; i < _countof(c_gdDisplayAttributeInfo); i++)
	{
		if (IsEqualGUID(guid, c_gdDisplayAttributeInfo[i].guid))
		{
			try
			{
				pDisplayAttributeInfo = new CDisplayAttributeInfo(c_gdDisplayAttributeInfo[i].guid, &display_attribute_info[i]);
			}
			catch (...)
			{
				return E_OUTOFMEMORY;
			}
			break;
		}
	}

	if (pDisplayAttributeInfo == nullptr)
	{
		return E_INVALIDARG;
	}

	*ppInfo = pDisplayAttributeInfo;

	return S_OK;
}

void CTextService::_ClearCompositionDisplayAttributes(TfEditCookie ec, ITfContext *pContext)
{
	if (_IsComposing())
	{
		CComPtr<ITfRange> pRange;
		if (SUCCEEDED(_pComposition->GetRange(&pRange)) && (pRange != nullptr))
		{
			CComPtr<ITfProperty> pProperty;
			if (SUCCEEDED(pContext->GetProperty(GUID_PROP_ATTRIBUTE, &pProperty)) && (pProperty != nullptr))
			{
				pProperty->Clear(ec, pRange);
			}
		}
	}
}

BOOL CTextService::_SetCompositionDisplayAttributes(TfEditCookie ec, ITfContext *pContext, ITfRange *pRange, TfGuidAtom gaDisplayAttribute)
{
	HRESULT hr = E_FAIL;

	CComPtr<ITfProperty> pProperty;
	if (SUCCEEDED(pContext->GetProperty(GUID_PROP_ATTRIBUTE, &pProperty)) && (pProperty != nullptr))
	{
		CComVariant var;
		V_VT(&var) = VT_I4;
		V_I4(&var) = gaDisplayAttribute;
		hr = pProperty->SetValue(ec, pRange, &var);
	}

	return SUCCEEDED(hr);
}

BOOL CTextService::_InitDisplayAttributeGuidAtom()
{
	HRESULT hr = E_FAIL;
	const struct {
		const GUID guid;
		TfGuidAtom *patom;
	} displayAttributeAtom[] = {
		{c_guidDisplayAttributeInputMark,	&_gaDisplayAttributeInputMark},
		{c_guidDisplayAttributeInputText,	&_gaDisplayAttributeInputText},
		{c_guidDisplayAttributeInputOkuri,	&_gaDisplayAttributeInputOkuri},
		{c_guidDisplayAttributeConvMark,	&_gaDisplayAttributeConvMark},
		{c_guidDisplayAttributeConvText,	&_gaDisplayAttributeConvText},
		{c_guidDisplayAttributeConvOkuri,	&_gaDisplayAttributeConvOkuri},
		{c_guidDisplayAttributeConvAnnot,	&_gaDisplayAttributeConvAnnot}
	};

	CComPtr<ITfCategoryMgr> pCategoryMgr;
	hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pCategoryMgr));
	if (SUCCEEDED(hr) && (pCategoryMgr != nullptr))
	{
		for (int i = 0; i < _countof(displayAttributeAtom); i++)
		{
			hr = pCategoryMgr->RegisterGUID(displayAttributeAtom[i].guid, displayAttributeAtom[i].patom);
		}
	}

	return SUCCEEDED(hr);
}

BOOL CTextService::display_attribute_series[DISPLAYATTRIBUTE_INFO_NUM] =
{
	c_gdDisplayAttributeInfo[0].se,
	c_gdDisplayAttributeInfo[1].se,
	c_gdDisplayAttributeInfo[2].se,
	c_gdDisplayAttributeInfo[3].se,
	c_gdDisplayAttributeInfo[4].se,
	c_gdDisplayAttributeInfo[5].se,
	c_gdDisplayAttributeInfo[6].se
};

TF_DISPLAYATTRIBUTE CTextService::display_attribute_info[DISPLAYATTRIBUTE_INFO_NUM] =
{
	c_gdDisplayAttributeInfo[0].da,
	c_gdDisplayAttributeInfo[1].da,
	c_gdDisplayAttributeInfo[2].da,
	c_gdDisplayAttributeInfo[3].da,
	c_gdDisplayAttributeInfo[4].da,
	c_gdDisplayAttributeInfo[5].da,
	c_gdDisplayAttributeInfo[6].da
};
