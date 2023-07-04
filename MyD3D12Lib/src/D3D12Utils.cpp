#include <MyD3D12Lib/D3D12Utils.h>
#include <MyD3D12Lib/Helpers.h>

// compile shader from file
ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const std::string& entrypoint,
	const std::string& target)
{
	ComPtr<ID3DBlob> shaderBlob;
	ComPtr<ID3DBlob> error;
	HRESULT hr = S_OK;
	UINT flags = 0;
#ifdef _DEBUG
	flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

	hr = D3DCompileFromFile(
		filename.c_str(),
		NULL,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(),
		target.c_str(),
		flags,
		0,
		&shaderBlob,
		&error
	);

	if (error != nullptr) {
		OutputDebugString((char*)error->GetBufferPointer());
		OutputDebugString("\n");
	}

	ThrowIfFailed(hr);

	return shaderBlob;
}

// DirectX 12 initialization functions
void EnableDebugLayer() {
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
#endif // DEBUG
}

bool CheckTearingSupport() {
	BOOL allowTearing = FALSE;
	ComPtr<IDXGIFactory4> factory4;
	ComPtr<IDXGIFactory5> factory5;

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory4)));
	ThrowIfFailed(factory4.As(&factory5));

	ThrowIfFailed(factory5->CheckFeatureSupport(
		DXGI_FEATURE_PRESENT_ALLOW_TEARING,
		&allowTearing, sizeof(allowTearing)
	));

#ifdef _DEBUG
	if (allowTearing == TRUE) {
		::OutputDebugStringW(L"Allow tearing true\n");
	}
	else {
		::OutputDebugStringW(L"Allow tearing false\n");
	}
#endif // _DEBUG

	return allowTearing == TRUE;
}

ComPtr<IDXGIAdapter4> CreateAdapter(bool useWarp) {
	ComPtr<IDXGIFactory4> factory;
	UINT factorFlags = 0;
#ifdef _DEBUG
	factorFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif // DEBUG

	ThrowIfFailed(CreateDXGIFactory2(factorFlags, IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter1> adapter1;
	ComPtr<IDXGIAdapter4> adapter4;

	if (useWarp) {
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter1)));
	}
	else {
		UINT i = 0;
		SIZE_T maxDedicatedMemory = 0;

		while (factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND) {
			DXGI_ADAPTER_DESC1 adapterDesc;
			adapter1->GetDesc1(&adapterDesc);

#ifdef _DEBUG
			OutputDebugStringW(adapterDesc.Description);
			OutputDebugStringW(L"\n");
#endif // DEBUG

			if ((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device2), NULL)) &&
				adapterDesc.DedicatedVideoMemory > maxDedicatedMemory)
			{
				ThrowIfFailed(adapter1.As(&adapter4));
				maxDedicatedMemory = adapterDesc.DedicatedVideoMemory;
			}

			++i;
		}
	}

	return adapter4;
}

ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter) {
	ComPtr<ID3D12Device2> device;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> infoQueue;
	ThrowIfFailed(device.As(&infoQueue));

	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

	//D3D12_MESSAGE_CATEGORY categories[] = {};

	//D3D12_MESSAGE_SEVERITY severities[] = {
	//	D3D12_MESSAGE_SEVERITY_INFO
	//};

	//D3D12_MESSAGE_ID denyIDs[]{
	//	D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
	//	D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
	//	D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
	//};

	//D3D12_INFO_QUEUE_FILTER filter{};
	//filter.DenyList.NumSeverities = _countof(severities);
	//filter.DenyList.pSeverityList = severities;
	//filter.DenyList.NumIDs = _countof(denyIDs);
	//filter.DenyList.pIDList = denyIDs;
	//
	//ThrowIfFailed(infoQueue->PushStorageFilter(&filter));
#endif // _DEBUG

	return device;
}

ComPtr<IDXGISwapChain4> CreateSwapChain(
	ComPtr<ID3D12CommandQueue> commandQueue,
	HWND windowHandle,
	uint32_t numBackBuffers,
	uint32_t width, uint32_t height,
	bool allowTearing)
{
	ComPtr<IDXGISwapChain1> swapChain1;
	ComPtr<IDXGISwapChain4> swapChain4;

	ComPtr<IDXGIFactory4> factory;
	UINT factorFlags = 0;
#ifdef _DEBUG
	factorFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif // DEBUG
	ThrowIfFailed(CreateDXGIFactory2(factorFlags, IID_PPV_ARGS(&factory)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;

	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = numBackBuffers;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		commandQueue.Get(),
		windowHandle,
		&swapChainDesc,
		NULL,
		NULL,
		&swapChain1
	));

	ThrowIfFailed(factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1.As(&swapChain4));

	return swapChain4;
}

ComPtr<ID3D12Resource> CreateDepthStencilBuffer(
	ComPtr<ID3D12Device2> device,
	uint32_t width, uint32_t height, 
	DXGI_FORMAT format, 
	float depthClearValue, 
	uint8_t stencilClearValue)
{
	ComPtr<ID3D12Resource> depthStencilBuffer;

	D3D12_RESOURCE_DESC dsBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		format,
		width, height,
		1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		0
	);

	D3D12_CLEAR_VALUE dsClearValue{};
	dsClearValue.Format = format;
	dsClearValue.DepthStencil = { depthClearValue, stencilClearValue };

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&dsBufferDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&dsClearValue,
		IID_PPV_ARGS(&depthStencilBuffer)
	));

	return depthStencilBuffer;
}

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
	ComPtr<ID3D12Device2> device,
	UINT numDescriptors,
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC descriptroHeapDesc;
	descriptroHeapDesc.Type = type;
	descriptroHeapDesc.NumDescriptors = numDescriptors;
	descriptroHeapDesc.Flags = flags;
	descriptroHeapDesc.NodeMask = 0;

	ThrowIfFailed(device->CreateDescriptorHeap(&descriptroHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

ComPtr<ID3D12Resource> CreateGPUResourceAndLoadData(
	ComPtr<ID3D12Device2> device,
	ComPtr<ID3D12GraphicsCommandList> commandList,
	ComPtr<ID3D12Resource>& intermediateResource,
	const void* pData,
	size_t dataSize)
{
	ComPtr<ID3D12Resource> destinationResource;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(dataSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		NULL,
		IID_PPV_ARGS(&destinationResource)
	));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(dataSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		IID_PPV_ARGS(&intermediateResource)
	));

	D3D12_SUBRESOURCE_DATA subresourceData;
	subresourceData.pData = pData;
	subresourceData.RowPitch = dataSize;
	subresourceData.SlicePitch = dataSize;

	UpdateSubresources(
		commandList.Get(),
		destinationResource.Get(),
		intermediateResource.Get(),
		0, 0, 1, &subresourceData
	);

	return destinationResource;
}