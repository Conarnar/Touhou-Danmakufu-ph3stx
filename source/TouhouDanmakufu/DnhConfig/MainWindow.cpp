#include "source/GcLib/pch.h"

#include "MainWindow.hpp"
#include "GcLibImpl.hpp"

/**********************************************************
//MainWindow
**********************************************************/
MainWindow::MainWindow() {
}
MainWindow::~MainWindow() {
}
bool MainWindow::Initialize() {
	hWnd_ = ::CreateDialog((HINSTANCE)GetWindowLong(NULL, GWL_HINSTANCE),
		MAKEINTRESOURCE(IDD_DIALOG_MAIN),
		NULL, (DLGPROC)_StaticWindowProcedure);

//	::SetClassLong(hWnd_, GCL_HICON, 
//		( LONG )(HICON)LoadImage(Application::GetApplicationHandle(), MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 32, 32, 0));

	Attach(hWnd_);
	::ShowWindow(hWnd_, SW_HIDE);

	//�^�u
	HWND hTab = GetDlgItem(hWnd_, IDC_TAB_MAIN);
	wndTab_ = new WTabControll();
	wndTab_->Attach(hTab);

	//DevicePanel
	panelDevice_ = new DevicePanel();
	panelDevice_->Initialize(hTab);
	wndTab_->AddTab(L"Device", panelDevice_);

	//KeyPanel
	panelKey_ = new KeyPanel();
	panelKey_->Initialize(hTab);
	wndTab_->AddTab(L"Key", panelKey_);

	//OptionPanel
	panelOption_ = new OptionPanel();
	panelOption_->Initialize(hTab);
	wndTab_->AddTab(L"Option", panelOption_);

	//����������
	ReadConfiguration();
	MoveWindowCenter();
	wndTab_->ShowPage();

	wndTab_->LocateParts();
	::ShowWindow(hWnd_, SW_SHOW);

	MainWindow::GetInstance()->Load();

	return true;
}
bool MainWindow::StartUp() {
	panelKey_->StartUp();
	return true;
}
LRESULT MainWindow::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
	{
		::DestroyWindow(hWnd);
		break;
	}
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		break;
	}
	case WM_COMMAND:
	{
		switch (wParam & 0xffff) {
		case ID_MENUITEM_MAIN_EXIT:
		case IDCANCEL:
			::DestroyWindow(hWnd_);
			break;

		case IDOK:
			Save();
			::DestroyWindow(hWnd_);
			break;

		case ID_BUTTON_EXECUTE:
		{
			Save();
			_RunExecutor();
			::DestroyWindow(hWnd_);
			break;
		}
		}
		break;
	}

	case WM_NOTIFY:
	{
		switch (((NMHDR*)lParam)->code) {
		case TCN_SELCHANGE:
			wndTab_->ShowPage();
			break;
		}
		break;
	}

	}
	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);
}
void MainWindow::_RunExecutor() {
	PROCESS_INFORMATION infoProcess;
	ZeroMemory(&infoProcess, sizeof(infoProcess));

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(STARTUPINFO);

	BOOL res = ::CreateProcess(
		NULL,
		(wchar_t*)panelOption_->GetExecutablePath().c_str(),
		NULL, NULL,
		TRUE, 0,
		NULL, NULL,
		&si, &infoProcess
	);
	/*
	if (res == 0) {
		std::wstring log = StringUtility::Format(L"Could not start the game. [%s]\r\n", ErrorUtility::GetLastErrorMessage().c_str());
		Logger::WriteTop(log);
		return;
	}
	*/

	::CloseHandle(infoProcess.hProcess);
	::CloseHandle(infoProcess.hThread);
}

