#include "source/GcLib/pch.h"

#include "StgItem.hpp"
#include "StgSystem.hpp"
#include "StgStageScript.hpp"
#include "StgPlayer.hpp"

/**********************************************************
//StgItemManager
**********************************************************/
StgItemManager::StgItemManager(StgStageController* stageController) {
	stageController_ = stageController;
	listItemData_ = new StgItemDataList();

	std::wstring dir = EPathProperty::GetSystemImageDirectory();

	listSpriteItem_ = new SpriteList2D();
	std::wstring pathItem = PathProperty::GetUnique(dir + L"System_Stg_Item.png");
	shared_ptr<Texture> textureItem(new Texture());
	textureItem->CreateFromFile(pathItem, false, false);
	listSpriteItem_->SetTexture(textureItem);

	listSpriteDigit_ = new SpriteList2D();
	std::wstring pathDigit = PathProperty::GetUnique(dir + L"System_Stg_Digit.png");
	shared_ptr<Texture> textureDigit(new Texture());
	textureDigit->CreateFromFile(pathDigit, false, false);
	listSpriteDigit_->SetTexture(textureDigit);

	bDefaultBonusItemEnable_ = true;
	bAllItemToPlayer_ = false;

	{
		RenderShaderManager* shaderManager_ = ShaderManager::GetBase()->GetRenderLib();
		effectLayer_ = shaderManager_->GetRender2DShader();
		handleEffectWorld_ = effectLayer_->GetParameterBySemantic(nullptr, "WORLD");
	}
}
StgItemManager::~StgItemManager() {
	ptr_delete(listSpriteItem_);
	ptr_delete(listSpriteDigit_);
	ptr_delete(listItemData_);
}
void StgItemManager::Work() {
	shared_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
	float px = objPlayer->GetX();
	float py = objPlayer->GetY();
	float pr = objPlayer->GetItemIntersectionRadius();
	pr *= pr;
	int pAutoItemCollectY = objPlayer->GetAutoItemCollectY();

	std::list<shared_ptr<StgItemObject>>::iterator itr = listObj_.begin();
	for (; itr != listObj_.end();) {
		shared_ptr<StgItemObject> obj = *itr;

		if (obj->IsDeleted()) {
			//obj->Clear();
			itr = listObj_.erase(itr);
		}
		else {
			float ix = obj->GetPositionX();
			float iy = obj->GetPositionY();
			if (objPlayer->GetState() == StgPlayerObject::STATE_NORMAL) {
				bool bMoveToPlayer = false;

				float dx = px - ix;
				float dy = py - iy;

				//if (obj->GetItemType() == StgItemObject::ITEM_SCORE && obj->GetFrameWork() > 60 * 15)
				//	obj->Intersect(nullptr, nullptr);

				bool deleted = false;

				float radius = dx * dx + dy * dy;
				if (radius <= 16 * 16) {
					obj->Intersect(nullptr, nullptr);
					deleted = true;
				}
				else if (radius <= pr && obj->IsPermitMoveToPlayer()) {
					obj->SetMoveToPlayer(true);
					bMoveToPlayer = true;
				}

				if (!deleted) {
					if (bCancelToPlayer_) {
						//��������L�����Z��
						obj->SetMoveToPlayer(false);
					}
					else if (obj->IsPermitMoveToPlayer() && !bMoveToPlayer) {
						if (pAutoItemCollectY >= 0) {
							//�㕔�������
							int typeMove = obj->GetMoveType();
							if (!obj->IsMoveToPlayer() && py <= pAutoItemCollectY)
								bMoveToPlayer = true;
						}

						if (listItemTypeToPlayer_.size() > 0) {
							//���@�ɃA�C�e�����W�߂�
							int typeItem = obj->GetItemType();
							bool bFind = listItemTypeToPlayer_.find(typeItem) != listItemTypeToPlayer_.end();
							if (bFind)
								bMoveToPlayer = true;
						}

						for (auto itr = listCircleToPlayer_.begin(); itr != listCircleToPlayer_.end(); ++itr) {
							DxCircle circle = *itr;

							float cdx = ix - circle.GetX();
							float cdy = iy - circle.GetY();
							float rr = (float)circle.GetR() * (float)circle.GetR();

							if ((cdx * cdx + cdy * cdy) <= rr) {
								bMoveToPlayer = true;
								break;
							}
						}

						if (bAllItemToPlayer_)
							bMoveToPlayer = true;

						if (bMoveToPlayer)
							obj->SetMoveToPlayer(true);
					}
				}
			}
			else {
				/*
				if (obj->GetItemType() == StgItemObject::ITEM_SCORE) {
					std::shared_ptr<StgMovePattern_Item> move =
						std::shared_ptr<StgMovePattern_Item>(new StgMovePattern_Item(obj.get()));
					move->SetItemMoveType(StgMovePattern_Item::MOVE_DOWN);
					obj->SetPattern(move);
				}
				*/
				obj->SetMoveToPlayer(false);
			}

			++itr;
		}
	}
	listItemTypeToPlayer_.clear();
	listCircleToPlayer_.clear();
	bAllItemToPlayer_ = false;
	bCancelToPlayer_ = false;

}
void StgItemManager::Render(int targetPriority) {
	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();

	graphics->SetZBufferEnable(false);
	graphics->SetZWriteEnable(false);
	graphics->SetCullingMode(D3DCULL_NONE);
	graphics->SetLightingEnable(false);
	graphics->SetTextureFilter(DirectGraphics::MODE_TEXTURE_FILTER_LINEAR, 0);

	//�t�H�O����������
	DWORD bEnableFog = FALSE;
	device->GetRenderState(D3DRS_FOGENABLE, &bEnableFog);
	if (bEnableFog)
		graphics->SetFogEnable(false);

	ref_count_ptr<DxCamera> camera3D = graphics->GetCamera();
	ref_count_ptr<DxCamera2D> camera2D = graphics->GetCamera2D();

	D3DXMATRIX& matCamera = camera2D->GetMatrix();

	for (auto itr = listObj_.begin(); itr != listObj_.end(); ++itr) {
		shared_ptr<StgItemObject> obj = *itr;
		if (obj->IsDeleted() || obj->GetRenderPriorityI() != targetPriority) continue;
		obj->RenderOnItemManager();
	}

	{
		D3DXMATRIX matProj = matCamera * graphics->GetViewPortMatrix();
		effectLayer_->SetMatrix(handleEffectWorld_, &matProj);
	}

	RenderShaderManager* shaderManager = ShaderManager::GetBase()->GetRenderLib();
	VertexBufferManager* bufferManager = VertexBufferManager::GetBase();

	device->SetFVF(VERTEX_TLX::fvf);

	int countBlendType = StgItemDataList::RENDER_TYPE_COUNT;
	int blendMode[] = { 
		DirectGraphics::MODE_BLEND_ADD_ARGB, 
		DirectGraphics::MODE_BLEND_ADD_RGB, 
		DirectGraphics::MODE_BLEND_ALPHA 
	};

	{
		graphics->SetBlendMode(DirectGraphics::MODE_BLEND_ADD_ARGB);
		listSpriteDigit_->Render();
		listSpriteDigit_->ClearVertexCount();
		graphics->SetBlendMode(DirectGraphics::MODE_BLEND_ALPHA);
		listSpriteItem_->Render();
		listSpriteItem_->ClearVertexCount();

		device->SetVertexDeclaration(shaderManager->GetVertexDeclarationTLX());
		device->SetStreamSource(0, bufferManager->GetGrowableVertexBuffer()->GetBuffer(), 0, sizeof(VERTEX_TLX));

		effectLayer_->SetTechnique("Render");

		UINT cPass = 1U;
		effectLayer_->Begin(&cPass, 0);
		if (cPass >= 1) {
			for (size_t iBlend = 0; iBlend < countBlendType; iBlend++) {
				bool hasPolygon = false;
				std::vector<StgItemRenderer*>& listRenderer = listItemData_->GetRendererList(blendMode[iBlend] - 1);

				for (auto itr = listRenderer.begin(); itr != listRenderer.end() && !hasPolygon; ++itr)
					hasPolygon = (*itr)->countRenderVertex_ >= 3U;
				if (!hasPolygon) continue;

				graphics->SetBlendMode(blendMode[iBlend]);

				effectLayer_->BeginPass(0);
				for (auto itrRender = listRenderer.begin(); itrRender != listRenderer.end(); ++itrRender)
					(*itrRender)->Render(this);
				effectLayer_->EndPass();
			}
		}
		effectLayer_->End();

		device->SetVertexDeclaration(nullptr);
	}

	if (bEnableFog)
		graphics->SetFogEnable(true);
}

