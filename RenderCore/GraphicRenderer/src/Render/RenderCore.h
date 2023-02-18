#pragma once
#include <Windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

class RenderCore
{
public:
	bool InitD3D(HWND hWnd);
	void TermD3D();
	void Render();

private:
	static const int frameCount = 2;

	HINSTANCE m_hInst;
	uint32_t m_Width;
	uint32_t m_Height;

	ID3D12Device* m_pDevice;
	ID3D12CommandQueue* m_pQueue;
	IDXGISwapChain3* m_pSwapChain;
	ID3D12Resource* m_pColorBuffer[frameCount];
	ID3D12CommandAllocator* m_pCmdAllocator[frameCount];
	ID3D12GraphicsCommandList* m_pCmdList;
	ID3D12DescriptorHeap* m_pHeapRTV;
	ID3D12Fence* m_pFence;
	HANDLE m_FenceEvent;
	uint64_t m_FenceCounter[frameCount];
	uint32_t m_FrameIndex;
	D3D12_CPU_DESCRIPTOR_HANDLE m_hRTV[frameCount];
	void Present(uint32_t interval);
	void WaitGPU();
};

