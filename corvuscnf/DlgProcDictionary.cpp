﻿
#include "configxml.h"
#include "corvuscnf.h"
#include "resource.h"

//デフォルト
static LPCWSTR defaultHost = L"localhost";
static LPCWSTR defaultPort = L"1178";
static LPCWSTR defaultTimeOut = L"1000";

INT_PTR CALLBACK DlgProcDictionary(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hWndListView;
	LV_COLUMNW lvc;
	LVITEMW item;
	int	index, count;
	OPENFILENAMEW ofn;
	WCHAR path[MAX_PATH];
	WCHAR pathBak[MAX_PATH];
	WCHAR num[16];
	WCHAR host[MAX_SKKSERVER_HOST];
	WCHAR port[MAX_SKKSERVER_PORT];
	std::wstring strxmlval;

	switch(message)
	{
	case WM_INITDIALOG:
		hWndListView = GetDlgItem(hDlg, IDC_LIST_SKK_DIC);
		ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT);
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_CENTER;

		lvc.iSubItem = 0;
		lvc.cx = 220;
		lvc.pszText = L"";
		ListView_InsertColumn(hWndListView, 0, &lvc);

		LoadDictionary(hDlg);

		LoadCheckButton(hDlg, IDC_CHECKBOX_SKKSRV, SectionServer, Serv);

		ReadValue(pathconfigxml, SectionServer, Host, strxmlval);
		if(strxmlval.empty()) strxmlval = defaultHost;
		SetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_HOST, strxmlval.c_str());

		ReadValue(pathconfigxml, SectionServer, Port, strxmlval);
		if(strxmlval.empty()) strxmlval = defaultPort;
		SetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_PORT, strxmlval.c_str());

		ReadValue(pathconfigxml, SectionServer, TimeOut, strxmlval);
		if(strxmlval.empty()) strxmlval = defaultTimeOut;
		SetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_TIMEOUT, strxmlval.c_str());

		return (INT_PTR)TRUE;

	case WM_COMMAND:
		hWndListView = GetDlgItem(hDlg, IDC_LIST_SKK_DIC);

		switch(LOWORD(wParam))
		{
		case IDC_BUTTON_SKK_DIC_UP:
			index = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
			if(index > 0)
			{
				PropSheet_Changed(GetParent(hDlg), hDlg);

				ListView_GetItemText(hWndListView, index - 1, 0, pathBak, _countof(pathBak));
				ListView_GetItemText(hWndListView, index, 0, path, _countof(path));
				ListView_SetItemText(hWndListView, index - 1, 0, path);
				ListView_SetItemText(hWndListView, index, 0, pathBak);
				ListView_SetItemState(hWndListView, index - 1, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
				ListView_EnsureVisible(hWndListView, index - 1, FALSE);
			}
			return (INT_PTR)TRUE;

		case IDC_BUTTON_SKK_DIC_DOWN:
			index = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
			count = ListView_GetItemCount(hWndListView);
			if(index >= 0 && index < count - 1)
			{
				PropSheet_Changed(GetParent(hDlg), hDlg);

				ListView_GetItemText(hWndListView, index + 1, 0, pathBak, _countof(pathBak));
				ListView_GetItemText(hWndListView, index, 0, path, _countof(path));
				ListView_SetItemText(hWndListView, index + 1, 0, path);
				ListView_SetItemText(hWndListView, index, 0, pathBak);
				ListView_SetItemState(hWndListView, index + 1, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
				ListView_EnsureVisible(hWndListView, index + 1, FALSE);
			}
			return (INT_PTR)TRUE;

		case IDC_BUTTON_SKK_DIC_ADD:
			path[0]=0;
			memset(&ofn, 0,sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = path;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST;
			if(GetOpenFileNameW(&ofn) != 0)
			{
				PropSheet_Changed(GetParent(hDlg), hDlg);

				index = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
				count = ListView_GetItemCount(hWndListView);
				if(index == -1)
				{
					index = count;
				}
				else
				{
					++index;
				}
				item.mask = LVIF_TEXT;
				item.pszText = path;
				item.iItem = index;
				item.iSubItem = 0;
				ListView_InsertItem(hWndListView, &item);
				ListView_SetItemState(hWndListView, index, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
				ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);
				ListView_EnsureVisible(hWndListView, index, FALSE);
			}
			return (INT_PTR)TRUE;

		case IDC_BUTTON_SKK_DIC_DEL:
			index = ListView_GetNextItem(hWndListView, -1, LVNI_SELECTED);
			if(index != -1)
			{
				PropSheet_Changed(GetParent(hDlg), hDlg);

				ListView_DeleteItem(hWndListView, index);
				ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE);
			}
			return (INT_PTR)TRUE;

		case IDC_BUTTON_SKK_DIC_MAKE:
			if(IDOK == MessageBoxW(hDlg,
				L"SKK辞書を読み込み、独自形式辞書を作成します。\nよろしいですか？",
				TextServiceDesc, MB_OKCANCEL))
			{
				MakeSKKDic(hDlg);
				MessageBoxW(hDlg, L"完了しました。", TextServiceDesc, MB_OK);
			}
			return (INT_PTR)TRUE;

		case IDC_CHECKBOX_SKKSRV:
			PropSheet_Changed(GetParent(hDlg), hDlg);
			return (INT_PTR)TRUE;

		case IDC_EDIT_SKKSRV_HOST:
		case IDC_EDIT_SKKSRV_PORT:
		case IDC_EDIT_SKKSRV_TIMEOUT:
			switch(HIWORD(wParam))
			{
			case EN_CHANGE:
				PropSheet_Changed(GetParent(hDlg), hDlg);
				return (INT_PTR)TRUE;
			default:
				break;
			}
			break;

		default:
			break;
		}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->code)
		{
		case PSN_APPLY:
			WriterStartSection(pXmlWriter, SectionDictionary);

			SaveDictionary(hDlg);

			WriterEndSection(pXmlWriter);

			WriterStartSection(pXmlWriter, SectionServer);

			SaveCheckButton(hDlg, IDC_CHECKBOX_SKKSRV, SectionServer, Serv);

			GetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_HOST, host, _countof(host));
			WriterKey(pXmlWriter, Host, host);

			GetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_PORT, port, _countof(port));
			WriterKey(pXmlWriter, Port, port);

			GetDlgItemTextW(hDlg, IDC_EDIT_SKKSRV_TIMEOUT, num, _countof(num));
			WriterKey(pXmlWriter, TimeOut, num);

			WriterEndSection(pXmlWriter);

			return (INT_PTR)TRUE;
		default:
			break;
		}
		break;

	default:
		break;
	}

	return (INT_PTR)FALSE;
}