void StgItemManager::GetValidRenderPriorityList(std::vector<PriListBool>& list) {
	auto objectManager = stageController_->GetMainObjectManager();
	list.resize(objectManager->GetRenderBucketCapacity());
	ZeroMemory(&list[0], objectManager->GetRenderBucketCapacity() * sizeof(PriListBool));

	for (auto itr = listObj_.begin(); itr != listObj_.end(); ++itr) {
		shared_ptr<StgItemObject> obj = *itr;
		if (obj->IsDeleted()) continue;
		int pri = obj->GetRenderPriorityI();
		list[pri] = true;
	}
}

bool StgItemManager::LoadItemData(std::wstring path, bool bReload) {
	return listItemData_->AddItemDataList(path, bReload);
}
shared_ptr<StgItemObject> StgItemManager::CreateItem(int type) {
	shared_ptr<StgItemObject> res;
	switch (type) {
	case StgItemObject::ITEM_1UP:
	case StgItemObject::ITEM_1UP_S:
		res = shared_ptr<StgItemObject>(new StgItemObject_1UP(stageController_));
		break;
	case StgItemObject::ITEM_SPELL:
	case StgItemObject::ITEM_SPELL_S:
		res = shared_ptr<StgItemObject>(new StgItemObject_Bomb(stageController_));
		break;
	case StgItemObject::ITEM_POWER:
	case StgItemObject::ITEM_POWER_S:
		res = shared_ptr<StgItemObject>(new StgItemObject_Power(stageController_));
		break;
	case StgItemObject::ITEM_POINT:
	case StgItemObject::ITEM_POINT_S:
		res = shared_ptr<StgItemObject>(new StgItemObject_Point(stageController_));
		break;
	case StgItemObject::ITEM_USER:
		res = shared_ptr<StgItemObject>(new StgItemObject_User(stageController_));
		break;
	}
	res->SetItemType(type);

	return res;
}
void StgItemManager::CollectItemsAll() {
	bAllItemToPlayer_ = true;
}
void StgItemManager::CollectItemsByType(int type) {
	listItemTypeToPlayer_.insert(type);
}
void StgItemManager::CollectItemsInCircle(DxCircle circle) {
	listCircleToPlayer_.push_back(circle);
}
void StgItemManager::CancelCollectItems() {
	bCancelToPlayer_ = true;
}

