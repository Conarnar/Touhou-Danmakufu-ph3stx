#include "source/GcLib/pch.h"

#include "DnhCommon.hpp"
#include "DnhGcLibImpl.hpp"

#if defined(DNH_PROJ_EXECUTOR)
/**********************************************************
//ScriptInformation
**********************************************************/
const std::wstring ScriptInformation::DEFAULT = L"DEFAULT";

ref_count_ptr<ScriptInformation> ScriptInformation::CreateScriptInformation(std::wstring pathScript, bool bNeedHeader) {
	ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(pathScript);
	if (reader == nullptr || !reader->Open()) {
		Logger::WriteTop(ErrorUtility::GetFileNotFoundErrorMessage(pathScript));
		return nullptr;
	}

	std::string source = "";
	size_t size = reader->GetFileSize();
	source.resize(size);
	reader->Read(&source[0], size);

	ref_count_ptr<ScriptInformation> res = CreateScriptInformation(pathScript, L"", source, bNeedHeader);
	return res;
}
ref_count_ptr<ScriptInformation> ScriptInformation::CreateScriptInformation(std::wstring pathScript, std::wstring pathArchive, std::string source, bool bNeedHeader) {
	ref_count_ptr<ScriptInformation> res = nullptr;

	Scanner scanner(source);
	int encoding = scanner.GetEncoding();
	try {
		bool bScript = false;
		int type = TYPE_SINGLE;
		if (!bNeedHeader) {
			type = TYPE_UNKNOWN;
			bScript = true;
		}
		std::wstring idScript = L"";
		std::wstring title = L"";
		std::wstring text = L"";
		std::wstring pathImage = L"";
		std::wstring pathSystem = DEFAULT;
		std::wstring pathBackground = DEFAULT;
		std::wstring pathBGM = DEFAULT;
		std::vector<std::wstring> listPlayer;
		std::wstring replayName = L"";

		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			if (tok.GetType() == Token::TK_EOF)//Eof�̎��ʎq��������t�@�C���̒����I��
			{
				break;
			}
			else if (tok.GetType() == Token::TK_SHARP) {
				tok = scanner.Next();
				std::wstring element = tok.GetElement();
				bool bShiftJisDanmakufu = false;
				if (encoding == Encoding::UTF8) {
					int start = tok.GetStartPointer();
					int end = tok.GetEndPointer();

					{
						std::wstring wThDmf = L"�����e����";
						std::string unicodeThDmf = StringUtility::ConvertWideToMulti(wThDmf);
						bShiftJisDanmakufu = scanner.CompareMemory(start, end, unicodeThDmf.data());
					}
				}

				if (element == L"�����e����" || element == L"TouhouDanmakufu" || bShiftJisDanmakufu) {
					bScript = true;
					if (scanner.Next().GetType() != Token::TK_OPENB)continue;
					tok = scanner.Next();
					std::wstring strType = tok.GetElement();
					if (scanner.Next().GetType() != Token::TK_CLOSEB)throw gstd::wexception();

					if (strType == L"Single")type = TYPE_SINGLE;
					else if (strType == L"Plural")type = TYPE_PLURAL;
					else if (strType == L"Stage")type = TYPE_STAGE;
					else if (strType == L"Package")type = TYPE_PACKAGE;
					else if (strType == L"Player")type = TYPE_PLAYER;
					else if (!bNeedHeader)throw gstd::wexception();
				}
				else if (element == L"ID") {
					idScript = _GetString(scanner);
				}
				else if (element == L"Title") {
					title = _GetString(scanner);
				}
				else if (element == L"Text") {
					text = _GetString(scanner);
				}
				else if (element == L"Image") {
					pathImage = _GetString(scanner);
				}
				else if (element == L"System") {
					pathSystem = _GetString(scanner);
				}
				else if (element == L"Background") {
					pathBackground = _GetString(scanner);
				}
				else if (element == L"BGM") {
					pathBGM = _GetString(scanner);
				}
				else if (element == L"Player") {
					listPlayer = _GetStringList(scanner);
				}
				else if (element == L"ReplayName") {
					replayName = _GetString(scanner);
				}
			}
		}

		if (bScript) {
			//ID���Ȃ������ꍇ�͂��傤���Ȃ��̂Ńt�@�C�����ɂ���B
			if (idScript.size() == 0)
				idScript = PathProperty::GetFileNameWithoutExtension(pathScript);

			if (replayName.size() == 0) {
				replayName = idScript;
				if (replayName.size() > 8)
					replayName = replayName.substr(0, 8);
			}

			if (pathImage.size() > 2) {
				if (pathImage[0] == L'.' &&
					(pathImage[1] == L'/' || pathImage[1] == L'\\')) {
					pathImage = pathImage.substr(2);
					pathImage = PathProperty::GetFileDirectory(pathScript) + pathImage;
				}
			}

			res = new ScriptInformation();
			res->SetScriptPath(pathScript);
			res->SetArchivePath(pathArchive);
			res->SetType(type);

			res->SetID(idScript);
			res->SetTitle(title);
			res->SetText(text);
			res->SetImagePath(pathImage);
			res->SetSystemPath(pathSystem);
			res->SetBackgroundPath(pathBackground);
			res->SetBgmPath(pathBGM);
			res->SetPlayerList(listPlayer);
			res->SetReplayName(replayName);
		}
	}
	catch (...) {
		res = nullptr;
	}

	return res;
}
bool ScriptInformation::IsExcludeExtention(std::wstring ext) {
	bool res = false;
	if (ext == L".dat" || ext == L".mid" || ext == L".wav" || ext == L".mp3" || ext == L".ogg" ||
		ext == L".bmp" || ext == L".png" || ext == L".jpg" || ext == L".jpeg" ||
		ext == L".mqo" || ext == L".elem") {
		res = true;
	}
	return res;
}
std::wstring ScriptInformation::_GetString(Scanner& scanner) {
	std::wstring res = DEFAULT;
	scanner.CheckType(scanner.Next(), Token::TK_OPENB);
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::TK_STRING) {
		res = tok.GetString();
	}
	scanner.CheckType(scanner.Next(), Token::TK_CLOSEB);
	return res;
}
std::vector<std::wstring> ScriptInformation::_GetStringList(Scanner& scanner) {
	std::vector<std::wstring> res;
	scanner.CheckType(scanner.Next(), Token::TK_OPENB);
	while (true) {
		Token& tok = scanner.Next();
		int type = tok.GetType();
		if (type == Token::TK_CLOSEB)break;
		else if (type == Token::TK_STRING) {
			std::wstring wstr = tok.GetString();
			res.push_back(wstr);
		}
	}
	return res;
}
std::vector<ref_count_ptr<ScriptInformation> > ScriptInformation::CreatePlayerScriptInformationList() {
	std::vector<ref_count_ptr<ScriptInformation> > res;
	std::vector<std::wstring> listPlayerPath = GetPlayerList();
	std::wstring dirInfo = PathProperty::GetFileDirectory(GetScriptPath());
	for (int iPath = 0; iPath < listPlayerPath.size(); iPath++) {
		std::wstring pathPlayer = listPlayerPath[iPath];
		std::wstring path = EPathProperty::ExtendRelativeToFull(dirInfo, pathPlayer);

		ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
		if (reader == nullptr || !reader->Open()) {
			Logger::WriteTop(ErrorUtility::GetFileNotFoundErrorMessage(path));
			continue;
		}

		std::string source = "";
		size_t size = reader->GetFileSize();
		source.resize(size);
		reader->Read(&source[0], size);

		ref_count_ptr<ScriptInformation> info =
			ScriptInformation::CreateScriptInformation(path, L"", source);
		if (info != nullptr && info->GetType() == ScriptInformation::TYPE_PLAYER) {
			res.push_back(info);
		}
	}
	return res;
}
std::vector<ref_count_ptr<ScriptInformation>> ScriptInformation::CreateScriptInformationList(std::wstring path, bool bNeedHeader) {
	std::vector<ref_count_ptr<ScriptInformation>> res;
	File file(path);
	if (!file.Open())return res;
	if (file.GetSize() < ArchiveFileHeader::MAGIC_LENGTH) return res;

	char header[ArchiveFileHeader::MAGIC_LENGTH];
	file.Read(&header, ArchiveFileHeader::MAGIC_LENGTH);
	if (memcmp(header, ArchiveEncryption::HEADER_ARCHIVEFILE, ArchiveFileHeader::MAGIC_LENGTH) == 0) {
		file.Close();

		ArchiveFile archive(path);
		if (!archive.Open())return res;

		std::multimap<std::wstring, ArchiveFileEntry::ptr> mapEntry = archive.GetEntryMap();
		for (auto itr = mapEntry.begin(); itr != mapEntry.end(); itr++) {
			//���炩�Ɋ֌W�Ȃ������Ȋg���q�͏��O
			ArchiveFileEntry::ptr entry = itr->second;
			std::wstring dir = PathProperty::GetFileDirectory(path);
			std::wstring tPath = dir + entry->directory + entry->name;

			std::wstring ext = PathProperty::GetFileExtension(entry->name);
			if (ScriptInformation::IsExcludeExtention(ext))continue;

			ref_count_ptr<gstd::ByteBuffer> buffer = ArchiveFile::CreateEntryBuffer(entry);
			std::string source = "";
			size_t size = buffer->GetSize();
			source.resize(size);
			buffer->Read(&source[0], size);

			ref_count_ptr<ScriptInformation> info = CreateScriptInformation(tPath, path, source, bNeedHeader);
			if (info) res.push_back(info);
		}
	}
	else {

		//���炩�Ɋ֌W�Ȃ������Ȋg���q�͏��O
		std::wstring ext = PathProperty::GetFileExtension(path);
		if (ScriptInformation::IsExcludeExtention(ext))return res;

		file.SetFilePointerBegin();
		std::string source = "";
		size_t size = file.GetSize();
		source.resize(size);
		file.Read(&source[0], size);

		ref_count_ptr<ScriptInformation> info = CreateScriptInformation(path, L"", source, bNeedHeader);
		if (info) res.push_back(info);
	}

	return res;
}
std::vector<ref_count_ptr<ScriptInformation>> ScriptInformation::FindPlayerScriptInformationList(std::wstring dir) {
	std::vector<ref_count_ptr<ScriptInformation>> res;
	WIN32_FIND_DATA data;
	HANDLE hFind;
	std::wstring findDir = dir + L"*.*";
	hFind = FindFirstFile(findDir.c_str(), &data);
	do {
		std::wstring name = data.cFileName;
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(name != L".." && name != L".")) {
			//�f�B���N�g��
			std::wstring tDir = dir + name;
			tDir += L"\\";

			std::vector<ref_count_ptr<ScriptInformation>> list = FindPlayerScriptInformationList(tDir);
			for (std::vector<ref_count_ptr<ScriptInformation>>::iterator itr = list.begin(); itr != list.end(); itr++) {
				res.push_back(*itr);
			}
			continue;
		}
		if (wcscmp(data.cFileName, L"..") == 0 || wcscmp(data.cFileName, L".") == 0)
			continue;

		//�t�@�C��
		std::wstring path = dir + name;

		//�X�N���v�g���
		std::vector<ref_count_ptr<ScriptInformation>> listInfo = CreateScriptInformationList(path, true);
		for (size_t iInfo = 0; iInfo < listInfo.size(); iInfo++) {
			ref_count_ptr<ScriptInformation> info = listInfo[iInfo];
			if (info != nullptr && info->GetType() == ScriptInformation::TYPE_PLAYER)
				res.push_back(info);
		}

	} while (FindNextFile(hFind, &data));
	FindClose(hFind);

	return res;
}
#endif

