#pragma once

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"

// 오브젝트 상수 버퍼
struct ObjectConstants
{
	DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
};

// 재질 상수 버퍼
struct MatConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FreshnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;
};

// 패스 상수 버퍼
struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;

	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	Light Lights[MaxLights];
};

class FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT matCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource* operator=(const FrameResource& rhs) = delete;
	~FrameResource();

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
	std::unique_ptr<UploadBuffer<MatConstants>> MatCB = nullptr;	// 재질 상수 버퍼

	UINT64 Fence = 0;
};