/**********************************************************
//StgItemDataList
**********************************************************/
StgItemDataList::StgItemDataList() {
	listRenderer_.resize(RENDER_TYPE_COUNT);
}
StgItemDataList::~StgItemDataList() {
	for (std::vector<StgItemRenderer*>& renderList : listRenderer_) {
		for (StgItemRenderer* renderer : renderList)
			ptr_delete(renderer);
		renderList.clear();
	}
	listRenderer_.clear();

	for (StgItemData* itemData : listData_)
		ptr_delete(itemData);
	listData_.clear();
}
bool StgItemDataList::AddItemDataList(std::wstring path, bool bReload) {
	if (!bReload && listReadPath_.find(path) != listReadPath_.end())return true;

	ref_count_ptr<FileReader> reader = FileManager::GetBase()->GetFileReader(path);
	if (reader == nullptr) throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path));
	if (!reader->Open())throw gstd::wexception(ErrorUtility::GetFileNotFoundErrorMessage(path));
	std::string source = reader->ReadAllString();

	bool res = false;
	Scanner scanner(source);
	try {
		std::vector<StgItemData*> listData;

		std::wstring pathImage = L"";
		RECT rcDelay = { -1, -1, -1, -1 };
		while (scanner.HasNext()) {
			Token& tok = scanner.Next();
			if (tok.GetType() == Token::TK_EOF)//Eof�̎��ʎq��������t�@�C���̒����I��
			{
				break;
			}
			else if (tok.GetType() == Token::TK_ID) {
				std::wstring element = tok.GetElement();
				if (element == L"ItemData") {
					_ScanItem(listData, scanner);
				}
				else if (element == L"item_image") {
					scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
					pathImage = scanner.Next().GetString();
				}

				if (scanner.HasNext())
					tok = scanner.Next();

			}
		}

		//�e�N�X�`���ǂݍ���
		if (pathImage.size() == 0)throw gstd::wexception("Item texture must be set.");
		std::wstring dir = PathProperty::GetFileDirectory(path);
		pathImage = StringUtility::Replace(pathImage, L"./", dir);

		shared_ptr<Texture> texture(new Texture());
		bool bTexture = texture->CreateFromFile(PathProperty::GetUnique(pathImage), false, false);
		if (!bTexture)throw gstd::wexception("The specified item texture cannot be found.");

		int textureIndex = -1;
		for (int iTexture = 0; iTexture < listTexture_.size(); iTexture++) {
			shared_ptr<Texture> tSearch = listTexture_[iTexture];
			if (tSearch->GetName() == texture->GetName()) {
				textureIndex = iTexture;
				break;
			}
		}
		if (textureIndex < 0) {
			textureIndex = listTexture_.size();
			listTexture_.push_back(texture);
			for (size_t iRender = 0; iRender < listRenderer_.size(); iRender++) {
				StgItemRenderer* render = new StgItemRenderer();
				render->SetTexture(texture);
				listRenderer_[iRender].push_back(render);
			}
		}

		if (listData_.size() < listData.size())
			listData_.resize(listData.size());
		for (size_t iData = 0; iData < listData.size(); iData++) {
			StgItemData* data = listData[iData];
			if (data == nullptr)continue;

			data->indexTexture_ = textureIndex;
			{
				auto texture = listTexture_[data->indexTexture_];
				data->textureSize_.x = texture->GetWidth();
				data->textureSize_.y = texture->GetHeight();
			}

			listData_[iData] = data;
		}

		listReadPath_.insert(path);
		Logger::WriteTop(StringUtility::Format(L"Loaded item data: %s", path.c_str()));
		res = true;
	}
	catch (gstd::wexception& e) {
		std::wstring log = StringUtility::Format(L"Failed to load item data: [Line=%d] (%s)", scanner.GetCurrentLine(), e.what());
		Logger::WriteTop(log);
		res = false;
	}
	catch (...) {
		std::wstring log = StringUtility::Format(L"Failed to load item data: [Line=%d] (Unknown error.)", scanner.GetCurrentLine());
		Logger::WriteTop(log);
		res = false;
	}

	return res;
}
void StgItemDataList::_ScanItem(std::vector<StgItemData*>& listData, Scanner& scanner) {
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::TK_NEWLINE)tok = scanner.Next();
	scanner.CheckType(tok, Token::TK_OPENC);

	StgItemData* data = new StgItemData(this);
	int id = -1;
	int typeItem = -1;

	while (true) {
		tok = scanner.Next();
		if (tok.GetType() == Token::TK_CLOSEC) {
			break;
		}
		else if (tok.GetType() == Token::TK_ID) {
			std::wstring element = tok.GetElement();

			if (element == L"id") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				id = scanner.Next().GetInteger();
			}
			else if (element == L"type") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				typeItem = scanner.Next().GetInteger();
			}
			else if (element == L"rect") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);

				StgItemData::AnimationData anime;

				RECT rect;
				rect.left = StringUtility::ToInteger(list[0]);
				rect.top = StringUtility::ToInteger(list[1]);
				rect.right = StringUtility::ToInteger(list[2]);
				rect.bottom = StringUtility::ToInteger(list[3]);
				anime.rcSrc_ = rect;

				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
				if (width % 2 == 1) rcDest.right += 1;
				if (height % 2 == 1) rcDest.bottom += 1;
				anime.rcDest_ = rcDest;

				data->listAnime_.resize(1);
				data->listAnime_[0] = anime;
				data->totalAnimeFrame_ = 1;
			}
			else if (element == L"out") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);
				RECT rect;
				rect.left = StringUtility::ToInteger(list[0]);
				rect.top = StringUtility::ToInteger(list[1]);
				rect.right = StringUtility::ToInteger(list[2]);
				rect.bottom = StringUtility::ToInteger(list[3]);
				data->rcOutSrc_ = rect;

				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
				if (width % 2 == 1) rcDest.right += 1;
				if (height % 2 == 1) rcDest.bottom += 1;
				data->rcOutDest_ = rcDest;
			}
			else if (element == L"render") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				std::wstring render = scanner.Next().GetElement();
				if (render == L"ADD" || render == L"ADD_RGB")
					data->typeRender_ = DirectGraphics::MODE_BLEND_ADD_RGB;
				else if (render == L"ADD_ARGB")
					data->typeRender_ = DirectGraphics::MODE_BLEND_ADD_ARGB;
			}
			else if (element == L"alpha") {
				scanner.CheckType(scanner.Next(), Token::TK_EQUAL);
				data->alpha_ = scanner.Next().GetInteger();
			}
			else if (element == L"AnimationData") {
				_ScanAnimation(data, scanner);
			}
		}
	}

	if (id >= 0) {
		if (listData.size() <= id)
			listData.resize(id + 1);

		if (typeItem < 0)
			typeItem = id;
		data->typeItem_ = typeItem;

		listData[id] = data;
	}
}
void StgItemDataList::_ScanAnimation(StgItemData* itemData, Scanner& scanner) {
	Token& tok = scanner.Next();
	if (tok.GetType() == Token::TK_NEWLINE)tok = scanner.Next();
	scanner.CheckType(tok, Token::TK_OPENC);

	while (true) {
		tok = scanner.Next();
		if (tok.GetType() == Token::TK_CLOSEC) {
			break;
		}
		else if (tok.GetType() == Token::TK_ID) {
			std::wstring element = tok.GetElement();

			if (element == L"animation_data") {
				std::vector<std::wstring> list = _GetArgumentList(scanner);
				if (list.size() == 5) {
					StgItemData::AnimationData anime;
					int frame = StringUtility::ToInteger(list[0]);
					RECT rcSrc = {
						StringUtility::ToInteger(list[1]),
						StringUtility::ToInteger(list[2]),
						StringUtility::ToInteger(list[3]),
						StringUtility::ToInteger(list[4]),
					};

					int width = rcSrc.right - rcSrc.left;
					int height = rcSrc.bottom - rcSrc.top;
					RECT rcDest = { -width / 2, -height / 2, width / 2, height / 2 };
					if (width % 2 == 1)rcDest.right += 1;
					if (height % 2 == 1)rcDest.bottom += 1;

					anime.frame_ = frame;
					anime.rcSrc_ = rcSrc;
					anime.rcDest_ = rcDest;

					itemData->listAnime_.push_back(anime);
					itemData->totalAnimeFrame_ += frame;
				}
			}
		}
	}
}
std::vector<std::wstring> StgItemDataList::_GetArgumentList(Scanner& scanner) {
	std::vector<std::wstring> res;
	scanner.CheckType(scanner.Next(), Token::TK_EQUAL);

	Token& tok = scanner.Next();

	if (tok.GetType() == Token::TK_OPENP) {
		while (true) {
			tok = scanner.Next();
			int type = tok.GetType();
			if (type == Token::TK_CLOSEP)break;
			else if (type != Token::TK_COMMA) {
				std::wstring str = tok.GetElement();
				res.push_back(str);
			}
		}
	}
	else {
		res.push_back(tok.GetElement());
	}
	return res;
}

