
// =====おまじない=====
#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<d3dcompiler.h>
#ifdef _DEBUG
#include<iostream>
#endif
#include<vector>
#include<string>
using namespace std;
using namespace DirectX;
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
//=====================

//===メインコード===


//Window 
const unsigned int window_width = 1280;
const unsigned int window_height = 720;
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//Debug
void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}

//DirectX12 values
IDXGIFactory6* _dxgiFactory = nullptr;
ID3D12Device* _dev = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

#ifdef _DEBUG
int main() {
#else
#include<Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif

	//===========Window Setting=================
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;
	w.lpszClassName = _T("DirectXTest");
	w.hInstance = GetModuleHandle(0);
	RegisterClassEx(&w);

	RECT wrc = { 0,0, window_width, window_height };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	HWND hwnd = CreateWindow(w.lpszClassName,
		_T("DirectXTest"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		w.hInstance,
		nullptr);
	//========================================
	
	//===========Debug Layer Enable===============
#ifdef _DEBUG
	EnableDebugLayer();
#endif
	//========================================
	
	//===========Device Setting================
	HRESULT result = S_OK;
	//Adapter Search
#ifdef _DEBUG
	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else 
	result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif
	IDXGIAdapter* tmpAdapter = nullptr;
	_dxgiFactory->EnumAdapterByGpuPreference(
		0,                                      // 0番目（つまり一番性能が高いGPU）を要求
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,   // 高性能なGPUを優先して探す設定
		IID_PPV_ARGS(&tmpAdapter)
	);
	DXGI_ADAPTER_DESC desc = {};
	tmpAdapter->GetDesc(&desc);

	//Device Create
	result = D3D12CreateDevice(tmpAdapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&_dev));
	//=========================================


	//===========Command Setting================
//Command Setting
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
	//=========================================


	// ===========Render Target Setting================
	//swapchain Create
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);

	// Render Target View Descriptor Heap Create
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvhandle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;// gamma correction (sRGB)
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < swcDesc.BufferCount; i++)
	{
		result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&_backBuffers[i]));
		_dev->CreateRenderTargetView(_backBuffers[i], &rtvDesc, rtvhandle);
		rtvhandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	//=========================================
	
	//===========Fence Setting================
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	//=========================================


	ShowWindow(hwnd, SW_SHOW);

	//===========Vertex Buffer Setting================

	XMFLOAT3 vertices[] = {
	{-0.5f,-0.9f,0.0f},
	{-0.5f,0.9f,0.0f} ,
	{0.5f,-0.9f,0.0f},
	{0.5f,0.9f,0.0f},
	};

	D3D12_HEAP_PROPERTIES vertheapprop = {};
	vertheapprop.Type = D3D12_HEAP_TYPE_UPLOAD; 
	vertheapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	vertheapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC vertresdesc = {};
	vertresdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertresdesc.Width = sizeof(vertices);
	vertresdesc.Height = 1;
	vertresdesc.DepthOrArraySize = 1;
	vertresdesc.MipLevels = 1;
	vertresdesc.Format = DXGI_FORMAT_UNKNOWN;
	vertresdesc.SampleDesc.Count = 1;
	vertresdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	vertresdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertBuff = nullptr;
	result = _dev->CreateCommittedResource(
		&vertheapprop,
		D3D12_HEAP_FLAG_NONE,
		&vertresdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	XMFLOAT3* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	// Vertex Buffer View Create
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeof(vertices);
	vbView.StrideInBytes = sizeof(vertices[0]);


	//index 
	unsigned short indices[] = { 0,1,2, 2,1,3 };
	// Index Buffer Create
	ID3D12Resource* idxBuff = nullptr;
	vertresdesc.Width = sizeof(indices);
	result = _dev->CreateCommittedResource(
		&vertheapprop,
		D3D12_HEAP_FLAG_NONE,
		&vertresdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	//=========================================

	// ===========Graphics Pipeline Setting================

	//-----------Shader Compile-----------
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_vsBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("FILE NOT FOUND");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}
	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("FILE NOT FOUND");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);
	}
	//------------------------------------------
	

	//----------Input Layout Create----------
	//input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};
	//----------------------------------------------

	//-----------Root Signature Create-----------
	ID3D12RootSignature* rootsignature = nullptr;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	result = _dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	rootSigBlob->Release();
	//------------------------------------------

	//-----------Pipeline State Object Create-----------
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};

	// 1. Root Signature (パイプライン全体のリソース定義)
	gpipeline.pRootSignature = rootsignature;

	// 2. Input Assembler (IA) ステージ: 頂点データの入力設定
	gpipeline.InputLayout.pInputElementDescs = inputLayout;
	gpipeline.InputLayout.NumElements = _countof(inputLayout);
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	// 3. Vertex Shader (VS) ステージ: 頂点の座標変換
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();

	// 4. Rasterizer (RS) ステージ: ポリゴンをピクセルに分割
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpipeline.RasterizerState.DepthClipEnable = true;
	gpipeline.RasterizerState.MultisampleEnable = false;

	// 5. Pixel Shader (PS) ステージ: ピクセルの色を計算
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	// 6. Output Merger (OM) ステージ: 最終的な色の書き込み・ブレンド
	// Render Target Settings
	gpipeline.NumRenderTargets = 1;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// Blend State
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// 7. Multi-Sampling (アンチエイリアスなどの設定)
	gpipeline.SampleDesc.Count = 1;
	gpipeline.SampleDesc.Quality = 0;
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// Pipeline State Object Create
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));
	//------------------------------------------
	//=========================================

	//=========== View Port and Scissor Rect Setting================
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;
	viewport.Height = window_height;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;

	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;
	scissorrect.left = 0;
	scissorrect.right = scissorrect.left + window_width;
	scissorrect.bottom = scissorrect.top + window_height;
	//=========================================


	//===========Game Loop===========================
	MSG msg = {};
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) {
			break;
		}
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			break;
		}


		// Back Buffer Render Target Transition
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		//------Resource Barrier------
		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;// Transition
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx]; // Back Buffer
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;// Present状態から
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;// Render Target状態へ
		_cmdList->ResourceBarrier(1, &BarrierDesc);
		//------------------------------------

		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += static_cast<ULONG_PTR>(bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
		_cmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);


		//----------Render Operations----------
		//Clear Render Target
		float r, g, b;
		r = (float)(0.1);
		g = (float)(0.1);
		b = (float)(0.1);
		float clearColor[] = { r,g,b,1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		
		//set pipeline state
		_cmdList->SetPipelineState(_pipelinestate);
		//set root signature
		_cmdList->SetGraphicsRootSignature(rootsignature);

		//set viewport and scissor rect
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		//set primitive topology
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//set vertex and index buffer
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);
		//draw call
		//_cmdList->DrawInstanced(3, 1, 0, 0);
		_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);


		//-------------------------------------

		//===Resource Barrier===
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;// Render Target状態から
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;// Present状態へ
		_cmdList->ResourceBarrier(1, &BarrierDesc);
		//=============

		// Command List Execute
		_cmdList->Close();
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);
		_cmdList->Reset(_cmdAllocator, nullptr); 


		// Wait for GPU to finish
		_cmdQueue->Signal(_fence, ++_fenceVal);
		if (_fence->GetCompletedValue() != _fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}

		// Present (Display) Transition
		_swapchain->Present(1, 0);

	}
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
	//========================================
}
//=====================