void MainWindow::ClearData() {

}
bool MainWindow::Load() {
	RecordBuffer record;
	std::string path;
/*
	bool res = record.ReadFromFile(path);
	if(!res)
	{
		//::MessageBox(hWnd_, "�ǂݍ��ݎ��s", "�ݒ���J��", MB_OK);
		return false;
	}
*/

	return true;
}
bool MainWindow::Save() {
	WriteConfiguration();
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	config->SaveConfigFile();

	return true;
}
void MainWindow::UpdateKeyAssign() {
	if (!panelKey_->IsWindowVisible())return;
	panelKey_->UpdateKeyAssign();

}
void MainWindow::ReadConfiguration() {
	panelDevice_->ReadConfiguration();
	panelKey_->ReadConfiguration();
	panelOption_->ReadConfiguration();
}
void MainWindow::WriteConfiguration() {
	panelDevice_->WriteConfiguration();
	panelKey_->WriteConfiguration();
	panelOption_->WriteConfiguration();
}

/**********************************************************
//DevicePanel
**********************************************************/
DevicePanel::DevicePanel() {
}
DevicePanel::~DevicePanel() {

}
bool DevicePanel::Initialize(HWND hParent) {
	hWnd_ = ::CreateDialog((HINSTANCE)GetWindowLong(NULL, GWL_HINSTANCE),
		MAKEINTRESOURCE(IDD_PANEL_DEVICE),
		hParent, (DLGPROC)_StaticWindowProcedure);
	Attach(hWnd_);

	comboWindowSize_.Attach(::GetDlgItem(hWnd_, IDC_COMBO_WINDOWSIZE));
	comboWindowSize_.AddString(L"640x480");
	comboWindowSize_.AddString(L"800x600");
	comboWindowSize_.AddString(L"960x720");
	comboWindowSize_.AddString(L"1280x960");
	SetWindowPos(comboWindowSize_.GetWindowHandle(), NULL, 0, 0,
		comboWindowSize_.GetClientWidth(), 200, SWP_NOMOVE);

	comboMultisample_.Attach(::GetDlgItem(hWnd_, IDC_COMBO_MULTISAMPLE));
	{
		D3DMULTISAMPLE_TYPE listSamples[] = {
			D3DMULTISAMPLE_NONE,
			D3DMULTISAMPLE_2_SAMPLES,
			D3DMULTISAMPLE_3_SAMPLES,
			D3DMULTISAMPLE_4_SAMPLES
		};
		std::wstring listText[] = {
			L"None (Default)",
			L"2x",
			L"3x",
			L"4x"
		};

		for (size_t i = 0; i < sizeof(listSamples) / sizeof(D3DMULTISAMPLE_TYPE); ++i) {
			comboMultisample_.AddString(listText[i]);
		}
	}
	SetWindowPos(comboMultisample_.GetWindowHandle(), NULL, 0, 0,
		comboMultisample_.GetClientWidth(), 200, SWP_NOMOVE);

	return true;
}
LRESULT DevicePanel::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return WPanel::_WindowProcedure(hWnd, uMsg, wParam, lParam);
}
void DevicePanel::ReadConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	int screenMode = config->GetScreenMode();
	switch (screenMode) {
	case DirectGraphics::SCREENMODE_FULLSCREEN:
		SendDlgItemMessage(hWnd_, IDC_RADIO_FULLSCREEN, BM_SETCHECK, BST_CHECKED, 0);
		break;
	default:
		SendDlgItemMessage(hWnd_, IDC_RADIO_WINDOW, BM_SETCHECK, BST_CHECKED, 0);
		break;
	}

	comboWindowSize_.SetSelectedIndex(config->sizeWindow_);

	{
		std::map<D3DMULTISAMPLE_TYPE, int> mapSampleIndex = {
			{ D3DMULTISAMPLE_NONE, 0 },
			{ D3DMULTISAMPLE_2_SAMPLES, 1 },
			{ D3DMULTISAMPLE_3_SAMPLES, 2 },
			{ D3DMULTISAMPLE_4_SAMPLES, 3 },
		};
		comboMultisample_.SetSelectedIndex(mapSampleIndex[config->multiSamples_]);
	}

	int fpsType = config->GetFpsType();
	switch (fpsType) {
	case DnhConfiguration::FPS_NORMAL:
		SendDlgItemMessage(hWnd_, IDC_RADIO_FPS_1, BM_SETCHECK, BST_CHECKED, 0);
		break;
	case DnhConfiguration::FPS_1_2:
		SendDlgItemMessage(hWnd_, IDC_RADIO_FPS_2, BM_SETCHECK, BST_CHECKED, 0);
		break;
	case DnhConfiguration::FPS_1_3:
		SendDlgItemMessage(hWnd_, IDC_RADIO_FPS_3, BM_SETCHECK, BST_CHECKED, 0);
		break;
	case DnhConfiguration::FPS_AUTO:
		SendDlgItemMessage(hWnd_, IDC_RADIO_FPS_AUTO, BM_SETCHECK, BST_CHECKED, 0);
		break;
	}

	SendDlgItemMessage(hWnd_, (config->modeColor_ == DirectGraphicsConfig::COLOR_MODE_32BIT) ?
		IDC_RADIO_COLOR_32 : IDC_RADIO_COLOR_16, BM_SETCHECK, BST_CHECKED, 0);
	SendDlgItemMessage(hWnd_, IDC_VSYNC, BM_SETCHECK, 
		config->IsEnableVSync() ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hWnd_, IDC_REFERENCERASTERIZER, BM_SETCHECK,
		config->IsEnableRef() ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(hWnd_, IDC_PSEUDOFULLSCREEN, BM_SETCHECK,
		config->bPseudoFullscreen_ ? BST_CHECKED : BST_UNCHECKED, 0);
}
void DevicePanel::WriteConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	int screenMode = DirectGraphics::SCREENMODE_WINDOW;
	if (SendDlgItemMessage(hWnd_, IDC_RADIO_FULLSCREEN, BM_GETCHECK, 0, 0))
		screenMode = DirectGraphics::SCREENMODE_FULLSCREEN;
	config->SetScreenMode(screenMode);

	int windowSize = comboWindowSize_.GetSelectedIndex();
	config->SetWindowSize(windowSize);

	{
		std::map<int, D3DMULTISAMPLE_TYPE> mapIndexSample = {
			{ 0, D3DMULTISAMPLE_NONE },
			{ 1, D3DMULTISAMPLE_2_SAMPLES },
			{ 2, D3DMULTISAMPLE_3_SAMPLES },
			{ 3, D3DMULTISAMPLE_4_SAMPLES },
		};

		int multiSample = comboMultisample_.GetSelectedIndex();
		config->SetMultiSampleType(mapIndexSample[multiSample]);
	}

	int fpsType = DnhConfiguration::FPS_NORMAL;
	if (SendDlgItemMessage(hWnd_, IDC_RADIO_FPS_1, BM_GETCHECK, 0, 0))
		fpsType = DnhConfiguration::FPS_NORMAL;
	else if (SendDlgItemMessage(hWnd_, IDC_RADIO_FPS_2, BM_GETCHECK, 0, 0))
		fpsType = DnhConfiguration::FPS_1_2;
	else if (SendDlgItemMessage(hWnd_, IDC_RADIO_FPS_3, BM_GETCHECK, 0, 0))
		fpsType = DnhConfiguration::FPS_1_3;
	else if (SendDlgItemMessage(hWnd_, IDC_RADIO_FPS_AUTO, BM_GETCHECK, 0, 0))
		fpsType = DnhConfiguration::FPS_AUTO;
	config->SetFpsType(fpsType);

	int modeColor = DirectGraphicsConfig::COLOR_MODE_32BIT;
	if (SendDlgItemMessage(hWnd_, IDC_RADIO_COLOR_32, BM_GETCHECK, 0, 0))
		modeColor = DirectGraphicsConfig::COLOR_MODE_32BIT;
	else if (SendDlgItemMessage(hWnd_, IDC_RADIO_COLOR_16, BM_GETCHECK, 0, 0))
		modeColor = DirectGraphicsConfig::COLOR_MODE_16BIT;
	config->SetColorMode(modeColor);

	config->SetEnableVSync(SendDlgItemMessage(hWnd_, IDC_VSYNC, BM_GETCHECK, 0, 0) == BST_CHECKED);
	config->SetEnableRef(SendDlgItemMessage(hWnd_, IDC_REFERENCERASTERIZER, BM_GETCHECK, 0, 0) == BST_CHECKED);
	config->bPseudoFullscreen_ = SendDlgItemMessage(hWnd_, IDC_PSEUDOFULLSCREEN, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

/**********************************************************
//KeyPanel
**********************************************************/
KeyPanel::KeyPanel() {
}
KeyPanel::~KeyPanel() {

}
bool KeyPanel::Initialize(HWND hParent) {
	hWnd_ = ::CreateDialog((HINSTANCE)GetWindowLong(NULL, GWL_HINSTANCE),
		MAKEINTRESOURCE(IDD_PANEL_KEY),
		hParent, (DLGPROC)_StaticWindowProcedure);
	Attach(hWnd_);

	comboPadIndex_.Attach(::GetDlgItem(hWnd_, IDC_COMBO_PADINDEX));

	HWND hListKey = ::GetDlgItem(hWnd_, IDC_LIST_KEY);
	DWORD dwStyle = ListView_GetExtendedListViewStyle(hListKey);
	dwStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
	ListView_SetExtendedListViewStyle(hListKey, dwStyle);
//	HIMAGELIST hImageList = ImageList_Create(32 , 22 , ILC_COLOR4 |ILC_MASK , 2 , 1);
//	ListView_SetImageList(hList, hImageList, LVSIL_SMALL) ;

	viewKey_ = new KeyListView();
	viewKey_->Attach(hListKey);
	viewKey_->AddColumn(120, COL_ACTION, L"Action");
	viewKey_->AddColumn(100, COL_KEY_ASSIGN, L"Keyboard");
	viewKey_->AddColumn(80, COL_PAD_ASSIGN, L"Pad");

	std::map<int, std::wstring> mapViewText;
	mapViewText[EDirectInput::KEY_LEFT] = L"Left";
	mapViewText[EDirectInput::KEY_RIGHT] = L"Right";
	mapViewText[EDirectInput::KEY_UP] = L"Up";
	mapViewText[EDirectInput::KEY_DOWN] = L"Down";
	mapViewText[EDirectInput::KEY_OK] = L"Accept";
	mapViewText[EDirectInput::KEY_CANCEL] = L"Cancel";
	mapViewText[EDirectInput::KEY_SHOT] = L"Shot";
	mapViewText[EDirectInput::KEY_BOMB] = L"Bomb";
	mapViewText[EDirectInput::KEY_SLOWMOVE] = L"Focus";
	mapViewText[EDirectInput::KEY_USER1] = L"User 1";
	mapViewText[EDirectInput::KEY_USER2] = L"User 2";
	mapViewText[EDirectInput::KEY_PAUSE] = L"Pause";
	for (int iView = 0; iView < mapViewText.size(); iView++) {
		std::wstring text = mapViewText[iView];
		viewKey_->SetText(iView, COL_ACTION, text);
		_UpdateText(iView);
	}

	return true;
}
bool KeyPanel::StartUp() {
	int padDeviceTextWidth = comboPadIndex_.GetClientWidth();
	EDirectInput* input = EDirectInput::GetInstance();
	int padCount = input->GetPadDeviceCount();
	for (int iPad = 0; iPad < padCount; iPad++) {
		DIDEVICEINSTANCE info = input->GetPadDeviceInformation(iPad);
		std::wstring strPad = StringUtility::Format(L"%d : %s [%s]", iPad + 1, info.tszInstanceName, info.tszProductName);
		comboPadIndex_.AddString(strPad);

		int textCount = StringUtility::CountAsciiSizeCharacter(strPad);
		//padDeviceTextWidth = max(padDeviceTextWidth, textCount * 10);
	}
	if (padCount == 0) {
		comboPadIndex_.AddString(L"(none)");
		comboPadIndex_.SetWindowEnable(false);
	}
	SetWindowPos(comboPadIndex_.GetWindowHandle(), NULL, 0, 0,
		padDeviceTextWidth, 200, SWP_NOMOVE);

	DnhConfiguration* config = DnhConfiguration::GetInstance();
	int padIndex = config->GetPadIndex();
	padIndex = std::min(padIndex, padCount - 1);
	padIndex = std::max(padIndex, 0);
	comboPadIndex_.SetSelectedIndex(padIndex);

	return true;
}

LRESULT KeyPanel::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return WPanel::_WindowProcedure(hWnd, uMsg, wParam, lParam);
}
void KeyPanel::_UpdateText(int row) {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	ref_count_ptr<VirtualKey> vk = config->GetVirtualKey(row);
	if (vk == NULL)return;

	int keyCode = vk->GetKeyCode();
	std::wstring strKey = listKeyCode_.GetCodeText(keyCode);
	viewKey_->SetText(row, COL_KEY_ASSIGN, strKey);

	int padButton = vk->GetPadButton();
	std::wstring strPad = StringUtility::Format(L"PAD %02d", padButton);
	viewKey_->SetText(row, COL_PAD_ASSIGN, strPad);

}
void KeyPanel::UpdateKeyAssign() {
	int row = viewKey_->GetSelectedRow();
	if (row < 0)return;

	DnhConfiguration* config = DnhConfiguration::GetInstance();
	ref_count_ptr<VirtualKey> vk = config->GetVirtualKey(row);
	EDirectInput* input = EDirectInput::GetInstance();

	bool bChange = false;
	int pushKeyCode = -1;
	std::vector<int>& listValidCode = listKeyCode_.GetValidCodeList();
	for (int iCode = 0; iCode < listValidCode.size(); iCode++) {
		int code = listValidCode[iCode];
		int state = input->GetKeyState(code);
		if (state != KEY_PUSH)continue;

		pushKeyCode = code;
		bChange = true;
		break;
	}
	if (pushKeyCode >= 0)
		vk->SetKeyCode(pushKeyCode);

	int pushPadIndex = comboPadIndex_.GetSelectedIndex();
	int pushPadButton = -1;
	for (int iButton = 0; iButton < DirectInput::MAX_PAD_BUTTON; iButton++) {
		int state = input->GetPadState(pushPadIndex, iButton);
		if (state != KEY_PUSH)continue;

		pushPadButton = iButton;
		bChange = true;
		break;
	}
	if (pushPadButton >= 0) {
		vk->SetPadIndex(pushPadIndex);
		vk->SetPadButton(pushPadButton);
	}

	if (bChange) {
		_UpdateText(row);

		int nextRow = row + 1;
		if (pushKeyCode >= 0 && false) {
			if (pushKeyCode == DIK_UP)
				nextRow++;
			if (pushKeyCode == DIK_DOWN)
				nextRow--;
		}
		if (nextRow >= viewKey_->GetRowCount())
			nextRow = 0;
		viewKey_->SetSelectedRow(nextRow);
	}
}