//StgItemData
StgItemData::StgItemData(StgItemDataList* listItemData) {
	listItemData_ = listItemData;
	typeRender_ = DirectGraphics::MODE_BLEND_ALPHA;
	SetRect(&rcOutSrc_, 0, 0, 0, 0);
	SetRect(&rcOutDest_, 0, 0, 0, 0);
	alpha_ = 255;
	totalAnimeFrame_ = 0;
}
StgItemData::~StgItemData() {}
StgItemData::AnimationData* StgItemData::GetData(int frame) {
	if (totalAnimeFrame_ <= 1)
		return &listAnime_[0];

	frame = frame % totalAnimeFrame_;
	int total = 0;

	std::vector<AnimationData>::iterator itr = listAnime_.begin();
	for (; itr != listAnime_.end(); itr++) {
		//AnimationData* anime = itr;
		total += itr->frame_;
		if (total >= frame)
			return &(*itr);
	}
	return &listAnime_[0];
}
StgItemRenderer* StgItemData::GetRenderer(int type) {
	if (type < DirectGraphics::MODE_BLEND_ALPHA || type > DirectGraphics::MODE_BLEND_ADD_ARGB)
		return listItemData_->GetRenderer(indexTexture_, 0);
	return listItemData_->GetRenderer(indexTexture_, type - 1);
}

