#include "source/GcLib/pch.h"

#include "StgPackageController.hpp"
#include "StgSystem.hpp"

/**********************************************************
//StgPackageController
**********************************************************/
StgPackageController::StgPackageController(StgSystemController* systemController) {
	systemController_ = systemController;
	scriptManager_ = nullptr;
}
StgPackageController::~StgPackageController() {
}
void StgPackageController::Initialize() {
	infoPackage_ = new StgPackageInformation();
	scriptManager_ = std::shared_ptr<StgPackageScriptManager>(new StgPackageScriptManager(systemController_));

	ref_count_ptr<StgSystemInformation> infoSystem = systemController_->GetSystemInformation();
	ref_count_ptr<ScriptInformation> infoScript = infoSystem->GetMainScriptInformation();
	infoPackage_->SetMainScriptInformation(infoScript);

	//���C���X�N���v�g
	std::wstring pathMainScript = infoScript->GetScriptPath();
	ELogger::WriteTop(StringUtility::Format(L"Package script: [%s]", pathMainScript.c_str()));
	auto idScript = scriptManager_->LoadScript(pathMainScript, StgPackageScript::TYPE_PACKAGE_MAIN);
	scriptManager_->StartScript(idScript);

	infoPackage_->SetPackageStartTime(timeGetTime());
}
void StgPackageController::Work() {
	scriptManager_->Work();
	//�X�N���v�g������ꂽ�ꍇ�͍ēx���s(�`��̌p���ڂ�ڗ����Ȃ�����)
	if (scriptManager_->IsHasCloseScliptWork())
		scriptManager_->Work();
}
void StgPackageController::Render() {
	//scriptManager_->Render();
}
void StgPackageController::RenderToTransitionTexture() {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	TextureManager* textureManager = ETextureManager::GetInstance();
	shared_ptr<Texture> texture = textureManager->GetTexture(TextureManager::TARGET_TRANSITION);

	graphics->SetRenderTarget(texture);
	graphics->BeginScene(false, true);

	scriptManager_->Render();

	graphics->EndScene(false);
	graphics->SetRenderTarget(nullptr);
}

/**********************************************************
//StgPackageInformation
**********************************************************/
StgPackageInformation::StgPackageInformation() {
	bEndPackage_ = false;
	timeStart_ = 0;
}
StgPackageInformation::~StgPackageInformation() {}
void StgPackageInformation::InitializeStageData() {
	infoReplay_ = nullptr;
	listStageData_.clear();

	nextStageStartData_ = new StgStageStartData();
	ref_count_ptr<StgStageInformation> infoStage = new StgStageInformation();
	nextStageStartData_->SetStageInformation(infoStage);
}
void StgPackageInformation::FinishCurrentStage() {
	ref_count_ptr<StgStageStartData> currentStageStartData = nextStageStartData_;
	ref_count_ptr<StgStageInformation> currentStageInfo = currentStageStartData->GetStageInformation();
	listStageData_.push_back(currentStageStartData);

	nextStageStartData_ = new StgStageStartData();
	ref_count_ptr<StgStageInformation> nextStageInfo = new StgStageInformation();
	nextStageStartData_->SetStageInformation(nextStageInfo);

	nextStageStartData_->SetPrevStageInformation(currentStageInfo);

	ref_count_ptr<StgPlayerInformation> infoPlayer = currentStageInfo->GetPlayerObjectInformation();
	nextStageStartData_->SetPrevPlayerInformation(infoPlayer);

}