#include "source/GcLib/pch.h"

#include "StgStageController.hpp"
#include "StgSystem.hpp"

/**********************************************************
//StgStageController
**********************************************************/
StgStageController::StgStageController(StgSystemController* systemController) {
	systemController_ = systemController;
	infoSystem_ = systemController_->GetSystemInformation();

	scriptManager_ = nullptr;
	enemyManager_ = nullptr;
	shotManager_ = nullptr;
	itemManager_ = nullptr;
	intersectionManager_ = nullptr;
}
StgStageController::~StgStageController() {
	objectManagerMain_ = nullptr;
	scriptManager_ = nullptr;
	ptr_delete(enemyManager_);
	ptr_delete(shotManager_);
	ptr_delete(itemManager_);
	ptr_delete(intersectionManager_);
}
void StgStageController::Initialize(ref_count_ptr<StgStageStartData> startData) {
	//FPU������
	Math::InitializeFPU();

	//�L�[������
	EDirectInput* input = EDirectInput::GetInstance();
	input->ClearKeyState();

	//3D�J����
	DirectGraphics* graphics = DirectGraphics::GetBase();
	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	camera3D->Reset();
	camera3D->SetPerspectiveWidth(384);
	camera3D->SetPerspectiveHeight(448);
	camera3D->SetPerspectiveClip(10, 2000);
	camera3D->thisProjectionChanged_ = true;

	//2D�J����
	ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();
	camera2D->Reset();

	ref_count_ptr<StgStageInformation> infoStage = startData->GetStageInformation();
	ref_count_ptr<ReplayInformation::StageData> replayStageData = startData->GetStageReplayData();
	ref_count_ptr<StgStageInformation> prevStageData = startData->GetPrevStageInformation();
	ref_count_ptr<StgPlayerInformation> prevPlayerInfo = startData->GetPrevPlayerInformation();

	infoStage_ = infoStage;
	infoStage_->SetReplay(replayStageData != nullptr);

	//���v���C�L�[�ݒ�
	int replayState = infoStage_->IsReplay() ? KeyReplayManager::STATE_REPLAY : KeyReplayManager::STATE_RECORD;
	keyReplayManager_ = new KeyReplayManager(EDirectInput::GetInstance());
	keyReplayManager_->SetManageState(replayState);
	keyReplayManager_->AddTarget(EDirectInput::KEY_LEFT);
	keyReplayManager_->AddTarget(EDirectInput::KEY_RIGHT);
	keyReplayManager_->AddTarget(EDirectInput::KEY_UP);
	keyReplayManager_->AddTarget(EDirectInput::KEY_DOWN);
	keyReplayManager_->AddTarget(EDirectInput::KEY_SHOT);
	keyReplayManager_->AddTarget(EDirectInput::KEY_BOMB);
	keyReplayManager_->AddTarget(EDirectInput::KEY_SLOWMOVE);
	keyReplayManager_->AddTarget(EDirectInput::KEY_USER1);
	keyReplayManager_->AddTarget(EDirectInput::KEY_USER2);
	keyReplayManager_->AddTarget(EDirectInput::KEY_OK);
	keyReplayManager_->AddTarget(EDirectInput::KEY_CANCEL);
	std::set<int> listReplayTargetKey = infoSystem_->GetReplayTargetKeyList();
	std::set<int>::iterator itrKey = listReplayTargetKey.begin();
	for (; itrKey != listReplayTargetKey.end(); itrKey++) {
		int id = *itrKey;
		keyReplayManager_->AddTarget(id);
	}

	if (replayStageData == nullptr)
		replayStageData = new ReplayInformation::StageData();
	infoStage_->SetReplayData(replayStageData);

	//�X�e�[�W�v�f
	infoSlow_ = new PseudoSlowInformation();
	ref_count_weak_ptr<PseudoSlowInformation> wPtr = infoSlow_;
	EFpsController::GetInstance()->AddFpsControlObject(wPtr);

	//�O�X�e�[�W��񔽉f
	if (prevStageData != nullptr) {
		infoStage_->SetScore(prevStageData->GetScore());
		infoStage_->SetGraze(prevStageData->GetGraze());
		infoStage_->SetPoint(prevStageData->GetPoint());
	}


	//���v���C�֘A(�X�N���v�g�������O)
	if (!infoStage_->IsReplay()) {
		//����
		uint32_t randSeed = infoStage_->GetRandProvider()->GetSeed();
		replayStageData->SetRandSeed(randSeed);

		ELogger* logger = ELogger::GetInstance();
		if (logger->IsWindowVisible())
			logger->SetInfo(11, L"Rand seed", StringUtility::Format(L"%08x", randSeed));

		//�X�e�[�W���
		ref_count_ptr<ScriptInformation> infoParent = systemController_->GetSystemInformation()->GetMainScriptInformation();
		ref_count_ptr<ScriptInformation> infoMain = infoStage_->GetMainScriptInformation();
		std::wstring pathParentScript = infoParent->GetScriptPath();
		std::wstring pathMainScript = infoMain->GetScriptPath();
		std::wstring filenameMainScript = PathProperty::GetFileName(pathMainScript);
		std::wstring pathMainScriptRelative = PathProperty::GetRelativeDirectory(pathParentScript, pathMainScript);

		replayStageData->SetMainScriptID(infoMain->GetID());
		replayStageData->SetMainScriptName(filenameMainScript);
		replayStageData->SetMainScriptRelativePath(pathMainScriptRelative);
		replayStageData->SetStartScore(infoStage_->GetScore());
		replayStageData->SetGraze(infoStage_->GetGraze());
		replayStageData->SetPoint(infoStage_->GetPoint());
	}
	else {
		//����
		uint32_t randSeed = replayStageData->GetRandSeed();
		infoStage_->GetRandProvider()->Initialize(randSeed);

		ELogger* logger = ELogger::GetInstance();
		if (logger->IsWindowVisible())
			logger->SetInfo(11, L"Rand seed", StringUtility::Format(L"%08x", randSeed));

		//���v���C�L�[
		keyReplayManager_->ReadRecord(*replayStageData->GetReplayKeyRecord());

		//�X�e�[�W���
		infoStage_->SetScore(replayStageData->GetStartScore());
		infoStage_->SetGraze(replayStageData->GetGraze());
		infoStage_->SetPoint(replayStageData->GetPoint());

		//���@�ݒ�
		prevPlayerInfo = new StgPlayerInformation();
		prevPlayerInfo->SetLife(replayStageData->GetPlayerLife());
		prevPlayerInfo->SetSpell(replayStageData->GetPlayerBombCount());
		prevPlayerInfo->SetPower(replayStageData->GetPlayerPower());
		prevPlayerInfo->SetRebirthFrame(replayStageData->GetPlayerRebirthFrame());
	}

	objectManagerMain_ = std::shared_ptr<StgStageScriptObjectManager>(new StgStageScriptObjectManager(this));
	//_objectManagerMain_ = objectManagerMain_;
	scriptManager_ = std::shared_ptr<StgStageScriptManager>(new StgStageScriptManager(this));
	enemyManager_ = new StgEnemyManager(this);
	shotManager_ = new StgShotManager(this);
	itemManager_ = new StgItemManager(this);
	intersectionManager_ = new StgIntersectionManager();
	pauseManager_ = new StgPauseScene(systemController_);

	//�p�b�P�[�W�X�N���v�g�̏ꍇ�́A�X�e�[�W�X�N���v�g�Ɗ֘A�t����
	StgPackageController* packageController = systemController_->GetPackageController();
	if (packageController != nullptr) {
		shared_ptr<ScriptManager> packageScriptManager = std::dynamic_pointer_cast<ScriptManager>(packageController->GetScriptManagerRef());
		shared_ptr<ScriptManager> stageScriptManager = std::dynamic_pointer_cast<ScriptManager>(scriptManager_);
		ScriptManager::AddRelativeScriptManagerMutual(packageScriptManager, stageScriptManager);
	}

	auto objectManager = scriptManager_->GetObjectManager();

	//���C���X�N���v�g���
	ref_count_ptr<ScriptInformation> infoMain = infoStage_->GetMainScriptInformation();
	std::wstring dirInfo = PathProperty::GetFileDirectory(infoMain->GetScriptPath());

	ELogger::WriteTop(StringUtility::Format(L"Main script: [%s]", infoMain->GetScriptPath().c_str()));

	//�V�X�e���X�N���v�g
	std::wstring pathSystemScript = infoMain->GetSystemPath();
	if (pathSystemScript == ScriptInformation::DEFAULT)
		pathSystemScript = EPathProperty::GetStgDefaultScriptDirectory() + L"Default_System.txt";
	if (pathSystemScript.size() > 0) {
		pathSystemScript = EPathProperty::ExtendRelativeToFull(dirInfo, pathSystemScript);
		ELogger::WriteTop(StringUtility::Format(L"System script: [%s]", pathSystemScript.c_str()));

		auto script = scriptManager_->LoadScript(pathSystemScript, StgStageScript::TYPE_SYSTEM);
		scriptManager_->StartScript(script);
	}

	//���@�X�N���v�g
	shared_ptr<StgPlayerObject> objPlayer = nullptr;
	ref_count_ptr<ScriptInformation> infoPlayer = infoStage_->GetPlayerScriptInformation();
	std::wstring pathPlayerScript = infoPlayer->GetScriptPath();

	if (pathPlayerScript.size() > 0) {
		ELogger::WriteTop(StringUtility::Format(L"Player script: [%s]", pathPlayerScript.c_str()));
		int idPlayer = objectManager->CreatePlayerObject();
		objPlayer = std::dynamic_pointer_cast<StgPlayerObject>(GetMainRenderObject(idPlayer));

		if (systemController_->GetSystemInformation()->IsPackageMode())
			objPlayer->SetEnableStateEnd(false);

		auto script = scriptManager_->LoadScript(pathPlayerScript, StgStageScript::TYPE_PLAYER);
		_SetupReplayTargetCommonDataArea(script->GetScriptID());

		shared_ptr<StgStagePlayerScript> scriptPlayer =
			std::dynamic_pointer_cast<StgStagePlayerScript>(script);
		objPlayer->SetScript(scriptPlayer.get());

		scriptManager_->SetPlayerScript(script);
		scriptManager_->StartScript(script);

		//�O�X�e�[�W��񔽉f
		if (prevPlayerInfo != nullptr)
			objPlayer->SetPlayerInforamtion(prevPlayerInfo);
	}
	if (objPlayer != nullptr)
		infoStage_->SetPlayerObjectInformation(objPlayer->GetPlayerInformation());

	//���C���X�N���v�g
	if (infoMain->GetType() == ScriptInformation::TYPE_SINGLE) {
		std::wstring pathMainScript = EPathProperty::GetSystemResourceDirectory() + L"script/System_SingleStage.txt";
		auto script = scriptManager_->LoadScript(pathMainScript, StgStageScript::TYPE_STAGE);
		scriptManager_->StartScript(script);
	}
	else if (infoMain->GetType() == ScriptInformation::TYPE_PLURAL) {
		std::wstring pathMainScript = EPathProperty::GetSystemResourceDirectory() + L"script/System_PluralStage.txt";
		auto script = scriptManager_->LoadScript(pathMainScript, StgStageScript::TYPE_STAGE);
		scriptManager_->StartScript(script);
	}
	else {
		std::wstring pathMainScript = infoMain->GetScriptPath();
		if (pathMainScript.size() > 0) {
			auto script = scriptManager_->LoadScript(pathMainScript, StgStageScript::TYPE_STAGE);
			_SetupReplayTargetCommonDataArea(script->GetScriptID());
			scriptManager_->StartScript(script);
		}
	}

	//�w�i�X�N���v�g
	std::wstring pathBack = infoMain->GetBackgroundPath();
	if (pathBack == ScriptInformation::DEFAULT)
		pathBack = L"";
	if (pathBack.size() > 0) {
		pathBack = EPathProperty::ExtendRelativeToFull(dirInfo, pathBack);
		ELogger::WriteTop(StringUtility::Format(L"Background script: [%s]", pathBack.c_str()));
		auto script = scriptManager_->LoadScript(pathBack, StgStageScript::TYPE_STAGE);
		scriptManager_->StartScript(script);
	}

	//�����Đ�
	std::wstring pathBGM = infoMain->GetBgmPath();
	if (pathBGM == ScriptInformation::DEFAULT)
		pathBGM = L"";
	if (pathBGM.size() > 0) {
		pathBGM = EPathProperty::ExtendRelativeToFull(dirInfo, pathBGM);
		ELogger::WriteTop(StringUtility::Format(L"BGM: [%s]", pathBGM.c_str()));
		ref_count_ptr<SoundPlayer> player = DirectSoundManager::GetBase()->GetPlayer(pathBGM);
		if (player != nullptr) {
			player->SetSoundDivision(SoundDivision::DIVISION_BGM);
			SoundPlayer::PlayStyle style;
			style.SetLoopEnable(true);
			player->Play(style);
		}
	}

	//���v���C�֘A(�X�N���v�g��������)
	if (!infoStage_->IsReplay()) {
		//���@���
		shared_ptr<StgPlayerObject> objPlayer = GetPlayerObject();
		if (objPlayer != nullptr) {
			replayStageData->SetPlayerLife(objPlayer->GetLife());
			replayStageData->SetPlayerBombCount(objPlayer->GetSpell());
			replayStageData->SetPlayerPower(objPlayer->GetPower());
			replayStageData->SetPlayerRebirthFrame(objPlayer->GetRebirthFrame());
		}
		std::wstring pathPlayerScript = infoPlayer->GetScriptPath();
		std::wstring filenamePlayerScript = PathProperty::GetFileName(pathPlayerScript);
		replayStageData->SetPlayerScriptFileName(filenamePlayerScript);
		replayStageData->SetPlayerScriptID(infoPlayer->GetID());
		replayStageData->SetPlayerScriptReplayName(infoPlayer->GetReplayName());
	}

	infoStage_->SetStageStartTime(timeGetTime());


}
void StgStageController::CloseScene() {
	ref_count_weak_ptr<PseudoSlowInformation> wPtr = infoSlow_;
	EFpsController::GetInstance()->RemoveFpsControlObject(wPtr);

	//���v���C
	if (!infoStage_->IsReplay()) {
		//�L�[
		ref_count_ptr<RecordBuffer> recKey = new RecordBuffer();
		keyReplayManager_->WriteRecord(*recKey.GetPointer());

		ref_count_ptr<ReplayInformation::StageData> replayStageData = infoStage_->GetReplayData();
		replayStageData->SetReplayKeyRecord(recKey);

		//�ŏI�t���[��
		int stageFrame = infoStage_->GetCurrentFrame();
		replayStageData->SetEndFrame(stageFrame);

		replayStageData->SetLastScore(infoStage_->GetScore());
	}
}
void StgStageController::_SetupReplayTargetCommonDataArea(int64_t idScript) {
	shared_ptr<StgStageScript> script =
		std::dynamic_pointer_cast<StgStageScript>(scriptManager_->GetScript(idScript));
	if (script == nullptr)return;

	const gstd::value& res = script->RequestEvent(StgStageScript::EV_REQUEST_REPLAY_TARGET_COMMON_AREA);
	if (!res.has_data())return;
	type_data::type_kind kindRes = res.get_type()->get_kind();
	if (kindRes != type_data::type_kind::tk_array)return;

	ref_count_ptr<ReplayInformation::StageData> replayStageData = infoStage_->GetReplayData();
	std::set<std::string> listArea;
	
	for (size_t iArray = 0; iArray < res.length_as_array(); iArray++) {
		const value& arrayValue = res.index_as_array(iArray);
		std::string area = StringUtility::ConvertWideToMulti(arrayValue.as_string());
		listArea.insert(area);
	}

	gstd::ref_count_ptr<ScriptCommonDataManager> scriptCommonDataManager = systemController_->GetCommonDataManager();
	if (!infoStage_->IsReplay()) {
		std::set<std::string>::iterator itrArea = listArea.begin();
		for (; itrArea != listArea.end(); itrArea++) {
			std::string area = (*itrArea);
			shared_ptr<ScriptCommonData> commonData = scriptCommonDataManager->GetData(area);
			replayStageData->SetCommonData(area, commonData);
		}
	}
	else {
		std::set<std::string>::iterator itrArea = listArea.begin();
		for (; itrArea != listArea.end(); itrArea++) {
			std::string area = (*itrArea);
			shared_ptr<ScriptCommonData> commonData = replayStageData->GetCommonData(area);
			scriptCommonDataManager->SetData(area, commonData);
		}
	}
}