/**********************************************************
//StgItemRenderer
**********************************************************/
StgItemRenderer::StgItemRenderer() {
	countRenderVertex_ = 0;
	countMaxVertex_ = 256 * 256;
	SetVertexCount(countMaxVertex_);
}
StgItemRenderer::~StgItemRenderer() {

}
void StgItemRenderer::Render(StgItemManager* manager) {
	if (countRenderVertex_ < 3) return;

	DirectGraphics* graphics = DirectGraphics::GetBase();
	IDirect3DDevice9* device = graphics->GetDevice();
	IDirect3DTexture9* pTexture = texture_[0] ? texture_[0]->GetD3DTexture() : nullptr;
	device->SetTexture(0, pTexture);

	VertexBufferManager* bufferManager = VertexBufferManager::GetBase();

	GrowableVertexBuffer* vBuffer = bufferManager->GetGrowableVertexBuffer();
	IDirect3DVertexBuffer9* pVBuffer = vBuffer->GetBuffer();
	if (countMaxVertex_ > vBuffer->GetSize()) {
		vBuffer->Expand(countMaxVertex_);

		pVBuffer = vBuffer->GetBuffer();
		device->SetStreamSource(0, pVBuffer, 0, sizeof(VERTEX_TLX));
	}

	countRenderVertex_ = std::min(countRenderVertex_, vBuffer->GetSize());
	{
		void* tmp;
		pVBuffer->Lock(0, 0, &tmp, D3DLOCK_DISCARD);
		memcpy(tmp, vertex_.data(), countRenderVertex_ * sizeof(VERTEX_TLX));
		pVBuffer->Unlock();
	}

	device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, countRenderVertex_ / 3U);

	countRenderVertex_ = 0;
}
void StgItemRenderer::AddVertex(VERTEX_TLX& vertex) {
	if (countRenderVertex_ == countMaxVertex_ - 1) {
		countMaxVertex_ *= 2;
		SetVertexCount(countMaxVertex_);
	}

	SetVertex(countRenderVertex_, vertex);
	++countRenderVertex_;
}