/**********************************************************
//ErrorDialog
**********************************************************/
HWND ErrorDialog::hWndParentStatic_ = nullptr;
ErrorDialog::ErrorDialog(HWND hParent) {
	hParent_ = hParent;
}
LRESULT ErrorDialog::_WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
	{
		_FinishMessageLoop();
		break;
	}
	case WM_CLOSE:
		DestroyWindow(hWnd_);
		break;
	case WM_KEYDOWN:
		if (wParam == VK_RETURN) {
			DestroyWindow(hWnd_);
		}
		break;
	case WM_COMMAND:
	{
		int param = wParam & 0xffff;
		if (param == button_.GetWindowId()) {
			DestroyWindow(hWnd_);
		}

		break;
	}
	case WM_SIZE:
	{
		RECT rect;
		::GetClientRect(hWnd_, &rect);
		int wx = rect.left;
		int wy = rect.top;
		int wWidth = rect.right - rect.left;
		int wHeight = rect.bottom - rect.top;

		RECT rcButton = button_.GetClientRect();
		int widthButton = rcButton.right - rcButton.left;
		int heightButton = rcButton.bottom - rcButton.top;
		button_.SetBounds(wWidth / 2 - widthButton / 2, wHeight - heightButton - 8, widthButton, heightButton);

		edit_.SetBounds(wx + 8, wy + 8, wWidth - 16, wHeight - heightButton - 24);

		break;
	}

	}
	return _CallPreviousWindowProcedure(hWnd, uMsg, wParam, lParam);
}
bool ErrorDialog::ShowModal(std::wstring msg) {
	HINSTANCE hInst = ::GetModuleHandle(NULL);
	std::wstring wName = L"ErrorWindow";

	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = (WNDPROC)WindowBase::_StaticWindowProcedure;
	wcex.hInstance = hInst;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = wName.c_str();
	wcex.hIconSm = NULL;
	RegisterClassEx(&wcex);

	hWnd_ = ::CreateWindow(wcex.lpszClassName,
		wName.c_str(),
		WS_OVERLAPPEDWINDOW,
		0, 0, 480, 320, hParent_, (HMENU)NULL, hInst, NULL);
	::ShowWindow(hWnd_, SW_HIDE);
	this->Attach(hWnd_);


	gstd::WEditBox::Style styleEdit;
	styleEdit.SetStyle(WS_CHILD | WS_VISIBLE |
		ES_MULTILINE | ES_READONLY | ES_AUTOHSCROLL | ES_AUTOVSCROLL |
		WS_HSCROLL | WS_VSCROLL);
	styleEdit.SetStyleEx(WS_EX_CLIENTEDGE);
	edit_.Create(hWnd_, styleEdit);
	edit_.SetText(msg);

	button_.Create(hWnd_);
	button_.SetText(L"OK");
	button_.SetBounds(0, 0, 88, 20);

	MoveWindowCenter();
	SetWindowVisible(true);
	_RunMessageLoop();
	return true;
}

