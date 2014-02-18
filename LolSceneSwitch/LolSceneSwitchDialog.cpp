#include "LolSceneSwitchDialog.h"
#include <ShlObj.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include "resource.h"
#include "Log.h"

bool IsValidLoLPath(TCHAR const * path)
{
	TCHAR const launcherName[] = TEXT("\\lol.launcher.exe");
	size_t const nameLength = (sizeof launcherName) / (sizeof *launcherName);
	size_t pathLength;
	
	TCHAR launcherPath[MAX_PATH];

	if (SUCCEEDED(StringCchLength(path, MAX_PATH, &pathLength)) && ((pathLength + nameLength) <= MAX_PATH) &&
		SUCCEEDED(StringCchCopy(launcherPath, MAX_PATH, path)) && 
		SUCCEEDED(StringCchCat(launcherPath, MAX_PATH, launcherName)) &&
		PathFileExists(launcherPath))
	{
		Log("IsValidLoLPath() - path was valid: ", path);
		return true;
	}
	else
	{
		Log("IsValidLoLPath() - path was invalid: ", path);
		return false;
	}
}

class OpenFolderDialog
{
private:

	IFileDialog *pfd;

	IFileDialogEvents *pfde;

	DWORD dwCookie;

public:

	OpenFolderDialog(void);

	~OpenFolderDialog(void);

	bool Show(WCHAR * pszFilePath, HWND hWndParent);
};

#pragma region OpenFolderDialog
class CDialogEventHandler : public IFileDialogEvents, public IFileDialogControlEvents
{
public:

	// IUnknown methods
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] = {
			QITABENT(CDialogEventHandler, IFileDialogEvents),
			QITABENT(CDialogEventHandler, IFileDialogControlEvents),
			{ 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		long cRef = InterlockedDecrement(&_cRef);
		if (!cRef)
			delete this;
		return cRef;
	}

	// IFileDialogEvents methods
	IFACEMETHODIMP OnFileOk(__RPC__in_opt IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnFolderChange(__RPC__in_opt IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnFolderChanging(__RPC__in_opt IFileDialog *, __RPC__in_opt IShellItem *) { return S_OK; };
	IFACEMETHODIMP OnHelp(__RPC__in_opt IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnSelectionChange(__RPC__in_opt IFileDialog *) { return S_OK; };
	IFACEMETHODIMP OnShareViolation(__RPC__in_opt IFileDialog *, __RPC__in_opt IShellItem *, __RPC__in_opt FDE_SHAREVIOLATION_RESPONSE *) { return S_OK; };
	IFACEMETHODIMP OnTypeChange(__RPC__in_opt IFileDialog *pfd) { return S_OK; };
	IFACEMETHODIMP OnOverwrite(__RPC__in_opt IFileDialog *, __RPC__in_opt IShellItem *, __RPC__in_opt FDE_OVERWRITE_RESPONSE *) { return S_OK; };

	// IFileDialogControlEvents methods
	IFACEMETHODIMP OnItemSelected(__RPC__in_opt IFileDialogCustomize *pfdc, DWORD dwIDCtl, DWORD dwIDItem) { return S_OK; };
	IFACEMETHODIMP OnButtonClicked(__RPC__in_opt IFileDialogCustomize *, DWORD) { return S_OK; };
	IFACEMETHODIMP OnCheckButtonToggled(__RPC__in_opt IFileDialogCustomize *, DWORD, BOOL) { return S_OK; };
	IFACEMETHODIMP OnControlActivating(__RPC__in_opt IFileDialogCustomize *, DWORD) { return S_OK; };

	CDialogEventHandler() : _cRef(1) { };

private:

	~CDialogEventHandler() { };

	long _cRef;
};

// Instance creation helper
HRESULT CDialogEventHandler_CreateInstance(REFIID riid, void **ppv)
{
	*ppv = nullptr;
	CDialogEventHandler *pDialogEventHandler = new (std::nothrow) CDialogEventHandler();
	HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
	if (SUCCEEDED(hr))
	{
		hr = pDialogEventHandler->QueryInterface(riid, ppv);
		pDialogEventHandler->Release();
	}
	return hr;
}

OpenFolderDialog::OpenFolderDialog(void)
{
	if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
	{
		pfd = nullptr;
		pfde = nullptr;
		// CoCreate the File Open Dialog object.
		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

		if (SUCCEEDED(hr))
		{
			// Create an event handling object, and hook it up to the dialog.
			hr = CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde));
			if (SUCCEEDED(hr))
			{
				// Hook up the event handler.
				hr = pfd->Advise(pfde, &dwCookie);
				if (SUCCEEDED(hr))
				{
					// Set the options on the dialog.
					DWORD dwFlags;

					// Before setting, always get the options first in order 
					// not to override existing options.
					hr = pfd->GetOptions(&dwFlags);
					if (SUCCEEDED(hr))
					{
						// In this case, get shell items only for file system items.
						hr = pfd->SetOptions(dwFlags | FOS_PICKFOLDERS);
					}
				}
			}
		}
		CoUninitialize();
	}
}

bool OpenFolderDialog::Show(WCHAR * path, HWND hWndParent)
{
	// Show the dialog
	HRESULT hr = pfd->Show(hWndParent);

	if (SUCCEEDED(hr))
	{
		// Obtain the result once the user clicks 
		// the 'Open' button.
		// The result is an IShellItem object.
		IShellItem *psiResult;
		hr = pfd->GetResult(&psiResult);

		if (SUCCEEDED(hr))
		{
			LPWSTR folderPath;
			hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &folderPath);

			if (folderPath != nullptr)
			{
				StringCchCopy(path, MAX_PATH, folderPath);
				if (SUCCEEDED(hr))
				{
					CoTaskMemFree(folderPath);
					return true;
				}
			}
			psiResult->Release();
		}
	}

	return false;
}

