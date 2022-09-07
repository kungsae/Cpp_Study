//***************************************************************************************
// GeometryApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Shows how to draw a box in Direct3D 12.
//
// Controls:
//   Hold the left mouse button down and move the mouse to rotate.
//   Hold the right mouse button down and move the mouse to zoom in and out.
//***************************************************************************************

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"

#include "FrameResource.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

// 정점 구조체
struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT3 Normal;
};

static const int gNumFrameResources = 3;

struct RenderItem
{
    RenderItem() = default;

    XMFLOAT4X4 World = MathHelper::Identity4x4();

    // 물체의 갱신 여부 변수
    int NumFramesDirty = gNumFrameResources;

    // 물체 상수 버퍼의 색인
    UINT ObjCBIndex = -1;

    MeshGeometry* Geo = nullptr;        // 기하 도형 정보
    Material* Mat = nullptr;            // 재질 정보

    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

class GeometryApp : public D3DApp
{
public:
    GeometryApp(HINSTANCE hInstance);
    GeometryApp(const GeometryApp& rhs) = delete;
    GeometryApp& operator=(const GeometryApp& rhs) = delete;
    ~GeometryApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void OnKeyboardInput(const GameTimer& gt);
    void UpdateCamera(const GameTimer& gt);
    void UpdateObjectCBs(const GameTimer& gt);
    void UpdateMainPassCB(const GameTimer& gt);
    void UpdateMaterialCBs(const GameTimer& gt);

    void BuildDescriptorHeaps();
    void BuildConstantBufferViews();        // 상수 버퍼 뷰
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildShapeGeometry();              // 기하도형 생성
    void BuildMaterials();                  // 재질 생성
    void BuildPSO();

    void BuildFrameResources();
    void BuildRenderItems();                // 렌더링할 기하도형 정보 셋팅

    // 기하도형 렌더링
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

private:
    // 루트 시그니처
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

    // 상수 버퍼 서술자 (파이프라인에 Binding되기 위한 Desc)
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

    // 입력 배치
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    // 패스별 상수버퍼의 시작 오프셋 저장 변수(마지막 세 서술자)
    UINT mPassCbvOffset = 0;

    // 재질 상수 버퍼의 시작 오프셋
    UINT mMatCbvOffset = 0;

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;     // 기하도형
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;          // 재질 정보
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;                     // 셰이더 맵
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;             // 렌더링파이프라인 맵

    // 프레임 자원과 현재 프레임 자원을 가리키는 멤버 변수
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    // 렌더 항목 목록
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;

    // PSO 렌더 항목
    std::vector<RenderItem*> mOpaqueRitems;
    std::vector<RenderItem*> mTransparentRitems;

    bool mIsWireframe = false;

    // 월드 / 시야 / 투영 행렬
    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };

    // 구면 좌표 제어 값
    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;

