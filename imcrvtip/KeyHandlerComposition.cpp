﻿
#include "imcrvtip.h"
#include "TextService.h"
#include "CandidateList.h"
#include "moji.h"

HRESULT CTextService::_Update(TfEditCookie ec, ITfContext *pContext, BOOL fixed, BOOL back)
{
	std::wstring comptext;
	HRESULT ret = _Update(ec, pContext, comptext, fixed, back);
	_RedrawVKeyboardWindow();
	return ret;
}

HRESULT CTextService::_Update(TfEditCookie ec, ITfContext *pContext, std::wstring &comptext, BOOL fixed, BOOL back)
{
	WCHAR candidatecount[16];
	WCHAR useraddmode = REQ_USER_ADD_1;
	LONG cchCursor = 0;
	LONG cchOkuri = 0;
	BOOL showmodemark = cx_showmodemark;

	if(pContext == nullptr)	//辞書登録用
	{
		showmodemark = TRUE;
	}

	if(showentry &&
		(	(fixed && showcandlist) ||
			(cx_untilcandlist == 0) ||
			(candidx + 1 < cx_untilcandlist) ||
			(candidates.size() + 1 == cx_untilcandlist)	))
	{
		if(!candidates.empty() && candidx < candidates.size())
		{
			if(!fixed && showmodemark)
			{
				comptext.append(markHenkan);
			}

			comptext.append(candidates[candidx].first.first);

			if(okuriidx != 0)
			{
				cchOkuri = (LONG)comptext.size();
				comptext.append(kana.substr(okuriidx + 1));
				useraddmode = REQ_USER_ADD_0;
			}
			//活用する語の語尾
			if(_IsYomiInflection() && postyomied < postyomi.size() - 1) //-1:'―'
			{
				cchOkuri = (LONG)comptext.size();
				comptext.append(postyomi.substr(postyomied, postyomi.size() - 1 - postyomied));
			}

			cchCursor = (LONG)comptext.size();

			if(!fixed)
			{
				if(cx_annotation && !cx_annotatlst && !candidates[candidx].first.second.empty())
				{
					comptext.append(markAnnotation + candidates[candidx].first.second);
				}

				if(cx_untilcandlist == 0 && cx_dispcandnum)
				{
					comptext.append(L" (");
					_snwprintf_s(candidatecount, _TRUNCATE, L"%u", (UINT)candidx + 1);
					comptext.append(candidatecount);
					comptext.append(L"/");
					_snwprintf_s(candidatecount, _TRUNCATE, L"%u", (UINT)candidates.size());
					comptext.append(candidatecount);
					comptext.append(L")");
				}

				if(!showmodemark && comptext.empty())
				{
					comptext.append(markSP);
				}
			}

			//ユーザー辞書登録
			if(fixed && !candidates[candidx].second.first.empty())
			{
				_AddUserDic(useraddmode, ((candorgcnt <= candidx) ? searchkey : searchkeyorg),
					candidates[candidx].second.first, candidates[candidx].second.second);
				_ShowAutoHelp(candidates[candidx].second.first, searchkeyorg);
			}
		}
		else
		{
			//候補なし or 候補が尽きた
			if(!fixed)
			{
				if(!showmodemark)
				{
					if(kana.empty())
					{
						comptext.append(markSP);
					}
				}
				else
				{
					comptext.append(markHenkan);
				}
			}

			if(okuriidx == 0)
			{
				comptext.append(kana);
			}
			else
			{
				comptext.append(kana.substr(0, okuriidx));
				cchOkuri = (LONG)comptext.size();
				if(!fixed && showmodemark)
				{
					comptext.append(markOkuri);
				}
				comptext.append(kana.substr(okuriidx + 1));
			}

			cchCursor = (LONG)comptext.size();

			//読みを縮めながらの後置型交ぜ書き変換。変換可能な候補無し
			if(postyomiResizing == PYR_SHRINKING)
			{
				std::wstring yomi;
				if(_ShrinkPostMaze(&yomi) == S_OK)
				{
					_StartConvWithYomi(ec, pContext, yomi);
					return S_OK;
				}
			}
			else if(postyomiResizing == PYR_EXTENDING && postyomist > 0)
			{
				//読みを伸ばして交ぜ書き変換
				size_t st = BackwardMoji(postyomi, postyomist, 1);
				if(st < postyomist)
				{
					postyomist = st;
					std::wstring yomi(postyomi.substr(st));
					_StartConvWithYomi(ec, pContext, yomi);
					return S_OK;
				}
			}

			if(pContext == nullptr && _pCandidateList != nullptr)	//辞書登録用
			{
				_pCandidateList->_SetText(comptext, FALSE, wm_register);
				return S_OK;
			}
			else
			{
				_SetText(ec, pContext, comptext, cchCursor, cchOkuri, FALSE);
				//辞書登録表示開始
				return _ShowCandidateList(ec, pContext, wm_register);
			}
		}
	}
	else
	{
		if(inputkey)
		{
			if(!fixed)
			{
				if(!showmodemark)
				{
					if(kana.empty() && roman.empty())
					{
						comptext.append(markSP);
					}
				}
				else
				{
					if(showentry && (candidx + 1 == cx_untilcandlist))
					{
						comptext.append(markHenkan);
					}
					else
					{
						comptext.append(markMidashi);
					}
				}
			}

			if(!roman.empty() || !kana.empty())
			{
				if(okuriidx == 0)
				{
					comptext.append(kana);
					if(pContext == nullptr && !fixed && cursoridx != kana.size())	//辞書登録用
					{
						comptext.insert(cursoridx + (comptext.size() - kana.size()), markCursor);
					}
				}
				else
				{
					comptext.append(kana.substr(0, okuriidx));
					cchOkuri = (LONG)comptext.size();
					if(!fixed && showmodemark && !complement)
					{
						comptext.append(markOkuri);
					}
					if(okuriidx + 1 < kana.size())
					{
						comptext.append(kana.substr(okuriidx + 1));
					}
					if(pContext == nullptr && !fixed && roman.empty() && cursoridx != kana.size())	//辞書登録用
					{
						if(!showmodemark)
						{
							if(cursoridx < okuriidx)
							{
								comptext.insert(cursoridx, markCursor);
							}
							else
							{
								comptext.insert(cursoridx - 1, markCursor);
							}
						}
						else
						{
							if(complement && okuriidx != 0 && (okuriidx + 1 != kana.size()))
							{
								comptext.insert(okuriidx + 1, L" [");
								comptext.append(L"]");
							}
							comptext.insert(cursoridx + 1, markCursor);
						}
					}
				}
				if(!fixed && !roman.empty())
				{
					if(!showmodemark)
					{
						if(okuriidx != 0 && okuriidx < cursoridx)
						{
							if(cx_showroman)
							{
								comptext.insert(cursoridx - 1, roman);
							}
							else
							{
								comptext.insert(cursoridx - 1, markSP);
							}
						}
						else
						{
							if(cx_showroman)
							{
								comptext.insert(cursoridx, roman);
							}
							else
							{
								comptext.insert(cursoridx, markSP);
							}
						}
					}
					else
					{
						if(cx_showroman)
						{
							comptext.insert(cursoridx + 1, roman);
						}
						else
						{
							comptext.insert(cursoridx + 1, markSP);
						}
					}
					if(okuriidx != 0 && cursoridx <= okuriidx)
					{
						if(cx_showroman)
						{
							cchOkuri += (LONG)roman.size();
						}
						else
						{
							cchOkuri += 1;
						}
					}
				}
			}

			if(showentry && (candidx + 1 == cx_untilcandlist))
			{
				cchCursor = (LONG)comptext.size();
			}
		}
		else
		{
			if(!kana.empty())
			{
				comptext.append(kana);
			}
			else
			{
				if(!fixed)
				{
					if(cx_showroman)
					{
						comptext.append(roman);
					}
					else
					{
						comptext.append(markSP);
					}
				}
			}
		}
	}

	if(fixed && back && cx_backincenter && !comptext.empty())
	{
		// surrogate pair
		if(comptext.size() >= 2 &&
			IS_SURROGATE_PAIR(comptext[comptext.size() - 2], comptext[comptext.size() - 1]))
		{
			comptext.pop_back();
			comptext.pop_back();
		}
		else
		{
			comptext.pop_back();
		}
	}

	_EndInputModeWindow();
	postyomiResizing = PYR_NO;

	if(inputkey && !fixed && !showcandlist && showentry &&
		(((cx_untilcandlist != 1) && (candidx + 1 == cx_untilcandlist)) || (cx_untilcandlist == 1)) &&
		(candidates.size() + 1 != cx_untilcandlist))
	{
		if(pContext == nullptr && _pCandidateList != nullptr)	//辞書登録用
		{
			showcandlist = TRUE;
			candidx = 0;
			_pCandidateList->_SetText(comptext, FALSE, wm_candidate);
			return S_OK;
		}
		else
		{
			_SetText(ec, pContext, comptext, cchCursor, cchOkuri, fixed);
			//候補一覧表示開始
			showcandlist = TRUE;
			candidx = 0;
			return _ShowCandidateList(ec, pContext, wm_candidate);
		}
	}

	if(pContext == nullptr && _pCandidateList != nullptr)	//辞書登録用
	{
		//文字数指定無し後置型交ぜ書き変換で読みを縮め/伸ばした場合
		if(postyomist > 0 && fixed)
		{
			//読みから外した部分は確定(fixed=TRUEで_SetText())
			_pCandidateList->_SetText(postyomi.substr(0, postyomist), TRUE, wm_none);
			postyomi.erase(0, postyomist);
			postyomist = 0;
			postyomied = postyomi.size();
		}
		//読み伸ばし/縮め途中で、読みから外した部分は表示用の値を更新する
		_pCandidateList->_SetTextExcludedPostyomi(postyomi.substr(0, postyomist));
		_pCandidateList->_SetText(comptext, fixed, wm_none);
		return S_OK;
	}
	else
	{
		return _SetText(ec, pContext, comptext, cchCursor, cchOkuri, fixed);
	}
}

