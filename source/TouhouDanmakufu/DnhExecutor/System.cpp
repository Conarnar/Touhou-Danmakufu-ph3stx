#include "source/GcLib/pch.h"

#include "System.hpp"
#include "TitleScene.hpp"
#include "ScriptSelectScene.hpp"
#include "StgScene.hpp"

/**********************************************************
//SystemController
**********************************************************/
SystemController::SystemController() {
	sceneManager_ = new SceneManager();
	transitionManager_ = new TransitionManager();
	infoSystem_ = new SystemInformation();

	//�풓�^�X�N�o�^
	ETaskManager* taskManager = ETaskManager::GetInstance();
	shared_ptr<SystemResidentTask> task(new SystemResidentTask());
	taskManager->AddTask(task);
	taskManager->AddRenderFunction(TTaskFunction<SystemResidentTask>::Create(task, 
		&SystemResidentTask::RenderFps), SystemResidentTask::TASK_PRI_RENDER_FPS);
}
SystemController::~SystemController() {
	transitionManager_ = nullptr;
	sceneManager_ = nullptr;
}
void SystemController::Reset() {
	EFileManager* fileManager = EFileManager::GetInstance();
	fileManager->ResetArchiveFile();

	DnhConfiguration* config = DnhConfiguration::CreateInstance();
	std::wstring pathPackageScript = config->GetPackageScriptPath();
	if (pathPackageScript.size() == 0) {
		infoSystem_->UpdateFreePlayerScriptInformationList();
		sceneManager_->TransTitleScene();
	}
	else {
		ref_count_ptr<ScriptInformation> info = ScriptInformation::CreateScriptInformation(pathPackageScript, false);
		if (info == nullptr)
			ShowErrorDialog(ErrorUtility::GetFileNotFoundErrorMessage(pathPackageScript));
		else 
			sceneManager_->TransPackageScene(info, true);
	}
}
void SystemController::ClearTaskWithoutSystem() {
	std::set<const std::type_info*> listInfo;
	listInfo.insert(&typeid(SystemTransitionEffectTask));
	listInfo.insert(&typeid(SystemResidentTask));

	ETaskManager* taskManager = ETaskManager::GetInstance();
	taskManager->RemoveTaskWithoutTypeInfo(listInfo);
}
void SystemController::ShowErrorDialog(std::wstring msg) {
	HWND hParent = EDirectGraphics::GetInstance()->GetAttachedWindowHandle();
	ErrorDialog dialog(hParent);
	dialog.ShowModal(msg);
}