    // 마우스 좌표
    POINT mLastMousePos = { 0, 0 };

};

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        GeometryApp theApp(hInstance);
        if (!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

GeometryApp::GeometryApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

GeometryApp::~GeometryApp()
{
}

bool GeometryApp::Initialize()
{
    // Direct3D 초기화
    if (!D3DApp::Initialize())
        return false;

    // 초기화 명령들을 준비하기 위해 명령 목록을 재설정 한다.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // 초기화 명령들
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildShapeGeometry();
    BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildDescriptorHeaps();
    BuildConstantBufferViews();
    BuildPSO();

    // 초기화 명령들을 실행한다.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 초기화가 완료 될 때까지 기다린다.
    FlushCommandQueue();

    return true;
}

void GeometryApp::OnResize()
{
    D3DApp::OnResize();

    // 창의 크기가 바뀌었으므로 종횡비를 갱신하고 투영 행렬을 다시 계산한다.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void GeometryApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
    UpdateCamera(gt);

    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // GPU가 현재 프레임 자원의 명령들을 다 처리했는지 확인 코드
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    UpdateObjectCBs(gt);
    UpdateMainPassCB(gt);
    UpdateMaterialCBs(gt);
}

void GeometryApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
    ThrowIfFailed(cmdListAlloc->Reset());

    if (mIsWireframe)
    {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["wireframe"].Get()));
    }
    else
    {
        ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
    }

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // 자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // 새로 그리기 위해 백버퍼와 깊이 버퍼를 지운다.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    /// 렌더링 결과가 기록될 렌더 대상 버퍼들을 지정한다.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    // 루트 시그니처와 서술자 힙을 설정한다.
    ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    int passCbvIndex = mPassCbvOffset + mCurrFrameResourceIndex;
    auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(2, passCbvHandle);

    // 렌더 아이템 렌더링
    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);    

    // 자원 용도에 관련된 상태 전이를 Direct3D에 통보한다.
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // 명령들의 기록을 마친다.
    ThrowIfFailed(mCommandList->Close());

    // 명령 실행을 위해 명령 목록을 명령 대기열에 추가한다.
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 후면 버퍼와 전면 버퍼를 교환한다.
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // 현재 울타리 지점까지의 명령들을 표시하도록 울타리 값을 전진
    mCurrFrameResource->Fence = ++mCurrentFence;

    // 새 울타리 지점을 설정하는 Signal을 명령대기열에 추가한다.
    // 새 울타리 지점은 GPU가 이 Signal() 명령까지의 모든 명령을 처리하기 전까지 대기
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void GeometryApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void GeometryApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void GeometryApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        // 마우스 한 픽셀 이동을 4분의 1도에 대응시킨다.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

        // 마우스 입력에 기초해 각도를 갱신한다. 카메라가 상자를 중심으로 공전한다.
        mTheta += dx;
        mPhi += dy;

        // 파이 각도를 제한한다.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // 마우스 한 픽셀 이동을 장면의 0.005단위에 대응시킨다.
        float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

        // 마우스 입력에 기초해서 카메라 반지름을 갱신한다.
        mRadius += dx - dy;

        // 반지름을 제한한다.
        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void GeometryApp::OnKeyboardInput(const GameTimer& gt)
{
    if (GetAsyncKeyState('1') & 0x8000)
        mIsWireframe = true;
    else
        mIsWireframe = false;
}

void GeometryApp::UpdateCamera(const GameTimer& gt)
{
    // 구면 좌표를 직교 좌표로 변환한다.
    mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
    mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
    mEyePos.y = mRadius * cosf(mPhi);

    // 시야 행렬을 구축한다.
    XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);
}
void GeometryApp::UpdateObjectCBs(const GameTimer& gt)
{
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();

    for (auto& item : mAllRitems)
    {
        if (item->NumFramesDirty)
        {
            XMMATRIX world = XMLoadFloat4x4(&item->World);

            ObjectConstants objConstants;
            XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));

            currObjectCB->CopyData(item->ObjCBIndex, objConstants);

            item->NumFramesDirty--;
        }
    }
}
void GeometryApp::UpdateMainPassCB(const GameTimer& gt)
{
    PassConstants mMainPassCB;

    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

    mMainPassCB.EyePosW = mEyePos;

    mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };

    mMainPassCB.Lights[5].Strength = { 0.9f, 0.9f, 0.9f };
    mMainPassCB.Lights[5].Position = { 0.0f,10.0f,5.0f };
    mMainPassCB.Lights[5].Direction = { 0.0f, -1.0f,0.0f};
    mMainPassCB.Lights[5].FalloffStart = 10;
    mMainPassCB.Lights[5].FalloffEnd = 20;
    mMainPassCB.Lights[5].SpotPower = 64;




    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void GeometryApp::UpdateMaterialCBs(const GameTimer& gt)
{
    auto currMaterialCB = mCurrFrameResource->MatCB.get();
    for (auto& e : mMaterials)
    {
        Material* mat = e.second.get();
        if (mat->NumFramesDirty > 0)
        {
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

            MatConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FreshnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;

            currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

            mat->NumFramesDirty--;
        }
    }
}