void StgStageController::Work() {
	EDirectInput* input = EDirectInput::GetInstance();
	ref_count_ptr<StgSystemInformation> infoSystem = systemController_->GetSystemInformation();
	bool bPackageMode = infoSystem->IsPackageMode();

	bool bPermitRetryKey = !input->IsTargetKeyCode(DIK_BACK);
	if (!bPackageMode && bPermitRetryKey && input->GetKeyState(DIK_BACK) == KEY_PUSH) {
		//���g���C
		if (!infoStage_->IsReplay()) {
			ref_count_ptr<StgSystemInformation> infoSystem = systemController_->GetSystemInformation();
			infoSystem->SetRetry();
			return;
		}
	}

	bool bCurrentPause = infoStage_->IsPause();
	if (bPackageMode && bCurrentPause) {
		//�p�b�P�[�W���[�h�Œ�~���̏ꍇ�́A�p�b�P�[�W�X�N���v�g�ŏ�������
		return;
	}

	bool bPauseKey = (input->GetVirtualKeyState(EDirectInput::KEY_PAUSE) == KEY_PUSH);
	if (bPauseKey && !bPackageMode) {
		//��~�L�[����
		if (!bCurrentPause)
			pauseManager_->Start();
		else
			pauseManager_->Finish();
	}
	else {
		if (!bCurrentPause) {
			//���v���C�L�[�X�V
			keyReplayManager_->Update();

			//�X�N���v�g�����ŁA���@�A�G�A�e�̓��삪�s����B
			scriptManager_->Work(StgStageScript::TYPE_SYSTEM);
			scriptManager_->Work(StgStageScript::TYPE_STAGE);
			scriptManager_->Work(StgStageScript::TYPE_SHOT);
			scriptManager_->Work(StgStageScript::TYPE_ITEM);

			shared_ptr<StgPlayerObject> objPlayer = GetPlayerObject();
			if (objPlayer) objPlayer->Move(); //���@������Ɉړ�
			scriptManager_->Work(StgStageScript::TYPE_PLAYER);

			//�I�u�W�F�N�g���쏈��
			if (infoStage_->IsEnd())return;
			objectManagerMain_->WorkObject();

			enemyManager_->Work();
			shotManager_->Work();
			itemManager_->Work();

			//�����蔻�菈��
			enemyManager_->RegistIntersectionTarget();
			shotManager_->RegistIntersectionTarget();
			intersectionManager_->Work();

			if (!infoStage_->IsReplay()) {
				//���v���C�p���X�V
				int stageFrame = infoStage_->GetCurrentFrame();
				if (stageFrame % 60 == 0) {
					ref_count_ptr<ReplayInformation::StageData> replayStageData = infoStage_->GetReplayData();
					float framePerSecond = EFpsController::GetInstance()->GetCurrentFps();
					replayStageData->AddFramePerSecond(framePerSecond);
				}
			}

			infoStage_->AdvanceFrame();

		}
		else {
			//��~��
			pauseManager_->Work();
		}
	}

	ELogger* logger = ELogger::GetInstance();
	if (logger->IsWindowVisible()) {
		//���O�֘A
		logger->SetInfo(6, L"Shot count", StringUtility::Format(L"%d", shotManager_->GetShotCountAll()));
		logger->SetInfo(7, L"Enemy count", StringUtility::Format(L"%d", enemyManager_->GetEnemyCount()));
		logger->SetInfo(8, L"Item count", StringUtility::Format(L"%d", itemManager_->GetItemCount()));
	}
}
void StgStageController::Render() {
	bool bPause = infoStage_->IsPause();
	if (!bPause) {
		objectManagerMain_->RenderObject();

		if (infoStage_->IsReplay()) {
			//���v���C��
		}
	}
	else {
		//��~
		pauseManager_->Render();
	}
}
void StgStageController::RenderToTransitionTexture() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	TextureManager* textureManager = ETextureManager::GetInstance();
	shared_ptr<Texture> texture = textureManager->GetTexture(TextureManager::TARGET_TRANSITION);

	graphics->SetRenderTarget(texture);
	graphics->BeginScene(false, true);

	//objectManager->RenderObject();
	systemController_->RenderScriptObject();

	graphics->EndScene(false);
	graphics->SetRenderTarget(nullptr);

	/*
	if (false) {
		static int count = 0;
		std::wstring path = PathProperty::GetModuleDirectory() + StringUtility::FormatToWide("tempRT_transition\\temp_%04d.png", count);
		RECT rect = { 0, 0, 640, 480 };
		IDirect3DSurface9* pBackSurface = texture->GetD3DSurface();
		D3DXSaveSurfaceToFile(path.c_str(), D3DXIFF_PNG, pBackSurface, nullptr, &rect);
		count++;
	}
	*/
}
/*
shared_ptr<DxScriptObjectBase> StgStageController::GetMainRenderObject(int idObject) {
	return objectManagerMain_->GetObject(idObject);
}
shared_ptr<StgPlayerObject> StgStageController::GetPlayerObject() {
	int idPlayer = objectManagerMain_->GetPlayerObjectID();
	if (idPlayer == DxScript::ID_INVALID)return nullptr;
	return std::dynamic_pointer_cast<StgPlayerObject>(GetMainRenderObject(idPlayer));
}
*/