void KeyPanel::ReadConfiguration() {
}
void KeyPanel::WriteConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	int padIndex = comboPadIndex_.GetSelectedIndex();
	config->SetPadIndex(padIndex);
}

//KeyPanel::KeyListView
LRESULT KeyPanel::KeyListView::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_KEYDOWN://�L�[���͂𖳎�
		return FALSE;
	}
	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);
}

/**********************************************************
//OptionPanel
**********************************************************/
OptionPanel::OptionPanel() {
}
OptionPanel::~OptionPanel() {

}
bool OptionPanel::Initialize(HWND hParent) {
	hWnd_ = ::CreateDialog((HINSTANCE)GetWindowLong(NULL, GWL_HINSTANCE),
		MAKEINTRESOURCE(IDD_PANEL_OPTION),
		hParent, (DLGPROC)_StaticWindowProcedure);
	Attach(hWnd_);

	HWND hListOption = ::GetDlgItem(hWnd_, IDC_LIST_OPTION);
	DWORD dwStyle = ListView_GetExtendedListViewStyle(hListOption);
	dwStyle |= LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
	ListView_SetExtendedListViewStyle(hListOption, dwStyle);

	viewOption_ = new WListView();
	viewOption_->Attach(hListOption);
	viewOption_->AddColumn(330, 0, L"Option");
	viewOption_->SetText(ROW_LOG_WINDOW, 0, L"Show LogWindow on startup");
	viewOption_->SetText(ROW_LOG_FILE, 0, L"Save LogWindow logs to file");
	viewOption_->SetText(ROW_MOUSE_UNVISIBLE, 0, L"Hide mouse cursor");

	HWND hPathBox = ::GetDlgItem(hWnd_, IDC_TEXT_EXE_PATH);

	exePath_ = new WEditBox();
	exePath_->Attach(hPathBox);

	//executorPath_ = DNH_EXE_DEFAULT;

	return true;
}
LRESULT OptionPanel::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_COMMAND:
	{
		switch (wParam & 0xffff) {
		case IDC_BUTTON_EXE_PATH_BROWSE:
		{
			wchar_t filename[MAX_PATH];
			ZeroMemory(filename, sizeof(filename));

			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWnd_;
			ofn.lpstrFilter = L"Executable\0*.exe\0";
			ofn.lpstrFile = filename;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrTitle = L"Select game executable";
			ofn.lpstrInitialDir = PathProperty::GetModuleDirectory().c_str();
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

			if (GetOpenFileName(&ofn)) {
				std::wstring fileNameStr = filename;
				stdfs::path fileNameP = fileNameStr;
				std::wstring dir = PathProperty::ReplaceYenToSlash(fileNameP.parent_path()) + L"/";
				std::wstring moduleDir = PathProperty::GetModuleDirectory();
				if (dir == moduleDir) fileNameStr = fileNameP.filename();

				exePath_->SetText(fileNameStr);
			}

			break;
		}
		}
		break;
	}
	}
	return WPanel::_WindowProcedure(hWnd, uMsg, wParam, lParam);
}
void OptionPanel::ReadConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	HWND hListOption = viewOption_->GetWindowHandle();
	if (config->IsLogWindow())
		ListView_SetItemState(hListOption, ROW_LOG_WINDOW, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
	if (config->IsLogFile())
		ListView_SetItemState(hListOption, ROW_LOG_FILE, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
	if (!config->IsMouseVisible())
		ListView_SetItemState(hListOption, ROW_MOUSE_UNVISIBLE, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);

	std::wstring pathExe = config->GetExePath();
	exePath_->SetText(pathExe);
}
void OptionPanel::WriteConfiguration() {
	DnhConfiguration* config = DnhConfiguration::GetInstance();
	HWND hListOption = viewOption_->GetWindowHandle();
	config->SetLogWindow(ListView_GetCheckState(hListOption, ROW_LOG_WINDOW) ? true : false);
	config->SetLogFile(ListView_GetCheckState(hListOption, ROW_LOG_FILE) ? true : false);
	config->SetMouseVisible(ListView_GetCheckState(hListOption, ROW_MOUSE_UNVISIBLE) ? false : true);
	{
		std::wstring str = exePath_->GetText();
		config->SetExePath(str.size() > 0U ? str : DNH_EXE_DEFAULT);
	}
}