/**********************************************************
//SceneManager
**********************************************************/
SceneManager::SceneManager() {}
SceneManager::~SceneManager() {}
void SceneManager::TransTitleScene() {
	EDirectInput* input = EDirectInput::GetInstance();
	input->ClearKeyState();

	TransitionManager* transitionManager = SystemController::GetInstance()->GetTransitionManager();
	transitionManager->DoFadeOut();
	SystemController::GetInstance()->ClearTaskWithoutSystem();

	ETaskManager* taskManager = ETaskManager::GetInstance();
	shared_ptr<TitleScene> task(new TitleScene());
	taskManager->AddTask(task);
	taskManager->AddWorkFunction(TTaskFunction<TitleScene>::Create(task, 
		&TitleScene::Work), TitleScene::TASK_PRI_WORK);
	taskManager->AddRenderFunction(TTaskFunction<TitleScene>::Create(task, 
		&TitleScene::Render), TitleScene::TASK_PRI_RENDER);
}
void SceneManager::TransScriptSelectScene(int type) {
	EDirectInput* input = EDirectInput::GetInstance();
	input->ClearKeyState();

	TransitionManager* transitionManager = SystemController::GetInstance()->GetTransitionManager();
	transitionManager->DoFadeOut();
	SystemController::GetInstance()->ClearTaskWithoutSystem();

	ETaskManager* taskManager = ETaskManager::GetInstance();
	shared_ptr<ScriptSelectScene> task(new ScriptSelectScene());
	taskManager->AddTask(task);
	taskManager->AddWorkFunction(TTaskFunction<ScriptSelectScene>::Create(task, 
		&ScriptSelectScene::Work), ScriptSelectScene::TASK_PRI_WORK);
	taskManager->AddRenderFunction(TTaskFunction<ScriptSelectScene>::Create(task, 
		&ScriptSelectScene::Render), ScriptSelectScene::TASK_PRI_RENDER);

	ref_count_ptr<ScriptSelectModel> model = nullptr;
	if (type == ScriptSelectScene::TYPE_SINGLE ||
		type == ScriptSelectScene::TYPE_PLURAL ||
		type == ScriptSelectScene::TYPE_STAGE ||
		type == ScriptSelectScene::TYPE_PACKAGE ||
		type == ScriptSelectScene::TYPE_DIR ||
		type == ScriptSelectScene::TYPE_ALL) {
		std::wstring dir = EPathProperty::GetStgScriptRootDirectory();
		SystemInformation* systemInfo = SystemController::GetInstance()->GetSystemInformation();
		if (type == ScriptSelectScene::TYPE_DIR) {
			dir = systemInfo->GetLastScriptSearchDirectory();
		}
		ScriptSelectFileModel* fileModel = new ScriptSelectFileModel(type, dir);
		std::wstring pathWait = systemInfo->GetLastSelectedScriptPath();
		fileModel->SetWaitPath(pathWait);
		model = fileModel;
	}
	task->SetModel(model);
}
void SceneManager::TransScriptSelectScene_All() {
	TransScriptSelectScene(ScriptSelectScene::TYPE_ALL);
}
void SceneManager::TransScriptSelectScene_Single() {
	TransScriptSelectScene(ScriptSelectScene::TYPE_SINGLE);
}
void SceneManager::TransScriptSelectScene_Plural() {
	TransScriptSelectScene(ScriptSelectScene::TYPE_PLURAL);
}
void SceneManager::TransScriptSelectScene_Stage() {
	TransScriptSelectScene(ScriptSelectScene::TYPE_STAGE);
}
void SceneManager::TransScriptSelectScene_Package() {
	TransScriptSelectScene(ScriptSelectScene::TYPE_PACKAGE);
}
void SceneManager::TransScriptSelectScene_Directory() {
	TransScriptSelectScene(ScriptSelectScene::TYPE_DIR);
}
void SceneManager::TransScriptSelectScene_Last() {
	int type = SystemController::GetInstance()->GetSystemInformation()->GetLastSelectScriptSceneType();
	TransScriptSelectScene(type);
}
void SceneManager::TransStgScene(ref_count_ptr<ScriptInformation> infoMain, ref_count_ptr<ScriptInformation> infoPlayer, ref_count_ptr<ReplayInformation> infoReplay) {
	EDirectInput* input = EDirectInput::GetInstance();
	input->ClearKeyState();

	try {
		//STG�V�[��������
		ref_count_ptr<StgSystemInformation> infoStgSystem = new StgSystemInformation();
		infoStgSystem->SetMainScriptInformation(infoMain);
		shared_ptr<StgSystemController> task(new EStgSystemController());

		//STG�^�X�N������
		ETaskManager* taskManager = ETaskManager::GetInstance();
		task->Initialize(infoStgSystem);
		task->Start(infoPlayer, infoReplay);

		//�^�X�N�N���A
		TransitionManager* transitionManager = SystemController::GetInstance()->GetTransitionManager();
		transitionManager->DoFadeOut();
		SystemController::GetInstance()->ClearTaskWithoutSystem();

		//STG�^�X�N�o�^
		taskManager->AddTask(task);
		taskManager->AddWorkFunction(TTaskFunction<StgSystemController>::Create(task, 
			&StgSystemController::Work), StgSystemController::TASK_PRI_WORK);
		taskManager->AddRenderFunction(TTaskFunction<StgSystemController>::Create(task, 
			&StgSystemController::Render), StgSystemController::TASK_PRI_RENDER);
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(e.what());

		SystemController* system = SystemController::GetInstance();
		system->ShowErrorDialog(e.what());
		system->GetSceneManager()->TransScriptSelectScene_Last();
	}
}

