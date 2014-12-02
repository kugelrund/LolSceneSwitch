#include "MapsDialog.h"
#include "resource_maps.h"
#include "Log.h"



MapsDialog::MapsDialog(std::vector<CTSTR> scenes, MapScenes mapScenes) : scenes(scenes), selectedScenes(mapScenes)
{
}


_Success_(return) bool MapsDialog::Show(_In_ HINSTANCE hInstDLL, _In_ HWND hWnd, _Out_ MapScenes * scenes)
{
	if (DialogBoxParam(hInstDLL, MAKEINTRESOURCE(IDD_MAPSDIALOG), hWnd, DialogProc, reinterpret_cast<LPARAM>(this)) == IDOK)
	{
		*scenes = selectedScenes;
		return true;
	}
	return false;
}


void MapsDialog::GetControls(_In_ HWND hWnd)
{
	cmbSummonersRift = GetDlgItem(hWnd, IDC_CMB_SUMMONERSRIFT);
	cmbCrystalScar = GetDlgItem(hWnd, IDC_CMB_CRYSTALSCAR);
	cmbTwistedTreeline = GetDlgItem(hWnd, IDC_CMB_TWISTEDTREELINE);
	cmbHowlingAbyss = GetDlgItem(hWnd, IDC_CMB_HOWLINGABYSS);
}


void MapsDialog::LoadScenes()
{
	for (auto it = scenes.begin(); it != scenes.end(); ++it)
	{
		LPARAM lParam = reinterpret_cast<LPARAM>(*it);
		SendMessage(cmbSummonersRift, CB_ADDSTRING, 0, lParam);
		SendMessage(cmbCrystalScar, CB_ADDSTRING, 0, lParam);
		SendMessage(cmbTwistedTreeline, CB_ADDSTRING, 0, lParam);
		SendMessage(cmbHowlingAbyss, CB_ADDSTRING, 0, lParam);
	}

	WPARAM index = SendMessage(cmbSummonersRift, CB_FINDSTRINGEXACT, -1, 
							   reinterpret_cast<LPARAM>(static_cast<CTSTR>(selectedScenes.summonersRift)));
	SendMessage(cmbSummonersRift, CB_SETCURSEL, index, 0);
	index = SendMessage(cmbCrystalScar, CB_FINDSTRINGEXACT, -1,
						reinterpret_cast<LPARAM>(static_cast<CTSTR>(selectedScenes.crystalScar)));
	SendMessage(cmbCrystalScar, CB_SETCURSEL, index, 0);
	index = SendMessage(cmbTwistedTreeline, CB_FINDSTRINGEXACT, -1,
						reinterpret_cast<LPARAM>(static_cast<CTSTR>(selectedScenes.twistedTreeline)));
	SendMessage(cmbTwistedTreeline, CB_SETCURSEL, index, 0);
	index = SendMessage(cmbHowlingAbyss, CB_FINDSTRINGEXACT, -1,
						reinterpret_cast<LPARAM>(static_cast<CTSTR>(selectedScenes.howlingAbyss)));
	SendMessage(cmbHowlingAbyss, CB_SETCURSEL, index, 0);
}


void MapsDialog::ApplyScenes()
{
	selectedScenes.summonersRift = GetCBText(cmbSummonersRift, CB_ERR);
	selectedScenes.crystalScar = GetCBText(cmbCrystalScar, CB_ERR);
	selectedScenes.twistedTreeline = GetCBText(cmbTwistedTreeline, CB_ERR);
	selectedScenes.howlingAbyss = GetCBText(cmbHowlingAbyss, CB_ERR);
}


INT_PTR CALLBACK MapsDialog::DialogProc(_In_ HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			MapsDialog * dialog = (MapsDialog *)lParam;
			SetWindowLongPtr(hWnd, DWLP_USER, lParam);
			dialog->GetControls(hWnd);
			dialog->LoadScenes();
			return TRUE;
		}
		case WM_COMMAND:
		{
			MapsDialog * dialog = (MapsDialog *)GetWindowLongPtr(hWnd, DWLP_USER);
			switch (LOWORD(wParam))
			{
				case IDOK:
					dialog->ApplyScenes();
					EndDialog(hWnd, IDOK);
					break;
				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
					break;
			}
			break;
		}
	}
	return 0;
}