/**********************************************************
//StgItemObject
**********************************************************/
StgItemObject::StgItemObject(StgStageController* stageController) : StgMoveObject(stageController) {
	stageController_ = stageController;
	typeObject_ = TypeObject::OBJ_ITEM;

	pattern_ = std::shared_ptr<StgMovePattern_Item>(new StgMovePattern_Item(this));
	color_ = D3DCOLOR_ARGB(255, 255, 255, 255);
	score_ = 0;

	bMoveToPlayer_ = false;
	bPermitMoveToPlayer_ = true;
	bChangeItemScore_ = true;

	frameWork_ = 0;

	int priItemI = stageController_->GetStageInformation()->GetItemObjectPriority();
	SetRenderPriorityI(priItemI);
}
void StgItemObject::Work() {
	bool bDefaultMovePattern = std::dynamic_pointer_cast<StgMovePattern_Item>(GetPattern()) != nullptr;
	if (!bDefaultMovePattern && IsMoveToPlayer() && bEnableMovement_) {
		float speed = 8;
		shared_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
		float angle = atan2f(objPlayer->GetY() - GetPositionY(), objPlayer->GetX() - GetPositionX());
		float angDirection = angle;
		SetSpeed(speed);
		SetDirectionAngle(angDirection);
	}
	StgMoveObject::_Move();
	SetX(posX_);
	SetY(posY_);

	_DeleteInAutoClip();

	++frameWork_;
}
void StgItemObject::RenderOnItemManager() {
	StgItemManager* itemManager = stageController_->GetItemManager();
	SpriteList2D* renderer = typeItem_ == ITEM_SCORE ?
		itemManager->GetDigitRenderer() : itemManager->GetItemRenderer();

	if (typeItem_ != ITEM_SCORE) {
		float scale = 1.0f;
		switch (typeItem_) {
		case ITEM_1UP:
		case ITEM_SPELL:
		case ITEM_POWER:
		case ITEM_POINT:
			scale = 1.0f;
			break;
		case ITEM_1UP_S:
		case ITEM_SPELL_S:
		case ITEM_POWER_S:
		case ITEM_POINT_S:
		case ITEM_BONUS:
			scale = 0.75f;
			break;
		}

		RECT rcSrc;
		switch (typeItem_) {
		case ITEM_1UP:
		case ITEM_1UP_S:
			SetRect(&rcSrc, 1, 1, 16, 16);
			break;
		case ITEM_SPELL:
		case ITEM_SPELL_S:
			SetRect(&rcSrc, 20, 1, 35, 16);
			break;
		case ITEM_POWER:
		case ITEM_POWER_S:
			SetRect(&rcSrc, 40, 1, 55, 16);
			break;
		case ITEM_POINT:
		case ITEM_POINT_S:
			SetRect(&rcSrc, 1, 20, 16, 35);
			break;
		case ITEM_BONUS:
			SetRect(&rcSrc, 20, 20, 35, 35);
			break;
		}

		//��ɂ͂ݏo���Ă���
		float posY = posY_;
		D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255);
		if (posY_ <= 0) {
			D3DCOLOR colorOver = D3DCOLOR_ARGB(255, 255, 255, 255);
			switch (typeItem_) {
			case ITEM_1UP:
			case ITEM_1UP_S:
				colorOver = D3DCOLOR_ARGB(255, 236, 0, 236);
				break;
			case ITEM_SPELL:
			case ITEM_SPELL_S:
				colorOver = D3DCOLOR_ARGB(255, 0, 160, 0);
				break;
			case ITEM_POWER:
			case ITEM_POWER_S:
				colorOver = D3DCOLOR_ARGB(255, 209, 0, 0);
				break;
			case ITEM_POINT:
			case ITEM_POINT_S:
				colorOver = D3DCOLOR_ARGB(255, 0, 0, 160);
				break;
			}
			if (color != colorOver) {
				SetRect(&rcSrc, 113, 1, 126, 10);
				posY = 6;
			}
			color = colorOver;
		}

		RECT_D rcSrcD = GetRectD(rcSrc);
		renderer->SetColor(color);
		renderer->SetPosition(posX_, posY, 0);
		renderer->SetScaleXYZ(scale, scale, scale);
		renderer->SetSourceRect(rcSrcD);
		renderer->SetDestinationCenter();
		renderer->AddVertex();
	}
	else {
		renderer->SetScaleXYZ(1.0, 1.0, 1.0);
		renderer->SetColor(color_);
		renderer->SetPosition(0, 0, 0);

		int fontSize = 14;
		int64_t score = score_;
		std::vector<int> listNum;
		while (true) {
			int tnum = score % 10;
			score /= 10;
			listNum.push_back(tnum);
			if (score == 0)break;
		}
		for (int iNum = listNum.size() - 1; iNum >= 0; iNum--) {
			RECT_D rcSrc = { (double)(listNum[iNum] * 36), 0.,
				(double)((listNum[iNum] + 1) * 36 - 1), 31. };
			RECT_D rcDest = { (double)(posX_ + (listNum.size() - 1 - iNum) * fontSize / 2), (double)posY_,
				(double)(posX_ + (listNum.size() - iNum)*fontSize / 2), (double)(posY_ + fontSize) };
			renderer->SetSourceRect(rcSrc);
			renderer->SetDestinationRect(rcDest);
			renderer->AddVertex();
		}
	}
}
void StgItemObject::_DeleteInAutoClip() {
	DirectGraphics* graphics = DirectGraphics::GetBase();

	RECT rcClip;
	ZeroMemory(&rcClip, sizeof(RECT));
	rcClip.left = -64;
	rcClip.right = graphics->GetScreenWidth() + 64;
	rcClip.bottom = graphics->GetScreenHeight() + 64;
	bool bDelete = (posX_ < rcClip.left || posX_ > rcClip.right || posY_ > rcClip.bottom);
	if (!bDelete)return;

	stageController_->GetMainObjectManager()->DeleteObject(this);
}
void StgItemObject::_CreateScoreItem() {
	auto objectManager = stageController_->GetMainObjectManager();
	StgItemManager* itemManager = stageController_->GetItemManager();

	if (itemManager->GetItemCount() < StgItemManager::ITEM_MAX) {
		shared_ptr<StgItemObject_Score> obj = shared_ptr<StgItemObject_Score>(new StgItemObject_Score(stageController_));
		obj->SetX(posX_);
		obj->SetY(posY_);
		obj->SetScore(score_);

		objectManager->AddObject(obj);
		itemManager->AddItem(obj);
	}
}
void StgItemObject::_NotifyEventToPlayerScript(std::vector<float>& listValue) {
	//���@�X�N���v�g�֒ʒm
	shared_ptr<StgPlayerObject> player = stageController_->GetPlayerObject();
	StgStagePlayerScript* scriptPlayer = player->GetPlayerScript();
	std::vector<gstd::value> listScriptValue;
	for (size_t iVal = 0; iVal < listValue.size(); iVal++) {
		listScriptValue.push_back(scriptPlayer->CreateRealValue(listValue[iVal]));
	}

	scriptPlayer->RequestEvent(StgStageItemScript::EV_GET_ITEM, listScriptValue);
}
void StgItemObject::_NotifyEventToItemScript(std::vector<float>& listValue) {
	//�A�C�e���X�N���v�g�֒ʒm
	auto stageScriptManager = stageController_->GetScriptManager();
	int64_t idItemScript = stageScriptManager->GetItemScriptID();
	if (idItemScript != StgControlScriptManager::ID_INVALID) {
		shared_ptr<ManagedScript> scriptItem = stageScriptManager->GetScript(idItemScript);
		if (scriptItem) {
			std::vector<gstd::value> listScriptValue;
			for (size_t iVal = 0; iVal < listValue.size(); iVal++) {
				listScriptValue.push_back(scriptItem->CreateRealValue(listValue[iVal]));
			}
			scriptItem->RequestEvent(StgStageItemScript::EV_GET_ITEM, listScriptValue);
		}
	}
}
void StgItemObject::SetAlpha(int alpha) {
	ColorAccess::ClampColor(alpha);
	color_ = (color_ & 0x00ffffff) | ((byte)alpha << 24);
}
void StgItemObject::SetColor(int r, int g, int b) {
	ColorAccess::ClampColor(r);
	ColorAccess::ClampColor(g);
	ColorAccess::ClampColor(b);
	D3DCOLOR dc = D3DCOLOR_ARGB(0, r, g, b);
	color_ = (color_ & 0xff000000) | (dc & 0x00ffffff);
}
void StgItemObject::SetToPosition(POINT pos) {
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetToPosition(pos);
}
int StgItemObject::GetMoveType() {
	int res = StgMovePattern_Item::MOVE_NONE;

	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	if (move) res = move->GetItemMoveType();
	return res;
}
void StgItemObject::SetMoveType(int type) {
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	if (move) move->SetItemMoveType(type);
}