/**********************************************************
//DnhConfiguration
**********************************************************/
DnhConfiguration::DnhConfiguration() {
	modeScreen_ = DirectGraphics::SCREENMODE_WINDOW;
	modeColor_ = DirectGraphicsConfig::COLOR_MODE_32BIT;
	sizeWindow_ = WINDOW_SIZE_640x480;
	fpsType_ = FPS_NORMAL;
	fastModeSpeed_ = 20;

	bVSync_ = true;
	referenceRasterizer_ = false;
	bPseudoFullscreen_ = true;
	multiSamples_ = D3DMULTISAMPLE_NONE;

	pathExeLaunch_ = DNH_EXE_DEFAULT;

	//�L�[�o�^
	padIndex_ = 0;
	mapKey_[EDirectInput::KEY_LEFT] = new VirtualKey(DIK_LEFT, 0, 0);//�L�[�{�[�h�u���v�ƃW���C�p�b�h�u���v��o�^
	mapKey_[EDirectInput::KEY_RIGHT] = new VirtualKey(DIK_RIGHT, 0, 1);//�L�[�{�[�h�u���v�ƃW���C�p�b�h�u���v��o�^
	mapKey_[EDirectInput::KEY_UP] = new VirtualKey(DIK_UP, 0, 2);//�L�[�{�[�h�u���v�ƃW���C�p�b�h�u���v��o�^
	mapKey_[EDirectInput::KEY_DOWN] = new VirtualKey(DIK_DOWN, 0, 3);	//�L�[�{�[�h�u���v�ƃW���C�p�b�h�u���v��o�^

	mapKey_[EDirectInput::KEY_OK] = new VirtualKey(DIK_Z, 0, 5);
	mapKey_[EDirectInput::KEY_CANCEL] = new VirtualKey(DIK_X, 0, 6);

	mapKey_[EDirectInput::KEY_SHOT] = new VirtualKey(DIK_Z, 0, 5);
	mapKey_[EDirectInput::KEY_BOMB] = new VirtualKey(DIK_X, 0, 6);
	mapKey_[EDirectInput::KEY_SLOWMOVE] = new VirtualKey(DIK_LSHIFT, 0, 7);
	mapKey_[EDirectInput::KEY_USER1] = new VirtualKey(DIK_C, 0, 8);
	mapKey_[EDirectInput::KEY_USER2] = new VirtualKey(DIK_V, 0, 9);

	mapKey_[EDirectInput::KEY_PAUSE] = new VirtualKey(DIK_ESCAPE, 0, 10);

	bLogWindow_ = false;
	bLogFile_ = false;
	bMouseVisible_ = true;

	screenWidth_ = 640;
	screenHeight_ = 480;

	LoadConfigFile();
#if defined(DNH_PROJ_EXECUTOR)
	_LoadDefintionFile();
#endif
}
DnhConfiguration::~DnhConfiguration() {}