/**********************************************************
//StgStageInformation
**********************************************************/
StgStageInformation::StgStageInformation() {
	bEndStg_ = false;
	bPause_ = false;
	bReplay_ = false;
	frame_ = 0;
	stageIndex_ = 0;

	SetRect(&rcStgFrame_, 32, 16, 32 + 384, 16 + 448);
	SetStgFrameRect(rcStgFrame_);
	priMinStgFrame_ = 20;
	priMaxStgFrame_ = 80;
	priShotObject_ = 50;
	priItemObject_ = 60;
	priCameraFocusPermit_ = 69;
	SetRect(&rcShotAutoDeleteClip_, -64, -64, 64, 64);

	rand_ = std::make_shared<RandProvider>();
	rand_->Initialize(timeGetTime() ^ 0xf5682aeb);
	score_ = 0;
	graze_ = 0;
	point_ = 0;
	result_ = RESULT_UNKNOWN;

	timeStart_ = 0;
}
StgStageInformation::~StgStageInformation() {}
void StgStageInformation::SetStgFrameRect(RECT rect, bool bUpdateFocusResetValue) {
	rcStgFrame_ = rect;

	ref_count_ptr<D3DXVECTOR2> pos = new D3DXVECTOR2;
	pos->x = (rect.right - rect.left) / 2.0f;
	pos->y = (rect.bottom - rect.top) / 2.0f;

	if (bUpdateFocusResetValue) {
		DirectGraphics* graphics = DirectGraphics::GetBase();
		ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();
		camera2D->SetResetFocus(pos);
		camera2D->Reset();
	}
}

