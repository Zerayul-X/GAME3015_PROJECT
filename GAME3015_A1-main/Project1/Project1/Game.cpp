//***************************************************************************************
//Game.cpp
//by Zijie Wang and Wanhao Sun
//***************************************************************************************
#include "Game.hpp"
#include "GameState.hpp"
#include "TitleState.hpp"
#include "MenuState.h"
#include "PauseState.h"
#include "StateIdentifiers.hpp"

const int gNumFrameResources = 3;

// Constructor 
Game::Game(HINSTANCE hInstance)
	: D3DApp(hInstance)
	, mPlayer()
	, mStateStack(State::Context(this, &mPlayer))
{
}

// Destructor
Game::~Game()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

// Initializes the game, returns true if successful
bool Game::Initialize()
{
	// Initialize base class, return false if unsuccessful
	if (!D3DApp::Initialize())
		return false;


	// Set window caption
	mMainWndCaption = L"Assignment Solution";

	// Set camera position and orientation
	mCamera.SetPosition(0.0f, 54.0f, 0.0f);
	mCamera.Pitch(3.14f / 2.0f);

	// Reset command list
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get size of CBV/SRV descriptor heap
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildMaterials();
	RegisterStates();
	mStateStack.pushState(States::Title);
	BuildPSOs();

	// Close command list and execute on command queue
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
	return true;
}

// Called when the window is resized, updates camera lens
void Game::OnResize()
{
	// Call base class OnResize method
	D3DApp::OnResize();

	// Set camera lens
	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

// Updates the game, including state stack, camera, and constant buffers
void Game::Update(const GameTimer& gt)
{
	mStateStack.update(gt);
	mStateStack.handleRealtimeInput();

	// Quit the game if state stack is empty
	if (mStateStack.isEmpty())
	{
		PostQuitMessage(0);
		return;
	}

	UpdateCamera(gt);

	// Get the current frame resource and advance to the next one
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Wait for the previous frame resource to complete if necessary
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
}

// Draw the scene to the screen
void Game::Draw(const GameTimer& gt)
{
	// Get the current command list allocator
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reset the command list allocator
	ThrowIfFailed(cmdListAlloc->Reset());

	// Reset the command list with the opaque pipeline state object (PSO)
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mOpaquePSO.Get()));

	// Set the viewport and scissor rectangle
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Transition the back buffer to the render target state
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth stencil view
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set the render target and depth stencil view
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// Set the descriptor heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Set the root signature
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// Set the constant buffer view for the pass constant buffer
	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	// Draw the objects in the state stack
	mStateStack.draw();

	// Transition the back buffer to the present state
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Close the command list
	ThrowIfFailed(mCommandList->Close());

	// Execute the command list
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Present the current back buffer to the screen
	ThrowIfFailed(mSwapChain->Present(0, 0));

	// Update the current back buffer index
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Signal the fence value for the current frame resource.
	mCurrFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void Game::OnMouseDown(WPARAM btnState, int x, int y)
{
	// Store the current mouse position
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	// Capture the mouse so we can track its movement even if the cursor leaves the window
	SetCapture(mhMainWnd);
}

void Game::OnMouseUp(WPARAM btnState, int x, int y)
{
	// Release the mouse capture
	ReleaseCapture();
}

void Game::OnMouseMove(WPARAM btnState, int x, int y)
{
}

void Game::OnKeyDown(WPARAM btnState)
{
	// Pass the key event to the state stack for handling
	mStateStack.handleEvent(btnState);
}

// Updates the camera based on the current game timer
void Game::UpdateCamera(const GameTimer& gt)
{
	mCamera.UpdateViewMatrix();
}