void SceneManager::TransStgScene(ref_count_ptr<ScriptInformation> infoMain, ref_count_ptr<ReplayInformation> infoReplay) {
	try {
		std::wstring replayPlayerID = infoReplay->GetPlayerScriptID();
		std::wstring replayPlayerScriptFileName = infoReplay->GetPlayerScriptFileName();

		//���@������
		ref_count_ptr<ScriptInformation> infoPlayer;
		std::vector<ref_count_ptr<ScriptInformation> > listPlayer;
		std::vector<std::wstring> listPlayerPath = infoMain->GetPlayerList();
		if (listPlayerPath.size() == 0) {
			listPlayer =
				SystemController::GetInstance()->GetSystemInformation()->GetFreePlayerScriptInformationList();
		}
		else {
			listPlayer = infoMain->CreatePlayerScriptInformationList();
		}

		for (size_t iPlayer = 0; iPlayer < listPlayer.size(); iPlayer++) {
			ref_count_ptr<ScriptInformation> tInfo = listPlayer[iPlayer];
			if (tInfo->GetID() != replayPlayerID)continue;
			std::wstring tPlayerScriptFileName = PathProperty::GetFileName(tInfo->GetScriptPath());
			if (tPlayerScriptFileName != replayPlayerScriptFileName)continue;

			infoPlayer = tInfo;
			break;
		}

		if (infoPlayer == nullptr) {
			std::wstring log = StringUtility::Format(L"Player script not found: [%s]", replayPlayerScriptFileName.c_str());
			throw gstd::wexception(log.c_str());
		}

		TransStgScene(infoMain, infoPlayer, infoReplay);
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(e.what());

		SystemController* system = SystemController::GetInstance();
		system->ShowErrorDialog(e.what());
		system->GetSceneManager()->TransScriptSelectScene_Last();
	}

}
void SceneManager::TransPackageScene(ref_count_ptr<ScriptInformation> infoMain, bool bOnlyPackage) {
	EDirectInput* input = EDirectInput::GetInstance();
	input->ClearKeyState();

	try {
		//STG�V�[��������
		ref_count_ptr<StgSystemInformation> infoStgSystem = new StgSystemInformation();
		infoStgSystem->SetMainScriptInformation(infoMain);

		shared_ptr<StgSystemController> task = nullptr;
		if (!bOnlyPackage)
			task = std::make_shared<EStgSystemController>();
		else
			task = std::make_shared<PStgSystemController>();

		//STG�^�X�N������
		ETaskManager* taskManager = ETaskManager::GetInstance();
		task->Initialize(infoStgSystem);
		task->Start(nullptr, nullptr);

		//�^�X�N�N���A
		TransitionManager* transitionManager = SystemController::GetInstance()->GetTransitionManager();
		transitionManager->DoFadeOut();
		SystemController::GetInstance()->ClearTaskWithoutSystem();

		//STG�^�X�N�o�^
		taskManager->AddTask(task);
		taskManager->AddWorkFunction(TTaskFunction<StgSystemController>::Create(task, 
			&StgSystemController::Work), StgSystemController::TASK_PRI_WORK);
		taskManager->AddRenderFunction(TTaskFunction<StgSystemController>::Create(task, 
			&StgSystemController::Render), StgSystemController::TASK_PRI_RENDER);
	}
	catch (gstd::wexception& e) {
		Logger::WriteTop(e.what());

		SystemController* system = SystemController::GetInstance();
		system->ShowErrorDialog(e.what());
		if (!bOnlyPackage) {
			system->GetSceneManager()->TransScriptSelectScene_Last();
		}
		else {
			EApplication::GetInstance()->End();
		}

	}
}

/**********************************************************
//TransitionManager
**********************************************************/
TransitionManager::TransitionManager() {}
TransitionManager::~TransitionManager() {}
void TransitionManager::_CreateCurrentSceneTexture() {
	DirectGraphics* graphics = EDirectGraphics::GetInstance();
	WorkRenderTaskManager* taskManager = ETaskManager::GetInstance();
	TextureManager* textureManager = ETextureManager::GetInstance();
	shared_ptr<Texture> texture = textureManager->GetTexture(TextureManager::TARGET_TRANSITION);

	graphics->SetRenderTarget(texture);
	//graphics->ClearRenderTarget();
	graphics->BeginScene(false, true);
	taskManager->CallRenderFunction();
	graphics->EndScene(false);
	graphics->SetRenderTarget(nullptr);
}
void TransitionManager::_AddTask(ref_count_ptr<TransitionEffect> effect) {
	WorkRenderTaskManager* taskManager = ETaskManager::GetInstance();
	taskManager->RemoveTask(typeid(SystemTransitionEffectTask));

	shared_ptr<SystemTransitionEffectTask> task(new SystemTransitionEffectTask());
	task->SetTransition(effect);
	taskManager->AddTask(task);
	taskManager->AddWorkFunction(TTaskFunction<SystemTransitionEffectTask>::Create(task, 
		&SystemTransitionEffectTask::Work), TASK_PRI);
	taskManager->AddRenderFunction(TTaskFunction<SystemTransitionEffectTask>::Create(task, 
		&SystemTransitionEffectTask::Render), TASK_PRI);
}
void TransitionManager::DoFadeOut() {
	TextureManager* textureManager = ETextureManager::GetInstance();
	shared_ptr<Texture> texture = textureManager->GetTexture(TextureManager::TARGET_TRANSITION);
	_CreateCurrentSceneTexture();

	ref_count_ptr<TransitionEffect_FadeOut> effect = new TransitionEffect_FadeOut();
	effect->Initialize(10, texture);
	_AddTask(effect);

	EFpsController* fpsController = EFpsController::GetInstance();
	fpsController->SetCriticalFrame();
}

