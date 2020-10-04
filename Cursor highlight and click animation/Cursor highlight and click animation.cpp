#include <windows.h>
#include <iostream>
#include <objidl.h>
#include <gdiplus.h>
#include <shellapi.h>

#include "resource.h"

//gdiplus
#pragma comment (lib,"Gdiplus.lib")
using namespace Gdiplus;

#define CIRCLE_WIDTH 70
#define CIRCLE_HEIGHT 70

//钩子
typedef BOOL(*INSTLLHOOK)(HWND);
typedef BOOL(*UNINSTALLHOOK)();

INSTLLHOOK IH;
UNINSTALLHOOK UIH;
BOOL bIH;

//
HINSTANCE g_hInstance;

//动画
BOOL bAnimation;

//异形窗口回调函数
LRESULT __stdcall WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//托盘
#define WM_TRAY WM_USER + 100
#define IDM_SHOW WM_USER + 101
#define IDM_EXIT WM_USER + 102
#define WM_TASKBAR_CREATED RegisterWindowMessage(TEXT("TaskbarCreated"))
NOTIFYICONDATA nid;		//托盘属性
HMENU hMenu;			//托盘菜单
LPCTSTR szWndName = TEXT("鼠标点击动画");
LPCTSTR szClassName = TEXT("鼠标点击动画");
void InitTray(HINSTANCE hInstance, HWND hWnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int nCmdShow)
{
	// TODO: 在此处放置代码。
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;

	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	WNDCLASS wc = { 0 };
	wc.lpszClassName = szClassName;
	wc.hbrBackground = CreateSolidBrush(RGB(255, 99, 71));
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpfnWndProc = WinProc;
	RegisterClass(&wc);

	HWND hWnd = CreateWindowExW(
		WS_EX_LAYERED,
		szClassName,
		szWndName,
		WS_POPUP | WS_VISIBLE | WS_EX_TOPMOST,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CIRCLE_WIDTH,
		CIRCLE_HEIGHT,
		0,
		0,
		hInstance,
		0);

	POINT CursorPos;
	GetCursorPos(&CursorPos);
	MoveWindow(hWnd, CursorPos.x - CIRCLE_WIDTH / 2, CursorPos.y - CIRCLE_HEIGHT / 2, CIRCLE_WIDTH, CIRCLE_HEIGHT, FALSE);

	if (hWnd == NULL)
		return 1;

	SetLayeredWindowAttributes(hWnd, NULL, 100, LWA_ALPHA);		//设置透明度

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	InitTray(hInstance, hWnd); //实例化托盘

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam;
}

LRESULT _stdcall WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_USER:
		{
			if (bAnimation == FALSE)
			{
				POINT CursorPos;
				GetCursorPos(&CursorPos);
				SetWindowPos(hWnd, HWND_TOPMOST, CursorPos.x - CIRCLE_WIDTH / 2, CursorPos.y - CIRCLE_HEIGHT / 2,
					CIRCLE_WIDTH, CIRCLE_HEIGHT, SWP_SHOWWINDOW);
			}
			break;
		}
		case WM_USER + 1:			//鼠标左键点击
		{
			bAnimation = TRUE;
			HDC hDC = GetDC(hWnd);
			Graphics graphics(hDC);
			Pen redPen(Color(255, 255, 0, 0), 3);
			for (int i = 1; i < CIRCLE_WIDTH; i += 4)
			{
				graphics.DrawEllipse(&redPen, CIRCLE_WIDTH / 2 - i / 2, CIRCLE_HEIGHT / 2 - i / 2, i, i);
				Sleep(10);
				graphics.Clear(Color(255, 255, 255, 0));
			}
			ReleaseDC(hWnd, hDC);
			bAnimation = FALSE;
			break;
		}
		case WM_USER + 2:			//鼠标右键点击
		{
			bAnimation = TRUE;
			HDC hDC = GetDC(hWnd);
			Graphics graphics(hDC);
			Pen bluePen(Color(255, 0, 0, 255), 3);
			for (int i = 1; i < CIRCLE_WIDTH; i += 4)
			{
				graphics.DrawEllipse(&bluePen, CIRCLE_WIDTH / 2 - i / 2, CIRCLE_HEIGHT / 2 - i / 2, i, i);
				Sleep(10);
				graphics.Clear(Color(255, 255, 255, 0));
			}
			ReleaseDC(hWnd, hDC);
			bAnimation = FALSE;
			break;
		}
		case WM_ERASEBKGND:
		{
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT paintStruct;
			HDC hDC = BeginPaint(hWnd, &paintStruct);
			HRGN hRgn = CreateEllipticRgn(0, 0, CIRCLE_WIDTH, CIRCLE_HEIGHT);
			FillRgn(hDC, hRgn, CreateSolidBrush(RGB(255, 255, 0))); // 填充颜色
			EndPaint(hWnd, &paintStruct);
			DeleteObject(hRgn);
			break;
		}
		case WM_CREATE:
		{
			bAnimation = FALSE;
			HRGN hRgn = CreateEllipticRgn(0, 0, CIRCLE_WIDTH, CIRCLE_HEIGHT);
			SetWindowRgn(hWnd, hRgn, TRUE);
			SetWindowLong(hWnd, -20, (int)GetWindowLong(hWnd, -20) | WS_EX_TRANSPARENT);
			DeleteObject(hRgn);
			//安装钩子
			g_hInstance = LoadLibrary(TEXT("MouseKeyboardDll.dll"));
			if (g_hInstance != NULL)
			{
				IH = (INSTLLHOOK)GetProcAddress(g_hInstance, "InstallHook");
				bIH = IH(hWnd);
				UIH = (UNINSTALLHOOK)GetProcAddress(g_hInstance, "UninstallHook");
			}
			else
			{
				MessageBox(NULL, L"dll获取失败", L"dll获取", MB_OK);
			}
			break;
		}
		case WM_LBUTTONDOWN:
		{
			SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
			break;
		}
		case WM_TRAY:
		{
			switch (lParam)
			{
				case WM_RBUTTONDOWN:
				{
					POINT point;
					GetCursorPos(&point);
					SetForegroundWindow(hWnd);

					int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, point.x, point.y, NULL, hWnd, NULL); //显示菜单并获取选项ID

					if (cmd == IDM_SHOW)
					{
						MessageBox(NULL, TEXT("鼠标高亮和点击说明"), TEXT("鼠标高亮和点击"), MB_OK);
					}

					if (cmd == IDM_EXIT)
						PostMessage(hWnd, WM_DESTROY, NULL, NULL);
				}
			}
			break;
		}
		case WM_DESTROY:
		{
			UIH();
			FreeLibrary(g_hInstance);
			PostQuitMessage(0);
			break;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);;
}

void InitTray(HINSTANCE hInstance, HWND hWnd)
{
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = IDI_ICON1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	nid.uCallbackMessage = WM_TRAY;
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	//lstrcpy(nid.szTip, APP_NAME);

	hMenu = CreatePopupMenu();//生成托盘菜单
	//为托盘菜单添加两个选项
	AppendMenu(hMenu, MF_STRING, IDM_SHOW, TEXT("提示"));
	AppendMenu(hMenu, MF_STRING, IDM_EXIT, TEXT("退出"));

	Shell_NotifyIcon(NIM_ADD, &nid);
}