void Game::AnimateMaterials(const GameTimer& gt)
{

}
// Updates the constant buffers for all objects in the current state's render items
void Game::UpdateObjectCBs(const GameTimer& gt)
{
	State* currentState = mStateStack.getCurrentState();

	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : currentState->getRenderItems())
	{
		if (e->NumFramesDirty > 0)
		{
			// Load the world and texture transform matrices for the render item
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			// Copy the world and texture transform matrices to an object constants struct
			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			// Copy the object constants to the current frame's object constant buffer at the render item's index
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Decrement the number of frames dirty for the render item
			e->NumFramesDirty--;
		}
	}
}

// Updates the constant buffers for all materials in the game
void Game::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			// Load the material transform matrix for the material
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			// Copy the material properties to a material constants struct
			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			// Copy the material constants to the current frame's material constant buffer at the material's index
			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Decrement the number of frames dirty for the material
			mat->NumFramesDirty--;
		}
	}
}

void Game::UpdateMainPassCB(const GameTimer& gt)
{
	// Get the camera's view and projection matrices
	XMMATRIX view = mCamera.GetView();
	XMMATRIX proj = mCamera.GetProj();

	// Calculate the view-projection matrix and its inverse
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	// Transpose and store the matrices in the main pass constant buffer
	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	
	// Set other properties of the main pass constant buffer
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	
	// Set properties for three lights in the main pass constant buffer
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	// Copy the data to the current frame resource's pass constant buffer
	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

// Load different textures
void Game::LoadTextures()
{
	// Load texture for Eagle
	auto EagleTex = std::make_unique<Texture>();
	EagleTex->Name = "EagleTex";
	EagleTex->Filename = L"../../Textures/Eagle.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), EagleTex->Filename.c_str(),
		EagleTex->Resource, EagleTex->UploadHeap));

	mTextures[EagleTex->Name] = std::move(EagleTex);

	// Load texture for Raptor
	auto RaptorTex = std::make_unique<Texture>();
	RaptorTex->Name = "RaptorTex";
	RaptorTex->Filename = L"../../Textures/Raptor.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), RaptorTex->Filename.c_str(),
		RaptorTex->Resource, RaptorTex->UploadHeap));

	mTextures[RaptorTex->Name] = std::move(RaptorTex);

	// Load texture for Desert
	auto DesertTex = std::make_unique<Texture>();
	DesertTex->Name = "DesertTex";
	DesertTex->Filename = L"../../Textures/Desert.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), DesertTex->Filename.c_str(),
		DesertTex->Resource, DesertTex->UploadHeap));

	mTextures[DesertTex->Name] = std::move(DesertTex);

	// Load texture for TitlePage
	auto AircraftsTexTitle = std::make_unique<Texture>();
	AircraftsTexTitle->Name = "AircraftsTexTitle";
	AircraftsTexTitle->Filename = L"../../Textures/Aircrafts_Title.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), AircraftsTexTitle->Filename.c_str(),
		AircraftsTexTitle->Resource, AircraftsTexTitle->UploadHeap));

	mTextures[AircraftsTexTitle->Name] = std::move(AircraftsTexTitle);

	// Load texture for Main Menu
	auto Aircrafts_Menu = std::make_unique<Texture>();
	Aircrafts_Menu->Name = "AircraftsTexMenu";
	Aircrafts_Menu->Filename = L"../../Textures/Aircrafts_Menu.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), Aircrafts_Menu->Filename.c_str(),
		Aircrafts_Menu->Resource, Aircrafts_Menu->UploadHeap));

	mTextures[Aircrafts_Menu->Name] = std::move(Aircrafts_Menu);

	// Load texture for PauseMenu
	auto AircraftsTexPause = std::make_unique<Texture>();
	AircraftsTexPause->Name = "AircraftsTexPause";
	AircraftsTexPause->Filename = L"../../Textures/Aircrafts_Pause.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), AircraftsTexPause->Filename.c_str(),
		AircraftsTexPause->Resource, AircraftsTexPause->UploadHeap));

	mTextures[AircraftsTexPause->Name] = std::move(AircraftsTexPause);
}
// Creates a textureand adds it to the list of textures for the game
void Game::CreateTexture(std::string Name, std::wstring FileName)
{
	auto texture = std::make_unique<Texture>();
	texture->Name = Name;
	texture->Filename = FileName;
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), texture->Filename.c_str(),
		texture->Resource, texture->UploadHeap));
	mTextures[texture->Name] = std::move(texture);
}

