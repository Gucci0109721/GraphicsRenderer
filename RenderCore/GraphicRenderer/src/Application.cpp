#include "Application.h"

#define CLASSNAME  L"Renderer"

Application::Application(uint32_t width, uint32_t height)
	:m_hInst(nullptr)
	, m_hWnd(nullptr)
	, m_Width(width)
	, m_Height(height)
{
	// nothing
}

Application::~Application()
{
	// nothing
}

void Application::Run()
{
	if (InitApp())
	{
		MainLoop();
	}

	TermApp();
}

bool Application::InitApp()
{
	// Initialize window
	if (!InitWnd()) return false;

	if (!render.InitD3D(m_hWnd))	return false;

	// Succesed
	return true;
}

void Application::TermApp()
{
	render.TermD3D();
	TermWnd();
}

bool Application::InitWnd()
{
	// Get instance handle
	HINSTANCE hInst = GetModuleHandle(nullptr);
	if (hInst == nullptr)	return false;

	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = CLASSNAME;
	wc.hIconSm = LoadIcon(hInst, IDI_APPLICATION);

	// Register window
	if (!RegisterClassEx(&wc))	return false;

	m_hInst = hInst;

	RECT rc = {};
	rc.right = static_cast<long>(m_Width);
	rc.bottom = static_cast<long>(m_Height);

	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rc, style, false);

	// Create window
	m_hWnd = CreateWindowEx(
		0,
		CLASSNAME,
		TEXT("Renderer"),
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		m_hInst,
		nullptr);

	if (m_hWnd == nullptr)	return false;

	ShowWindow(m_hWnd, SW_SHOWNORMAL);

	UpdateWindow(m_hWnd);

	SetFocus(m_hWnd);

	return true;
}

void Application::TermWnd()
{
	if (m_hInst != nullptr)
	{
		UnregisterClass(CLASSNAME, m_hInst);
	}

	m_hInst = nullptr;
	m_hWnd = nullptr;
}

void Application::MainLoop()
{
	MSG msg = {};
	while (WM_QUIT!=msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == true) 
		{
			render.Render();

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

// Window procedure
LRESULT Application::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:

		PostQuitMessage(0);
		break;

	default:
		break;
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}