HRESULT CTextService::_SetText(TfEditCookie ec, ITfContext *pContext, const std::wstring &comptext, LONG cchCursor, LONG cchOkuri, BOOL fixed)
{
	TF_SELECTION tfSelection;
	ULONG cFetched = 0;
	LONG cch, cchRes;

	if(pContext == nullptr && _pCandidateList != nullptr)	//辞書登録用
	{
		_pCandidateList->_SetText(comptext, fixed, wm_none);
		return S_OK;
	}

	if(!_IsComposing())
	{
		if(!_StartComposition(pContext))
		{
			return S_FALSE;
		}
	}

	std::wstring text(comptext);
	//文字数指定無し後置型交ぜ書き変換で読みを縮め/伸ばした場合
	if(postyomist > 0)
	{	//外した部分が表示されるように、textに挿入
		text.insert(0, postyomi.substr(0, postyomist));
		cchCursor += (LONG)postyomist;
		//確定時の後始末(しないと以降の入力で常にpostyomiが付いてしまう)
		if(fixed)
		{
			postyomist = 0;
			postyomied = 0;
			postyomi.clear();
		}
	}

	if(pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &cFetched) != S_OK)
	{
		return S_FALSE;
	}

	if(cFetched != 1)
	{
		SafeRelease(&tfSelection.range);
		return S_FALSE;
	}

	ITfRange *pRange;
	if(_IsComposing() && _pComposition->GetRange(&pRange) == S_OK)
	{
		if(_IsRangeCovered(ec, tfSelection.range, pRange))
		{
			pRange->SetText(ec, 0, text.c_str(), (LONG)text.size());

			// shift from end to start.
			// shift over mathematical operators (U+2200-U+22FF) is rejected by OneNote.
			if(cchCursor == 0)
			{
				cchRes = (LONG)cursoridx - (LONG)kana.size();
				if((complement && okuriidx != 0) ||
					(!cx_showmodemark && okuriidx != 0 && cursoridx <= okuriidx && cursoridx < kana.size()))
				{
					cchRes += 1;
				}
			}
			else
			{
				cchRes = cchCursor - (LONG)text.size();
				if(cchRes > 0)
				{
					cchRes = 0;
				}
				else if(cchRes < -(LONG)text.size())
				{
					cchRes = -(LONG)text.size();
				}
			}

			tfSelection.range->ShiftEndToRange(ec, pRange, TF_ANCHOR_END);
			tfSelection.range->ShiftStartToRange(ec, pRange, TF_ANCHOR_END);
			tfSelection.range->ShiftStart(ec, cchRes, &cch, nullptr);
			//decide cursor position
			tfSelection.range->Collapse(ec, TF_ANCHOR_START);
			pContext->SetSelection(ec, 1, &tfSelection);

			//composition attribute
			if(!fixed)
			{
				ITfRange *pRangeClone;
				if(pRange->Clone(&pRangeClone) == S_OK)
				{
					pRangeClone->ShiftEndToRange(ec, pRange, TF_ANCHOR_END);
					pRangeClone->ShiftStartToRange(ec, pRange, TF_ANCHOR_START);

					if(cchCursor == 0 || !showentry)
					{
						if(inputkey)
						{
							_SetCompositionDisplayAttributes(ec, pContext, pRangeClone, _gaDisplayAttributeInputMark);
							if(cx_showmodemark)
							{
								pRangeClone->ShiftStart(ec, 1, &cch, nullptr);
							}
						}

						if(!display_attribute_series[1] || !inputkey)
						{
							_SetCompositionDisplayAttributes(ec, pContext, pRangeClone, _gaDisplayAttributeInputText);
						}

						if(cchOkuri != 0)
						{
							pRangeClone->ShiftStartToRange(ec, pRange, TF_ANCHOR_START);
							pRangeClone->ShiftStart(ec, cchOkuri, &cch, nullptr);
							if(!display_attribute_series[2])
							{
								_SetCompositionDisplayAttributes(ec, pContext, pRangeClone, _gaDisplayAttributeInputOkuri);
							}

							if(hintmode && text.find_first_of(CHAR_SKK_HINT) != std::wstring::npos)
							{
								LONG hintpos = (LONG)text.find_first_of(CHAR_SKK_HINT);
								if(cchOkuri < hintpos)
								{
									pRangeClone->ShiftStartToRange(ec, pRange, TF_ANCHOR_START);
									pRangeClone->ShiftStart(ec, hintpos, &cch, nullptr);
									_SetCompositionDisplayAttributes(ec, pContext, pRangeClone, _gaDisplayAttributeInputText);
								}
							}
						}
					}
					else
					{
						_SetCompositionDisplayAttributes(ec, pContext, pRangeClone, _gaDisplayAttributeConvMark);
						if(cx_showmodemark)
						{
							pRangeClone->ShiftStart(ec, 1, &cch, nullptr);
						}

						if(!display_attribute_series[4])
						{
							_SetCompositionDisplayAttributes(ec, pContext, pRangeClone, _gaDisplayAttributeConvText);
						}

						if(cchOkuri != 0)
						{
							pRangeClone->ShiftStartToRange(ec, pRange, TF_ANCHOR_START);
							pRangeClone->ShiftStart(ec, cchOkuri, &cch, nullptr);
							if(!display_attribute_series[5])
							{
								_SetCompositionDisplayAttributes(ec, pContext, pRangeClone, _gaDisplayAttributeConvOkuri);
							}
						}

						pRangeClone->ShiftEndToRange(ec, pRange, TF_ANCHOR_END);
						pRangeClone->ShiftStartToRange(ec, tfSelection.range, TF_ANCHOR_END);
						if(!display_attribute_series[6])
						{
							_SetCompositionDisplayAttributes(ec, pContext, pRangeClone, _gaDisplayAttributeConvAnnot);
						}
					}
					SafeRelease(&pRangeClone);
				}
			}

			// for Excel's PHONETIC function
			if(fixed && !text.empty())
			{
				ITfProperty *pProperty;
				if(pContext->GetProperty(GUID_PROP_READING, &pProperty) == S_OK)
				{
					VARIANT var;
					var.vt = VT_BSTR;
					std::wstring phone(kana);
					if(okuriidx == 0)
					{
						switch(inputmode)
						{
						case im_hiragana:
						case im_katakana:
						case im_katakana_ank:
							//接辞
							if(!abbrevmode && kana.size() >= 2)
							{
								if(kana.front() == L'>')
								{
									phone = kana.substr(1);
								}
								else if(kana.back() == L'>')
								{
									phone = kana.substr(0, kana.size() - 1);
								}
							}
							break;
						default:
							break;
						}
					}
					else
					{
						if(kana.size() > (okuriidx + 1))
						{
							phone = kana.substr(0, okuriidx) + kana.substr(okuriidx + 1);
						}
						else if(kana.size() >= okuriidx)
						{
							phone = kana.substr(0, okuriidx);
						}
					}
					if(!phone.empty())
					{
						var.bstrVal = SysAllocString(phone.c_str());
						pProperty->SetValue(ec, pRange, &var);
						SysFreeString(var.bstrVal);
					}
					SafeRelease(&pProperty);
				}
			}
		}

		SafeRelease(&pRange);
	}

	SafeRelease(&tfSelection.range);

	return S_OK;
}