void GeometryApp::BuildDescriptorHeaps()
{
    UINT objCount = (UINT)mOpaqueRitems.size();
    UINT matCount = (UINT)mMaterials.size();

    UINT numDescriptors = (objCount + matCount + 1) * gNumFrameResources;

    // 머티리얼 상수 버퍼의 오프셋(시작위치)
    mMatCbvOffset = objCount * gNumFrameResources;
    // 패스 상수 버퍼의 오프셋(시작위치)
    mPassCbvOffset = (objCount + matCount) * gNumFrameResources;

    // 상수 버퍼 서슬자
    // 루프 매개변수에 쓰기 위한 서술자 힙을 만들다.
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = numDescriptors;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
        IID_PPV_ARGS(&mCbvHeap)));
}

void GeometryApp::BuildConstantBufferViews()
{
    // 상수버퍼 하나의 크기
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    // 오브젝트 갯수
    UINT objCount = (UINT)mOpaqueRitems.size();

    // 각 프레임 자원의 물체마다 하나씩 CBV 서술자가 필요(오브젝트의 상수 버퍼 뷰)
    for (int frameindex = 0; frameindex < gNumFrameResources; ++frameindex)
    {
        auto objectCB = mFrameResources[frameindex]->ObjectCB->Resource();
        for (UINT i = 0; i < objCount; ++i)
        {
            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();

            // i번째 물체별 상수 버퍼의 오프셋
            cbAddress += i * objCBByteSize;

            // 서술자 힙에서 i번째 물체별 CBV의 오프셋
            int heapIndex = frameindex * objCount + i;
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = cbAddress;
            cbvDesc.SizeInBytes = objCBByteSize;

            md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    // 머티리얼(재질) 상수 버프 크기
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MatConstants));
    // 머티리얼(재질) 갯수
    UINT matCount = (UINT)mMaterials.size();

    // 각 프레임 자원의 물체마다 하나씩 CBV 서술자가 필요(오브젝트의 상수 버퍼 뷰)
    for (int frameindex = 0; frameindex < gNumFrameResources; ++frameindex)
    {
        auto materialCB = mFrameResources[frameindex]->MatCB->Resource();
        for (UINT i = 0; i < matCount; ++i)
        {
            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = materialCB->GetGPUVirtualAddress();

            // i번째 물체별 상수 버퍼의 오프셋
            cbAddress += i * matCBByteSize;

            // 서술자 힙에서 i번째 물체별 CBV의 오프셋
            int heapIndex = mMatCbvOffset + frameindex * matCount + i;
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = cbAddress;
            cbvDesc.SizeInBytes = matCBByteSize;

            md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    // 패스별 상수 버퍼 뷰
    UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

    for (int frameindex = 0; frameindex < gNumFrameResources; ++frameindex)
    {
        auto passCB = mFrameResources[frameindex]->PassCB->Resource();

        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

        int heapIndex = mPassCbvOffset + frameindex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = passCBByteSize;

        md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
    }
}

void GeometryApp::BuildRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);  // b0

    CD3DX12_DESCRIPTOR_RANGE cbvTable1;
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);  // b1

    CD3DX12_DESCRIPTOR_RANGE cbvTable2;
    cbvTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);  // b2

    CD3DX12_ROOT_PARAMETER slotRootParameter[3];

    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);
    slotRootParameter[2].InitAsDescriptorTable(1, &cbvTable2);

    // 루트 시그니처는 루트 매개변수들의 배열
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // 상수 버퍼 하나로 구성된 서술자 구간을 가리키는 슬롯 하나로 이루어진 루트 시그니처를 생성한다.
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr)
    {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(hr);

    ThrowIfFailed(md3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&mRootSignature)));
}