#if defined(DNH_PROJ_EXECUTOR)
bool DnhConfiguration::_LoadDefintionFile() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"th_dnh.def";
	PropertyFile prop;
	if (!prop.Load(path))return false;

	pathPackageScript_ = prop.GetString(L"package.script.main", L"");
	if (pathPackageScript_.size() > 0) {
		pathPackageScript_ = PathProperty::GetModuleDirectory() + pathPackageScript_;
		pathPackageScript_ = PathProperty::ReplaceYenToSlash(pathPackageScript_);
	}

	windowTitle_ = prop.GetString(L"window.title", L"");

	screenWidth_ = prop.GetInteger(L"screen.width", 640);
	screenWidth_ = std::max(screenWidth_, 320);
	screenWidth_ = std::min(screenWidth_, 1920);

	screenHeight_ = prop.GetInteger(L"screen.height", 480);
	screenHeight_ = std::max(screenHeight_, 240);
	screenHeight_ = std::min(screenHeight_, 1200);

	fastModeSpeed_ = prop.GetInteger(L"skip.rate", 20);
	fastModeSpeed_ = std::max(fastModeSpeed_, 1);
	fastModeSpeed_ = std::min(fastModeSpeed_, 50);

	return true;
}
#endif

bool DnhConfiguration::LoadConfigFile() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"config.dat";
	RecordBuffer record;
	bool res = record.ReadFromFile(path);
	if (!res)return false;

	{
		size_t version;
		record.GetRecord<size_t>("version", version);
		if (version != DNH_VERSION_NUM) return false;
	}

	record.GetRecord<int>("modeScreen", modeScreen_);
	record.GetRecord<int>("sizeWindow", sizeWindow_);
	record.GetRecord<int>("fpsType", fpsType_);

	record.GetRecord<int>("modeColor", modeColor_);
	record.GetRecord<bool>("bVSync", bVSync_);
	record.GetRecord<bool>("bDeviceREF", referenceRasterizer_);
	record.GetRecord<bool>("bPseudoFullscreen", bPseudoFullscreen_);

	record.GetRecord<DWORD>("typeMultiSamples", (DWORD&)multiSamples_);

	pathExeLaunch_ = record.GetRecordAsStringW("pathLaunch");
	if (pathExeLaunch_.size() == 0) pathExeLaunch_ = DNH_EXE_DEFAULT;

	if (record.IsExists("padIndex"))
		padIndex_ = record.GetRecordAsInteger("padIndex");

	ByteBuffer bufKey;
	int bufKeySize = record.GetRecordAsInteger("mapKey_size");
	bufKey.SetSize(bufKeySize);
	{
		std::string key = "mapKey";
		record.GetRecord(key, bufKey.GetPointer(), bufKey.GetSize());
	}

	size_t mapKeyCount = bufKey.ReadValue<size_t>();
	if (mapKeyCount == mapKey_.size()) {
		for (size_t iKey = 0; iKey < mapKeyCount; iKey++) {
			int id = bufKey.ReadInteger();
			int keyCode = bufKey.ReadInteger();
			int padIndex = bufKey.ReadInteger();
			int padButton = bufKey.ReadInteger();

			mapKey_[id] = new VirtualKey(keyCode, padIndex, padButton);
		}
	}

	bLogWindow_ = record.GetRecordAsBoolean("bLogWindow");
	bLogFile_ = record.GetRecordAsBoolean("bLogFile");
	if (record.IsExists("bMouseVisible"))
		bMouseVisible_ = record.GetRecordAsBoolean("bMouseVisible");

	return res;
}
bool DnhConfiguration::SaveConfigFile() {
	std::wstring path = PathProperty::GetModuleDirectory() + L"config.dat";
	RecordBuffer record;
	record.SetRecordAsInteger("version", DNH_VERSION_NUM);

	record.SetRecordAsInteger("modeScreen", modeScreen_);
	record.SetRecordAsInteger("sizeWindow", sizeWindow_);
	record.SetRecordAsInteger("fpsType", fpsType_);

	record.SetRecordAsInteger("modeColor", modeColor_);
	record.SetRecordAsBoolean("bVSync", bVSync_);
	record.SetRecordAsBoolean("bDeviceREF", referenceRasterizer_);
	record.SetRecordAsBoolean("bPseudoFullscreen", bPseudoFullscreen_);
	record.SetRecord<DWORD>("typeMultiSamples", (DWORD&)multiSamples_);

	record.SetRecordAsStringW("pathLaunch", pathExeLaunch_);

	record.SetRecordAsInteger("padIndex", padIndex_);
	ByteBuffer bufKey;
	bufKey.WriteValue(mapKey_.size());
	std::map<int, ref_count_ptr<VirtualKey>>::iterator itrKey = mapKey_.begin();
	for (; itrKey != mapKey_.end(); itrKey++) {
		int id = itrKey->first;
		ref_count_ptr<VirtualKey> vk = itrKey->second;

		bufKey.WriteInteger(id);
		bufKey.WriteInteger(vk->GetKeyCode());
		bufKey.WriteInteger(padIndex_);
		bufKey.WriteInteger(vk->GetPadButton());
	}
	record.SetRecordAsInteger("mapKey_size", bufKey.GetSize());
	{
		std::string key = "mapKey";
		record.SetRecord(key, bufKey.GetPointer(), bufKey.GetSize());
	}

	record.SetRecordAsBoolean("bLogWindow", bLogWindow_);
	record.SetRecordAsBoolean("bLogFile", bLogFile_);
	record.SetRecordAsBoolean("bMouseVisible", bMouseVisible_);

	record.WriteToFile(path);
	return true;
}
ref_count_ptr<VirtualKey> DnhConfiguration::GetVirtualKey(int id) {
	auto itr = mapKey_.find(id);
	if (itr == mapKey_.end()) return nullptr;
	return itr->second;
}