HRESULT CTextService::_ShowCandidateList(TfEditCookie ec, ITfContext *pContext, int mode)
{
	HRESULT hr = E_FAIL;

	try
	{
		if(_pCandidateList == nullptr)
		{
			_pCandidateList = new CCandidateList(this);
		}

		ITfDocumentMgr *pDocumentMgr;
		if((pContext->GetDocumentMgr(&pDocumentMgr) == S_OK) && (pDocumentMgr != nullptr))
		{
			ITfRange *pRange;
			if(_IsComposing() && _pComposition->GetRange(&pRange) == S_OK)
			{
				hr = _pCandidateList->_StartCandidateList(_ClientId, pDocumentMgr, pContext, ec, pRange, mode);
				SafeRelease(&pRange);
			}
			SafeRelease(&pDocumentMgr);
		}

		if(hr != S_OK)
		{
			_ResetStatus();
			_CancelComposition(ec, pContext);
		}
	}
	catch(...)
	{
	}

	return hr;
}

void CTextService::_EndCandidateList()
{
	if(_pCandidateList != nullptr)
	{
		_pCandidateList->_EndCandidateList();
	}
	SafeRelease(&_pCandidateList);
}

void CTextService::_EndCompletionList(TfEditCookie ec, ITfContext *pContext)
{
	if(pContext != nullptr && !showcandlist)
	{
		_EndCandidateList();
	}
}

BOOL CTextService::_GetVertical(TfEditCookie ec, ITfContext *pContext)
{
	BOOL ret = FALSE;

	if(pContext != nullptr)
	{
		ITfRange *pRange;
		if(_IsComposing() && _pComposition->GetRange(&pRange) == S_OK)
		{
			ITfReadOnlyProperty *pReadOnlyProperty;
			if(pContext->GetAppProperty(TSATTRID_Text_VerticalWriting, &pReadOnlyProperty) == S_OK)
			{
				VARIANT var;
				if(pReadOnlyProperty->GetValue(ec, pRange, &var) == S_OK)
				{
					if(var.vt == VT_BOOL)
					{
						ret = var.boolVal;
					}
				}
				SafeRelease(&pReadOnlyProperty);
			}
			SafeRelease(&pRange);
		}
	}

	return ret;
}