void GeometryApp::BuildShadersAndInputLayout()
{
    // 쉐이더 프로그램을 빌드하고, 입력 레이아웃에 대한 세부 요소들이 무엇인지 설정한다.
    HRESULT hr = S_OK;
    mShaders["standardVS"] = d3dUtil::CompileShader(TEXT("Shaders\\Spot.hlsl"), nullptr, "VS", "vs_5_1");
    mShaders["opaquePS"] = d3dUtil::CompileShader(TEXT("Shaders\\Spot.hlsl"), nullptr, "PS", "ps_5_1");
    
    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void GeometryApp::BuildShapeGeometry()
{
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
    GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    //
    // We are concatenating all the geometry into one big vertex/index buffer.  So
    // define the regions in the buffer each submesh covers.
    //

    // Cache the vertex offsets to each object in the concatenated vertex buffer.
    UINT boxVertexOffset = 0;
    UINT gridVertexOffset = (UINT)box.Vertices.size();
    UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
    UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

    // Cache the starting index for each object in the concatenated index buffer.
    UINT boxIndexOffset = 0;
    UINT gridIndexOffset = (UINT)box.Indices32.size();
    UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
    UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

    // Define the SubmeshGeometry that cover different 
    // regions of the vertex/index buffers.

    SubmeshGeometry boxSubmesh;
    boxSubmesh.IndexCount = (UINT)box.Indices32.size();
    boxSubmesh.StartIndexLocation = boxIndexOffset;
    boxSubmesh.BaseVertexLocation = boxVertexOffset;

    SubmeshGeometry gridSubmesh;
    gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
    gridSubmesh.StartIndexLocation = gridIndexOffset;
    gridSubmesh.BaseVertexLocation = gridVertexOffset;

    SubmeshGeometry sphereSubmesh;
    sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
    sphereSubmesh.StartIndexLocation = sphereIndexOffset;
    sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

    SubmeshGeometry cylinderSubmesh;
    cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
    cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
    cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

    //
    // Extract the vertex elements we are interested in and pack the
    // vertices of all the meshes into one vertex buffer.
    //

    auto totalVertexCount =
        box.Vertices.size() +
        grid.Vertices.size() +
        sphere.Vertices.size() +
        cylinder.Vertices.size();

    std::vector<Vertex> vertices(totalVertexCount);

    UINT k = 0;
    for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = box.Vertices[i].Position;
        vertices[k].Normal = box.Vertices[i].Normal;
    }

    for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = grid.Vertices[i].Position;
        vertices[k].Normal = grid.Vertices[i].Normal;
    }

    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Normal = sphere.Vertices[i].Normal;
    }

    for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
    {
        vertices[k].Pos = cylinder.Vertices[i].Position;
        vertices[k].Normal = cylinder.Vertices[i].Normal;
    }

    std::vector<std::uint16_t> indices;
    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
    indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
    indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->Name = "shapeGeo";

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
    CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
    CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

    geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

    geo->VertexByteStride = sizeof(Vertex);
    geo->VertexBufferByteSize = vbByteSize;
    geo->IndexFormat = DXGI_FORMAT_R16_UINT;
    geo->IndexBufferByteSize = ibByteSize;

    geo->DrawArgs["box"] = boxSubmesh;
    geo->DrawArgs["grid"] = gridSubmesh;
    geo->DrawArgs["sphere"] = sphereSubmesh;
    geo->DrawArgs["cylinder"] = cylinderSubmesh;

    mGeometries[geo->Name] = std::move(geo);
}

void GeometryApp::BuildMaterials()
{
    auto bricks0 = std::make_unique<Material>();
    bricks0->Name = "bricks0";
    bricks0->MatCBIndex = 0;
    bricks0->DiffuseSrvHeapIndex = 0;
    bricks0->DiffuseAlbedo = XMFLOAT4(Colors::ForestGreen);
    bricks0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    bricks0->Roughness = 0.1f;

    auto stone0 = std::make_unique<Material>();
    stone0->Name = "stone0";
    stone0->MatCBIndex = 1;
    stone0->DiffuseSrvHeapIndex = 1;
    stone0->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
    stone0->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    stone0->Roughness = 0.3f;

    auto tile0 = std::make_unique<Material>();
    tile0->Name = "tile0";
    tile0->MatCBIndex = 2;
    tile0->DiffuseSrvHeapIndex = 2;
    tile0->DiffuseAlbedo = XMFLOAT4(Colors::LightGray);
    tile0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    tile0->Roughness = 0.2f;

    auto skullMat = std::make_unique<Material>();
    skullMat->Name = "skullMat";
    skullMat->MatCBIndex = 3;
    skullMat->DiffuseSrvHeapIndex = 3;
    skullMat->DiffuseAlbedo = XMFLOAT4(Colors::White);
    skullMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    skullMat->Roughness = 0.3f;

    mMaterials["bricks0"] = std::move(bricks0);
    mMaterials["stone0"] = std::move(stone0);
    mMaterials["tile0"] = std::move(tile0);
    mMaterials["skullMat"] = std::move(skullMat);
}

