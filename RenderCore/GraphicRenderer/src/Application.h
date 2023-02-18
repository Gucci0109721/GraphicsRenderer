#pragma once
#include <Windows.h>
#include <cstdint>
#include "Render/RenderCore.h"

class Application
{
public:
	Application(uint32_t width, uint32_t height);
	~Application();
	void Run();
private:
	HINSTANCE m_hInst;
	HWND m_hWnd;
	uint32_t m_Width;
	uint32_t m_Height;

	bool InitApp();
	void TermApp();
	bool InitWnd();
	void TermWnd();
	void MainLoop();

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

	RenderCore render;
};