OpenFolderDialog::~OpenFolderDialog(void)
{
	pfd->Unadvise(dwCookie);
	pfde->Release();
	pfd->Release();
}
#pragma endregion





enum SceneSelections
{
	CLOSED_SCENE = 0, TABBEDOUT_SCENE = 1, LOADSCREEN_SCENE = 2, GAME_SCENE = 3, ENDGAME_SCENE = 4
};





SceneSelector::SceneSelector(HWND hWndDialog, HWND chkEnabled, HWND cmbScene) :
							 chkEnabled(chkEnabled),
							 chkUseMultiple(nullptr),
							 cmbScenes(new HWND[1]),
							 lblScenes(nullptr),
							 length(1),
							 multiple(false)
{
	cmbScenes[0] = cmbScene;
}

SceneSelector::SceneSelector(HWND hWndDialog, HWND chkEnabled, HWND chkUseMultiple, int startID, unsigned int length) :
							 chkEnabled(chkEnabled),
							 chkUseMultiple(chkUseMultiple),
							 cmbScenes(new HWND[length]),
							 lblScenes(new HWND[length]),
							 length(length),
							 multiple(true)
{
	for (unsigned int i = 0; i < length; ++i)
	{
		cmbScenes[i] = GetDlgItem(hWndDialog, startID + 2 * i);
		lblScenes[i] = GetDlgItem(hWndDialog, startID + 2 * i + 1);
	}
	AddScene(L"");
}

SceneSelector::~SceneSelector()
{
	delete[] cmbScenes;
	if (multiple)
	{
		delete[] lblScenes;
	}
}

