int g_windowWidth = 500;
int g_windowHeight = 400;
int g_fontSize = 32;

#include "resource.h"
#include <windows.h>
#include <tchar.h>
#include <cstdlib>
#include <ctime>

#define TIMER_ID 1
#define TOTAL_ROUNDS 5
#define MIN_DELAY 500
#define MAX_DELAY 2000
HINSTANCE g_hInst;
enum AppState
{
    STATE_WAITING_START,
    STATE_WAITING_RED,
    STATE_RED,
    STATE_TOO_EARLY,
    STATE_FINISHED
};
AppState g_state = STATE_WAITING_START;
int g_round = 0;
int g_reactionTimes[TOTAL_ROUNDS] = { 0 };
DWORD g_startTick = 0;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawApp(HDC hdc, HWND hwnd);
COLORREF GetBackgroundColor();
LPCTSTR GetDisplayText();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    g_hInst = hInstance;
    srand((unsigned)time(NULL));

    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = _T("ReactionTestWindow");
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, _T("窗口类注册失败！"), _T("错误"), MB_ICONERROR);
        return 1;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    if (screenHeight < 720)
    {
        g_windowWidth = 400;
        g_windowHeight = 300;
        g_fontSize = 18;
    }
    else if (screenHeight < 1080)
    {
        g_windowWidth = 640;
        g_windowHeight = 480;
        g_fontSize = 25;
    }
    else if (screenHeight < 1440)
    {
        g_windowWidth = 800;
        g_windowHeight = 600;
        g_fontSize = 32;
    }
    else if (screenHeight < 2160)
    {
        g_windowWidth = 1000;
        g_windowHeight = 750;
        g_fontSize = 40;
    }
    else
    {
        g_windowWidth = 1600;
        g_windowHeight = 1200;
        g_fontSize = 64;
    }

    HWND hWnd = CreateWindow(
        _T("ReactionTestWindow"),
        _T("MRG 反应测试"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        g_windowWidth, g_windowHeight,
        NULL, NULL, hInstance, NULL);
    if (!hWnd)
    {
        MessageBox(NULL, _T("窗口创建失败！"), _T("错误"), MB_ICONERROR);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            DrawApp(hdc, hWnd);
            EndPaint(hWnd, &ps);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            if (g_state == STATE_WAITING_START)
            {
                g_state = STATE_WAITING_RED;
                int delay = rand() % (MAX_DELAY - MIN_DELAY + 1) + MIN_DELAY;
                SetTimer(hWnd, TIMER_ID, delay, NULL);
                InvalidateRect(hWnd, NULL, TRUE);
            }
            else if (g_state == STATE_RED)
            {
                DWORD elapsed = GetTickCount() - g_startTick;
                g_reactionTimes[g_round] = (int)elapsed;
                g_round++;

                if (g_round >= TOTAL_ROUNDS)
                {
                    g_state = STATE_FINISHED;
                }
                else
                {
                    g_state = STATE_WAITING_START;
                }
                InvalidateRect(hWnd, NULL, TRUE);
            }
            else if (g_state == STATE_WAITING_RED)
            {
                KillTimer(hWnd, TIMER_ID);
                g_state = STATE_TOO_EARLY;
                SetTimer(hWnd, TIMER_ID + 1, 1000, NULL);
                InvalidateRect(hWnd, NULL, TRUE);
            }
            else if (g_state == STATE_FINISHED)
            {
                g_round = 0;
                for (int i = 0; i < TOTAL_ROUNDS; i++)
                {
                    g_reactionTimes[i] = 0;
                }
                g_state = STATE_WAITING_START;
                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
        }
        case WM_TIMER:
        {
            if (wParam == TIMER_ID && g_state == STATE_WAITING_RED)
            {
                KillTimer(hWnd, TIMER_ID);
                g_state = STATE_RED;
                g_startTick = GetTickCount();
                InvalidateRect(hWnd, NULL, TRUE);
            }
            else if (wParam == TIMER_ID + 1 && g_state == STATE_TOO_EARLY)
            {
                KillTimer(hWnd, TIMER_ID + 1);

                g_state = STATE_WAITING_RED;

                int delay = rand() % (MAX_DELAY - MIN_DELAY + 1) + MIN_DELAY;
                SetTimer(hWnd, TIMER_ID, delay, NULL);

                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

COLORREF GetBackgroundColor()
{
    switch (g_state)
    {
        case STATE_WAITING_START: return RGB(0, 0, 255);
        case STATE_WAITING_RED: return RGB(0, 255, 0);
        case STATE_RED: return RGB(255, 0, 0);
        case STATE_TOO_EARLY: return RGB(255, 255, 0);
        case STATE_FINISHED: return RGB(0, 0, 255);
        default: return RGB(255, 255, 255);
    }
}

LPCTSTR GetDisplayText()
{
    static TCHAR buffer[128];
    switch (g_state)
    {
        case STATE_WAITING_START:
            if (g_round > 0)
            {
                int lastIndex = g_round - 1;
                _stprintf_s(buffer, _T("本次成绩: %d ms\n点击开始"), g_reactionTimes[lastIndex]);
            }
            else
            {
                // 尚未开始任何测试（初始状态）
                _tcscpy_s(buffer, _T("点击开始"));
            }
            break;
        case STATE_WAITING_RED:
            _tcscpy_s(buffer, _T("请等待变为红色"));
            break;
        case STATE_RED:
            _tcscpy_s(buffer, _T("点击！"));
            break;
        case STATE_TOO_EARLY:
            _tcscpy_s(buffer, _T("不要提前点击！"));
            break;
        case STATE_FINISHED:
        {
            int sum = 0;
            for (int i = 0; i < TOTAL_ROUNDS; i++)
            {
                sum += g_reactionTimes[i];
            }
            int avg = sum / TOTAL_ROUNDS;
            _stprintf_s(buffer, _T("平均反应时间: %d ms\n单击以重新开始"), avg);
            break;
        }
        default:
            buffer[0] = _T('\0');
    }
    return buffer;
}

void DrawApp(HDC hdc, HWND hwnd)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    HBRUSH hBrush = CreateSolidBrush(GetBackgroundColor());
    FillRect(hdc, &clientRect, hBrush);
    DeleteObject(hBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));

    HFONT hFont = CreateFont(
        -g_fontSize,
        0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("微软雅黑"));
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    LPCTSTR text = GetDisplayText();

    RECT textRect = clientRect;
    DrawText(hdc, text, -1, &textRect, DT_CENTER | DT_CALCRECT);
    int textHeight = textRect.bottom - textRect.top;
    int drawTop = (clientRect.bottom - textHeight) / 2;
    RECT drawRect = { clientRect.left, drawTop, clientRect.right, drawTop + textHeight };

    DrawText(hdc, text, -1, &drawRect, DT_CENTER | DT_NOPREFIX);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}