//StgItemObject_1UP
StgItemObject_1UP::StgItemObject_1UP(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_1UP;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
}
void StgItemObject_1UP::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	std::vector<float> listValue;
	listValue.push_back(typeItem_);
	listValue.push_back(idObject_);
	_NotifyEventToPlayerScript(listValue);
	_NotifyEventToItemScript(listValue);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Bomb
StgItemObject_Bomb::StgItemObject_Bomb(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_SPELL;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
}
void StgItemObject_Bomb::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	std::vector<float> listValue;
	listValue.push_back(typeItem_);
	listValue.push_back(idObject_);
	_NotifyEventToPlayerScript(listValue);
	_NotifyEventToItemScript(listValue);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Power
StgItemObject_Power::StgItemObject_Power(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_POWER;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
	score_ = 10;
}
void StgItemObject_Power::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	if (bChangeItemScore_)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	std::vector<float> listValue;
	listValue.push_back(typeItem_);
	listValue.push_back(idObject_);
	_NotifyEventToPlayerScript(listValue);
	_NotifyEventToItemScript(listValue);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Point
StgItemObject_Point::StgItemObject_Point(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_POINT;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_TOPOSITION_A);
}
void StgItemObject_Point::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	if (bChangeItemScore_)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	std::vector<float> listValue;
	listValue.push_back(typeItem_);
	listValue.push_back(idObject_);
	_NotifyEventToPlayerScript(listValue);
	_NotifyEventToItemScript(listValue);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Bonus
StgItemObject_Bonus::StgItemObject_Bonus(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_BONUS;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_TOPLAYER);

	int graze = stageController->GetStageInformation()->GetGraze();
	score_ = (int)(graze / 40) * 10 + 300;
}
void StgItemObject_Bonus::Work() {
	StgItemObject::Work();

	shared_ptr<StgPlayerObject> objPlayer = stageController_->GetPlayerObject();
	if (objPlayer->GetState() != StgPlayerObject::STATE_NORMAL) {
		_CreateScoreItem();
		stageController_->GetStageInformation()->AddScore(score_);

		auto objectManager = stageController_->GetMainObjectManager();
		objectManager->DeleteObject(this);
	}
}
void StgItemObject_Bonus::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}

//StgItemObject_Score
StgItemObject_Score::StgItemObject_Score(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_SCORE;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_SCORE);

	bPermitMoveToPlayer_ = false;

	frameDelete_ = 0;
}
void StgItemObject_Score::Work() {
	StgItemObject::Work();
	int alpha = 255 - frameDelete_ * 8;
	color_ = D3DCOLOR_ARGB(alpha, alpha, alpha, alpha);

	if (frameDelete_ > 30) {
		stageController_->GetMainObjectManager()->DeleteObject(this);
		return;
	}
	++frameDelete_;
}
void StgItemObject_Score::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) { }