void SceneSelector::EnableControls(bool disableAll) const
{
	if (disableAll)
	{
		EnableWindow(chkEnabled, FALSE);
		if (!multiple)
		{
			EnableWindow(cmbScenes[0], FALSE);
		}
		else
		{
			EnableWindow(chkUseMultiple, FALSE);
			for (unsigned int i = 0; i < length; ++i)
			{
				EnableWindow(cmbScenes[i], FALSE);
				EnableWindow(lblScenes[i], FALSE);
			}
		}
	}
	else
	{
		EnableWindow(chkEnabled, TRUE);
		BOOL const enableControls = SendMessage(chkEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED;
		if (!multiple)
		{
			EnableWindow(cmbScenes[0], enableControls);
		}
		else
		{
			EnableWindow(chkUseMultiple, TRUE);
			for (unsigned int i = 0; i < length; ++i)
			{
				EnableWindow(cmbScenes[i], enableControls);
				EnableWindow(lblScenes[i], enableControls);
			}

			EnableWindow(chkUseMultiple, enableControls);
			BOOL const showControls = (SendMessage(chkUseMultiple, BM_GETCHECK, 0, 0) == BST_CHECKED) ? SW_SHOW : SW_HIDE;
			ShowWindow(lblScenes[0], showControls);
			for (unsigned int i = 1; i < length; ++i)
			{
				ShowWindow(cmbScenes[i], showControls);
				ShowWindow(lblScenes[i], showControls);
			}
		}
	}
}

void SceneSelector::SetEnabled() const
{
	bool enabled = !GetCBText(cmbScenes[0], CB_ERR).IsEmpty();
	SendMessage(chkEnabled, BM_SETCHECK, enabled ? BST_CHECKED : BST_UNCHECKED, 0);
}

void SceneSelector::SetEnabled(bool useMultiple) const
{
	bool enabled = false;
	for (unsigned int i = 0; i < length; ++i)
	{
		if (!GetCBText(cmbScenes[i], CB_ERR).IsEmpty())
		{
			enabled = true;
			break;
		}
	}
	SendMessage(chkEnabled, BM_SETCHECK, enabled ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(chkUseMultiple, BM_SETCHECK, useMultiple ? BST_CHECKED : BST_UNCHECKED, 0);
}

void SceneSelector::AddScene(CTSTR scene) const
{
	for (unsigned int i = 0; i < length; ++i)
	{
		SendMessage(cmbScenes[i], CB_ADDSTRING, 0, reinterpret_cast<LPARAM> (scene));
	}
}

void SceneSelector::SetSelectedScenes(String const * scenes) const
{
	for (unsigned int i = 0; i < length; ++i)
	{
		SendMessage(cmbScenes[i], CB_SETCURSEL, 
					SendMessage(cmbScenes[i], CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(static_cast<CTSTR>(scenes[i]))), 0);
	}
}

void SceneSelector::GetSelectedScenes(String * scenes) const
{
	if (SendMessage(chkEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		for (unsigned int i = 0; i < length; ++i)
		{
			scenes[i] = GetCBText(cmbScenes[i], CB_ERR);
		}
	}
	else
	{
		for (unsigned int i = 0; i < length; ++i)
		{
			scenes[i] = String();
		}
	}
}

bool SceneSelector::UseMultiple() const
{
	return (SendMessage(chkUseMultiple, BM_GETCHECK, 0, 0) == BST_CHECKED);
}





LolSceneSwitchDialog::LolSceneSwitchDialog(LolSceneSwitchSettings & settings) : settings(settings), sceneSelectors()
{
}

void LolSceneSwitchDialog::Show(HWND hWnd, HINSTANCE hInstDLL) const
{
	DialogBoxParam(hInstDLL, MAKEINTRESOURCE(IDD_CONFIGDIALOG), hWnd, ConfigDlgProc, reinterpret_cast<LPARAM>(this));
}

void LolSceneSwitchDialog::GetControls(HWND hWnd)
{
	hWndDialog = hWnd;
	sceneSelectors.reserve(5);
	sceneSelectors.emplace_back(hWnd, GetDlgItem(hWnd, IDC_CHK_CLOSEDSCENE), GetDlgItem(hWnd, IDC_CMB_CLOSEDSCENE));
	sceneSelectors.emplace_back(hWnd, GetDlgItem(hWnd, IDC_CHK_TABBEDOUTSCENE), GetDlgItem(hWnd, IDC_CMB_TABBEDOUTSCENE));
	sceneSelectors.emplace_back(hWnd, GetDlgItem(hWnd, IDC_CHK_LOADSCREENSCENE), GetDlgItem(hWnd, IDC_CHK_LOADSCREENMAPS),
								IDC_CMB_LOADSCREENSCENE0, 4);
	sceneSelectors.emplace_back(hWnd, GetDlgItem(hWnd, IDC_CHK_GAMESCENE), GetDlgItem(hWnd, IDC_CHK_GAMEMAPS),
								IDC_CMB_GAMESCENE0, 4);
	sceneSelectors.emplace_back(hWnd, GetDlgItem(hWnd, IDC_CHK_ENDGAMESCENE), GetDlgItem(hWnd, IDC_CHK_ENDGAMEMAPS),
								IDC_CMB_ENDGAMESCENE0, 4);
}

void LolSceneSwitchDialog::EnableControls() const
{
	BOOL const enableControls = IsDlgButtonChecked(hWndDialog, IDC_CHK_ENABLED) == BST_CHECKED ? TRUE : FALSE;

	EnableWindow(GetDlgItem(hWndDialog, IDC_LBL_LOLPATH), enableControls);
	EnableWindow(GetDlgItem(hWndDialog, IDC_TXT_LOLPATH), enableControls);
	EnableWindow(GetDlgItem(hWndDialog, IDC_BTN_LOLPATH), enableControls);
	
	EnableWindow(GetDlgItem(hWndDialog, IDC_LBL_INTERVALL), enableControls);
	EnableWindow(GetDlgItem(hWndDialog, IDC_TXT_INTERVALL), enableControls);

	for (auto it = sceneSelectors.begin(); it != sceneSelectors.end(); ++it)
	{
		it->EnableControls(enableControls == FALSE);
	}
}

void LolSceneSwitchDialog::LoadSettings() const
{
	CheckDlgButton(hWndDialog, IDC_CHK_ENABLED, settings.enabled ? BST_CHECKED : BST_UNCHECKED);
	
	SetDlgItemText(hWndDialog, IDC_TXT_LOLPATH, settings.lolPath);

	XElement* sceneList = OBSGetSceneListElement();
	if (sceneList)
	{
		CTSTR sceneName = TEXT("");
		unsigned int const len = sceneList->NumElements();
		for (unsigned int i = 0; i < len; ++i)
		{
			sceneName = (sceneList->GetElementByID(i))->GetName();
			for (auto it = sceneSelectors.begin(); it != sceneSelectors.end(); ++it)
			{
				it->AddScene(sceneName);
			}
		}
	}
	
	sceneSelectors[CLOSED_SCENE].SetSelectedScenes(&settings.closedScene);
	sceneSelectors[TABBEDOUT_SCENE].SetSelectedScenes(&settings.tabbedoutScene);
	sceneSelectors[LOADSCREEN_SCENE].SetSelectedScenes(settings.loadscreenScene);
	sceneSelectors[GAME_SCENE].SetSelectedScenes(settings.gameScene);
	sceneSelectors[ENDGAME_SCENE].SetSelectedScenes(settings.endgameScene);

	sceneSelectors[CLOSED_SCENE].SetEnabled();
	sceneSelectors[TABBEDOUT_SCENE].SetEnabled();
	sceneSelectors[LOADSCREEN_SCENE].SetEnabled(settings.loadscreenMaps);
	sceneSelectors[GAME_SCENE].SetEnabled(settings.gameMaps);
	sceneSelectors[ENDGAME_SCENE].SetEnabled(settings.endgameMaps);
	
	SetDlgItemInt(hWndDialog, IDC_TXT_INTERVALL, settings.intervall, FALSE);

	EnableControls();
}

void LolSceneSwitchDialog::ApplySettings()
{
	settings.enabled = (IsDlgButtonChecked(hWndDialog, IDC_CHK_ENABLED) == BST_CHECKED);

	WCHAR lolPath[MAX_PATH];
	GetDlgItemText(hWndDialog, IDC_TXT_LOLPATH, lolPath, MAX_PATH);
	settings.lolPath = String(lolPath);

	sceneSelectors[CLOSED_SCENE].GetSelectedScenes(&settings.closedScene);
	sceneSelectors[TABBEDOUT_SCENE].GetSelectedScenes(&settings.tabbedoutScene);
	sceneSelectors[LOADSCREEN_SCENE].GetSelectedScenes(settings.loadscreenScene);
	sceneSelectors[GAME_SCENE].GetSelectedScenes(settings.gameScene);
	sceneSelectors[ENDGAME_SCENE].GetSelectedScenes(settings.endgameScene);
	
	settings.loadscreenMaps = sceneSelectors[LOADSCREEN_SCENE].UseMultiple();
	settings.gameMaps = sceneSelectors[GAME_SCENE].UseMultiple();
	settings.endgameMaps = sceneSelectors[ENDGAME_SCENE].UseMultiple();

	settings.intervall = GetDlgItemInt(hWndDialog, IDC_TXT_INTERVALL, nullptr, FALSE);

	settings.SaveSettings();
}

INT_PTR CALLBACK LolSceneSwitchDialog::ConfigDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			LolSceneSwitchDialog * dialog = (LolSceneSwitchDialog *)lParam;
			SetWindowLongPtr(hWnd, DWLP_USER, lParam);
			dialog->GetControls(hWnd);
			dialog->LoadSettings();
			return TRUE;
		}
		case WM_COMMAND:
		{
			LolSceneSwitchDialog * dialog = (LolSceneSwitchDialog *)GetWindowLongPtr(hWnd, DWLP_USER);
			switch (LOWORD(wParam))
			{
				case IDOK:
					dialog->ApplySettings();
					EndDialog(hWnd, LOWORD(wParam));
					break;
				case IDCANCEL:
					EndDialog(hWnd, LOWORD(wParam));
					break;
				case IDC_BTN_LOLPATH:
				{
					OpenFolderDialog ofd = OpenFolderDialog();
					WCHAR folder[MAX_PATH];
					if (ofd.Show(folder, hWnd) && IsValidLoLPath(folder))
					{
						SendDlgItemMessageW(hWnd, IDC_TXT_LOLPATH, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(folder));
					}
					else
					{
						MessageBox(hWnd,
							TEXT("The selected folder is not a valid League of Legends folder!"),
							TEXT("Invalid Folder"),
							MB_OK | MB_ICONERROR);
					}
					break;
				}
				case IDC_CHK_ENABLED:
					dialog->EnableControls();
					break;
				case IDC_CHK_CLOSEDSCENE:
					dialog->sceneSelectors[CLOSED_SCENE].EnableControls(false);
					break;
				case IDC_CHK_TABBEDOUTSCENE:
					dialog->sceneSelectors[TABBEDOUT_SCENE].EnableControls(false);
					break;
				case IDC_CHK_LOADSCREENSCENE:
				case IDC_CHK_LOADSCREENMAPS:
					dialog->sceneSelectors[LOADSCREEN_SCENE].EnableControls(false);
					break;
				case IDC_CHK_GAMESCENE:
				case IDC_CHK_GAMEMAPS:
					dialog->sceneSelectors[GAME_SCENE].EnableControls(false);
					break;
				case IDC_CHK_ENDGAMESCENE:
				case IDC_CHK_ENDGAMEMAPS:
					dialog->sceneSelectors[ENDGAME_SCENE].EnableControls(false);
					break;
				case IDC_TXT_INTERVALL:
					switch (HIWORD(wParam))
					{
						case EN_KILLFOCUS:
						{
							BOOL success = FALSE;
							unsigned int const intervall = GetDlgItemInt(hWnd, IDC_TXT_INTERVALL, &success, FALSE);
							if (success == FALSE || intervall < 50 || intervall > 5000)
							{
								MessageBox(hWnd,
									L"The intervall has to be in a range from 100 to 5000 ms",
									L"Invalid Value",
									MB_OK | MB_ICONERROR);
								SetFocus(GetDlgItem(hWnd, IDC_TXT_INTERVALL));
							}
							break;
						}
					}
					break;
			}
			break;
		}
	}

	return 0;
}