//SystemTransitionEffectTask
void SystemTransitionEffectTask::Work() {
	TransitionEffectTask::Work();
	if (effect_ != nullptr && effect_->IsEnd()) {
		WorkRenderTaskManager* taskManager = ETaskManager::GetInstance();
		taskManager->RemoveTask(this);
		return;
	}
}
void SystemTransitionEffectTask::Render() {
	TransitionEffectTask::Render();
}

/**********************************************************
//SystemInformation
**********************************************************/
SystemInformation::SystemInformation() {
	lastTitleSelectedIndex_ = 0;
	dirLastScriptSearch_ = EPathProperty::GetStgScriptRootDirectory();

	lastPlayerSelectIndex_ = 0;
}
SystemInformation::~SystemInformation() {

}
void SystemInformation::_SearchFreePlayerScript(std::wstring dir) {
	listFreePlayer_ = ScriptInformation::FindPlayerScriptInformationList(dir);
	for (size_t iPlayer = 0; iPlayer < listFreePlayer_.size(); iPlayer++) {
		ref_count_ptr<ScriptInformation> info = listFreePlayer_[iPlayer];
		std::wstring path = info->GetScriptPath();
		std::wstring log = StringUtility::Format(L"Found free player script: [%s]", path.c_str());
		ELogger::WriteTop(log);
	}
}
void SystemInformation::UpdateFreePlayerScriptInformationList() {
	listFreePlayer_.clear();
	std::wstring dir = EPathProperty::GetPlayerScriptRootDirectory();
	_SearchFreePlayerScript(dir);

	//�\�[�g
	std::sort(listFreePlayer_.begin(), listFreePlayer_.end(), ScriptInformation::PlayerListSort());
}

/**********************************************************
//SystemResidentTask
**********************************************************/
SystemResidentTask::SystemResidentTask() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	int screenWidth = graphics->GetScreenWidth();
	int screenHeight = graphics->GetScreenHeight();

	textFps_.SetFontColorTop(D3DCOLOR_ARGB(255, 160, 160, 255));
	textFps_.SetFontColorBottom(D3DCOLOR_ARGB(255, 64, 64, 255));
	textFps_.SetFontBorderType(directx::DxFont::BORDER_FULL);
	textFps_.SetFontBorderColor(D3DCOLOR_ARGB(255, 255, 255, 255));
	textFps_.SetFontBorderWidth(2);
	textFps_.SetFontSize(14);
	textFps_.SetFontWeight(FW_BOLD);
	textFps_.SetMaxWidth(screenWidth - 8);
	textFps_.SetHorizontalAlignment(DxText::ALIGNMENT_RIGHT);
	textFps_.SetPosition(0, screenHeight - 20);
}
SystemResidentTask::~SystemResidentTask() {}
void SystemResidentTask::RenderFps() {
	WorkRenderTaskManager* taskManager = ETaskManager::GetInstance();
	if (taskManager->GetTask(typeid(EStgSystemController)) != nullptr)return;
	if (taskManager->GetTask(typeid(PStgSystemController)) != nullptr)return;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	graphics->SetBlendMode(DirectGraphics::MODE_BLEND_ALPHA);
	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetFogEnable(false);

	EFpsController* fpsController = EFpsController::GetInstance();
	float fps = fpsController->GetCurrentWorkFps();
	textFps_.SetText(StringUtility::Format(L"%.2ffps", fps));
	textFps_.Render();
}