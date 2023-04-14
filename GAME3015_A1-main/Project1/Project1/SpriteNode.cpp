//***************************************************************************************
// SpriteNode.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************
#include "SpriteNode.h"
#include "Game.hpp"
#include "State.hpp"

// Constructor for SpriteNode
SpriteNode::SpriteNode(State* state) : Entity(state)
{

}


// Draws the SpriteNode
void SpriteNode::drawCurrent() const
{
	// Calculate the size of the object and material constant buffers
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	// Get the current frame resource and the game instance from the state
	Game* game = mState->getContext()->game;

	// Get the object and material constant buffers from the current frame resource
	auto objectCB = game->mCurrFrameResource->ObjectCB->Resource();
	auto matCB = game->mCurrFrameResource->MaterialCB->Resource();

	// Check if the render item for the SpriteNode exists
	if (mSpriteNodeRitem != nullptr)
	{
		// Set the vertex buffer, index buffer, and primitive topology for the render item
		game->getCmdList()->IASetVertexBuffers(0, 1, &mSpriteNodeRitem->Geo->VertexBufferView());
		game->getCmdList()->IASetIndexBuffer(&mSpriteNodeRitem->Geo->IndexBufferView());
		game->getCmdList()->IASetPrimitiveTopology(mSpriteNodeRitem->PrimitiveType);

		// Get the GPU descriptor handle for the diffuse texture and offset it based on the index in the material heap
		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(game->mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(mSpriteNodeRitem->Mat->DiffuseSrvHeapIndex, game->mCbvSrvDescriptorSize);

		// Get the GPU virtual address for the object and material constant buffers based on the object and material index
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + mSpriteNodeRitem->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + mSpriteNodeRitem->Mat->MatCBIndex * matCBByteSize;

		// Set the descriptor table and constant buffer views for the root signature
		game->getCmdList()->SetGraphicsRootDescriptorTable(0, tex);
		game->getCmdList()->SetGraphicsRootConstantBufferView(1, objCBAddress);
		game->getCmdList()->SetGraphicsRootConstantBufferView(3, matCBAddress);

		// Draw the indexed instanced primitives
		game->getCmdList()->DrawIndexedInstanced(mSpriteNodeRitem->IndexCount, 1, mSpriteNodeRitem->StartIndexLocation, mSpriteNodeRitem->BaseVertexLocation, 0);
	}

}

// Builds the current SpriteNode
void SpriteNode::buildCurrent()
{
	// Get the game instance from the state
	Game* game = mState->getContext()->game;

	// Create a new render item
	auto render = std::make_unique<RenderItem>();
	renderer = render.get();

	// Set the properties of the render item
	renderer->World = getTransform();
	XMStoreFloat4x4(&renderer->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	renderer->ObjCBIndex = (UINT) mState->getRenderItems().size();
	renderer->Mat = game->getMaterials()[mMat].get(); //"Desert"
	renderer->Geo = game->getGeometries()[mGeo].get(); //"boxGeo"
	renderer->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	renderer->IndexCount = renderer->Geo->DrawArgs[mDrawName].IndexCount; //"box"
	renderer->StartIndexLocation = renderer->Geo->DrawArgs[mDrawName].StartIndexLocation;
	renderer->BaseVertexLocation = renderer->Geo->DrawArgs[mDrawName].BaseVertexLocation;
	mSpriteNodeRitem = render.get();
	mState->getRenderItems().push_back(std::move(render));
}

// Set the material, geometry, and draw name for the sprite node
void SpriteNode::SetMatGeoDrawName(std::string Mat, std::string Geo, std::string DrawName)
{
	mMat = Mat;
	mGeo = Geo;
	mDrawName = DrawName;
}