void GeometryApp::BuildPSO()
{
    // 파이프라인 상태를 생성한다.
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), mShaders["standardVS"]->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()), mShaders["opaquePS"]->GetBufferSize()
    };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;   // 채워진 폴리곤
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC WireframePSODesc = psoDesc;
    WireframePSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;   // 와이어 프레임
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&WireframePSODesc, IID_PPV_ARGS(&mPSOs["wireframe"])));
}

void GeometryApp::BuildFrameResources()
{
    for (int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mAllRitems.size(), (UINT)mMaterials.size()));
    }
}

void GeometryApp::BuildRenderItems()
{
    auto boxRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&boxRitem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    boxRitem->ObjCBIndex = 0;
    boxRitem->Geo = mGeometries["shapeGeo"].get();
    boxRitem->Mat = mMaterials["stone0"].get();
    boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
    boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
    boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
    mAllRitems.push_back(std::move(boxRitem));

    auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->World = MathHelper::Identity4x4();
    gridRitem->ObjCBIndex = 1;
    gridRitem->Geo = mGeometries["shapeGeo"].get();
    gridRitem->Mat = mMaterials["tile0"].get();
    gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
    mAllRitems.push_back(std::move(gridRitem));

    UINT objCBIndex = 2;
    for (int i = 0; i < 5; ++i)
    {
        auto leftCylRitem = std::make_unique<RenderItem>();
        auto rightCylRitem = std::make_unique<RenderItem>();
        auto leftSphereRitem = std::make_unique<RenderItem>();
        auto rightSphereRitem = std::make_unique<RenderItem>();

        XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
        XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);

        XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
        XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

        XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
        leftCylRitem->ObjCBIndex = objCBIndex++;
        leftCylRitem->Geo = mGeometries["shapeGeo"].get();
        leftCylRitem->Mat = mMaterials["bricks0"].get();
        leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
        leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

        XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
        rightCylRitem->ObjCBIndex = objCBIndex++;
        rightCylRitem->Geo = mGeometries["shapeGeo"].get();
        rightCylRitem->Mat = mMaterials["bricks0"].get();
        rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
        rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
        rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

        XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
        leftSphereRitem->ObjCBIndex = objCBIndex++;
        leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
        leftSphereRitem->Mat = mMaterials["stone0"].get();
        leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
        leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

        XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
        rightSphereRitem->ObjCBIndex = objCBIndex++;
        rightSphereRitem->Geo = mGeometries["shapeGeo"].get();
        rightSphereRitem->Mat = mMaterials["stone0"].get();
        rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
        rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
        rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

        mAllRitems.push_back(std::move(leftCylRitem));
        mAllRitems.push_back(std::move(rightCylRitem));
        mAllRitems.push_back(std::move(leftSphereRitem));
        mAllRitems.push_back(std::move(rightSphereRitem));
    }

    // All the render items are opaque.
    for (auto& e : mAllRitems)
        mOpaqueRitems.push_back(e.get());
}

void GeometryApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MatConstants));

    auto objectCB = mCurrFrameResource->ObjectCB->Resource();
    auto matCB = mCurrFrameResource->MatCB->Resource();

    for (size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        UINT cbvIndex = mCurrFrameResourceIndex * (UINT)mOpaqueRitems.size() + ri->ObjCBIndex;
        auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHandle.Offset(cbvIndex, mCbvSrvUavDescriptorSize);

        UINT matIndex = mMatCbvOffset + mCurrFrameResourceIndex * (UINT)mMaterials.size() + ri->Mat->MatCBIndex;
        auto matHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
        matHandle.Offset(matIndex, mCbvSrvUavDescriptorSize);

        cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
        cmdList->SetGraphicsRootDescriptorTable(1, matHandle);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}