// Builds the root signature used by the graphics pipeline
void Game::BuildRootSignature()
{
	// Create a descriptor range for the texture table
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// Initialize the root parameters for the root signature
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	// Get the static samplers for the root signature
	auto staticSamplers = GetStaticSamplers();

	// Initialize the root signature descriptor
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),  
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	// Output any error messages from the root signature serialization
	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	// Create the root signature
	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}
// Creates a descriptor heap for storing shader resource views (SRVs)
void Game::BuildDescriptorHeaps()
{
	// Describe the heap by setting the number of descriptors, type, and flags
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 6;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	
	// Create the descriptor heap using the description and store it in a member variable
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Retrieve the resource of each texture from the texture map using its name
	auto EagleTex = mTextures["EagleTex"]->Resource;
	auto RaptorTex = mTextures["RaptorTex"]->Resource;
	auto DesertTex = mTextures["DesertTex"]->Resource;
	auto AircraftsTexTitle = mTextures["AircraftsTexTitle"]->Resource;
	auto AircraftsTexMenu = mTextures["AircraftsTexMenu"]->Resource;
	auto AircraftsTexPause = mTextures["AircraftsTexPause"]->Resource;

	// Describe the SRV using a descriptor and create it for each texture
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	// Set the format, view dimension, most detailed mip level, and mip levels of the SRV for the Eagle texture
	srvDesc.Format = EagleTex->GetDesc().Format;

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;

	srvDesc.Texture2D.MipLevels = EagleTex->GetDesc().MipLevels;

	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(EagleTex.Get(), &srvDesc, hDescriptor);

	// Offset the descriptor handle to the next descriptor and create an SRV for the Raptor texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	srvDesc.Format = RaptorTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(RaptorTex.Get(), &srvDesc, hDescriptor);

	// Desert texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	srvDesc.Format = DesertTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(DesertTex.Get(), &srvDesc, hDescriptor);

	// AircraftsTexTitle texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	srvDesc.Format = AircraftsTexTitle->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(AircraftsTexTitle.Get(), &srvDesc, hDescriptor);

	// AircraftsTexMenu texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	srvDesc.Format = AircraftsTexMenu->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(AircraftsTexMenu.Get(), &srvDesc, hDescriptor);

	// AircraftsTexPause texture
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	srvDesc.Format = AircraftsTexPause->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(AircraftsTexPause.Get(), &srvDesc, hDescriptor);
}

void Game::BuildShadersAndInputLayout()
{
	// Compile vertex and pixel shaders and store them in the shaders map
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

	// Define the input layout for the vertices
	// we define the structure of each vertex in our mesh
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void Game::BuildShapeGeometry()
{
	// Create a box mesh using the GeometryGenerator class
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1, 0, 1, 1);
	
	// Define a submesh for the box
	// This submesh will reference the vertices and indices used to draw the box
	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.BaseVertexLocation = 0;


	// Convert the mesh data into our own custom vertex format
	// We create a vector of Vertex objects and copy over the position, normal, and texture coordinates from the mesh data
	std::vector<Vertex> vertices(box.Vertices.size());
	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	// Get the index data for the box mesh
	// We store the indices as a vector of 16-bit integers
	std::vector<std::uint16_t> indices = box.GetIndices16();

	// Calculate the byte sizes for the vertex and index buffers
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	// Create a new MeshGeometry object to store the box mesh data
	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	// Create a CPU-visible buffer to store the vertex data and copy the data over
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	// Create a CPU-visible buffer to store the index data and copy the data over
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	// Create a GPU-visible buffer to store the vertex data and upload the data from the CPU buffer to the GPU buffer
	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	// Add the submesh named "box" and its corresponding draw arguments to the geometry object
	geo->DrawArgs["box"] = boxSubmesh;

	// Add the geometry object to the map of geometries with its name as the key 
	mGeometries[geo->Name] = std::move(geo);
}

