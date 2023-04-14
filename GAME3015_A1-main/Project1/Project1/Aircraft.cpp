//***************************************************************************************
// Aircraft.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************

#include "Aircraft.hpp"
#include "Game.hpp"
#include "State.hpp"

// Constructor
Aircraft::Aircraft(Type type, State* state) : Entity(state)
	, mType(type)
{
	// Set the aircraft's sprite based on its type
	switch (type)
	{
	case (Type::Eagle):
		mSprite = "Eagle";
		break;
	case (Type::Raptor):
		mSprite = "Raptor";
		break;
	default:
		mSprite = "Eagle";
		break;
	}
}

// Returns the category of the aircraft based on its type
unsigned int Aircraft::getCategory() const
{
	switch (mType)
	{
	case Type::Eagle:
		return Category::PlayerAircraft;

	default:
		return Category::EnemyAircraft;
	}
}

// Draws the current aircraft
void Aircraft::drawCurrent() const
{
	// Calculate the constant buffer byte size
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	Game* game = mState->getContext()->game;

	auto objectCB = game->mCurrFrameResource->ObjectCB->Resource();
	auto matCB = game->mCurrFrameResource->MaterialCB->Resource();

	// If the aircraft render item exists, configure the command list and draw it
	if (mAircraftRitem != nullptr)
	{
		game->getCmdList()->IASetVertexBuffers(0, 1, &mAircraftRitem->Geo->VertexBufferView());
		game->getCmdList()->IASetIndexBuffer(&mAircraftRitem->Geo->IndexBufferView());
		game->getCmdList()->IASetPrimitiveTopology(mAircraftRitem->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(game->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(mAircraftRitem->Mat->DiffuseSrvHeapIndex, game->mCbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress =  objectCB->GetGPUVirtualAddress() + (UINT64)mAircraftRitem->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + (UINT64)mAircraftRitem->Mat->MatCBIndex * matCBByteSize;

		game->getCmdList()->SetGraphicsRootDescriptorTable(0, tex);
		game->getCmdList()->SetGraphicsRootConstantBufferView(1, objCBAddress);
		game->getCmdList()->SetGraphicsRootConstantBufferView(3, matCBAddress);

		game->getCmdList()->DrawIndexedInstanced(mAircraftRitem->IndexCount, 1, mAircraftRitem->StartIndexLocation, mAircraftRitem->BaseVertexLocation, 0);
	}
}

// Builds the current aircraft
void Aircraft::buildCurrent()
{
	Game* game = mState->getContext()->game;

	auto render = std::make_unique<RenderItem>();
	renderer = render.get();
	renderer->World = getTransform();
	renderer->ObjCBIndex = (UINT) mState->getRenderItems().size();
	renderer->Mat = game->getMaterials()[mSprite].get();
	renderer->Geo = game->getGeometries()["boxGeo"].get();
	renderer->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	renderer->IndexCount = renderer->Geo->DrawArgs["box"].IndexCount;
	renderer->StartIndexLocation = renderer->Geo->DrawArgs["box"].StartIndexLocation;
	renderer->BaseVertexLocation = renderer->Geo->DrawArgs["box"].BaseVertexLocation;
	mAircraftRitem = render.get();
	mState->getRenderItems().push_back(std::move(render));
}