/**********************************************************
//PseudoSlowInformation
**********************************************************/
int PseudoSlowInformation::GetFps() {
	int fps = STANDARD_FPS;
	int target = TARGET_ALL;

	auto itrPlayer = mapDataPlayer_.find(target);
	if (itrPlayer != mapDataPlayer_.end())
		fps = std::min(fps, itrPlayer->second->GetFps());

	auto itrEnemy = mapDataEnemy_.find(target);
	if (itrEnemy != mapDataEnemy_.end())
		fps = std::min(fps, itrEnemy->second->GetFps());

	return fps;
}
bool PseudoSlowInformation::IsValidFrame(int target) {
	auto itr = mapValid_.find(target);
	bool res = itr == mapValid_.end() || itr->second;
	return res;
}
void PseudoSlowInformation::Next() {
	int fps = STANDARD_FPS;
	int target = TARGET_ALL;

	auto itrPlayer = mapDataPlayer_.find(target);
	if (itrPlayer != mapDataPlayer_.end())
		fps = std::min(fps, itrPlayer->second->GetFps());

	auto itrEnemy = mapDataEnemy_.find(target);
	if (itrEnemy != mapDataEnemy_.end())
		fps = std::min(fps, itrEnemy->second->GetFps());

	bool bValid = false;
	if (fps == STANDARD_FPS) {
		bValid = true;
	}
	else {
		current_ += fps;
		if (current_ >= STANDARD_FPS) {
			current_ %= STANDARD_FPS;
			bValid = true;
		}
	}

	mapValid_[target] = bValid;
}
void PseudoSlowInformation::AddSlow(int fps, int owner, int target) {
	fps = std::max(1, fps);
	fps = std::min(STANDARD_FPS, fps);
	ref_count_ptr<SlowData> data = new SlowData();
	data->SetFps(fps);
	switch (owner) {
	case OWNER_PLAYER:
		mapDataPlayer_[target] = data;
		break;
	case OWNER_ENEMY:
		mapDataEnemy_[target] = data;
		break;
	}
}
void PseudoSlowInformation::RemoveSlow(int owner, int target) {
	switch (owner) {
	case OWNER_PLAYER:
		mapDataPlayer_.erase(target);
		break;
	case OWNER_ENEMY:
		mapDataEnemy_.erase(target);
		break;
	}
}