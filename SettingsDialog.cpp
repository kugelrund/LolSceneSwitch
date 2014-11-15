#include "SettingsDialog.h"
#include <ShlObj.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include "resource_main.h"
#include "MapsDialog.h"
#include "Log.h"



bool IsValidLoLPath(TCHAR const * path)
{
	TCHAR const logDir[] = TEXT("\\Logs\\Game - R3d Logs");
	size_t nameLength = (sizeof logDir) / (sizeof *logDir);
	size_t pathLength;
	
	TCHAR logPath[MAX_PATH];

	if (SUCCEEDED(StringCchLength(path, MAX_PATH, &pathLength)) && ((pathLength + nameLength) <= MAX_PATH) &&
		SUCCEEDED(StringCchCopy(logPath, MAX_PATH, path)) && 
		SUCCEEDED(StringCchCat(logPath, MAX_PATH, logDir)) &&
		PathFileExists(logPath))
	{
		Log("IsValidLoLPath() - path was valid: ", path);
		return true;
	}
	else
	{
		// if log dir was deleted or the game was never played
		TCHAR const radsDir[] = TEXT("\\RADS");
		nameLength = (sizeof radsDir) / (sizeof *radsDir);

		TCHAR radsPath[MAX_PATH];

		if (SUCCEEDED(StringCchLength(path, MAX_PATH, &pathLength)) && ((pathLength + nameLength) <= MAX_PATH) &&
			SUCCEEDED(StringCchCopy(radsPath, MAX_PATH, path)) &&
			SUCCEEDED(StringCchCat(radsPath, MAX_PATH, radsDir)) &&
			PathFileExists(radsPath))
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



SceneSelector::SceneSelector() :
chkEnabled(nullptr),
cmbScene(nullptr),
chkMaps(nullptr),
btnMaps(nullptr),
mapsAvail(false),
scenes()
{
}


SceneSelector::SceneSelector(_In_ HWND chkEnabled, _In_ HWND cmbScene) :
							 chkEnabled(chkEnabled),
							 cmbScene(cmbScene),
							 chkMaps(nullptr),
							 btnMaps(nullptr),
							 mapsAvail(false),
							 scenes()
{
}


SceneSelector::SceneSelector(_In_ HWND chkEnabled, _In_ HWND cmbScene, _In_ HWND chkMaps, _In_ HWND btnMaps) :
							 chkEnabled(chkEnabled),
							 cmbScene(cmbScene),
							 chkMaps(chkMaps),
							 btnMaps(btnMaps),
							 mapsAvail(true),
							 scenes()
{
}


void SceneSelector::EnableControls(bool disableAll) const
{
	if (disableAll)
	{
		EnableWindow(chkEnabled, FALSE);
		EnableWindow(cmbScene, FALSE);
		if (mapsAvail)
		{ 
			EnableWindow(chkMaps, FALSE);
			EnableWindow(btnMaps, FALSE);
		}
	}
	else
	{
		EnableWindow(chkEnabled, TRUE);
		BOOL const enableControls = (SendMessage(chkEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED);

		EnableWindow(cmbScene, enableControls);
		if (mapsAvail)
		{
			EnableWindow(chkMaps, enableControls);
			if (enableControls)
			{
				if (SendMessage(chkMaps, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					EnableWindow(cmbScene, FALSE);
					EnableWindow(btnMaps, TRUE);
				}
				else
				{
					EnableWindow(btnMaps, FALSE);
				}
			}
			else
			{
				EnableWindow(btnMaps, FALSE);
			}
		}
	}
}


void SceneSelector::SetMapScenes(MapScenes scenes)
{
	this->scenes.mapScenes = scenes;
}


MapScenes SceneSelector::GetMapScenes()
{
	return scenes.mapScenes;
}


void SceneSelector::AddScenes(std::vector<CTSTR> scenes) const
{
	for (auto it = scenes.begin(); it != scenes.end(); ++it)
	{
		SendMessage(cmbScene, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(*it));
	}
}


void SceneSelector::LoadSettings(StateScenes scenes)
{
	this->scenes = scenes;
	bool enabled = (!scenes.useMaps && !scenes.single.IsEmpty()) 
				    || (scenes.useMaps && !scenes.mapScenes.IsEmpty());

	WPARAM index = SendMessage(cmbScene, CB_FINDSTRINGEXACT, -1, 
							   reinterpret_cast<LPARAM>(static_cast<CTSTR>(scenes.single)));
	SendMessage(cmbScene, CB_SETCURSEL, index, 0);
	if (mapsAvail)
	{
		SendMessage(chkMaps, BM_SETCHECK, scenes.useMaps ? BST_CHECKED : BST_UNCHECKED, 0);
		if (scenes.useMaps)
		{
			enabled = !scenes.mapScenes.IsEmpty();
		}
	}

	SendMessage(chkEnabled, BM_SETCHECK, enabled, 0);
}


void SceneSelector::GetSettings(_Inout_ StateScenes &scenes)
{
	if (SendMessage(chkEnabled, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		scenes.single = GetCBText(cmbScene, CB_ERR);
		if (mapsAvail)
		{
			scenes.useMaps = (SendMessage(chkMaps, BM_GETCHECK, 0, 0) == BST_CHECKED);
			scenes.mapScenes = this->scenes.mapScenes;
		}
	}
	else
	{
		scenes.single = String();
		if (mapsAvail)
		{
			scenes.mapScenes = this->scenes.mapScenes;
		}
	}
}



SettingsDialog::SettingsDialog(_In_ Settings &settings) : settings(settings)
{
}


void SettingsDialog::Show(_In_ HWND hWnd, _In_ HINSTANCE hInstDLL)
{
	this->hInstDLL = hInstDLL;
	DialogBoxParam(hInstDLL, MAKEINTRESOURCE(IDD_SETTINGSDIALOG), hWnd, ConfigDlgProc, reinterpret_cast<LPARAM>(this));
}


void SettingsDialog::GetControls(_In_ HWND hWnd)
{
	hWndDialog = hWnd;
	SceneSelector clientScene(GetDlgItem(hWnd, IDC_CHK_CLIENTSCENE), GetDlgItem(hWnd, IDC_CMB_CLIENTSCENE));
	SceneSelector clientoutScene(GetDlgItem(hWnd, IDC_CHK_CLIENTOUTSCENE), GetDlgItem(hWnd, IDC_CMB_CLIENTOUTSCENE));
	SceneSelector champselectScene(GetDlgItem(hWnd, IDC_CHK_CHAMPSELECTSCENE), GetDlgItem(hWnd, IDC_CMB_CHAMPSELECTSCENE));
	SceneSelector postgameScene(GetDlgItem(hWnd, IDC_CHK_POSTGAMESCENE), GetDlgItem(hWnd, IDC_CMB_POSTGAMESCENE));
	SceneSelector loadscreenScene(GetDlgItem(hWnd, IDC_CHK_LOADSCREENSCENE), GetDlgItem(hWnd, IDC_CMB_LOADSCREENSCENE),
								  GetDlgItem(hWnd, IDC_CHK_LOADSCREENMAPS), GetDlgItem(hWnd, IDC_BTN_LOADSCREENMAPS));
	SceneSelector gameScene(GetDlgItem(hWnd, IDC_CHK_GAMESCENE), GetDlgItem(hWnd, IDC_CMB_GAMESCENE),
							GetDlgItem(hWnd, IDC_CHK_GAMEMAPS), GetDlgItem(hWnd, IDC_BTN_GAMEMAPS));
	SceneSelector endgameScene(GetDlgItem(hWnd, IDC_CHK_ENDGAMESCENE), GetDlgItem(hWnd, IDC_CMB_ENDGAMESCENE),
							   GetDlgItem(hWnd, IDC_CHK_ENDGAMEMAPS), GetDlgItem(hWnd, IDC_BTN_ENDGAMEMAPS));
	SceneSelector gameoutScene(GetDlgItem(hWnd, IDC_CHK_GAMEOUTSCENE), GetDlgItem(hWnd, IDC_CMB_GAMEOUTSCENE));
	selectors.emplace(State::CLIENT, clientScene);
	selectors.emplace(State::CLIENTOUT, clientoutScene);
	selectors.emplace(State::CHAMPSELECT, champselectScene);
	selectors.emplace(State::POSTGAME, postgameScene);
	selectors.emplace(State::LOADSCREEN, loadscreenScene);
	selectors.emplace(State::GAME, gameScene);
	selectors.emplace(State::ENDGAME, endgameScene);
	selectors.emplace(State::GAMEOUT, gameoutScene);
	selectorIndices.emplace(IDC_CHK_CLIENTSCENE, State::CLIENT);
	selectorIndices.emplace(IDC_CHK_CLIENTOUTSCENE, State::CLIENTOUT);
	selectorIndices.emplace(IDC_CHK_CHAMPSELECTSCENE, State::CHAMPSELECT);
	selectorIndices.emplace(IDC_CHK_POSTGAMESCENE, State::POSTGAME);
	selectorIndices.emplace(IDC_CHK_LOADSCREENSCENE, State::LOADSCREEN);
	selectorIndices.emplace(IDC_CHK_LOADSCREENMAPS, State::LOADSCREEN);
	selectorIndices.emplace(IDC_BTN_LOADSCREENMAPS, State::LOADSCREEN);
	selectorIndices.emplace(IDC_CHK_GAMESCENE, State::GAME);
	selectorIndices.emplace(IDC_CHK_GAMEMAPS, State::GAME);
	selectorIndices.emplace(IDC_BTN_GAMEMAPS, State::GAME);
	selectorIndices.emplace(IDC_CHK_ENDGAMESCENE, State::ENDGAME);
	selectorIndices.emplace(IDC_CHK_ENDGAMEMAPS, State::ENDGAME);
	selectorIndices.emplace(IDC_BTN_ENDGAMEMAPS, State::ENDGAME);
	selectorIndices.emplace(IDC_CHK_GAMEOUTSCENE, State::GAMEOUT);
}


void SettingsDialog::EnableControls() const
{
	BOOL const enableControls = IsDlgButtonChecked(hWndDialog, IDC_CHK_ENABLED) == BST_CHECKED ? TRUE : FALSE;

	EnableWindow(GetDlgItem(hWndDialog, IDC_LBL_LOLPATH), enableControls);
	EnableWindow(GetDlgItem(hWndDialog, IDC_TXT_LOLPATH), enableControls);
	EnableWindow(GetDlgItem(hWndDialog, IDC_BTN_LOLPATH), enableControls);
	
	EnableWindow(GetDlgItem(hWndDialog, IDC_LBL_INTERVALL), enableControls);
	EnableWindow(GetDlgItem(hWndDialog, IDC_TXT_INTERVALL), enableControls);

	for (auto selector : selectors)
	{
		selector.second.EnableControls(enableControls == FALSE);
	}
}


void SettingsDialog::LoadSettings()
{
	CheckDlgButton(hWndDialog, IDC_CHK_ENABLED, settings.enabled ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemText(hWndDialog, IDC_TXT_LOLPATH, settings.lolPath);

	XElement* sceneList = OBSGetSceneListElement();
	if (sceneList)
	{
		unsigned int const len = sceneList->NumElements();
		scenes.reserve(len);

		CTSTR sceneName = TEXT("");
		for (unsigned int i = 0; i < len; ++i)
		{
			sceneName = (sceneList->GetElementByID(i))->GetName();
			scenes.emplace_back(sceneName);
		}
	}

	for (auto &selector : selectors)
	{
		selector.second.AddScenes(scenes);
		selector.second.LoadSettings(settings.scenes[selector.first]);
	}
	
	SetDlgItemInt(hWndDialog, IDC_TXT_INTERVALL, settings.intervall, FALSE);
	EnableControls();
}

void SettingsDialog::ApplySettings()
{
	settings.enabled = (IsDlgButtonChecked(hWndDialog, IDC_CHK_ENABLED) == BST_CHECKED);
	TCHAR lolPath[MAX_PATH];
	GetDlgItemText(hWndDialog, IDC_TXT_LOLPATH, lolPath, MAX_PATH);
	settings.lolPath = String(lolPath);

	for (auto selector : selectors)
	{
		selector.second.GetSettings(settings.scenes[selector.first]);
	}

	settings.intervall = GetDlgItemInt(hWndDialog, IDC_TXT_INTERVALL, nullptr, FALSE);
	settings.SaveSettings();
}

INT_PTR CALLBACK SettingsDialog::ConfigDlgProc(_In_ HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			SettingsDialog * dialog = reinterpret_cast<SettingsDialog *>(lParam);
			SetWindowLongPtr(hWnd, DWLP_USER, lParam);
			dialog->GetControls(hWnd);
			dialog->LoadSettings();
			return TRUE;
		}
		case WM_COMMAND:
		{
			SettingsDialog * dialog = reinterpret_cast<SettingsDialog *>(GetWindowLongPtr(hWnd, DWLP_USER));
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
				case IDC_TXT_INTERVALL:
					switch (HIWORD(wParam))
					{
						case EN_KILLFOCUS:
						{
							BOOL success = FALSE;
							unsigned int const INTERVALL = GetDlgItemInt(hWnd, IDC_TXT_INTERVALL, &success, FALSE);
							if (success == FALSE || INTERVALL < 50 || INTERVALL > 5000)
							{
								MessageBox(hWnd,
									L"The intervall has to be in a range from 50 to 5000 ms",
									L"Invalid Value",
									MB_OK | MB_ICONERROR);
								SetFocus(GetDlgItem(hWnd, IDC_TXT_INTERVALL));
							}
							break;
						}
					}
					break;
				case IDC_BTN_LOADSCREENMAPS:
				case IDC_BTN_GAMEMAPS:
				case IDC_BTN_ENDGAMEMAPS:
				{
					SceneSelector * selector = &dialog->selectors[dialog->selectorIndices[LOWORD(wParam)]];
					MapsDialog mapsDialog(dialog->scenes, selector->GetMapScenes());
					MapScenes scenes;
					if (mapsDialog.Show(dialog->hInstDLL, hWnd, &scenes))
					{
						selector->SetMapScenes(scenes);
					}
					break;
				}
				default:
					dialog->selectors[dialog->selectorIndices[LOWORD(wParam)]].EnableControls(false);
					break;
			}
			break;
		}
	}
	return 0;
}