//StgItemObject_User
StgItemObject_User::StgItemObject_User(StgStageController* stageController) : StgItemObject(stageController) {
	typeItem_ = ITEM_USER;
	idImage_ = -1;
	frameWork_ = 0;
	auto move = std::dynamic_pointer_cast<StgMovePattern_Item>(pattern_);
	move->SetItemMoveType(StgMovePattern_Item::MOVE_DOWN);

	bChangeItemScore_ = true;
}
void StgItemObject_User::SetImageID(int id) {
	idImage_ = id;
	StgItemData* data = _GetItemData();
	if (data) {
		typeItem_ = data->GetItemType();
	}
}
StgItemData* StgItemObject_User::_GetItemData() {
	StgItemData* res = nullptr;
	StgItemManager* itemManager = stageController_->GetItemManager();
	StgItemDataList* dataList = itemManager->GetItemDataList();

	if (dataList) {
		res = dataList->GetData(idImage_);
	}

	return res;
}
void StgItemObject_User::Work() {
	StgItemObject::Work();
	++frameWork_;
}
void StgItemObject_User::RenderOnItemManager() {
	if (!IsVisible()) return;

	StgItemData* itemData = _GetItemData();
	if (itemData == nullptr) return;

	StgItemRenderer* renderer = nullptr;

	int objBlendType = GetBlendType();
	if (objBlendType == DirectGraphics::MODE_BLEND_NONE) {
		objBlendType = itemData->GetRenderType();
	}
	
	renderer = itemData->GetRenderer(objBlendType);

	if (renderer == nullptr)return;

	float scaleX = scale_.x;
	float scaleY = scale_.y;
	float c = 1.0f;
	float s = 0.0f;
	float posy = position_.y;

	StgItemData::AnimationData* frameData = itemData->GetData(frameWork_);

	RECT* rcSrc = &frameData->rcSrc_;
	RECT* rcDst = &frameData->rcDest_;
	D3DCOLOR color;

	{
		bool bOutY = false;
		if (position_.y + (rcSrc->bottom - rcSrc->top) / 2 <= 0) {
			bOutY = true;
			rcSrc = itemData->GetOutSrc();
			rcDst = itemData->GetOutDest();
		}

		if (!bOutY) {
			c = cosf(angle_.z);
			s = sinf(angle_.z);
		}
		else {
			scaleX = 1.0;
			scaleY = 1.0;
			posy = (rcSrc->bottom - rcSrc->top) / 2;
		}

		bool bBlendAddRGB = (objBlendType == DirectGraphics::MODE_BLEND_ADD_RGB);

		color = color_;
		float alphaRate = itemData->GetAlpha() / 255.0f;
		if (bBlendAddRGB) {
			color = ColorAccess::ApplyAlpha(color, alphaRate);
		}
		else {
			byte alpha = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * alphaRate);
			color = (color & 0x00ffffff) | (alpha << 24);
		}
	}

	//if(bIntersected_)color = D3DCOLOR_ARGB(255, 255, 0, 0);//�ڐG�e�X�g

	VERTEX_TLX verts[4];
	/*
	int srcX[] = { rcSrc.left, rcSrc.right, rcSrc.left, rcSrc.right };
	int srcY[] = { rcSrc.top, rcSrc.top, rcSrc.bottom, rcSrc.bottom };
	int destX[] = { rcDest.left, rcDest.right, rcDest.left, rcDest.right };
	int destY[] = { rcDest.top, rcDest.top, rcDest.bottom, rcDest.bottom };
	*/
	LONG* ptrSrc = reinterpret_cast<LONG*>(rcSrc);
	LONG* ptrDst = reinterpret_cast<LONG*>(rcDst);

	for (size_t iVert = 0; iVert < 4; iVert++) {
		VERTEX_TLX vt;

		_SetVertexUV(vt, ptrSrc[(iVert & 0b1) << 1] / itemData->GetTextureSize().x, 
			ptrSrc[iVert | 0b1] / itemData->GetTextureSize().y);
		_SetVertexPosition(vt, ptrDst[(iVert & 0b1) << 1], ptrDst[iVert | 0b1]);
		_SetVertexColorARGB(vt, color);

		float px = vt.position.x * scaleX;
		float py = vt.position.y * scaleY;
		vt.position.x = (px * c - py * s) + position_.x;
		vt.position.y = (px * s + py * c) + posy;
		vt.position.z = position_.z;

		//D3DXVec3TransformCoord((D3DXVECTOR3*)&vt.position, (D3DXVECTOR3*)&vt.position, &mat);
		verts[iVert] = vt;
	}

	renderer->AddSquareVertex(verts);
}
void StgItemObject_User::Intersect(StgIntersectionTarget::ptr ownTarget, StgIntersectionTarget::ptr otherTarget) {
	if (bChangeItemScore_)
		_CreateScoreItem();
	stageController_->GetStageInformation()->AddScore(score_);

	std::vector<float> listValue;
	listValue.push_back(typeItem_);
	listValue.push_back(idObject_);
	_NotifyEventToItemScript(listValue);

	auto objectManager = stageController_->GetMainObjectManager();
	objectManager->DeleteObject(this);
}


/**********************************************************
//StgMovePattern_Item
**********************************************************/
StgMovePattern_Item::StgMovePattern_Item(StgMoveObject* target) : StgMovePattern(target) {
	frame_ = 0;
	typeMove_ = MOVE_DOWN;
	speed_ = 0;
	angDirection_ = Math::DegreeToRadian(270);
	ZeroMemory(&posTo_, sizeof(POINT));
}
void StgMovePattern_Item::Move() {
	StgItemObject* itemObject = (StgItemObject*)target_;
	StgStageController* stageController = itemObject->GetStageController();

	double px = target_->GetPositionX();
	double py = target_->GetPositionY();
	if (typeMove_ == MOVE_TOPLAYER || itemObject->IsMoveToPlayer()) {
		if (frame_ == 0) speed_ = 6;
		speed_ += 0.075;
		shared_ptr<StgPlayerObject> objPlayer = stageController->GetPlayerObject();
		double angle = atan2(objPlayer->GetY() - py, objPlayer->GetX() - px);
		angDirection_ = angle;
		c_ = cos(angDirection_);
		s_ = sin(angDirection_);
	}
	else if (typeMove_ == MOVE_TOPOSITION_A) {
		double dx = posTo_.x - px;
		double dy = posTo_.y - py;
		speed_ = sqrt(dx * dx + dy * dy) / 16.0;

		double angle = atan2(dy, dx);
		angDirection_ = angle;
		if (frame_ == 0) {
			c_ = cos(angDirection_);
			s_ = sin(angDirection_);
		}
		else if (frame_ == 60) {
			speed_ = 0;
			angDirection_ = Math::DegreeToRadian(90);
			typeMove_ = MOVE_DOWN;
			c_ = 0;
			s_ = 1;
		}
	}
	else if (typeMove_ == MOVE_DOWN) {
		speed_ += 3.0 / 60.0;
		if (speed_ > 2.5) speed_ = 2.5;
		angDirection_ = Math::DegreeToRadian(90);
		c_ = 0;
		s_ = 1;
	}
	else if (typeMove_ == MOVE_SCORE) {
		speed_ = 1;
		angDirection_ = Math::DegreeToRadian(270);
		c_ = 0;
		s_ = -1;
	}

	if (typeMove_ != MOVE_NONE) {
		//c_ = cos(angDirection_);
		//s_ = sin(angDirection_);
		double sx = speed_ * c_;
		double sy = speed_ * s_;
		px = target_->GetPositionX() + sx;
		py = target_->GetPositionY() + sy;
		target_->SetPositionX(px);
		target_->SetPositionY(py);
	}

	++frame_;
}