void Game::BuildPSOs()
{
	// Create a description for the opaque graphics pipeline state object (PSO)
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	// Set all members of the description to zero
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	// Set the input layout for the PSO using the array of D3D12_INPUT_ELEMENT_DESC objects
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	// Set the root signature for the PSO 
	opaquePsoDesc.pRootSignature = mRootSignature.Get();

	// Set the vertex shader for the PSO using the bytecode and size of the shader
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};

	// Set the pixel shader for the PSO using the bytecode and size of the shader
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	// Set the rasterizer state to default
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	// Set the blend state to default
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	// Set the depth stencil state to default
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	// Set the sample mask to maximum unsigned integer value
	opaquePsoDesc.SampleMask = UINT_MAX;
	// Set the primitive topology type to triangle
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// Set the number of render targets to 1
	opaquePsoDesc.NumRenderTargets = 1;
	// Set the format of the render target view to the back buffer format of the swap chain
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	// Set the number of samples per pixel to 4 if 4x MSAA is enabled, otherwise 1
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	// Set the quality level of the samples to the maximum value if 4x MSAA is enabled, otherwise 0
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	// Set the format of the depth stencil view to the depth stencil format
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	// Create the opaque PSO using the PSO description and the device, and store it in the member variable
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mOpaquePSO)));
}

void Game::BuildFrameResources(int renderItemCount)
{

	// Create gNumFrameResources number of unique frame resources
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)renderItemCount, (UINT)mMaterials.size()));
		// Each frame resource consists of one constant buffer and a number of render items
		// The render item count is passed as a parameter to this function
		// The number of unique materials is also stored in the frame resource
	}
}

// Clear the list of frame resources
void Game::ResetFrameResources()
{
	mFrameResources.clear();
}

// Create materials with different properties
void Game::BuildMaterials()
{
	mCurrentMaterialCBIndex = 0;
	mCurrentDiffuseSrvHeapIndex = 0;
	// Create materials with specific properties for different objects in the scene
	CreateMaterials("Eagle", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.2f);
	CreateMaterials("Raptor", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.2f);
	CreateMaterials("Desert", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.2f);
	CreateMaterials("Aircrafts_Title", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.2f);
	CreateMaterials("Aircrafts_Menu", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.2f);
	CreateMaterials("Aircrafts_Pause", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.05f, 0.05f, 0.05f), 0.2f);
}

void Game::CreateMaterials(std::string Name, XMFLOAT4 DiffuseAlbedo, XMFLOAT3 FresnelR0, float Roughness)
{
	// Create a unique pointer to a new material object
	auto material = std::make_unique<Material>();

	// Set the material's properties based on the parameters passed in
	material->Name = Name;
	material->MatCBIndex = mCurrentMaterialCBIndex++;
	material->DiffuseSrvHeapIndex = mCurrentDiffuseSrvHeapIndex++;
	material->DiffuseAlbedo = DiffuseAlbedo;
	material->FresnelR0 = FresnelR0;
	material->Roughness = Roughness;

	// Add the material to the game's collection of materials
	mMaterials[Name] = std::move(material);
}

void Game::RegisterStates()
{
	// Register the TitleState, GameState, MenuState, and PauseState classes with the state stack,
	// using their respective enum values as keys.
	mStateStack.registerState<TitleState>(States::Title);
	mStateStack.registerState<GameState>(States::Game);
	mStateStack.registerState<MenuState>(States::Menu);
	mStateStack.registerState<PauseState>(States::Pause);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Game::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}
