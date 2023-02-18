#include "RenderCore.h"
#include <assert.h>

template<class T>
void SafeRelease(T*& ptr)
{
	ptr->Release();
	ptr = nullptr;
}

bool RenderCore::InitD3D(HWND hWnd)
{
	// Create device
	HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice));
	if (FAILED(hr)) return false;

	// Create command queue
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		hr = m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_pQueue));

		if (FAILED(hr))  return false;
	}

	// Create swap chain
	{
		IDXGIFactory4* pFactory = nullptr;
		hr = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));

		if (FAILED(hr))  return false;

		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferDesc.Width = m_Width;
		desc.BufferDesc.Height = m_Height;
		desc.BufferDesc.RefreshRate.Numerator = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = frameCount;
		desc.OutputWindow = hWnd;
		desc.Windowed = true;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		IDXGISwapChain* pSwapChain = nullptr;
		hr = pFactory->CreateSwapChain(m_pQueue, &desc, &pSwapChain);
		if (FAILED(hr))
		{
			SafeRelease(pFactory);

			return false;
		}

		hr = pSwapChain->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
		if (FAILED(hr))
		{
			SafeRelease(pFactory);

			return false;
		}

		m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		SafeRelease(pFactory);

		SafeRelease(pSwapChain);
	}

	// Crate command allocator
	for (int idx = 0; idx < frameCount; ++idx)
	{
		hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCmdAllocator[idx]));
		if (FAILED(hr))  return false;
	}

	// Create command list
	{
		hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,m_pCmdAllocator[m_FrameIndex], nullptr, IID_PPV_ARGS(&m_pCmdList));
		if(FAILED(hr)) return false;
	}

	// Craete render target view
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.NumDescriptors = frameCount;
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pHeapRTV));
		if (FAILED(hr))	return false;

		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pHeapRTV->GetCPUDescriptorHandleForHeapStart();
		UINT incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (int idx = 0; idx < frameCount; ++idx)
		{
			hr = m_pSwapChain->GetBuffer(idx, IID_PPV_ARGS(&m_pColorBuffer[idx]));
			if (FAILED(hr))	return false;

			D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
			viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = 0;
			viewDesc.Texture2D.PlaneSlice = 0;

			m_pDevice->CreateRenderTargetView(m_pColorBuffer[idx], &viewDesc, handle);
			m_hRTV[idx] = handle;
			handle.ptr += incrementSize;
		}
	}

	// Create fence
	{
		for (int idx = 0; idx < frameCount; ++idx)
		{
			m_FenceCounter[idx] = 0;
		}

		hr = m_pDevice->CreateFence(m_FenceCounter[m_FrameIndex],
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&m_pFence));
		if (FAILED(hr))	return false;

		m_FenceCounter[m_FrameIndex]++;

		m_FenceEvent = CreateEvent(nullptr, false, false, nullptr);
		if (m_FenceEvent == nullptr)	return false;
	}

	m_pCmdList->Close();

	return true;
}

void RenderCore::TermD3D()
{
	WaitGPU();

	if (m_FenceEvent != nullptr)
	{
		CloseHandle(m_FenceEvent);
		m_FenceEvent = nullptr;
	}

	SafeRelease(m_pFence);

	SafeRelease(m_pHeapRTV);

	for (auto idx = 0; idx < frameCount; ++idx)
	{
		SafeRelease(m_pColorBuffer[idx]);
	}

	SafeRelease(m_pCmdList);

	for (int idx = 0; idx < frameCount; ++idx)
	{
		SafeRelease(m_pCmdAllocator[idx]);
	}

	SafeRelease(m_pSwapChain);

	SafeRelease(m_pQueue);

	SafeRelease(m_pDevice);
}

void RenderCore::Render()
{
	m_pCmdAllocator[m_FrameIndex]->Reset();
	m_pCmdList->Reset(m_pCmdAllocator[m_FrameIndex], nullptr);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pColorBuffer[m_FrameIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_pCmdList->ResourceBarrier(1, &barrier);

	m_pCmdList->OMSetRenderTargets(1, &m_hRTV[m_FrameIndex], false, nullptr);

	float clearColor[] = { 0.25f,0.25f ,0.25f ,1.0f };
	m_pCmdList->ClearRenderTargetView(m_hRTV[m_FrameIndex], clearColor, 0, nullptr);

	{

	}

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pColorBuffer[m_FrameIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_pCmdList->ResourceBarrier(1, &barrier);

	m_pCmdList->Close();

	ID3D12CommandList* ppCmdList[] = { m_pCmdList };
	m_pQueue->ExecuteCommandLists(1, ppCmdList);

	Present(1);
}

void RenderCore::WaitGPU()
{
	assert(m_pQueue != nullptr);
	assert(m_pFence != nullptr);
	assert(m_FenceEvent != nullptr);

	m_pQueue->Signal(m_pFence, m_FenceCounter[m_FrameIndex]);

	m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);

	WaitForSingleObjectEx(m_FenceEvent, INFINITE, false);

	m_FenceCounter[m_FrameIndex]++;
}

void RenderCore::Present(uint32_t interval)
{
	m_pSwapChain->Present(interval, 0);

	const auto currentValue = m_FenceCounter[m_FrameIndex];
	m_pQueue->Signal(m_pFence, currentValue);

	m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	if (m_pFence->GetCompletedValue() < m_FenceCounter[m_FrameIndex])
	{
		m_pFence->SetEventOnCompletion(m_FenceCounter[m_FrameIndex], m_FenceEvent);
		WaitForSingleObjectEx(m_FenceEvent, INFINITE, false);
	}
	m_FenceCounter[m_FrameIndex] = currentValue + 1;
}
